/**
  ******************************************************************************
  * @file    at_custom_modem_sid.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
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
#include "at_core.h"
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

#if defined(USE_MODEM_TYPE1SC)
#if defined(HWREF_MURATA_TYPE1SC_EVK)
#else
#error Hardware reference not specified
#endif /* HWREF_MURATA_TYPE1SC_EVK */
#endif /* USE_MODEM_TYPE1SC */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_SID AT_CUSTOM ALTAIR_T1SC SID
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SID_Private_Defines AT_CUSTOM ALTAIR_T1SC SID Private Defines
  * @{
  */
#define SUPPORT_BOOTEV     (1)   /* support or not %BOOTEV (0 or 1) */
#define USE_AT_IFC         (1)   /* 1 to use AT+IFC to set Hw Flow Control mode
                                  * 0 to use AT&K3 */
#define MURATA_CMD_SUBSET  (0)   /* In Murata implementation, only a subset of ALTAIR AT commands is supported
                                  * set this flag to 1 for Murata */
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SID_Private_Macros AT_CUSTOM ALTAIR_T1SC SID Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "TYPE1SC API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "TYPE1SC ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...)  (void) printf("TYPE1SC:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("TYPE1SC ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

#define CHECK_STEP(stepval) (p_atp_ctxt->step == stepval)

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SID_Private_Functions_Prototypes
  *    AT_CUSTOM ALTAIR_T1SC SID Private Functions Prototypes
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
#endif /*(USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

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
/**
  * @}
  */


/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SID_Exported_Functions AT_CUSTOM ALTAIR_T1SC SID Exported Functions
  * @{
  */

/**
  * @brief  Get next AT command for current Service ID (SID).
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
at_status_t at_custom_SID_T1SC(atcustom_modem_context_t *p_mdm_ctxt,
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

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SID_Private_Functions AT_CUSTOM ALTAIR_T1SC SID Private Functions
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
  static const sid_LUT_t SID_TYPE1SC_LUT[] =
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
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

    /**
      * LOW POWER SIDs
      */
    {SID_CS_INIT_POWER_CONFIG, at_SID_CS_INIT_POWER_CONFIG},
    {SID_CS_SET_POWER_CONFIG, at_SID_CS_SET_POWER_CONFIG},
    {SID_CS_SLEEP_REQUEST, at_SID_CS_SLEEP_REQUEST},
    {SID_CS_SLEEP_COMPLETE, at_SID_CS_SLEEP_COMPLETE},
    {SID_CS_SLEEP_CANCEL, at_SID_CS_SLEEP_CANCEL},
    {SID_CS_WAKEUP, at_SID_CS_WAKEUP},
  };
#define SIZE_SID_TYPE1SC_LUT ((uint16_t) (sizeof (SID_TYPE1SC_LUT) / sizeof (sid_LUT_t)))

  at_status_t retval = ATSTATUS_ERROR;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  uint16_t index = 0U;
  bool leave_loop = false;

  /* search SID in the LUT */
  do
  {
    if (SID_TYPE1SC_LUT[index].sid == curSID)
    {
      /* execute the function corresponding to the given SID */
      retval = SID_TYPE1SC_LUT[index].sid_Function(p_mdm_ctxt, p_at_ctxt, p_atp_ctxt, p_ATcmdTimeout);
      leave_loop = true;
    }
    index++;
  } while ((leave_loop == false) && (index < SIZE_SID_TYPE1SC_LUT));

  return (retval);
}

