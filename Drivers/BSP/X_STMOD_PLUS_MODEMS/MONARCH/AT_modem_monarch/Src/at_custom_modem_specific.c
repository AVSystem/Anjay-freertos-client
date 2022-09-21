/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
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

/* GM01Q COMPILATION FLAGS to define in project option if needed:
*
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

#if defined(USE_MODEM_GM01Q)
#if defined(HWREF_GM01QDBA1)
#else
#error Hardware reference not specified
#endif /* HWREF_GM01QDBA1 */
#endif /* USE_MODEM_GM01Q */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC AT_CUSTOM SEQUANS_MONARCH SPECIFIC
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Private_Macros AT_CUSTOM SEQUANS_MONARCH SPECIFIC Private Macros
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

/**
  * @}
  */


/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Private_Variables AT_CUSTOM SEQUANS_MONARCH SPECIFIC Private Variables
  * @{
  */
/* GM01Q Modem device context */
static atcustom_modem_context_t SEQMONARCH_ctxt;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Exported_Variables
  *    AT_CUSTOM SEQUANS_MONARCH SPECIFIC Exported Variables
  * @{
  */
/* shared variables specific to MONARCH */
monarch_shared_variables_t monarch_shared =
{
  .SMST_sim_error_status = 0U,
  .waiting_for_ring_irq = false,
};
/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Private_Functions_Prototypes
  *   AT_CUSTOM SEQUANS_MONARCH SPECIFIC Private Functions Prototypes
  * @{
  */
static void monarch_modem_init(atcustom_modem_context_t *p_modem_ctxt);
static void socketHeaderRX_reset(void);
static at_bool_t SocketHeaderRX_valid_header(void);
static at_bool_t SocketHeaderRX_analyze_current_header(uint8_t rxchar);
static at_bool_t SocketHeaderRX_update_maxByte(uint8_t rxchar);
/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Exported_Functions
  *    AT_CUSTOM SEQUANS_MONARCH SPECIFIC Exported Functions
  * @{
  */

/**
  * @brief  Initialize specific AT commands.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @retval none
  */
void ATCustom_MONARCH_init(atparser_context_t *p_atp_ctxt)
{
  /* Commands Look-up table : list of commands supported for this modem */
  static const atcustom_LUT_t ATCMD_SEQMONARCH_LUT[] =
  {
    /* cmd enum - cmd string - cmd timeout (in ms) - build cmd ftion - analyze cmd ftion */
    {CMD_AT,             "",             SEQMONARCH_AT_TIMEOUT,       fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_OK,          "OK",           SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CONNECT,     "CONNECT",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_RING,        "RING",         SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_CARRIER,  "NO CARRIER",   SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_ERROR,       "ERROR",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_MONARCH},
    {CMD_AT_NO_DIALTONE, "NO DIALTONE",  SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_BUSY,        "BUSY",         SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_ANSWER,   "NO ANSWER",    SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CME_ERROR,   "+CME ERROR",   SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_MONARCH},
    {CMD_AT_CMS_ERROR,   "+CMS ERROR",   SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CmsErr},

    /* GENERIC MODEM commands */
    {CMD_AT_CGMI,        "+CGMI",        SEQMONARCH_CMD_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMI},
    {CMD_AT_CGMM,        "+CGMM",        SEQMONARCH_CMD_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMM},
    {CMD_AT_CGMR,        "+CGMR",        SEQMONARCH_CMD_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMR},
    {CMD_AT_CGSN,        "+CGSN",        SEQMONARCH_CMD_TIMEOUT,  fCmdBuild_CGSN,       fRspAnalyze_CGSN},
    {CMD_AT_CIMI,        "+CIMI",        SEQMONARCH_CMD_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CIMI},
    {CMD_AT_CEER,        "+CEER",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEER},
    {CMD_AT_CMEE,        "+CMEE",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CMEE,       fRspAnalyze_None},
    {CMD_AT_CPIN,        "+CPIN",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CPIN,       fRspAnalyze_CPIN},
    {CMD_AT_CFUN,        "+CFUN",        SEQMONARCH_CFUN_TIMEOUT,     fCmdBuild_CFUN,       fRspAnalyze_None},
    {CMD_AT_COPS,        "+COPS",        SEQMONARCH_COPS_TIMEOUT,     fCmdBuild_COPS,       fRspAnalyze_COPS},
    {CMD_AT_CNUM,        "+CNUM",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CNUM},
    {CMD_AT_CGATT,       "+CGATT",       SEQMONARCH_CGATT_TIMEOUT,    fCmdBuild_CGATT,      fRspAnalyze_CGATT},
    {CMD_AT_CEREG,       "+CEREG",       SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CEREG,      fRspAnalyze_CEREG},
    {CMD_AT_CGEREP,      "+CGEREP",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CGEREP,     fRspAnalyze_None},
    {CMD_AT_CGEV,        "+CGEV",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGEV},
    {CMD_AT_CSQ,         "+CSQ",         SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CSQ},
    {CMD_AT_CGDCONT,     "+CGDCONT",     SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CGDCONT,    fRspAnalyze_None},
    {CMD_AT_CGACT,       "+CGACT",       SEQMONARCH_CGACT_TIMEOUT,    fCmdBuild_CGACT,      fRspAnalyze_None},
    {CMD_AT_CGAUTH,      "+CGAUTH",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CGAUTH,     fRspAnalyze_None},
    {CMD_AT_CGDATA,      "+CGDATA",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CGDATA,     fRspAnalyze_None},
    {CMD_AT_CGPADDR,     "+CGPADDR",     SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CGPADDR,    fRspAnalyze_CGPADDR},
    {CMD_ATD,            "D",            SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_ATD_MONARCH, fRspAnalyze_None},
    {CMD_ATO,            "O",            SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATE,            "E",            SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_ATE,        fRspAnalyze_None},
    {CMD_ATV,            "V",            SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_ATV,        fRspAnalyze_None},
    {CMD_AT_ESC_CMD,     "+++",          SEQMONARCH_ESCAPE_TIMEOUT,   fCmdBuild_ESCAPE_CMD, fRspAnalyze_None},
    {CMD_AT_DIRECT_CMD,  "",             SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_DIRECT_CMD, fRspAnalyze_DIRECT_CMD},
    {CMD_AT_IPR,         "+IPR",         SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_IPR,        fRspAnalyze_IPR},
    {CMD_AT_IFC,         "+IFC",         SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_IFC,        fRspAnalyze_None},
    {CMD_AT_CPSMS,       "+CPSMS",       SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CPSMS,      fRspAnalyze_CPSMS},
    {CMD_AT_CEDRXS,      "+CEDRXS",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CEDRXS,     fRspAnalyze_CEDRXS},
    {CMD_AT_CEDRXP,      "+CEDRXP",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEDRXP},
    {CMD_AT_CEDRXRDP,    "+CEDRXRDP",    SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEDRXRDP},
    {CMD_AT_CSIM,        "+CSIM",        SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CSIM,       fRspAnalyze_CSIM},

    /* MODEM SPECIFIC COMMANDS */
    {CMD_AT_RESET,       "^RESET",       SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,           fRspAnalyze_None},
    {CMD_AT_SQNCTM,      "+SQNCTM",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_SQNCTM_MONARCH,     fRspAnalyze_None},
    {CMD_AT_AUTOATT,     "^AUTOATT",     SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_AUTOATT_MONARCH,    fRspAnalyze_None},
    {CMD_AT_CGDCONT_REPROGRAM, "",  SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_CGDCONT_REPROGRAM_MONARCH, fRspAnalyze_None},
    {CMD_AT_ICCID,       "+SQNCCID",   SEQMONARCH_SQNSH_TIMEOUT,    fCmdBuild_NoParams, fRspAnalyze_SQNCCID_MONARCH},
    {CMD_AT_SMST,        "+SMST",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_SMST_MONARCH, fRspAnalyze_SMST_MONARCH},
    {CMD_AT_CESQ,        "+CESQ",      SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,  fRspAnalyze_CESQ_MONARCH},
    {CMD_AT_SQNSSHDN,    "+SQNSSHDN",  SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,            fRspAnalyze_None},
    {CMD_AT_SHUTDOWN,    "+SHUTDOWN",  SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,            fRspAnalyze_None},

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* MODEM SPECIFIC COMMANDS USED FOR SOCKET MODE */
    {CMD_AT_SQNSRING,    "+SQNSRING",    SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_SQNSRING_MONARCH},
    {CMD_AT_SQNSCFG,     "+SQNSCFG",     SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_SQNSCFG_MONARCH,    fRspAnalyze_None},
    {CMD_AT_SQNSCFGEXT,  "+SQNSCFGEXT",  SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_SQNSCFGEXT_MONARCH, fRspAnalyze_None},
    {CMD_AT_SQNSD,       "+SQNSD",       SEQMONARCH_SQNSD_TIMEOUT,    fCmdBuild_SQNSD_MONARCH,      fRspAnalyze_None},
    {CMD_AT_SQNSH,       "+SQNSH", SEQMONARCH_SQNSH_TIMEOUT,    fCmdBuild_SQNSH_MONARCH, fRspAnalyze_SQNSH_MONARCH},
    {CMD_AT_SQNSI,       "+SQNSI", SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_SQNSI_MONARCH, fRspAnalyze_SQNSI_MONARCH},
    {CMD_AT_SQNSS,       "+SQNSS", SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_SQNSS_MONARCH},
    {
      CMD_AT_SQNSRECV, "+SQNSRECV", SEQMONARCH_DEFAULT_TIMEOUT,
      fCmdBuild_SQNSRECV_MONARCH, fRspAnalyze_SQNSRECV_MONARCH
    },
    {CMD_AT_SQNSSENDEXT, "+SQNSSENDEXT", SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_SQNSSENDEXT_MONARCH, fRspAnalyze_None},
    {
      CMD_AT_SQNSSEND_WRITE_DATA, "", SEQMONARCH_DEFAULT_TIMEOUT,
      fCmdBuild_SQNSSEND_WRITE_DATA_MONARCH, fRspAnalyze_None
    },
    {CMD_AT_SOCKET_PROMPT,  "> ",        SEQMONARCH_SOCKET_PROMPT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_PING,        "+PING", SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_PING_MONARCH,       fRspAnalyze_PING_MONARCH},
    {
      CMD_AT_SQNDNSLKUP, "+SQNDNSLKUP", SEQMONARCH_DEFAULT_TIMEOUT,
      fCmdBuild_SQNDNSLKUP_MONARCH, fRspAnalyze_SQNDNSLKUP_MONARCH
    },
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

    /* MODEM SPECIFIC EVENTS */
    {CMD_AT_WAIT_EVENT,     "",          SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_SYSSTART_TYPE1, "+SYSSTART", SEQMONARCH_SYSSTART_TIMEOUT, fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_SYSSTART_TYPE2, "^SYSSTART", SEQMONARCH_SYSSTART_TIMEOUT, fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_SYSSHDN,        "+SYSSHDN",  SEQMONARCH_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    /* */
  };
#define SIZE_ATCMD_SEQMONARCH_LUT ((uint16_t) (sizeof (ATCMD_SEQMONARCH_LUT) / sizeof (atcustom_LUT_t)))
  /* common init */
  monarch_modem_init(&SEQMONARCH_ctxt);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  SEQMONARCH_ctxt.modem_LUT_size = SIZE_ATCMD_SEQMONARCH_LUT;
  SEQMONARCH_ctxt.p_modem_LUT = (const atcustom_LUT_t *)ATCMD_SEQMONARCH_LUT;

  /* set default termination char for AT command: <CR> */
  (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->endstr, "\r");
  /* ###########################  END CUSTOMIZATION PART  ########################### */
}

/**
  * @brief  Check if message received is complete.
  * @param  rxChar Character received from modem.
  * @retval uint8_t Returns 1 if message is complete, else returns 0.
  */
uint8_t ATCustom_MONARCH_checkEndOfMsgCallback(uint8_t rxChar)
{
  uint8_t last_char = 0U;

  /*---------------------------------------------------------------------------------------*/
  if (SEQMONARCH_ctxt.state_SyntaxAutomaton == WAITING_FOR_INIT_CR)
  {
    /* waiting for first valid <CR>, char received before are considered as trash */
    if ((AT_CHAR_t)('\r') == rxChar)  /* 0x0A */
    {
      /* current     : xxxxx<CR>
      *  command format : <CR><LF>xxxxxxxx<CR><LF>
      *  waiting for : <LF>
      */
      SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (SEQMONARCH_ctxt.state_SyntaxAutomaton == WAITING_FOR_CR)
  {
    if ((AT_CHAR_t)('\r') == rxChar) /* 0x0A */
    {
      /* current     : xxxxx<CR>
      *  command format : <CR><LF>xxxxxxxx<CR><LF>
      *  waiting for : <LF>
      */
      SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (SEQMONARCH_ctxt.state_SyntaxAutomaton == WAITING_FOR_LF)
  {
    /* waiting for <LF> */
    if ((AT_CHAR_t)('\n') == rxChar)  /* 0x0D */
    {
      /*  current        : <CR><LF>
      *   command format : <CR><LF>xxxxxxxx<CR><LF>
      *   waiting for    : x or <CR>
      */
      SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_FIRST_CHAR;
      last_char = 1U;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (SEQMONARCH_ctxt.state_SyntaxAutomaton == WAITING_FOR_FIRST_CHAR)
  {
    if (SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_waiting_header)
    {
      /* Socket Data RX - waiting for Header: we are waiting for first <CR>
      *
      * <CR><LF>+SQNSRECV: 1,522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF>...
      *  ^- waiting this <CR>
      */
      if ((AT_CHAR_t)('\r') == rxChar)
      {
        /* first <CR> detected, next step */
        socketHeaderRX_reset();
        SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_header;
        SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received = 0U;
      }
    }
    else if (SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_header)
    {
      /* Socket Data RX - Header received: we are waiting for second <CR>
      *
      * <CR><LF>+SQNSRECV: 1,522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF>...
      *                          ^- waiting this <CR>
      */
      if ((AT_CHAR_t)('\r') == rxChar)
      {
        /* second <CR> detected, we have received a message.
         * Verify that this is +SQNSRECV header and, if it is the case, prepare to receive data.
         * Otherwise, this is an unexpected message. Forward it then wait again for +SQNSRECV header.
         */
        if (SocketHeaderRX_valid_header() == AT_TRUE)
        {
          /* valid +SQSNRECV header received. Now wait for <LF> then will start to receive data */
          SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
          SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_data;
        }
        else
        {
          /* received message is not the +SQSNRECV header. Wait for <LF> then wait again
           * for +SQSNRECV header.
           */
          SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
          SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_waiting_header;
        }
      }
      else
      {
        /* treament of received character */
        if (SocketHeaderRX_analyze_current_header(rxChar) == AT_FALSE)
        {
          /* received message is not the +SQSNRECV header. Go back to normal case, then wait again
           * for +SQSNRECV header.
           */
          SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
          SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_waiting_header;
        }
        else
        {
          /* no problem detected, continue to receive header */
        }

      }
    }
    else if (SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_data)
    {
      /* receiving socket data: do not analyze char, just count expected size
      *
      * <CR><LF>+SQNSRECV: 1,522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF>...
      *.                                ^- start to read data: count char
      */
      SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_SOCKET_DATA;
      SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
      /* check if full buffer has been received
       * (special case if buffer size = 1)
       */
      if (SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received ==
          SEQMONARCH_ctxt.socket_ctxt.socket_rx_expected_buf_size)
      {
        SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
        SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
      }
    }
    else if ((AT_CHAR_t)('\r') == rxChar)
    {
      /* Normal case: receiving a message
      *   current        : <CR>
      *   command format : <CR><LF>xxxxxxxx<CR><LF>
      *   waiting for    : <LF>
      */
      SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
    else {/* nothing to do */ }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (SEQMONARCH_ctxt.state_SyntaxAutomaton == WAITING_FOR_SOCKET_DATA)
  {
    SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
    /* check if full buffer has been received */
    if (SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received ==
        SEQMONARCH_ctxt.socket_ctxt.socket_rx_expected_buf_size)
    {
      SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
      SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else
  {
    /* should not happen */
  }

  /* ###########################  START CUSTOMIZATION PART  ######################### */
  /* if modem does not use standard syntax or has some specificities, replace previous
  *  function by a custom function
  */
  if (last_char == 0U)
  {
    /* MONARCH special cases
    *
    *  SOCKET MODE: when sending DATA using AT+SQNSSENDEXT, we are waiting for socket prompt "<CR><LF>> "
    *               before to send DATA. Then we should receive "OK<CR><LF>".
    */

    if (SEQMONARCH_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
    {
      switch (SEQMONARCH_ctxt.socket_ctxt.socket_send_state)
      {
        case SocketSendState_WaitingPrompt1st_greaterthan:
        {
          /* detecting socket prompt first char: "greater than" */
          if ((AT_CHAR_t)('>') == rxChar)
          {
            SEQMONARCH_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt2nd_space;
          }
          break;
        }

        case SocketSendState_WaitingPrompt2nd_space:
        {
          /* detecting socket prompt second char: "space" */
          if ((AT_CHAR_t)(' ') == rxChar)
          {
            SEQMONARCH_ctxt.socket_ctxt.socket_send_state = SocketSendState_Prompt_Received;
            last_char = 1U;
          }
          else
          {
            /* if char iommediatly after "greater than" is not a "space", reinit state */
            SEQMONARCH_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
          }
          break;
        }

        default:
          break;
      }
    }
  }

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (last_char);
}

/**
  * @brief  Returns the next AT command to send in the current context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_ATcmdTimeout Pointer to timeout value allocated for this command.
  * @retval at_status_t
  */
at_status_t ATCustom_MONARCH_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout)
{
  /********************************************************************
  Modem Model Selection
    *********************************************************************/
  at_status_t retval;
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);

  PRINT_API("enter ATCustom_MONARCH_getCmd() for SID %d", p_atp_ctxt->current_SID)

  /* retrieve parameters from SID command (will update SID_ctxt) */
  retval = atcm_retrieve_SID_parameters(&SEQMONARCH_ctxt, p_atp_ctxt);

  if (retval == ATSTATUS_OK)
  {
    /* new command: reset command context */
    atcm_reset_CMD_context(&SEQMONARCH_ctxt.CMD_ctxt);

    /* For each SID, the sequence of AT commands to send id defined (it can be dynamic)
      * Determine and prepare the next command to send for this SID
      */
    retval = at_custom_SID_monarch(&SEQMONARCH_ctxt, p_at_ctxt, p_ATcmdTimeout);

    /* if no error, build the command to send */
    if (retval == ATSTATUS_OK)
    {
      retval = atcm_modem_build_cmd(&SEQMONARCH_ctxt, p_atp_ctxt, p_ATcmdTimeout);
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
at_endmsg_t ATCustom_MONARCH_extractElement(atparser_context_t *p_atp_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos)
{
#if (USE_SOCKETS_TYPE != USE_SOCKETS_MODEM)
  UNUSED(p_atp_ctxt);
#endif /* (USE_SOCKETS_TYPE != USE_SOCKETS_MODEM) */

  at_endmsg_t retval_msg_end_detected = ATENDMSG_NO;
  bool exit_loop;
  uint16_t *p_parseIndex = &(element_infos->current_parse_idx);

  PRINT_API("enter ATCustom_MONARCH_extractElement()")
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

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    exit_loop = false;
    for (uint16_t idx = *p_parseIndex; (idx < (p_msg_in->size - 1U)) && (exit_loop == false); idx++)
    {
      if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SQNSRECV) &&
          (SEQMONARCH_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Payload) &&
          (SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state != SocketRxDataState_finished))
      {
        element_infos->str_start_idx = 0U;
        element_infos->str_end_idx = (uint16_t) SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        element_infos->str_size = (uint16_t) SEQMONARCH_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        SEQMONARCH_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_finished;
        retval_msg_end_detected = ATENDMSG_YES;
        exit_loop = true;
      }
      else if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SQNSS)
      {
        /* SPECIAL CASE:
          * SNQNSS without <CR><LF> sequence has been found, it is a command line
          */
        PRINT_DBG("SQNSS command detected")
        *p_parseIndex = 0U;
        exit_loop = true;
      }
      else { /* nothing to do */ }
    }

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

    /* ###########################  END CUSTOMIZATION PART  ########################### */
  }

  /* check if end of message has been detected */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  if (retval_msg_end_detected != ATENDMSG_YES)
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
  {
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
        switch (p_msg_in->buffer[*p_parseIndex])
        {
          /* ###########################  START CUSTOMIZATION PART  ########################### */
          /* ----- test separators ----- */
          case ':':
          case ',':
            exit_loop = true;
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
        };
      } while (exit_loop == false);

      /* increase parameter rank */
      element_infos->param_rank = (element_infos->param_rank + 1U);
    }
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
at_action_rsp_t ATCustom_MONARCH_analyzeCmd(at_context_t *p_at_ctxt,
                                            const IPC_RxMessage_t *p_msg_in,
                                            at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval;

  PRINT_API("enter ATCustom_MONARCH_analyzeCmd()")

  /* Analyze data received from the modem and
    * search in LUT the ID corresponding to command received
    */
  if (ATSTATUS_OK != atcm_searchCmdInLUT(&SEQMONARCH_ctxt, p_atp_ctxt, p_msg_in, element_infos))
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
    retval = atcm_check_text_line_cmd(&SEQMONARCH_ctxt, p_at_ctxt, p_msg_in, element_infos);

    /* 2nd STEP: search in specific modems commands if not found at 1st STEP
      *
      * This is the case in socket mode when receiving data.
      * The command is decomposed in 2 lines:
      * The 1st part of the response is analyzed by atcm_searchCmdInLUT:
      *   <CR><LF>+SQNSRECV: 1,522<CR><LF>
      * The 2nd part of the response, corresponding to the data, falls here:
      *   HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
      */
    if (retval == ATACTION_RSP_NO_ACTION)
    {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      switch (p_atp_ctxt->current_atcmd.id)
      {
        /* ###########################  START CUSTOMIZED PART  ########################### */
        case CMD_AT_SQNSRECV:
          /* receive data in SQNSRECV command */
          if (fRspAnalyze_SQNSRECV_data_MONARCH(p_at_ctxt, &SEQMONARCH_ctxt, p_msg_in, element_infos)
              != ATACTION_RSP_ERROR)
          {
            /* received a valid intermediate answer */
            retval = ATACTION_RSP_INTERMEDIATE;
          }
          break;

        /* ###########################  END CUSTOMIZED PART  ########################### */
        default:
          /* this is not one of modem common command, need to check if this is an answer to a modem's specific cmd */
          PRINT_ERR("receive an un-expected line... is it a text line ?")
          retval = ATACTION_RSP_IGNORED;
          break;
      }
#else
      __NOP();
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
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
        else if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON)
                 || (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
        {
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT)
          {
            /* modem is synchronized */
            SEQMONARCH_ctxt.persist.modem_at_ready = AT_TRUE;
          }
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATE)
          {
            PRINT_DBG("Echo successfully disabled")
          }
        }
        else
        {
          /* nothing to do */
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

      case CMD_AT_SYSSTART_TYPE1:
      case CMD_AT_SYSSTART_TYPE2:
        /* We received SYSSTART event from the modem.
          * If received during Power ON or RESET, it is indicating that the modem is ready.
          * If received in another state, we report to upper layer a modem reboot event.
          */

        /* modem is ready */
        SEQMONARCH_ctxt.persist.modem_at_ready = AT_TRUE;

        if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON) ||
            (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
        {
          PRINT_DBG("continue INIT modem sequence  (final = %d)", p_atp_ctxt->is_final_cmd)
          SEQMONARCH_ctxt.persist.modem_at_ready = AT_FALSE;
          /* UNLOCK the WAIT EVENT */
          retval = ATACTION_RSP_FRC_END;
        }
        else
        {
          /* if event is received in other states, it's an unexpected modem reboot */
          if (atcm_modem_event_received(&SEQMONARCH_ctxt, CS_MDMEVENT_BOOT) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        break;

      case CMD_AT_SHUTDOWN:
      case CMD_AT_SQNSSHDN:
        /* if we were waiting for this event, we can continue the sequence */
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_OFF)
        {
          PRINT_DBG("modem shutdown confirmation received")
          /* UNLOCK the WAIT EVENT */
          retval = ATACTION_RSP_FRC_END;
        }
        else
        {
          /* unexpected modem POWER DOWN EVENT DETECTED
            * forward it to upper layer if it has subscribed to this event
            */
          if (atcm_modem_event_received(&SEQMONARCH_ctxt, CS_MDMEVENT_POWER_DOWN) == AT_TRUE)
          {
            retval = ATACTION_RSP_URC_FORWARDED;
          }
          else
          {
            retval = ATACTION_RSP_URC_IGNORED;
          }
        }
        break;

      case CMD_AT_SYSSHDN:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_CGEV:
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case CMD_AT_SMST:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      case CMD_AT_SOCKET_PROMPT:
        PRINT_INFO(" SOCKET PROMPT RECEIVED")
        /* if we were waiting for this event, we can continue the sequence */
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SEND_DATA)
        {
          /* UNLOCK the WAIT EVENT */
          retval = ATACTION_RSP_FRC_END;
        }
        else
        {
          retval = ATACTION_RSP_URC_IGNORED;
        }
        break;

      case CMD_AT_SQNSH:
        /* check if response received corresponds to the command we have send
          *  if not => this is an URC
          */
        if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
        {
          PRINT_INFO("+SQNSH intermediate")
          retval = ATACTION_RSP_INTERMEDIATE;
        }
        else
        {
          PRINT_INFO("+SQNSH URC")
          retval = ATACTION_RSP_URC_FORWARDED;
        }
        break;

      case CMD_AT_SQNSRING:
        /* this is an URC, socket RING (format depends on the last +SQNSCFGEXT setting)  */
        retval = ATACTION_RSP_URC_FORWARDED;
        break;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

      /* ###########################  END CUSTOMIZATION PART  ########################### */

      case CMD_AT:
        retval = ATACTION_RSP_IGNORED;
        break;

      case CMD_AT_INVALID:
        retval = ATACTION_RSP_ERROR;
        break;

      case CMD_AT_ERROR:
        /* ERROR does not contains parameters, call the analyze function explicitly */
        retval = fRspAnalyze_Error_MONARCH(p_at_ctxt, &SEQMONARCH_ctxt, p_msg_in, element_infos);
        break;

      case CMD_AT_CME_ERROR:
      case CMD_AT_CMS_ERROR:
        /* do the analyze here because will not be called by the parser */
        retval = fRspAnalyze_Error_MONARCH(p_at_ctxt, &SEQMONARCH_ctxt, p_msg_in, element_infos);
        break;

      default:
        /* check if response received corresponds to the command we have send
          *  if not => this is an ERROR
          */
        if (element_infos->cmd_id_received == p_atp_ctxt->current_atcmd.id)
        {
          retval = ATACTION_RSP_INTERMEDIATE;
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
at_action_rsp_t ATCustom_MONARCH_analyzeParam(at_context_t *p_at_ctxt,
                                              const IPC_RxMessage_t *p_msg_in,
                                              at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PRINT_API("enter ATCustom_MONARCH_analyzeParam()")

  /* analyse parameters of the command we received:
    * call the corresponding function from the LUT
    */
  retval = (atcm_get_CmdAnalyzeFunc(&SEQMONARCH_ctxt, element_infos->cmd_id_received))(p_at_ctxt,
           &SEQMONARCH_ctxt,
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
at_action_rsp_t ATCustom_MONARCH_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter ATCustom_MONARCH_terminateCmd()")

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  if (SEQMONARCH_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
  {
    /* special case for SID_CS_SEND_DATA
      * indeed, this function is called when an AT cmd is finished
      * but for AT+SQNSSENDEXT, it is called a 1st time when prompt is received
      * and a second time when data have been sent.
      */
    if (p_atp_ctxt->current_SID != (at_msg_t) SID_CS_SEND_DATA)
    {
      /* reset socket_send_state */
      SEQMONARCH_ctxt.socket_ctxt.socket_send_state = SocketSendState_No_Activity;
    }
  }

  /* additional tests */
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

  /* test +SMST result
    * note: works also if AT+SMST sent through direct command
    */
  if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_SMST) ||
      (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_DIRECT_CMD))
  {
    if (monarch_shared.SMST_sim_error_status != 0U)
    {
      monarch_shared.SMST_sim_error_status = 0U;
      retval = ATACTION_RSP_ERROR;
    }
  }
#if (GM01Q_ACTIVATE_PING_REPORT == 0)
  if (p_atp_ctxt->current_atcmd.id == CMD_AT_PING)
  {
    PRINT_ERR("PING not supported actually, force an ERROR !!!")
    retval = ATACTION_RSP_ERROR;
  }
#endif /* (GM01Q_ACTIVATE_PING_REPORT == 0) */

  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/**
  * @brief  Returns a buffer containing the response to send to upper layer depending of current context.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_rsp_buf Pointer to buffer with the response to send.
  * @retval at_status_t.
  */
at_status_t ATCustom_MONARCH_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_MONARCH_get_rsp()")

  /* prepare response for a SID - common part */
  retval = atcm_modem_get_rsp(&SEQMONARCH_ctxt, p_atp_ctxt, p_rsp_buf);

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
                               (uint16_t) sizeof(monarch_shared.SQNDNSLKUP_dns_info.hostIPaddr),
                               (void *)monarch_shared.SQNDNSLKUP_dns_info.hostIPaddr) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;

    case SID_CS_POWER_OFF:
      /* reinit context for power off case */
      ATC_Monarch_modem_reset(&SEQMONARCH_ctxt);
      break;

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#if (GM01Q_ACTIVATE_PING_REPORT == 1)
    case SID_CS_PING_IP_ADDRESS:
    {
      /* PING responses from the modem are synchronous for this modem.
        * It is not possible to trigger an URC now, so update the PING report now.
        * Final reports (needed by upper layer) will be send by cellular service.
        *  note: only the last PING received will be sent in this case.
        */
      PRINT_INFO("Ping final report")

      /* prepare response */
      clear_ping_resp_struct(&SEQMONARCH_ctxt);
      SEQMONARCH_ctxt.persist.ping_resp_urc.ping_status = CELLULAR_OK;
      /* simulate final report data */
      SEQMONARCH_ctxt.persist.ping_resp_urc.is_final_report = CELLULAR_TRUE;
      /* index expected by COM  for final report = number of pings requested + 1 */
      SEQMONARCH_ctxt.persist.ping_resp_urc.index = SEQMONARCH_ctxt.persist.ping_infos.ping_params.pingnum + 1U;
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_URC_PING_RSP,
                               (uint16_t) sizeof(CS_Ping_response_t),
                               (void *)&SEQMONARCH_ctxt.persist.ping_resp_urc) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;
    }
#endif /* (GM01Q_ACTIVATE_PING_REPORT == 1) */
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

    default:
      break;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  /* reset SID context */
  atcm_reset_SID_context(&SEQMONARCH_ctxt.SID_ctxt);

  /* reset socket context */
  atcm_reset_SOCKET_context(&SEQMONARCH_ctxt);

  return (retval);
}

/**
  * @brief  Returns a buffer containing the URC received.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_rsp_buf Pointer to buffer with the response to send.
  * @retval at_status_t.
  */
at_status_t ATCustom_MONARCH_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_MONARCH_get_urc()")

  /* prepare response for an URC - common part */
  retval = atcm_modem_get_urc(&SEQMONARCH_ctxt, p_atp_ctxt, p_rsp_buf);

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
at_status_t ATCustom_MONARCH_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_MONARCH_get_error()")

  /* prepare response when an error occurred - common part */
  retval = atcm_modem_get_error(&SEQMONARCH_ctxt, p_atp_ctxt, p_rsp_buf);

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
at_status_t ATCustom_MONARCH_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate)
{
  UNUSED(gstate);
  at_status_t retval = ATSTATUS_ERROR;
  /* IMPORTANT: Do not add traces int this function of in functions called
    * (this function called under interrupt if GPIO event)
    */

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  UNUSED(deviceType);
  UNUSED(hwEvent);
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/**
  * @brief  Global reset of modem parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none.
  */
void ATC_Monarch_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter monarch_modem_reset")

  /* common reset function (reset all contexts except SID) */
  atcm_modem_reset(p_modem_ctxt);

  /* modem specific actions if any */
}

/**
  * @brief  Reset modem shared parameters.
  * @retval none.
  */
void ATC_Monarch_reset_variables(void)
{
  /* Set default values of MONARCH specific variables after SWITCH ON or RESET */
  /* add default variables value if needed */
  monarch_shared.SMST_sim_error_status = 0U;

  /* RxSQNSRECV_header_info structure */
  socketHeaderRX_reset();
}

/**
  * @brief  Reinitialize state of Syntax Automaton used to manage characters received from modem.
  * @retval none.
  */
void ATC_Monarch_reinitSyntaxAutomaton_monarch(void)
{
  SEQMONARCH_ctxt.state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

/**
  * @brief  Initialization of modem low power parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_bool_t Returns true if Low Power is enabled.
  */
at_bool_t ATC_Monarch_init_monarch_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  UNUSED(p_modem_ctxt);

  at_bool_t lp_enabled;

  if (SEQMONARCH_ctxt.SID_ctxt.init_power_config.low_power_enable == CELLULAR_TRUE)
  {
    /* enable PSM in CGREG/CEREG (value = 4) */
    SEQMONARCH_ctxt.persist.psm_urc_requested = AT_TRUE;

    /* send PSM and EDRX commands: need to populate SID_ctxt.set_power_config
     * Provide psm and edrx default parameters provided but disable them for the moment
     */
    SEQMONARCH_ctxt.SID_ctxt.set_power_config.psm_present = CELLULAR_TRUE;
#if (ENABLE_MONARCH_PSM == 1U)
    SEQMONARCH_ctxt.SID_ctxt.set_power_config.psm_mode = PSM_MODE_ENABLE;
#else
    SEQMONARCH_ctxt.SID_ctxt.set_power_config.psm_mode = PSM_MODE_DISABLE;
#endif /* (ENABLE_MONARCH_PSM == 1U) */
    (void) memcpy((void *) &SEQMONARCH_ctxt.SID_ctxt.set_power_config.psm,
                  (void *) &SEQMONARCH_ctxt.SID_ctxt.init_power_config.psm,
                  sizeof(CS_PSM_params_t));

    /* EDRX not implemented yet */
    SEQMONARCH_ctxt.SID_ctxt.set_power_config.edrx_present = CELLULAR_TRUE;
    SEQMONARCH_ctxt.SID_ctxt.set_power_config.edrx_mode = EDRX_MODE_DISABLE;
    (void) memcpy((void *) &SEQMONARCH_ctxt.SID_ctxt.set_power_config.edrx,
                  (void *) &SEQMONARCH_ctxt.SID_ctxt.init_power_config.edrx,
                  sizeof(CS_EDRX_params_t));

    lp_enabled = AT_TRUE;

  }
  else
  {
    /* disable PSM in CGREG/CEREG (value = 2) */
    SEQMONARCH_ctxt.persist.psm_urc_requested = AT_FALSE;

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
at_bool_t ATC_Monarch_set_monarch_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  at_bool_t lp_set_and_enabled;

  if (p_modem_ctxt->SID_ctxt.set_power_config.psm_present == CELLULAR_TRUE)
  {
    /* the modem info structure SID_ctxt.set_power_config has been already updated */

    /* PSM parameters are present */
    if (p_modem_ctxt->SID_ctxt.set_power_config.psm_mode == PSM_MODE_ENABLE)
    {
#if (ENABLE_MONARCH_PSM == 1U)
      /* PSM is enabled */
      p_modem_ctxt->persist.psm_urc_requested = AT_TRUE;
      lp_set_and_enabled = AT_TRUE;
#else
      /* PSM is enabled by user but not activated */
      p_modem_ctxt->persist.psm_urc_requested = AT_FALSE;
      lp_set_and_enabled = AT_FALSE;
#endif /* (ENABLE_MONARCH_PSM == 1U) */
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

    /* EDRX not implemented yet, parameters ignored */
  }
  else
  {
    /* PSM parameters are not present */
    lp_set_and_enabled = AT_FALSE;
  }

  return (lp_set_and_enabled);
}
/**
  * @}
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SPECIFIC_Private_Functions  AT_CUSTOM SEQUANS_MONARCH SPECIFIC Private Functions
  * @{
  */

/**
  * @brief  Initialization of modem parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none.
  */
static void monarch_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter monarch_modem_init")

  /* common init function (reset all contexts) */
  atcm_modem_init(p_modem_ctxt);

  /* modem specific actions if any */
}

/**
  * @brief  Reset header structure for RX socket data
  * @retval none.
  */
static void socketHeaderRX_reset(void)
{
  ATCustom_MONARCH_RxSQNSRECV_header_t *p_header = &(monarch_shared.RxSQNSRECV_header_info);

  p_header->analyze_state = RX_SQNSRECV_header_prefix;
  p_header->counter_header = 0U;
  p_header->counter_maxByte = 0U;
  (void) memset((void *)p_header->buf_maxByte, 0, MAXBYTE_MAXIMUM_SIZE);
}

/**
  * @brief  Analyze current header for RX socket
  * @param  rxchar Character received.
  * @retval at_bool_t Returns true if header received is +SQNSRECV.
  */
static at_bool_t SocketHeaderRX_analyze_current_header(uint8_t rxchar)
{
  static const uint8_t SQNSRECV_header_prefix[] = "+SQNSRECV";
#define SQNSRECV_HEADER_PREFIX_SIZE  ((uint8_t) ((sizeof (SQNSRECV_header_prefix) / sizeof (uint8_t)) - 1U))

  /* We are expecting to receive: +SQNSRECV: 1,522<CR><LF>
  * Analyze the header goals:
  *  1- check that message received is +SQNSRECV as expected
  *  2- if this is the case, check that data size (522 in this example) and
  *     update socket_rx_expected_buf_size if needed.
  *
  * This function returns AT_FALSE if we detect that it's not +SQNSRECV
  */
  at_bool_t retval = AT_TRUE;
  ATCustom_MONARCH_RxSQNSRECV_header_t *p_header = &(monarch_shared.RxSQNSRECV_header_info);

  if (p_header->analyze_state == RX_SQNSRECV_header_prefix)
  {
    if (rxchar == SQNSRECV_header_prefix[p_header->counter_header])
    {
      /* receiving header */
      p_header->counter_header++;
      if (p_header->counter_header == SQNSRECV_HEADER_PREFIX_SIZE)
      {
        /* full prefix received, wait for connId parameter */
        p_header->analyze_state = RX_SQNSRECV_header_connId;
      }
    }
    else
    {
      /* this is not +SQNSRECV:
       * special trick: only consider that it's not the good header if counter_header has
       * started (for the case we receive "trash" characters before the '+').
       * Anyway, if a <CR> character is received, it will be treated in calling function.
       * */
      if (p_header->counter_header >= 1U)
      {
        p_header->analyze_state = RX_SQNSRECV_header_not_started;
        retval = AT_FALSE;
      }
    }
  }
  else if (p_header->analyze_state == RX_SQNSRECV_header_connId)
  {
    /* Receiving connId parameter.
     * Ignore parameter value, just wait for the separator with next parameter
     */
    if (rxchar == (AT_CHAR_t)(','))
    {
      p_header->analyze_state = RX_SQNSRECV_header_maxByte;
    }
  }
  else if (p_header->analyze_state == RX_SQNSRECV_header_maxByte)
  {
    /* Receiving maxByte parameter.
     * This is the parameter we need to know the size of data which are following.
     */
    if (rxchar == (AT_CHAR_t)(','))
    {
      /* in case of additional parameters, receiving a comma indicates the end of maxByte parameter
       */
      p_header->analyze_state = RX_SQNSRECV_header_other;
    }
    else if ((rxchar >= (AT_CHAR_t)('0')) && (rxchar <= (AT_CHAR_t)('9')))
    {
      /* receiving maxByte parameter.
       *  maxBye will be complete if :
       *     - we receive ','
       *     - we receive <CR>
       */
      if (SocketHeaderRX_update_maxByte(rxchar) == AT_FALSE)
      {
        /* an error occurred */
        p_header->analyze_state = RX_SQNSRECV_header_not_started;
        retval = AT_FALSE;
      }
    }
    else
    {
      /* error invalid character received for maxByte */
      p_header->analyze_state = RX_SQNSRECV_header_not_started;
      retval = AT_FALSE;
    }
  }
  else
  {
    /* other cases: nothing to do, waiting for next <CR> */
  }

  return (retval);
}

/**
  * @brief  Update size of received header.
  * @param  rxchar Character received.
  * @retval at_bool_t Returns false if an error occurred.
  */
static at_bool_t SocketHeaderRX_update_maxByte(uint8_t rxchar)
{
  at_bool_t retval = AT_TRUE;
  ATCustom_MONARCH_RxSQNSRECV_header_t *p_header = &(monarch_shared.RxSQNSRECV_header_info);

  if (p_header->counter_maxByte < MAXBYTE_MAXIMUM_SIZE)
  {
    p_header->buf_maxByte[p_header->counter_maxByte] = rxchar;
    p_header->counter_maxByte++;
  }
  else
  {
    /* error, exceed maxByte buffer size*/
    retval = AT_FALSE;
  }

  return (retval);
}

/**
  * @brief  Check if received header is valid.
  * @param  rxchar Character received.
  * @retval at_bool_t Returns true if header is valid.
  */
static at_bool_t SocketHeaderRX_valid_header(void)
{
  at_bool_t retval = AT_FALSE;
  ATCustom_MONARCH_RxSQNSRECV_header_t *p_header = &(monarch_shared.RxSQNSRECV_header_info);

  /* check maxByte parameter validity */
  if (p_header->counter_maxByte > 0U)
  {
    uint16_t data_size_from_header =
      (uint16_t) ATutil_convertStringToInt(
        (uint8_t *)p_header->buf_maxByte,
        (uint16_t)p_header->counter_maxByte);

    if (SEQMONARCH_ctxt.socket_ctxt.socket_rx_expected_buf_size != data_size_from_header)
    {
      /* update buffer size received - should not happen */
      SEQMONARCH_ctxt.socket_ctxt.socket_rx_expected_buf_size = data_size_from_header;
    }
    retval = AT_TRUE;
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

