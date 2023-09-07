/**
  ******************************************************************************
  * @file    at_custom_modem_signalling.c
  * @author  MCD Application Team
  * @brief   This file provides all the 'signalling' code to the
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_custom_modem_signalling.h"
#include "at_custom_modem_specific.h"
#include "at_datapack.h"
#include "at_util.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#include "at_modem_socket.h"
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SIGNALLING AT_CUSTOM QUECTEL_BG96 SIGNALLING
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SIGNALLING_Private_Macros AT_CUSTOM QUECTEL_BG96 SIGNALLING Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "BG96:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "BG96:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "BG96 API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "BG96 ERROR:" format "\n\r", ## args)
#define PRINT_BUF(pbuf, size)       TRACE_PRINT_BUF_CHAR(DBG_CHAN_ATCMD, DBL_LVL_P1, (const CRC_CHAR_t *)pbuf, size);
#else
#define PRINT_INFO(format, args...)  (void) printf("BG96:" format "\n\r", ## args);
#define PRINT_DBG(...)  __NOP(); /* Nothing to do */
#define PRINT_API(...)  __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("BG96 ERROR:" format "\n\r", ## args);
#define PRINT_BUF(...)  __NOP(); /* Nothing to do */
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

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SIGNALLING_Private_Defines AT_CUSTOM QUECTEL_BG96 SIGNALLING Private Defines
  * @{
  */
/*
List of bands parameters (cf .h file to see the list of enum values for each parameter)
  - BG96_BAND_GSM    : hexadecimal value that specifies the GSM frequency band (cf AT+QCFG="band")
  - BG96_BAND_CAT_M1 : hexadecimal value that specifies the LTE Cat.M1 frequency band (cf AT+QCFG="band")
                       64bits bitmap split in two 32bits bitmaps  (MSB and LSB parts)
  - BG96_BAND_CAT_NB1: hexadecimal value that specifies the LTE Cat.NB1 frequency band (cf AT+QCFG="band")
                       64bits bitmap split in two 32bits bitmaps  (MSB and LSB parts)
  - BG96_IOTOPMODE   : network category to be searched under LTE network (cf AT+QCFG="iotopmode")
  - BG96_SCANSEQ     : network search sequence (GSM, Cat.M1, Cat.NB1) (cf AT+QCFG="nwscanseq")
  - BG96_SCANMODE    : network to be searched (cf AT+QCFG="nwscanmode")

Below are define default band values that will be used if calling write form of AT+QCFG
(mainly for test purposes)
*/
#define BG96_BAND_GSM          ((ATCustom_BG96_QCFGbandGSM_t)    QCFGBANDGSM_ANY)
#define BG96_BAND_CAT_M1_MSB   ((ATCustom_BG96_QCFGbandCatM1_t)  QCFGBANDCATM1_ANY_MSB)
#define BG96_BAND_CAT_M1_LSB   ((ATCustom_BG96_QCFGbandCatM1_t)  QCFGBANDCATM1_ANY_LSB)
#define BG96_BAND_CAT_NB1_MSB  ((ATCustom_BG96_QCFGbandCatNB1_t) QCFGBANDCATNB1_ANY_MSB)
#define BG96_BAND_CAT_NB1_LSB  ((ATCustom_BG96_QCFGbandCatNB1_t) QCFGBANDCATNB1_ANY_LSB)
#define BG96_IOTOPMODE         ((ATCustom_BG96_QCFGiotopmode_t)  QCFGIOTOPMODE_CATM1CATNB1)
#define BG96_SCANSEQ           ((ATCustom_BG96_QCFGscanseq_t)    QCFGSCANSEQ_M1_NB1_GSM)
#define BG96_SCANMODE          ((ATCustom_BG96_QCFGscanmode_t)   QCFGSCANMODE_AUTO)

#define BG96_PDP_DUPLICATECHK_ENABLE ((uint8_t)0U) /* parameter of AT+QCFG="PDP/DuplicateChk":
                                                    * 0 to refuse, 1 to allow
                                                    */

#define BG96_PSM_URC_ENABLE ((uint8_t)1U) /* parameter of AT+QCFG="psm/urc":
                                           * 0 to disable QPSMTIMER URC report
                                           * 1 to enable QPSMTIMER URC report
                                           */

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SIGNALLING_Exported_Functions AT_CUSTOM QUECTEL_BG96 SIGNALLING Exported Functions
  * @{
  */

/* Build command functions ------------------------------------------------------- */

/**
  * @brief  Build specific modem command : ATD.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_ATD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_ATD_BG96()")

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

/**
  * @brief  Build specific modem command : CGSN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_CGSN_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_CGSN_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* BG96 only supports EXECUTION form of CGSN */
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

/**
  * @brief  Build specific modem command : QPOWD.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QPOWD_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QPOWD_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* Normal Power Down */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "1");
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  /* Commands Look-up table for AT+QCFG
   * important: it has to be aligned with ATCustom_BG96_QCFG_function_t
   */
  static const AT_CHAR_t BG96_QCFG_LUT[][32] =
  {
    {"unknown"}, /* QCFG_unknown */
    {"gprsattach"}, /* QCFG_gprsattach */
    {"nwscanseq"}, /* QCFG_nwscanseq */
    {"nwscanmode"}, /* QCFG_nwscanmode */
    {"iotopmode"}, /* QCFG_iotopmode */
    {"roamservice"}, /* QCFG_roamservice */
    {"band"}, /* QCFG_band */
    {"servicedomain"}, /* QCFG_servicedomain */
    {"sgsn"}, /* QCFG_sgsn */
    {"msc"}, /* QCFG_msc */
    {"pdp/duplicatechk"}, /* QCFG_PDP_DuplicateChk */
    {"urc/ri/ring"}, /* QCFG_urc_ri_ring */
    {"urc/ri/smsincoming"}, /* QCFG_urc_ri_smsincoming */
    {"urc/ri/other"}, /* QCFG_urc_ri_other */
    {"signaltype"}, /* QCFG_signaltype */
    {"urc/delay"}, /* QCFG_urc_delay */
    {"psm/urc"}, /* QCFG_urc_psm */
  };

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    AT_CHAR_t cmd_param1[16] = "\0";
    AT_CHAR_t cmd_param2[16] = "\0";
    AT_CHAR_t cmd_param3[16] = "\0";
    AT_CHAR_t cmd_param4[16] = "\0";
    AT_CHAR_t cmd_param5[16] = "\0";
    uint8_t  cmd_nb_params = 0U;

    if (bg96_shared.QCFG_command_write == AT_TRUE)
    {
      /* BG96_AT_Commands_Manual_V2.0 */
      switch (bg96_shared.QCFG_command_param)
      {
        case QCFG_gprsattach:
          /* cmd_nb_params = 1U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_nwscanseq:
          cmd_nb_params = 2U;
          /* param 1 = scanseq */
          (void) sprintf((CRC_CHAR_t *)&cmd_param1, "0%lx",
                         BG96_SCANSEQ);  /* print as hexa but without prefix, need to add 1st digit = 0*/
          /* param 2 = effect */
          (void) sprintf((CRC_CHAR_t *)&cmd_param2, "%d", 1);  /* 1 means take effect immediately */
          break;
        case QCFG_nwscanmode:
          cmd_nb_params = 2U;
          /* param 1 = scanmode */
          (void) sprintf((CRC_CHAR_t *)&cmd_param1, "%ld", BG96_SCANMODE);
          /* param 2 = effect */
          (void) sprintf((CRC_CHAR_t *)&cmd_param2, "%d", 1);  /* 1 means take effect immediately */
          break;
        case QCFG_iotopmode:
          cmd_nb_params = 2U;
          /* param 1 = iotopmode */
          (void) sprintf((CRC_CHAR_t *)&cmd_param1, "%ld", BG96_IOTOPMODE);
          /* param 2 = effect */
          (void) sprintf((CRC_CHAR_t *)&cmd_param2, "%d", 1);  /* 1 means take effect immediately */
          break;
        case QCFG_roamservice:
          /* cmd_nb_params = 2U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_band:
          cmd_nb_params = 4U;
          /* param 1 = gsmbandval */
          (void) sprintf((CRC_CHAR_t *)&cmd_param1, "%lx", BG96_BAND_GSM);
          /* param 2 = catm1bandval */
          (void) sprintf((CRC_CHAR_t *)&cmd_param2, "%lx%lx", BG96_BAND_CAT_M1_MSB, BG96_BAND_CAT_M1_LSB);
          /* param 3 = catnb1bandval */
          (void) sprintf((CRC_CHAR_t *)&cmd_param3, "%lx%lx", BG96_BAND_CAT_NB1_MSB, BG96_BAND_CAT_NB1_LSB);
          /* param 4 = effect */
          (void) sprintf((CRC_CHAR_t *)&cmd_param4, "%d", 1);  /* 1 means take effect immediately */
          break;
        case QCFG_servicedomain:
          /* cmd_nb_params = 2U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_sgsn:
          /* cmd_nb_params = 1U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_msc:
          /* cmd_nb_params = 1U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_PDP_DuplicateChk:
          cmd_nb_params = 1U;
          /* param 1 = enable */
          (void) sprintf((CRC_CHAR_t *)&cmd_param1, "%d", BG96_PDP_DUPLICATECHK_ENABLE);
          break;
        case QCFG_urc_ri_ring:
          cmd_nb_params = 5U;
          /* NOT IMPLEMENTED */
          break;
        case QCFG_urc_ri_smsincoming:
          /* cmd_nb_params = 2U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_urc_ri_other:
          /* cmd_nb_params = 2U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_signaltype:
          /* cmd_nb_params = 1U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_urc_delay:
          /* cmd_nb_params = 1U */
          /* NOT IMPLEMENTED */
          break;
        case QCFG_urc_psm:
          cmd_nb_params = 1U;
          /* param 1 = iotopmode */
          (void) sprintf((CRC_CHAR_t *)&cmd_param1, "%d", BG96_PSM_URC_ENABLE);
          break;
        default:
          break;
      }
    }

    if (cmd_nb_params == 5U)
    {
      /* command has 5 parameters (this is a WRITE command) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s,%s,%s,%s",
                     BG96_QCFG_LUT[bg96_shared.QCFG_command_param],
                     cmd_param1, cmd_param2, cmd_param3, cmd_param4, cmd_param5);
    }
    else if (cmd_nb_params == 4U)
    {
      /* command has 4 parameters (this is a WRITE command) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s,%s,%s",
                     BG96_QCFG_LUT[bg96_shared.QCFG_command_param], cmd_param1, cmd_param2, cmd_param3, cmd_param4);
    }
    /* this case never happen until now
    * else if (cmd_nb_params == 3U)
    * { --
    *   command has 3 parameters (this is a WRITE command)
    *   (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s,%s",
    *                  BG96_QCFG_LUT[bg96_shared.QCFG_command_param], cmd_param1, cmd_param2, cmd_param3) --
    * } --
    */
    else if (cmd_nb_params == 2U)
    {
      /* command has 2 parameters (this is a WRITE command) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s,%s",
                     BG96_QCFG_LUT[bg96_shared.QCFG_command_param], cmd_param1, cmd_param2);
    }
    else if (cmd_nb_params == 1U)
    {
      /* command has 1 parameters (this is a WRITE command) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\",%s",
                     BG96_QCFG_LUT[bg96_shared.QCFG_command_param], cmd_param1);
    }
    else
    {
      /* command has 0 parameters (this is a READ command) */
      (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\"",
                     BG96_QCFG_LUT[bg96_shared.QCFG_command_param]);
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
at_status_t fCmdBuild_CGDCONT_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  at_status_t retval;
  PRINT_API("enter fCmdBuild_CGDCONT_BG96()")

  /* normal case */
  retval = fCmdBuild_CGDCONT(p_atp_ctxt, p_modem_ctxt);

  return (retval);
}

/**
  * @brief  Build specific modem command : QICFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QICFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QICFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QINDCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QINDCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QINDCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    switch (bg96_shared.QINDCFG_command_param)
    {
      case QINDCFG_csq:
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
        {
          /* subscribe to CSQ URC event, do not save to nvram */
          (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"csq\",1,0");
        }
        else
        {
          /* unsubscribe to CSQ URC event, do not save to nvram */
          (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"csq\",0,0");
        }
        break;

      case QINDCFG_all:
      case QINDCFG_smsfull:
      case QINDCFG_ring:
      case QINDCFG_smsincoming:
      default:
        /* not implemented yet or error */
        break;
    }
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QENG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QENG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  /* Commands Look-up table for AT+QENG */
  static const AT_CHAR_t BG96_QENG_LUT[][32] =
  {
    {"servingcell"}, /* servingcell */
    {"neighbourcell"}, /* neighbourcell */
    {"psinfo"}, /* psinfo */
  };

  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QENG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* cf addendum for description of QENG AT command (for debug only)
     * AT+QENG=<celltype>
     * Engineering mode used to report information of serving cells, neighbouring cells and packet
     * switch parameters.
     */

    /* select param: QENG_CELLTYPE_SERVINGCELL
     *               QENG_CELLTYPE_NEIGHBOURCELL
     *               QENG_CELLTYPE_PSINFO
     */
    uint8_t BG96_QENG_command_param = QENG_CELLTYPE_SERVINGCELL;

    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"%s\"", BG96_QENG_LUT[BG96_QENG_command_param]);
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QURCCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QURCCFG_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QURCCFG_BG96()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* set URC output port to UART */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "\"urcport\",\"uart1\"");
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : QPSMEXTCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_QPSMEXTCFG(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval = ATSTATUS_OK;
  PRINT_API("enter fCmdBuild_QPSMEXTCFG()")

  /* only for write command, set parameters */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD)
  {
    /* Normal Power Down */
    (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->current_atcmd.params, "0,,,,,3");
  }

  return (retval);
}

