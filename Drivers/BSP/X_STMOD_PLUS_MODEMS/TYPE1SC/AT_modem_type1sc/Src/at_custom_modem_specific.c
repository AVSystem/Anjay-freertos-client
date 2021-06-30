/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
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

/* AT commands format
 * AT+<X>=?    : TEST COMMAND
 * AT+<X>?     : READ COMMAND
 * AT+<X>=...  : WRITE COMMAND
 * AT+<X>      : EXECUTION COMMAND
*/

/* TYPE1SC COMPILATION FLAGS to define in project option if needed:*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_core.h"
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
#include "at_custom_modem_specific.h"
#include "at_custom_modem_signalling.h"
#include "at_custom_modem_socket.h"
#include "at_datapack.h"
#include "at_util.h"
#include "sysctrl_specific.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "plf_config.h"
#include "plf_modem_config.h"
#include "error_handler.h"

#if defined(USE_MODEM_TYPE1SC)
#if defined(HWREF_MURATA_TYPE1SC_EVK)
#else
#error Hardware reference not specified
#endif /* HWREF_MURATA_TYPE1SC_EVK */
#endif /* USE_MODEM_TYPE1SC */

/* Private typedef -----------------------------------------------------------*/
#define SUPPORT_BOOTEV     (1)   /* support or not %BOOTEV (0 or 1) */
#define USE_AT_IFC         (1)   /* 1 to use AT+IFC to set Hw Flow Control mode
                                  * 0 to use AT&K3 */
#define MURATA_CMD_SUBSET  (0)   /* In Murata implementation, only a subset of ALTAIR AT commands is supported
                                  * set this flag to 1 for Murata */

/* Private macros ------------------------------------------------------------*/
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

/* Private defines -----------------------------------------------------------*/
#define CHECK_STEP(stepval) (p_atp_ctxt->step == stepval)

/* ###########################  START CUSTOMIZATION PART  ########################### */
#define TYPE1SC_DEFAULT_TIMEOUT       ((uint32_t)15000)
#define TYPE1SC_CSQ_TIMEOUT           ((uint32_t)25000)
#define TYPE1SC_ESCAPE_TIMEOUT        ((uint32_t)3000)   /* max time allowed to receive a response to an Escape cmd */
#define TYPE1SC_COPS_TIMEOUT          ((uint32_t)180000) /* 180 sec */
#define TYPE1SC_CGATT_TIMEOUT         ((uint32_t)140000) /* 140 sec */
#define TYPE1SC_CGACT_TIMEOUT         ((uint32_t)150000) /* 150 sec */
#define TYPE1SC_ATH_TIMEOUT           ((uint32_t)90000)  /* 90 sec */
#define TYPE1SC_AT_TIMEOUT            ((uint32_t)1000)   /* timeout for AT */
#define TYPE1SC_SOCKET_PROMPT_TIMEOUT ((uint32_t)10000)
#define TYPE1SC_PING_TIMEOUT          ((uint32_t)15000)  /* 15 sec */
#define TYPE1SC_ATK_TIMEOUT           ((uint32_t)3000)   /* initially set to 1 sec but seems not enough sometimes... */
#define TYPE1SC_CPSMS_TIMEOUT         ((uint32_t)60000)
#define TYPE1SC_CEDRX_TIMEOUT         ((uint32_t)60000)
#define TYPE1SC_BOOTEV_TIMEOUT        ((uint32_t)5000) /* maxiboot time allowed to wait %BOOTEV*/
#define TYPE1SC_SIMREADY_TIMEOUT      ((uint32_t)3000U)

#define TYPE1SC_MODEM_SYNCHRO_AT_MAX_RETRIES ((uint8_t)30U)
#define TYPE1SC_MAX_SIM_STATUS_RETRIES  ((uint8_t)20U) /* maximum number of retries to wait SIM ready */
/* Global variables ----------------------------------------------------------*/
/* TYPE1SC Modem device context */
static atcustom_modem_context_t TYPE1SC_ctxt;

