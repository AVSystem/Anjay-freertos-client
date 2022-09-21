/**
  ******************************************************************************
  * @file    at_signalling_analyze.c
  * @author  MCD Application Team
  * @brief   This file provides common code for the modems
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2021 STMicroelectronics.
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
#include "at_modem_common.h"
#include "at_signalling.h"
#include "at_signalling_analyze.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_sysctrl.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SIGNALLING_ANALYZE AT_CORE SIGNALLING ANALYZE
  * @{
  */

/** @defgroup AT_CORE_SIGNALLING_ANALYZE_Private_Macros AT_CORE SIGNALLING ANALYZE Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_MODEM == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCModem:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCModem:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCModem API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCModem ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P1, (const CRC_CHAR_t *)pbuf, size);
#else
#define PRINT_INFO(format, args...)  (void) printf("ATCModem:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("ATCModem ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#define PRINT_BUF(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_MODEM */

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

#define ATC_GET_MINIMUM_SIZE(a,b) (((a)<(b))?(a):(b))
/**
  * @}
  */

/** @defgroup AT_CORE_SIGNALLING_ANALYZE_Private_Defines AT_CORE SIGNALLING ANALYZE Private Defines
  * @{
  */
#define USE_COPS_ACT (1U) /* indicates if AT+COPS uses AcT parameter
                         *  0U if AcT ignored / 1U if AcT is used
                         */
#define MAX_CGEV_PARAM_SIZE         (32U)
#define UNKNOWN_NETWORK_TYPE (uint8_t)(0x0) /* should match to NETWORK_TYPE_LUT */
#define CS_NETWORK_TYPE      (uint8_t)(0x1) /* should match to NETWORK_TYPE_LUT */
#define GPRS_NETWORK_TYPE    (uint8_t)(0x2) /* should match to NETWORK_TYPE_LUT */
#define EPS_NETWORK_TYPE     (uint8_t)(0x3) /* should match to NETWORK_TYPE_LUT */

#define LAC_TAC_SIZE   4 /* size = 2 bytes -> 4 half-bytes */
#define CI_SIZE        8 /* size = 4 bytes -> 8 half_bytes */
#define RAC_SIZE       2 /* size = 1 byte  -> 2 half-bytes */
#define APN_EMPTY_STRING ""
/**
  * @}
  */

/** @defgroup AT_CORE_SIGNALLING_ANALYZE_Private_Functions_Prototypes
  *     AT_CORE SIGNALLING ANALYZE Private Functions Prototypes
  * @{
  */
static void display_clear_network_state(CS_NetworkRegState_t state, uint8_t network_type);
static CS_NetworkRegState_t convert_NetworkState(uint32_t state, uint8_t network_type);
static CS_PDN_conf_id_t find_user_cid_with_matching_ip_addr(atcustom_persistent_context_t *p_persistent_ctxt,
                                                            csint_ip_addr_info_t *ip_addr_struct);
static at_action_rsp_t analyze_CmeError(at_context_t *p_at_ctxt,
                                        atcustom_modem_context_t *p_modem_ctxt,
                                        const IPC_RxMessage_t *p_msg_in,
                                        at_element_info_t *element_infos);
/**
  * @}
  */

/** @defgroup AT_CORE_SIGNALLING_ANALYZE_Exported_Functions AT_CORE SIGNALLING ANALYZE Exported Functions
  * @{
  */
/* ==========================  Analyze 3GPP TS 27.007 commands ========================== */
/**
  * @brief  Analyze specific modem response : Without parameters.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_None(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_None()")

  /* no parameters expected */

  return (retval);
}