/**
  * @brief  Build specific modem command : COPS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_status_t fCmdBuild_COPS_BG96(atparser_context_t *p_atp_ctxt, atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);
  at_status_t retval;
  PRINT_API("enter fCmdBuild_COPS_BG96()")

  /* BG96 does not follow 3GPP TS 27.007 values for <AcT> parameter in AT+COPS
   * Encapsulate generic COPS build function to manage this difference
   */

  CS_OperatorSelector_t *operatorSelect = &(p_modem_ctxt->SID_ctxt.write_operator_infos);
  if (operatorSelect->AcT_present == CS_TRUE)
  {
    if (operatorSelect->AcT == CS_ACT_E_UTRAN)
    {
      /* BG96 AcT = 8 means cat.M1
      *  3GPP AcT = 8 means CS_ACT_EC_GSM_IOT, cat.M1 value is 9
      *  => convert CS_ACT_E_UTRAN to CS_ACT_EC_GSM_IOT) for BG96
      */
      operatorSelect->AcT = CS_ACT_EC_GSM_IOT;
    }
  }

  /* finally call the common COPS function */
  retval = fCmdBuild_COPS(p_atp_ctxt, p_modem_ctxt);

  return (retval);
}

/* Analyze command functions ------------------------------------------------------- */

/**
  * @brief  Analyze specific modem response : ERROR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_Error_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_ERROR;
  PRINT_API("enter fRspAnalyze_Error_BG96()")

  /* analyze Error for BG96 */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case CMD_AT_CREG:
    case CMD_AT_CGREG:
    case CMD_AT_CEREG:
      /* error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_CPSMS:
    case CMD_AT_QPSMCFG:
    case CMD_AT_QPSMEXTCFG:
    case CMD_AT_CEDRXS:
    case CMD_AT_QCSQ:
    case CMD_AT_QNWINFO:
    case CMD_AT_QENG:
      /* error is ignored */
      retval = ATACTION_RSP_FRC_END;
      break;

    case CMD_AT_CGDCONT:
      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_INIT_MODEM)
      {
        /* error is ignored in this case because this cmd is only informative */
        retval = ATACTION_RSP_FRC_END;
      }
      break;

    case CMD_AT_QINISTAT:
      /* error is ignored: this command is supported from BG96 modem FW
       *  needed to avoid blocking errors with previous modem FW versions
       */
      bg96_shared.QINISTAT_error = AT_TRUE;
      retval = ATACTION_RSP_FRC_CONTINUE;
      break;

    case CMD_AT_CPIN:
      /* error is ignored when bg96_sim_status_retries is not null
       *
       */
      if (bg96_shared.bg96_sim_status_retries != 0U)
      {
        PRINT_INFO("error ignored (waiting for SIM ready)")
        retval = ATACTION_RSP_FRC_CONTINUE;
      }
      else
      {
        retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      }
      break;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    case CMD_AT_QIOPEN:
      /* specific error case:
       *  when socket creation fails, the error management will be done in at_SID_CS_DIAL_COMMAND.
       */
      retval = ATACTION_RSP_FRC_CONTINUE;
      break;

    case CMD_AT_QICLOSE:
      if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DIAL_COMMAND)
      {
        /* specific error case:
         *  when QICLOSE requested during SID_CS_DIAL_COMMAND fails, errors will be managed in SID_CS_DIAL_COMMAND.
         */
        retval = ATACTION_RSP_FRC_CONTINUE;
      }
      else
      {
        retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      }
      break;

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

    default:
      retval = fRspAnalyze_Error(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);
      break;
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CPIN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CPIN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CPIN_BG96()")

  /*  Quectel BG96 AT Commands Manual V1.0
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
    /* this is an URC */
    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      PRINT_DBG("URC +CPIN received")
      PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)
    }
    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : CFUN.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_CFUN_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_CFUN_BG96()")

  /*  Quectel BG96 AT Commands Manual V1.0
  *   analyze parameters for +CFUN
  *
  *   if +CFUN is received, it's an URC
  */

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

