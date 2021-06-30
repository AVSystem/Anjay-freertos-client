/**
  ******************************************************************************
  * @file    at_custom_modem_signalling.c
  * @author  MCD Application Team
  * @brief   This file provides all the 'signalling' code to the
  *          MURATA-TYPE1SC-EVK module (ALT1250 modem: LTE-cat-M1)
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_core.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_custom_modem_signalling.h"
#include "at_custom_modem_specific.h"
#include "at_datapack.h"
#include "at_util.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

/* Private typedef -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "TYPE1SC:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "TYPE1SC API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "TYPE1SC ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P1, (const CRC_CHAR_t *)pbuf, size);
#else
#define PRINT_INFO(format, args...)  (void) printf("TYPE1SC:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("TYPE1SC ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)  __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_SPECIFIC */

/* START_PARAM_LOOP and END_PARAM_LOOP macros are used to loop on all fields
*  received in a message.
*  Only non-null length fields are analysed.
*  End the analyze when the end of the message or an error has been detected.
*/
#define START_PARAM_LOOP()  uint8_t exitcode = 0U;\
  do {\
    if (atcc_extractElement(p_at_ctxt, p_msg_in, element_infos) != ATENDMSG_NO) {exitcode = 1U;}\
    if (element_infos->str_size != 0U)\
    {\

#define END_PARAM_LOOP()  }\
  if (retval == ATACTION_RSP_ERROR) {exitcode = 1U;}\
  } while (exitcode == 0U);

/* Private defines -----------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static CS_IPaddrType_t atcm_PDNRDP_get_ip_address_type(const uint8_t *p_addr_str,
                                                       uint8_t addr_str_size,
                                                       uint8_t *ip_addr_size);

/* Functions Definition ------------------------------------------------------*/
/* Build command functions ---------------------------------------------------*/
at_status_t fCmdBuild_ATD_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATD_TYPE1SC()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PRINT_INFO("Activate PDN (user cid = %d, modem cid = %d)", (uint8_t)current_conf_id, modem_cid)

    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "*99***%d#", modem_cid);
  }
  return (retval);
}

