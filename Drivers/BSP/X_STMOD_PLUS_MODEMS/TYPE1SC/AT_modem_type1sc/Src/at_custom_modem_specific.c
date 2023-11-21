/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
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
#include "at_custom_modem_specific.h"
#include "at_custom_modem_sid.h"
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
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC AT_CUSTOM ALTAIR_T1SC SPECIFIC
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Private_Macros AT_CUSTOM ALTAIR_T1SC SPECIFIC Private Macros
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

/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Private_Defines AT_CUSTOM ALTAIR_T1SC SPECIFIC Private Defines
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

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Private_Variables AT_CUSTOM ALTAIR_T1SC SPECIFIC Private Variables
  * @{
  */
/* TYPE1SC Modem device context */
static atcustom_modem_context_t TYPE1SC_ctxt;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Exported_Variables AT_CUSTOM ALTAIR_T1SC SPECIFIC Exported Variables
  * @{
  */
/* shared variables specific to Type1SC */
type1sc_shared_variables_t type1sc_shared =
{
  .sim_status_retries = 0U,
  .SocketCmd_Allocated_SocketID = AT_FALSE,
  .SocketCmd_Activated = AT_FALSE,
  .setcfg_function = SETGETCFG_UNDEFINED,
  .getcfg_function = SETGETCFG_UNDEFINED,
  .syscfg_function = SETGETSYSCFG_UNDEFINED,
  .modem_waiting_for_bootev = false,
  .modem_bootev_received = false,
  .notifyev_mode = 0U,
  .modem_sim_same_as_selected = true,
#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
  .host_lp_state  = HOST_LP_STATE_IDLE,
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */
};
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SPECIFIC_Exported_Functions AT_CUSTOM ALTAIR_T1SC SPECIFIC Exported Functions
  * @{
  */

/**
  * @brief  Initialize specific AT commands.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @retval none
  */
void ATCustom_TYPE1SC_init(atparser_context_t *p_atp_ctxt)
{
  /* Commands Look-up table */
  static const atcustom_LUT_t ATCMD_TYPE1SC_LUT[] =
  {
    /* cmd enum - cmd string - cmd timeout (in ms) - build cmd ftion - analyze cmd ftion */
    {CMD_AT,             "",             TYPE1SC_AT_TIMEOUT,       fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_OK,          "OK",           TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CONNECT,     "CONNECT",      TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_RING,        "RING",         TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_CARRIER,  "NO CARRIER",   TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_ERROR,       "ERROR",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_TYPE1SC},
    {CMD_AT_NO_DIALTONE, "NO DIALTONE",  TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_BUSY,        "BUSY",         TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_ANSWER,   "NO ANSWER",    TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CME_ERROR,   "+CME ERROR",   TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_TYPE1SC},
    {CMD_AT_CMS_ERROR,   "+CMS ERROR",   TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CmsErr},

    /* GENERIC MODEM commands */
    {CMD_AT_CGMI,        "+CGMI",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMI},
    {CMD_AT_CGMM,        "+CGMM",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMM},
    {CMD_AT_CGMR,        "+CGMR",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMR},
    {CMD_AT_CGSN,        "+CGSN",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CGSN,       fRspAnalyze_CGSN},
    {CMD_AT_GSN,         "+GSN",         TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_GSN},
    {CMD_AT_CIMI,        "+CIMI",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CIMI},
    {CMD_AT_CEER,        "%CEER",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEER},
    {CMD_AT_CMEE,        "+CMEE",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CMEE,       fRspAnalyze_None},
    {CMD_AT_CPIN,        "+CPIN",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CPIN,       fRspAnalyze_CPIN_TYPE1SC},
    {CMD_AT_CFUN,        "+CFUN",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CFUN,       fRspAnalyze_CFUN_TYPE1SC},
    {CMD_AT_COPS,        "+COPS",        TYPE1SC_COPS_TIMEOUT,     fCmdBuild_COPS,       fRspAnalyze_COPS},
    {CMD_AT_CNUM,        "+CNUM",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CNUM},
    {CMD_AT_CGATT,       "+CGATT",       TYPE1SC_CGATT_TIMEOUT,    fCmdBuild_CGATT,      fRspAnalyze_CGATT},
    {CMD_AT_CGPADDR,     "+CGPADDR",     TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CGPADDR,    fRspAnalyze_CGPADDR},
    {CMD_AT_CEREG,       "+CEREG",       TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CEREG,      fRspAnalyze_CEREG},
    {CMD_AT_CREG,        "+CREG",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CREG,       fRspAnalyze_CREG},
    {CMD_AT_CGREG,       "+CGREG",       TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CGREG,      fRspAnalyze_CGREG},
    {CMD_AT_CSQ,         "+CSQ",         TYPE1SC_CSQ_TIMEOUT,      fCmdBuild_NoParams,   fRspAnalyze_CSQ},
    {CMD_AT_CGDCONT,     "+CGDCONT",     TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CGDCONT,    fRspAnalyze_None},
    {CMD_AT_CGACT,       "+CGACT",       TYPE1SC_CGACT_TIMEOUT,    fCmdBuild_CGACT,      fRspAnalyze_None},
    {CMD_AT_CGDATA,      "+CGDATA",      TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CGDATA,     fRspAnalyze_None},
    {CMD_AT_CGEREP,      "+CGEREP",      TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CGEREP,     fRspAnalyze_None},
    {CMD_AT_CGEV,        "+CGEV",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGEV},
    {CMD_ATD,            "D",            TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_ATD_TYPE1SC,   fRspAnalyze_None},
    {CMD_ATE,            "E",            TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_ATE,        fRspAnalyze_None},
    {CMD_ATH,            "H",            TYPE1SC_ATH_TIMEOUT,      fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATO,            "O",            TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATV,            "V",            TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_ATV,        fRspAnalyze_None},
    {CMD_ATX,            "X",            TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_ATX,        fRspAnalyze_None},
    {CMD_ATZ,            "Z",            TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_ESC_CMD,     "+++",          TYPE1SC_ESCAPE_TIMEOUT,   fCmdBuild_ESCAPE_CMD, fRspAnalyze_None},
    {CMD_AT_IPR,         "+IPR",         TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_IPR,        fRspAnalyze_IPR},
    {CMD_AT_IFC,         "+IFC",         TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_IFC,        fRspAnalyze_IFC},
    {CMD_AT_AND_W,       "&W",           TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_AND_D,       "&D",           TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_AT_AND_D,   fRspAnalyze_None},
    {CMD_AT_DIRECT_CMD,  "",             TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_DIRECT_CMD, fRspAnalyze_DIRECT_CMD},
    /* note: for CMD_AT_DIRECT_CMD, the default timeout value will be replaced by the timeout
     *       value given by the upper layer.
     */
    {CMD_AT_AND_K3,      "&K3",          TYPE1SC_ATK_TIMEOUT,     fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_AND_K0,      "&K0",          TYPE1SC_ATK_TIMEOUT,     fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CPSMS,       "+CPSMS",       TYPE1SC_CPSMS_TIMEOUT,    fCmdBuild_CPSMS,      fRspAnalyze_CPSMS},
    {CMD_AT_CEDRXS,      "+CEDRXS",      TYPE1SC_CEDRX_TIMEOUT,    fCmdBuild_CEDRXS,     fRspAnalyze_CEDRXS},
    {CMD_AT_CEDRXP,      "+CEDRXP",      TYPE1SC_CEDRX_TIMEOUT,    fCmdBuild_NoParams,   fRspAnalyze_CEDRXP},
    {CMD_AT_CEDRXRDP,    "+CEDRXRDP",    TYPE1SC_CEDRX_TIMEOUT,    fCmdBuild_NoParams,   fRspAnalyze_CEDRXRDP},
    {CMD_AT_CSIM,        "+CSIM",        TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_CSIM,       fRspAnalyze_CSIM},

    /* MODEM SPECIFIC COMMANDS */
    {CMD_AT_PDNSET,    "%PDNSET",    TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_PDNSET_TYPE1SC, fRspAnalyze_None},
    {CMD_AT_CCID,      "%CCID",      TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_NoParams,       fRspAnalyze_CCID_TYPE1SC},
#if (MURATA_CMD_SUBSET == 0)
    {CMD_AT_SETCFG,    "%SETCFG",    TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_SETCFG_TYPE1SC, fRspAnalyze_None},
    {CMD_AT_GETCFG,    "%GETCFG",    TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_GETCFG_TYPE1SC, fRspAnalyze_GETCFG_TYPE1SC},
#endif /* MURATA_CMD_SUBSET == 0 */
    {CMD_AT_SETACFG,   "%SETACFG",   TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_SETCFG_TYPE1SC, fRspAnalyze_None},
    {CMD_AT_GETACFG,   "%GETACFG",   TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_GETCFG_TYPE1SC, fRspAnalyze_GETCFG_TYPE1SC},
    {CMD_AT_SETSYSCFG, "%SETSYSCFG", TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_SETSYSCFG_TYPE1SC, fRspAnalyze_None},
    {
      CMD_AT_GETSYSCFG, "%GETSYSCFG", TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_GETSYSCFG_TYPE1SC,
      fRspAnalyze_GETSYSCFG_TYPE1SC
    },
    {CMD_AT_NOTIFYEV, "%NOTIFYEV", TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_NOTIFYEV_TYPE1SC, fRspAnalyze_NOTIFYEV_TYPE1SC},
    {CMD_AT_SETBDELAY, "%SETBDELAY", TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_SETBDELAY_TYPE1SC,  fRspAnalyze_None},
    {CMD_AT_PDNRDP,     "%PDNRDP",   TYPE1SC_DEFAULT_TIMEOUT, fCmdBuild_PDNRDP_TYPE1SC,  fRspAnalyze_PDNRDP_TYPE1SC},

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* MODEM SPECIFIC COMMANDS USED FOR SOCKET MODE */
    {CMD_AT_PDNACT,                 "%PDNACT",      TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_PDNACT, fRspAnalyze_PDNACT},
    {
      CMD_AT_SOCKETCMD_ALLOCATE,     "%SOCKETCMD",   TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETCMD_ALLOCATE, fRspAnalyze_SOCKETCMD
    },
    {
      CMD_AT_SOCKETCMD_ACTIVATE,     "%SOCKETCMD",   TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETCMD_ACTIVATE,   fRspAnalyze_SOCKETCMD
    },
    {
      CMD_AT_SOCKETCMD_INFO,         "%SOCKETCMD",   TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETCMD_INFO,       fRspAnalyze_SOCKETCMD
    },
    {
      CMD_AT_SOCKETCMD_DEACTIVATE,   "%SOCKETCMD",   TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETCMD_DEACTIVATE, fRspAnalyze_SOCKETCMD
    },
    {
      CMD_AT_SOCKETCMD_DELETE,       "%SOCKETCMD",   TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETCMD_DELETE,     fRspAnalyze_SOCKETCMD
    },
    {
      CMD_AT_SOCKETDATA_SEND,        "%SOCKETDATA",  TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETDATA_SEND,      fRspAnalyze_SOCKETDATA
    },
    {
      CMD_AT_SOCKETDATA_RECEIVE,     "%SOCKETDATA",  TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_SOCKETDATA_RECEIVE,   fRspAnalyze_SOCKETDATA
    },
    {
      CMD_AT_DNSRSLV,                "%DNSRSLV",     TYPE1SC_DNSRSLV_TIMEOUT,
      fCmdBuild_DNSRSLV,              fRspAnalyze_DNSRSLV
    },
    {
      CMD_AT_PINGCMD,                "%PINGCMD",     TYPE1SC_PING_TIMEOUT,
      fCmdBuild_PINGCMD,              fRspAnalyze_PINGCMD
    },
    {CMD_AT_SOCKETEV,   "%SOCKETEV",  TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,    fRspAnalyze_SOCKETEV},
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

    /* MODEM SPECIFIC EVENTS */
    {CMD_AT_BOOTEV,     "%BOOTEV",    TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,    fRspAnalyze_None},

  };
#define SIZE_ATCMD_TYPE1SC_LUT ((uint16_t) (sizeof (ATCMD_TYPE1SC_LUT) / sizeof (atcustom_LUT_t)))

  /* common init */
  ATC_TYPE1SC_modem_init(&TYPE1SC_ctxt);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  TYPE1SC_ctxt.modem_LUT_size = SIZE_ATCMD_TYPE1SC_LUT;
  TYPE1SC_ctxt.p_modem_LUT = (const atcustom_LUT_t *)ATCMD_TYPE1SC_LUT;

  /* override default termination string for AT command: <CR> */
  (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->endstr, "\r");

  /* ###########################  END CUSTOMIZATION PART  ########################### */
}

/**
  * @brief  Check if message received is complete.
  * @param  rxChar Character received from modem.
  * @retval uint8_t Returns 1 if message is complete, else returns 0.
  */
uint8_t ATCustom_TYPE1SC_checkEndOfMsgCallback(uint8_t rxChar)
{
  uint8_t last_char = 0U;

  /*---------------------------------------------------------------------------------------*/
  if (TYPE1SC_ctxt.state_SyntaxAutomaton == WAITING_FOR_INIT_CR)
  {
    /* waiting for first valid <CR>, char received before are considered as trash */
    if ((AT_CHAR_t)('\r') == rxChar)
    {
      /* current     : xxxxx<CR>
      *  command format : <CR><LF>xxxxxxxx<CR><LF>
      *  waiting for : <LF>
      */
      TYPE1SC_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (TYPE1SC_ctxt.state_SyntaxAutomaton == WAITING_FOR_CR)
  {
    if ((AT_CHAR_t)('\r') == rxChar)
    {
      /* current     : xxxxx<CR>
      *  command format : <CR><LF>xxxxxxxx<CR><LF>
      *  waiting for : <LF>
      */
      TYPE1SC_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (TYPE1SC_ctxt.state_SyntaxAutomaton == WAITING_FOR_LF)
  {
    /* waiting for <LF> */
    if ((AT_CHAR_t)('\n') == rxChar)
    {
      /*  current        : <CR><LF>
      *   command format : <CR><LF>xxxxxxxx<CR><LF>
      *   waiting for    : x or <CR>
      */
      TYPE1SC_ctxt.state_SyntaxAutomaton = WAITING_FOR_FIRST_CHAR;
      last_char = 1U;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (TYPE1SC_ctxt.state_SyntaxAutomaton == WAITING_FOR_FIRST_CHAR)
  {
    /* NOTE about Socket mode:
    * No need to use/manage socket_RxData_state as it is done for other modems.
    * Indeed as data are received in HEX format, no risk to detect <CR> or <LF>
    * in the received data.
    */

    /* waiting for <CR> or x */
    if ((AT_CHAR_t)('\r') == rxChar)
    {
      /*   current        : <CR>
      *   command format : <CR><LF>xxxxxxxx<CR><LF>
      *   waiting for    : <LF>
      */
      TYPE1SC_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
    else {/* nothing to do */ }
  }
  /*---------------------------------------------------------------------------------------*/
  else
  {
    /* should not happen */
    __NOP();
  }

  /* ###########################  START CUSTOMIZATION PART  ######################### */
  /* if modem does not use standard syntax or has some specificities, replace previous
  *  function by a custom function
  *  for example, apply special treatment if (last_char = 0U)
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (last_char);
}

/**
  * @brief  Returns the next AT command to send in the current context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for this command.
  * @retval at_status_t
  */
at_status_t ATCustom_TYPE1SC_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout)
{
  at_status_t retval;
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);

  PRINT_API("enter ATCustom_TYPE1SC_getCmd() for SID %d", p_atp_ctxt->current_SID)

  /* retrieve parameters from SID command (will update SID_ctxt) */
  retval = atcm_retrieve_SID_parameters(&TYPE1SC_ctxt, p_atp_ctxt);

  if (retval == ATSTATUS_OK)
  {
    /* new command: reset command context */
    atcm_reset_CMD_context(&TYPE1SC_ctxt.CMD_ctxt);

    /* For each SID, the sequence of AT commands to send id defined (it can be dynamic)
    * Determine and prepare the next command to send for this SID
    */
    retval = at_custom_SID_T1SC(&TYPE1SC_ctxt, p_at_ctxt,  p_ATcmdTimeout);

    /* if no error, build the command to send */
    if (retval == ATSTATUS_OK)
    {
      retval = atcm_modem_build_cmd(&TYPE1SC_ctxt, p_atp_ctxt, p_ATcmdTimeout);
    }
  }

  return (retval);
}

/**
  * @brief  Extract next element of AT command actually analyzed.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_msg_in Pointer to buffer containing the received AT command.
  * @param  element_infos Pointer to structure used to parse AT command buffer.
  * @retval at_endmsg_t Returns ATENDMSG_YES if end of AT command detected, ATENDMSG_NO else.
  */
at_endmsg_t ATCustom_TYPE1SC_extractElement(atparser_context_t *p_atp_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos)
{
  UNUSED(p_atp_ctxt);
  at_endmsg_t retval_msg_end_detected = ATENDMSG_NO;
  bool exit_loop;
  static bool first_colon_found = false;
  static bool inside_quotes = false;
  uint16_t *p_parseIndex = &(element_infos->current_parse_idx);

  PRINT_API("enter ATCustom_TYPE1SC_extractElement()")
  PRINT_DBG("input message: size=%d ", p_msg_in->size)

  /* if this is beginning of message, check that message header is correct and jump over it */
  if (*p_parseIndex == 0U)
  {
    first_colon_found = false;
    /* ###########################  START CUSTOMIZATION PART  ########################### */
    /* MODEM RESPONSE SYNTAX:
      * <CR><LF><response><CR><LF>
      *
      */
    /* search initial <CR><LF> sequence (for robustness) */
    if ((p_msg_in->buffer[0] == (AT_CHAR_t)('\r')) && (p_msg_in->buffer[1] == (AT_CHAR_t)('\n')))
    {
      /* <CR><LF> sequence has been found, it is a command line */
      PRINT_DBG("cmd init sequence <CR><LF> found - break")
      *p_parseIndex = 2U;
    }
    /* ###########################  END CUSTOMIZATION PART  ########################### */
  }

  element_infos->str_start_idx = *p_parseIndex;
  element_infos->str_end_idx = *p_parseIndex;
  element_infos->str_size = 0U;

  /* reach limit of input buffer ? (empty message received) */
  if (*p_parseIndex >= p_msg_in->size)
  {
    retval_msg_end_detected = ATENDMSG_YES;
  }
  else
  {
    /* extract parameter from message */
    exit_loop = false;
    do
    {
      bool ignore_quote = false;

      switch (p_msg_in->buffer[*p_parseIndex])
      {
        /* ###########################  START CUSTOMIZATION PART  ########################### */
        /* ----- test separators -----
        *  AT responses and URC format : +CMD: vvv,www,,xxx,"yyy",zzz
        *  only first ':' is considered as a separator (':' can be part of a field for IPv6 address for example)
        *  if a field is inside quotes (like ,"yyy", above), comma separtor should not be analyzed.
        *  String inside a string is also considered : \"
        */
        case 0x3A: /* : = colon */
          /* only first colon character found is considered as a separator. */
          if (!first_colon_found)
          {
            first_colon_found = true;
            exit_loop = true;
          }
          break;

        case 0x2C: /* , = comma */
          /* usual fields separator.
          * but ignore comma inside a string.
          */
          if (!inside_quotes)
          {
            exit_loop = true;
          }
          break;

        case 0x22: /* " = double quote*/
        {
          /* check if this is a string inside a string */
          if (*p_parseIndex > 0U)
          {
            /* do we have an anti-slash before the quote ? */
            if (p_msg_in->buffer[(*p_parseIndex) - 1U] == 0x5CU) /* \ = anti-slash */
            {
              ignore_quote = true;
            }
          }
          /* is it s a valid quote ? */
          if (!ignore_quote)
          {
            inside_quotes = !inside_quotes;
          }
          break;
        }

        /* ==========================
         *  Separators: special cases
         * ==========================
         */
        case '=':
          /* special separator case for AT+IFC?
          *  The read form of AT+IFC returns AT+IFC=x,x instead of AT+IFC:x,x
          *  Consider "=" as a separator only when this command is currently ongoing.
          */
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t)CMD_AT_IFC)
          {
            exit_loop = true;
          }
          break;

        /* ----- test end of message ----- */
        case '\r':
          exit_loop = true;
          retval_msg_end_detected = ATENDMSG_YES;
          break;

        default:
          /* nothing special */
          __NOP();
          break;
          /* ###########################  END CUSTOMIZATION PART  ########################### */
      }

      if (!exit_loop)
      {
        /* increment end position (except if exit has been detected) */
        element_infos->str_end_idx = *p_parseIndex;
        element_infos->str_size++;
      }


      /* increment index */
      (*p_parseIndex)++;

      /* reach limit of input buffer ? */
      if (*p_parseIndex >= p_msg_in->size)
      {
        exit_loop = true;
        retval_msg_end_detected = ATENDMSG_YES;
      }
    } while (exit_loop == false);

    /* increase parameter rank */
    element_infos->param_rank = (element_infos->param_rank + 1U);
  }

  return (retval_msg_end_detected);
}

/**
  * @brief  Determine which AT command has been received and take actions if no further analyze needed.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_msg_in Pointer to buffer containing the received AT command.
  * @param  element_infos Pointer to structure used to parse AT command buffer.
  * @retval at_action_rsp_t Returns action to apply for this command.
  */
at_action_rsp_t ATCustom_TYPE1SC_analyzeCmd(at_context_t *p_at_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval;

  PRINT_API("enter ATCustom_TYPE1SC_analyzeCmd()")

  /* Analyze data received from the modem and
    * search in LUT the ID corresponding to command received
    */
  if (ATSTATUS_OK != atcm_searchCmdInLUT(&TYPE1SC_ctxt, p_atp_ctxt, p_msg_in, element_infos))
  {
    /* No command corresponding to a LUT entry has been found.
      * May be we received a text line without command prefix.
      *
      * This is the case for some commands which are using following syntax:
      *    AT+MYCOMMAND
      *    parameters
      *    OK
      *
      * ( usually, command are using the syntax:
      *    AT+MYCOMMAND
      *    +MYCOMMAND=parameters
      *    OK
      *  )
      */

    /* 1st STEP: search in common modems commands
      * (CGMI, CGMM, CGMR, CGSN, GSN, IPR, CIMI, CGPADDR, ...)
      */
    retval = atcm_check_text_line_cmd(&TYPE1SC_ctxt, p_at_ctxt, p_msg_in, element_infos);

    /* 2nd STEP: search in specific modems commands if not found at 1st STEP */
    if (retval == ATACTION_RSP_NO_ACTION)
    {
      switch (p_atp_ctxt->current_atcmd.id)
      {
        /* ###########################  START CUSTOMIZED PART  ########################### */
        case CMD_AT_GETACFG:
          retval = fRspAnalyze_GETCFG_TYPE1SC(p_at_ctxt, &TYPE1SC_ctxt, p_msg_in, element_infos);
          break;

        /* ###########################  END CUSTOMIZED PART  ########################### */
        default:
          /* this is not one of modem common command, need to check if this is an answer to a modem's specific cmd */
          PRINT_DBG("receive an un-expected line... is it a text line ?")
          retval = ATACTION_RSP_IGNORED;
          break;
      }
    }

    /* we fall here when cmd_id not found in the LUT
      * 2 cases are possible:
      *  - this is a valid line: ATACTION_RSP_INTERMEDIATE
      *  - this is an invalid line: ATACTION_RSP_ERROR
      */
  }
  else
  {
    /* cmd_id has been found in the LUT: determine next action */
    switch (element_infos->cmd_id_received)
    {
      case CMD_AT_OK:
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DATA_SUSPEND)
        {
          PRINT_INFO("MODEM SWITCHES TO COMMAND MODE")
        }
        if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON) ||
            (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
        {
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT)
          {
            /* modem is synchronized */
            TYPE1SC_ctxt.persist.modem_at_ready = AT_TRUE;
          }
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATE)
          {
            PRINT_DBG("Echo successfully disabled")
          }
        }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DIAL_COMMAND)
        {
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_ALLOCATE)
          {
            if (type1sc_shared.SocketCmd_Allocated_SocketID == AT_TRUE)
            {
              /* if socket Id has been allocated and OK is received, the socket Id is valid */
              type1sc_shared.SocketCmd_Allocated_SocketID_OK = AT_TRUE;
            }
          }
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_ACTIVATE)
          {
            /* socket is activated */
            type1sc_shared.SocketCmd_Activated = AT_TRUE;
          }
        }

        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SOCKET_CLOSE)
        {
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SOCKETCMD_DELETE)
          {
            /* socket has been deleted successfully */
            type1sc_shared.SocketCmd_Delete_success = AT_TRUE;
          }
        }

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/
        retval = ATACTION_RSP_FRC_END;
        break;

      case CMD_AT_NO_CARRIER:
      case CMD_AT_NO_ANSWER:
        retval = ATACTION_RSP_ERROR;
        break;

      case CMD_AT_RING:
      case CMD_AT_NO_DIALTONE:
      case CMD_AT_BUSY:
        /* VALUES NOT MANAGED IN CURRENT IMPLEMENTATION BECAUSE NOT EXPECTED */
        retval = ATACTION_RSP_ERROR;
        break;

      case CMD_AT_CONNECT:
        PRINT_INFO("MODEM SWITCHES TO DATA MODE")
        retval = (at_action_rsp_t)(ATACTION_RSP_FLAG_DATA_MODE | ATACTION_RSP_FRC_END);
        break;

      /* ###########################  START CUSTOMIZATION PART  ########################### */
      case CMD_AT_CEREG:
      case CMD_AT_CREG:
        /* check if response received corresponds to the command we have send
          *  if not => this is an URC
          */
        if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
        {
          retval = ATACTION_RSP_INTERMEDIATE;
        }
        else
        {
          retval = ATACTION_RSP_URC_FORWARDED;
        }
        break;

      case CMD_AT_CGREG:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_CFUN:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_CPIN:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_CGEV:
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      case CMD_AT_SOCKETEV:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_SOCKETDATA_SEND:
      case CMD_AT_SOCKETDATA_RECEIVE:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

      case CMD_AT_DNSRSLV:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

      case CMD_AT_PINGCMD:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

      case CMD_AT_NOTIFYEV:
        /* parsing of message will be done later */
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_BOOTEV:
      {
        /* We received %BOOTEV event from the modem.
          * If received during Power ON or RESET, it is indicating that the modem is ready.
          * If received in another state, we report to upper layer a modem reboot event.
          */
        if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON) ||
            (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
        {
#if (SUPPORT_BOOTEV == 1)
          type1sc_shared.modem_bootev_received = true;
          if (type1sc_shared.modem_waiting_for_bootev == true)
          {
            type1sc_shared.modem_waiting_for_bootev = false;
            PRINT_INFO("continue INIT modem sequence")
            /* UNLOCK the WAIT EVENT */
            retval = ATACTION_RSP_FRC_END;
          }
          else
          {
            PRINT_DBG("memorize BOOTEV received")
            retval = ATACTION_RSP_URC_IGNORED;
          }
#else
          /* ignore the %BOOTEV event during POWER ON or RESET */
          retval = ATACTION_RSP_URC_IGNORED;
#endif /* (SUPPORT_BOOTEV == 1) */
        }
        else
        {
          /* if event is received in other states, it's an unexpected modem reboot */
          if (atcm_modem_event_received(&TYPE1SC_ctxt, CS_MDMEVENT_BOOT) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        break;
      }


      /* ###########################  END CUSTOMIZATION PART  ########################### */

      case CMD_AT:
        retval = ATACTION_RSP_IGNORED;
        break;

      case CMD_AT_INVALID:
        retval = ATACTION_RSP_ERROR;
        break;

      case CMD_AT_ERROR:
        /* ERROR does not contains parameters, call the analyze function explicitly */
        retval = fRspAnalyze_Error_TYPE1SC(p_at_ctxt, &TYPE1SC_ctxt, p_msg_in, element_infos);
        break;

      case CMD_AT_CME_ERROR:
      case CMD_AT_CMS_ERROR:
        /* do the analyze here because will not be called by parser */
        retval = fRspAnalyze_Error_TYPE1SC(p_at_ctxt, &TYPE1SC_ctxt, p_msg_in, element_infos);
        break;

      default:
        /* check if response received corresponds to the command we have send
          *  if not => this is an ERROR
          */
        if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
        {
          retval = ATACTION_RSP_INTERMEDIATE;
        }
        else if (p_atp_ctxt->current_atcmd.type == ATTYPE_RAW_CMD)
        {
          retval = ATACTION_RSP_IGNORED;
        }
        else
        {
          PRINT_INFO("UNEXPECTED MESSAGE RECEIVED")
          retval = ATACTION_RSP_IGNORED;
        }
        break;
    }
  }

  return (retval);
}

/**
  * @brief  In case the AT command received contains parameters, analyze these parameters one by one.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_msg_in Pointer to buffer containing the received AT command.
  * @param  element_infos Pointer to structure used to parse AT command buffer.
  * @retval at_action_rsp_t Returns action to apply for this command.
  */
at_action_rsp_t ATCustom_TYPE1SC_analyzeParam(at_context_t *p_at_ctxt,
                                              const IPC_RxMessage_t *p_msg_in,
                                              at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PRINT_API("enter ATCustom_TYPE1SC_analyzeParam()")

  /* analyse parameters of the command we received:
    * call the corresponding function from the LUT
  */
  retval = (atcm_get_CmdAnalyzeFunc(&TYPE1SC_ctxt, element_infos->cmd_id_received))(p_at_ctxt,
           &TYPE1SC_ctxt,
           p_msg_in,
           element_infos);

  return (retval);
}

/**
  * @brief  Take specific actions after having finished the analyze of the received AT command.
  *         Indicates if other AT command has to be sent after this one.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  element_infos Pointer to structure used to parse AT command buffer.
  * @retval at_action_rsp_t Returns action to apply for this command.
  */
at_action_rsp_t ATCustom_TYPE1SC_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter ATCustom_TYPE1SC_terminateCmd()")

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* additional tests */
  if (TYPE1SC_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
  {
    /* special case for SID_CS_SEND_DATA
    * indeed, this function is called when an AT cmd is finished
    * but for AT+QISEND, it is called a 1st time when prompt is received
    * and a second time when data have been sent.
    */
    if (p_atp_ctxt->current_SID != (at_msg_t) SID_CS_SEND_DATA)
    {
      /* reset socket_send_state */
      TYPE1SC_ctxt.socket_ctxt.socket_send_state = SocketSendState_No_Activity;
    }
  }

  if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATD) ||
      (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATO) ||
      (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_CGDATA))
  {
    if (element_infos->cmd_id_received == (CMD_ID_t) CMD_AT_CONNECT)
    {
      /* force last command (no command can be sent in DATA mode) */
      p_atp_ctxt->is_final_cmd = 1U;
      PRINT_DBG("CONNECT received")
    }
    else
    {
      PRINT_ERR("expected CONNECT not received !!!")
      retval = ATACTION_RSP_ERROR;
    }
  }

  /* ###########################  END CUSTOMIZATION PART  ########################### */
  return (retval);
}