/* shared variables specific to Type1SC */
type1sc_shared_variables_t type1sc_shared =
{
  .sim_status_retries = 0U,
  .SocketCmd_Allocated_SocketID = AT_FALSE,
  .setcfg_function = SETGETCFG_UNDEFINED,
  .getcfg_function = SETGETCFG_UNDEFINED,
  .syscfg_function = SETGETSYSCFG_UNDEFINED,
  .modem_waiting_for_bootev = false,
  .modem_bootev_received = false,
  .notifyev_mode = 0U,
  .modem_sim_same_as_selected = true,
  .host_lp_state  = HOST_LP_STATE_IDLE,
};

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void reinitSyntaxAutomaton_TYPE1SC(void);
static void reset_variables_TYPE1SC(void);
static void type1sc_modem_init(atcustom_modem_context_t *p_modem_ctxt);
static void type1sc_modem_reset(atcustom_modem_context_t *p_modem_ctxt);
static at_bool_t init_type1sc_low_power(atcustom_modem_context_t *p_modem_ctxt);
static at_bool_t set_type1sc_low_power(atcustom_modem_context_t *p_modem_ctxt);
static void low_power_event(ATCustom_T1SC_LP_event_t event, bool called_under_it);

/* Functions Definition ------------------------------------------------------*/
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
      CMD_AT_DNSRSLV,                "%DNSRSLV",     TYPE1SC_DEFAULT_TIMEOUT,
      fCmdBuild_DNSRSLV,              fRspAnalyze_DNSRSLV
    },
    {
      CMD_AT_PINGCMD,                "%PINGCMD",     TYPE1SC_PING_TIMEOUT,
      fCmdBuild_PINGCMD,              fRspAnalyze_PINGCMD
    },

    /* MODEM SPECIFIC EVENTS */
    {CMD_AT_SOCKETEV,   "%SOCKETEV",  TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,    fRspAnalyze_SOCKETEV},
    {CMD_AT_BOOTEV,     "%BOOTEV",    TYPE1SC_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,    fRspAnalyze_None},

  };
#define SIZE_ATCMD_TYPE1SC_LUT ((uint16_t) (sizeof (ATCMD_TYPE1SC_LUT) / sizeof (atcustom_LUT_t)))

  /* common init */
  type1sc_modem_init(&TYPE1SC_ctxt);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  TYPE1SC_ctxt.modem_LUT_size = SIZE_ATCMD_TYPE1SC_LUT;
  TYPE1SC_ctxt.p_modem_LUT = (const atcustom_LUT_t *)ATCMD_TYPE1SC_LUT;

  /* override default termination string for AT command: <CR> */
  (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->endstr, "\r");

  /* ###########################  END CUSTOMIZATION PART  ########################### */
}

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

at_status_t ATCustom_TYPE1SC_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout)
{
  at_status_t retval = ATSTATUS_OK;
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_msg_t curSID = p_atp_ctxt->current_SID;

  PRINT_API("enter ATCustom_TYPE1SC_getCmd() for SID %d", curSID)

  /* retrieve parameters from SID command (will update SID_ctxt) */
  if (atcm_retrieve_SID_parameters(&TYPE1SC_ctxt, p_atp_ctxt) != ATSTATUS_OK)
  {
    retval = ATSTATUS_ERROR;
    goto exit_ATCustom_TYPE1SC_getCmd;
  }

  /* new command: reset command context */
  atcm_reset_CMD_context(&TYPE1SC_ctxt.CMD_ctxt);

  /* For each SID, athe sequence of AT commands to send id defined (it can be dynamic)
  * Determine and prepare the next command to send for this SID
  */

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  if (curSID == (at_msg_t) SID_CS_CHECK_CNX)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_MODEM_CONFIG)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if ((curSID == (at_msg_t) SID_CS_POWER_ON) ||
           (curSID == (at_msg_t) SID_CS_RESET))
  {
    uint8_t common_start_sequence_step = TYPE1SC_MODEM_SYNCHRO_AT_MAX_RETRIES + 1U;
    static bool reprogram_bdelay = false;

    /* POWER_ON and RESET are almost the same, specific differences are managed case by case */
    if ((curSID == (at_msg_t) SID_CS_RESET) && (TYPE1SC_ctxt.SID_ctxt.reset_type != CS_RESET_HW))
    {
      /* for reset, only HW reset is supported */
      PRINT_ERR("Reset type (%d) not supported", TYPE1SC_ctxt.SID_ctxt.reset_type)
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
        reset_variables_TYPE1SC();

        /* reinit modem at ready status */
        TYPE1SC_ctxt.persist.modem_at_ready = AT_FALSE;

        /* in case of RESET, reset all the contexts to start from a fresh state */
        if (curSID == (at_msg_t) SID_CS_RESET)
        {
          type1sc_modem_reset(&TYPE1SC_ctxt);
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
        if (TYPE1SC_ctxt.persist.modem_at_ready == AT_FALSE)
        {
          /* use optional as we are not sure to receive a response from the modem: this allows to avoid to return
            * an error to upper layer
            */
          PRINT_DBG("test connection [try number %d] ", p_atp_ctxt->step)
          atcm_program_AT_CMD_ANSWER_OPTIONAL(&TYPE1SC_ctxt, p_atp_ctxt,
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
        if (TYPE1SC_ctxt.persist.modem_at_ready == AT_FALSE)
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
        TYPE1SC_ctxt.CMD_ctxt.cfun_value = 0U;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 1U))
      {
        /* disable echo */
        TYPE1SC_ctxt.CMD_ctxt.command_echo = AT_FALSE;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATE, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 2U))
      {
        /* request detailed error report */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CMEE, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 3U))
      {
        /* enable full response format */
        TYPE1SC_ctxt.CMD_ctxt.dce_full_resp_format = AT_TRUE;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATV, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 4U))
      {
        /* Read FW revision */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 5U))
      {
#if (MURATA_CMD_SUBSET == 0)
        /* Read bands configuration */
        type1sc_shared.getcfg_function = SETGETCFG_BAND;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETCFG, INTERMEDIATE_CMD);