at_status_t fCmdBuild_SETCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SETCFG_TYPE1SC()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (type1sc_shared.setcfg_function == SETGETCFG_SIM_POLICY)
    {
      /* SIM_INIT_SELECT_POLICY
      *  0 : N/A, single SIM
      *  1 : SIM 1 only
      *  2 : SIM 2 only
      *  3 : SIM 1 with fallback to SIM 2
      *  4 : SIM 2 with fallback to SIM 1
      *  5 : iUICC
      */
      uint8_t asim_selected;
      switch (p_modem_ctxt->persist.sim_selected)
      {
        case CS_MODEM_SIM_SOCKET_0:
          asim_selected = 1;
          break;
        case CS_MODEM_SIM_ESIM_1:
          asim_selected = 2;
          break;
        case CS_STM32_SIM_2:
          PRINT_ERR("not supported yet");
          asim_selected = 0;
          retval = ATSTATUS_ERROR;
          break;
        default:
          asim_selected = 0;
          break;
      }

      if (retval != ATSTATUS_ERROR)
      {
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",\"%d\"",
                       "SIM_INIT_SELECT_POLICY",
                       asim_selected);
      }
    }
    else if (type1sc_shared.setcfg_function == SETGETCFG_HIFC_MODE)
    {
      /* advanced command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.hifc.mode,A\"");
    }
    else if (type1sc_shared.setcfg_function == SETGETCFG_BOOT_EVENT_TRUE)
    {
      /* advanced command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"manager.urcBootEv.enabled\",\"true\"");
    }
    else if (type1sc_shared.setcfg_function == SETGETCFG_BOOT_EVENT_FALSE)
    {
      /* advanced command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"manager.urcBootEv.enabled\",\"false\"");
    }
    else
    {
      PRINT_ERR("setcfg value not implemented !")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_GETCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_GETCFG_TYPE1SC()")

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    switch (type1sc_shared.getcfg_function)
    {
      case SETGETCFG_BAND:
        /* normal command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"BAND\"");
        break;

      case SETGETCFG_OPER:
        /* normal command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"OPER\"");
        break;

      case SETGETCFG_HIFC_MODE:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.hifc.mode\"");
        break;

      case SETGETCFG_PMCONF_SLEEP_MODE:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.conf.sleep_mode\"");
        break;

      case SETGETCFG_PMCONF_MAX_ALLOWED:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"pm.conf.max_allowed_pm_mode\"");
        break;

      case SETGETCFG_UART_FLOW_CONTROL:
        /* advanced command */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"manager.uartB.flowcontrol\"");
        break;

      default:
        retval = ATSTATUS_ERROR;
        break;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_GETSYSCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_GETSYSCFG_TYPE1SC()")

  /* AT%GETSYSCFG=<obj>[,<value>]
   * <obj> :
   *  "SW_CFG.sim.dual_init_select"
   *  "SW_CFG.catm_band_table.band#1" to "SW_CFG.catm_band_table.band#40"
   */

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (type1sc_shared.syscfg_function == SETGETSYSCFG_SIM_POLICY)
    {
      /* build SIM selection command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\"",
                     "SW_CFG.sim.dual_init_select");
    }
    else
    {
      PRINT_DBG("getsyscfg value not implemented (ignored)")
    }
  }

  return (retval);
}

at_status_t fCmdBuild_SETBDELAY_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SETBDELAY_TYPE1SC()")

  (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");

  return (retval);
}

#define APN_NULL_STRING "null"

at_status_t fCmdBuild_PDNSET_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_PDNSET_TYPE1SC()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_CHAR_t *p_apn;
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PRINT_INFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)

    /* build command */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn_present == CELLULAR_TRUE)
    {
      /* use the APN explicitly providedby user */
      p_apn = (CS_CHAR_t *) &p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].apn;
    }
    else
    {
      /* no APN provided by user: use empty APN string (ALTAIR specific) to force the network to
      *  select the appropriate APN.
      */
      p_apn = (CS_CHAR_t *) &APN_NULL_STRING;
    }

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id != CS_PDN_NOT_DEFINED)
    {
      /* check is an username is provided */
      if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username[0] == 0U)
      {
        /* no authentication parameters are provided */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\"",
                       modem_cid,
                       p_apn,
                       atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type));
      }
      else
      {
        /* authentication parameters are provided */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\",\"PAP\",\"%s\",\"%s\"",
                       modem_cid,
                       p_apn,
                       atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type),
                       p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username,
                       p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.password);

      }
    }
    else
    {
      PRINT_ERR("PDP context not defined for conf_id %d", current_conf_id)
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_status_t fCmdBuild_PDNRDP_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_PDNRDP_TYPE1SC()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id == CS_PDN_NOT_DEFINED)
    {
      /* Trace only */
      PRINT_INFO("PDP context not explicitly defined for conf_id %d (using modem params)", current_conf_id)
    }

    /* build the command */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", modem_cid);
  }

  return (retval);
}

