/**
  ******************************************************************************
  * @file    at_modem_common.c
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_core.h"
#include "at_modem_common.h"
#include "at_modem_api.h"
#include "at_modem_socket.h"
#include "at_modem_signalling.h"
#include "at_datapack.h"
#include "at_util.h"
#include "at_sysctrl.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_COMMON AT_CORE COMMON
  * @{
  */

/** @defgroup AT_CORE_COMMON_Private_Macros AT_CORE COMMON Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_MODEM == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "ATCModem:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "ATCModem:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "ATCModem API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "ATCModem ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...)  (void) printf("ATCModem:" format "\n\r", ## args);
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("ATCModem ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_INFO(...)   __NOP(); /* Nothing to do */
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_API(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_ATCUSTOM_MODEM */
/**
  * @}
  */

/** @defgroup AT_CORE_COMMON_Private_Functions_Prototypes AT_CORE COMMON Private Functions Prototypes
  * @{
  */
static void reserve_user_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt,
                                   CS_PDN_conf_id_t conf_id,
                                   uint8_t reserved_modem_cid);
static void affect_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt,
                             CS_PDN_conf_id_t conf_id);
/**
  * @}
  */

/** @defgroup AT_CORE_COMMON_Exported_Functions  AT_CORE COMMON Exported Functions
  * @{
  */
/**
  * @brief  Put IP Address infos for the selected PDP config.
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @param  modem_cid Modem CID.
  * @param  ip_addr_info Pointer to IP address information buffer.
  * @retval none
  */
void atcm_put_IP_address_infos(atcustom_persistent_context_t *p_persistent_ctxt,
                               uint8_t modem_cid,
                               csint_ip_addr_info_t *ip_addr_info)
{
  bool leave_loop = false;
  uint8_t i = 0U;
  /* parse modem cid table to find matching value */
  do
  {
    atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[i];
    if (p_tmp->mdm_cid_value == modem_cid)
    {
      /* save IP address parameters */
      (void) memcpy((void *)& p_tmp->ip_addr_infos,
                    (void *)ip_addr_info,
                    sizeof(csint_ip_addr_info_t));
      leave_loop = true;
    }
    i++;
  } while ((leave_loop == false) && (i < MODEM_MAX_NB_PDP_CTXT));

  return;
}

/**
  * @brief  Get IP Address infos for the selected PDP config
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @param  conf_id PDN configuration ID.
  * @param  ip_addr_info Pointer to IP address information buffer.
  * @retval none
  */
void atcm_get_IP_address_infos(atcustom_persistent_context_t *p_persistent_ctxt,
                               CS_PDN_conf_id_t conf_id,
                               csint_ip_addr_info_t  *ip_addr_info)
{
  bool leave_loop = false;
  uint8_t i = 0U;
  /* parse modem cid table to find matching value */
  do
  {
    atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[i];
    if (p_tmp->affected_config == conf_id)
    {
      /* retrieve IP address parameters */
      (void) memcpy((void *)ip_addr_info,
                    (void *)& p_tmp->ip_addr_infos,
                    sizeof(csint_ip_addr_info_t));
      leave_loop = true;
    }
    i++;
  } while ((leave_loop == false) && (i < MODEM_MAX_NB_PDP_CTXT));

  return;
}

/**
  * @brief  Get IP Address type.
  * @note   Analyze IP address buffer to identify IP address type (IPv4 or IPv6).
  * @param  p_addr_str Pointer to IP address buffer.
  * @retval CS_IPaddrType_t
  */
CS_IPaddrType_t atcm_get_ip_address_type(AT_CHAR_t *p_addr_str)
{
  CS_IPaddrType_t retval;
  /* parse the address to determine its type:
  *  IPv4 address: 32 bits represented by 4 groups, group separator = '.'
  *                format= aaaa.bbbb.cccc.dddd
  *                (max string size = 19 characters)
  *  IPv6 address: 128 bits represented by 8 groups, group separator = ':'
  *                format= aaaa:bbbb:cccc:dddd:eeee:ffff:gggg:hhhh
  *                (max string size = 39 characters)
  *                According to 3GP TS27.007, IPv6 can also use IPv4-like dot-notation:
  *                format= a1.a2.a3.a4.a5a.a6.a7.a8.a9.a10.a11.a12a.a13.a14.a15.a16  (where 0<=ax<=255)
  *                (max string size = 63 characters )
  *
  */
  if (p_addr_str != NULL)
  {
    uint8_t str_size_cpt = (uint8_t) strlen((const CRC_CHAR_t *)p_addr_str);
    str_size_cpt = (str_size_cpt > 39U) ? 39U : str_size_cpt;
    uint8_t count_dots = 0U;
    uint8_t count_colons = 0U;
    const AT_CHAR_t *pTmp = p_addr_str;

    /* start parsing of the IP address */
    do
    {
      /* count '.' characters */
      if (*pTmp == ((AT_CHAR_t)'.'))
      {
        count_dots++;
      }
      /* count ':' characters */
      else if (*pTmp == ((AT_CHAR_t)':'))
      {
        count_colons++;
      }
      else
      {
        /* nothing to do - MISRA */
      }
      /* next character */
      pTmp++;
      str_size_cpt--;
    }  while (str_size_cpt > 0U);

    /* analyze result to determine IP address type */
    if ((count_dots >= 1U) && (count_dots <= 3U))
    {
      /* IPv4 type */
      retval = CS_IPAT_IPV4;
    }
    else if (((count_colons >= 1U) && (count_colons <= 7U)) ||
             (count_dots > 3U))
    {
      /* IPv6 type */
      retval = CS_IPAT_IPV6;
    }
    else
    {
      /* can't find a valid type */
      retval = CS_IPAT_INVALID;
    }
  }
  else
  {
    retval = CS_IPAT_INVALID;
  }

  return (retval);
}

/**
  * @brief  Extract IP address from a p_Src buffer, remove double quotes ""
  *         and recopy the IP  address to p_Dst buffer.
  * @param  p_Src ptr to source buffer (IP address + quotes).
  * @param  size of p_Src buffer.
  * @param  p_Dst ptr to Destination Buffer !!! this buffer has to be defined
  *               with buffer[MAX_IP_ADDR_SIZE] !!!
  * @retval at_status_t.
  */
void atcm_extract_IP_address(const uint8_t *p_Src, uint16_t size, uint8_t *p_Dst)
{
  uint16_t src_idx;
  uint16_t dest_idx = 0U;

  /* reset p_Dst buffer */
  (void) memset((void *)p_Dst, 0, MAX_IP_ADDR_SIZE);

  /* parse p_Src */
  for (src_idx = 0; ((src_idx < size) && (dest_idx < MAX_IP_ADDR_SIZE)); src_idx++)
  {
    /* remove quotes from the string */
    if (p_Src[src_idx] != 0x22U)
    {
      /* write to p_Dst*/
      p_Dst[dest_idx] = p_Src[src_idx];
      dest_idx++;
    }
  }
}

/**
  * @brief  Selection of the SIM slot to use.
  * @param  sim SIM slot.
  * @retval at_status_t.
  */
at_status_t atcm_select_hw_simslot(CS_SimSlot_t sim)
{
  at_status_t retval = ATSTATUS_OK;

  /* step 1:
  * convert sim slot from "Cellulat Service format" to "SysCtrl format"
  */
  sysctrl_sim_slot_t converted_sim;

  switch (sim)
  {
    case CS_MODEM_SIM_SOCKET_0:
      converted_sim = SC_MODEM_SIM_SOCKET_0;
      break;
    case CS_MODEM_SIM_ESIM_1:
      converted_sim = SC_MODEM_SIM_ESIM_1;
      break;
    case CS_STM32_SIM_2:
      converted_sim = SC_STM32_SIM_2;
      break;
    default:
      converted_sim = SC_MODEM_SIM_SOCKET_0;
      PRINT_ERR("Invalid sim slot value, use default one")
      break;
  }

  /* step 2:
  * call sysctrl interface
  */
  if (SysCtrl_sim_select(DEVTYPE_MODEM_CELLULAR, converted_sim) != SCSTATUS_OK)
  {
    retval = ATSTATUS_ERROR;
  }

  return (retval);
}

/**
  * @brief  Get conf_ig in current SID context (interpret default if needed)
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval CS_PDN_conf_id_t
  */
CS_PDN_conf_id_t atcm_get_cid_current_SID(atcustom_modem_context_t *p_modem_ctxt)
{
  /* get conf_id received for current SID */
  CS_PDN_conf_id_t current_conf_id = p_modem_ctxt->SID_ctxt.pdn_conf_id;

  /* if default PDN is required, retrieve corresponding conf_id */
  if (current_conf_id == CS_PDN_CONFIG_DEFAULT)
  {
    current_conf_id = p_modem_ctxt->persist.pdn_default_conf_id;
    PRINT_DBG("Default PDP context selected (conf_id = %d)", current_conf_id)
  }

  return (current_conf_id);
}

/**
  * @brief  Search command string corresponding to a command Id
  *
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  cmd_id Id of the command to find.
  * @retval string of the command name
  */
const AT_CHAR_t *atcm_get_CmdStr(const atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  const AT_CHAR_t *retval = ((uint8_t *)"");
  uint16_t i = 0U;
  bool leave_loop = false;

  /* check if this is the invalid cmd id*/
  if (cmd_id != CMD_AT_INVALID)
  {
    /* search in LUT the cmd ID */
    do
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        retval = (const AT_CHAR_t *)(&p_modem_ctxt->p_modem_LUT[i].cmd_str);
        leave_loop = true;
      }
      i++;
    } while ((leave_loop == false) && (i < p_modem_ctxt->modem_LUT_size));
  }

  return (retval);
}

/**
  * @brief  Search timeout corresponding to a command Id
  *
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  cmd_id Id of the command to find
  * @retval timeout for this command
  */
uint32_t atcm_get_CmdTimeout(const atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  uint32_t retval = MODEM_DEFAULT_TIMEOUT;

  /* check if this is the invalid cmd id */
  if (cmd_id != CMD_AT_INVALID)
  {
    /* search in LUT the cmd ID */
    bool leave_loop = false;
    uint16_t i = 0U;
    do
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        retval = p_modem_ctxt->p_modem_LUT[i].cmd_timeout;
        leave_loop = true;
      }
      i++;
    } while ((leave_loop == false) && (i < p_modem_ctxt->modem_LUT_size));
  }

  return (retval);
}

/**
  * @brief  Search Build Function corresponding to a command Id
  *
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  cmd_id Id of the command to find
  * @retval BuildFunction
  */
CmdBuildFuncTypeDef atcm_get_CmdBuildFunc(const atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  CmdBuildFuncTypeDef retval = fCmdBuild_NoParams; /* return default value */

  if (cmd_id != CMD_AT_INVALID)
  {
    uint16_t i = 0U;
    bool leave_loop = false;
    /* search in LUT the cmd ID */
    do
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        retval = p_modem_ctxt->p_modem_LUT[i].cmd_BuildFunc;
        leave_loop = true;
      }
      i++;
    } while ((leave_loop == false) && (i < p_modem_ctxt->modem_LUT_size));
  }

  return (retval);
}