/**
  * @brief  Returns a buffer containing the response to send to upper layer depending of current context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_rsp_buf Pointer to buffer with the response to send.
  * @retval at_status_t.
  */
at_status_t ATCustom_TYPE1SC_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_TYPE1SC_get_rsp()")

  /* prepare response for a SID - common part */
  retval = atcm_modem_get_rsp(&TYPE1SC_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* prepare response for a SID
  *  all specific behaviors for SID which are returning data in rsp_buf have to be implemented here
  */
  switch (p_atp_ctxt->current_SID)
  {
    case SID_CS_DNS_REQ:
      /* PACK data to response buffer */
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_DNS_REQ,
                               (uint16_t) sizeof(type1sc_shared.DNSRSLV_dns_info.hostIPaddr),
                               (void *)type1sc_shared.DNSRSLV_dns_info.hostIPaddr) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;

    case SID_CS_POWER_OFF:
      /* reinit context for power off case */
      ATC_TYPE1SC_modem_reset(&TYPE1SC_ctxt);
      break;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#if (TYPE1SC_ACTIVATE_PING_REPORT == 1)
    case SID_CS_PING_IP_ADDRESS:
    {
      /* it is not possible to trigger an URC now
        * Final reports (needed by upper layer) will be send by cellular service
        */
      PRINT_INFO("Ping final report")

      /* prepare response */
      clear_ping_resp_struct(&TYPE1SC_ctxt);
      TYPE1SC_ctxt.persist.ping_resp_urc.ping_status = CS_OK;
      /* simulate final report data */
      TYPE1SC_ctxt.persist.ping_resp_urc.is_final_report = CS_TRUE;
      /* index expected by COM  for final report = number of pings requested + 1 */
      TYPE1SC_ctxt.persist.ping_resp_urc.index = TYPE1SC_ctxt.persist.ping_infos.ping_params.pingnum + 1U;
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_URC_PING_RSP,
                               (uint16_t) sizeof(CS_Ping_response_t),
                               (void *)&TYPE1SC_ctxt.persist.ping_resp_urc) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;
    }