/**
  * @brief  Analyze specific modem response : ERROR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_Error(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PRINT_API("enter fRspAnalyze_Error()")

  /* analyse parameters for ERROR */
  /* use CmeErr function for the moment */
  retval = fRspAnalyze_CmeErr(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CME ERROR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CmeErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  const atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  /*UNUSED(p_msg_in);*/
  /*UNUSED(element_infos);*/

  at_action_rsp_t retval = ATACTION_RSP_ERROR;
  PRINT_API("enter fRspAnalyze_CmeErr()")

  /* Analyze CME error to report it to upper layers */
  (void) analyze_CmeError(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  /* specific treatments for +CME ERROR, depending of current command */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    /* Error for Serial Number, IMEI, IMEISV or SVN */
    case CMD_AT_CGSN:
    {
      /* used for Serial Number */
      if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SN)
      {
        if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
        {
          PRINT_DBG("Modem Error for CGSN_SN, use uninitialized value")
          (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.serial_number), 0, MAX_SIZE_SN);
        }
      }
      /* used for IMEI */
      else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEI)
      {
        if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
        {
          PRINT_DBG("Modem Error for CGSN_IMEI, use uninitialized value")
          (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.imei), 0, MAX_SIZE_IMEI);
        }
      }
      /* used for IMEISV */
      else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEISV)
      {
        PRINT_DBG("Modem Error for CGSN_IMEISV, use uninitialized value, parameter ignored")
      }
      /* used for SVN */
      else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SVN)
      {
        PRINT_DBG("Modem Error for CGSN_SVN, use uninitialized value, parameter ignored")
      }
      else
      {
        PRINT_DBG("Modem Error for CGSN, unexpected parameter")
        retval = ATACTION_RSP_ERROR;
      }
      break;
    }

    /* Error for IMSI */
    case CMD_AT_CIMI:
      if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
      {
        PRINT_DBG("Modem Error for CIMI, use uninitialized value")
        (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.imsi), 0, MAX_SIZE_IMSI);
      }
      break;

    /* Error for Manufacturer Name */
    case CMD_AT_CGMI:
      if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
      {
        PRINT_DBG("Modem Error for CGMI, use uninitialized value")
        (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.manufacturer_name), 0,
                      MAX_SIZE_MANUFACT_NAME);
      }
      break;

    /* Error for Model */
    case CMD_AT_CGMM:
      if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
      {
        PRINT_DBG("Modem Error for CGMM, use uninitialized value")
        (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.model), 0, MAX_SIZE_MODEL);
      }
      break;

    /* Error for Revision */
    case CMD_AT_CGMR:
      if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
      {
        PRINT_DBG("Modem Error for CGMR, use uninitialized value")
        (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.revision), 0, MAX_SIZE_REV);
      }
      break;

    /* Error for Phone Number */
    case CMD_AT_CNUM:
      if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
      {
        PRINT_DBG("Modem Error for CNUM, use uninitialized value")
        (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.phone_number), 0, MAX_SIZE_PHONE_NBR);
      }
      break;

    /* Error for IMEI */
    case CMD_AT_GSN:
      if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
      {
        PRINT_DBG("Modem Error for GSN, use uninitialized value")
        (void) memset((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.imei), 0, MAX_SIZE_IMEI);
      }
      break;

    /* Error for CPIN */
    case CMD_AT_CPIN:
      PRINT_DBG("Analyze Modem Error for CPIN")
      break;

    /* consider all other error cases for AT commands
      * case ?:
      * etc...
      */

    default:
      PRINT_DBG("Modem Error for cmd (id=%ld)", p_atp_ctxt->current_atcmd.id)
      retval = ATACTION_RSP_ERROR;
      break;
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CMS ERROR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CmsErr(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CmsErr()")

  /* analyze parameters for +CMS ERROR */
  /* Not implemented */

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGMI.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGMI()")

  /* analyze parameters for +CGMI */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PRINT_DBG("Manufacturer name:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.manufacturer_name),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t)element_infos->str_size);
    }
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGMM.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGMM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGMM()")

  /* analyze parameters for +CGMM */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PRINT_DBG("Model:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.model),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t)element_infos->str_size);
    }
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGMR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGMR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGMR()")

  /* analyze parameters for +CGMR */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PRINT_DBG("Revision:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.revision),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t)element_infos->str_size);
    }
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGSN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGSN()")

  /* analyze parameters for +CGSN */

  /* Serial Number (response to AT+CGSN or AT+CGSN=0)
   * modem answer = SN without +CGSN as prefix
   */
  if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SN)
  {
    PRINT_DBG("Serial Number:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.serial_number),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
    }
  }
  /* IMEI, IMEISV, SVN (response to AT+CGSN=1, AT+CGSN=2 or AT+CGSN=3)
   * modem answer = +CGSN: as prefix followed by the requested value
   */
  else if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEI)
      {
        /* IMEI */
        PRINT_DBG("IMEI:")
        PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

        if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
        {
          uint8_t tmp_array[MAX_SIZE_IMEI] = {0};
          uint16_t real_size =
            ATutil_extract_str_from_quotes((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx],
                                           element_infos->str_size,
                                           &tmp_array[0],
                                           element_infos->str_size);

          (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.imei),
                        (const void *)&tmp_array[0],
                        (size_t) real_size);
        }
      }
      else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_IMEISV)
      {
        /* IMEISV */
        PRINT_DBG("IMEISV (NOT USED):")
        PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
      }
      else if (p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param == CGSN_SVN)
      {
        /* SVN */
        PRINT_DBG("SVN (NOT USED):")
        PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
      }
      else
      {
        /* nothing to do */
      }
    }
    END_PARAM_LOOP()
  }
  else
  {
    /* nothing to do */
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CIMI.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CIMI(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CIMI()")

  /* analyze parameters for +CIMI */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PRINT_DBG("IMSI:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.imsi),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
    }
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CEER.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CEER(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CEER()")

  /* analyze parameters for CEER */

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CPIN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CPIN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CPIN()")

  /* analyze parameters for CPIN */
  START_PARAM_LOOP()
  /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0U};
    PRINT_DBG("CPIN parameter received:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);

      /* extract value and compare it to expected value */
      if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PIN") != NULL)
      {
        /* waiting for SIM PIN */
        p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
        p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
        atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PUK") != NULL)
      {
        /* waiting for SIM PUK */
        p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
        p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
        atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PIN2") != NULL)
      {
        /* waiting for SIM PUK2 */
        p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
        p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK2_REQUIRED;
        atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PUK2") != NULL)
      {
        /* waiting for SIM PUK */
        p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
        p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
        atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "READY") != NULL)
      {
        /* CPIN READY */
        p_modem_ctxt->persist.sim_pin_code_ready = AT_TRUE;
        p_modem_ctxt->persist.sim_state = CS_SIMSTATE_READY;
      }
      else
      {
        /* UNEXPECTED CPIN STATE */
        PRINT_ERR("UNEXPECTED CPIN STATE")
        p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
        p_modem_ctxt->persist.sim_state = CS_SIMSTATE_UNKNOWN;
        atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
      }
    }
    else
    {
      PRINT_ERR("line exceed maximum size, line ignored...")
      retval = ATACTION_RSP_IGNORED;
    }

  }
  END_PARAM_LOOP()
  return (retval);
}

/**
  * @brief  Analyze specific modem response : CFUN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CFUN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CFUN()")

  /* analyze parameters for +CFUN
  *  answer to CFUN read command
  *     +CFUN: <state>
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      uint32_t cfun_status = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                       element_infos->str_size);
      if (cfun_status == 1U)
      {
        p_modem_ctxt->SID_ctxt.cfun_status = CS_CMI_FULL;
      }
      else if (cfun_status == 4U)
      {
        p_modem_ctxt->SID_ctxt.cfun_status = CS_CMI_SIM_ONLY;
      }
      else
      {
        /* default value ( if equal to O or anything else) */
        p_modem_ctxt->SID_ctxt.cfun_status = CS_CMI_MINI;
      }
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : COPS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_COPS(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_COPS()")

  /* analyze parameters for +COPS
    *  Different cases to consider (as format is different)
    *  1/ answer to COPS read command
    *     +COPS: <mode>[,<format>,<oper>[,<AcT>]]
    *  2/ answer to COPS test command
    *     +COPS: [list of supported (<stat>,long alphanumeric <oper>,
    *            short alphanumeric <oper>,numeric <oper>[,<AcT>])s]
    *            [,,(list ofsupported <mode>s),(list of supported <format>s)]
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      /* mode (mandatory) */
      uint32_t mode = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
      switch (mode)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_AUTO;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_MANUAL;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_DEREGISTER;
          break;
        case 4:
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_MANUAL_THEN_AUTO;
          break;
        default:
          PRINT_ERR("invalid mode value in +COPS")
          p_modem_ctxt->SID_ctxt.read_operator_infos.mode = CS_NRM_AUTO;
          break;
      }

      PRINT_DBG("+COPS: mode = %d", p_modem_ctxt->SID_ctxt.read_operator_infos.mode)
    }
    else if (element_infos->param_rank == 3U)
    {
      /* format (optional) */
      uint32_t format = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_FORMAT_PRESENT; /* bitfield */
      switch (format)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_LONG;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_SHORT;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_NUMERIC;
          break;
        default:
          PRINT_ERR("invalid format value")
          p_modem_ctxt->SID_ctxt.read_operator_infos.format = CS_ONF_NOT_PRESENT;
          break;
      }
      PRINT_DBG("+COPS: format = %d", p_modem_ctxt->SID_ctxt.read_operator_infos.format)
    }
    else if (element_infos->param_rank == 4U)
    {
      /* operator name (optional) */
      if (element_infos->str_size <= MAX_SIZE_OPERATOR_NAME)
      {
        p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |=
          CS_RSF_OPERATOR_NAME_PRESENT; /* bitfield */
        (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.read_operator_infos.operator_name[0]),
                      (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                      (size_t) element_infos->str_size);
        PRINT_DBG("+COPS: operator name = %s", p_modem_ctxt->SID_ctxt.read_operator_infos.operator_name)
      }
      else
      {
        PRINT_ERR("error, operator name too long")
        retval = ATACTION_RSP_ERROR;
      }
    }
    else if (element_infos->param_rank == 5U)
    {
      /* AccessTechno (optional) */
      p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence |= CS_RSF_ACT_PRESENT;  /* bitfield */
      uint32_t AcT = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size);
      switch (AcT)
      {
        case 0:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM;
          break;
        case 1:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM_COMPACT;
          break;
        case 2:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN;
          break;
        case 3:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_GSM_EDGE;
          break;
        case 4:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSDPA;
          break;
        case 5:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSUPA;
          break;
        case 6:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_UTRAN_HSDPA_HSUPA;
          break;
        case 7:
          PRINT_DBG(">>> Access Technology : LTE Cat.M1 <<<")
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN;
          break;
        case 8:
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_EC_GSM_IOT;
          break;
        case 9:
          PRINT_DBG(">>> Access Technology : LTE Cat.NB1 <<<")
          p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN_NBS1;
          break;
        default:
          PRINT_ERR("invalid AcT value")
          break;
      }

      PRINT_DBG("+COPS: Access technology = %ld", AcT)

    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PRINT_DBG("+COPS for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CNUM.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CNUM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fCmdBuild_CNUM()")

  PRINT_DBG("+CNUM cmd NOT IMPLEMENTED")

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGATT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGATT(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGATT()")

  /* analyze parameters for +CGATT
  *  Different cases to consider (as format is different)
  *  1/ answer to CGATT read command
  *     +CGATT: <state>
  *  2/ answer to CGATT test command
  *     +CGATT: (list of supported <state>s)
  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      uint32_t attach = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
      p_modem_ctxt->SID_ctxt.attach_status = (attach == 1U) ? CS_PS_ATTACHED : CS_PS_DETACHED;
      PRINT_DBG("attach status = %d", p_modem_ctxt->SID_ctxt.attach_status)
    }
    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PRINT_DBG("+CGATT for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CREG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CREG()")
  PRINT_DBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CREG read command
  *     +CREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
  *  2/ answer to CREG test command
  *     +CREG: (list of supported <n>s)
  *  3/ URC:
  *     +CREG: <stat>[,[<lac>],[<ci>],[<AcT>]]
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CREG)
  {
    /* analyze parameters for +CREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PRINT_DBG("+CREG: n=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                            element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
        p_modem_ctxt->persist.cs_network_state = convert_NetworkState(stat, CS_NETWORK_TYPE);
        PRINT_DBG("+CREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t lac = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                            element_infos->str_size, LAC_TAC_SIZE);
        p_modem_ctxt->persist.cs_location_info.lac = (uint16_t)lac;
        PRINT_INFO("+CREG: lac=%ld =0x%lx", lac, lac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                           element_infos->str_size, CI_SIZE);
        p_modem_ctxt->persist.cs_location_info.ci = (uint32_t)ci;
        PRINT_INFO("+CREG: ci=%ld =0x%lx", ci, ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PRINT_DBG("+CREG: act=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* other parameters are not supported yet */
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PRINT_DBG("+CREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }

  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_cs_network_registration = AT_TRUE;
      p_modem_ctxt->persist.cs_network_state = convert_NetworkState(stat, CS_NETWORK_TYPE);
      PRINT_DBG("+CREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t lac = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                          element_infos->str_size, LAC_TAC_SIZE);
      p_modem_ctxt->persist.urc_avail_cs_location_info_lac = AT_TRUE;
      p_modem_ctxt->persist.cs_location_info.lac = (uint16_t)lac;
      PRINT_INFO("+CREG URC: lac=%ld =0x%lx", lac, lac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                         element_infos->str_size, CI_SIZE);
      p_modem_ctxt->persist.urc_avail_cs_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.cs_location_info.ci = (uint32_t)ci;
      PRINT_INFO("+CREG URC: ci=%ld =0x%lx", ci, ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PRINT_DBG("+CREG URC: act=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                          element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGREG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGREG()")
  PRINT_DBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CGREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CGREG read command
  *     +CGREG: <n>,<stat>[,[<lac>],[<ci>],[<AcT>, [<rac>] [,<cause_type>,<reject_cause>]]]
  *  2/ answer to CGREG test command
  *     +CGREG: (list of supported <n>s)
  *  3/ URC:
  *     +CGREG: <stat>[,[<lac>],[<ci>],[<AcT>],[<rac>]]
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CGREG)
  {
    /* analyze parameters for +CREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      START_PARAM_LOOP()
      /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
      if (element_infos->param_rank == 2U)
      {
        /* param traced only */
        PRINT_DBG("+CGREG: n=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
        p_modem_ctxt->persist.gprs_network_state = convert_NetworkState(stat, GPRS_NETWORK_TYPE);
        PRINT_DBG("+CGREG: stat=%ld", stat)
      }
      if (element_infos->param_rank == 4U)
      {
        uint32_t lac = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                            element_infos->str_size, LAC_TAC_SIZE);
        p_modem_ctxt->persist.gprs_location_info.lac = (uint16_t)lac;
        PRINT_INFO("+CGREG: lac=%ld =0x%lx", lac, lac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                           element_infos->str_size, CI_SIZE);
        p_modem_ctxt->persist.gprs_location_info.ci = (uint32_t)ci;
        PRINT_INFO("+CGREG: ci=%ld =0x%lx", ci, ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PRINT_DBG("+CGREG: act=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 7U)
      {
        /* param traced only */
        PRINT_DBG("+CGREG: rac=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 8U)
      {
        /* param traced only */
        PRINT_DBG("+CGREG: cause_type=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 9U)
      {
        /* param traced only */
        PRINT_DBG("+CGREG: reject_cause=%ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      if (element_infos->param_rank == 10U)
      {
        /* parameter present only if n=4 or 5
         * active_time */
        PRINT_INFO("+CGREG: active_time= 0x%lx)",
                   ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size, 8))
      }
      if (element_infos->param_rank == 11U)
      {
        /* parameter present only if n=4 or 5
         * periodic_rau */
        PRINT_INFO("+CGREG: periodic_rau= 0x%lx",
                   ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size, 8))
      }
      if (element_infos->param_rank == 12U)
      {
        /* parameter present only if n=4 or 5
         * gprs_ready_timer */
        PRINT_INFO("+CGREG: gprs_ready_timer= 0x%lx",
                   ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size, 8))
      }
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CGREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PRINT_DBG("+CGREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }
  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_gprs_network_registration = AT_TRUE;
      p_modem_ctxt->persist.gprs_network_state = convert_NetworkState(stat, GPRS_NETWORK_TYPE);
      PRINT_DBG("+CGREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t lac = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                          element_infos->str_size, LAC_TAC_SIZE);
      p_modem_ctxt->persist.urc_avail_gprs_location_info_lac = AT_TRUE;
      p_modem_ctxt->persist.gprs_location_info.lac = (uint16_t)lac;
      PRINT_INFO("+CGREG URC: lac=%ld =0x%lx", lac, lac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                         element_infos->str_size, CI_SIZE);
      p_modem_ctxt->persist.urc_avail_gprs_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.gprs_location_info.ci = (uint32_t)ci;
      PRINT_INFO("+CGREG URC: ci=%ld =0x%lx", ci, ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PRINT_DBG("+CGREG URC: act=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 6U)
    {
      /* param traced only */
      PRINT_DBG("+CGREG URC: rac=%ld",
                ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size, RAC_SIZE))
    }
    if (element_infos->param_rank == 7U)
    {
      /* param traced only */
      PRINT_DBG("+CGREG URC: cause_type=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 8U)
    {
      /* param traced only */
      PRINT_DBG("+CGREG URC: reject_cause=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 9U)
    {
      /* active_time */
      PRINT_INFO("+CGREG URC: active_time= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    if (element_infos->param_rank == 10U)
    {
      /* periodic_rau */
      PRINT_INFO("+CGREG URC: periodic_rau= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    if (element_infos->param_rank == 11U)
    {
      /* gprs_ready_timer */
      PRINT_INFO("+CGREG URC: gprs_ready_timer= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    END_PARAM_LOOP()
  }

  return (retval);

}

/**
  * @brief  Analyze specific modem response : CEREG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CEREG(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CEREG()")
  PRINT_DBG("current cmd = %ld", p_atp_ctxt->current_atcmd.id)

  /* analyze parameters for +CEREG
  *  Different cases to consider (as format is different)
  *  1/ answer to CEREG read command
  *   when <n>=0, 1, 2 or 3 and command successful:
  *    +CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
  *   when <n>=4 or 5 and command successful:
  *    +CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<ActiveTime>],[<Periodic-TAU>]]]]
  *  2/ answer to CEREG test command
  *    +CEREG: (list of supported <n>s)
  *  3/ URC:
  *    +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]]
  */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CEREG)
  {
    /* analyze parameters for +CEREG */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      uint32_t n_val = 0U;

      START_PARAM_LOOP()
      /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
      if (element_infos->param_rank == 2U)
      {
        /* <n> parameter */
        n_val = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size);
        PRINT_DBG("+CEREG: n=%ld", n_val)
      }
      if (element_infos->param_rank == 3U)
      {
        uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
        p_modem_ctxt->persist.eps_network_state = convert_NetworkState(stat, EPS_NETWORK_TYPE);
        PRINT_DBG("+CEREG: stat=%ld", stat)
      }

      if (element_infos->param_rank == 4U)
      {
        uint32_t tac = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                            element_infos->str_size, LAC_TAC_SIZE);
        p_modem_ctxt->persist.eps_location_info.lac = (uint16_t)tac;
        PRINT_INFO("+CEREG: tac=%ld =0x%lx", tac, tac)
      }
      if (element_infos->param_rank == 5U)
      {
        uint32_t ci = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                           element_infos->str_size, CI_SIZE);
        p_modem_ctxt->persist.eps_location_info.ci = (uint32_t)ci;
        PRINT_INFO("+CEREG: ci=%ld =0x%lx", ci, ci)
      }
      if (element_infos->param_rank == 6U)
      {
        /* param traced only */
        PRINT_INFO("+CEREG: act=%ld",
                   ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
      }
      /* for other parameters, two cases to consider:
       * n=(0,1,2 or 3) or n=(4 or 5)
       */
      if (n_val <= 3U)
      {
        if (element_infos->param_rank == 7U)
        {
          /* param traced only */
          PRINT_DBG("+CEREG: cause_type=%ld",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
        }
        if (element_infos->param_rank == 8U)
        {
          /* param traced only */
          PRINT_DBG("+CEREG: reject_cause=%ld",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
        }
      }
      else if ((n_val == 4U) || (n_val == 5U))
      {
        if (element_infos->param_rank == 7U)
        {
          /* param traced only */
          PRINT_DBG("+CEREG: cause_type=%ld",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
        }
        if (element_infos->param_rank == 8U)
        {
          /* param traced only */
          PRINT_DBG("+CEREG: reject_cause=%ld",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
        }
        if (element_infos->param_rank == 9U)
        {
          /* active_time */
          uint32_t t3324_bin = ATutil_extract_bin_value_from_quotes(
                                 &p_msg_in->buffer[element_infos->str_start_idx],
                                 element_infos->str_size, 8);
          uint32_t t3324_value = ATutil_convert_T3324_to_seconds(t3324_bin);
          if (t3324_value != p_modem_ctxt->persist.low_power_status.nwk_active_time)
          {
            p_modem_ctxt->persist.low_power_status.nwk_active_time = t3324_value;
            p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
            PRINT_INFO("New T3324 value detected")
          }
          PRINT_INFO("+CEREG: active_time= %ld sec [0x%lx]", t3324_value, t3324_bin)
        }
        if (element_infos->param_rank == 10U)
        {
          /* periodic_tau */
          uint32_t t3412_bin = ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                                    element_infos->str_size, 8);
          uint32_t t3412_value = ATutil_convert_T3412_to_seconds(t3412_bin);
          if (t3412_value != p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU)
          {
            p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU = t3412_value;
            p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
            PRINT_INFO("New T3412 value detected")
          }
          PRINT_INFO("+CEREG: periodic_tau= %ld sec [0x%lx]", t3412_value, t3412_bin)
        }
      }
      else
      { /* unexpected n value, ignore it */}
      END_PARAM_LOOP()
    }
    /* analyze parameters for +CEREG */
    else if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
    {
      PRINT_DBG("+CEREG for test cmd NOT IMPLEMENTED")
    }
    else
    {
      /* nothing to do */
    }
  }
  else
  {
    /* this is an URC */
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      uint32_t stat = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
      p_modem_ctxt->persist.urc_avail_eps_network_registration = AT_TRUE;
      p_modem_ctxt->persist.eps_network_state = convert_NetworkState(stat, EPS_NETWORK_TYPE);
      PRINT_DBG("+CEREG URC: stat=%ld", stat)
    }
    if (element_infos->param_rank == 3U)
    {
      uint32_t tac = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                          element_infos->str_size, LAC_TAC_SIZE);
      p_modem_ctxt->persist.urc_avail_eps_location_info_tac = AT_TRUE;
      p_modem_ctxt->persist.eps_location_info.lac = (uint16_t)tac;
      PRINT_INFO("+CEREG URC: tac=%ld =0x%lx", tac, tac)
    }
    if (element_infos->param_rank == 4U)
    {
      uint32_t ci = ATutil_extract_hex_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                         element_infos->str_size, CI_SIZE);
      p_modem_ctxt->persist.urc_avail_eps_location_info_ci = AT_TRUE;
      p_modem_ctxt->persist.eps_location_info.ci = (uint32_t)ci;
      PRINT_INFO("+CEREG URC: ci=%ld =0x%lx", ci, ci)
    }
    if (element_infos->param_rank == 5U)
    {
      /* param traced only */
      PRINT_DBG("+CEREG URC: act=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 6U)
    {
      /* param traced only */
      PRINT_DBG("+CEREG URC: cause_type=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 7U)
    {
      /* param traced only */
      PRINT_DBG("+CEREG URC: reject_cause=%ld",
                ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    if (element_infos->param_rank == 8U)
    {
      /* active_time */
      uint32_t t3324_bin = ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                                element_infos->str_size, 8);
      uint32_t t3324_value = ATutil_convert_T3324_to_seconds(t3324_bin);
      if (t3324_value != p_modem_ctxt->persist.low_power_status.nwk_active_time)
      {
        p_modem_ctxt->persist.low_power_status.nwk_active_time = t3324_value;
        p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
        PRINT_INFO("New T3324 value detected")
      }
      PRINT_INFO("+CEREG URC: active_time= %ld sec [0x%lx]", t3324_value, t3324_bin)
    }
    if (element_infos->param_rank == 9U)
    {
      /* periodic_tau */
      uint32_t t3412_bin = ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                                element_infos->str_size, 8);
      uint32_t t3412_value = ATutil_convert_T3412_to_seconds(t3412_bin);
      if (t3412_value != p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU)
      {
        p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU = t3412_value;
        p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
        PRINT_INFO("New T3412 value detected")
      }
      PRINT_INFO("+CEREG URC: periodic_tau= %ld sec [0x%lx]", t3412_value, t3412_bin)
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGEV
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGEV(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGEV()")

  /* cf 3GPP TS 27.007 */
  /* this is an URC */
  START_PARAM_LOOP()
  /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
  if (element_infos->param_rank == 2U)
  {
    /* due to parser implementation (spaces are not considered as a split character) and the format of +CGEV,
    *  we can receive an additional parameter with the PDN event name in the 1st CGEV parameter.
    *  For example:
    *    +CGEV: NW DETACH                                 => no additional parameter
    *    +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]       => <cid> will be present here
    *    +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]  => <PDP_type>  will be present here
    *
    * In consequence, a specific treatment is done here.

    * Here is a list of different events we can receive:
    *   +CGEV: NW DETACH
    *   +CGEV: ME DETACH
    *   +CGEV: NW CLASS <class>
    *   +CGEV: ME CLASS <class>
    *   +CGEV: NW PDN ACT <cid>[,<WLAN_Offload>]
    *   +CGEV: ME PDN ACT <cid>[,<reason>[,<cid_other>]][,<WLAN_Offload>]
    *   +CGEV: NW ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: ME ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
    *   +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>]
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
    *   +CGEV: ME PDN DEACT <cid>
    *   +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>].
    *   +CGEV: ME DEACT <p_cid>, <cid>, <event_type>
    *   +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>].
    *   +CGEV: NW MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: ME MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
    *   +CGEV: REJECT <PDP_type>, <PDP_addr>
    *   +CGEV: NW REACT <PDP_type>, <PDP_addr>, [<cid>]
    *
    *  We are only interested by following events:
    *   +CGEV: NW DETACH : the network has forced a Packet domain detach (all contexts)
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>] : the nw has forced a context deactivation
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>] : context deactivated
    */

    /* check that previous PDN URC has been reported
    *  if this is not the case, we can not report this one
    */
    if (p_modem_ctxt->persist.urc_avail_pdn_event != AT_TRUE)
    {
      /* reset event params */
      reset_pdn_event(&p_modem_ctxt->persist);

      /* create a copy of params */
      uint8_t copy_params[MAX_CGEV_PARAM_SIZE] = {0};
      AT_CHAR_t *found;
      size_t size_mini = ATC_GET_MINIMUM_SIZE(element_infos->str_size, MAX_CGEV_PARAM_SIZE);
      (void) memcpy((void *)copy_params,
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    size_mini);
      found = (AT_CHAR_t *)strtok((CRC_CHAR_t *)copy_params, " ");
      while (found  != NULL)
      {
        /* analyze of +CGEV event received */
        if (0 == strcmp((CRC_CHAR_t *)found, "NW"))
        {
          /* NW */
          p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_NW;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "ME"))
        {
          /* ME */
          p_modem_ctxt->persist.pdn_event.event_origine = CGEV_EVENT_ORIGINE_ME;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "PDN"))
        {
          /* PDN */
          p_modem_ctxt->persist.pdn_event.event_scope = CGEV_EVENT_SCOPE_PDN;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "ACT"))
        {
          /* ACT */
          p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_ACTIVATION;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "DEACT"))
        {
          /* DEACT */
          p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DEACTIVATION;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "REJECT"))
        {
          /* REJECT */
          p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_REJECT;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "DETACH"))
        {
          /* DETACH */
          p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_DETACH;
          /* all PDN are concerned */
          p_modem_ctxt->persist.pdn_event.conf_id = CS_PDN_ALL;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "CLASS"))
        {
          /* CLASS */
          p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_CLASS;
        }
        else if (0 == strcmp((CRC_CHAR_t *)found, "MODIFY"))
        {
          /* MODIFY */
          p_modem_ctxt->persist.pdn_event.event_type = CGEV_EVENT_TYPE_MODIFY;
        }
        else
        {
          /* if falling here, this is certainly an additional parameter (cf above explanation) */
          if (p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW)
          {
            if (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DETACH)
            {
              /*  we are in the case:
              *  +CGEV: NW DETACH
              *  => no parameter to analyze
              */
              PRINT_ERR("No parameter expected for  NW DETACH")
            }
            else if (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION)
            {
              if (p_modem_ctxt->persist.pdn_event.event_scope == CGEV_EVENT_SCOPE_PDN)
              {
                /* we are in the case:
                *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
                *   => parameter to analyze = <cid>
                */
                uint32_t cgev_cid = ATutil_convertStringToInt((uint8_t *)found,
                                                              (uint16_t)strlen((CRC_CHAR_t *)found));
                p_modem_ctxt->persist.pdn_event.conf_id = atcm_get_configID_for_modem_cid(&p_modem_ctxt->persist,
                                                                                          (uint8_t)cgev_cid);
                PRINT_DBG("+CGEV modem cid=%ld (user conf Id =%d)", cgev_cid, p_modem_ctxt->persist.pdn_event.conf_id)
              }
              else
              {
                /* we are in the case:
                *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
                *   => parameter to analyze = <PDP_type>
                *
                *   skip, parameter not needed in this case
                */
              }
            }
            else
            {
              PRINT_DBG("event type (= %d) ignored", p_modem_ctxt->persist.pdn_event.event_type)
            }
          }
          else
          {
            PRINT_DBG("ME events ignored")
          }
        }

        PRINT_DBG("(%d) ---> %s", strlen((CRC_CHAR_t *)found), (uint8_t *) found)
        found = (AT_CHAR_t *)strtok(NULL, " ");
      }

      /* Indicate that a +CGEV URC has been received */
      p_modem_ctxt->persist.urc_avail_pdn_event = AT_TRUE;
    }
    else
    {
      PRINT_ERR("an +CGEV URC still not reported, ignore this one")
      retval = ATACTION_RSP_ERROR;
    }

  }
  else if (element_infos->param_rank == 3U)
  {
    if ((p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW) &&
        (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
    {
      /* receive <PDP_addr> for:
      * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
      */

      /* analyze <IP_address> and try to find a matching user cid */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* recopy IP address value, ignore type */
      ip_addr_info.ip_addr_type = CS_IPAT_INVALID;
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PRINT_DBG("<PDP_addr>=%s", (AT_CHAR_t *)&ip_addr_info.ip_addr_value)

      /* find user cid matching this IP addr (if any) */
      p_modem_ctxt->persist.pdn_event.conf_id = find_user_cid_with_matching_ip_addr(&p_modem_ctxt->persist,
                                                                                    &ip_addr_info);

    }
    else
    {
      PRINT_DBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    if ((p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW) &&
        (p_modem_ctxt->persist.pdn_event.event_type == CGEV_EVENT_TYPE_DEACTIVATION))
    {
      /* receive <cid> for:
      * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
      */
      /* CID not used: we could use it if problem with <PDP_addr> occurred */
    }
    else
    {
      PRINT_DBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
    }
  }
  else
  {
    PRINT_DBG("+CGEV parameter rank %d ignored", element_infos->param_rank)
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CSQ.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CSQ(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CSQ()")

  /* analyze parameters for CSQ */
  /* for EXECUTION COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* 3GP TS27.007
    *  format: +CSQ: <rssi>,<ber>
    *
    *  <rssi>: integer type
    *          0  -113dBm or less
    *          1  -111dBm
    *          2...30  -109dBm to -53dBm
    *          31  -51dBm or greater
    *          99  unknown or not detectable
    *  <ber>: integer type (channel bit error rate in percent)
    *          0...7  as RXQUAL values in the table 3GPP TS 45.008
    *          99     not known ot not detectable
    */

    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      /* RSSI parameter */
      uint32_t rssi = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
      PRINT_DBG("+CSQ rssi=%ld", rssi)

      if (p_modem_ctxt->SID_ctxt.p_signal_quality != NULL)
      {
        p_modem_ctxt->SID_ctxt.p_signal_quality->rssi = (uint8_t)rssi;
      }
    }
    if (element_infos->param_rank == 3U)
    {
      /* BER parameter */
      uint32_t ber = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size);
      PRINT_DBG("+CSQ ber=%ld", ber)
      if (p_modem_ctxt->SID_ctxt.p_signal_quality != NULL)
      {
        p_modem_ctxt->SID_ctxt.p_signal_quality->ber = (uint8_t)ber;
      }
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CGPADDR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CGPADDR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                    const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CGPADDR()")

  /* analyze parameters for CGPADDR */
  /* for WRITE COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* 3GP TS27.007
    *  format: +CGPADDR: <cid>[,<PDP_addr_1>[,>PDP_addr_2>]]]
    *
    *  <cid>: integer type
    *         specifies a particular PDP context definition
    *  <PDP_addr_1> and <PDP_addr_2>: string type
    *         format = a1.a2.a3.a4 for IPv4
    *         format = a1.a2.a3.a4.a5a.a6.a7.a8.a9.a10.a11.a12a.a13.a14.a15.a16 for IPv6
    */

    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    PRINT_DBG("+CGPADDR param_rank = %d", element_infos->param_rank)
    if (element_infos->param_rank == 2U)
    {
      /* CID parameter */
      uint32_t modem_cid = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                     element_infos->str_size);
      PRINT_DBG("+CGPADDR cid=%ld", modem_cid)
      p_modem_ctxt->CMD_ctxt.modem_cid = modem_cid;
    }
    else if ((element_infos->param_rank == 3U) || (element_infos->param_rank == 4U))
    {
      /* analyze <PDP_addr_1> and <PDP_addr_2> */
      csint_ip_addr_info_t  ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));

      /* retrieve IP address value */
      (void) memcpy((void *) & (ip_addr_info.ip_addr_value),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
      PRINT_DBG("+CGPADDR addr=%s", (AT_CHAR_t *)&ip_addr_info.ip_addr_value)

      /* determine IP address type */
      ip_addr_info.ip_addr_type = atcm_get_ip_address_type((AT_CHAR_t *)&ip_addr_info.ip_addr_value);

      /* save IP address infos in modem_cid_table */
      if (element_infos->param_rank == 3U)
      {
        atcm_put_IP_address_infos(&p_modem_ctxt->persist, (uint8_t)p_modem_ctxt->CMD_ctxt.modem_cid, &ip_addr_info);
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

/**
  * @brief  Analyze specific modem response : CPSMS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CPSMS(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                  const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CPSMS()")

  /* analyze parameters for CPSMS */
  /* for READ COMMAND only  */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    /* 3GP TS27.007
    *  format: +CPSMS: <mode>,[<Requested_Periodic-RAU>],[<Requested_GPRS-READYtimer>],
                       [<Requested_Periodic-TAU>],[<Requested_Active-Time>]
    */

    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    PRINT_DBG("+CPSMS param_rank = %d", element_infos->param_rank)
    if (element_infos->param_rank == 2U)
    {
      /* mode */
      PRINT_INFO("+CPSMS: mode= %ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                           element_infos->str_size))
    }
    else if (element_infos->param_rank == 3U)
    {
      /* req_periodic_rau */
      PRINT_INFO("+CPSMS: req_periodic_rau= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    else if (element_infos->param_rank == 4U)
    {
      /* req_gprs_ready_timer */
      PRINT_INFO("+CPSMS: req_gprs_ready_timer= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    else if (element_infos->param_rank == 5U)
    {
      /* req_periodic_tau */
      PRINT_INFO("+CPSMS: req_periodic_tau= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    else if (element_infos->param_rank == 6U)
    {
      /* req_active_time */
      PRINT_INFO("+CPSMS: req_active_time= 0x%lx",
                 ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size, 8))
    }
    else
    {
      /* parameters ignored */
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CEDRXS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CEDRXS(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CEDRXS()")

  /* analyze parameters for CEDRXS */
  if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CEDRXS)
  {
    /* for READ COMMAND only  */
    if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
    {
      /* 3GP TS27.007
      *  eDRX URC
      *
      *  format: +CEDRXS: <AcT-type>,<Requested_eDRX_value>
      *                   [<CR><LF>+CEDRXS:<AcT-type>,<Requested_eDRX_value>[...]]]
      */

      START_PARAM_LOOP()
      /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
      PRINT_DBG("+CEDRXS param_rank = %d", element_infos->param_rank)
      if (element_infos->param_rank == 2U)
      {
        /* act_type */
        PRINT_DBG("+CEDRXS: act_type= %ld",
                  ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                            element_infos->str_size))
      }
      else if (element_infos->param_rank == 3U)
      {
        /* req_edrx_value */
        PRINT_INFO("+CEDRXS: req_edrx_value= 0x%lx",
                   ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                        element_infos->str_size, 4))
      }
      else
      {
        /* parameters ignored */
      }
      END_PARAM_LOOP()
    }
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CEDRXP.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CEDRXP(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                   const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CEDRXP()")

  /* 3GP TS27.007
  *  format: +CEDRXP: <AcT-type>[,<Requested_eDRX_value>[,<NW-provided_eDRX_value>[,<Paging_time_window>]]]
  *
  */

  START_PARAM_LOOP()
  /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
  PRINT_DBG("+CEDRXS param_rank = %d", element_infos->param_rank)
  if (element_infos->param_rank == 2U)
  {
    /* act_type */
    PRINT_DBG("+CEDRXP URC: act_type= %ld",
              ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                        element_infos->str_size))
  }
  else if (element_infos->param_rank == 3U)
  {
    /* req_edrx_value */
    PRINT_INFO("+CEDRXP URC: req_edrx_value= 0x%lx",
               ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size, 4))
  }
  else if (element_infos->param_rank == 4U)
  {
    /* nw_provided_edrx_value */
    PRINT_INFO("+CEDRXP URC: nw_provided_edrx_value= 0x%lx",
               ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size, 4))
  }
  else if (element_infos->param_rank == 5U)
  {
    /* paging_time_window */
    PRINT_INFO("+CEDRXP URC: paging_time_window= 0x%lx",
               ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size, 4))
  }
  else
  {
    /* parameters ignored */
  }
  END_PARAM_LOOP()


  return (retval);
}

/**
  * @brief  Analyze specific modem response : CEDRXRDP.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CEDRXRDP(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                     const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CEDRXRDP()")

  /* 3GP TS27.007
  *  eDRX Read Dynamic Parameters
  *
  *  format: +CEDRXRDP: <AcT-type>[,<Requested_eDRX_value>[,<NW-provided_eDRX_value>[,<Paging_time_window>]]]
  *
  */

  START_PARAM_LOOP()
  /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
  PRINT_DBG("+CEDRXDP param_rank = %d", element_infos->param_rank)
  if (element_infos->param_rank == 2U)
  {
    /* act_type */
    PRINT_DBG("+CEDRXRDP: act_type= %ld",
              ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                        element_infos->str_size))
  }
  else if (element_infos->param_rank == 3U)
  {
    /* req_edrx_value */
    PRINT_INFO("+CEDRXRDP: req_edrx_value= 0x%lx",
               ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size, 4))
  }
  else if (element_infos->param_rank == 4U)
  {
    /* nw_provided_edrx_value */
    PRINT_INFO("+CEDRXRDP: nw_provided_edrx_value= 0x%lx",
               ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size, 4))
  }
  else if (element_infos->param_rank == 5U)
  {
    /* paging_time_window */
    PRINT_INFO("+CEDRXRDP: paging_time_window= 0x%lx",
               ATutil_extract_bin_value_from_quotes(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size, 4))
  }
  else
  {
    /* parameters ignored */
  }
  END_PARAM_LOOP()


  return (retval);
}

/**
  * @brief  Analyze specific modem response : CSIM.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CSIM(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                 const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CSIM()")

  /* analyze parameters for +CSIM
   *  answer to CSIM write command
   *     +CSIM: <length>,<response>
   */

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      uint32_t rsp_length = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                      element_infos->str_size);

      p_modem_ctxt->SID_ctxt.sim_generic_access.bytes_received = rsp_length;

    }
    else if (element_infos->param_rank == 3U)
    {
      uint32_t length_to_copy;
      uint8_t *p_end;

      /* Copy the maximum of possible bytes in response string */
      /* Reserve one byte at the end the string with a '\0' */
      length_to_copy = ATC_GET_MINIMUM_SIZE(p_modem_ctxt->SID_ctxt.sim_generic_access.bytes_received,
                                            (p_modem_ctxt->SID_ctxt.sim_generic_access.data->rsp_str_size - 1U));
      (void)memcpy((CRC_CHAR_t *)p_modem_ctxt->SID_ctxt.sim_generic_access.data->p_rsp_str,
                   (const CS_CHAR_t *)&p_msg_in->buffer[element_infos->str_start_idx + 1U],  /* skip '"' */
                   (size_t)length_to_copy);

      /* Last byte is always set to '\0' */
      p_end = (uint8_t *)&p_modem_ctxt->SID_ctxt.sim_generic_access.data->p_rsp_str[length_to_copy];
      *p_end = (uint8_t)'\0';

    }
    else
    {
      /* parameters ignored */
    }

    END_PARAM_LOOP()
  }

  if (p_atp_ctxt->current_atcmd.type == ATTYPE_TEST_CMD)
  {
    PRINT_DBG("+CSIM for test cmd NOT IMPLEMENTED")
  }

  return (retval);
}

/* ==========================  Analyze V.25ter commands ========================== */
/**
  * @brief  Analyze specific modem response : GSN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_GSN(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_GSN()")

  /* analyze parameters for +GSN */
  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    PRINT_DBG("IMEI:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.imei),
                    (const void *)&p_msg_in->buffer[element_infos->str_start_idx],
                    (size_t) element_infos->str_size);
    }
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : IPR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_IPR(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
#if (USE_TRACE_ATCUSTOM_MODEM == 0U)
  UNUSED(p_msg_in); /* for MISRA-2012 */
#endif /* USE_TRACE_ATCUSTOM_MODEM */

  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_IPR()")

  /* analyze parameters for +IPR */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    PRINT_DBG("BAUD RATE:")
    if (element_infos->param_rank == 2U)
    {
      /* param trace only */
      PRINT_INFO("+IPR baud rate=%ld",
                 ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size))
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : IFC.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_IFC(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
#if (USE_TRACE_ATCUSTOM_MODEM == 0U)
  UNUSED(p_msg_in); /* for MISRA-2012 */
#endif /* USE_TRACE_ATCUSTOM_MODEM */

  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_IFC()")

  /* initialize RTS and CTS with invalid values */
  p_modem_ctxt->persist.flowCtrl_RTS = 0xFF;
  p_modem_ctxt->persist.flowCtrl_CTS = 0xFF;

  /* analyze parameters for +IFC */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_READ_CMD)
  {
    START_PARAM_LOOP()
    /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
    if (element_infos->param_rank == 2U)
    {
      /* dce_by_dte flow control:
       * 0 = None
       * 2= RTS flow control */
      uint32_t rts_fc = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
      PRINT_DBG("+IFC: RTS flow control=%ld", rts_fc)
      if (rts_fc == 2U)
      {
        /* RTS flow control */
        p_modem_ctxt->persist.flowCtrl_RTS = 2U;
      }
      else if (rts_fc == 0U)
      {
        /* No flow control */
        p_modem_ctxt->persist.flowCtrl_RTS = 0U;
      }
      else { /* keep 0xFF */ }
    }
    if (element_infos->param_rank == 3U)
    {
      /* dte_by_dce flow control: 0:None 2= CTS flow control */
      uint32_t cts_fc = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                  element_infos->str_size);
      PRINT_DBG("+IFC: CTS flow control=%ld", cts_fc)
      if (cts_fc == 2U)
      {
        p_modem_ctxt->persist.flowCtrl_CTS = 2U;
      }
      else if (cts_fc == 0U)
      {
        p_modem_ctxt->persist.flowCtrl_CTS = 0U;
      }
      else { /* keep 0xFF */ }
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : Direct Command.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_DIRECT_CMD(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_DIRECT_CMD()")

  /* NOT IMPLEMENTED YET */

  return (retval);
}
/**
  * @}
  */

/** @defgroup AT_CORE_SIGNALLING_ANALYZE_Private_Functions AT_CORE SIGNALLING ANALYZE Private Functions
  * @{
  */
/**
  * @brief  Decode and display network state.
  * @param  state
  * @param  network_type
  * @retval none
  */
static void display_clear_network_state(CS_NetworkRegState_t state, uint8_t network_type)
{
#if (USE_TRACE_ATCUSTOM_MODEM == 1U) /* to avoid warning when no traces */
  /* Commands Look-up table for AT+QCFG */
  static const AT_CHAR_t NETWORK_TYPE_LUT[4][16] =
  {
    {"(unknown)"},
    {"(CS)"},
    {"(GPRS)"},
    {"(EPS)"},
  };

  switch (state)
  {
    case CS_NRS_NOT_REGISTERED_NOT_SEARCHING:
      /* Trace only */
      PRINT_INFO("NetworkState %s = NOT_REGISTERED_NOT_SEARCHING", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTERED_HOME_NETWORK:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTERED_HOME_NETWORK", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_NOT_REGISTERED_SEARCHING:
      /* Trace only */
      PRINT_INFO("NetworkState %s = NOT_REGISTERED_SEARCHING", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTRATION_DENIED:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTRATION_DENIED", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_UNKNOWN:
      /* Trace only */
      PRINT_INFO("NetworkState %s = UNKNOWN", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTERED_ROAMING:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTERED_ROAMING", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTERED_SMS_ONLY_HOME_NETWORK", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTERED_SMS_ONLY_ROAMING:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTERED_SMS_ONLY_ROAMING", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_EMERGENCY_ONLY:
      /* Trace only */
      PRINT_INFO("NetworkState %s = EMERGENCY_ONLY", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTERED_CFSB_NP_HOME_NETWORK", NETWORK_TYPE_LUT[network_type])
      break;
    case CS_NRS_REGISTERED_CFSB_NP_ROAMING:
      /* Trace only */
      PRINT_INFO("NetworkState %s = REGISTERED_CFSB_NP_ROAMING", NETWORK_TYPE_LUT[network_type])
      break;
    default:
      /* Trace only */
      PRINT_INFO("unknown state value")
      break;
  }
#else /* USE_TRACE_ATCUSTOM_MODEM == 0U */
  UNUSED(state);
  UNUSED(network_type);
#endif /* USE_TRACE_ATCUSTOM_MODEM == 1U */
}

/**
  * @brief  Convert network state to readable value.
  * @param  state
  * @param  network_type
  * @retval CS_NetworkRegState_t
  */
static CS_NetworkRegState_t convert_NetworkState(uint32_t state, uint8_t network_type)
{
  CS_NetworkRegState_t retval;

  switch (state)
  {
    case 0:
      retval = CS_NRS_NOT_REGISTERED_NOT_SEARCHING;
      break;
    case 1:
      retval = CS_NRS_REGISTERED_HOME_NETWORK;
      break;
    case 2:
      retval = CS_NRS_NOT_REGISTERED_SEARCHING;
      break;
    case 3:
      retval = CS_NRS_REGISTRATION_DENIED;
      break;
    case 4:
      retval = CS_NRS_UNKNOWN;
      break;
    case 5:
      retval = CS_NRS_REGISTERED_ROAMING;
      break;
    case 6:
      retval = CS_NRS_REGISTERED_SMS_ONLY_HOME_NETWORK;
      break;
    case 7:
      retval = CS_NRS_REGISTERED_SMS_ONLY_ROAMING;
      break;
    case 8:
      retval = CS_NRS_EMERGENCY_ONLY;
      break;
    case 9:
      retval = CS_NRS_REGISTERED_CFSB_NP_HOME_NETWORK;
      break;
    case 10:
      retval = CS_NRS_REGISTERED_CFSB_NP_ROAMING;
      break;
    default:
      retval = CS_NRS_UNKNOWN;
      break;
  }

  display_clear_network_state(retval, network_type);

  return (retval);
}

/**
  * @brief  Try to find user cid with matching IP address
  * @param  p_persistent_ctxt
  * @param  ip_addr_struct
  * @retval CS_PDN_conf_id_t
  */
static CS_PDN_conf_id_t find_user_cid_with_matching_ip_addr(atcustom_persistent_context_t *p_persistent_ctxt,
                                                            csint_ip_addr_info_t *ip_addr_struct)
{
  CS_PDN_conf_id_t user_cid = CS_PDN_NOT_DEFINED;

  /* search user config ID corresponding to this IP address */
  for (uint8_t loop = 0U; loop < MODEM_MAX_NB_PDP_CTXT; loop++)
  {
    atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[loop];
    PRINT_DBG("[Compare ip addr with user cid=%d]: <%s> vs <%s>",
              loop,
              (CRC_CHAR_t *)&ip_addr_struct->ip_addr_value,
              (CRC_CHAR_t *)&p_tmp->ip_addr_infos.ip_addr_value)

    /* quick and dirty solution
     * should implement a better solution */
    uint8_t size1;
    uint8_t size2;
    uint8_t minsize;
    size1 = (uint8_t) strlen((CRC_CHAR_t *)&ip_addr_struct->ip_addr_value);
    size2 = (uint8_t) strlen((CRC_CHAR_t *)&p_tmp->ip_addr_infos.ip_addr_value);
    minsize = (size1 < size2) ? size1 : size2;
    if ((0 == memcmp((AT_CHAR_t *)&ip_addr_struct->ip_addr_value[0],
                     (AT_CHAR_t *)&p_tmp->ip_addr_infos.ip_addr_value[0],
                     (size_t) minsize)) &&
        (minsize != 0U)
       )
    {
      user_cid = atcm_convert_index_to_PDN_conf(loop);
      PRINT_DBG("Found matching user cid=%d", user_cid)
    }
  }

  return (user_cid);
}

/**
  * @brief  Analyze +CME ERROR.
  * @param  p_at_ctxt
  * @param  p_modem_ctxt
  * @param  p_msg_in
  * @param  element_infos
  * @retval at_action_rsp_t
  */
static at_action_rsp_t analyze_CmeError(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                        const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter analyze_CmeError()")

  START_PARAM_LOOP()
  /* check parameter rank : rank 1 is command name, rank 2 is first parameter after =, etc... */
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0U};
    PRINT_DBG("CME ERROR parameter received:")
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
    if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM NOT INSERTED") != NULL)
    {
      /* SIM NOT INSERTED */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_NOT_INSERTED;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PIN NECESSARY") != NULL)
    {
      /* SIM PIN NECESSARY */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PIN REQUIRED") != NULL)
    {
      /* SIM PIN REQUIRED */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN_REQUIRED;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PUK REQUIRED") != NULL)
    {
      /* SIM PUK REQUIRED */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK_REQUIRED;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM FAILURE") != NULL)
    {
      /* SIM FAILURE */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_FAILURE;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM BUSY") != NULL)
    {
      /* SIM BUSY */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_BUSY;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM WRONG") != NULL)
    {
      /* SIM WRONG */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_WRONG;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "INCORRECT PASSWORD") != NULL)
    {
      /* INCORRECT PASSWORD */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_INCORRECT_PASSWORD;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PIN2 REQUIRED") != NULL)
    {
      /* SIM PIN2 REQUIRED */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PIN2_REQUIRED;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "SIM PUK2 REQUIRED") != NULL)
    {
      /* SIM PUK2 REQUIRED */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_SIM_PUK2_REQUIRED;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
    else
    {
      /* other error code */
      p_modem_ctxt->persist.sim_pin_code_ready = AT_FALSE;
      p_modem_ctxt->persist.sim_state = CS_SIMSTATE_UNKNOWN;
      atcm_set_error_report(CSERR_SIM, p_modem_ctxt);
    }
  }
  END_PARAM_LOOP()

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
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