/**
  * @brief  Search Analyze Function corresponding to a command Id
  *
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  cmd_id Id of the command to find
  * @retval AnalyzeFunction
  */
CmdAnalyzeFuncTypeDef atcm_get_CmdAnalyzeFunc(const atcustom_modem_context_t *p_modem_ctxt, uint32_t cmd_id)
{
  CmdAnalyzeFuncTypeDef retval = fRspAnalyze_None;

  if (cmd_id != CMD_AT_INVALID)
  {
    /* search in LUT the cmd ID */
    uint16_t i = 0U;
    bool leave_loop = false;
    do
    {
      if (p_modem_ctxt->p_modem_LUT[i].cmd_id == cmd_id)
      {
        retval = p_modem_ctxt->p_modem_LUT[i].rsp_AnalyzeFunc;
        leave_loop = true;
      }
      i++;
    } while ((leave_loop == false) && (i < p_modem_ctxt->modem_LUT_size));
  }

  return (retval);
}

/**
  * @brief  Get type of PDP address
  * @param  pdp_type
  * @retval AT_CHAR_t
  */
const AT_CHAR_t *atcm_get_PDPtypeStr(CS_PDPtype_t pdp_type)
{
  const AT_CHAR_t *retval = ((uint8_t *)"");

  /* LUT definition:
   * correspondence between PDPTYPE enum and string (for CGDCONT)
   */
  static const atcm_pdp_type_LUT_t ACTM_PDP_TYPE_LUT[] =
  {
    {CS_PDPTYPE_IP,     "IP"},
    {CS_PDPTYPE_IPV6,   "IPV6"},
    {CS_PDPTYPE_IPV4V6, "IPV4V6"},
    {CS_PDPTYPE_PPP,    "PPP"},
  };
  uint16_t i = 0U;
  uint16_t max_array_size = (uint16_t)(sizeof(ACTM_PDP_TYPE_LUT) / sizeof(atcm_pdp_type_LUT_t));
  bool leave_loop = false;
  do
  {
    if (pdp_type == ACTM_PDP_TYPE_LUT[i].pdp_type)
    {
      retval = ((const AT_CHAR_t *)(&ACTM_PDP_TYPE_LUT[i].pdp_type_string));
      leave_loop = true;
    }
    i++;
  } while ((leave_loop == false) && (i < max_array_size));

  /* string no found, return empty string */
  return (retval);
}

/**
  * @brief  Program an AT command: answer is mandatory, an error will be raised if no answer received before timeout
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  cmd_type Type of AT command.
  * @param  final Indicates it this is the last command for current Service ID.
  * @retval none
  */
void atcm_program_AT_CMD(atcustom_modem_context_t *p_modem_ctxt,
                         atparser_context_t *p_atp_ctxt,
                         at_type_t cmd_type,
                         uint32_t cmd_id,
                         atcustom_FinalCmd_t final)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = cmd_type;
  /* command id */
  p_atp_ctxt->current_atcmd.id = cmd_id;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  /* is an answer is expected ? */
  p_atp_ctxt->answer_expected = CMD_MANDATORY_ANSWER_EXPECTED;

  /* set command timeout according to LUT */
  p_atp_ctxt->cmd_timeout =  atcm_get_CmdTimeout(p_modem_ctxt, p_atp_ctxt->current_atcmd.id);
}

/**
  * @brief  Program an AT command: answer is optional, no error will be raised if no answer received before timeout
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  cmd_type Type of AT command.
  * @param  final Indicates it this is the last command for current Service ID.
  * @retval none
  */
void atcm_program_AT_CMD_ANSWER_OPTIONAL(atcustom_modem_context_t *p_modem_ctxt,
                                         atparser_context_t *p_atp_ctxt,
                                         at_type_t cmd_type,
                                         uint32_t cmd_id,
                                         atcustom_FinalCmd_t final)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = cmd_type;
  /* command id */
  p_atp_ctxt->current_atcmd.id = cmd_id;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  /* is an answer is expected ? */
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;

  /* set command timeout according to LUT */
  p_atp_ctxt->cmd_timeout =  atcm_get_CmdTimeout(p_modem_ctxt, p_atp_ctxt->current_atcmd.id);
}

/**
  * @brief  Allow to specify a command timeout different from the default one.
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  new_timeout Timeout to apply for the command.
  * @retval none
  */
void atcm_program_CMD_TIMEOUT(atcustom_modem_context_t *p_modem_ctxt,
                              atparser_context_t *p_atp_ctxt,
                              uint32_t new_timeout)
{
  if (new_timeout == 0U)
  {
    /* set command timeout according to LUT */
    p_atp_ctxt->cmd_timeout =  atcm_get_CmdTimeout(p_modem_ctxt, p_atp_ctxt->current_atcmd.id);
  }
  else
  {
    /* set timeout value to provided value */
    p_atp_ctxt->cmd_timeout = new_timeout;
  }
}

/**
  * @brief  Program an event to wait: if event does not occur before the specified time, an error will be raised
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  tempo_value Timeout value to apply.
  * @param  final Indicates it this is the last command for current Service ID.
  * @retval none
  */
void atcm_program_WAIT_EVENT(atparser_context_t *p_atp_ctxt, uint32_t tempo_value, atcustom_FinalCmd_t final)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  /* command id */
  p_atp_ctxt->current_atcmd.id = CMD_AT_INVALID;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  /* is an answer is expected ? */
  p_atp_ctxt->answer_expected = CMD_MANDATORY_ANSWER_EXPECTED;
  /* set command timeout */
  p_atp_ctxt->cmd_timeout = tempo_value;
}

/**
  * @brief  Program a tempo: nothing special expected, if no event there is no error raised
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  tempo_value Timeout to apply for the command.
  * @param  final Indicates it this is the last command for current Service ID.
  * @retval none
  */
void atcm_program_TEMPO(atparser_context_t *p_atp_ctxt, uint32_t tempo_value, atcustom_FinalCmd_t final)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  /* command id */
  p_atp_ctxt->current_atcmd.id = CMD_AT_INVALID;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = (final == FINAL_CMD) ? 1U : 0U;
  /* is an answer is expected ? */
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;
  /* set command timeout */
  p_atp_ctxt->cmd_timeout = tempo_value;

  PRINT_INFO("Tempo started (%ld ms)...", tempo_value)
}

/**
  * @brief  No command to send: nothing to send or to wait, last command
  * @param  p_atp_ctxt Pointer to parser context.
  * @retval none
  */
void atcm_program_NO_MORE_CMD(atparser_context_t *p_atp_ctxt)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  /* command id */
  p_atp_ctxt->current_atcmd.id = CMD_AT_INVALID;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = 1U;
  /* is an answer is expected ? */
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;
  /* set command timeout */
  p_atp_ctxt->cmd_timeout = 0U;
}

/**
  * @brief  Skip command to send: nothing to send or to wait, not the last command
  * @param  p_atp_ctxt Pointer to parser context.
  * @retval none
  */
void atcm_program_SKIP_CMD(atparser_context_t *p_atp_ctxt)
{
  /* command type */
  p_atp_ctxt->current_atcmd.type = ATTYPE_NO_CMD;
  /* command id */
  p_atp_ctxt->current_atcmd.id = CMD_AT_INVALID;
  /* is it final command ? */
  p_atp_ctxt->is_final_cmd = 0U;
  /* is an answer is expected ? */
  p_atp_ctxt->answer_expected = CMD_OPTIONAL_ANSWER_EXPECTED;
  /* set command timeout */
  p_atp_ctxt->cmd_timeout = 0U;
}

/**
  * @brief  atcm_modem_init
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval none
  */
void atcm_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_modem_init")

  /* reset all contexts at init */
  atcm_reset_persistent_context(&p_modem_ctxt->persist);
  atcm_reset_SID_context(&p_modem_ctxt->SID_ctxt);
  atcm_reset_CMD_context(&p_modem_ctxt->CMD_ctxt);
  atcm_reset_SOCKET_context(p_modem_ctxt);
  p_modem_ctxt->state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

/**
  * @brief  atcm_modem_reset
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval none
  */
void atcm_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_modem_reset")

  /* reset all contexts except SID when reset */
  atcm_reset_persistent_context(&p_modem_ctxt->persist);
  atcm_reset_CMD_context(&p_modem_ctxt->CMD_ctxt);
  atcm_reset_SOCKET_context(p_modem_ctxt);
  p_modem_ctxt->state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

/**
  * @brief  atcm_modem_build_cmd
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  p_ATcmdTimeout Pointer to Timetout value defined for this command.
  * @retval at_status_t
  */
at_status_t atcm_modem_build_cmd(atcustom_modem_context_t *p_modem_ctxt,
                                 atparser_context_t *p_atp_ctxt,
                                 uint32_t *p_ATcmdTimeout)
{
  at_status_t retval = ATSTATUS_OK;

  /* 1- set the commande name (get it from LUT) */
  const AT_CHAR_t *p_cmd_name_string = atcm_get_CmdStr(p_modem_ctxt, p_atp_ctxt->current_atcmd.id);
  uint8_t string_length = (uint8_t) strlen((const CRC_CHAR_t *) p_cmd_name_string);
  (void) memcpy((AT_CHAR_t *)p_atp_ctxt->current_atcmd.name,
                (const AT_CHAR_t *)p_cmd_name_string,
                string_length);

  PRINT_DBG("<modem custom> build the cmd %s (type=%d, length=%d)",
            p_atp_ctxt->current_atcmd.name,
            p_atp_ctxt->current_atcmd.type,
            string_length)

  /* 2- set the command parameters (only for write or execution commands or for data) */
  if ((p_atp_ctxt->current_atcmd.type == ATTYPE_WRITE_CMD) ||
      (p_atp_ctxt->current_atcmd.type == ATTYPE_EXECUTION_CMD) ||
      (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD))
  {
    retval = (atcm_get_CmdBuildFunc(p_modem_ctxt, p_atp_ctxt->current_atcmd.id))(p_atp_ctxt, p_modem_ctxt);
  }

  /* 3- set command timeout (has been set in command programmation) */
  *p_ATcmdTimeout = p_atp_ctxt->cmd_timeout;
  PRINT_DBG("==== CMD TIMEOUT = %ld ====", *p_ATcmdTimeout)

  /* increment step in SID treatment */
  p_atp_ctxt->step++;

  PRINT_DBG("atcm_modem_build_cmd returned status = %d", retval)
  return (retval);
}

/**
  * @brief  Prepare response buffer with SID infos. This rsp buffer is sent to Cellular Service.
  * @note   Only some SID return a response buffer.
  * @note   Called by AT-Core (via ATParser_get_rsp) when SID command returns OK.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @param  p_rsp_buf     pointer to response buffer
  * @retval at_status_t
  */