#endif /* (TYPE1SC_ACTIVATE_PING_REPORT == 1) */
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

    default:
      break;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  /* reset SID context */
  atcm_reset_SID_context(&TYPE1SC_ctxt.SID_ctxt);

  /* reset socket context */
  atcm_reset_SOCKET_context(&TYPE1SC_ctxt);

  return (retval);
}

/**
  * @brief  Returns a buffer containing the URC received.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_rsp_buf Pointer to buffer with the response to send.
  * @retval at_status_t.
  */
at_status_t ATCustom_TYPE1SC_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_TYPE1SC_get_urc()")

  /* prepare response for an URC - common part */
  retval = atcm_modem_get_urc(&TYPE1SC_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* prepare response for an URC
  *  all specific behaviors for an URC have to be implemented here
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/**
  * @brief  Returns a buffer containing an ERROR report message.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_rsp_buf Pointer to buffer with the error report message to send.
  * @retval at_status_t.
  */
at_status_t ATCustom_TYPE1SC_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_TYPE1SC_get_error()")

  /* prepare response when an error occurred - common part */
  retval = atcm_modem_get_error(&TYPE1SC_ctxt, p_atp_ctxt, p_rsp_buf);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /*  prepare response when an error occurred
  *  all specific behaviors for an error have to be implemented here
  */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/**
  * @brief  Callback function used when a Hardware Event occurs
  * @param  deviceType Indicates which device type has detected the event.
  * @param  hwEvent Hardware event received.
  * @param  gstate GPIO state corresponding to the HW event.
  * @retval at_status_t.
  */
at_status_t ATCustom_TYPE1SC_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate)
{
  UNUSED(gstate);
  UNUSED(deviceType);

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
  at_status_t retval = ATSTATUS_ERROR;
  /* IMPORTANT: Do not add traces int this function of in functions called
   * (this function called under interrupt if GPIO event)
   */
  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* check that:
  *  - event RING from modem
  *  - we are waiting for this event (low power request ongoing)
  *  - RING state is the expected one
  */
  if (hwEvent == HWEVT_MODEM_RING)
  {
    ATC_TYPE1SC_low_power_event(EVENT_LP_MDM_RING, true);
    retval = ATSTATUS_OK;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */
#else
  UNUSED(hwEvent);
  at_status_t retval = ATSTATUS_OK;
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

  return (retval);
}

/**
  * @brief  Global reset of modem parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none.
  */
void ATC_TYPE1SC_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter ATC_TYPE1SC_modem_reset")

  /* common reset function (reset all contexts except SID) */
  atcm_modem_reset(p_modem_ctxt);

  /* modem specific actions if any */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    p_tmp->socket_connId_value = ((uint8_t)UNDEFINED_MODEM_SOCKET_ID);
  }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
}

