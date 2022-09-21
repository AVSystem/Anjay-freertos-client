/**
  ******************************************************************************
  * @file    at_signalling_build.c
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
#include "at_signalling_build.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_sysctrl.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SIGNALLING_BUILD AT_CORE SIGNALLING BUILD
  * @{
  */

/** @defgroup AT_CORE_SIGNALLING_BUILD_Private_Macros AT_CORE SIGNALLING BUILD Private Macros
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

/** @defgroup AT_CORE_SIGNALLING_BUILD_Private_Defines AT_CORE SIGNALLING BUILD Private Defines
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

/** @defgroup AT_CORE_SIGNALLING_BUILD_Exported_Functions AT_CORE SIGNALLING BUILD Exported Functions
  * @{
  */
/* ==========================  Build 3GPP TS 27.007 commands ========================== */
/**
  * @brief  Build specific modem command : without parameters.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_NoParams(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_atp_ctxt);
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  /* Command as no parameters - STUB function */
  PRINT_API("enter fCmdBuild_NoParams()")

  return (retval);
}

/**
  * @brief  Build specific modem command : CGSN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGSN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGSN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+CGSN=<n> where <n>:
      * 0: serial number
      * 1: IMEI
      * 2: IMEISV (IMEI and Software version number)
      * 3: SVN (Software version number)
      *
      * <n> parameter is set previously
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                   p_modem_ctxt->CMD_ctxt.cgsn_write_cmd_param);
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : CMEE.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CMEE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CMEE()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* AT+CMEE=<n> where <n>:
      * 0: <err> result code disabled and ERROR used
      * 1: <err> result code enabled and numeric <ERR> values used
      * 2: <err> result code enabled and verbose <ERR> values used
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", p_modem_ctxt->persist.cmee_level);
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : CPIN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CPIN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CPIN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    PRINT_DBG("pin code= %s", p_modem_ctxt->SID_ctxt.modem_init.pincode.pincode)

    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\"",
                   p_modem_ctxt->SID_ctxt.modem_init.pincode.pincode);
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : CFUN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CFUN(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CFUN()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_INIT_MODEM)
    {
      uint8_t fun;
      uint8_t rst;
      /* at modem init, get CFUN mode from parameters */
      const csint_modemInit_t *modemInit_struct = &(p_modem_ctxt->SID_ctxt.modem_init);

      /* AT+CFUN=<fun>,<rst>
        * where <fun>:
        *  0: minimum functionality
        *  1: full functionality
        *  4: disable phone TX & RX
        * where <rst>:
        *  0: do not reset modem before setting <fun> parameter
        *  1: reset modem before setting <fun> parameter
        */

      /* convert cellular service parameters to Modem format */
      if (modemInit_struct->init == CS_CMI_FULL)
      {
        fun = 1U;
      }
      else if (modemInit_struct->init == CS_CMI_SIM_ONLY)
      {
        fun = 4U;
      }
      else
      {
        fun = 0U; /* default value, if CS_CMI_MINI */
      }

      (modemInit_struct->reset == CELLULAR_TRUE) ? (rst = 1U) : (rst = 0U);
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d", fun, rst);
    }
    else /* user settings */
    {
      /* set parameter defined by user */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,0", p_modem_ctxt->CMD_ctxt.cfun_value);
    }
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : COPS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_COPS(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_COPS()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    CS_OperatorSelector_t *operatorSelect = &(p_modem_ctxt->SID_ctxt.write_operator_infos);

    if (operatorSelect->mode == CS_NRM_AUTO)
    {
      if (operatorSelect->AcT_present == CELLULAR_FALSE)
      {
        /* no specific Access Technology is requested */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
      }
      else
      {
        /* a specific Access Technology is requested
         * it will be used as a priority if found
         */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0,,,%d",
                       operatorSelect->AcT);
      }
    }
    else if ((operatorSelect->mode == CS_NRM_MANUAL) ||
             (operatorSelect->mode == CS_NRM_MANUAL_THEN_AUTO))
    {
      /* same behaviour for "manual" mode and "manuel then auto" mode */
      uint8_t selected_mode;
      if (operatorSelect->mode == CS_NRM_MANUAL)
      {
        selected_mode = 1U;
      }
      else
      {
        selected_mode = 4U;
      }

      /* according to 3GPP TS 27.007, <oper> field shall be present
      * in manual modes. */
      if (operatorSelect->format != CS_ONF_NOT_PRESENT)
      {
        if (operatorSelect->AcT_present == CELLULAR_FALSE)
        {
          /* no specific Access Technology is requested */
          (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d,\"%s\"",
                         selected_mode,
                         operatorSelect->format,
                         operatorSelect->operator_name);
        }
        else
        {
          /* a specific Access Technology is requested
          * it will be used as a priority if found
          */
          (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d,\"%s\",%d",
                         selected_mode,
                         operatorSelect->format,
                         operatorSelect->operator_name,
                         operatorSelect->AcT);
        }
      }
      else
      {
        /* <oper> is not present */
        retval = ATSTATUS_ERROR;
      }
    }
    else if (operatorSelect->mode == CS_NRM_DEREGISTER)
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2");
    }
    else
    {
      PRINT_ERR("invalid mode value for +COPS")
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGATT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGATT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGATT()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* if cgatt set by user or if in ATTACH sequence */
    if ((p_modem_ctxt->CMD_ctxt.cgatt_write_cmd_param == CGATT_ATTACHED) ||
        (p_atp_ctxt->current_SID == (at_msg_t) SID_ATTACH_PS_DOMAIN))
    {
      /* request attach */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
    }
    /* if cgatt set by user or if in DETACH sequence */
    else if ((p_modem_ctxt->CMD_ctxt.cgatt_write_cmd_param == CGATT_DETACHED) ||
             (p_atp_ctxt->current_SID == (at_msg_t) SID_DETACH_PS_DOMAIN))
    {
      /* request detach */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    /* not in ATTACH or DETACH sequence and cgatt_write_cmd_param not set by user: error ! */
    else
    {
      PRINT_ERR("CGATT state parameter not set")
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CREG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
    {
      /* always request all notif with +CREG:2, will be sorted at cellular service level */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", (uint8_t)CXREG_ENABLE_NETWK_REG_LOC_URC);
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                     (uint8_t) p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGREG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
    {
      atcustom_CxREG_n_t param_value;
      if (p_modem_ctxt->persist.psm_urc_requested == AT_TRUE)
      {
        param_value = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
      }
      else
      {
        param_value = CXREG_ENABLE_NETWK_REG_LOC_URC;
      }
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", (uint8_t) param_value);
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                     (uint8_t) p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CEREG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CEREG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CEREG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
    {
      atcustom_CxREG_n_t param_value;
      if (p_modem_ctxt->persist.psm_urc_requested == AT_TRUE)
      {
        param_value = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
      }
      else
      {
        param_value = CXREG_ENABLE_NETWK_REG_LOC_URC;
      }
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", (uint8_t)param_value);
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
    {
      /* disable notifications */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* for other SID, use param value set by user */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                     (uint8_t) p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param);
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGEREP.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGEREP(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGEREP()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* Packet Domain event reporting +CGEREP
      * 3GPP TS 27.007
      * Write command: +CGEREP=[<mode>[,<bfr>]]
      * where:
      *  <mode> = 0: no URC (+CGEV) are reported
      *         = 1: URC are discsarded when link is reserved (data on) and forwarded otherwise
      *         = 2: URC are buffered when link is reserved and send when link freed, and forwarded otherwise
      *  <bfr>  = 0: MT buffer of URC is cleared when <mode> 1 or 2 is entered
      *         = 1: MT buffer of URC is flushed to TE when <mode> 1 or 2 is entered
      */
    if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_REGISTER_PDN_EVENT)
    {
      /* enable notification (hard-coded value 1,0) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1,0");
    }
    else if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DEREGISTER_PDN_EVENT)
    {
      /* disable notifications */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
    else
    {
      /* nothing to do */
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGDCONT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGDCONT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGDCONT()")

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
      /* no APN provided by user: use empty APN string to force the network to
      *  select the appropriate APN.
      *  NOTE: assumption is that a modem using CGDCONT to specify the APN is able to interpret "" APN and do not
      *        send an APN to the network in ATTACH REQUEST (to let the network select the appropriate APN).
      *        If the modem behavior is not aligned, you have to create a specific fCmdBuild_CGDCONT_XXX function.
      */
      p_apn = (CS_CHAR_t *) &APN_EMPTY_STRING;
    }

    /* check if this PDP context has been defined */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].conf_id != CS_PDN_NOT_DEFINED)
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\"",
                     modem_cid,
                     atcm_get_PDPtypeStr(p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.pdp_type),
                     p_apn);
    }
    else
    {
      PRINT_ERR("PDP context not defined for conf_id %d", current_conf_id)
      retval = ATSTATUS_ERROR;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGACT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGACT(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGACT()")

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

    /* PDP context activate or deactivate
      *  3GPP TS 27.007
      *  AT+CGACT=[<state>[,<cid>[,<cid>[,...]]]]
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d",
                   ((p_modem_ctxt->CMD_ctxt.pdn_state == PDN_STATE_ACTIVATE) ? 1 : 0),
                   modem_cid);
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGAUTH.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGAUTH(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGAUTH()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf 3GPP TS 27.007
    *  AT+CGAUTH: Define PDP context Authentication Parameters
    *  AT+QICSGP=<cid>[,<auth_pro>[,<userid>[,<password>]]]
    *  - cid: context id (specifies a paritcular PDP context definition)
    *  - auth_pro: 0 for none, 1 for PAP, 2 for CHAP
    *  - username: string
    *  - password: string
    */

    CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
    uint8_t modem_cid = atcm_get_affected_modem_cid(&p_modem_ctxt->persist, current_conf_id);
    PRINT_INFO("user cid = %d, modem cid = %d", (uint8_t)current_conf_id, modem_cid)
    uint8_t auth_protocol;

    /*  authentication protocol: 0,1 or 2. 0 for none, 1 for PAP, 2 for CHAP */
    if (p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username[0] == 0U)
    {
      /* no username => no authentication */
      auth_protocol = 0U;

      /* build command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d",
                     modem_cid,
                     auth_protocol);
    }
    else
    {
      /* username => PAP authentication protocol */
      auth_protocol = 1U;

      /* build command */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d,\"%s\",\"%s\"",
                     modem_cid,
                     auth_protocol,
                     p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.username,
                     p_modem_ctxt->persist.pdp_ctxt_infos[current_conf_id].pdn_conf.password);
    }

  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGDATA.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGDATA(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGDATA()")

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

    /* Enter data state
      *  3GPP TS 27.007
      *  AT+CGDATA[=<L2P>[,<cid>[,<cid>[,...]]]]
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"PPP\",%d",
                   modem_cid);
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CGPADDR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGPADDR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGPADDR()")

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

    /* Show PDP address(es)
      *  3GPP TS 27.007
      *  AT+CGPADDR[=<cid>[,<cid>[,...]]]
      *
      *  implementation: we only request address for 1 cid (if more cid required, call it again)
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", modem_cid);
  }

  return (retval);
}

/* ==========================  Build V.25ter commands ========================== */
/**
  * @brief  Build specific modem command : ATD.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ATD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATD()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* actually implemented specifically for each modem
      *  following example is not guaranteed ! (cid is not specified here)
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "*99#");
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : ATE.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ATE(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATE()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* echo mode ON or OFF */
    if (p_modem_ctxt->CMD_ctxt.command_echo == AT_TRUE)
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
    }
    else
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : ATV.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ATV(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATV()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* echo mode ON or OFF */
    if (p_modem_ctxt->CMD_ctxt.dce_full_resp_format == AT_TRUE)
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
    }
    else
    {
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
    }
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : ATX.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ATX(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATX()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* CONNECT Result code and monitor call progress
      *  for the moment, ATX0 to return result code only, dial tone and busy detection are both disabled
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : ATZ.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ATZ(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATZ()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    uint8_t profile_nbr = 0;
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d", profile_nbr);
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : IPR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_IPR(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_IPR()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set baud rate */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%ld", p_modem_ctxt->CMD_ctxt.baud_rate);
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : IFC.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_IFC(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_IFC()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set flow control */
    if (p_modem_ctxt->CMD_ctxt.flow_control_cts_rts == AT_FALSE)
    {
      /* No flow control */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0,0");
    }
    else
    {
      /* CTS/RTS activated */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "2,2");
    }
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : ESCAPE COMMAND.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ESCAPE_CMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ESCAPE_CMD()")

  /* only for RAW command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    /* set escape sequence (as define in custom modem specific) */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%s", p_atp_ctxt->current_atcmd.name);
    /* set raw command size */
    p_atp_ctxt->current_atcmd.raw_cmd_size = strlen((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params);
  }
  else
  {
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : AT&D.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_AT_AND_D(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_AT_AND_D()")

  /* only for execution command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /* Set DTR function mode  (cf V.25ter)
      * hard-coded to 0
      */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CPSMS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CPSMS(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CPSMS()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* 3GPP TS 27.007
     * Power Saving Mode Setting
     *
     * AT+CPSMS=[<mode>[,<Requested_Periodic-RAU>[,<Requested_GPRS-READY-timer>[,<Requested_Periodic-TAU>[
     * ,<Requested_Active-Time>]]]]]
     *
     * mode: 0 disable PSM, 1 enable PSM
     * > parameters for GERAN/UTRAN:
     * Requested_Periodic-RAU: (stringtype, one byte in an 8 bit format) cf Table 10.5.163a from TS 24.008, T3312
     * Requested_GPRS-READY-timer: (string type, one byte in an 8 bit format) cf Table 10.5.172 from TS 24.008, T3314
     * > parameters for E-UTRAN:
     * Requested_Periodic-TAU: (string type, one byte in an 8 bit format) cf Table 10.5.163a from TS 24.008, T3412
     * Requested_Active-Time: (string type, one byte in an 8 bit format) cf Table 10.5.163 from TS 24.008, T3324
     *
     * Note 1:
     * There is a special form of the command: AT+CPSMS= (with all parameters omitted).
     * In this form, PSM is disabled (mode=0) and all parameters set to the manufacturer specific default values.
     *
     * Note 2:
     * Table 10.5.163a from TS 24.008
     * Bits 5 to 1 represent the binary coded timer value
     * Bits 6 to 8 defines the timer value unit as follow (in order : bits 8 7 6)
     *   000: value is incremented in multiples of 10 minutes
     *   001: value is incremented in multiples of 1 hour
     *   010: value is incremented in multiples of 10 hours
     *   011: value is incremented in multiples of 2 seconds
     *   100: value is incremented in multiples of 30 seconds
     *   101: value is incremented in multiples of 1 minute
     *   110: value is incremented in multiples of 320 hours
     *   111: value indicates that the timer is deactivated
     *
     * Note 3:
     * Table 10.5.163 and Table 10.5.172 from TS 24.008
     * Bits 5 to 1 represent the binary coded timer value
     * Bits 6 to 8 defines the timer value unit as follow (in order : bits 8 7 6)
     *   000: value is incremented in multiples of 2 seconds
     *   001: value is incremented in multiples of 1 minute
     *   010: value is incremented in multiples of 10 hours
     *   111: value indicates that the timer is deactivated
     *   other values shall be interpreted as mutliples of 1 minute
     *
     * exple:
     * AT+CPSMS=1,,,"00000100","00001111"
     * Set the requested T3412 value to 40 minutes, and set the requested T3324 value to 30 seconds
    */

    /* buffers used to convert values to binary string (size = number of bits + 1 for end string character) */
    CS_PSM_params_t *p_psm_params = &(p_modem_ctxt->SID_ctxt.set_power_config.psm);
    uint8_t mode;
    uint8_t req_periodic_rau[9] = {0U};
    uint8_t req_gprs_ready_time[9] = {0U};
    uint8_t req_periodic_tau[9] = {0U};
    uint8_t req_active_time[9] = {0U};

    /* convert Periodic RAU */
    (void)ATutil_convert_uint8_to_binary_string((uint32_t)p_psm_params->req_periodic_RAU,
                                                (uint8_t)8U,
                                                (uint8_t)sizeof(req_periodic_rau),
                                                &req_periodic_rau[0]);
    /* convert GPRS Ready Timer */
    (void)ATutil_convert_uint8_to_binary_string((uint32_t)p_psm_params->req_GPRS_READY_timer,
                                                (uint8_t)8U,
                                                (uint8_t)sizeof(req_gprs_ready_time),
                                                &req_gprs_ready_time[0]);
    /* convert Periodic TAU */
    (void) ATutil_convert_uint8_to_binary_string((uint32_t)p_psm_params->req_periodic_TAU,
                                                 (uint8_t)8U,
                                                 (uint8_t)sizeof(req_periodic_tau),
                                                 &req_periodic_tau[0]);
    /* convert Active Time */
    (void) ATutil_convert_uint8_to_binary_string((uint32_t)p_psm_params->req_active_time,
                                                 (uint8_t)8U,
                                                 (uint8_t)sizeof(req_active_time),
                                                 &req_active_time[0]);

    if (p_modem_ctxt->SID_ctxt.set_power_config.psm_present == CELLULAR_TRUE)
    {
      if (p_modem_ctxt->SID_ctxt.set_power_config.psm_mode == PSM_MODE_DISABLE)
      {
        /* PSM disabled */
        mode = 0U;

        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d",
                       mode);
      }
      else
      {
        /* PSM enabled */
        mode = 1U;

        /* prepare the command
        *  Note: do not send values for 2G/3G networks
        */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,,,\"%s\",\"%s\"",
                       mode,
                       req_periodic_tau,
                       req_active_time);
      }

      /* full command version:
       *
       * (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,\"%s\",\"%s\",\"%s\",\"%s\"",
       *                mode,
       *                req_periodic_rau,
       *                req_gprs_ready_time,
       *                req_periodic_tau,
       *                req_active_time );
       */

    }
    else
    {
      /* no PSM parameters, skip the command */
      PRINT_INFO("No PSM parameters available, command skipped")
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
  }
  else
  {
    PRINT_ERR("invalid pointer to PSM parameters")
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CEDRXS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CEDRXS(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CEDRXS()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* 3GPP TS 27.007
     * eDRX Setting
     *
     * AT+CEDRXS=[<mode>,[,<AcT-type>[,<Requested_eDRX_value>]]]
     *
     * mode: 0 disable eDRX, 1 enable eDRX, 2 enable eDRX and enable URC +CEDRXP, 3 disable eDRX and reset default
     * AcT-type: 0 Act not using eDRX, 1 EC-GSM-IoT , 2 GSM, 3 UTRAN,
     *           4 E-UTRAN WB-S1 mode(LTE and LTE cat.M1), 5 E-UTRAN NB-S1 mode(LTE cat.NB1)
     * Requested_eDRX_value: (string type, half a byte in an 4 bit format)
     *                        Bits 4 to 1 of octet 3 of Extended DRX parameters
     *                        cf Table 10.5.5.32 from TS 24.008
     *
     * exple:
     * AT+CEDRX=1,5,"0000"
     * Set the requested e-I-DRX value to 5.12 second
    */

    if (p_modem_ctxt->SID_ctxt.set_power_config.edrx_present == CELLULAR_TRUE)
    {
      uint8_t edrx_req_value[5] = {0};
      (void) ATutil_convert_uint8_to_binary_string((uint32_t) p_modem_ctxt->SID_ctxt.set_power_config.edrx.req_value,
                                                   (uint8_t) 4U,
                                                   (uint8_t) sizeof(edrx_req_value),
                                                   &edrx_req_value[0]);

      if (p_modem_ctxt->SID_ctxt.set_power_config.edrx_mode == EDRX_MODE_DISABLE)
      {
        /* eDRX disabled */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0");
      }
      else if (p_modem_ctxt->SID_ctxt.set_power_config.edrx_mode == EDRX_MODE_DISABLE_AND_RESET)
      {
        /* eDRX disabled */
        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "3");
      }
      else
      {
        /* eDRX enabled */
        uint8_t edrx_mode;
        edrx_mode = (p_modem_ctxt->SID_ctxt.set_power_config.edrx_mode == EDRX_MODE_ENABLE_WITH_URC) ? 2U : 1U;

        (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "%d,%d,\"%s\"",
                       edrx_mode,
                       p_modem_ctxt->SID_ctxt.set_power_config.edrx.act_type,
                       edrx_req_value);
      }
    }
    else
    {
      /* no eDRX parameters, skip the command */
      PRINT_INFO("No EDRX parameters available, command skipped")
      atcm_program_SKIP_CMD(p_atp_ctxt);
    }
  }
  else
  {
    PRINT_ERR("invalid pointer to EDRX parameters")
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : CSIM.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CSIM(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CSIM()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* 3GPP TS 27.007
     * CSIM command
     *
     * AT+CSIM=<length>,<command>
     *
     * exple:
     * AT+CSIM=AT+CSIM=24,"00A4040C07A0000000871002"
     * Send the command under ""
     */
    uint16_t length;
    length = (uint16_t) p_modem_ctxt->SID_ctxt.sim_generic_access.data->cmd_str_size;
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params,
                   "%d,\"%.*s\"",
                   length, (int16_t)length,
                   (const CS_CHAR_t *)p_modem_ctxt->SID_ctxt.sim_generic_access.data->p_cmd_str);
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : Direct AT Command provided by upper layer.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_DIRECT_CMD(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_DIRECT_CMD()")

  /* only for RAW command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
  {
    if (p_modem_ctxt->SID_ctxt.p_direct_cmd_tx != NULL)
    {
      if (p_modem_ctxt->SID_ctxt.p_direct_cmd_tx->cmd_size != 0U)
      {
        uint32_t str_size = p_modem_ctxt->SID_ctxt.p_direct_cmd_tx->cmd_size;
        (void) memcpy((void *)p_atp_ctxt->current_atcmd.params,
                      (const CS_CHAR_t *)p_modem_ctxt->SID_ctxt.p_direct_cmd_tx->cmd_str,
                      str_size);

        /* add termination characters */
        uint32_t endstr_size = strlen((CRC_CHAR_t *)&p_atp_ctxt->endstr);
        (void) memcpy((void *)&p_atp_ctxt->current_atcmd.params[str_size],
                      p_atp_ctxt->endstr,
                      endstr_size);

        /* set raw command size */
        p_atp_ctxt->current_atcmd.raw_cmd_size = str_size + endstr_size;

      }
      else
      {
        PRINT_ERR("ERROR, send buffer is empty")
        retval = ATSTATUS_ERROR;
      }
    }
    else
    {
      PRINT_ERR("ERROR, no context available")
      retval = ATSTATUS_ERROR;
    }
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