at_status_t atcm_modem_get_rsp(atcustom_modem_context_t *p_modem_ctxt,
                               const atparser_context_t *p_atp_ctxt,
                               at_buf_t *p_rsp_buf)
{
  at_status_t retval = ATSTATUS_OK;

  /* prepare response buffer for a SID
  *  all common behaviors for SID which are returning data in rsp_buf have to be implemented here
  */

  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_GET_ATTACHSTATUS:
      /* Prepare response buffer to send back to Cellular Service */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_ATTACHSTATUS,
                               (uint16_t) sizeof(CS_PSattach_t),
                               (void *)&p_modem_ctxt->SID_ctxt.attach_status) != DATAPACK_OK)
      {
        PRINT_ERR("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_RECEIVE_DATA:
      /* Prepare response buffer to send back to Cellular Service */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_SOCKET_RXDATA,
                               (uint16_t) sizeof(uint32_t),
                               (void *)&p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size) != DATAPACK_OK)
      {
        PRINT_ERR("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_RECEIVE_DATA_FROM:
    {
      /* Initialize the local data structure used to create response buffer */
      csint_socket_rxdata_from_t  rxdata_from;
      (void) memset((void *)&rxdata_from, 0, sizeof(csint_socket_rxdata_from_t));
      /* recopy info received */
      /* buffer size */
      rxdata_from.bytes_received = p_modem_ctxt->socket_ctxt.socketReceivedata.buffer_size;
      /* remote IP address type */
      rxdata_from.ip_addr_type = p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_type;
      /* remote IP address value */
      (void) memcpy((void *)&rxdata_from.ip_addr_value,
                    (void *)p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value,
                    strlen((CRC_CHAR_t *)p_modem_ctxt->socket_ctxt.socketReceivedata.ip_addr_value));
      /* remote port value */
      rxdata_from.remote_port = p_modem_ctxt->socket_ctxt.socketReceivedata.remote_port;
      /* Prepare response buffer to send back to Cellular Service */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_SOCKET_RXDATA_FROM,
                               (uint16_t) sizeof(csint_socket_rxdata_from_t),
                               (void *)&rxdata_from) != DATAPACK_OK)
      {
        PRINT_ERR("Receive data from problem")
        retval = ATSTATUS_ERROR;
      }
      break;
    }

    case SID_CS_REGISTER_NET:
    case SID_CS_GET_NETSTATUS:
      /* Add EPS, GPRS and CS registration states (from CREG, CGREG, CEREG commands) */
      p_modem_ctxt->SID_ctxt.read_operator_infos.EPS_NetworkRegState = p_modem_ctxt->persist.eps_network_state;
      p_modem_ctxt->SID_ctxt.read_operator_infos.GPRS_NetworkRegState = p_modem_ctxt->persist.gprs_network_state;
      p_modem_ctxt->SID_ctxt.read_operator_infos.CS_NetworkRegState = p_modem_ctxt->persist.cs_network_state;
      /* Prepare response buffer to send back to Cellular Service */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_REGISTRATIONSTATUS,
                               (uint16_t) sizeof(CS_RegistrationStatus_t),
                               (void *)&p_modem_ctxt->SID_ctxt.read_operator_infos) != DATAPACK_OK)
      {
        PRINT_ERR("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;

    case SID_CS_GET_IP_ADDRESS:
    {
      CS_PDN_conf_id_t current_conf_id = atcm_get_cid_current_SID(p_modem_ctxt);
      /* Initialize the local data structure used to create response buffer */
      csint_ip_addr_info_t ip_addr_info;
      (void) memset((void *)&ip_addr_info, 0, sizeof(csint_ip_addr_info_t));
      /* retrieve IP infos for request config_id */
      atcm_get_IP_address_infos(&p_modem_ctxt->persist, current_conf_id, &ip_addr_info);
      PRINT_DBG("retrieve IP address for cid %d = %s", current_conf_id, ip_addr_info.ip_addr_value)
      /* Prepare response buffer to send back to Cellular Service */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_GET_IP_ADDRESS,
                               (uint16_t) sizeof(csint_ip_addr_info_t),
                               (void *)&ip_addr_info) != DATAPACK_OK)
      {
        PRINT_ERR("Buffer size problem")
        retval = ATSTATUS_ERROR;
      }
      break;
    }

    case SID_CS_SIM_GENERIC_ACCESS:
    {
      /* Prepare response buffer to send back to Cellular Service */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_SIM_GENERIC_ACCESS,
                               (uint16_t) sizeof(csint_sim_generic_access_t),
                               (void *)&p_modem_ctxt->SID_ctxt.sim_generic_access) != DATAPACK_OK)
      {
        PRINT_ERR("SIM generic access problem")
        retval = ATSTATUS_ERROR;
      }
      break;
    }

#if defined(USE_COM_MDM)
    case SID_CS_COM_MDM_TRANSACTION:
    {
      /* recopy info received for cellular service in p_rsp_buf */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_COM_MDM,
                               (uint16_t) sizeof(csint_ComMdm_t),
                               (void *)&p_modem_ctxt->SID_ctxt.com_mdm_data) != DATAPACK_OK)
      {
        PRINT_ERR("SIM generic access problem")
        retval = ATSTATUS_ERROR;
      }
      break;
    }
#endif /* defined(USE_COM_MDM) */

    default:
      break;
  }

  return (retval);
}

/**
  * @brief  Prepare response buffer with URC infos. This rsp buffer is sent to Cellular Service.
  * @note   Called by AT-Core (via ATParser_get_urc) when URC are available.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @param  p_rsp_buf     pointer to response buffer
  * @retval ATSTATUS_OK if no error and no more pending URC
  *         ATSTATUS_OK_PENDING_URC if no error and still some pending URC
  *         ATSTATUS_ERROR if an error occurred
  */