/**
  * @brief  Reset modem shared parameters.
  * @retval none.
  */
void ATC_TYPE1SC_reset_variables(void)
{
  /* Set default values of TYPE1SC specific variables after SWITCH ON or RESET */
  type1sc_shared.modem_waiting_for_bootev = false;
  type1sc_shared.modem_bootev_received = false;

  /* other values */
#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
  type1sc_shared.host_lp_state = HOST_LP_STATE_IDLE;
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */
}

/**
  * @brief  Reinitialize state of Syntax Automaton used to manage characters received from modem.
  * @retval none.
  */
void ATC_TYPE1SC_reinitSyntaxAutomaton(void)
{
  TYPE1SC_ctxt.state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

/**
  * @brief  Initialization of modem parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none.
  */
void ATC_TYPE1SC_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter ATC_TYPE1SC_modem_init")

  /* common init function (reset all contexts) */
  atcm_modem_init(p_modem_ctxt);

  /* modem specific actions if any */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    p_tmp->socket_connId_value = ((uint8_t)UNDEFINED_MODEM_SOCKET_ID);
  }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
}

#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
/**
  * @brief  Initialization of modem low power parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_bool_t Returns true if Low Power is enabled.
  */
at_bool_t ATC_TYPE1SC_init_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  at_bool_t lp_enabled;

  if (p_modem_ctxt->SID_ctxt.init_power_config.low_power_enable == CS_TRUE)
  {
    /* this parameter is used in CGREG/CEREG to enable PSM (value = 4) */
    p_modem_ctxt->persist.psm_urc_requested = AT_TRUE;

    /* send PSM and EDRX commands: need to populate SID_ctxt.set_power_config
     * Provide psm and edrx default parameters provided but disable them for the moment
     */
    p_modem_ctxt->SID_ctxt.set_power_config.psm_present = CS_TRUE;
    p_modem_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_ENABLE;
    (void) memcpy((void *) &p_modem_ctxt->SID_ctxt.set_power_config.psm,
                  (void *) &p_modem_ctxt->SID_ctxt.init_power_config.psm,
                  sizeof(CS_PSM_params_t));

    p_modem_ctxt->SID_ctxt.set_power_config.edrx_present = CS_TRUE;
    p_modem_ctxt->SID_ctxt.set_power_config.edrx_mode = EDRX_MODE_DISABLE;
    (void) memcpy((void *) &p_modem_ctxt->SID_ctxt.set_power_config.edrx,
                  (void *) &p_modem_ctxt->SID_ctxt.init_power_config.edrx,
                  sizeof(CS_EDRX_params_t));

    lp_enabled = AT_TRUE;

  }
  else
  {
    /* this parameter is used in CGREG/CEREG to not enable PSM (value = 2) */
    p_modem_ctxt->persist.psm_urc_requested = AT_FALSE;
    p_modem_ctxt->SID_ctxt.set_power_config.psm_present = CS_FALSE;
    p_modem_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
    p_modem_ctxt->SID_ctxt.set_power_config.edrx_present = CS_FALSE;
    p_modem_ctxt->SID_ctxt.set_power_config.edrx_mode = EDRX_MODE_DISABLE;

    /* do not send PSM and EDRX commands */
    lp_enabled = AT_FALSE;
  }

  return (lp_enabled);
}