#else
        atcm_program_SKIP_CMD(p_atp_ctxt);
#endif /* MURATA_CMD_SUBSET == 0 */
      }
      else if CHECK_STEP((common_start_sequence_step + 6U))
      {
        /* Read HIFC mode (low power) */
        type1sc_shared.getcfg_function = SETGETCFG_HIFC_MODE;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETACFG, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 7U))
      {
        /* Read PMCONF SLEEP mode (low power) */
        type1sc_shared.getcfg_function = SETGETCFG_PMCONF_SLEEP_MODE;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETACFG, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 8U))
      {
        /* Read PMCONF MAX ALLOWED (low power) */
        type1sc_shared.getcfg_function = SETGETCFG_PMCONF_MAX_ALLOWED;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETACFG, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 9U))
      {
        /* long boot time detected ? try to shorten it */
        if (reprogram_bdelay == true)
        {
          PRINT_INFO("Reprogram modem boot delay")
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt,
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
        TYPE1SC_ctxt.SID_ctxt.set_power_config.psm_present = CELLULAR_TRUE;
        TYPE1SC_ctxt.SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
      }
#if (CONFIG_MODEM_UART_RTS_CTS == 0)
      /* --- No HW Flow control requested ----
        *   This is the default mode when modem is booting, nothing to do.
        */
      else if CHECK_STEP((common_start_sequence_step + 11U))
      {
#if (USE_AT_IFC == 1)
        /* apply current Hw Flow Control mode */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
#else
        atcm_program_AT_CMD_ANSWER_OPTIONAL(&TYPE1SC_ctxt, p_atp_ctxt,
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
        atcm_program_AT_CMD_ANSWER_OPTIONAL(&TYPE1SC_ctxt, p_atp_ctxt,
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
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_IFC, INTERMEDIATE_CMD);
#else
        atcm_program_AT_CMD_ANSWER_OPTIONAL(&TYPE1SC_ctxt, p_atp_ctxt,
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
        atcm_program_AT_CMD_ANSWER_OPTIONAL(&TYPE1SC_ctxt, p_atp_ctxt,
                                            ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, INTERMEDIATE_CMD);
      }

#endif /* (CONFIG_MODEM_UART_RTS_CTS == 0) */
#if (SUPPORT_BOOTEV == 1)
      else if CHECK_STEP((common_start_sequence_step + 14U))
      {
        /* request to receive boot event */
        type1sc_shared.setcfg_function = SETGETCFG_BOOT_EVENT_TRUE;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETACFG, INTERMEDIATE_CMD);
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
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETACFG, INTERMEDIATE_CMD);
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
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_NOTIFYEV, INTERMEDIATE_CMD);
      }
      else if CHECK_STEP((common_start_sequence_step + 17U))
      {
        /* check connection */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT, FINAL_CMD);
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
  }
  else if (curSID == (at_msg_t) SID_CS_POWER_OFF)
  {
    if CHECK_STEP((0U))
    {
      /* it's upper layer responsibility to manage proper detach before switch off
        * TYPE1SC_ctxt.CMD_ctxt.cfun_value = 0U;
        * atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, FINAL_CMD);
        */
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_INIT_MODEM)
  {
    if CHECK_STEP((0U))
    {
      /* cfun parameters coming from client API for SID_CS_INIT_MODEM */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CFUN, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      if (TYPE1SC_ctxt.SID_ctxt.modem_init.init == CS_CMI_MINI)
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
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPIN, INTERMEDIATE_CMD);
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
        PRINT_INFO("SIM sim_pin_code_ready %d", TYPE1SC_ctxt.persist.sim_pin_code_ready)
        PRINT_INFO("SIM sim_state %d", TYPE1SC_ctxt.persist.sim_state)
        if ((TYPE1SC_ctxt.persist.sim_pin_code_ready == AT_FALSE) &&
            ((TYPE1SC_ctxt.persist.sim_state == CS_SIMSTATE_SIM_BUSY)))
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

      if (TYPE1SC_ctxt.persist.sim_pin_code_ready == AT_FALSE)
      {
        if (strlen((const CRC_CHAR_t *)&TYPE1SC_ctxt.SID_ctxt.modem_init.pincode.pincode) != 0U)
        {
          /* send PIN value */
          PRINT_INFO("CPIN required, we send user value to modem")
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPIN, FINAL_CMD);
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
  }
  else if (curSID == (at_msg_t) SID_CS_GET_DEVICE_INFO)
  {
    if CHECK_STEP((0U))
    {
      switch (TYPE1SC_ctxt.SID_ctxt.device_info->field_requested)
      {
        case CS_DIF_MANUF_NAME_PRESENT:
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMI, FINAL_CMD);
          break;

        case CS_DIF_MODEL_PRESENT:
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMM, FINAL_CMD);
          break;

        case CS_DIF_REV_PRESENT:
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGMR, FINAL_CMD);
          break;

        case CS_DIF_SN_PRESENT:
          TYPE1SC_ctxt.CMD_ctxt.cgsn_write_cmd_param = CGSN_SN;
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMEI_PRESENT:
          TYPE1SC_ctxt.CMD_ctxt.cgsn_write_cmd_param = CGSN_IMEI;
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGSN, FINAL_CMD);
          break;

        case CS_DIF_IMSI_PRESENT:
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CIMI, FINAL_CMD);
          break;

        case CS_DIF_PHONE_NBR_PRESENT:
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CNUM, FINAL_CMD);
          break;

        case CS_DIF_ICCID_PRESENT:
          atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CCID, FINAL_CMD);
          break;

        default:
          /* error, invalid step */
          retval = ATSTATUS_ERROR;
          break;
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_SIGNAL_QUALITY)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_AT_CSQ, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_ATTACHSTATUS)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_REGISTER_NET)
  {
    if CHECK_STEP((0U))
    {
      /* read registration status */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      /* check if actual registration status is the expected one or if AcT is explicitly specified */
      CS_OperatorSelector_t *operatorSelect = &(TYPE1SC_ctxt.SID_ctxt.write_operator_infos);
      if ((TYPE1SC_ctxt.SID_ctxt.read_operator_infos.mode != operatorSelect->mode) ||
          (operatorSelect->AcT_present == CELLULAR_TRUE))
      {
        /* write registration status */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_COPS, INTERMEDIATE_CMD);
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
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEREG, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_GET_NETSTATUS)
  {
    if CHECK_STEP((0U))
    {
      /* read registration status */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      /* read registration status */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CREG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((2U))
    {
      /* read extended error report */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEER, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((3U))
    {
      /* read registration status */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_COPS, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SUSBCRIBE_NET_EVENT)
  {
    if CHECK_STEP((0U))
    {
      CS_UrcEvent_t urcEvent = TYPE1SC_ctxt.SID_ctxt.urcEvent;

      /* is an event linked to CREG or CEREG ? */
      if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
      {
        (void) atcm_subscribe_net_event(&TYPE1SC_ctxt, p_atp_ctxt);
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
  }
  else if (curSID == (at_msg_t) SID_CS_UNSUSBCRIBE_NET_EVENT)
  {
    if CHECK_STEP((0U))
    {
      CS_UrcEvent_t urcEvent = TYPE1SC_ctxt.SID_ctxt.urcEvent;

      /* is an event linked to CREG, CGREG or CEREG ? */
      if ((urcEvent == CS_URCEVENT_EPS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_EPS_LOCATION_INFO) ||
          (urcEvent == CS_URCEVENT_CS_NETWORK_REG_STAT) || (urcEvent == CS_URCEVENT_CS_LOCATION_INFO))
      {
        (void) atcm_unsubscribe_net_event(&TYPE1SC_ctxt, p_atp_ctxt);
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
  }
  else if (curSID == (at_msg_t) SID_CS_REGISTER_PDN_EVENT)
  {
    if CHECK_STEP((0U))
    {
      if (TYPE1SC_ctxt.persist.urc_subscript_pdn_event == CELLULAR_FALSE)
      {
        /* set event as subscribed */
        TYPE1SC_ctxt.persist.urc_subscript_pdn_event = CELLULAR_TRUE;

        /* request PDN events */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGEREP, FINAL_CMD);
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
  }
  else if (curSID == (at_msg_t) SID_CS_DEREGISTER_PDN_EVENT)
  {
    if CHECK_STEP((0U))
    {
      if (TYPE1SC_ctxt.persist.urc_subscript_pdn_event == CELLULAR_TRUE)
      {
        /* set event as unsuscribed */
        TYPE1SC_ctxt.persist.urc_subscript_pdn_event = CELLULAR_FALSE;

        /* request PDN events */
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGEREP, FINAL_CMD);
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
  }
  else if (curSID == (at_msg_t) SID_ATTACH_PS_DOMAIN)
  {
    if CHECK_STEP((0U))
    {
      TYPE1SC_ctxt.CMD_ctxt.cgatt_write_cmd_param = CGATT_ATTACHED;
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_DETACH_PS_DOMAIN)
  {
    if CHECK_STEP((0U))
    {
      TYPE1SC_ctxt.CMD_ctxt.cgatt_write_cmd_param = CGATT_DETACHED;
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGATT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_ACTIVATE_PDN)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    if CHECK_STEP((0U))
    {
      /* PDN activation */
      TYPE1SC_ctxt.CMD_ctxt.pdn_state = PDN_STATE_ACTIVATE;
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNACT, FINAL_CMD);
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
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATD, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == (at_msg_t) SID_CS_DEACTIVATE_PDN)
  {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* SOCKET MODE */
    if CHECK_STEP((0U))
    {
      /* PDN deactivation */
      TYPE1SC_ctxt.CMD_ctxt.pdn_state = PDN_STATE_DEACTIVATE;
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNACT, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#else
    /* DATA MODE */
    /* not implemented yet */
    retval = ATSTATUS_ERROR;
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == (at_msg_t) SID_CS_DEFINE_PDN)
  {
    /* DATA MODE*/
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNSET, INTERMEDIATE_CMD);
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
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_PDNSET, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SET_DEFAULT_PDN)
  {
    /* nothing to do here
      * Indeed, default PDN has been saved automatically during analysis of SID command
      * cf function: atcm_retrieve_SID_parameters()
      */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else if (curSID == (at_msg_t) SID_CS_GET_IP_ADDRESS)
  {
    /* get IP address */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PDNRDP, FINAL_CMD);
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
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CGPADDR, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
#endif /* USE_SOCKETS_TYPE */
  }
  else if (curSID == (at_msg_t) SID_CS_DIAL_COMMAND)
  {
    if CHECK_STEP((0U))
    {
      /* step  - allocate a socket and request a socket_id */
      type1sc_shared.SocketCmd_Allocated_SocketID = AT_FALSE;
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_ALLOCATE,
                          INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      /* step 2 - verify that a socket_id has been allocated then activate the socket*/
      if (type1sc_shared.SocketCmd_Allocated_SocketID == AT_TRUE)
      {
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_ACTIVATE,
                            INTERMEDIATE_CMD);
      }
      else
      {
        PRINT_ERR("No valid socket_id affected by the modem has been reecived")
        retval = ATSTATUS_ERROR;
      }
    }
    else if CHECK_STEP((2U))
    {
      /* socket is connected */
      (void) atcm_socket_set_connected(&TYPE1SC_ctxt, TYPE1SC_ctxt.socket_ctxt.socket_info->socket_handle);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SEND_DATA)
  {
    if CHECK_STEP((0U))
    {
      /* Check data size to send */
      if (TYPE1SC_ctxt.SID_ctxt.socketSendData_struct.buffer_size > MODEM_MAX_SOCKET_TX_DATA_SIZE)
      {
        PRINT_ERR("Data size to send %ld exceed maximum size %ld",
                  TYPE1SC_ctxt.SID_ctxt.socketSendData_struct.buffer_size,
                  MODEM_MAX_SOCKET_TX_DATA_SIZE)
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
        retval = ATSTATUS_ERROR;
      }
      else
      {
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETDATA_SEND, FINAL_CMD);
      }
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if ((curSID == (at_msg_t) SID_CS_RECEIVE_DATA) ||
           (curSID == (at_msg_t) SID_CS_RECEIVE_DATA_FROM))
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETDATA_RECEIVE, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SOCKET_CLOSE)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_DEACTIVATE,
                          INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_DELETE, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SOCKET_CNX_STATUS)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SOCKETCMD_INFO, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DATA_SUSPEND)
  {
    if CHECK_STEP((0U))
    {
      /* wait for 1 second */
      atcm_program_TEMPO(p_atp_ctxt, 1000U, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      /* send escape sequence +++ (RAW command type)
        */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_ESC_CMD, INTERMEDIATE_CMD);
      reinitSyntaxAutomaton_TYPE1SC();

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
  }
  else if (curSID == (at_msg_t) SID_CS_DATA_RESUME)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATO, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_INIT_POWER_CONFIG)
  {
    if CHECK_STEP((0U))
    {
      /* Init parameters are available in to SID_ctxt.init_power_config
        * SID_ctxt.init_power_config  is used to build AT+CPSMS and AT+CEDRX commands
        * Built it from SID_ctxt.init_power_config  and modem specificities
        */
      if (init_type1sc_low_power(&TYPE1SC_ctxt) == AT_FALSE)
      {
        /* Low Power not enabled, stop here the SID */
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
      else
      {
        /* Low Power enabled : update CEREG to request PSM parameters then send commands */
        TYPE1SC_ctxt.CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
      }
    }
    else if CHECK_STEP((1U))
    {
      /* read EDRX params */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CEDRXS, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((2U))
    {
      /* read PSM params */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_READ_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((3U))
    {
      /* set EDRX params (default) */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEDRXS, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((4U))
    {
      /* set PSM params (default) */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
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
  }
  else if (curSID == (at_msg_t) SID_CS_SET_POWER_CONFIG)
  {
    if CHECK_STEP((0U))
    {
      if (set_type1sc_low_power(&TYPE1SC_ctxt) == AT_FALSE)
      {
        atcm_program_SKIP_CMD(p_atp_ctxt);
      }
      else
      {
        /* Low Power enabled : update CEREG to request PSM parameters then send commands */
        TYPE1SC_ctxt.CMD_ctxt.cxreg_write_cmd_param = CXREG_ENABLE_PSM_NETWK_REG_LOC_URC;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEREG, INTERMEDIATE_CMD);
      }
    }
    else if CHECK_STEP((1U))
    {
      /* set EDRX params (if available) */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CEDRXS, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((2U))
    {
      /* set PSM params (if available) */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CPSMS, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((3U))
    {
      /* eDRX Read Dynamix Parameters */
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt,
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
  }
  else if (curSID == (at_msg_t) SID_CS_SLEEP_REQUEST)
  {
    if CHECK_STEP((0U))
    {
      /* update LP automaton state */
      low_power_event(EVENT_LP_HOST_SLEEP_REQ, false);

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
  }
  else if (curSID == (at_msg_t) SID_CS_SLEEP_COMPLETE)
  {
    if CHECK_STEP((0U))
    {
      low_power_event(EVENT_LP_HOST_SLEEP_COMPLETE, false);
      (void) SysCtrl_TYPE1SC_complete_suspend_channel(p_at_ctxt->ipc_handle, DEVTYPE_MODEM_CELLULAR);
      atcm_program_NO_MORE_CMD(p_atp_ctxt);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SLEEP_CANCEL)
  {
    if CHECK_STEP((0U))
    {
      low_power_event(EVENT_LP_HOST_SLEEP_CANCEL, false);

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
  }
  else if (curSID == (at_msg_t) SID_CS_WAKEUP)
  {
    if CHECK_STEP((0U))
    {
      low_power_event(EVENT_LP_HOST_WAKEUP_REQ, false);

      uint8_t modem_originated = (TYPE1SC_ctxt.SID_ctxt.wakeup_origin == MODEM_WAKEUP) ? 1U : 0U;
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
  }
  else if (curSID == (at_msg_t) SID_CS_DNS_REQ)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_DNSRSLV, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SUSBCRIBE_MODEM_EVENT)
  {
    /* nothing to do here
      * Indeed, default modem events subscribed havebeen saved automatically during analysis of SID command
      * cf function: atcm_retrieve_SID_parameters()
      */
    atcm_program_NO_MORE_CMD(p_atp_ctxt);
  }
  else if (curSID == (at_msg_t) SID_CS_PING_IP_ADDRESS)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_PINGCMD, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_DIRECT_CMD)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_RAW_CMD, (CMD_ID_t) CMD_AT_DIRECT_CMD, FINAL_CMD);
      atcm_program_CMD_TIMEOUT(&TYPE1SC_ctxt, p_atp_ctxt, TYPE1SC_ctxt.SID_ctxt.direct_cmd_tx->cmd_timeout);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SIM_SELECT)
  {
    if CHECK_STEP((0U))
    {
      type1sc_shared.syscfg_function = SETGETSYSCFG_SIM_POLICY;
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_GETSYSCFG, INTERMEDIATE_CMD);
    }
    else if CHECK_STEP((1U))
    {
      /* if current sim in modem is different than the requested SIM,
        * send cmd to configure the modem (a modem reboot is needed) */
      if (type1sc_shared.modem_sim_same_as_selected == false)
      {
        type1sc_shared.syscfg_function = SETGETSYSCFG_SIM_POLICY;
        atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_SETSYSCFG, INTERMEDIATE_CMD);
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
        atcm_set_error_report(CSERR_UNKNOWN, &TYPE1SC_ctxt);
        /* atcm_set_error_report(CSERR_MODEM_REBOOT_NEEDED, &TYPE1SC_ctxt); */
        retval = ATSTATUS_ERROR;
      }
      else
      {
        atcm_program_NO_MORE_CMD(p_atp_ctxt);
      }
    }
  }
  else if (curSID == (at_msg_t) SID_CS_SIM_GENERIC_ACCESS)
  {
    if CHECK_STEP((0U))
    {
      atcm_program_AT_CMD(&TYPE1SC_ctxt, p_atp_ctxt, ATTYPE_WRITE_CMD, (CMD_ID_t) CMD_AT_CSIM, FINAL_CMD);
    }
    else
    {
      /* error, invalid step */
      retval = ATSTATUS_ERROR;
    }
  }

  /* ###########################  END CUSTOMIZATION PART  ########################### */
  else
  {
    PRINT_ERR("Error, invalid command ID %d", curSID)
    retval = ATSTATUS_ERROR;
  }

  /* if no error, build the command to send */
  if (retval == ATSTATUS_OK)
  {
    retval = atcm_modem_build_cmd(&TYPE1SC_ctxt, p_atp_ctxt, p_ATcmdTimeout);
  }

exit_ATCustom_TYPE1SC_getCmd:
  return (retval);
}

at_endmsg_t ATCustom_TYPE1SC_extractElement(atparser_context_t *p_atp_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos)
{
  UNUSED(p_atp_ctxt);
  at_endmsg_t retval_msg_end_detected = ATENDMSG_NO;
  bool exit_loop;
  uint16_t *p_parseIndex = &(element_infos->current_parse_idx);

  PRINT_API("enter ATCustom_TYPE1SC_extractElement()")
  PRINT_DBG("input message: size=%d ", p_msg_in->size)

  /* if this is beginning of message, check that message header is correct and jump over it */
  if (*p_parseIndex == 0U)
  {
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
    goto exit_ATCustom_TYPE1SC_extractElement;
  }

  /* extract parameter from message */
  exit_loop = false;
  do
  {
    switch (p_msg_in->buffer[*p_parseIndex])
    {
      /* ###########################  START CUSTOMIZATION PART  ########################### */
      /* ----- test separators ----- */
      case ':':
      case ',':
        exit_loop = true;
        break;

      case '=':
        /* special separator case for AT+ICF?
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
        /* increment end position */
        element_infos->str_end_idx = *p_parseIndex;
        element_infos->str_size++;
        break;
        /* ###########################  END CUSTOMIZATION PART  ########################### */
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

exit_ATCustom_TYPE1SC_extractElement:
  return (retval_msg_end_detected);
}

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

/* function called to finalize an AT command */
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

/* function called to finalize a SID */
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
      type1sc_modem_reset(&TYPE1SC_ctxt);
      break;

#if (TYPE1SC_ACTIVATE_PING_REPORT == 1)
    case SID_CS_PING_IP_ADDRESS:
    {
      /* it is not possible to trigger an URC now
        * Final reports (needed by upper layer) will be send by cellular service
        */
      PRINT_INFO("Ping final report")

      /* prepare response */
      clear_ping_resp_struct(&TYPE1SC_ctxt);
      TYPE1SC_ctxt.persist.ping_resp_urc.ping_status = CELLULAR_OK;
      /* simulate final report data */
      TYPE1SC_ctxt.persist.ping_resp_urc.is_final_report = CELLULAR_TRUE;
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

at_status_t ATCustom_TYPE1SC_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate)
{
  UNUSED(gstate);
  UNUSED(deviceType);
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
    low_power_event(EVENT_LP_MDM_RING, true);
    retval = ATSTATUS_OK;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/* Private function Definition -----------------------------------------------*/
static void reinitSyntaxAutomaton_TYPE1SC(void)
{
  TYPE1SC_ctxt.state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

static void reset_variables_TYPE1SC(void)
{
  /* Set default values of TYPE1SC specific variables after SWITCH ON or RESET */
  type1sc_shared.modem_waiting_for_bootev = false;
  type1sc_shared.modem_bootev_received = false;

  /* other values */
  type1sc_shared.host_lp_state = HOST_LP_STATE_IDLE;
}

/* Type1SC modem init function
*  call common init function and then do actions specific to this modem
*/
static void type1sc_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter type1sc_modem_init")

  /* common init function (reset all contexts) */
  atcm_modem_init(p_modem_ctxt);

  /* modem specific actions if any */
  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    p_tmp->socket_connId_value = ((uint8_t)UNDEFINED_MODEM_SOCKET_ID);

  }
}

/* Type1SC modem reset function
*  call common reset function and then do actions specific to this modem
*/
static void type1sc_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter type1sc_modem_reset")

  /* common reset function (reset all contexts except SID) */
  atcm_modem_reset(p_modem_ctxt);

  /* modem specific actions if any */
  for (uint8_t i = 0U; i < CELLULAR_MAX_SOCKETS; i++)
  {
    atcustom_persistent_SOCKET_context_t *p_tmp;
    p_tmp = &p_modem_ctxt->persist.socket[i];
    p_tmp->socket_connId_value = ((uint8_t)UNDEFINED_MODEM_SOCKET_ID);
  }
}

static at_bool_t init_type1sc_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  at_bool_t lp_enabled;

  if (p_modem_ctxt->SID_ctxt.init_power_config.low_power_enable == CELLULAR_TRUE)
  {
    /* this parameter is used in CGREG/CEREG to enable PSM (value = 4) */
    p_modem_ctxt->persist.psm_urc_requested = AT_TRUE;

    /* send PSM and EDRX commands: need to populate SID_ctxt.set_power_config
     * Provide psm and edrx default parameters provided but disable them for the moment
     */
    p_modem_ctxt->SID_ctxt.set_power_config.psm_present = CELLULAR_TRUE;
    p_modem_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_ENABLE;
    (void) memcpy((void *) &p_modem_ctxt->SID_ctxt.set_power_config.psm,
                  (void *) &p_modem_ctxt->SID_ctxt.init_power_config.psm,
                  sizeof(CS_PSM_params_t));

    p_modem_ctxt->SID_ctxt.set_power_config.edrx_present = CELLULAR_TRUE;
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
    p_modem_ctxt->SID_ctxt.set_power_config.psm_present = CELLULAR_FALSE;
    p_modem_ctxt->SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
    p_modem_ctxt->SID_ctxt.set_power_config.edrx_present = CELLULAR_FALSE;
    p_modem_ctxt->SID_ctxt.set_power_config.edrx_mode = EDRX_MODE_DISABLE;

    /* do not send PSM and EDRX commands */
    lp_enabled = AT_FALSE;
  }

  return (lp_enabled);
}

static at_bool_t set_type1sc_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  at_bool_t lp_set_and_enabled;

  if (p_modem_ctxt->SID_ctxt.set_power_config.psm_present == CELLULAR_TRUE)
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

static void low_power_event(ATCustom_T1SC_LP_event_t event, bool called_under_it)
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

  /* --- report automaton status : only if nat called during an interrupt !!! --- */
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
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