/**
  * @brief  Manage Service ID (SID): CS_CHECK_CNX.
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
  * @brief  Manage Service ID (SID): SID_CS_POWER_ON and SID_CS_RESET.
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_atp_ctxt Pointer to the structure of AT Parser context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
static at_status_t at_SID_CS_POWER_ON(atcustom_modem_context_t *p_mdm_ctxt, at_context_t *p_at_ctxt,
                                      atparser_context_t *p_atp_ctxt, uint32_t *p_ATcmdTimeout)
{
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  uint8_t common_start_sequence_step = TYPE1SC_MODEM_SYNCHRO_AT_MAX_RETRIES + 1U;
  static bool reprogram_bdelay = false;

  /* POWER_ON and RESET are almost the same, specific differences are managed case by case */
  if ((curSID == (at_msg_t) SID_CS_RESET) && (p_mdm_ctxt->SID_ctxt.reset_type != CS_RESET_HW))
  {
    /* for reset, only HW reset is supported */
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
      ATC_TYPE1SC_reset_variables();

      /* reinit modem at ready status */
      p_mdm_ctxt->persist.modem_at_ready = AT_FALSE;

      /* in case of RESET, reset all the contexts to start from a fresh state */
      if (curSID == (at_msg_t) SID_CS_RESET)
      {
        ATC_TYPE1SC_modem_reset(p_mdm_ctxt);
      }

      /* NOTE:
        *   ALTAIR modem always boots without HW flow control activated.
        *   Force the HwFlowControl to none until we use AT&K command to set the requested value.
        */
      (void) SysCtrl_TYPE1SC_reinit_channel(p_at_ctxt->ipc_handle, SYSCTRL_HW_FLOW_CONTROL_NONE);

      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
    else if ((p_atp_ctxt->step >= 1U) && (p_atp_ctxt->step < TYPE1SC_MODEM_SYNCHRO_AT_MAX_RETRIES))
    {
      if (p_atp_ctxt->step >= 11U)
      {
        /* we wait too much time, force BDELAY to 0 t to minimize boot time */
        reprogram_bdelay = true;
      }

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
        p_atp_ctxt->step = common_start_sequence_step - 1U;
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    else if CHECK_STEP((TYPE1SC_MODEM_SYNCHRO_AT_MAX_RETRIES))
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
      /* set modem in minimum function in order to let application to configure it before activating the RF */
      p_mdm_ctxt->CMD_ctxt.cfun_value = 0U;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
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
      /* Read FW revision */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 5U))
    {
#if (MURATA_CMD_SUBSET == 0)
      /* Read bands configuration */
      type1sc_shared.getcfg_function = SETGETCFG_BAND;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETCFG, INTERMEDIATE_CMD);
#else
      atcm_program_SKIP_CMD(p_atp_ctxt);
#endif /* MURATA_CMD_SUBSET == 0 */
    }
    else if CHECK_STEP((common_start_sequence_step + 6U))
    {
      /* Read HIFC mode (low power) */
      type1sc_shared.getcfg_function = SETGETCFG_HIFC_MODE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETACFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 7U))
    {
      /* Read PMCONF SLEEP mode (low power) */
      type1sc_shared.getcfg_function = SETGETCFG_PMCONF_SLEEP_MODE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETACFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 8U))
    {
      /* Read PMCONF MAX ALLOWED (low power) */
      type1sc_shared.getcfg_function = SETGETCFG_PMCONF_MAX_ALLOWED;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETACFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 9U))
    {
      /* long boot time detected ? try to shorten it */
      if (reprogram_bdelay == true)
      {
        PRINT_INFO("Reprogram modem boot delay")
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                            ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETBDELAY, INTERMEDIATE_CMD);
      }
      else
      {
        PRINT_INFO("Do not reprogram modem boot delay")
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    else if CHECK_STEP((common_start_sequence_step + 10U))
    {
      /*  force to disable PSM in case modem was switched off with PSM enabled
        *  With current FW version, after reboot, modem applies PSM state that was set before
        *  to switch off. By default, HOST considers that modem starts without PSM, this is why
        *  we send AT+CPSMS=0 during power on (modem and Host are synchro).
        *  OPTIM: if modem FW evolves to starts without PSM by default, this command can be removed.
        */
      p_mdm_ctxt->SID_ctxt.set_power_config.psm_present = CELLULAR_TRUE;
      p_mdm_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
    }
#if (CONFIG_MODEM_UART_RTS_CTS == 0)
    /* --- No HW Flow control requested ----
      *   This is the default mode when modem is booting, nothing to do.
      */
    else if CHECK_STEP((common_start_sequence_step + 11U))
    {
#if (USE_AT_IFC == 1)
      /* apply current Hw Flow Control mode */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
#else
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt,
                                          ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_AND_K0, INTERMEDIATE_CMD);
#endif /* (USE_AT_IFC == 1) */
    }
    else if CHECK_STEP((common_start_sequence_step + 12U))
    {
      /* add a tempo */
      atcm_program_TEMPO(p_atp_ctxt, 3000U, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 13U))
    {
      /* Check connection with modem */
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt,
                                          ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, INTERMEDIATE_CMD);
    }