/**
  * @brief  Configure modem low power parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_bool_t Returns true if Low Power is enabled and configured.
  */
at_bool_t ATC_TYPE1SC_set_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  at_bool_t lp_set_and_enabled;

  if (p_modem_ctxt->SID_ctxt.set_power_config.psm_present == CS_TRUE)
  {
    /* the modem info structure SID_ctxt.set_power_config has been already updated */

    /* PSM parameters are present */
    if (p_modem_ctxt->SID_ctxt.set_power_config.psm_mode == PSM_MODE_ENABLE)
    {
      /* PSM is enabled */
      p_modem_ctxt->persist.psm_urc_requested = AT_TRUE;
      lp_set_and_enabled = AT_TRUE;
    }
    else
    {
      /* PSM is explicitly disabled */
      p_modem_ctxt->persist.psm_urc_requested = AT_FALSE;
      lp_set_and_enabled = AT_FALSE;

      /* RESET T3412 and T3324 values reported to upper layer */
      /* T3412, default value (0U means value not available) */
      p_modem_ctxt->persist.low_power_status.nwk_periodic_TAU = 0U;
      /* T3324, default value (0U means value not available) */
      p_modem_ctxt->persist.low_power_status.nwk_active_time = 0U;
      p_modem_ctxt->persist.urc_avail_lp_status = AT_TRUE;
    }
  }
  else
  {
    /* PSM parameters are not present */
    lp_set_and_enabled = AT_FALSE;
  }

  return (lp_set_and_enabled);
}