/**
  * @brief  Analyze specific modem response : QIND.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QIND_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QIND_BG96()")

  at_bool_t bg96_current_qind_is_csq = AT_FALSE;
  /* FOTA infos */
  at_bool_t bg96_current_qind_is_fota = AT_FALSE;
  uint8_t bg96_current_qind_fota_action = 0U; /* 0: ignored FOTA action , 1: FOTA start, 2: FOTA end  */

  /*  Quectel BG96 AT Commands Manual V1.0
  *   analyze parameters for +QIND
  *
  *   it's an URC
  */

  /* this is an URC */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0};

    /* init param received info */
    bg96_current_qind_is_csq = AT_FALSE;

    PRINT_DBG("URC +QIND received")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);

      /* extract value and compare it to expected value */
      if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "csq") != NULL)
      {
        PRINT_DBG("SIGNAL QUALITY INFORMATION")
        bg96_current_qind_is_csq = AT_TRUE;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "FOTA") != NULL)
      {
        PRINT_DBG("URC FOTA infos received")
        bg96_current_qind_is_fota = AT_TRUE;
      }
      else
      {
        retval = ATACTION_RSP_URC_IGNORED;
        PRINT_DBG("QIND info not managed: urc ignored")
      }
    }
    else
    {
      PRINT_ERR("param ignored (exceed maximum size)")
      retval = ATACTION_RSP_IGNORED;
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    if (bg96_current_qind_is_csq == AT_TRUE)
    {
      uint32_t rssi = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                element_infos->str_size);
      PRINT_DBG("+CSQ rssi=%ld", rssi)
      p_modem_ctxt->persist.urc_avail_signal_quality = AT_TRUE;
      p_modem_ctxt->persist.signal_quality.rssi = (uint8_t)rssi;
      p_modem_ctxt->persist.signal_quality.ber = 99U; /* in case ber param is not present */
    }
    else if (bg96_current_qind_is_fota == AT_TRUE)
    {
      AT_CHAR_t line[32] = {0};
      /* copy element to line for parsing */
      if (element_infos->str_size <= 32U)
      {
        (void) memcpy((void *)&line[0],
                      (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                      (size_t) element_infos->str_size);
        /* WARNING KEEP ORDER UNCHANGED (END comparison has to be after HTTPEND and FTPEND) */
        if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "FTPSTART") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "FTPEND") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "HTTPSTART") != NULL)
        {
          PRINT_INFO("URC FOTA start detected !")
          bg96_current_qind_fota_action = 1U;
          if (atcm_modem_event_received(p_modem_ctxt, CS_MDMEVENT_FOTA_START) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "HTTPEND") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "START") != NULL)
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "END") != NULL)
        {
          PRINT_INFO("URC FOTA end detected !")
          bg96_current_qind_fota_action = 2U;
          if (atcm_modem_event_received(p_modem_ctxt, CS_MDMEVENT_FOTA_END) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        else
        {
          bg96_current_qind_fota_action = 0U; /* ignored */
          retval = ATACTION_RSP_URC_IGNORED;
        }
      }
      else
      {
        PRINT_ERR("param ignored (exceed maximum size)")
        retval = ATACTION_RSP_IGNORED;
      }
    }
    else
    {
      /* ignore */
    }

  }
  else if (element_infos->param_rank == 4U)
  {
    if (bg96_current_qind_is_csq == AT_TRUE)
    {
      uint32_t ber = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size);
      PRINT_DBG("+CSQ ber=%ld", ber)
      p_modem_ctxt->persist.signal_quality.ber = (uint8_t)ber;
    }
    else if (bg96_current_qind_is_fota == AT_TRUE)
    {
      if (bg96_current_qind_fota_action == 1U)
      {
        /* FOTA END status
         * parameter ignored for the moment
         */
        retval = ATACTION_RSP_URC_IGNORED;
      }
    }
    else
    {
      /* ignored */
      __NOP(); /* to avoid warning */
    }
  }
  else
  {
    /* other parameters ignored */
    __NOP(); /* to avoid warning */
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QINDCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QINDCFG_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                         const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  UNUSED(p_msg_in);
  UNUSED(element_infos);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QINDCFG_BG96()")

  /* not implemented yet */

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QCFG.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QCFG_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);

  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QCFG_BG96()")

  /* memorize which is current QCFG command received */
  ATCustom_BG96_QCFG_function_t bg96_current_qcfg_cmd = QCFG_unknown;

  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    AT_CHAR_t line[32] = {0};

    /* init param received info */
    bg96_current_qcfg_cmd = QCFG_unknown;

    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* copy element to line for parsing */
    if (element_infos->str_size <= 32U)
    {
      (void) memcpy((void *)&line[0],
                    (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                    (size_t) element_infos->str_size);

      /* extract value and compare it to expected value */
      if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "nwscanseq") != NULL)
      {
        PRINT_DBG("+QCFG nwscanseq infos received")
        bg96_current_qcfg_cmd = QCFG_nwscanseq;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "nwscanmode") != NULL)
      {
        PRINT_DBG("+QCFG nwscanmode infos received")
        bg96_current_qcfg_cmd = QCFG_nwscanmode;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "iotopmode") != NULL)
      {
        PRINT_DBG("+QCFG iotopmode infos received")
        bg96_current_qcfg_cmd = QCFG_iotopmode;
      }
      else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "band") != NULL)
      {
        PRINT_DBG("+QCFG band infos received")
        bg96_current_qcfg_cmd = QCFG_band;
      }
      else
      {
        PRINT_ERR("+QCFDG field not managed")
      }
    }
    else
    {
      PRINT_ERR("param ignored (exceed maximum size)")
      retval = ATACTION_RSP_IGNORED;
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    switch (bg96_current_qcfg_cmd)
    {
      case QCFG_nwscanseq:
        bg96_shared.mode_and_bands_config.nw_scanseq =
          (ATCustom_BG96_QCFGscanseq_t) ATutil_convertHexaStringToInt32(
            &p_msg_in->buffer[element_infos->str_start_idx],
            element_infos->str_size);
        break;
      case QCFG_nwscanmode:
        bg96_shared.mode_and_bands_config.nw_scanmode =
          (ATCustom_BG96_QCFGscanmode_t) ATutil_convertStringToInt(
            &p_msg_in->buffer[element_infos->str_start_idx],
            element_infos->str_size);
        break;
      case QCFG_iotopmode:
        bg96_shared.mode_and_bands_config.iot_op_mode =
          (ATCustom_BG96_QCFGiotopmode_t) ATutil_convertStringToInt(
            &p_msg_in->buffer[element_infos->str_start_idx],
            element_infos->str_size);
        break;
      case QCFG_band:
        bg96_shared.mode_and_bands_config.gsm_bands =
          (ATCustom_BG96_QCFGbandGSM_t) ATutil_convertHexaStringToInt32(
            &p_msg_in->buffer[element_infos->str_start_idx],
            element_infos->str_size);
        break;
      default:
        break;
    }
  }
  else if (element_infos->param_rank == 4U)
  {
    switch (bg96_current_qcfg_cmd)
    {
      case QCFG_band:
        (void) ATutil_convertHexaStringToInt64(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size,
                                               &bg96_shared.mode_and_bands_config.CatM1_bands_MsbPart,
                                               &bg96_shared.mode_and_bands_config.CatM1_bands_LsbPart);
        break;
      default:
        break;
    }
  }
  else if (element_infos->param_rank == 5U)
  {
    switch (bg96_current_qcfg_cmd)
    {
      case QCFG_band:
        (void) ATutil_convertHexaStringToInt64(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size,
                                               &bg96_shared.mode_and_bands_config.CatNB1_bands_MsbPart,
                                               &bg96_shared.mode_and_bands_config.CatNB1_bands_LsbPart);
        break;
      default:
        break;
    }
  }
  else
  {
    /* other parameters ignored */
    __NOP(); /* to avoid warning */
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QCCID.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QCCID_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                       const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);

  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fRspAnalyze_QCCID_BG96()")

  /* analyze parameters for +QCCID */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PRINT_DBG("ICCID:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    /* BG96 specific treatment:
     *  ICCID reported by the modem includes a blank character (space, code=0x20) at the beginning
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

    if (p_modem_ctxt->SID_ctxt.p_device_info != NULL)
    {
      /* copy ICCID */
      (void) memcpy((void *) & (p_modem_ctxt->SID_ctxt.p_device_info->u.iccid),
                    (const void *)&p_msg_in->buffer[src_idx],
                    (size_t)ccid_size);
    }
  }
  else
  {
    /* other parameters ignored */
    __NOP(); /* to avoid warning */
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QINISTAT.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QINISTAT_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                          const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);

  at_action_rsp_t retval = ATACTION_RSP_INTERMEDIATE; /* received a valid intermediate answer */
  PRINT_API("enter fRspAnalyze_QINISTAT_BG96()")

  /* analyze parameters for +QINISTAT */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    PRINT_DBG("QINISTAT:")
    PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

    uint32_t sim_status = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                    element_infos->str_size);
    /* check is CPIN is ready */
    if ((sim_status & QCINITSTAT_CPINREADY) != 0U)
    {
      p_modem_ctxt->persist.modem_sim_ready = AT_TRUE;
      PRINT_INFO("Modem SIM is ready")
    }
    else
    {
      p_modem_ctxt->persist.modem_sim_ready = AT_FALSE;
      PRINT_INFO("Modem SIM not ready yet...")
    }
  }
  else
  {
    /* other parameters ignored */
    __NOP(); /* to avoid warning */
  }
  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QCSQ.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QCSQ_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_modem_ctxt);
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QCSQ_BG96()")

  /* memorize sysmode for current QCSQ */
  ATCustom_BG96_QCSQ_sysmode_t bg96_current_qcsq_sysmode = QCSQ_unknown;

  /* analyze parameters for QCSQ */
  if (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD)
  {
    /*
    *  format: +QCSQ: <sysmode>,sysmode>,[,<value1>[,<value2>[,<value3>[,<value4>]]]]
    *
    *  <sysmode> "NOSERVICE", "GSM", "CAT-M1" or "CAT-NB1"
    *
    * if <sysmode> = "NOSERVICE"
    *    no values
    * if <sysmode> = "GSM"
    *    <value1> = <gsm_rssi>
    * if <sysmode> = "CAT-M1" or "CAT-NB1"
    *    <value1> = <lte_rssi> / <value2> = <lte_rssp> / <value3> = <lte_sinr> / <value4> = <lte_rsrq>
    */

    START_PARAM_LOOP()
    if (element_infos->param_rank == 2U)
    {
      AT_CHAR_t line[32] = {0};

      /* init param received info */
      bg96_current_qcsq_sysmode = QCSQ_unknown;

      PRINT_BUF((const uint8_t *)&p_msg_in->buffer[element_infos->str_start_idx], element_infos->str_size)

      /* copy element to line for parsing */
      if (element_infos->str_size <= 32U)
      {
        (void) memcpy((void *)&line[0],
                      (const void *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                      (size_t) element_infos->str_size);

        /* extract value and compare it to expected value */
        if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "NOSERVICE") != NULL)
        {
          PRINT_DBG("+QCSQ sysmode=NOSERVICE")
          bg96_current_qcsq_sysmode = QCSQ_noService;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "GSM") != NULL)
        {
          PRINT_DBG("+QCSQ sysmode=GSM")
          bg96_current_qcsq_sysmode = QCSQ_gsm;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "CAT-M1") != NULL)
        {
          PRINT_DBG("+QCSQ sysmode=CAT-M1")
          bg96_current_qcsq_sysmode = QCSQ_catM1;
        }
        else if ((AT_CHAR_t *) strstr((const CRC_CHAR_t *)&line[0], "CAT-NB1") != NULL)
        {
          PRINT_DBG("+QCSQ sysmode=CAT-NB1")
          bg96_current_qcsq_sysmode = QCSQ_catNB1;
        }
        else
        {
          PRINT_ERR("+QCSQ field not managed")
        }
      }
      else
      {
        PRINT_ERR("param ignored (exceed maximum size)")
        retval = ATACTION_RSP_IGNORED;
      }
    }
    else if (element_infos->param_rank == 3U)
    {
      /* <value1> */
      switch (bg96_current_qcsq_sysmode)
      {
        case QCSQ_gsm:
          /* <gsm_rssi> */
          PRINT_DBG("<gsm_rssi> = %s%ld",
                    (ATutil_isNegative(&p_msg_in->buffer[element_infos->str_start_idx],
                                       element_infos->str_size) == 1U) ? "-" : " ",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
          break;

        case QCSQ_catM1:
        case QCSQ_catNB1:
          /* <lte_rssi> */
          PRINT_DBG("<lte_rssi> = %s%ld",
                    (ATutil_isNegative(&p_msg_in->buffer[element_infos->str_start_idx],
                                       element_infos->str_size) == 1U) ? "-" : " ",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
          break;

        default:
          /* parameter ignored */
          break;
      }
    }
    else if (element_infos->param_rank == 4U)
    {
      /* <value2> */
      switch (bg96_current_qcsq_sysmode)
      {
        case QCSQ_catM1:
        case QCSQ_catNB1:
          /* <lte_rsrp> */
          /* rsrp range is -44 dBm to -140 dBm */
          PRINT_INFO("<lte_rsrp> = %s%ld dBm",
                     (ATutil_isNegative(&p_msg_in->buffer[element_infos->str_start_idx],
                                        element_infos->str_size) == 1U) ? "-" : " ",
                     ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                               element_infos->str_size))
          break;

        default:
          /* parameter ignored */
          break;
      }
    }
    else if (element_infos->param_rank == 5U)
    {
      /* <value3> */
      switch (bg96_current_qcsq_sysmode)
      {
        case QCSQ_catM1:
        case QCSQ_catNB1:
          /* <lte_sinr> */
          PRINT_DBG("<lte_sinr> = %s%ld",
                    (ATutil_isNegative(&p_msg_in->buffer[element_infos->str_start_idx],
                                       element_infos->str_size) == 1U) ? "-" : " ",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
          break;

        default:
          /* parameter ignored */
          break;
      }
    }
    else if (element_infos->param_rank == 6U)
    {
      /* <value4> */
      switch (bg96_current_qcsq_sysmode)
      {
        case QCSQ_catM1:
        case QCSQ_catNB1:
          /* <lte_rsrq> */
          PRINT_DBG("<lte_rsrq> = %s%ld",
                    (ATutil_isNegative(&p_msg_in->buffer[element_infos->str_start_idx],
                                       element_infos->str_size) == 1U) ? "-" : " ",
                    ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                              element_infos->str_size))
          break;

        default:
          /* parameter ignored */
          break;
      }
    }
    else
    {
      /* parameter ignored */
      __NOP(); /* to avoid warning */
    }

    END_PARAM_LOOP()
  }

  return (retval);
}