#else
    /* --- HW Flow control is requested ----
      *  Send command to set Hw Flow Control
      *  then reinit UART with HwFlowControl activated and check connection with modem.
      */
    else if CHECK_STEP((common_start_sequence_step + 11U))
    {
#if (USE_AT_IFC == 1)
      /* apply current Hw Flow Control mode */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
#else
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt,
                                          ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_AND_K3, INTERMEDIATE_CMD);
#endif /* (USE_AT_IFC == 1) */
    }
    else if CHECK_STEP((common_start_sequence_step + 12U))
    {
      (void) SysCtrl_TYPE1SC_reinit_channel(p_at_ctxt->ipc_handle, SYSCTRL_HW_FLOW_CONTROL_RTS_CTS);
      /* add a tempo */
      atcm_program_TEMPO(p_atp_ctxt, 3000U, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 13U))
    {
      /* Check connection with modem */
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt,
                                          ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, INTERMEDIATE_CMD);
    }

#endif /* (CONFIG_MODEM_UART_RTS_CTS == 0) */
#if (SUPPORT_BOOTEV == 1)
    else if CHECK_STEP((common_start_sequence_step + 14U))
    {
      /* request to receive boot event */
      type1sc_shared.setcfg_function = SETGETCFG_BOOT_EVENT_TRUE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETACFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 15U))
    {
      if (type1sc_shared.modem_bootev_received == true)
      {
        PRINT_INFO("***** BOOTEV already received, continue init sequence *****")
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        type1sc_shared.modem_waiting_for_bootev = true;
        PRINT_INFO("***** wait for optional BOOTEV *****")
        /* wait for +%BOOTEV */
        atcm_program_TEMPO(p_atp_ctxt, TYPE1SC_BOOTEV_TIMEOUT, INTERMEDIATE_CMD);
      }
    }