/**
  * @brief  Manage reception of a Low Power event and take actions.
  * @param  event Descrpition of received Low Power event.
  * @param  called_under_it If true, be careful with code called (no traces, ...)
  * @retval none.
  */
void ATC_TYPE1SC_low_power_event(ATCustom_T1SC_LP_event_t event, bool called_under_it)
{
  uint8_t host_state_error = 1U;

  type1sc_shared_variables_t *p_modem_shared = &type1sc_shared;

  /* ##### manage HOST LP state ##### */
  if (p_modem_shared->host_lp_state == HOST_LP_STATE_IDLE)
  {
    /* Host events */
    if (event == EVENT_LP_HOST_SLEEP_REQ)
    {
      p_modem_shared->host_lp_state = HOST_LP_STATE_LP_REQUIRED;
      host_state_error = 0U;
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (p_modem_shared->host_lp_state == HOST_LP_STATE_LP_REQUIRED)
  {
    if (event == EVENT_LP_MDM_RING)
    {
      /* a sleep confirmation from modem has been received,
      * check if this event was susbribed by upper layers
      */
      if (atcm_modem_event_received(&TYPE1SC_ctxt, CS_MDMEVENT_LP_ENTER) == AT_TRUE)
      {
        /* trigger an internal event to ATCORE (ie an event not received from IPC)
         * to allow to report an URC to upper layers
         */
        AT_internalEvent(DEVTYPE_MODEM_CELLULAR);
        p_modem_shared->host_lp_state = HOST_LP_STATE_LP_CONFIRMED;
        host_state_error = 0U;
      }
    }
    else if (event == EVENT_LP_HOST_SLEEP_CANCEL)
    {
      p_modem_shared->host_lp_state = HOST_LP_STATE_IDLE;
      host_state_error = 0U;
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (p_modem_shared->host_lp_state == HOST_LP_STATE_LP_CONFIRMED)
  {
    if (event == EVENT_LP_HOST_SLEEP_COMPLETE)
    {
      p_modem_shared->host_lp_state = HOST_LP_STATE_LP_ONGOING;
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_HOST_SLEEP_CANCEL)
    {
      /* crossing case: modem confirmed sleep state but host has decided to cancel so consider it
       *  as a wakeup request from host */
      p_modem_shared->host_lp_state = HOST_LP_STATE_WAKEUP_REQUIRED;
      host_state_error = 0U;
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (p_modem_shared->host_lp_state == HOST_LP_STATE_LP_ONGOING)
  {
    if (event == EVENT_LP_HOST_WAKEUP_REQ)
    {
      /* usecase: wakeup requested by Host, need modem confirmation */
      p_modem_shared->host_lp_state = HOST_LP_STATE_WAKEUP_REQUIRED;
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_RING)
    {
      /* a wakeup request from modem has been received during low power state,
      * check if this event was susbribed by upper layers
      */
      if (atcm_modem_event_received(&TYPE1SC_ctxt, CS_MDMEVENT_WAKEUP_REQ) == AT_TRUE)
      {
        /* trigger an internal event for ATCORE (ie an event not received from IPC)
        * to allow to report an URC to upper layers
        */
        AT_internalEvent(DEVTYPE_MODEM_CELLULAR);
        p_modem_shared->host_lp_state = HOST_LP_STATE_WAKEUP_REQUIRED;
        host_state_error = 0U;
      }
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (p_modem_shared->host_lp_state == HOST_LP_STATE_WAKEUP_REQUIRED)
  {
    if (event == EVENT_LP_MDM_RING)
    {
      /* usecase: wakeup requested by Host, waiting from modem confirmation */
      p_modem_shared->host_lp_state = HOST_LP_STATE_IDLE;
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_HOST_WAKEUP_REQ)
    {
      /* usecase: wakeup requested by Modem, waiting from Host confirmation */
      p_modem_shared->host_lp_state = HOST_LP_STATE_IDLE;
      host_state_error = 0U;
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else /* state equal HOST_LP_STATE_ERROR or unexpected */
  {
    /* error or unexpected state  */
    host_state_error = 2U;
  }

  /* --- report automaton status : only if not called during an interrupt !!! --- */
  if (called_under_it == false)
  {
    if (host_state_error == 0U)
    {
      PRINT_INFO("LP HOST AUTOMATON new state=%d ", p_modem_shared->host_lp_state)
    }
    else if (host_state_error == 1U)
    {
      PRINT_INFO("LP HOST AUTOMATON ignore event %d in state %d", event, p_modem_shared->host_lp_state)
    }
    else
    {
      PRINT_INFO("LP HOST AUTOMATON ERROR: unexpected state %d", p_modem_shared->host_lp_state)
    }
  }
}
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

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