at_status_t atcm_modem_get_urc(atcustom_modem_context_t *p_modem_ctxt,
                               const atparser_context_t *p_atp_ctxt,
                               at_buf_t *p_rsp_buf)
{
  UNUSED(p_atp_ctxt);
  at_status_t retval = ATSTATUS_OK;

  /* prepare response for an URC
  *  all common behaviors for URC have to be implemented here
  */

  /* URC for EPS network registration */
  if (p_modem_ctxt->persist.urc_avail_eps_network_registration == AT_TRUE)
  {
    PRINT_DBG("urc_avail_eps_network_registration")
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_EPS_NETWORK_REGISTRATION_STATUS,
                             (uint16_t) sizeof(CS_NetworkRegState_t),
                             (void *)&p_modem_ctxt->persist.eps_network_state) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_eps_network_registration = AT_FALSE;
  }
  /* URC for GPRS network registration */
  else if (p_modem_ctxt->persist.urc_avail_gprs_network_registration == AT_TRUE)
  {
    PRINT_DBG("urc_avail_gprs_network_registration")
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_GPRS_NETWORK_REGISTRATION_STATUS,
                             (uint16_t) sizeof(CS_NetworkRegState_t),
                             (void *)&p_modem_ctxt->persist.gprs_network_state) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_gprs_network_registration = AT_FALSE;
  }
  /* URC for CS network registration */
  else if (p_modem_ctxt->persist.urc_avail_cs_network_registration == AT_TRUE)
  {
    PRINT_DBG("urc_avail_cs_network_registration")
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_CS_NETWORK_REGISTRATION_STATUS,
                             (uint16_t) sizeof(CS_NetworkRegState_t),
                             (void *)&p_modem_ctxt->persist.cs_network_state) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_cs_network_registration = AT_FALSE;
  }
  /* URC for EPS location info */
  else if ((p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE)
           || (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE))
  {
    PRINT_DBG("urc_avail_eps_location_info_tac or urc_avail_eps_location_info_ci")

    /* Initialize the local data structure used to create response buffer */
    csint_location_info_t loc_struct = { .ci_updated = CS_FALSE, .lac_updated = CS_FALSE, };
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE)
    {
      loc_struct.lac = p_modem_ctxt->persist.eps_location_info.lac;
      loc_struct.lac_updated = CS_TRUE;
    }
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE)
    {
      loc_struct.ci = p_modem_ctxt->persist.eps_location_info.ci;
      loc_struct.ci_updated = CS_TRUE;
    }
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_EPS_LOCATION_INFO,
                             (uint16_t) sizeof(csint_location_info_t),
                             (void *)&loc_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flags (systematically to avoid never ending URC) */
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_eps_location_info_tac = AT_FALSE;
    }
    if (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_eps_location_info_ci = AT_FALSE;
    }
  }
  /* URC for GPRS location info */
  else if ((p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE)
           || (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE))
  {
    PRINT_DBG("urc_avail_gprs_location_info_tac or urc_avail_gprs_location_info_ci")

    /* Initialize the local data structure used to create response buffer */
    csint_location_info_t loc_struct = { .ci_updated = CS_FALSE, .lac_updated = CS_FALSE, };
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE)
    {
      loc_struct.lac = p_modem_ctxt->persist.gprs_location_info.lac;
      loc_struct.lac_updated = CS_TRUE;
    }
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE)
    {
      loc_struct.ci = p_modem_ctxt->persist.gprs_location_info.ci;
      loc_struct.ci_updated = CS_TRUE;
    }
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_GPRS_LOCATION_INFO,
                             (uint16_t) sizeof(csint_location_info_t),
                             (void *)&loc_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flags (systematically to avoid never ending URC) */
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_gprs_location_info_lac = AT_FALSE;
    }
    if (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_gprs_location_info_ci = AT_FALSE;
    }
  }
  /* URC for CS location info */
  else if ((p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE)
           || (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE))
  {
    PRINT_DBG("urc_avail_cs_location_info_lac or urc_avail_cs_location_info_ci")

    /* Initialize the local data structure used to create response buffer */
    csint_location_info_t loc_struct = { .ci_updated = CS_FALSE, .lac_updated = CS_FALSE, };
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE)
    {
      loc_struct.lac = p_modem_ctxt->persist.cs_location_info.lac;
      loc_struct.lac_updated = CS_TRUE;
    }
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE)
    {
      loc_struct.ci = p_modem_ctxt->persist.cs_location_info.ci;
      loc_struct.ci_updated = CS_TRUE;
    }
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_CS_LOCATION_INFO,
                             (uint16_t) sizeof(csint_location_info_t),
                             (void *)&loc_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flags  (systematically to avoid never ending URC) */
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_cs_location_info_lac = AT_FALSE;
    }
    if (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE)
    {
      p_modem_ctxt->persist.urc_avail_cs_location_info_ci = AT_FALSE;
    }
  }
  /* URC for Signal Quality */
  else if (p_modem_ctxt->persist.urc_avail_signal_quality == AT_TRUE)
  {
    PRINT_DBG("urc_avail_signal_quality")

    /* Initialize the local data structure used to create response buffer */
    CS_SignalQuality_t signal_quality_struct;
    signal_quality_struct.rssi = p_modem_ctxt->persist.signal_quality.rssi;
    signal_quality_struct.ber = p_modem_ctxt->persist.signal_quality.ber;
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_SIGNAL_QUALITY,
                             (uint16_t) sizeof(CS_SignalQuality_t),
                             (void *)&signal_quality_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag  (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_signal_quality = AT_FALSE;
  }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  /* URC for Pending Socket Data */
  else if (p_modem_ctxt->persist.urc_avail_socket_data_pending == AT_TRUE)
  {
    PRINT_DBG("urc_avail_socket_data_pending")

    /* Initialize the local data structure used to create response buffer */
    socket_handle_t sockHandle = atcm_socket_get_hdle_urc_data_pending(p_modem_ctxt);
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_SOCKET_DATA_PENDING,
                             (uint16_t) sizeof(socket_handle_t),
                             (void *)&sockHandle) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag if no more socket data pending */
    p_modem_ctxt->persist.urc_avail_socket_data_pending = atcm_socket_remaining_urc_data_pending(p_modem_ctxt);
  }
  /* URC for Socket Closed by Remote */
  else if (p_modem_ctxt->persist.urc_avail_socket_closed_by_remote == AT_TRUE)
  {
    PRINT_DBG("urc_avail_socket_closed_by_remote")

    /* Initialize the local data structure used to create response buffer */
    socket_handle_t sockHandle = atcm_socket_get_hdle_urc_closed_by_remote(p_modem_ctxt);
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_SOCKET_CLOSED,
                             (uint16_t) sizeof(socket_handle_t),
                             (void *)&sockHandle) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag if no more socket data pending */
    p_modem_ctxt->persist.urc_avail_socket_closed_by_remote = atcm_socket_remaining_urc_closed_by_remote(p_modem_ctxt);
  }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
  /* URC for PDN event */
  else if (p_modem_ctxt->persist.urc_avail_pdn_event == AT_TRUE)
  {
    PRINT_DBG("urc_avail_pdn_event")

    /*  We are only interested by following +CGEV URC :
    *   +CGEV: NW DETACH : the network has forced a Packet domain detach (all contexts)
    *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>] : the nw has forced a context deactivation
    *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>] : context deactivated
    */
    if (p_modem_ctxt->persist.pdn_event.event_origine == CGEV_EVENT_ORIGINE_NW)
    {
      switch (p_modem_ctxt->persist.pdn_event.event_type)
      {
        case CGEV_EVENT_TYPE_DETACH:
          /*  we are in the case:
          *  +CGEV: NW DETACH
          */
          /* Prepare response buffer to send back to Cellular Service */
          if (DATAPACK_writeStruct(p_rsp_buf,
                                   (uint16_t) CSMT_URC_PACKET_DOMAIN_EVENT,
                                   (uint16_t) sizeof(csint_PDN_event_desc_t),
                                   (void *)&p_modem_ctxt->persist.pdn_event) != DATAPACK_OK)
          {
            retval = ATSTATUS_ERROR;
          }
          break;

        case CGEV_EVENT_TYPE_DEACTIVATION:
          if (p_modem_ctxt->persist.pdn_event.event_scope == CGEV_EVENT_SCOPE_PDN)
          {
            /* we are in the case:
            *   +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
            */
            /* Prepare response buffer to send back to Cellular Service */
            if (DATAPACK_writeStruct(p_rsp_buf,
                                     (uint16_t) CSMT_URC_PACKET_DOMAIN_EVENT,
                                     (uint16_t) sizeof(csint_PDN_event_desc_t),
                                     (void *)&p_modem_ctxt->persist.pdn_event) != DATAPACK_OK)
            {
              retval = ATSTATUS_ERROR;
            }
          }
          else
          {
            /* we are in the case:
            *   +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
            */
            /* Prepare response buffer to send back to Cellular Service */
            if (DATAPACK_writeStruct(p_rsp_buf,
                                     (uint16_t) CSMT_URC_PACKET_DOMAIN_EVENT,
                                     (uint16_t) sizeof(csint_PDN_event_desc_t),
                                     (void *)&p_modem_ctxt->persist.pdn_event) != DATAPACK_OK)
            {
              retval = ATSTATUS_ERROR;
            }
          }
          break;

        default:
          PRINT_INFO("+CGEV URC discarded (NW), type=%d", p_modem_ctxt->persist.pdn_event.event_type)
          retval = ATSTATUS_ERROR;
          break;
      }
    }
    else
    {
      PRINT_INFO("+CGEV URC discarded (ME)")
      retval = ATSTATUS_ERROR;
    }

    /* reset flag and pdn event (systematically to avoid never ending URC) */
    reset_pdn_event(&p_modem_ctxt->persist);
    p_modem_ctxt->persist.urc_avail_pdn_event = AT_FALSE;
  }
  /* URC for PING response */
  else if (p_modem_ctxt->persist.urc_avail_ping_rsp == AT_TRUE)
  {
    PRINT_DBG("urc_avail_ping_rsp")
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_PING_RSP,
                             (uint16_t) sizeof(CS_Ping_response_t),
                             (void *)&p_modem_ctxt->persist.ping_resp_urc) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_ping_rsp = AT_FALSE;
  }
  /* URC for Sim refresh event */
  else if (p_modem_ctxt->persist.urc_avail_sim_refresh_event == AT_TRUE)
  {
    PRINT_DBG("urc_avail_sim_refresh_event")

    /* Initialize the local data structure used to create response buffer */
    CS_SimEvent_status_t sim_status_struct;
    (void) memcpy((void *)&sim_status_struct,
                  (void *)&p_modem_ctxt->persist.sim_refresh_infos,
                  sizeof(CS_SimEvent_status_t));
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_SIM_EVENT,
                             (uint16_t) sizeof(CS_SimEvent_status_t),
                             (void *)&sim_status_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_sim_refresh_event = AT_FALSE;
  }
  /* URC for Sim detect event */
  else if (p_modem_ctxt->persist.urc_avail_sim_detect_event == AT_TRUE)
  {
    PRINT_DBG("urc_avail_sim_detect_event")

    /* Initialize the local data structure used to create response buffer */
    CS_SimEvent_status_t sim_status_struct;
    (void) memcpy((void *)&sim_status_struct,
                  (void *)&p_modem_ctxt->persist.sim_detect_infos,
                  sizeof(CS_SimEvent_status_t));
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_SIM_EVENT,
                             (uint16_t) sizeof(CS_SimEvent_status_t),
                             (void *)&sim_status_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_sim_detect_event = AT_FALSE;
  }
  /* URC for Sim state event */
  else if (p_modem_ctxt->persist.urc_avail_sim_state_event == AT_TRUE)
  {
    PRINT_DBG("urc_avail_sim_state_event")

    /* Initialize the local data structure used to create response buffer */
    CS_SimEvent_status_t sim_status_struct;
    (void) memcpy((void *)&sim_status_struct,
                  (void *)&p_modem_ctxt->persist.sim_state_infos,
                  sizeof(CS_SimEvent_status_t));
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_SIM_EVENT,
                             (uint16_t) sizeof(CS_SimEvent_status_t),
                             (void *)&sim_status_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_sim_state_event = AT_FALSE;
  }
  /* URC for Low Power status event */
  else if (p_modem_ctxt->persist.urc_avail_lp_status == AT_TRUE)
  {
    PRINT_DBG("urc_avail_lp_status")

    /* Initialize the local data structure used to create response buffer */
    CS_LowPower_status_t lp_status_struct;
    lp_status_struct.nwk_periodic_TAU = p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU;
    lp_status_struct.nwk_active_time = p_modem_ctxt->persist.low_power_status.nwk_active_time;
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_LP_STATUS_EVENT,
                             (uint16_t) sizeof(CS_LowPower_status_t),
                             (void *)&lp_status_struct) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_lp_status = AT_FALSE;
  }
  /* URC for Modem event */
  else if (p_modem_ctxt->persist.urc_avail_modem_events != CS_MDMEVENT_NONE)
  {
    PRINT_DBG("urc_avail_modem_events")
    /* Prepare response buffer to send back to Cellular Service */
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_MODEM_EVENT,
                             (uint16_t) sizeof(CS_ModemEvent_t),
                             (void *)&p_modem_ctxt->persist.urc_avail_modem_events) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* reset flag (systematically to avoid never ending URC) */
    p_modem_ctxt->persist.urc_avail_modem_events = CS_MDMEVENT_NONE;
  }
#if defined(USE_COM_MDM)
  else if (p_modem_ctxt->persist.urc_avail_commdm_event_count != 0U)
  {
    PRINT_INFO("urc_avail_commdm_event")

    CS_comMdm_status_t comMdmd_event_infos;
    /* report number of pending COMMDM messages */
    comMdmd_event_infos.param1 = p_modem_ctxt->persist.urc_avail_commdm_event_count - 1U;
    if (DATAPACK_writeStruct(p_rsp_buf,
                             (uint16_t) CSMT_URC_COMMDM_EVENT,
                             (uint16_t) sizeof(comMdmd_event_infos),
                             (void *)&comMdmd_event_infos) != DATAPACK_OK)
    {
      retval = ATSTATUS_ERROR;
    }

    /* decrease counter of pending COMMDM messages stored */
    p_modem_ctxt->persist.urc_avail_commdm_event_count--;
  }
#endif /* defined(USE_COM_MDM) */
  else
  {
    PRINT_INFO("no pending URC")
    retval = ATSTATUS_ERROR;
  }

  /* Check if there are still some pending URC.
   * If still some pending URC, return the information to caller.
   */
  if ((p_modem_ctxt->persist.urc_avail_eps_network_registration == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_gprs_network_registration == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_gprs_location_info_lac == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_eps_location_info_tac == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_eps_location_info_ci == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_gprs_location_info_ci == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_cs_network_registration == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_cs_location_info_lac == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_cs_location_info_ci == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_signal_quality == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_socket_data_pending == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_socket_closed_by_remote == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_pdn_event == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_ping_rsp == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_lp_status == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_sim_refresh_event == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_sim_detect_event == AT_TRUE) ||
      (p_modem_ctxt->persist.urc_avail_sim_state_event == AT_TRUE) ||
#if defined(USE_COM_MDM)
      (p_modem_ctxt->persist.urc_avail_commdm_event_count != 0U) ||
#endif /* defined(USE_COM_MDM) */

      (p_modem_ctxt->persist.urc_avail_modem_events != CS_MDMEVENT_NONE))
  {
    /* there is at least one pending URC */
    retval = ATSTATUS_OK_PENDING_URC;
  }

  return (retval);
}

/**
  * @brief  Prepare response buffer with error infos. This rsp buffer is sent to Cellular Service.
  * @note   Called by AT-Core (via ATParser_get_error) when a command returns an error.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @param  p_rsp_buf     pointer to response buffer
  * @retval at_status_t
  */
at_status_t atcm_modem_get_error(atcustom_modem_context_t *p_modem_ctxt,
                                 const atparser_context_t *p_atp_ctxt,
                                 at_buf_t *p_rsp_buf)
{
  UNUSED(p_atp_ctxt);
  at_status_t retval = ATSTATUS_OK;

  /* prepare error report to send back to Cellular Service */
  if (DATAPACK_writeStruct(p_rsp_buf,
                           (uint16_t) CSMT_ERROR_REPORT,
                           (uint16_t) sizeof(csint_error_report_t),
                           (void *)&p_modem_ctxt->SID_ctxt.error_report) != DATAPACK_OK)
  {
    PRINT_ERR("Buffer size problem")
    retval = ATSTATUS_ERROR;
  }
  return (retval);
}

