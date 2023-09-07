/**
  ******************************************************************************
  * @file    at_custom_modem_sid.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          BG96 Quectel modem: LTE-cat-M1 or LTE-cat.NB1(=NB-IOT) or GSM
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* AT commands format
 * AT+<X>=?    : TEST COMMAND
 * AT+<X>?     : READ COMMAND
 * AT+<X>=...  : WRITE COMMAND
 * AT+<X>      : EXECUTION COMMAND
*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_custom_modem_sid.h"
#include "at_custom_modem_specific.h"
#include "at_custom_modem_signalling.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_custom_sysctrl.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#include "at_modem_socket.h"
#include "at_custom_modem_socket.h"
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @_
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SID AT_CUSTOM QUECTEL_BG96 SID
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SID_Private_Macros AT_CUSTOM QUECTEL_BG96 SID Private Macros
  * @{
  */

#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "BG96:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "BG96:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "BG96 API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "BG96 ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...)  (void) printf("BG96:" format "\n\r", ## args);
#define PRINT_DBG(...) __NOP(); /* Nothing to do */
#define PRINT_API(...) __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("BG96 ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...) __NOP(); /* Nothing to do */
#define PRINT_DBG(...)  __NOP(); /* Nothing to do */
#define PRINT_API(...)  __NOP(); /* Nothing to do */
#define PRINT_ERR(...)  __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
/* add a pre-step */
#define CHECK_STEP(stepval) ((p_atp_ctxt->step) == ((stepval)+1U))
#define CHECK_STEP_EXCEEDS(stepval) (p_atp_ctxt->step >= ((stepval)+1U))
#define CHECK_STEP_BETWEEN(low_step, high_step) ((p_atp_ctxt->step >= ((low_step)+1U)) &&\
                                                 (p_atp_ctxt->step <= ((high_step)+1U)))