at_status_t fCmdBuild_NOTIFYEV_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_NOTIFYEV_TYPE1SC()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (type1sc_shared.notifyev_mode == 1U)
    {
      /* mode = 1
      * activate SIM notifications
      *
      */

      /* build the command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params,
                     "\"SIMREFRESH\",1,\"SIMD\",1,\"SIMSTATE\",1");
    }
    else
    {
      /* mode = 0 or invalid mode value
      * deactivate all notification
      */

      /* build the command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params,
                     "\"ALL\",0");

    }
  }

  return (retval);
}

/* Analyze command functions -------------------------------------------------*/
at_action_rsp_t fRspAnalyze_Error_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval;
  PRINT_API("enter fRspAnalyze_Error_TYPE1SC()")

  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_DIAL_COMMAND:
      /* in case of error during socket connection,
      * release the modem CID for this socket_handle
      */
      (void) atcm_socket_release_modem_cid(p_modem_ctxt, p_modem_ctxt->socket_ctxt.socket_info->socket_handle);
      break;

    default:
      /* nothing to do */
      break;
  }

  /* analyze Error for TYPE1SC */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case CMD_AT_IFC:
      /* error is ignored for backward compatibility:
       *   this command is not implemented in old FW versions
       */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_CREG:
    case CMD_AT_CGREG:
    case CMD_AT_CEREG:
      /* error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_SETCFG:
    case CMD_AT_GETCFG:
    case CMD_AT_SETACFG:
    case CMD_AT_GETACFG:
    case CMD_AT_SETBDELAY:
    case CMD_AT_NOTIFYEV:
      /* modem specific commands, error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_CPIN:
      /* error is ignored when bg96_sim_status_retries is not null
       *
       */
      if (type1sc_shared.sim_status_retries != 0U)
      {
        PRINT_INFO("error ignored (waiting for SIM ready)")
        (void) fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
        retval = ATACTION_RSP_FRC_CONTINUE;
      }
      else
      {
        retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      }
      break;

    default:
      retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      break;
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CPIN_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CPIN_TYPE1SC()")

  /*
  *   analyze parameters for +CPIN
  *
  *   if +CPIN is not received after AT+CPIN request, it's an URC
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CPIN)
  {
    retval = fRspAnalyze_CPIN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
  }
  else
  {
    START_PARAM_LOOP()
    /* this is an URC */
    if (element_infos->param_rank == 2U)
    {
      PRINT_DBG("URC +CPIN received")
      PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_CFUN_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CFUN_TYPE1SC()")

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PRINT_DBG("URC +CFUN received")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_CCID_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fRspAnalyze_QCCID_TYPE1SC()")

  /* analyze parameters for +QCCID */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PRINT_DBG("ICCID:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* if ICCID reported by the modem includes a blank character (space, code=0x20) at the beginning
     *  remove it if this is the case
     */
    uint16_t src_idx = element_infos->str_start_idx;
    size_t ccid_size = element_infos->str_size;
    if ((p_msg_in->buffer[src_idx] == 0x20U) &&
        (ccid_size >= 2U))
    {
      ccid_size -= 1U;
      src_idx += 1U;
    }

    /* copy ICCID */
    (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.device_info->u.iccid),
                  (const void *)&p_msg_in->buffer[src_idx],
                  (size_t) ccid_size);
  }
  else
  {
    /* other parameters ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

at_action_rsp_t fRspAnalyze_GETCFG_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fRspAnalyze_GETCFG_TYPE1SC()")

  /* analyze parameters for GETCFG or GETACFG */
  if (type1sc_shared.getcfg_function == SETGETCFG_UART_FLOW_CONTROL)
  {
    uint32_t hwFC_status = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
    PRINT_DBG("GETCFG/GETACFG: Hw Flow Control setting = %ld", hwFC_status)
    if (hwFC_status == 1U)
    {
      PRINT_DBG("Hw Flow Control setting is ON")
      p_modem_ctxt->persist.flowCtrl_RTS = 2U;
      p_modem_ctxt->persist.flowCtrl_CTS = 2U;
    }
    else
    {
      PRINT_DBG("Hw Flow Control setting is OFF")
      p_modem_ctxt->persist.flowCtrl_RTS = 0U;
      p_modem_ctxt->persist.flowCtrl_CTS = 0U;
    }
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_PDNRDP_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE;
  PRINT_API("enter fCRspAnalyze_PDNRDP_TYPE1SC()")

  /* analyze parameters for CGPADDR */
  /* for WRITE COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /*
    *  format: AT%PDNRDP=<ext_sessionID>
    *
    * response:
    *  [%PDNRDP: <ext_sessionID>,<bearer_id>,<apn>[,<local_addr and subnet_mask>[,
    *     <gw_addr>[,<DNS_prim_addr>[,<DNS_sec_addr>[,<P-CSCF_prim_addr>[,<PCSCF_sec_addr>]]]]]]
    *    [<CR><LF>%PDNRDP: . . .]]
    */

    START_PARAM_LOOP()
    PRINT_DBG("%%PDNRDP param_rank = %d", element_infos->param_rank)
    if (element_infos->param_rank == 2U)
    {
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
      PRINT_DBG("%%PDNRDP cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if (element_infos->param_rank == 5U)
    {
      /* <local_addr and subnet_mask>
       *  3GP TS27.007, same as AT+CGCONTRDP
       *  for IPv4:
       * "a1.a2.a3.a4.m1.m2.m3.m4"
       * for IPv6:
       * "a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a11.a12.a13.a14.a15.a16.m1.m2.m3.m4.m5.
       *  m6.m7.m8.m9.m10.m11.m12.m13.m14.m15.m16"
       */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* determine IP address type and determine how to cut to remove subnet mask */
      uint8_t ip_addr_size = 0U;
      ip_addr_info.ip_addr_type =
        atcm_PDNRDP_get_ip_address_type((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                                        (uint8_t) element_infos->str_size,
                                        (uint8_t *) &ip_addr_size);
      /* extract IP address (remove subnet) */
      if ((ip_addr_size != 0U) &&
          (ip_addr_info.ip_addr_type != CS_IPAT_INVALID))
      {
        (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                      (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                      (size_t) ip_addr_size);

        PRINT_INFO("%%PDNRDP addr=%s (size=%d)", (AT_CHAR_t *)&ip_addr_info.ip_addr_value, ip_addr_size)
      }
    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_NOTIFYEV_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                             const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_NOTIFYEV_TYPE1SC()")

  /* init param received info */
  int32_t event_type = -1; /* event type:
                            * -1 for an invalid -1 means not valid or not supported
                            * 0 for SIM refresh event
                            * 1 for SIM detect event
                            * 2 for SIM state event
                            */
  uint32_t param1 = 0U;
  bool p1_received = false;

  /* analyze parameters */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* event type */
    AT_CHAR_t line[32] = {0U};
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);
    }
    else
    {
      PRINT_ERR("line exceed maximum size, line ignored...")
    }

    /* convert string to test to upper case */
    ATutil_convertStringToUpperCase(&line[0], 32U);

    /* extract value and compare it to expected value */
    if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIMREFRESH") != NULL)
    {
      event_type = 0;
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIMD") != NULL)
    {
      event_type = 1;
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIMSTATE") != NULL)
    {
      event_type = 2;
    }
    else
    {
      /* invalid or not supported */
      event_type = -1;
    }

    PRINT_DBG("NOTIFY_EVENT <event_type> = %ld", event_type)
  }
  else if (element_infos->param_rank == 3U)
  {
    /* parameter 1: analyzed only if corresponding event is supported */
    if (event_type != -1)
    {
      param1 = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                         element_infos->str_size);
      p1_received = true;
      PRINT_DBG("param1 %ld", param1)
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    /* parameter 2: analyzed only if corresponding event is supported
     * for SIMREFRESH and LTIME events only
     * ignored for the moment
     */
  }
  else if (element_infos->param_rank == 5U)
  {
    /* parameter 3: analyzed only if corresponding event is supported
     * for SIMREFRESH and LTIME events only
     * ignored for the moment
     */
  }
  else
  {
    /* other parameters ignored */
  }
  END_PARAM_LOOP()

  /* after analyze, fill report */
  if (event_type != -1)
  {
    retval = ATACTION_RSP_URC_FORWARDED;
    CS_SimEvent_status_t sim_event;
    bool valid_urc_to_report = false;

    switch (event_type)
    {
      case 0: /* SIMREFRESH */
        sim_event.event = CS_SIMEVENT_SIM_REFRESH;
        /* parameters are ignored */
        sim_event.param1 = CS_SIMINFOS_UNKNOWN;
        valid_urc_to_report = true;
        break;

      case 1: /* SIMDSIMD */
        sim_event.event = CS_SIMEVENT_SIM_DETECT;
        if (p1_received)
        {
          if (param1 == 0U)
          {
            sim_event.param1 = CS_SIMINFOS_CARD_REMOVED;
            valid_urc_to_report = true;
          }
          else if (param1 == 1U)
          {
            sim_event.param1 = CS_SIMINFOS_CARD_INSERTED;
            valid_urc_to_report = true;
          }
          else
          {
            sim_event.param1 = CS_SIMINFOS_UNKNOWN;
          }
        }
        break;

      case 2: /* SIMSTATE */
        /* info not forwarded to upper layer */
        break;

      default: /* ERROR */
        PRINT_INFO("ERROR")
        break;
    }

    /* is a valid urc is ready to report ? */
    if (valid_urc_to_report == true)
    {
      atcm_report_urc_sim_event(p_modem_ctxt, &sim_event);
    }
  }

  return (retval);
}

/**
  * @brief  Get IP Address type + extract IP address
  * @param  p_addr_str_tmp Ptr to "IP address + subnet mask" string
  * @param  addr_str_size size of given string
  * @param  ip_addr_size  return size of IP address without subnet mask
  * @retval CS_IPaddrType_t
  */
static CS_IPaddrType_t atcm_PDNRDP_get_ip_address_type(const uint8_t *p_addr_str_tmp,
                                                       uint8_t addr_str_size,
                                                       uint8_t *ip_addr_size)
{
  CS_IPaddrType_t retval;
  *ip_addr_size = 0U;

  /* <local_addr and subnet_mask>
  *  3GP TS27.007, same as AT+CGCONTRDP
  * string type; shows the IP address and subnet mask of the MT. The string
  *        is given as dot-separated numeric (0-255) parameters on the form:
  *  for IPv4:
  *    "a1.a2.a3.a4.m1.m2.m3.m4"
  * for IPv6:
  *    "a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a11.a12.a13.a14.a15.a16.m1.m2.m3.m4.m5.
  *      m6.m7.m8.m9.m10.m11.m12.m13.m14.m15.m16"
  */
  PRINT_DBG("atcm_PDNRDP_get_ip_address_type (%d) %s  ", addr_str_size, p_addr_str_tmp)
  if (p_addr_str_tmp != NULL)
  {
    uint8_t count_dots = 0U;
    uint8_t cpt;

    /* save ptr value
     * we will iterate this pointer
     * before to return, we need to restore pointer to its original value
     */
    const uint8_t *save_addr = p_addr_str_tmp;

    /* count number of dots in address */
    for (cpt = 0; cpt < addr_str_size; cpt++)
    {
      if (*p_addr_str_tmp == ((AT_CHAR_t)'.'))
      {
        count_dots++;
      }
      /* next character */
      p_addr_str_tmp++;
    }

    /* restore ptr value for parsing same string again */
    p_addr_str_tmp = save_addr;

    /* analyze result */
    if (count_dots == 7U)
    {
      retval = CS_IPAT_IPV4;
    }
    else if (count_dots == 30U)
    {
      retval = CS_IPAT_IPV6;
    }
    else
    {
      retval = CS_IPAT_INVALID;
    }

    /* now that IP type is known, parse again to
    * split Ip address and subnet mask
    */
    count_dots = 0U;
    if (retval != CS_IPAT_INVALID)
    {
      cpt = 0U;
      do
      {
        if (*p_addr_str_tmp == ((AT_CHAR_t)'.'))
        {
          count_dots++;
        }
        /* if IPV4, cut before 4th dot*/
        if ((retval == CS_IPAT_IPV4) && (count_dots == 4U))
        {
          *ip_addr_size = cpt;
        }
        /* if IPV6, cut before 16th dot*/
        else if ((retval == CS_IPAT_IPV4) && (count_dots == 16U))
        {
          *ip_addr_size = cpt;
        }
        else
        {
          /* nothing to do */
        }

        /* next character */
        cpt++;
        p_addr_str_tmp++;
      } while ((*ip_addr_size == 0U) && (cpt < addr_str_size));

      /* restore original ptr value */
      p_addr_str_tmp = save_addr; /* MISRA: ignore error, this ptr is returned to caller with its original value */
    }
  }
  else
  {
    retval = CS_IPAT_INVALID;
  }

  PRINT_DBG("ip_addr_size (%d) type=%d", *ip_addr_size, retval)
  return (retval);
}


#define T1SC_GETMIN(a,b) (((a)>(b))?(b):(a))
#define T1SC_STR_OBJ_SIM     "SW_CFG.sim.dual_init_select"
#define T1SC_STR_VALUE_SIM_1 "SIM1_ONLY"
#define T1SC_STR_VALUE_SIM_2 "SIM2_ONLY"
#define T1SC_MAX_SETGETSYSCFG_SIZE 30

at_status_t fCmdBuild_SETSYSCFG_TYPE1SC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_SETSYSCFG_TYPE1SC()")

  /* AT%SETSYSCFG=<obj>[,<value>]
   * <obj> :
   *  "SW_CFG.sim.dual_init_select"
   *  "SW_CFG.catm_band_table.band#1" to "SW_CFG.catm_band_table.band#40"
   * <value> depends of <obj> context (cf modem documentation)
   */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (type1sc_shared.syscfg_function == SETGETSYSCFG_SIM_POLICY)
    {
      /* SW_CFG.sim.dual_init_select
       * "DUAL_SIM2_FALLBACK" – Use external SIM if present. Otherwise, use internal eSIM.
       * "SIM1_ONLY" – External SIM only
       * "SIM2_ONLY" – Internal eSIM only
       */
      const CRC_CHAR_t *param_obj;
      switch (p_modem_ctxt->persist.sim_selected)
      {
        case CS_MODEM_SIM_SOCKET_0:
          param_obj = T1SC_STR_VALUE_SIM_1;
          break;
        case CS_MODEM_SIM_ESIM_1:
          param_obj = T1SC_STR_VALUE_SIM_2;
          break;
        case CS_STM32_SIM_2:
          PRINT_ERR("not supported, use default value");
          param_obj = "";
          break;
        default:
          param_obj = "";
          break;
      }

      /* build SIM selection command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",\"%s\"",
                     T1SC_STR_OBJ_SIM,
                     param_obj);
    }
    else
    {
      PRINT_ERR("setsyscfg value not implemented")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

at_action_rsp_t fRspAnalyze_GETSYSCFG_TYPE1SC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                              const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fRspAnalyze_GETSYSCFG_TYPE1SC()")

  ATCustom_T1SC_SETGETSYSCFG_t receved_obj_type = SETGETSYSCFG_UNDEFINED;
  uint8_t cleanStrArray[T1SC_MAX_SETGETSYSCFG_SIZE];
  uint16_t cleanStrSize;

  /* set default value */
  type1sc_shared.modem_sim_same_as_selected = true;

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    /* extract <obj> */
    (void) memset((void *)cleanStrArray, 0, T1SC_MAX_SETGETSYSCFG_SIZE);
    cleanStrSize = ATutil_extract_str_from_quotes(
                     (const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                     element_infos->str_size,
                     cleanStrArray,
                     T1SC_MAX_SETGETSYSCFG_SIZE);

    /* check is we received SIM obj field */
    if (memcmp(cleanStrArray, T1SC_STR_OBJ_SIM,  T1SC_GETMIN(cleanStrSize, sizeof(T1SC_STR_OBJ_SIM))) == 0)
    {
      receved_obj_type = SETGETSYSCFG_SIM_POLICY;
    }
  }
  else  if (element_infos->param_rank == 3U)
  {
    /* extract <value> */
    if (receved_obj_type == SETGETSYSCFG_SIM_POLICY)
    {
      (void) memset((void *)cleanStrArray, 0, T1SC_MAX_SETGETSYSCFG_SIZE);
      cleanStrSize = ATutil_extract_str_from_quotes(
                       (const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                       element_infos->str_size,
                       cleanStrArray,
                       T1SC_MAX_SETGETSYSCFG_SIZE);

      if (memcmp(cleanStrArray, T1SC_STR_VALUE_SIM_1, T1SC_GETMIN(cleanStrSize, sizeof(T1SC_STR_VALUE_SIM_1))) == 0)
      {
        /* check if requested SIM is the same as the one configured in the modem */
        if (p_modem_ctxt->persist.sim_selected != CS_MODEM_SIM_SOCKET_0)
        {
          type1sc_shared.modem_sim_same_as_selected = false;
        }

      }
      else if (memcmp(cleanStrArray, T1SC_STR_VALUE_SIM_2, T1SC_GETMIN(cleanStrSize,
                                                                       sizeof(T1SC_STR_VALUE_SIM_2))) == 0)
      {
        /* check if requested SIM is the same as the one configured in the modem */
        if (p_modem_ctxt->persist.sim_selected != CS_MODEM_SIM_ESIM_1)
        {
          type1sc_shared.modem_sim_same_as_selected = false;
        }
      }
      else
      {
        PRINT_DBG("SIM value not supported");
        type1sc_shared.modem_sim_same_as_selected = false;
      }
    }
  }
  else
  {
    /* other parameters ignored */
  }
  END_PARAM_LOOP()

  return (retval);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