/**
  * @brief  Analyze specific modem response : QGMR.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QGMR_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QGMR_BG96()")

  /* analyze parameters for +QGMR */
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
  * @brief  Analyze specific modem response : QPSMTIMER.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_QPSMTIMER_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                           const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  UNUSED(p_at_ctxt);
  UNUSED(p_modem_ctxt);
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter fRspAnalyze_QPSMTIMER_BG96()")

  /* analyze parameters for +QPSMTIMER
   * this is an URC
   */
  START_PARAM_LOOP()
  if (element_infos->param_rank == 2U)
  {
    uint32_t t3412_nwk_value = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                         element_infos->str_size);
    PRINT_INFO("URC +QPSMTIMER received: TAU_duration (T3412) = %ld sec", t3412_nwk_value)

    if (t3412_nwk_value != p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU)
    {
      p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU = t3412_nwk_value;
      p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
      PRINT_DBG("New T3412 value detected")
    }
  }
  else if (element_infos->param_rank == 3U)
  {
    uint32_t t3324_nwk_value = ATutil_convertStringToInt(&p_msg_in->buffer[element_infos->str_start_idx],
                                                         element_infos->str_size);
    PRINT_INFO("URC +QPSMTIMER received: Active_duration (T3324) = %ld sec", t3324_nwk_value)

    if (t3324_nwk_value != p_modem_ctxt->persist.low_power_status.nwk_active_time)
    {
      p_modem_ctxt->persist.low_power_status.nwk_active_time = t3324_nwk_value;
      p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
      PRINT_DBG("New T3324 value detected")
    }
  }
  else
  {
    /* parameter ignored */
    __NOP(); /* to avoid warning */
  }

  END_PARAM_LOOP()

  return (retval);
}

