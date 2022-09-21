/**
  ******************************************************************************
  * @file    at_custom_modem_sid.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
  *          Sequans Monarch modem
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
#include "string.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_custom_modem_sid.h"
#include "at_custom_modem_specific.h"
#include "at_custom_modem_signalling.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_custom_sysctrl.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#include "at_modem_socket.h"
#include "at_custom_modem_socket.h"
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_SID AT_CUSTOM SEQUANS_MONARCH SID
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SID_Private_Macros AT_CUSTOM SEQUANS_MONARCH SID Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "MONARCH:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "MONARCH:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "MONARCH API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "MONARCH ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...)  (void) printf("MONARCH:" format "\n\r", ## args);
#define PRINT_DBG(...) __NOP(); /* Nothing to do */
#define PRINT_API(...) __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("MONARCH ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...) __NOP(); /* Nothing to do */
#define PRINT_DBG(...)  __NOP(); /* Nothing to do */
#define PRINT_API(...)  __NOP(); /* Nothing to do */
#define PRINT_ERR(...)  __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

#define CHECK_STEP(stepval) (p_atp_ctxt->step == stepval)
/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SID_Private_Functions_Prototypes
  *     AT_CUSTOM SEQUANS_MONARCH SID Private Function _Prototypes
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
static void init_dns_info(void);
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

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

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SID_Exported_Functions AT_CUSTOM SEQUANS_MONARCH SID Exported Functions
  * @{
  */
/**
  * @brief  Get next AT command for current Service ID (SID).
  * @param  p_mdm_ctxt Pointer to the structure of Modem context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for current AT command.
  * @retval at_status_t
  */
at_status_t at_custom_SID_monarch(atcustom_modem_context_t *p_mdm_ctxt,
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


/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SID_Private_Functions AT_CUSTOM SEQUANS_MONARCH SID Private Function
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
  static const sid_LUT_t SID_MONARCH_LUT[] =
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
#define SIZE_SID_MONARCH_LUT ((uint16_t) (sizeof (SID_MONARCH_LUT) / sizeof (sid_LUT_t)))

  at_status_t retval = ATSTATUS_ERROR;
  at_msg_t curSID = p_atp_ctxt->current_SID;

  uint16_t index = 0U;
  bool leave_loop = false;

  /* search SID in the LUT */
  do
  {
    if (SID_MONARCH_LUT[index].sid == curSID)
    {
      /* execute the function corresponding to the given SID */
      retval = SID_MONARCH_LUT[index].sid_Function(p_mdm_ctxt, p_at_ctxt, p_atp_ctxt, p_ATcmdTimeout);
      leave_loop = true;
    }
    index++;
  } while ((leave_loop == false) && (index < SIZE_SID_MONARCH_LUT));

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
    /* NOT IMPLEMENTED */
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
  uint8_t common_start_sequence_step = 2U;

  /* POWER_ON and RESET are almost the same, specific differences are managed case by case */

  /* for reset, only HW reset is supported and treated as Power ON  */
  if ((curSID == (at_msg_t) SID_CS_POWER_ON) ||
      ((curSID == (at_msg_t) SID_CS_RESET) && (p_mdm_ctxt->SID_ctxt.reset_type == CS_RESET_HW)))
  {
    /****************************************************************************
      * POWER_ON and RESET first steps
      * try to establish the communiction with the modem by sending "AT" commands
      ****************************************************************************/
    if CHECK_STEP((0U))
    {
      /* reset modem specific variables */
      ATC_Monarch_reset_variables();

      /* check if +SYSSTART has been received */
      if (p_mdm_ctxt->persist.modem_at_ready == AT_TRUE)
      {
        PRINT_DBG("Modem START indication already received, continue init sequence...")
        /* now reset modem_at_ready (in case of reception of reset indication) */
        p_mdm_ctxt->persist.modem_at_ready = AT_FALSE;

        if (curSID == (at_msg_t) SID_CS_RESET)
        {
          /* reinit context for reset case */
          ATC_Monarch_modem_reset(p_mdm_ctxt);
        }

        /* force flow control */
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
      }
      else
      {
        if (curSID == (at_msg_t) SID_CS_RESET)
        {
          /* reinit context for reset case */
          ATC_Monarch_modem_reset(p_mdm_ctxt);
        }

        PRINT_DBG("Modem START indication not received yet, waiting for it...")
        /* wait for +SYSSTART */
        atcm_program_WAIT_EVENT(p_atp_ctxt, SEQMONARCH_SYSSTART_TIMEOUT, INTERMEDIATE_CMD);
      }
    }
    else if CHECK_STEP((1U))
    {
      /* check if SIM is ready */
      if (p_mdm_ctxt->persist.modem_sim_ready == AT_TRUE)
      {
        PRINT_DBG("Modem SIM indication already received, continue init sequence...")
        p_mdm_ctxt->persist.modem_sim_ready = AT_FALSE;

        /* skip this step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        /* skip this step */
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
    }
    /********************************************************************
      * common power ON/RESET sequence starts here
      * when communication with modem has been successfully established
      ********************************************************************/
    else if CHECK_STEP((common_start_sequence_step))
    {
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
    else if CHECK_STEP((common_start_sequence_step + 1U))
    {
      /* force flow control */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 2U))
    {
      /* disable echo */
      p_mdm_ctxt->CMD_ctxt.command_echo = AT_FALSE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATE, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 3U))
    {
      /* Read FW revision */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR,
                          INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 4U))
    {
      /* request detailed error report */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CMEE, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 5U))
    {
      /* enable full response format */
      p_mdm_ctxt->CMD_ctxt.dce_full_resp_format = AT_TRUE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATV, INTERMEDIATE_CMD);
    }
    /* request detailed error report */
    else if CHECK_STEP((common_start_sequence_step + 6U))
    {
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
    else if CHECK_STEP((common_start_sequence_step + 7U))
    {
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGDCONT, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 8U))
    {
      /* set CFUN = 0 for GM01Q */
      p_mdm_ctxt->CMD_ctxt.cfun_value = 0U;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((common_start_sequence_step + 9U))
    {
      /* force to disable PSM in case modem was switched off with PSM enabled */
      p_mdm_ctxt->SID_ctxt.set_power_config.psm_present = CELLULAR_TRUE;
      p_mdm_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else
  {
    /* manage other RESET types (different from HW)
      */
    PRINT_DBG("Reset type (%d) not supported", p_mdm_ctxt->SID_ctxt.reset_type)
    retval = ATSTATUS_ERROR;
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

  at_status_t retval = ATSTATUS_OK;

  if CHECK_STEP((0U))
  {
    /* hardware power off for this modem
      * will be done by cellular service just after CMD_AT_SQNSSHDN/CMD_AT_SHUTDOWN
      */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_SQNSSHDN, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    PRINT_INFO("***** waiting for shutdown confirmation urc *****")
    /* wait for +SHUTDOWN or +SQNSSSHDN URC before to safely cut device power */
    atcm_program_WAIT_EVENT(p_atp_ctxt, SEQMONARCH_SHUTDOWN_TIMEOUT, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
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
#if (SEQUANS_WAIT_SIM_TEMPO == 0U)
      /* otherwise, wait SIM for a few seconds */
      atcm_program_SKIP_CMD(p_atp_ctxt);
#else /* SEQUANS_WAIT_SIM_TEMPO != 0U */
      atcm_program_TEMPO(p_atp_ctxt, ((uint32_t)SEQUANS_WAIT_SIM_TEMPO), INTERMEDIATE_CMD);
      PRINT_INFO("waiting %d msec before to continue (GM01Q specific)", SEQUANS_WAIT_SIM_TEMPO)
#endif /*(SEQUANS_WAIT_SIM_TEMPO == 0U) */
    }
  }
  else if CHECK_STEP((2U))
  {
    /* request for CCID - only possible under CFUN = 1 or 4 */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_ICCID, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((3U))
  {
    /* check is CPIN is requested */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((4U))
  {
    if (p_mdm_ctxt->persist.sim_pin_code_ready == AT_FALSE)
    {
      PRINT_DBG("CPIN required, we send user value to modem")

      if (strlen((const CRC_CHAR_t *)p_mdm_ctxt->SID_ctxt.modem_init.pincode.pincode) != 0U)
      {
        /* send PIN value */
        atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPIN, FINAL_CMD);
      }
      else
      {
        /* no PIN provided by user */
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      PRINT_DBG("CPIN not required");
      /* no PIN required, end of init */
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
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
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
          atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_ICCID, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CSQ, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CESQ, FINAL_CMD);
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
    /* check if actual registration status is the expected one */
    const CS_OperatorSelector_t *operatorSelect = &(p_mdm_ctxt->SID_ctxt.write_operator_infos);
    if (p_mdm_ctxt->SID_ctxt.read_operator_infos.mode != operatorSelect->mode)
    {
      /* write registration status */
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
    }
    else
    {
      /* skip this step */
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

    /* is an event linked to CEREG ? */
    if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO))
    {
      (void) atcm_subscribe_net_event(p_mdm_ctxt, p_atp_ctxt);
    }
    else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
    {
      /* no command to monitor signal quality with URC in MONARCH */
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

    /* is an event linked to CEREG ? */
    if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO))
    {
      (void) atcm_unsubscribe_net_event(p_mdm_ctxt, p_atp_ctxt);
    }
    else if (urcEvent == CS_URCEVENT_SIGNAL_QUALITY)
    {
      /* no command to monitor signal quality with URC in MONARCH */
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
  if CHECK_STEP((0U))
  {
    /* PDN activation */
    p_mdm_ctxt->CMD_ctxt.pdn_state = PDN_STATE_ACTIVATE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGACT, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* get IP address */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }
#else
  if CHECK_STEP((0U))
  {
    /* PDN activation */
    p_mdm_ctxt->CMD_ctxt.pdn_state = PDN_STATE_ACTIVATE;
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGACT, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* get IP address */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGDATA, FINAL_CMD);
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
  UNUSED(p_mdm_ctxt);
  UNUSED(p_atp_ctxt);

  at_status_t retval;

  /* not implemented yet */
  retval = ATSTATUS_ERROR;

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

  if CHECK_STEP((0U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGDCONT, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGAUTH, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((2U))
  {
    /* nothing to do here */
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

  /* nothing to do here */
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
  atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, FINAL_CMD);


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
    atcm_program_TEMPO(p_atp_ctxt, SEQMONARCH_DATA_SUSPEND_TIMEOUT, INTERMEDIATE_CMD);
  }
  else if CHECK_STEP((1U))
  {
    /* send escape sequence +++ (RAW command type)
      *  CONNECT expected before 1000 ms
      */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_ESC_CMD, FINAL_CMD);
    ATC_Monarch_reinitSyntaxAutomaton_monarch();
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
    /* skip this step */
    atcm_program_SKIP_CMD(p_atp_ctxt);
  }
  else if CHECK_STEP((1U))
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

  /* SOCKET CONNECTION FOR COMMAND DATA MODE */
  if (p_mdm_ctxt->socket_ctxt.p_socket_info != NULL)
  {
    if CHECK_STEP((0U))
    {
      /* reserve a modem CID for this socket_handle */
      socket_handle_t sockHandle = p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle;
      (void) atcm_socket_reserve_modem_cid(p_mdm_ctxt, sockHandle);
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSCFG, INTERMEDIATE_CMD);
      PRINT_INFO("For Client Socket Handle=%ld : MODEM CID affected=%d",
                 sockHandle,
                 p_mdm_ctxt->persist.socket[sockHandle].socket_connId_value)
    }
    else if CHECK_STEP((1U))
    {
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                          ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSCFGEXT, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((2U))
    {
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSD, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((3U))
    {
      /* socket is connected */
      (void) atcm_socket_set_connected(p_mdm_ctxt, p_mdm_ctxt->socket_ctxt.p_socket_info->socket_handle);
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
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                          ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSSENDEXT, INTERMEDIATE_CMD);
    }
  }
  else if CHECK_STEP((1U))
  {
    /* waiting for socket prompt: "<CR><LF>> " */
    if (p_mdm_ctxt->socket_ctxt.socket_send_state == SocketSendState_Prompt_Received)
    {
      PRINT_DBG("SOCKET PROMPT ALREADY RECEIVED")
      /* skip this step */
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
    else
    {
      PRINT_DBG("WAITING FOR SOCKET PROMPT")
      atcm_program_WAIT_EVENT(p_atp_ctxt, SEQMONARCH_SOCKET_PROMPT_TIMEOUT, INTERMEDIATE_CMD);
    }
  }
  else if CHECK_STEP((2U))
  {
    /* socket prompt received, send DATA */
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt,
                        ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_SQNSSEND_WRITE_DATA, FINAL_CMD);

    /* reinit automaton to receive answer */
    ATC_Monarch_reinitSyntaxAutomaton_monarch();
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
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSI, INTERMEDIATE_CMD);
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
      }

      /* receive data */
      p_mdm_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_RequestData_Header;
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSRECV, FINAL_CMD);
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
      atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNSH, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      /* release the modem CID for this socket_handle */
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_SQNSS, FINAL_CMD);
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
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PING, FINAL_CMD);
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
    init_dns_info();
    atcm_program_AT_CMD(p_mdm_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SQNDNSLKUP, FINAL_CMD);
  }
  else
  {
    /* error, invalid step */
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Reset DNS info structure
  * @retval none
  */
static void init_dns_info(void)
{
  (void) memset((void *)monarch_shared.SQNDNSLKUP_dns_info.hostIPaddr, 0, MAX_SIZE_IPADDR);
}
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

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
    if (ATC_Monarch_init_monarch_low_power(p_mdm_ctxt) == AT_FALSE)
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
    if (ATC_Monarch_set_monarch_low_power(p_mdm_ctxt) == AT_FALSE)
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

  if (atcm_modem_event_received(p_mdm_ctxt, CS_MDMEVENT_LP_ENTER) == AT_TRUE)
  {
    /* trigger an internal event to ATCORE (ie an event not received from IPC)
      * to allow to report an URC to upper layers (URC event = enter in LP)
      */
    AT_internalEvent(DEVTYPE_MODEM_CELLULAR);
  }
  atcm_program_NO_MORE_CMD(p_atp_ctxt);

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

  atcm_program_NO_MORE_CMD(p_atp_ctxt);

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

  atcm_program_NO_MORE_CMD(p_atp_ctxt);

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
  UNUSED(p_mdm_ctxt);

  at_status_t retval = ATSTATUS_OK;

  atcm_program_NO_MORE_CMD(p_atp_ctxt);

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