#else
#define CHECK_STEP(stepval) (p_atp_ctxt->step == stepval)
#define CHECK_STEP_EXCEEDS(stepval) (p_atp_ctxt->step >= stepval)
#define CHECK_STEP_BETWEEN(low_step, high_step) ((p_atp_ctxt->step >= low_step) && (p_atp_ctxt->step <= high_step))
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SID_Private_Functions_Prototypes
  *    AT_CUSTOM QUECTEL_BG96 SID Private Functions Prototypes
  * @{
  */
static at_status_t execute_SID_Func(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                    atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_CHECK_CNX(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                       atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_MODEM_CONFIG(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_POWER_ON(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                      atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_POWER_OFF(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                       atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_INIT_MODEM(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_GET_DEVICE_INFO(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                             atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_GET_SIGNAL_QUALITY(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_GET_ATTACHSTATUS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                              atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_REGISTER_NET(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_GET_NETSTATUS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SUSBCRIBE_NET_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                 atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_UNSUSBCRIBE_NET_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                   atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_REGISTER_PDN_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DEREGISTER_PDN_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                  atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_ATTACH_PS_DOMAIN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_DETACH_PS_DOMAIN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_ACTIVATE_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DEACTIVATE_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                            atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DEFINE_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SET_DEFAULT_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                             atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_GET_IP_ADDRESS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                            atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DATA_SUSPEND(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DATA_RESUME(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                         atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SUSBCRIBE_MODEM_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                   atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DIRECT_CMD(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SIM_SELECT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SIM_GENERIC_ACCESS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
static at_status_t at_SID_CS_DIAL_COMMAND(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SEND_DATA(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                       atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_RECEIVE_DATA(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SOCKET_CLOSE(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SOCKET_CNX_STATUS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                               atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_PING_IP_ADDRESS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                             atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_DNS_REQ(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                     atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static void init_bg96_qiurc_dnsgip(void);
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
static at_status_t at_SID_CS_INIT_POWER_CONFIG(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                               atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SET_POWER_CONFIG(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                              atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SLEEP_REQUEST(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SLEEP_COMPLETE(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                            atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_SLEEP_CANCEL(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
static at_status_t at_SID_CS_WAKEUP(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                    atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout);
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SID_Exported_Functions AT_CUSTOM QUECTEL_BG96 SID Exported Functions
  * @{
  */

/**
  * @brief  Get next AT command for current Service ID (SID).
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
at_status_t at_custom_SID_bg96(atcustom_modem_context_t *p_mdm_ctxt,
                               at_context_t *p_at_ctxt,
                               uint32_t *p_ATcmdTimeout)
{
  at_status_t retval;
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);

  retval = execute_SID_Func(p_mdm_ctxt, p_at_ctxt, p_atp_ctxt, p_ATcmdTimeout);

  return (retval);
}

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SID_Private_Functions AT_CUSTOM QUECTEL_BG96 SID Private Functions
  * @{
  */

/**
  * @brief  Search in LookUp table the SID Function corresponding to given SID.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t execute_SID_Func(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                    atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  /* Commands Look-up table */
  static const sid_LUT_t SID_BG96_LUT[] =
  {
    /* SID, SID_FUNCTION*/
    {SID_CS_CHECK_CNX, at_SID_CS_CHECK_CNX},
    {SID_CS_MODEM_CONFIG, at_SID_CS_MODEM_CONFIG},
    {SID_CS_RESET, at_SID_CS_POWER_ON}, /* SID_CS_RESET is very similar to SID_CS_POWER_ON */
    {SID_CS_POWER_ON, at_SID_CS_POWER_ON},
    {SID_CS_POWER_OFF, at_SID_CS_POWER_OFF},
    {SID_CS_INIT_MODEM, at_SID_CS_INIT_MODEM},
    {SID_CS_GET_DEVICE_INFO, at_SID_CS_GET_DEVICE_INFO},
    {SID_CS_GET_SIGNAL_QUALITY, at_SID_CS_GET_SIGNAL_QUALITY},
    {SID_CS_GET_ATTACHSTATUS, at_SID_CS_GET_ATTACHSTATUS},
    {SID_CS_REGISTER_NET, at_SID_CS_REGISTER_NET},
    {SID_CS_GET_NETSTATUS, at_SID_CS_GET_NETSTATUS},
    {SID_CS_SUSBCRIBE_NET_EVENT, at_SID_CS_SUSBCRIBE_NET_EVENT},
    {SID_CS_UNSUSBCRIBE_NET_EVENT, at_SID_CS_UNSUSBCRIBE_NET_EVENT},
    {SID_CS_REGISTER_PDN_EVENT, at_SID_CS_REGISTER_PDN_EVENT},
    {SID_CS_DEREGISTER_PDN_EVENT, at_SID_CS_DEREGISTER_PDN_EVENT},
    {SID_ATTACH_PS_DOMAIN, at_SID_ATTACH_PS_DOMAIN},
    {SID_DETACH_PS_DOMAIN, at_SID_DETACH_PS_DOMAIN},
    {SID_CS_ACTIVATE_PDN, at_SID_CS_ACTIVATE_PDN},
    {SID_CS_DEACTIVATE_PDN, at_SID_CS_DEACTIVATE_PDN},
    {SID_CS_DEFINE_PDN, at_SID_CS_DEFINE_PDN},
    {SID_CS_SET_DEFAULT_PDN, at_SID_CS_SET_DEFAULT_PDN},
    {SID_CS_GET_IP_ADDRESS, at_SID_CS_GET_IP_ADDRESS},
    {SID_CS_DATA_SUSPEND, at_SID_CS_DATA_SUSPEND},
    {SID_CS_DATA_RESUME, at_SID_CS_DATA_RESUME},
    {SID_CS_SUSBCRIBE_MODEM_EVENT, at_SID_CS_SUSBCRIBE_MODEM_EVENT},
    {SID_CS_DIRECT_CMD, at_SID_CS_DIRECT_CMD},
    {SID_CS_SIM_SELECT, at_SID_CS_SIM_SELECT},
    {SID_CS_SIM_GENERIC_ACCESS, at_SID_CS_SIM_GENERIC_ACCESS},

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    {SID_CS_DIAL_COMMAND, at_SID_CS_DIAL_COMMAND},
    {SID_CS_SEND_DATA, at_SID_CS_SEND_DATA},
    {SID_CS_RECEIVE_DATA, at_SID_CS_RECEIVE_DATA},
    {SID_CS_RECEIVE_DATA_FROM, at_SID_CS_RECEIVE_DATA}, /* SID_CS_RECEIVE_DATA_FROM managed as SID_CS_RECEIVE_DATA */
    {SID_CS_SOCKET_CLOSE, at_SID_CS_SOCKET_CLOSE},
    {SID_CS_SOCKET_CNX_STATUS, at_SID_CS_SOCKET_CNX_STATUS},
    {SID_CS_PING_IP_ADDRESS, at_SID_CS_PING_IP_ADDRESS},
    {SID_CS_DNS_REQ, at_SID_CS_DNS_REQ},
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
    /**
      * LOW POWER SIDs
      */
    {SID_CS_INIT_POWER_CONFIG, at_SID_CS_INIT_POWER_CONFIG},
    {SID_CS_SET_POWER_CONFIG, at_SID_CS_SET_POWER_CONFIG},
    {SID_CS_SLEEP_REQUEST, at_SID_CS_SLEEP_REQUEST},
    {SID_CS_SLEEP_COMPLETE, at_SID_CS_SLEEP_COMPLETE},
    {SID_CS_SLEEP_CANCEL, at_SID_CS_SLEEP_CANCEL},
    {SID_CS_WAKEUP, at_SID_CS_WAKEUP},
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */
  };
#define SIZE_SID_BG96_LUT ((uint16_t) (sizeof (SID_BG96_LUT) / sizeof (sid_LUT_t)))

  at_status_t retval = ATSTATUS_ERROR;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  uint16_t index = 0U;
  bool leave_loop = false;

  /* search SID in the LUT */
  do
  {
    if (SID_BG96_LUT[index].sid == curSID)
    {
      /* execute the function corresponding to the given SID */
      retval = SID_BG96_LUT[index].sid_Function(p_mdm_ctxt, p_at_ctxt, p_atp_ctxt, p_ATcmdTimeout);
      leave_loop = true;
    }
    index++;
  } while ((leave_loop == false) && (index < SIZE_SID_BG96_LUT));

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_CHECK_CNX.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_CHECK_CNX(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                       atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_MODEM_CONFIG.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_MODEM_CONFIG(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_POWER_ON and SID_CS_POWER_RESET.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_POWER_ON(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                      atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  uint8_t common_start_sequence_step = BG96_MODEM_SYNCHRO_AT_MAX_RETRIES + 1U;
  /* POWER_ON and RESET are almost the same, specific differences are managed case by case */
  /* for reset, only HW reset is supported */
  if ((curSID == (at_msg_t) SID_CS_RESET) &&
      (p_mdm_ctxt->SID_ctxt.reset_type != CS_RESET_HW))
  {
    PRINT_ERR("Reset type (%d) not supported", p_mdm_ctxt->SID_ctxt.reset_type)
    retval = ATSTATUS_ERROR;
  }
  else
  {
    /****************************************************************************
      * POWER_ON and RESET first steps
      * try to establish the communiction with the modem by sending "AT" commands
      ****************************************************************************/
    if CHECK_STEP((0U))
    {
      /* reset modem specific variables */
      ATC_BG96_reset_variables();

      /* reinit modem at ready status */
      p_mdm_ctxt->persist.modem_at_ready = AT_FALSE;

      /* in case of RESET, reset all the contexts to start from a fresh state */
      if (curSID == (at_msg_t) SID_CS_RESET)
      {
        ATC_BG96_modem_reset(p_mdm_ctxt);
      }

      /* force requested flow control
        * use optional as we are not sure to receive a response from the modem if modem Flow Control is not aligned.
        */
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt,
                                          ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
    }
    else if (CHECK_STEP_BETWEEN((1U), (BG96_MODEM_SYNCHRO_AT_MAX_RETRIES - 1U)))
    {
      /* start a loop to wait for modem : send AT commands */
      if (p_mdm_ctxt->persist.modem_at_ready == AT_FALSE)
      {
        /* use optional as we are not sure to receive a response from the modem: this allows to avoid to return
          * an error to upper layer
          */
        PRINT_DBG("test connection [try number %d] ", p_atp_ctxt->step)
        atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt,
                                            ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, INTERMEDIATE_CMD);
      }
      else
      {
        /* modem has answered to the command AT: it is ready */
        PRINT_INFO("modem synchro established, proceed to normal power sequence")

        /* go to next step: jump to POWER ON sequence step */
        p_atp_ctxt->step = common_start_sequence_step;
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    else if CHECK_STEP((BG96_MODEM_SYNCHRO_AT_MAX_RETRIES))
    {
      /* if we fall here and the modem is not ready, we have a communication problem */
      if (p_mdm_ctxt->persist.modem_at_ready == AT_FALSE)
      {
        /* error, impossible to synchronize with modem */
        PRINT_ERR("Impossible to sync with modem")
        retval = ATSTATUS_ERROR;
      }
      else
      {
        /* continue the boot sequence */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    /********************************************************************
      * common power ON/RESET sequence starts here
      * when communication with modem has been successfully established
      ********************************************************************/
    else if CHECK_STEP((common_start_sequence_step))
    {
      /* force flow control */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 1U))
    {
      /* disable echo */
      p_mdm_ctxt->CMD_ctxt.command_echo = AT_FALSE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATE, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 2U))
    {
      /* request detailed error report */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CMEE, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 3U))
    {
      /* enable full response format */
      p_mdm_ctxt->CMD_ctxt.dce_full_resp_format = AT_TRUE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATV, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 4U))
    {
      /* deactivate DTR */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_AND_D, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 5U))
    {
      /* Read FW revision */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 6U))
    {
      /* power on with AT+CFUN=0 */
      p_mdm_ctxt->CMD_ctxt.cfun_value = 0U;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 7U))
    {
      /* force to disable PSM in case modem was switched off with PSM enabled */
      p_mdm_ctxt->SID_ctxt.set_power_config.psm_present = CS_TRUE;
      p_mdm_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
    }
    /* ----- start specific power ON sequence here ----
      * BG96_AT_Commands_Manual_V2.0
      */
    /* Check bands parameters */
    else if CHECK_STEP((common_start_sequence_step + 8U))
    {
      bg96_shared.QCFG_command_write = AT_FALSE;
      bg96_shared.QCFG_command_param = QCFG_band;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 9U))
    {
      bg96_shared.QCFG_command_write = AT_FALSE;
      bg96_shared.QCFG_command_param = QCFG_iotopmode;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 10U))
    {
      bg96_shared.QCFG_command_write = AT_FALSE;
      bg96_shared.QCFG_command_param = QCFG_nwscanseq;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 11U))
    {
      bg96_shared.QCFG_command_write = AT_FALSE;
      bg96_shared.QCFG_command_param = QCFG_nwscanmode;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, FINAL_CMD);
    }
    else if CHECK_STEP_EXCEEDS((common_start_sequence_step + 12U))
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* ignore */
    }
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_POWER_OFF.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_POWER_OFF(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                       atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* since Quectel BG96 Hardware Design V1.4
      * Power Off sequence is done using GPIO
      * The write command CMD_AT_QPOWD is no more used.
      */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_INIT_MODEM.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_INIT_MODEM(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* CFUN parameters here are coming from user settings in CS_init_modem() */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
    bg96_shared.bg96_sim_status_retries = 0U;
    bg96_shared.QINISTAT_error = AT_FALSE;
  }
  else if CHECK_STEP((1U))
  {
    if (p_mdm_ctxt->SID_ctxt.modem_init.init == CS_CMI_MINI)
    {
      /* minimum functionality selected, no SIM access => leave now */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_QCCID, INTERMEDIATE_CMD);
    }

    /* reset SIM ready status, we will check it later */
    p_mdm_ctxt->persist.modem_sim_ready  = AT_FALSE;
  }
  else if CHECK_STEP((2U))
  {
    if (bg96_shared.bg96_sim_status_retries > BG96_MAX_SIM_STATUS_RETRIES)
    {
      if (bg96_shared.QINISTAT_error == AT_FALSE)
      {
        /* error, max sim status retries reached */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
        retval = ATSTATUS_ERROR;
      }
      else
      {
        /* special case: AT+QINISTAT reported an error because this cmd is not
          * supported by the modem FW version. Wait for maximum retries then
          * continue sequance
          */
        PRINT_INFO("warning, modem FW version certainly too old")
        p_mdm_ctxt->persist.modem_sim_ready  = AT_TRUE; /* assume that SIM is ready now */
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, INTERMEDIATE_CMD);
      }
    }
    else
    {
      /* if QINISTAT indicates that SIM is not ready, may be it's because there is a PIN code
        *  => check PIN code status regularly
        */
      if ((bg96_shared.bg96_sim_status_retries != 0U) &&
          ((bg96_shared.bg96_sim_status_retries % 3U) == 0U))
      {
        /* check PIN code status */
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
      }
      else
      {
        /* request SIM status */
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                            ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_QINISTAT, INTERMEDIATE_CMD);
      }
    }
  }
  else if CHECK_STEP((3U))
  {
    if (p_mdm_ctxt->persist.modem_sim_ready == AT_FALSE)
    {
      if (p_mdm_ctxt->persist.sim_state == CS_SIMSTATE_SIM_PIN_REQUIRED)
      {
        /* SIM is ready but need PIN code, go to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
        PRINT_INFO("SIM is ready, unlock sequence")
      }
      else
      {
        /* SIM not ready yet, wait before retry */
        atcm_program_TEMPO(p_atp_ctxt, BG96_SIMREADY_TIMEOUT, INTERMEDIATE_CMD);
        /* go back to previous step */
        p_atp_ctxt->step = p_atp_ctxt->step - 2U;
        PRINT_INFO("SIM not ready yet")
        bg96_shared.bg96_sim_status_retries++;
      }
    }
    else
    {
      /* SIM is ready, go to next step*/
      atcm_program_SKIP_CMD(p_atp_ctxt);
      PRINT_INFO("SIM is ready, unlock sequence")
    }
  }
  else if CHECK_STEP((4U))
  {
    /* reset bg96_sim_status_retries */
    bg96_shared.bg96_sim_status_retries = 0U;
    /* check is CPIN is requested */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((5U))
  {
    if (p_mdm_ctxt->persist.sim_pin_code_ready == AT_FALSE)
    {
      if (strlen((const CRC_CHAR_t *)p_mdm_ctxt->SID_ctxt.modem_init.pincode.pincode) != 0U)
      {
        /* send PIN value */
        PRINT_INFO("CPIN required, we send user value to modem")
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
      }
      else
      {
        /* no PIN provided by user */
        PRINT_INFO("CPIN required but not provided by user")
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      PRINT_INFO("CPIN not required")
      /* no PIN required, skip cmd and go to next step */
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
  }
  else if CHECK_STEP((6U))
  {
    /* check PDP context parameters */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGDCONT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_GET_DEVICE_INFO.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_GET_DEVICE_INFO(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                             atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    if (p_mdm_ctxt->SID_ctxt.p_device_info != NULL)
    {
      switch (p_mdm_ctxt->SID_ctxt.p_device_info->field_requested)
      {
        case CS_DIF_MANUF_NAME_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMI, FINAL_CMD);
          break;

        case CS_DIF_MODEL_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMM, FINAL_CMD);
          break;

        case CS_DIF_REV_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_QGMR, FINAL_CMD);
          break;

        case CS_DIF_SN_PRESENT:
          p_mdm_ctxt->CMD_ctxt.cgsn_write_cmd_param = CGSN_SN;
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMEI_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_GSN, FINAL_CMD);
          break;

        case CS_DIF_IMSI_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CIMI, FINAL_CMD);
          break;

        case CS_DIF_PHONE_NBR_PRESENT:
          /* not AT+CNUM not supported by BG96 */
          retval = ATSTATUS_ERROR;
          break;

        case CS_DIF_ICCID_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_QCCID, FINAL_CMD);
          break;

        default:
          /* error, invalid step */
          retval = ATSTATUS_ERROR;
          break;
      }
    }
    else
    {
      PRINT_ERR("No device info context")
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_GET_SIGNAL_QUALITY.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_GET_SIGNAL_QUALITY(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* get signal strength */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CSQ, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* get signal strength */
    atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD,
                                        (CMD_ID_t) CMD_AT_QCSQ, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
#if (BG96_OPTION_NETWORK_INFO == 1)
    /* NB: cmd answer is optional ie no error will be raised if no answer received from modem
      *  indeed, if requested here, it's just a bonus and should not generate an error
      */
    atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD,
                                        (CMD_ID_t) CMD_AT_QNWINFO, INTERMEDIATE_CMD);
#else
    atcm_program_SKIP_CMD(p_atp_ctxt);
#endif /* BG96_OPTION_NETWORK_INFO */
  }
  else if CHECK_STEP((3U))
  {
#if (BG96_OPTION_ENGINEERING_MODE == 1)
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QENG, FINAL_CMD);
#else
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
#endif /* BG96_OPTION_ENGINEERING_MODE */
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_GET_ATTACHSTATUS.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_GET_ATTACHSTATUS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                              atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_REGISTER_NET.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_REGISTER_NET(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
#if 0 /* check actual registration status */
    /* check if actual registration status is the expected one */
    CS_OperatorSelector_t *operatorSelect = &(p_mdm_ctxt->SID_ctxt.write_operator_infos);
    if ((p_mdm_ctxt->SID_ctxt.read_operator_infos.mode != operatorSelect->mode) ||
        (operatorSelect->mode == CS_NRM_MANUAL) ||
        (operatorSelect->AcT_present == CS_TRUE))
    {
      /* write registration status */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
    }
    else
    {
      /* skip this step */
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
#else
    /* due to problem observed on simu: does not register after reboot */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
#endif /* 0 */
  }
  else if CHECK_STEP((2U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((3U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CREG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((4U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGREG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((5U))
  {
    /* read registration status
     * This request is used to get updated mode (in case a new mode has been requested in previous steps and
     * so, report correct value applied to upper layers).
     *
     */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_GET_NETSTATUS.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_GET_NETSTATUS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CREG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGREG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((3U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SUSBCRIBE_NET_EVENT.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SUSBCRIBE_NET_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                 atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    CS_UrcEvent_t urcEvent = p_mdm_ctxt->SID_ctxt.urcEvent;

    /* is an event linked to CREG, CGREG or CEREG ? */
    if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
        (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO) ||
        (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
    {
      (void) atcm_subscribe_net_event(p_mdm_ctxt, p_atp_ctxt);
    }
    else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
    {
      /* Note: URC to monitor signal quality not implemented despite supported by BG96 */
      retval = ATSTATUS_ERROR;

#if 0 /* urc_subscript_signalQuality */
      /* If feature needed, use following code to subscribe to +QIND:"csq",<rssi>,<ber>
       * Need to update fRspAnalyze_QIND_BG96() to report URC when detected (in current state, +QIND URC
       * is decoded and info are updated but URC is not reported to upper layer.
       */

      /* if signal quality URC not yet suscbribe */
      if (p_mdm_ctxt->persist.urc_subscript_signalQuality == CS_FALSE)
      {
        /* set event as subscribed */
        p_mdm_ctxt->persist.urc_subscript_signalQuality = CS_TRUE;

        /* request the URC we want */
        bg96_shared.QINDCFG_command_param = QINDCFG_csq;
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QINDCFG, FINAL_CMD);
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
#endif /* urc_subscript_signalQuality */
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }

  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_UNSUSBCRIBE_NET_EVENT.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_UNSUSBCRIBE_NET_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                   atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    CS_UrcEvent_t urcEvent = p_mdm_ctxt->SID_ctxt.urcEvent;

    /* is an event linked to CREG, CGREG or CEREG ? */
    if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
        (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO) ||
        (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
    {
      (void) atcm_unsubscribe_net_event(p_mdm_ctxt, p_atp_ctxt);
    }
    else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
    {
      /* Note: URC to monitor signal quality not implemented despite supported by BG96 */
      retval = ATSTATUS_ERROR;

#if 0 /* urc_subscript_signalQuality */
      /* If feature needed, use following code to subscribe to +QIND:"csq",<rssi>,<ber>
       * Need to update fRspAnalyze_QIND_BG96() to report URC when detected (in current state, +QIND URC
       * is decoded and info are updated but URC is not reported to upper layer.
       */

      /* if signal quality URC suscbribed */
      if (p_mdm_ctxt->persist.urc_subscript_signalQuality == CS_TRUE)
      {
        /* set event as unsuscribed */
        p_mdm_ctxt->persist.urc_subscript_signalQuality = CS_FALSE;

        /* request the URC we don't want */
        bg96_shared.QINDCFG_command_param = QINDCFG_csq;
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QINDCFG, FINAL_CMD);
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
#endif /* urc_subscript_signalQuality */
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_REGISTER_PDN_EVENT.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_REGISTER_PDN_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    if (p_mdm_ctxt->persist.urc_subscript_pdn_event == CS_FALSE)
    {
      /* set event as subscribed */
      p_mdm_ctxt->persist.urc_subscript_pdn_event = CS_TRUE;

      /* request PDN events */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGEREP, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DEREGISTER_PDN_EVENT.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DEREGISTER_PDN_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                  atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    if (p_mdm_ctxt->persist.urc_subscript_pdn_event == CS_TRUE)
    {
      /* set event as unsuscribed */
      p_mdm_ctxt->persist.urc_subscript_pdn_event = CS_FALSE;

      /* request PDN events */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGEREP, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_ATTACH_PS_DOMAIN.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_ATTACH_PS_DOMAIN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    p_mdm_ctxt->CMD_ctxt.cgatt_write_cmd_param = CGATT_ATTACHED;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_DETACH_PS_DOMAIN.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_DETACH_PS_DOMAIN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    p_mdm_ctxt->CMD_ctxt.cgatt_write_cmd_param = CGATT_DETACHED;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_ACTIVATE_PDN.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_ACTIVATE_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  /* SOCKET MODE */
  if CHECK_STEP((0U))
  {
    /* ckeck PDN state */
    bg96_shared.pdn_already_active = AT_FALSE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QIACT, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* PDN activation */
    if (bg96_shared.pdn_already_active == AT_TRUE)
    {
      /* PDN already active - exit */
      PRINT_INFO("Skip PDN activation (already active)")
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* request PDN activation */
      p_mdm_ctxt->CMD_ctxt.pdn_state = PDN_STATE_ACTIVATE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIACT, INTERMEDIATE_CMD);
    }
  }
  else if CHECK_STEP((2U))
  {
    /* ckeck PDN state */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QIACT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#else
  /* DATA MODE*/
  if CHECK_STEP((0U))
  {
    /* get IP address */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATX, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATD, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#endif /* USE_SOCKETS_TYPE */

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DEACTIVATE_PDN.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DEACTIVATE_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                            atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  at_status_t retval = ATSTATUS_OK;

  /* SOCKET MODE */
  if CHECK_STEP((0U))
  {
    /* PDN deactivation */
    p_mdm_ctxt->CMD_ctxt.pdn_state = PDN_STATE_DEACTIVATE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIDEACT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#else
  UNUSED(p_atp_ctxt);
  UNUSED(p_mdm_ctxt);

  /* DATA MODE */
  /* not implemented yet */
  at_status_t retval = ATSTATUS_ERROR;
#endif /* USE_SOCKETS_TYPE */

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DEFINE_PDN.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DEFINE_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  /* SOCKET MODE */
  if CHECK_STEP((0U))
  {
    bg96_shared.QICGSP_config_command = AT_TRUE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICSGP, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    bg96_shared.QICGSP_config_command = AT_FALSE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICSGP, FINAL_CMD);
  }

  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#else
  /* DATA MODE*/
  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGDCONT, FINAL_CMD);
    /* add AT+CGAUTH for username and password if required */
    /* could also use AT+QICSGP */
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#endif /* USE_SOCKETS_TYPE */

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SET_DEFAULT_PDN.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SET_DEFAULT_PDN(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                             atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  /* nothing to do here
    * Indeed, default PDN has been saved automatically during analysis of SID command
    * cf function: atcm_retrieve_SID_parameters()
    */
  atcm_program_NO_MORE_CMD(p_atp_ctxt);

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_GET_IP_ADDRESS.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_GET_IP_ADDRESS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                            atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  /* get IP address */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  /* SOCKET MODE */
  atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QIACT, FINAL_CMD);
#else
  /* DATA MODE*/
  atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, FINAL_CMD);
#endif /* USE_SOCKETS_TYPE */

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DATA_SUSPEND.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DATA_SUSPEND(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* wait for 1 second */
    atcm_program_TEMPO(p_atp_ctxt, 1000U, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* send escape sequence +++ (RAW command type)
      *  CONNECT expected before 1000 ms
      */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_ESC_CMD, FINAL_CMD);
    ATC_BG96_reinitSyntaxAutomaton(p_mdm_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DATA_RESUME.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DATA_RESUME(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                         atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATX, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATO, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SUSBCRIBE_MODEM_EVENT.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SUSBCRIBE_MODEM_EVENT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                   atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  /* nothing to do here
    * Indeed, default modem events subscribed havebeen saved automatically during analysis of SID command
    * cf function: atcm_retrieve_SID_parameters()
    */
  atcm_program_NO_MORE_CMD(p_atp_ctxt);

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DIRECT_CMD.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DIRECT_CMD(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    if (p_mdm_ctxt->SID_ctxt.p_direct_cmd_tx != NULL)
    {
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_DIRECT_CMD, FINAL_CMD);
      /* overwrite default timeout by the timeout value given by the upper layer */
      atcm_program_CMD_TIMEOUT(p_mdm_ctxt, p_atp_ctxt, p_mdm_ctxt->SID_ctxt.p_direct_cmd_tx->cmd_timeout);
    }
    else
    {
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SIM_SELECT.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SIM_SELECT(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                        atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* select the SIM slot */
    if (atcm_select_hw_simslot(p_mdm_ctxt->persist.sim_selected) != ATSTATUS_OK)
    {
      retval = ATSTATUS_ERROR;
    }
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SIM_GENERIC_ACCESS.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SIM_GENERIC_ACCESS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                                atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CSIM, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}



#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

/**
  * @brief  Manage Service ID (SID): SID_CS_DIAL_COMMAND.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DIAL_COMMAND(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  /* Note:
   * A modem cid is reserved by the HOST with atcm_socket_reserve_modem_cid() before to send AT+QIOPEN.
   * Socket is connected when 9+QIOPEN URC is received without error.
   * In case of ERROR or timeout, the modem cid has to be released.
   * This why
   *   1- Uses CMD_ANSWER_OPTIONAL to continue STEPS even if a timeout occurs (otherwise, timeout is an abort case in
   *      AT-Core).
   *   2- Errors for QIOPEN are managed in fRspAnalyze_Error_BG96() where modem cid is released.
   */

  /* SOCKET CONNECTION FOR COMMAND DATA MODE */
  if (p_mdm_ctxt->socket_ctxt.p_socket_info != NULL)
  {
    if CHECK_STEP((0U))
    {
      bg96_shared.QIOPEN_OK = AT_FALSE; /* memorize if AT+QIOPEN command has received OK */
      bg96_shared.QIOPEN_URC_OK = AT_FALSE; /* memorize if +QIOPEN URC has been received without error */
      bg96_shared.QIOPEN_URC_ERROR = AT_FALSE; /* memorize if +QIOPEN URC has been received with an error */

      /* reserve a modem CID for this socket_handle (done by HOST) */
      socket_handle_t sockHandle = p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle;
      (void) atcm_socket_reserve_modem_cid(p_mdm_ctxt, sockHandle);
      /* send command */
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD,
                                          (CMD_ID_t) CMD_AT_QIOPEN, INTERMEDIATE_CMD);
      PRINT_INFO("For Client Socket Handle=%ld : MODEM CID affected=%d",
                 sockHandle,
                 p_mdm_ctxt->persist.socket[sockHandle].socket_connId_value)

    }
    else if CHECK_STEP((1U))
    {
      if (!bg96_shared.QIOPEN_OK)
      {
        /* ERROR CASE: QIOPEN error or timeout.
         * Release the modem CID for this socket_handle.
         */
        (void) atcm_socket_release_modem_cid(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);

        /* end of SID (error case) */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
        retval = ATSTATUS_ERROR;
      }
      else if (!bg96_shared.QIOPEN_URC_OK)
      {
        /* NORMAL CASE
         * AT+QIOPEN received OK answer.
         * Still waiting for +QIOPEN urc indicating that socket is open.
         */
        atcm_program_TEMPO(p_atp_ctxt, BG96_QIOPEN_TIMEOUT, INTERMEDIATE_CMD);
        /* go to the nextstep */
      }
      else /* bg96_shared.QIOPEN_URC_OK */
      {
        /* NORMAL CASE
         * AT+QIOPEN received OK answer.
         * +QIOPEN URC already received without error, socket opened (Do not wait for BG96_QIOPEN_TIMEOUT).
         * Socket is connected.
         */
        (void) atcm_socket_set_connected(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);

        /* end of SID (normal case) */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else  if CHECK_STEP((2U))
    {
      /* waiting for +QIOPEN URC Timeout case */
      if (bg96_shared.QIOPEN_OK && bg96_shared.QIOPEN_URC_OK)
      {
        /* +QIOPEN URC received without error, socket opened */
        /* socket is connected */
        (void) atcm_socket_set_connected(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);

        /* end of SID (normal case) */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        /* +QIOPEN URC NOT RECEIVED
         *  cf BG96 TCP/IP AT Commands Manual V1.0, paragraph 2.1.4 - 3/
         *  "if the URC cannot be received within 150 seconds, AT+QICLOSE should be used to close
         *   the socket".
         *
         *  then we will have to return an error to cellular service !!! (see next step)
         */
        atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD,
                                            (CMD_ID_t) CMD_AT_QICLOSE, INTERMEDIATE_CMD);
      }
    }
    else  if CHECK_STEP((3U))
    {
      /* if we fall here, it means we have send CMD_AT_QICLOSE on previous step
        *  now inform cellular service that opening has failed => returns an error.
        */
      /* release the modem CID for this socket_handle
       * This is done in all  cases (even CLOSE error or timeout).
       */
      (void) atcm_socket_release_modem_cid(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);

      /* end of SID (error case) */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    PRINT_ERR("No socket info context")
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SEND_DATA.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SEND_DATA(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                       atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* Check data size to send */
    if (p_mdm_ctxt->SID_ctxt.socketSendData_struct.buffer_size > MODEM_MAX_SOCKET_TX_DATA_SIZE)
    {
      PRINT_ERR("Data size to send %ld exceed maximum size %ld",
                p_mdm_ctxt->SID_ctxt.socketSendData_struct.buffer_size,
                MODEM_MAX_SOCKET_TX_DATA_SIZE)
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      p_mdm_ctxt->socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QISEND, INTERMEDIATE_CMD);
    }
  }
  else if CHECK_STEP((1U))
  {
    /* waiting for socket prompt: "<CR><LF>> " */
    if (p_mdm_ctxt->socket_ctxt.socket_send_state == SocketSendState_Prompt_Received)
    {
      PRINT_DBG("SOCKET PROMPT ALREADY RECEIVED")
      /* go to next step */
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
    else
    {
      PRINT_DBG("WAITING FOR SOCKET PROMPT")
      atcm_program_WAIT_EVENT(p_atp_ctxt, BG96_SOCKET_PROMPT_TIMEOUT, INTERMEDIATE_CMD);
    }
  }
  else if CHECK_STEP((2U))
  {
    /* socket prompt received, send DATA */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_QISEND_WRITE_DATA, FINAL_CMD);

    /* reinit automaton to receive answer */
    ATC_BG96_reinitSyntaxAutomaton(p_mdm_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_RECEIVE_DATA and SID_CS_RECEIVE_DATA_FROM.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_RECEIVE_DATA(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* Request size of data only if an URC indicating that data are available has been received.
     * This is an optimization to avoid unnecessary modem communications in case the client
     * application calls this function without having received data notification.
     */
    if (atcm_socket_request_available_data(p_mdm_ctxt))
    {
      p_mdm_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_RequestSize;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIRD, INTERMEDIATE_CMD);
    }
    else
    {
      /* stop here, no command send to the modem */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else if CHECK_STEP((1U))
  {
    /* check that data size to receive is not null */
    if (p_mdm_ctxt->socket_ctxt.socket_rx_expected_buf_size != 0U)
    {
      /* check that data size to receive does not exceed maximum size
        *  if it's the case, only request maximum size we can receive
        */
      if (p_mdm_ctxt->socket_ctxt.socket_rx_expected_buf_size >
          p_mdm_ctxt->socket_ctxt.socketReceivedata.max_buffer_size)
      {
        PRINT_INFO("Data size available (%ld) exceed buffer maximum size (%ld)",
                   p_mdm_ctxt->socket_ctxt.socket_rx_expected_buf_size,
                   p_mdm_ctxt->socket_ctxt.socketReceivedata.max_buffer_size)
        p_mdm_ctxt->socket_ctxt.socket_rx_expected_buf_size =
          p_mdm_ctxt->socket_ctxt.socketReceivedata.max_buffer_size;
      }

      /* receive data */
      p_mdm_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Header;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIRD, FINAL_CMD);
    }
    else
    {
      /* no data to receive */
      atcm_socket_clear_available_data_flag(p_mdm_ctxt);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SOCKET_CLOSE.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SOCKET_CLOSE(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if (p_mdm_ctxt->socket_ctxt.p_socket_info != NULL)
  {
    if CHECK_STEP((0U))
    {
      /* is socket connected ?
        * due to BG96 socket connection mechanism (waiting URC QIOPEN), we can fall here but socket
        * has been already closed if error occurs during connection
        */
      if (atcm_socket_is_connected(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle) == AT_TRUE)
      {
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QICLOSE, INTERMEDIATE_CMD);
      }
      else
      {
        /* release the modem CID for this socket_handle */
        (void) atcm_socket_release_modem_cid(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);

        /* end of SID */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
    else if CHECK_STEP((1U))
    {
      /* if no error or timeout for QICLOSE, we can release the modem CID for this socket_handle */
      (void) atcm_socket_release_modem_cid(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    PRINT_ERR("No socket info context")
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SOCKET_CNX_STATUS.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SOCKET_CNX_STATUS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                               atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QISTATE, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_PING_IP_ADDRESS.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_PING_IP_ADDRESS(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                             atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QPING, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_DNS_REQ.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_DNS_REQ(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                     atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIDNSCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* initialize +QIURC "dnsgip" parameters */
    init_bg96_qiurc_dnsgip();
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QIDNSGIP, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    /* do we have received a valid DNS response ? */
    if ((bg96_shared.QIURC_dnsgip_param.finished == AT_TRUE) && (bg96_shared.QIURC_dnsgip_param.error == 0U))
    {
      /* yes */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* not yet, waiting for DNS information */
      atcm_program_TEMPO(p_atp_ctxt, BG96_QIDNSGIP_TIMEOUT, FINAL_CMD);
    }
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Initialize QIURC dnsgip info structure
  * @retval none
  */
static void init_bg96_qiurc_dnsgip(void)
{
  bg96_shared.QIURC_dnsgip_param.finished = AT_FALSE;
  bg96_shared.QIURC_dnsgip_param.wait_header = AT_TRUE;
  bg96_shared.QIURC_dnsgip_param.error = 0U;
  bg96_shared.QIURC_dnsgip_param.ip_count = 0U;
  bg96_shared.QIURC_dnsgip_param.ttl = 0U;
  (void) memset((void *)bg96_shared.QIURC_dnsgip_param.hostIPaddr, 0, MAX_SIZE_IPADDR);
}
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
/**
  * @brief  Manage Service ID (SID): SID_CS_INIT_POWER_CONFIG.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_INIT_POWER_CONFIG(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                               atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* Init parameters are available into SID_ctxt.init_power_config
      * SID_ctxt.init_power_config is used to build AT+CPSMS and AT+CEDRX commands
      * Build it from SID_ctxt.init_power_config and modem specificities
      */
    if (ATC_BG96_init_low_power(p_mdm_ctxt) == AT_FALSE)
    {
      /* Low Power not enabled, stop here the SID */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* Low Power enabled, continue to next step
        * CEREG 4 not requested for BG96
        */
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
  }
  else if CHECK_STEP((1U))
  {
    /* read EDRX params */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEDRXS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    /* read PSM params */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((3U))
  {
    /* set EDRX params (default) */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEDRXS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((4U))
  {
    /* enable URC output */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QURCCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((5U))
  {
    /* request for +QPSMTIMER urc */
    bg96_shared.QCFG_command_write = AT_TRUE;
    bg96_shared.QCFG_command_param = QCFG_urc_psm;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((6U))
  {
    /* set PSM params (default) */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((7U))
  {
    /* configure advanced PSM parameters */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QPSMEXTCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((8U))
  {
    /* configure advanced PSM parameters */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_QPSMEXTCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((9U))
  {
    /* note: keep this as final command (previous command may be skipped if no valid PSM parameters) */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SET_POWER_CONFIG.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SET_POWER_CONFIG(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                              atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    (void) ATC_BG96_set_low_power(p_mdm_ctxt);
    atcm_program_SKIP_CMD(p_atp_ctxt);
  }
  else if CHECK_STEP((1U))
  {
    /* set EDRX params (if available) */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEDRXS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    /* set PSM params (if available) */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((3U))
  {
    /* eDRX Read Dynamix Parameters */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CEDRXRDP, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((4U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QPSMS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((5U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QPSMCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((6U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_QPSMEXTCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((7U))
  {
    /* note: keep this as final command (previous command may be skipped if no valid PSM parameters) */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SLEEP_REQUEST.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SLEEP_REQUEST(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                           atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* update LP automaton states */
    ATC_BG96_low_power_event(EVENT_LP_HOST_SLEEP_REQ);

    /* Simulate modem ack to enter in low power state
      * (UART is not deactivated yet)
      * check if this event was subscribed by upper layers
      */
    if (atcm_modem_event_received(p_mdm_ctxt, CS_MDMEVENT_LP_ENTER) == AT_TRUE)
    {
      /* trigger an internal event to ATCORE (ie an event not received from IPC)
        * to allow to report an URC to upper layers (URC event = enter in LP)
        */
      AT_internalEvent(DEVTYPE_MODEM_CELLULAR);
    }
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SLEEP_COMPLETE.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SLEEP_COMPLETE(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                            atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    ATC_BG96_low_power_event(EVENT_LP_HOST_SLEEP_COMPLETE);
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_SLEEP_CANCEL.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_SLEEP_CANCEL(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                          atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    ATC_BG96_low_power_event(EVENT_LP_HOST_SLEEP_CANCEL);
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): SID_CS_WAKEUP.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_WAKEUP(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                    atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    if (p_mdm_ctxt->SID_ctxt.wakeup_origin == HOST_WAKEUP)
    {
      ATC_BG96_low_power_event(EVENT_LP_HOST_WAKEUP_REQ);

      if (ATC_BG96_is_waiting_modem_wakeup() == true)
      {
        PRINT_INFO("this is a host wake up and modem is in PSM state")
        /* a non null delay is used in case wake-up is called just after PSM POWER DOWN indication,
          *  otherwise this wake up request is not taken into account by the modem
          */
        (void) SysCtrl_BG96_wakeup_from_PSM(1000U);
        /* No AT command to send, wait RDY from modem in next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        PRINT_INFO("this is a host wake up but modem is not in PSM state")
        /* modem is not in PSM, no need to wait for wake-up confirmation */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
  }
  else if CHECK_STEP((1U))
  {
    /* waiting for RDY indication from modem */
    atcm_program_WAIT_EVENT(p_atp_ctxt, BG96_APP_RDY_TIMEOUT, INTERMEDIATE_CMD);

  }
  else if CHECK_STEP((2U))
  {
    /* disable echo */
    p_mdm_ctxt->CMD_ctxt.command_echo = AT_FALSE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATE, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((3U))
  {
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