/**
  * @brief  Analyze specific modem response : COPS.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_status_t
  */
at_action_rsp_t fRspAnalyze_COPS_BG96(at_context_t *p_at_ctxt, atcustom_modem_context_t *p_modem_ctxt,
                                      const IPC_RxMessage_t *p_msg_in, at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PRINT_API("enter fRspAnalyze_COPS_BG96()")

  /* BG96 does not follow 3GPP TS 27.007 values for <AcT> parameter in AT+COPS
  * Encapsulate generic COPS build function to manage this difference
  */
  retval = fRspAnalyze_COPS(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos);

  if (retval != ATACTION_RSP_ERROR)
  {
    if ((p_modem_ctxt->SID_ctxt.read_operator_infos.optional_fields_presence & CS_RSF_ACT_PRESENT) != 0U)
    {
      if (p_modem_ctxt->SID_ctxt.read_operator_infos.AcT == CS_ACT_EC_GSM_IOT)
      {
        /* BG96 AcT = 8 means cat.M1
        *  3GPP AcT = 8 means CS_ACT_EC_GSM_IOT, cat.M1 value if 9
        *  => convert CS_ACT_EC_GSM_IOT to CS_ACT_E_UTRAN for upper layers
        */
        p_modem_ctxt->SID_ctxt.read_operator_infos.AcT = CS_ACT_E_UTRAN;
      }
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

/**
  * @}
  */