/**
  * @brief  atcm_subscribe_net_event
  * @note   Subscribe to a network event.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @retval at_status_t
  */
at_status_t atcm_subscribe_net_event(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt)
{
  /* Retrieve urc event request: CEREG, CREG or CGREG ?
  *  note: only one event at same time
  */
  CS_UrcEvent_t urcEvent = p_modem_ctxt->SID_ctxt.urcEvent;

  /* is an event linked to CEREG ? */
  if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO))
  {
    /* if CEREG not yet subscbribe */
    if ((p_modem_ctxt->persist.urc_subscript_eps_networkReg == CS_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_eps_locationInfo == CS_FALSE))
    {
      /* request all URC, we will filter them */
      if (p_modem_ctxt->persist.psm_urc_requested == AT_TRUE)
      {
        p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
      }
      else
      {
        p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_NETWK_REG_LOC_URC;
      }
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }

    /* set event as subscribed */
    if (urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_eps_networkReg = CS_TRUE;
    }
    if (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_eps_locationInfo = CS_TRUE;
    }
  }
  /* is an event linked to CGREG ?  */
  else if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO))
  {
    /* if CGREG not yet subscbribe */
    if ((p_modem_ctxt->persist.urc_subscript_gprs_networkReg == CS_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_gprs_locationInfo == CS_FALSE))
    {
      /* request all URC, we will filter them */
      if (p_modem_ctxt->persist.psm_urc_requested == AT_TRUE)
      {
        p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
      }
      else
      {
        p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_NETWK_REG_LOC_URC;
      }
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }

    /* set event as subscribed */
    if (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_gprs_networkReg = CS_TRUE;
    }
    if (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_gprs_locationInfo = CS_TRUE;
    }
  }
  /* is an event linked to CREG ? */
  else if ((urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
  {
    /* if CREG not yet subscbribe */
    if ((p_modem_ctxt->persist.urc_subscript_cs_networkReg == CS_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_cs_locationInfo == CS_FALSE))
    {
      /* request all URC, we will filter them */
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_NETWK_REG_LOC_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }

    /* set event as subscribed */
    if (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_cs_networkReg = CS_TRUE;
    }
    if (urcEvent == CS_URCEVENT_CS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_cs_locationInfo = CS_TRUE;
    }
  }
  else
  {
    /* nothing to do */
  }

  return (ATSTATUS_OK);
}

/**
  * @brief  atcm_unsubscribe_net_event
  * @note   Unsubscribe to a network event previously subscribed.
  * @param  p_modem_ctxt  pointer to modem context
  * @param  p_atp_ctxt    pointer to parser context
  * @retval at_status_t
  */
at_status_t atcm_unsubscribe_net_event(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt)
{
  /* Retrieve urc event request: CEREG, CREG or CGREG ?
  *  note: only one event at same time
  */
  CS_UrcEvent_t urcEvent = p_modem_ctxt->SID_ctxt.urcEvent;

  /* is an event linked to CEREG ? */
  if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO))
  {
    /* set event as unsubscribed */
    if (urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_eps_networkReg = CS_FALSE;
    }
    if (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_eps_locationInfo = CS_FALSE;
    }

    /* if no more event for CEREG, send cmd to modem to disable URC */
    if ((p_modem_ctxt->persist.urc_subscript_eps_networkReg == CS_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_eps_locationInfo == CS_FALSE))
    {
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  /* is an event linked to CGREG ? */
  else if ((urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO))
  {
    /* set event as unsubscribed */
    if (urcEvent == CS_URCEVENT_GPRS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_gprs_networkReg = CS_FALSE;
    }
    if (urcEvent == CS_URCEVENT_GPRS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_gprs_locationInfo = CS_FALSE;
    }

    /* if no more event for CGREG, send cmd to modem to disable URC */
    if ((p_modem_ctxt->persist.urc_subscript_gprs_networkReg == CS_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_gprs_locationInfo == CS_FALSE))
    {
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  /* is an event linked to CREG ? */
  else if ((urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
  {
    /* set event as unsubscribed */
    if (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT)
    {
      p_modem_ctxt->persist.urc_subscript_cs_networkReg = CS_FALSE;
    }
    if (urcEvent == CS_URCEVENT_CS_LOCATION_INFO)
    {
      p_modem_ctxt->persist.urc_subscript_cs_locationInfo = CS_FALSE;
    }

    /* if no more event for CREG, send cmd to modem to disable URC */
    if ((p_modem_ctxt->persist.urc_subscript_cs_networkReg == CS_FALSE) &&
        (p_modem_ctxt->persist.urc_subscript_cs_locationInfo == CS_FALSE))
    {
      p_modem_ctxt->CMD_ctxt.cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
      atcm_program_AT_CMD(p_modem_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CREG, FINAL_CMD);
    }
    else
    {
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
  }
  else
  {
    /* nothing to do */
  }

  return (ATSTATUS_OK);
}

/**
  * @brief  Validate the PING request is valid: recopy SID ping param to persistent context and
  *         initialize ping data structure for the response.
  * @note   If modem answer to ping response
  *         - asynchronous: call this function when receiving confirmation that
  *                         the ping request is valid (usually, after receiving OK).
  *         - synchronous: call this function when requesting ping to te modem.
  * @param  p_modem_ctxt Pointer to the modem context
  * @retval none
  */
void atcm_validate_ping_request(atcustom_modem_context_t *p_modem_ctxt)
{
  /* PING request is valid */
  /* reset and copy SID ping parameters to persistent context */
  (void) memset((void *)&p_modem_ctxt->persist.ping_infos, 0, sizeof(csint_ping_params_t));
  (void) memcpy((void *)&p_modem_ctxt->persist.ping_infos, (void *)&p_modem_ctxt->SID_ctxt.ping_infos,
                sizeof(csint_ping_params_t));
  /* reset other parameters */
  (void) memset((void *)&p_modem_ctxt->persist.ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  /* initialize ping index to invalid value */
  p_modem_ctxt->persist.ping_resp_urc.index = PING_INVALID_INDEX;
  /* initialize ping URC to "not available" */
  p_modem_ctxt->persist.urc_avail_ping_rsp = AT_FALSE;
}

/**
  * @brief  atcm_modem_event_received
  * @param  p_modem_ctxt  pointer to modem context
  * @param  mdm_evt Modem event.
  * @retval at_bool_t
  */
at_bool_t atcm_modem_event_received(atcustom_modem_context_t *p_modem_ctxt, CS_ModemEvent_t mdm_evt)
{
  at_bool_t event_subscribed = AT_FALSE;

  /* if the event received is subscribed, save it */
  if ((p_modem_ctxt->persist.modem_events_subscript & mdm_evt) != 0U) /* bitmask check */
  {
    p_modem_ctxt->persist.urc_avail_modem_events |= mdm_evt;
    event_subscribed = AT_TRUE;
  }

  /* returns true only if event has been subscribed */
  return (event_subscribed);
}

/**
  * @brief  atcm_reset_persistent_context
  * @note   Reset the persistent context.
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @retval none
  */
void atcm_reset_persistent_context(atcustom_persistent_context_t *p_persistent_ctxt)
{
  PRINT_API("enter reset_persistent_context()")

  /* URC subscriptions */
  p_persistent_ctxt->urc_subscript_eps_networkReg = CS_FALSE;
  p_persistent_ctxt->urc_subscript_eps_locationInfo = CS_FALSE;
  p_persistent_ctxt->urc_subscript_gprs_networkReg = CS_FALSE;
  p_persistent_ctxt->urc_subscript_gprs_locationInfo = CS_FALSE;
  p_persistent_ctxt->urc_subscript_cs_networkReg = CS_FALSE;
  p_persistent_ctxt->urc_subscript_cs_locationInfo = CS_FALSE;
  p_persistent_ctxt->urc_subscript_signalQuality = CS_FALSE;
  p_persistent_ctxt->urc_subscript_pdn_event = CS_FALSE;

  /* URC availabilities */
  p_persistent_ctxt->urc_avail_eps_network_registration = AT_FALSE;
  p_persistent_ctxt->urc_avail_eps_location_info_tac = AT_FALSE;
  p_persistent_ctxt->urc_avail_eps_location_info_ci = AT_FALSE;
  p_persistent_ctxt->urc_avail_gprs_network_registration = AT_FALSE;
  p_persistent_ctxt->urc_avail_gprs_location_info_lac = AT_FALSE;
  p_persistent_ctxt->urc_avail_gprs_location_info_ci = AT_FALSE;
  p_persistent_ctxt->urc_avail_cs_network_registration = AT_FALSE;
  p_persistent_ctxt->urc_avail_cs_location_info_lac = AT_FALSE;
  p_persistent_ctxt->urc_avail_cs_location_info_ci = AT_FALSE;
  p_persistent_ctxt->urc_avail_signal_quality = AT_FALSE;
  p_persistent_ctxt->urc_avail_socket_data_pending = AT_FALSE;
  p_persistent_ctxt->urc_avail_socket_closed_by_remote = AT_FALSE;
  p_persistent_ctxt->urc_avail_pdn_event = AT_FALSE;
  p_persistent_ctxt->urc_avail_sim_refresh_event = AT_FALSE;
  p_persistent_ctxt->urc_avail_sim_detect_event = AT_FALSE;
  p_persistent_ctxt->urc_avail_sim_state_event = AT_FALSE;
#if defined(USE_COM_MDM)
  p_persistent_ctxt->urc_avail_commdm_event_count = 0U;
#endif /* defined(USE_COM_MDM) */

  /* Modem events subscriptions */
  p_persistent_ctxt->modem_events_subscript = CS_MDMEVENT_NONE;

  /* Modem events availabilities */
  p_persistent_ctxt->urc_avail_modem_events = CS_MDMEVENT_NONE;

  /* reset PDN event parameters */
  reset_pdn_event(p_persistent_ctxt);

  /* initialize allocation table of modem cid */
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[i];
    p_tmp->mdm_cid_value = i; /* modem cid value (0 is a reserved value) */
    p_tmp->pdn_defined = AT_FALSE;
    p_tmp->affected_config = atcm_convert_index_to_PDN_conf(i);
    p_tmp->ip_addr_infos.ip_addr_type = CS_IPAT_INVALID;
    (void) memset((void *)&p_tmp->ip_addr_infos.ip_addr_value, 0, MAX_IP_ADDR_SIZE);
  }

  /* initialize PDP context parameters table */
  for (uint8_t i = 0U; i < MODEM_MAX_NB_PDP_CTXT; i++)
  {
    csint_pdn_infos_t *p_tmp;
    p_tmp = &p_persistent_ctxt->pdp_ctxt_infos[i];
    p_tmp->conf_id = CS_PDN_NOT_DEFINED; /* not used */
    p_tmp->apn_present = CS_TRUE;
    (void) memset((void *)&p_tmp->apn, 0, MAX_APN_SIZE);
    (void) memset((void *)&p_tmp->pdn_conf, 0, sizeof(CS_PDN_configuration_t));
    /* set default PDP type = IPV4 */
    /* code commented to avoid Code Sonar error (due to previous memset 0)
     *    p_tmp->pdn_conf.pdp_type = CS_PDPTYPE_IP
     */
  }

  /* set default PDP parameters (ie for PDN_PREDEF_CONFIG) from configuration file if defined */
  csint_pdn_infos_t *p_predef;
  p_predef = &p_persistent_ctxt->pdp_ctxt_infos[CS_PDN_PREDEF_CONFIG];
  p_persistent_ctxt->pdn_default_conf_id = CS_PDN_PREDEF_CONFIG;
  p_predef->conf_id = CS_PDN_PREDEF_CONFIG;
  p_predef->apn_present = CS_TRUE;

  /* set default PDN parameters */
  reserve_user_modem_cid(p_persistent_ctxt, CS_PDN_PREDEF_CONFIG, 1U);
  p_predef->pdn_conf.pdp_type = CS_PDPTYPE_IP;
  (void) memcpy((AT_CHAR_t *)&p_predef->apn, (AT_CHAR_t *)"", sizeof(""));

  /* socket */
  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_persistent_ctxt->socket[i];
    /* Important note about socket_connId_value parameter:
     *  This is the connection Id shared between the Host and the Modem for a specific socket connection.
     *  Two different behaviors exist depending of modems:
     *   > case #1: the Host is responsible to allocate a Socket ID. The constraint is to use Socket ID values
     *              that are in a range supported by the modem.
     *   > case #2: the Socket ID is allocated by the Modem during socket creation process.
     */
    p_tmp->socket_connId_value = ((uint8_t)i + 1U); /* socket ID range from 1 to 6,
                                                     * if a modem does not support this range => need to adapt */
    p_tmp->socket_connected = AT_FALSE;
    p_tmp->socket_data_pending_urc = AT_FALSE;
    p_tmp->socket_data_available = AT_FALSE;
    p_tmp->socket_closed_pending_urc = AT_FALSE;
  }

  /* Power Saving Mode info */
  p_persistent_ctxt->psm_urc_requested = AT_FALSE;       /* PSM default value */

  /* other */
  p_persistent_ctxt->modem_at_ready = AT_FALSE;     /* modem ready to receive AT commands */
  p_persistent_ctxt->modem_sim_ready = AT_FALSE;    /* modem sim ready */
  p_persistent_ctxt->sim_pin_code_ready = AT_FALSE; /* modem pin code status */
  p_persistent_ctxt->cmee_level = CMEE_VERBOSE;
  p_persistent_ctxt->sim_state = CS_SIMSTATE_UNKNOWN;
  p_persistent_ctxt->sim_selected = CS_MODEM_SIM_SOCKET_0; /* default SIM slot selected */
  p_persistent_ctxt->flowCtrl_RTS = 0xFF;       /* not initialized (from AT+IFC) */
  p_persistent_ctxt->flowCtrl_CTS = 0xFF;       /* not initialized (from AT+IFC) */

  /* ping infos */
  (void) memset((void *)&p_persistent_ctxt->ping_infos, 0, sizeof(csint_ping_params_t));
  (void) memset((void *)&p_persistent_ctxt->ping_resp_urc, 0, sizeof(CS_Ping_response_t));
  p_persistent_ctxt->ping_resp_urc.index = PING_INVALID_INDEX;
  p_persistent_ctxt->urc_avail_ping_rsp = AT_FALSE;

  /* low power status */
  p_persistent_ctxt->low_power_status.nwk_periodic_TAU = 0U; /* T3412, default value (0U means value not available) */
  p_persistent_ctxt->low_power_status.nwk_active_time = 0U;  /* T3324, default value (0U means value not available) */
  p_persistent_ctxt->urc_avail_lp_status = AT_FALSE;

  /* SIM infos - initialized for each event, parameters are set to 'not significant' (-1) */
  p_persistent_ctxt->sim_refresh_infos.event = CS_SIMEVENT_SIM_REFRESH;
  p_persistent_ctxt->sim_refresh_infos.param1 = CS_SIMINFOS_UNKNOWN;
  p_persistent_ctxt->sim_detect_infos.event = CS_SIMEVENT_SIM_DETECT;
  p_persistent_ctxt->sim_detect_infos.param1 = CS_SIMINFOS_UNKNOWN;
  p_persistent_ctxt->sim_state_infos.event = CS_SIMEVENT_SIM_STATE;
  p_persistent_ctxt->sim_state_infos.param1 = CS_SIMINFOS_UNKNOWN;
}

/**
  * @brief  atcm_reset_SID_context
  * @note   Reset the current Service ID (SID) context.
  * @param  p_sid_ctxt Pointer to Service ID (SID) context.
  * @retval none
  */
void atcm_reset_SID_context(atcustom_SID_context_t *p_sid_ctxt)
{
  PRINT_API("enter reset_SID_context()")

  p_sid_ctxt->attach_status = CS_PS_DETACHED;
  p_sid_ctxt->cfun_status = CS_CMI_MINI;

  (void) memset((void *)&p_sid_ctxt->write_operator_infos, 0, sizeof(CS_OperatorSelector_t));
  p_sid_ctxt->write_operator_infos.format = CS_ONF_NOT_PRESENT;
  /* code commented to avoid Code Sonar error (variable already initialized with same value)
   *   p_sid_ctxt->write_operator_infos.AcT_present = CS_FALSE
   */

  (void) memset((void *)&p_sid_ctxt->read_operator_infos, 0, sizeof(CS_RegistrationStatus_t));
  /* code commented to avoid Code Sonar error (variable already initialized with same value)
  *    read_operator_infos.EPS_NetworkRegState = CS_NRS_NOT_REGISTERED_NOT_SEARCHING
  *    read_operator_infos.GPRS_NetworkRegState = CS_NRS_NOT_REGISTERED_NOT_SEARCHING
  *    read_operator_infos.CS_NetworkRegState = CS_NRS_NOT_REGISTERED_NOT_SEARCHING
  *    read_operator_infos.optional_fields_presence = CS_RSF_NONE
  */
  p_sid_ctxt->modem_init.init = CS_CMI_MINI;
  p_sid_ctxt->modem_init.reset = CS_FALSE;
  (void) memset((void *)&p_sid_ctxt->modem_init.pincode.pincode, 0, sizeof(csint_pinCode_t));

  p_sid_ctxt->p_device_info = NULL;
  p_sid_ctxt->p_signal_quality = NULL;
  p_sid_ctxt->p_dns_request_infos = NULL;
  p_sid_ctxt->p_direct_cmd_tx = NULL;
  p_sid_ctxt->sim_generic_access.data = NULL;
  p_sid_ctxt->sim_generic_access.bytes_received = 0U;

  (void) memset((void *)&p_sid_ctxt->init_power_config, 0, sizeof(CS_init_power_config_t));
  (void) memset((void *)&p_sid_ctxt->set_power_config, 0, sizeof(CS_set_power_config_t));

  p_sid_ctxt->wakeup_origin = UNKNOWN_WAKEUP;

  p_sid_ctxt->urcEvent = CS_URCEVENT_NONE;
  p_sid_ctxt->pdn_conf_id = CS_PDN_CONFIG_DEFAULT;

  (void) memset((void *)&p_sid_ctxt->socketSendData_struct, 0, sizeof(csint_socket_data_buffer_t));
  (void) memset((void *)&p_sid_ctxt->ping_infos, 0, sizeof(csint_ping_params_t));

  p_sid_ctxt->error_report.error_type = CSERR_UNKNOWN;
  p_sid_ctxt->error_report.sim_state = CS_SIMSTATE_UNKNOWN;
}

/**
  * @brief  atcm_reset_CMD_context
  * @note   Reset the current AT command context.
  * @param  p_cmd_ctxt Pointer to AT command context.
  * @retval none
  */
void atcm_reset_CMD_context(atcustom_CMD_context_t *p_cmd_ctxt)
{
  PRINT_API("enter reset_CMD_context()")

  /* p_cmd_ctxt->cgsn_write_cmd_param parameter used in fCmdBuild_CGSN() for AT+GCSN cmd */
  p_cmd_ctxt->cgatt_write_cmd_param = CGATT_UNKNOWN;
  p_cmd_ctxt->cxreg_write_cmd_param = CXREG_DISABLE_NETWK_REG_URC;
  /* p_cmd_ctxt->command_echo parameter  used in fCmdBuild_ATE() for ATE cmd */
  /* p_cmd_ctxt->dce_full_resp_format parameter used in fCmdBuild_ATV() for ATV cmd*/
  p_cmd_ctxt->pdn_state = PDN_STATE_ACTIVATE;
  p_cmd_ctxt->modem_cid = 0xFFFFU;
  p_cmd_ctxt->baud_rate = MODEM_UART_BAUDRATE;
  p_cmd_ctxt->cfun_value = 1U;
#if (CONFIG_MODEM_UART_RTS_CTS == 1)
  p_cmd_ctxt->flow_control_cts_rts = AT_TRUE;
#else
  p_cmd_ctxt->flow_control_cts_rts = AT_FALSE;
#endif /* CONFIG_MODEM_UART_RTS_CTS */
}

/**
  * @brief  atcm_reset_SOCKET_context
  * @note   Reset the current Socket context.
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval none
  */
void atcm_reset_SOCKET_context(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter atcm_reset_SOCKET_context()")

  p_modem_ctxt->socket_ctxt.p_socket_info = NULL;
  (void) memset((void *)&p_modem_ctxt->socket_ctxt.socketReceivedata, 0, sizeof(csint_socket_data_buffer_t));
  p_modem_ctxt->socket_ctxt.socket_current_connId = 0U;
  p_modem_ctxt->socket_ctxt.socket_rx_expected_buf_size = 0U;
  p_modem_ctxt->socket_ctxt.socket_rx_count_bytes_received = 0U;

  p_modem_ctxt->socket_ctxt.socket_send_state = SocketSendState_No_Activity;
  p_modem_ctxt->socket_ctxt.socket_receive_state = SocketRcvState_No_Activity;
  p_modem_ctxt->socket_ctxt.socket_RxData_state = SocketRxDataState_not_started;
}

/**
  * @brief  atcm_searchCmdInLUT
  * @note   Search if the received AT command exists in the modem LookUp table (LUT)
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  p_msg_in Buffer which contains the received AT command.
  * @param  element_infos Pointer to buffer with information about current element.
  * @retval at_status_t
  */
at_status_t atcm_searchCmdInLUT(atcustom_modem_context_t *p_modem_ctxt,
                                const atparser_context_t  *p_atp_ctxt,
                                const IPC_RxMessage_t *p_msg_in,
                                at_element_info_t *element_infos)
{
  UNUSED(p_atp_ctxt);
  at_status_t retval = ATSTATUS_ERROR;

  element_infos->cmd_id_received = CMD_AT_INVALID;

  /* check if we receive empty command */
  if (element_infos->str_size == 0U)
  {
    /* empty answer */
    element_infos->cmd_id_received = (CMD_ID_t) CMD_AT;
    /* null size string */
    retval = ATSTATUS_OK;
  }
  else
  {
    /* search in LUT the ID corresponding to command received */
    bool leave_loop = false;
    uint16_t i = 0U;
    do
    {
      /* if string length > 0 */
      if (strlen((const CRC_CHAR_t *)(p_modem_ctxt->p_modem_LUT)[i].cmd_str) > 0U)
      {
        /* compare strings size first */
        if ((strlen((const CRC_CHAR_t *)(p_modem_ctxt->p_modem_LUT)[i].cmd_str) == element_infos->str_size))
        {
          /* compare strings content */
          if (0 == memcmp((const AT_CHAR_t *) & (p_msg_in->buffer[element_infos->str_start_idx]),
                          (const AT_CHAR_t *)(p_modem_ctxt->p_modem_LUT)[i].cmd_str,
                          (size_t) element_infos->str_size))
          {
            PRINT_DBG("we received LUT#%ld : %s \r\n", (p_modem_ctxt->p_modem_LUT)[i].cmd_id,
                      (p_modem_ctxt->p_modem_LUT)[i].cmd_str)

            element_infos->cmd_id_received = (p_modem_ctxt->p_modem_LUT)[i].cmd_id;
            retval = ATSTATUS_OK;
            leave_loop = true;
          }
        }
      }
      i++;
    } while ((leave_loop == false) && (i < p_modem_ctxt->modem_LUT_size));
  }
  return (retval);
}

/**
  * @brief  atcm_check_text_line_cmd
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  p_atp_ctxt Pointer to parser context.
  * @param  p_msg_in Buffer which contains the received AT command.
  * @param  element_infos Pointer to buffer with information about current element.
  * @retval at_action_rsp_t
  */
at_action_rsp_t atcm_check_text_line_cmd(atcustom_modem_context_t *p_modem_ctxt,
                                         at_context_t *p_at_ctxt,
                                         const IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  const atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval = ATACTION_RSP_ERROR;

  /* in this section, we treat all commands which can return text lines */
  switch (p_atp_ctxt->current_atcmd.id)
  {
    case CMD_AT_CGMI:
      if (fRspAnalyze_CGMI(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_CGMM:
      if (fRspAnalyze_CGMM(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_CGMR:
      if (fRspAnalyze_CGMR(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_CGSN:
      if (fRspAnalyze_CGSN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_GSN:
      if (fRspAnalyze_GSN(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_IPR:
      if (fRspAnalyze_IPR(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_IFC:
      if (fRspAnalyze_IFC(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_CIMI:
      if (fRspAnalyze_CIMI(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    case CMD_AT_CGPADDR:
      if (fRspAnalyze_CGPADDR(p_at_ctxt, p_modem_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
      {
        /* received a valid intermediate answer */
        retval = ATACTION_RSP_INTERMEDIATE;
      }
      break;

    default:
      /* this is not one of modem common command, need to check if this is an answer to a modem's specific cmd */
      retval = ATACTION_RSP_NO_ACTION;
      break;
  }

  return (retval);
}

/**
  * @brief  atcm_searchCmdInLUT
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  p_atp_ctxt Pointer to parser context.
  * @retval at_status_t
  */
at_status_t atcm_retrieve_SID_parameters(atcustom_modem_context_t *p_modem_ctxt, atparser_context_t *p_atp_ctxt)
{
  at_status_t retval = ATSTATUS_OK;

  /* only retrieve SID parameters on first call (step = 0)*/
  if (p_atp_ctxt->step == 0U)
  {
    switch (p_atp_ctxt->current_SID)
    {
      case SID_CS_MODEM_CONFIG:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_MODEMCONFIG,
                                (uint16_t) sizeof(CS_ModemConfig_t),
                                (void *)&p_modem_ctxt->SID_ctxt.modem_config) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_INIT_MODEM:
        /* retrieve  client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_INITMODEM,
                                (uint16_t) sizeof(csint_modemInit_t),
                                (void *)&p_modem_ctxt->SID_ctxt.modem_init) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_GET_DEVICE_INFO:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_DEVICE_INFO,
                             (void **)&p_modem_ctxt->SID_ctxt.p_device_info) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_GET_SIGNAL_QUALITY:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_SIGNAL_QUALITY,
                             (void **)&p_modem_ctxt->SID_ctxt.p_signal_quality) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_REGISTER_NET:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_OPERATORSELECT,
                                (uint16_t) sizeof(CS_OperatorSelector_t),
                                (void *)&p_modem_ctxt->SID_ctxt.write_operator_infos) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SUSBCRIBE_NET_EVENT:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_URC_EVENT,
                                (uint16_t) sizeof(CS_UrcEvent_t),
                                (void *)&p_modem_ctxt->SID_ctxt.urcEvent) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_UNSUSBCRIBE_NET_EVENT:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_URC_EVENT,
                                (uint16_t) sizeof(CS_UrcEvent_t),
                                (void *)&p_modem_ctxt->SID_ctxt.urcEvent) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DIAL_COMMAND:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_SOCKET_INFO,
                             (void **)&p_modem_ctxt->socket_ctxt.p_socket_info) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SEND_DATA:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_SOCKET_DATA_BUFFER,
                                (uint16_t) sizeof(csint_socket_data_buffer_t),
                                &p_modem_ctxt->SID_ctxt.socketSendData_struct) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_RECEIVE_DATA:
      case SID_CS_RECEIVE_DATA_FROM:
        /* retrieve pointer on client structure */
        if (DATAPACK_readStruct(p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_SOCKET_DATA_BUFFER,
                                (uint16_t) sizeof(csint_socket_data_buffer_t),
                                (void *)&p_modem_ctxt->socket_ctxt.socketReceivedata) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SOCKET_CLOSE:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_SOCKET_INFO,
                             (void **)&p_modem_ctxt->socket_ctxt.p_socket_info) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_RESET:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_RESET,
                                (uint16_t) sizeof(CS_Reset_t),
                                (void *)&p_modem_ctxt->SID_ctxt.reset_type) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_ACTIVATE_PDN:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_ACTIVATE_PDN,
                                (uint16_t) sizeof(CS_PDN_conf_id_t),
                                (void *)&p_modem_ctxt->SID_ctxt.pdn_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DEACTIVATE_PDN:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_DEACTIVATE_PDN,
                                (uint16_t) sizeof(CS_PDN_conf_id_t),
                                (void *)&p_modem_ctxt->SID_ctxt.pdn_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DEFINE_PDN:
      {
        csint_pdn_infos_t *ptr_pdn_infos;
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_DEFINE_PDN,
                             (void **)&ptr_pdn_infos) == DATAPACK_OK)
        {
          /* recopy pdn configuration infos to persistent context */
          (void) memcpy((void *)&p_modem_ctxt->persist.pdp_ctxt_infos[ptr_pdn_infos->conf_id],
                        (void *)ptr_pdn_infos,
                        sizeof(csint_pdn_infos_t));
          /* set SID ctxt pdn conf id */
          p_modem_ctxt->SID_ctxt.pdn_conf_id = ptr_pdn_infos->conf_id;
          /* affect a modem cid to this configuration */
          affect_modem_cid(&p_modem_ctxt->persist, ptr_pdn_infos->conf_id);
        }
        else
        {
          retval = ATSTATUS_ERROR;
        }
        break;
      }

      case SID_CS_SET_DEFAULT_PDN:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_SET_DEFAULT_PDN,
                                (uint16_t) sizeof(CS_PDN_conf_id_t),
                                (void *)&p_modem_ctxt->persist.pdn_default_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_GET_IP_ADDRESS:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_GET_IP_ADDRESS,
                                (uint16_t) sizeof(CS_PDN_conf_id_t),
                                (void *)&p_modem_ctxt->SID_ctxt.pdn_conf_id) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DNS_REQ:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_DNS_REQ,
                             (void **)&p_modem_ctxt->SID_ctxt.p_dns_request_infos) == DATAPACK_OK)
        {
          /* set SID ctxt pdn conf id */
          p_modem_ctxt->SID_ctxt.pdn_conf_id = p_modem_ctxt->SID_ctxt.p_dns_request_infos->conf_id;
        }
        else
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SOCKET_CNX_STATUS:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_SOCKET_CNX_STATUS,
                             (void **)&p_modem_ctxt->socket_ctxt.p_socket_cnx_infos) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SUSBCRIBE_MODEM_EVENT:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_MODEM_EVENT,
                                (uint16_t) sizeof(CS_ModemEvent_t),
                                (void *)&p_modem_ctxt->persist.modem_events_subscript) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_PING_IP_ADDRESS:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_PING_ADDRESS,
                                (uint16_t) sizeof(csint_ping_params_t),
                                (void *)&p_modem_ctxt->SID_ctxt.ping_infos) == DATAPACK_OK)
        {
          /* set SID ctxt pdn conf id */
          p_modem_ctxt->SID_ctxt.pdn_conf_id = p_modem_ctxt->SID_ctxt.ping_infos.conf_id;
        }
        else
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_DIRECT_CMD:
        /* retrieve pointer on client structure */
        if (DATAPACK_readPtr(p_atp_ctxt->p_cmd_input,
                             (uint16_t) CSMT_DIRECT_CMD,
                             (void **)&p_modem_ctxt->SID_ctxt.p_direct_cmd_tx) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SIM_SELECT:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_SIM_SELECT,
                                (uint16_t) sizeof(CS_SimSlot_t),
                                (void *)&p_modem_ctxt->persist.sim_selected) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SIM_GENERIC_ACCESS:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_SIM_GENERIC_ACCESS,
                                (uint16_t) sizeof(csint_sim_generic_access_t),
                                &p_modem_ctxt->SID_ctxt.sim_generic_access) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_CHECK_CNX:
      case SID_CS_POWER_ON:
      case SID_CS_POWER_OFF:
      case SID_CS_GET_NETSTATUS:
      case SID_CS_GET_ATTACHSTATUS:
      case SID_ATTACH_PS_DOMAIN:
      case SID_DETACH_PS_DOMAIN:
      case SID_CS_DATA_SUSPEND:
      case SID_CS_DATA_RESUME:
        /* no client data to retrieve */
        PRINT_DBG("No data to unpack for SID %d", p_atp_ctxt->current_SID)
        break;

      case SID_CS_REGISTER_PDN_EVENT:
      case SID_CS_DEREGISTER_PDN_EVENT:
        /* no client data to retrieve */
        PRINT_DBG("No data to unpack for SID %d", p_atp_ctxt->current_SID)
        break;

      case SID_CS_INIT_POWER_CONFIG:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_INIT_POWER_CONFIG,
                                (uint16_t) sizeof(CS_init_power_config_t),
                                (void *)&p_modem_ctxt->SID_ctxt.init_power_config) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

      case SID_CS_SET_POWER_CONFIG:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_SET_POWER_CONFIG,
                                (uint16_t) sizeof(CS_set_power_config_t),
                                (void *)&p_modem_ctxt->SID_ctxt.set_power_config) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;


      case SID_CS_SLEEP_REQUEST:
      case SID_CS_SLEEP_COMPLETE:
      case SID_CS_SLEEP_CANCEL:
        /* no client data to retrieve */
        PRINT_DBG("No data to unpack for SID %d", p_atp_ctxt->current_SID)
        break;

      case SID_CS_WAKEUP:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_WAKEUP_ORIGIN,
                                (uint16_t) sizeof(CS_wakeup_origin_t),
                                (void *)&p_modem_ctxt->SID_ctxt.wakeup_origin) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;

#if defined(USE_COM_MDM)
      case SID_CS_COM_MDM_TRANSACTION:
        /* retrieve client data */
        if (DATAPACK_readStruct((uint8_t *)p_atp_ctxt->p_cmd_input,
                                (uint16_t) CSMT_COM_MDM,
                                (uint16_t) sizeof(csint_ComMdm_t),
                                (void *)&p_modem_ctxt->SID_ctxt.com_mdm_data) != DATAPACK_OK)
        {
          retval = ATSTATUS_ERROR;
        }
        break;
#endif /* defined(USE_COM_MDM) */

      /*********************
      LIST OF SID NOT IMPLEMENTED YET:
        SID_CS_AUTOACTIVATE_PDN,
        SID_CS_CONNECT,
        SID_CS_DISCONNECT,
        SID_CS_SOCKET_RECEIVE_DATA,
        SID_CS_RESET,

        SID_CS_DIAL_ONLINE, - NOT SUPPORTED
        SID_CS_SOCKET_CREATE, - not needed, config is done at CS level
        SID_CS_SOCKET_SET_OPTION, - not needed, config is done at CS level
        SID_CS_SOCKET_GET_OPTION, - not needed, config is done at CS level
        ***************/
      default:
        PRINT_ERR("Missing treatment for SID %d", p_atp_ctxt->current_SID)
        retval = ATSTATUS_ERROR;
        break;
    }

  }

  return (retval);
}

/**
  * @brief  Get modem cid affected to requested PDP config
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @param  conf_id
  * @retval uint8_t affected modem CID
  */
uint8_t atcm_get_affected_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id)
{
  uint8_t retval = 1U; /* return first valid cid by default (do not return 0 which is a special cid value !!!) */
  CS_PDN_conf_id_t current_conf_id = conf_id;

  /* if default PDN is required, retrieve corresponding conf_id */
  if (conf_id == CS_PDN_CONFIG_DEFAULT)
  {
    /* overwrite current_conf_id */
    current_conf_id = p_persistent_ctxt->pdn_default_conf_id;
    PRINT_DBG("Default PDP context selected (conf_id = %d)", current_conf_id)
  }

  bool leave_loop = false;
  uint8_t i = 0U;
  /* parse modem cid table to find matching value */
  do
  {
    const atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[i];
    if (p_tmp->affected_config == current_conf_id)
    {
      /* return affected modem cid */
      retval = p_tmp->mdm_cid_value;
      leave_loop = true;
    }
    i++;
  } while ((leave_loop == false) && (i < MODEM_MAX_NB_PDP_CTXT));

  return (retval);
}

/**
  * @brief  Get user Config ID corresponding to this modem cid
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @param  modem_cid
  * @retval CS_PDN_conf_id_t
  */
CS_PDN_conf_id_t atcm_get_configID_for_modem_cid(const atcustom_persistent_context_t *p_persistent_ctxt,
                                                 uint8_t modem_cid)
{
  CS_PDN_conf_id_t retval = CS_PDN_NOT_DEFINED;
  bool leave_loop = false;
  uint8_t i = 0U;
  /* parse modem cid table to find matching value */
  do
  {
    const atcustom_modem_cid_table_t *p_tmp;
    p_tmp = &p_persistent_ctxt->modem_cid_table[i];
    if (p_tmp->mdm_cid_value == modem_cid)
    {
      /* return corresponding Used Config ID */
      retval = p_tmp->affected_config;
      leave_loop = true;
    }
    i++;
  } while ((leave_loop == false) && (i < MODEM_MAX_NB_PDP_CTXT));

  return (retval);
}

/**
  * @brief  atcm_convert_index_to_PDN_conf
  * @param  index
  * @retval CS_PDN_conf_id_t
  */
CS_PDN_conf_id_t atcm_convert_index_to_PDN_conf(uint8_t index)
{
  CS_PDN_conf_id_t PDNconf;
  /* convert provided index to a PDN configuration (enum) */
  switch (index)
  {
    case 0:
      PDNconf = CS_PDN_PREDEF_CONFIG;
      break;
    case 1:
      PDNconf = CS_PDN_USER_CONFIG_1;
      break;
    case 2:
      PDNconf = CS_PDN_USER_CONFIG_2;
      break;
    case 3:
      PDNconf = CS_PDN_USER_CONFIG_3;
      break;
    case 4:
      PDNconf = CS_PDN_USER_CONFIG_4;
      break;
    case 5:
      PDNconf = CS_PDN_USER_CONFIG_5;
      break;
    default:
      PDNconf = CS_PDN_NOT_DEFINED;
      break;
  }
  return (PDNconf);
}

/**
  * @brief  reset_pdn_event
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @retval none
  */
void reset_pdn_event(atcustom_persistent_context_t *p_persistent_ctxt)
{
  p_persistent_ctxt->pdn_event.event_origine = CGEV_EVENT_UNDEFINE;
  p_persistent_ctxt->pdn_event.event_scope   = CGEV_EVENT_SCOPE_GLOBAL;
  p_persistent_ctxt->pdn_event.event_type    = CGEV_EVENT_UNDEFINE;
  p_persistent_ctxt->pdn_event.conf_id       = CS_PDN_NOT_DEFINED;
}

/**
  * @brief  atcm_report_urc_sim_event is used to report a modem SIM event
  * @note   User of this function has to fill CS_SimEvent_status_t structure
  * @param  p_modem_ctxt Pointer to modem context.
  * @param  sim_event
  * @retval none
  */
void atcm_report_urc_sim_event(atcustom_modem_context_t *p_modem_ctxt, CS_SimEvent_status_t *sim_event)
{
  /* limitation: if an event of same type has not been reported yet, it will be overwritten
  *             by last event
  */
  if (sim_event != NULL)
  {
    /* recopy sim event info to the good structure */
    if (sim_event->event == CS_SIMEVENT_SIM_REFRESH)
    {
      /* SIM REFRESH event */
      (void) memcpy((void *)&p_modem_ctxt->persist.sim_refresh_infos,
                    (void *)sim_event,
                    sizeof(CS_SimEvent_status_t));
      p_modem_ctxt->persist.urc_avail_sim_refresh_event = AT_TRUE;
    }
    else if (sim_event->event == CS_SIMEVENT_SIM_DETECT)
    {
      /* SIM DETECT event */
      (void) memcpy((void *)&p_modem_ctxt->persist.sim_detect_infos,
                    (void *)sim_event,
                    sizeof(CS_SimEvent_status_t));
      p_modem_ctxt->persist.urc_avail_sim_detect_event = AT_TRUE;
    }
    else if (sim_event->event == CS_SIMEVENT_SIM_STATE)
    {
      /* SIM STATE event */
      (void) memcpy((void *)&p_modem_ctxt->persist.sim_state_infos,
                    (void *)sim_event,
                    sizeof(CS_SimEvent_status_t));
      p_modem_ctxt->persist.urc_avail_sim_state_event = AT_TRUE;
    }
    else
    {
      /* undefined SIM event */
      PRINT_ERR("SIM event not supported")
    }
  }
}

/**
  * @brief  atcm_set_error_report is used to report a modem ERROR.
  * @param  err_type
  * @param  p_modem_ctxt Pointer to modem context.
  * @retval none
  */
void atcm_set_error_report(csint_error_type_t err_type, atcustom_modem_context_t *p_modem_ctxt)
{
  p_modem_ctxt->SID_ctxt.error_report.error_type = err_type;

  switch (err_type)
  {
    case CSERR_SIM:
      p_modem_ctxt->SID_ctxt.error_report.sim_state = p_modem_ctxt->persist.sim_state;
      break;

    default:
      /* nothing to do*/
      break;
  }
}
/**
  * @}
  */

/** @defgroup AT_CORE_COMMON_Private_Functions_Prototypes AT_CORE COMMON Private Functions Prototypes
  * @{
  */

/**
  * @brief  Reserve a modem modem cid (normally used only for PDN_PREDEF_CONFIG at startup time)
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @param  conf_id
  * @param  reserved_user_modem_cid
  * @retval none
  */
static void reserve_user_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id,
                                   uint8_t reserved_user_modem_cid)
{
  /* User PDP CID and Modem CID are matching */
  UNUSED(conf_id);

  CS_PDN_conf_id_t user_pdn_conf = atcm_convert_index_to_PDN_conf(reserved_user_modem_cid);
  /* valid only for CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5 */
  if ((user_pdn_conf == CS_PDN_USER_CONFIG_1) ||
      (user_pdn_conf == CS_PDN_USER_CONFIG_2) ||
      (user_pdn_conf == CS_PDN_USER_CONFIG_3) ||
      (user_pdn_conf == CS_PDN_USER_CONFIG_4) ||
      (user_pdn_conf == CS_PDN_USER_CONFIG_5))
  {
    bool leave_loop = false;
    uint8_t i = 0U;
    /* parse modem cid table to find matching value */
    do
    {
      atcustom_modem_cid_table_t *p_tmp;
      p_tmp = &p_persistent_ctxt->modem_cid_table[i];
      if (reserved_user_modem_cid == p_tmp->mdm_cid_value)
      {
        /* reserve this modem cid */
        p_tmp->pdn_defined = AT_TRUE;
        leave_loop = true;
      }
      i++;
    } while ((leave_loop == false) && (i < MODEM_MAX_NB_PDP_CTXT));
  }
  else
  {
    PRINT_ERR("Trying to affect a non-valid modem CID")
  }

  return;
}

/**
  * @brief  Affect a modem cid to the specified user PDP config
  * @param  p_persistent_ctxt Pointer to persistent context.
  * @param  conf_id
  * @retval none
  */
static void affect_modem_cid(atcustom_persistent_context_t *p_persistent_ctxt, CS_PDN_conf_id_t conf_id)
{
  /* User PDP CID and Modem CID are matching */

  /* only for CS_PDN_USER_CONFIG_1 to CS_PDN_USER_CONFIG_5 */
  if ((conf_id == CS_PDN_USER_CONFIG_1) ||
      (conf_id == CS_PDN_USER_CONFIG_2) ||
      (conf_id == CS_PDN_USER_CONFIG_3) ||
      (conf_id == CS_PDN_USER_CONFIG_4) ||
      (conf_id == CS_PDN_USER_CONFIG_5))
  {
    /* reserve this modem cid */
    bool leave_loop = false;
    uint8_t i = 0U;
    /* parse modem cid table to find matching value */
    do
    {
      atcustom_modem_cid_table_t *p_tmp;
      p_tmp = &p_persistent_ctxt->modem_cid_table[i];
      if (conf_id == p_tmp->affected_config)
      {
        /* reserve this modem cid */
        p_tmp->pdn_defined = AT_TRUE;
        leave_loop = true;
      }
      i++;
    } while ((leave_loop == false) && (i < MODEM_MAX_NB_PDP_CTXT));
  }
  else
  {
    PRINT_ERR("Trying to affect a non-valid modem CID")
  }

  return;
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