#else
    else if CHECK_STEP((common_start_sequence_step + 14U))
    {
      /* request to not receive boot event */
      type1sc_shared.setcfg_function = SETGETCFG_BOOT_EVENT_FALSE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETACFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 15U))
    {
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
#endif /* (SUPPORT_BOOTEV == 1) */
    else if CHECK_STEP((common_start_sequence_step + 16U))
    {
      /* program events to notify  */
      type1sc_shared.notifyev_mode = 1U;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_NOTIFYEV, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 17U))
    {
      /* check connection */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
    }
    else if (p_atp_ctxt->step >= (common_start_sequence_step + 18U))
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
    /* it's upper layer responsibility to manage proper detach before switch off
      * p_mdm_ctxt->CMD_ctxt.cfun_value = 0U;
      * atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, FINAL_CMD);
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
    /* cfun parameters coming from client API for SID_CS_INIT_MODEM */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    if (p_mdm_ctxt->SID_ctxt.modem_init.init == CS_CMI_MINI)
    {
      /* Do not check PIN.
        * It is not activated in this mode.
        */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* check if CPIN is requested
        *
        * if SIM is not ready, we can receive +CME ERROR: SIM BUSY
        * in this case, retry to send CPIN command until TYPE1SC_MAX_SIM_STATUS_RETRIES
        */
      type1sc_shared.sim_status_retries++;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
    }
  }
  else if CHECK_STEP((2U))
  {
    if (type1sc_shared.sim_status_retries > TYPE1SC_MAX_SIM_STATUS_RETRIES)
    {
      /* error, max sim status retries reached */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
      retval = ATSTATUS_ERROR;
    }
    else
    {
      PRINT_INFO("SIM sim_pin_code_ready %d", p_mdm_ctxt->persist.sim_pin_code_ready)
      PRINT_INFO("SIM sim_state %d", p_mdm_ctxt->persist.sim_state)
      if ((p_mdm_ctxt->persist.sim_pin_code_ready == AT_FALSE) &&
          ((p_mdm_ctxt->persist.sim_state == CS_SIMSTATE_SIM_BUSY)))
      {
        /* SIM not ready yet, wait before retry */
        atcm_program_TEMPO(p_atp_ctxt, TYPE1SC_SIMREADY_TIMEOUT, INTERMEDIATE_CMD);
        /* go back to previous step */
        p_atp_ctxt->step = p_atp_ctxt->step - 2U;
        PRINT_INFO("SIM not ready yet")
      }
      else
      {
        /* continue to next step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
  }
  else if CHECK_STEP((3U))
  {
    /* reset sim_status_retries */
    type1sc_shared.sim_status_retries = 0U;

    if (p_mdm_ctxt->persist.sim_pin_code_ready == AT_FALSE)
    {
      if (strlen((const CRC_CHAR_t *)p_mdm_ctxt->SID_ctxt.modem_init.pincode.pincode) != 0U)
      {
        /* send PIN value */
        PRINT_INFO("CPIN required, we send user value to modem")
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPIN, FINAL_CMD);
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
      /* no PIN required */
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
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, FINAL_CMD);
          break;

        case CS_DIF_SN_PRESENT:
          p_mdm_ctxt->CMD_ctxt.cgsn_write_cmd_param = CGSN_SN;
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMEI_PRESENT:
          p_mdm_ctxt->CMD_ctxt.cgsn_write_cmd_param = CGSN_IMEI;
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMSI_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CIMI, FINAL_CMD);
          break;

        case CS_DIF_PHONE_NBR_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CNUM, FINAL_CMD);
          break;

        case CS_DIF_ICCID_PRESENT:
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CCID, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CSQ, FINAL_CMD);
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
    /* check if actual registration status is the expected one or if AcT is explicitly specified */
    CS_OperatorSelector_t *operatorSelect = &(p_mdm_ctxt->SID_ctxt.write_operator_infos);
    if ((p_mdm_ctxt->SID_ctxt.read_operator_infos.mode != operatorSelect->mode) ||
        (operatorSelect->AcT_present == CELLULAR_TRUE))
    {
      /* write registration status */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
    }
    else
    {
      /* skip this step
        * allow the modem to re-use the last cell that has been found.
        */
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
  }
  else if CHECK_STEP((2U))
  {
    /* read registration status */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEREG, FINAL_CMD);
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
    /* read extended error report */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEER, INTERMEDIATE_CMD);
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

    /* is an event linked to CREG or CEREG ? */
    if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
        (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
    {
      (void) atcm_subscribe_net_event(p_mdm_ctxt, p_atp_ctxt);
    }
    else if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO))
    {
      /* CGREG not supported in TYPE1SC
        *  ignore the request to avoid an error
        */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
    {
      /* no command to monitor signal quality with URC in TYPE1SC */
      retval = ATSTATUS_ERROR;
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
        (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
    {
      (void) atcm_unsubscribe_net_event(p_mdm_ctxt, p_atp_ctxt);
    }
    else if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO))
    {
      /* CGREG not supported in TYPE1SC
        *  ignore the request to avoid an error
        */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
    {
      /* no command to monitor signal quality with URC in TYPE1SC */
      retval = ATSTATUS_ERROR;
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
    if (p_mdm_ctxt->persist.urc_subscript_pdn_event == CELLULAR_FALSE)
    {
      /* set event as subscribed */
      p_mdm_ctxt->persist.urc_subscript_pdn_event = CELLULAR_TRUE;

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
    if (p_mdm_ctxt->persist.urc_subscript_pdn_event == CELLULAR_TRUE)
    {
      /* set event as unsuscribed */
      p_mdm_ctxt->persist.urc_subscript_pdn_event = CELLULAR_FALSE;

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
    /* PDN activation */
    p_mdm_ctxt->CMD_ctxt.pdn_state = PDN_STATE_ACTIVATE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNACT, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#else
  /* DATA MODE */
  if CHECK_STEP((0U))
  {
    /* get IP address */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNACT, FINAL_CMD);
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

  /* DATA MODE*/
  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNSET, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* For ALT1250, it looks that we need to wait sometimes after PDNSET before sending AT+CFUN.
      * so wait for 2 second
      */
    atcm_program_TEMPO(p_atp_ctxt, 2000U, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_PDNSET, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

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
  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNRDP, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, FINAL_CMD);
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
      */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_ESC_CMD, INTERMEDIATE_CMD);
    ATC_TYPE1SC_reinitSyntaxAutomaton();

  }
  else if CHECK_STEP((2U))
  {
    /* waiting for CONNECT */
    atcm_program_TEMPO(p_atp_ctxt, 2000U, FINAL_CMD);
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
      atcm_program_CMD_TIMEOUT(p_mdm_ctxt, p_atp_ctxt, p_mdm_ctxt->SID_ctxt.p_direct_cmd_tx->cmd_timeout);
    }
    else
    {
      PRINT_ERR("No direct cmd context")
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
    type1sc_shared.syscfg_function = SETGETSYSCFG_SIM_POLICY;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETSYSCFG, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* if current sim in modem is different than the requested SIM,
      * send cmd to configure the modem (a modem reboot is needed) */
    if (type1sc_shared.modem_sim_same_as_selected == false)
    {
      type1sc_shared.syscfg_function = SETGETSYSCFG_SIM_POLICY;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETSYSCFG, INTERMEDIATE_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    if (type1sc_shared.modem_sim_same_as_selected == false)
    {
      atcm_set_error_report(CSERR_UNKNOWN, p_mdm_ctxt);
      /* atcm_set_error_report(CSERR_MODEM_REBOOT_NEEDED, p_mdm_ctxt); */
      retval = ATSTATUS_ERROR;
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
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

  if CHECK_STEP((0U))
  {
    /* step 1 - allocate a socket and request a socket_id */
    type1sc_shared.SocketCmd_Allocated_SocketID = AT_FALSE;
    type1sc_shared.SocketCmd_Activated = AT_FALSE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_ALLOCATE,
                        INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* step 2 - verify that a socket_id has been allocated then activate the socket.
     *          If this is the case and if an error occurs during ACTIVATE phase, the socket_id has
     *          to be freed with DELETE command.
     *          2 types of error are possible: ERROR or Timeout
     *          To avoid an automatic abort from AT-Core in case of timeout, the answer is considered as
     *          optional to manage this case here.
     */
    if (type1sc_shared.SocketCmd_Allocated_SocketID == AT_TRUE)
    {
      atcm_program_AT_CMD_ANSWER_OPTIONAL(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD,
                                          (CMD_ID_t) CMD_AT_SOCKETCMD_ACTIVATE, INTERMEDIATE_CMD);
    }
    else
    {
      PRINT_ERR("No valid socket_id affected by the modem has been reecived")
      retval = ATSTATUS_ERROR;
    }
  }
  else if CHECK_STEP((2U))
  {
    if (type1sc_shared.SocketCmd_Activated == AT_TRUE)
    {
      /* ACTIVATE phase has been successful */
      if (p_mdm_ctxt->socket_ctxt.p_socket_info != NULL)
      {
        /* socket is connected */
        (void) atcm_socket_set_connected(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        PRINT_ERR("No socket context available")
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      /* an error occurred during ACTIVATE phase, delete the allocated socket_id */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_DELETE,
                          INTERMEDIATE_CMD);
      /* Note:
       * use an INTERMEDIATE_CMD to force to go to next step and return an ERROR to upper layer
       */
    }
  }
  else
  {
    /* an error occurred */
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
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETDATA_SEND, FINAL_CMD);
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
    /* Request data only if an URC indicating that data are available has been received.
     * This is an optimization to avoid unnecessary modem communications in case the client
     * application calls this function without having received data notification.
     */
    if (atcm_socket_request_available_data(p_mdm_ctxt))
    {
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                          ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETDATA_RECEIVE, INTERMEDIATE_CMD);
    }
    else
    {
      /* stop here, no command send to the modem */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else if CHECK_STEP((1U))
  {
    /* reset data available flag if no data received */
    if (p_mdm_ctxt->socket_ctxt.socketReceivedata.buffer_size == 0U)
    {
      atcm_socket_clear_available_data_flag(p_mdm_ctxt);
    }
    /* stop here, no command send to the modem */
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

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_DEACTIVATE,
                        INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_DELETE, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_INFO, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PINGCMD, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_DNSRSLV, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

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
    /* Init parameters are available in to SID_ctxt.init_power_config
      * SID_ctxt.init_power_config  is used to build AT+CPSMS and AT+CEDRX commands
      * Built it from SID_ctxt.init_power_config  and modem specificities
      */
    if (ATC_TYPE1SC_init_low_power(p_mdm_ctxt) == AT_FALSE)
    {
      /* Low Power not enabled, stop here the SID */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* Low Power enabled : update CEREG to request PSM parameters then send commands */
      p_mdm_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
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
    /* set PSM params (default) */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((5U))
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
    if (ATC_TYPE1SC_set_low_power(p_mdm_ctxt) == AT_FALSE)
    {
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
    else
    {
      /* Low Power enabled : update CEREG to request PSM parameters then send commands */
      p_mdm_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
    }
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
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* update LP automaton state */
    ATC_TYPE1SC_low_power_event(EVENT_LP_HOST_SLEEP_REQ, false);

    (void) SysCtrl_TYPE1SC_request_suspend_channel(p_at_ctxt->ipc_handle, DEVTYPE_MODEM_CELLULAR);
    atcm_program_SKIP_CMD(p_atp_ctxt);
  }
  else if CHECK_STEP((1U))
  {
    /* end of SID */
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
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    ATC_TYPE1SC_low_power_event(EVENT_LP_HOST_SLEEP_COMPLETE, false);
    (void) SysCtrl_TYPE1SC_complete_suspend_channel(p_at_ctxt->ipc_handle, DEVTYPE_MODEM_CELLULAR);
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
  UNUSED(p_ATcmdTimeout);
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    ATC_TYPE1SC_low_power_event(EVENT_LP_HOST_SLEEP_CANCEL, false);

    /* wake up modem (in case modem already enters in Low Power or we missed the URC from modem) */
    uint8_t modem_originated = 0U;
    (void) SysCtrl_TYPE1SC_resume_channel(p_at_ctxt->ipc_handle,
                                          DEVTYPE_MODEM_CELLULAR,
                                          modem_originated);
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
  UNUSED(p_ATcmdTimeout);

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    ATC_TYPE1SC_low_power_event(EVENT_LP_HOST_WAKEUP_REQ, false);

    uint8_t modem_originated = (p_mdm_ctxt->SID_ctxt.wakeup_origin == MODEM_WAKEUP) ? 1U : 0U;
    (void) SysCtrl_TYPE1SC_resume_channel(p_at_ctxt->ipc_handle,
                                          DEVTYPE_MODEM_CELLULAR,
                                          modem_originated);
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

