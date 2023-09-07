/**
  ******************************************************************************
  * @file    at_custom_modem_specific.c
  * @author  MCD Application Team
  * @brief   This file provides all the specific code to the
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

/* AT commands format
 * AT+<X>=?    : TEST COMMAND
 * AT+<X>?     : READ COMMAND
 * AT+<X>=...  : WRITE COMMAND
 * AT+<X>      : EXECUTION COMMAND
*/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "at_modem_api.h"
#include "at_modem_common.h"
#include "at_modem_signalling.h"
#include "at_modem_socket.h"
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
#include "at_custom_modem_socket.h"
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC AT_CUSTOM QUECTEL_BG96 SPECIFIC
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC_Private_Macros AT_CUSTOM QUECTEL_BG96 SPECIFIC Private Macros
  * @{
  */
#if (USE_TRACE_ATCUSTOM_SPECIFIC == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_INFO(format, args...) TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P0, "BG96:" format "\n\r", ## args)
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "BG96:" format "\n\r", ## args)
#define PRINT_API(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P2, "BG96 API:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "BG96 ERROR:" format "\n\r", ## args)
#else
#define PRINT_INFO(format, args...)  (void) printf("BG96:" format "\n\r", ## args);
#define PRINT_DBG(...) __NOP(); /* Nothing to do */
#define PRINT_API(...) __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("BG96 ERROR:" format "\n\r", ## args);
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


/** @defgroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC_Private_Variables AT_CUSTOM QUECTEL_BG96 SPECIFIC Private Variables
  * @{
  */
static atcustom_modem_context_t BG96_ctxt;

/* Socket Data receive: to analyze size received in data header */
static AT_CHAR_t SocketHeaderDataRx_Buf[4];
static uint8_t SocketHeaderDataRx_Cpt;
static uint8_t SocketHeaderDataRx_Cpt_Complete;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC_Exported_Variables AT_CUSTOM QUECTEL_BG96 SPECIFIC Exported Variables
  * @{
  */
/* shared variables specific to BG96 */
bg96_shared_variables_t bg96_shared =
{
  .QCFG_command_param              = QCFG_unknown,
  .QCFG_command_write              = AT_FALSE,
  .QINDCFG_command_param           = QINDCFG_unknown,
  .QICGSP_config_command           = AT_TRUE,
  .bg96_sim_status_retries         = 0U,
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  .pdn_already_active              = false,
#endif /* USE_SOCKETS_TYPE */
#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
  .host_lp_state                   = HOST_LP_STATE_IDLE,
  .modem_lp_state                  = MDM_LP_STATE_IDLE,
  .modem_resume_from_PSM           = false,
#endif /* (ENABLE_BG96_LOW_POWER_MODE == 1U) */
};

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC_Private_Functions_Prototypes
  *    AT_CUSTOM QUECTEL_BG96 SPECIFIC Private Functions Prototypes
  * @{
  */
static void bg96_modem_init(atcustom_modem_context_t *p_modem_ctxt);

static void socketHeaderRX_reset(void);
static void SocketHeaderRX_addChar(CRC_CHAR_t *rxchar);
static uint16_t SocketHeaderRX_getSize(void);

static void display_decoded_GSM_bands(uint32_t gsm_bands);
static void display_decoded_CatM1_bands(uint32_t CatM1_bands_MsbPart, uint32_t CatM1_bands_LsbPart);
static void display_decoded_CatNB1_bands(uint32_t CatNB1_bands_MsbPart, uint32_t CatNB1_bands_LsbPart);
static void display_user_friendly_mode_and_bands_config(void);
static uint8_t display_if_active_band(ATCustom_BG96_QCFGscanseq_t scanseq,
                                      uint8_t rank, uint8_t catM1_on, uint8_t catNB1_on, uint8_t gsm_on);

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC_Exported_Functions AT_CUSTOM QUECTEL_BG96 SPECIFIC Exported Functions
  * @{
  */

/**
  * @brief  Initialize specific AT commands.
  * @param  p_atp_ctxt Pointer to the structure of Parser context.
  * @retval none
  */
void ATCustom_BG96_init(atparser_context_t *p_atp_ctxt)
{
  /* Commands Look-up table */
  static const atcustom_LUT_t ATCMD_BG96_LUT[] =
  {
    /* cmd enum - cmd string - cmd timeout (in ms) - build cmd ftion - analyze cmd ftion */
    {CMD_AT,             "",             BG96_AT_TIMEOUT,       fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_OK,          "OK",           BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CONNECT,     "CONNECT",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_RING,        "RING",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_CARRIER,  "NO CARRIER",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_ERROR,       "ERROR",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_BG96},
    {CMD_AT_NO_DIALTONE, "NO DIALTONE",  BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_BUSY,        "BUSY",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_NO_ANSWER,   "NO ANSWER",    BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CME_ERROR,   "+CME ERROR",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_Error_BG96},
    {CMD_AT_CMS_ERROR,   "+CMS ERROR",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CmsErr},

    /* GENERIC MODEM commands */
    {CMD_AT_CGMI,        "+CGMI",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMI},
    {CMD_AT_CGMM,        "+CGMM",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMM},
    {CMD_AT_CGMR,        "+CGMR",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGMR},
    {CMD_AT_CGSN,        "+CGSN",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGSN_BG96,  fRspAnalyze_CGSN},
    {CMD_AT_GSN,         "+GSN",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_GSN},
    {CMD_AT_CIMI,        "+CIMI",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CIMI},
    {CMD_AT_CEER,        "+CEER",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CEER},
    {CMD_AT_CMEE,        "+CMEE",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CMEE,       fRspAnalyze_None},
    {CMD_AT_CPIN,        "+CPIN",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CPIN,       fRspAnalyze_CPIN_BG96},
    {CMD_AT_CFUN,        "+CFUN",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CFUN,       fRspAnalyze_CFUN_BG96},
    {CMD_AT_COPS,        "+COPS",        BG96_COPS_TIMEOUT,     fCmdBuild_COPS_BG96,  fRspAnalyze_COPS_BG96},
    {CMD_AT_CNUM,        "+CNUM",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CNUM},
    {CMD_AT_CGATT,       "+CGATT",       BG96_CGATT_TIMEOUT,    fCmdBuild_CGATT,      fRspAnalyze_CGATT},
    {CMD_AT_CGPADDR,     "+CGPADDR",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGPADDR,    fRspAnalyze_CGPADDR},
    {CMD_AT_CEREG,       "+CEREG",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_CEREG,      fRspAnalyze_CEREG},
    {CMD_AT_CREG,        "+CREG",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CREG,       fRspAnalyze_CREG},
    {CMD_AT_CGREG,       "+CGREG",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGREG,      fRspAnalyze_CGREG},
    {CMD_AT_CSQ,         "+CSQ",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CSQ},
    {CMD_AT_CGDCONT,     "+CGDCONT",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGDCONT_BG96,    fRspAnalyze_None},
    {CMD_AT_CGACT,       "+CGACT",       BG96_CGACT_TIMEOUT,    fCmdBuild_CGACT,      fRspAnalyze_None},
    {CMD_AT_CGDATA,      "+CGDATA",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGDATA,     fRspAnalyze_None},
    {CMD_AT_CGEREP,      "+CGEREP",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_CGEREP,     fRspAnalyze_None},
    {CMD_AT_CGEV,        "+CGEV",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_CGEV},
    {CMD_ATD,            "D",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATD_BG96,   fRspAnalyze_None},
    {CMD_ATE,            "E",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATE,        fRspAnalyze_None},
    {CMD_ATH,            "H",            BG96_ATH_TIMEOUT,      fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATO,            "O",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_ATV,            "V",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATV,        fRspAnalyze_None},
    {CMD_ATX,            "X",            BG96_DEFAULT_TIMEOUT,  fCmdBuild_ATX,        fRspAnalyze_None},
    {CMD_AT_ESC_CMD,     "+++",          BG96_ESCAPE_TIMEOUT,   fCmdBuild_ESCAPE_CMD, fRspAnalyze_None},
    {CMD_AT_IPR,         "+IPR",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_IPR,        fRspAnalyze_IPR},
    {CMD_AT_IFC,         "+IFC",         BG96_DEFAULT_TIMEOUT,  fCmdBuild_IFC,        fRspAnalyze_None},
    {CMD_AT_AND_W,       "&W",           BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_AND_D,       "&D",           BG96_DEFAULT_TIMEOUT,  fCmdBuild_AT_AND_D,   fRspAnalyze_None},
    {CMD_AT_DIRECT_CMD,  "",             BG96_DEFAULT_TIMEOUT,  fCmdBuild_DIRECT_CMD, fRspAnalyze_DIRECT_CMD},
    /* note: for CMD_AT_DIRECT_CMD, the default timeout value will be replaced by the timeout
     *       value given by the upper layer.
     */
    {CMD_AT_CSIM,        "+CSIM",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_CSIM,       fRspAnalyze_CSIM},

    /* MODEM SPECIFIC COMMANDS */
    {CMD_AT_QPOWD,       "+QPOWD",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_QPOWD_BG96, fRspAnalyze_None},
    {CMD_AT_QCFG,        "+QCFG",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_QCFG_BG96,  fRspAnalyze_QCFG_BG96},
    {CMD_AT_QIND,        "+QIND",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_QIND_BG96},
    {CMD_AT_SEND_OK,      "SEND OK",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_SEND_FAIL,    "SEND FAIL",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_QCCID,        "+QCCID",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QCCID_BG96},
    {CMD_AT_QICFG,       "+QICFG",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_QICFG_BG96, fRspAnalyze_None},
    {CMD_AT_QINISTAT,     "+QINISTAT",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QINISTAT_BG96},
    {CMD_AT_QCSQ,         "+QCSQ",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QCSQ_BG96},
    {CMD_AT_QUSIM,       "+QUSIM",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_CPSMS,       "+CPSMS",       BG96_CPSMS_TIMEOUT,    fCmdBuild_CPSMS,  fRspAnalyze_CPSMS},
    {CMD_AT_CEDRXS,      "+CEDRXS",      BG96_CEDRX_TIMEOUT,    fCmdBuild_CEDRXS, fRspAnalyze_CEDRXS},
    {CMD_AT_CEDRXP,      "+CEDRXP",      BG96_CEDRX_TIMEOUT,    fCmdBuild_NoParams,   fRspAnalyze_CEDRXP},
    {CMD_AT_CEDRXRDP,    "+CEDRXRDP",    BG96_CEDRX_TIMEOUT,    fCmdBuild_NoParams,   fRspAnalyze_CEDRXRDP},
    {CMD_AT_QNWINFO,     "+QNWINFO",     BG96_QNWINFO_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_None},
    {CMD_AT_QENG,        "+QENG",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_QENG_BG96, fRspAnalyze_None},
    {CMD_AT_QGMR,        "+QGMR",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QGMR_BG96},
    {CMD_AT_QPSMS,       "+QPSMS",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_None},
    {CMD_AT_QPSMCFG,     "+QPSMCFG",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_None},
    {CMD_AT_QPSMEXTCFG,  "+QPSMEXTCFG",  BG96_DEFAULT_TIMEOUT,  fCmdBuild_QPSMEXTCFG, fRspAnalyze_None},
    {CMD_AT_QURCCFG,     "+QURCCFG",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_QURCCFG_BG96, fRspAnalyze_None},
    {CMD_AT_QINDCFG,     "+QINDCFG",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_QINDCFG_BG96,  fRspAnalyze_QINDCFG_BG96},

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    /* MODEM SPECIFIC COMMANDS USED FOR SOCKET MODE */
    {CMD_AT_QIURC,       "+QIURC",       BG96_DEFAULT_TIMEOUT,  fCmdBuild_NoParams, fRspAnalyze_QIURC_BG96},
    {CMD_AT_QIACT,       "+QIACT",       BG96_QIACT_TIMEOUT,    fCmdBuild_QIACT_BG96,   fRspAnalyze_QIACT_BG96},
    {CMD_AT_QIDEACT,     "+QIDEACT",     BG96_QIDEACT_TIMEOUT,  fCmdBuild_QIDEACT_BG96,   fRspAnalyze_None},
    {CMD_AT_QIOPEN,      "+QIOPEN",      BG96_QIOPEN_TIMEOUT,   fCmdBuild_QIOPEN_BG96,  fRspAnalyze_QIOPEN_BG96},
    {CMD_AT_QICLOSE,     "+QICLOSE",     BG96_QICLOSE_TIMEOUT,  fCmdBuild_QICLOSE_BG96, fRspAnalyze_None},
    {CMD_AT_QISEND,      "+QISEND",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_QISEND_BG96,  fRspAnalyze_None},
    {CMD_AT_QISEND_WRITE_DATA,  "",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_QISEND_WRITE_DATA_BG96, fRspAnalyze_None},
    {CMD_AT_QIRD,        "+QIRD",        BG96_DEFAULT_TIMEOUT,  fCmdBuild_QIRD_BG96,    fRspAnalyze_QIRD_BG96},
    {CMD_AT_QISTATE,     "+QISTATE",     BG96_DEFAULT_TIMEOUT,  fCmdBuild_QISTATE_BG96, fRspAnalyze_QISTATE_BG96},
    {CMD_AT_QPING,        "+QPING",      BG96_QPING_TIMEOUT,    fCmdBuild_QPING_BG96,   fRspAnalyze_QPING_BG96},
    {CMD_AT_QIDNSCFG,     "+QIDNSCFG",   BG96_DEFAULT_TIMEOUT,  fCmdBuild_QIDNSCFG_BG96, fRspAnalyze_None},
    {CMD_AT_QIDNSGIP,     "+QIDNSGIP",   BG96_QIDNSGIP_TIMEOUT, fCmdBuild_QIDNSGIP_BG96, fRspAnalyze_None},
    {CMD_AT_SOCKET_PROMPT, "> ",         BG96_SOCKET_PROMPT_TIMEOUT,  fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_QICSGP,      "+QICSGP",      BG96_DEFAULT_TIMEOUT,  fCmdBuild_QICSGP_BG96, fRspAnalyze_None},
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

    /* MODEM SPECIFIC EVENTS */
    {CMD_AT_WAIT_EVENT,     "",          BG96_DEFAULT_TIMEOUT,        fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_BOOT_EVENT,     "",          BG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_RDY_EVENT,      "RDY",       BG96_RDY_TIMEOUT,            fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_APP_RDY_EVENT,  "APP RDY",   BG96_APP_RDY_TIMEOUT,        fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_POWERED_DOWN_EVENT,     "POWERED DOWN", BG96_RDY_TIMEOUT, fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_PSM_POWER_DOWN_EVENT, "PSM POWER DOWN", BG96_RDY_TIMEOUT, fCmdBuild_NoParams,   fRspAnalyze_None},
    {CMD_AT_QPSMTIMER_EVENT, "+QPSMTIMER", BG96_RDY_TIMEOUT, fCmdBuild_NoParams,   fRspAnalyze_QPSMTIMER_BG96},
  };
#define SIZE_ATCMD_BG96_LUT ((uint16_t) (sizeof (ATCMD_BG96_LUT) / sizeof (atcustom_LUT_t)))

  /* common init */
  bg96_modem_init(&BG96_ctxt);

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  BG96_ctxt.modem_LUT_size = SIZE_ATCMD_BG96_LUT;
  BG96_ctxt.p_modem_LUT = (const atcustom_LUT_t *)ATCMD_BG96_LUT;

  /* override default termination string for AT command: <CR> */
  (void) sprintf((CRC_CHAR_t *)p_atp_ctxt->endstr, "\r");

  /* ###########################  END CUSTOMIZATION PART  ########################### */
}

/**
  * @brief  Check if message received is complete.
  * @param  rxChar Character received from modem.
  * @retval uint8_t Returns 1 if message is complete, else returns 0.
  */
uint8_t ATCustom_BG96_checkEndOfMsgCallback(uint8_t rxChar)
{
  uint8_t last_char = 0U;

  /* static variables */
  static const uint8_t QIRD_string[] = "+QIRD";
  static uint8_t QIRD_Counter = 0U;

  /*---------------------------------------------------------------------------------------*/
  if (BG96_ctxt.state_SyntaxAutomaton == WAITING_FOR_INIT_CR)
  {
    /* waiting for first valid <CR>, char received before are considered as trash */
    if ((AT_CHAR_t)('\r') == rxChar)
    {
      /* current     : xxxxx<CR>
      *  command format : <CR><LF>xxxxxxxx<CR><LF>
      *  waiting for : <LF>
      */
      BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (BG96_ctxt.state_SyntaxAutomaton == WAITING_FOR_CR)
  {
    if ((AT_CHAR_t)('\r') == rxChar)
    {
      /* current     : xxxxx<CR>
      *  command format : <CR><LF>xxxxxxxx<CR><LF>
      *  waiting for : <LF>
      */
      BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (BG96_ctxt.state_SyntaxAutomaton == WAITING_FOR_LF)
  {
    /* waiting for <LF> */
    if ((AT_CHAR_t)('\n') == rxChar)
    {
      /*  current        : <CR><LF>
      *   command format : <CR><LF>xxxxxxxx<CR><LF>
      *   waiting for    : x or <CR>
      */
      BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_FIRST_CHAR;
      last_char = 1U;
      QIRD_Counter = 0U;
    }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (BG96_ctxt.state_SyntaxAutomaton == WAITING_FOR_FIRST_CHAR)
  {
    if (BG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_waiting_header)
    {
      /* Socket Data RX - waiting for Header: we are waiting for +QIRD
      *
      * +QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
      *    ^- waiting this string
      */
      if (rxChar == QIRD_string[QIRD_Counter])
      {
        QIRD_Counter++;
        if (QIRD_Counter == (uint8_t) strlen((const CRC_CHAR_t *)QIRD_string))
        {
          /* +QIRD detected, next step */
          socketHeaderRX_reset();
          BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_header;
        }
      }
      else
      {
        /* this is not +QIRD, skip this command and wait for header */
        socketHeaderRX_reset();
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
        BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_waiting_header;
      }
    }

    /* NOTE:
    * if we are in socket_RxData_state = SocketRxDataState_waiting_header, we are waiting for +QIRD (test above)
    * but if we receive another message, we need to evacuate it without modifying socket_RxData_state
    * That's why we are nt using "else if" here, if <CR> if received before +QIND, it means that we have received
    * something else
    */
    if (BG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_header)
    {
      /* Socket Data RX - Header received: we are waiting for second <CR>
      *
      * +QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
      *         ^- retrieving this size
      *             ^- waiting this <CR>
      */
      if ((AT_CHAR_t)('\r') == rxChar)
      {
        /* second <CR> detected, we have received data header
        *  now waiting for <LF>, then start to receive socket data
        *  Verify that size received in header is the expected one
        */
        uint16_t size_from_header = SocketHeaderRX_getSize();
        if (BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size != size_from_header)
        {
          /* update buffer size received - should not happen */
          BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size = size_from_header;
        }
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
        BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_receiving_data;
      }
      else if ((rxChar >= (AT_CHAR_t)('0')) && (rxChar <= (AT_CHAR_t)('9')))
      {
        /* receiving size of data in header */
        SocketHeaderRX_addChar((CRC_CHAR_t *)&rxChar);
      }
      else if (rxChar == (AT_CHAR_t)(','))
      {
        /* receiving data field separator in header: +QIRD: 4,"10.7.76.34",7678
        *  data size field has been received, now ignore all chars until <CR><LF>
        *  additional fields (remote IP address and port) will be analyzed later
        */
        SocketHeaderDataRx_Cpt_Complete = 1U;
      }
      else {/* nothing to do */ }
    }
    else if (BG96_ctxt.socket_ctxt.socket_RxData_state == SocketRxDataState_receiving_data)
    {
      /* receiving socket data: do not analyze char, just count expected size
      *
      * +QIRD: 522<CR><LF>HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
      *.                  ^- start to read data: count char
      */
      BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
      BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_SOCKET_DATA;
      /* check if full buffer has been received */
      if (BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received == BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size)
      {
        BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
        BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
      }
    }
    /* waiting for <CR> or x */
    else if ((AT_CHAR_t)('\r') == rxChar)
    {
      /*   current        : <CR>
      *   command format : <CR><LF>xxxxxxxx<CR><LF>
      *   waiting for    : <LF>
      */
      BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_LF;
    }
    else {/* nothing to do */ }
  }
  /*---------------------------------------------------------------------------------------*/
  else if (BG96_ctxt.state_SyntaxAutomaton == WAITING_FOR_SOCKET_DATA)
  {
    BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received++;
    /* check if full buffer has been received */
    if (BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received == BG96_ctxt.socket_ctxt.socket_rx_expected_buf_size)
    {
      BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_data_received;
      BG96_ctxt.state_SyntaxAutomaton = WAITING_FOR_CR;
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
    /* BG96 special cases
    *
    *  SOCKET MODE: when sending DATA using AT+QISEND, we are waiting for socket prompt "<CR><LF>> "
    *               before to send DATA. Then we should receive "OK<CR><LF>".
    */

    if (BG96_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
    {
      switch (BG96_ctxt.socket_ctxt.socket_send_state)
      {
        case SocketSendState_WaitingPrompt1st_greaterthan:
        {
          /* detecting socket prompt first char: "greater than" */
          if ((AT_CHAR_t)('>') == rxChar)
          {
            BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt2nd_space;
          }
          break;
        }

        case SocketSendState_WaitingPrompt2nd_space:
        {
          /* detecting socket prompt second char: "space" */
          if ((AT_CHAR_t)(' ') == rxChar)
          {
            BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_Prompt_Received;
            last_char = 1U;
          }
          else
          {
            /* if char iommediatly after "greater than" is not a "space", reinit state */
            BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_WaitingPrompt1st_greaterthan;
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
at_status_t ATCustom_BG96_getCmd(at_context_t *p_at_ctxt, uint32_t *p_ATcmdTimeout)
{
  /* static variables */
  /* memorize number of SIM status query retries (static) */

  /* local variables */
  at_status_t retval;
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);

  /* retrieve parameters from SID command (will update SID_ctxt) */
  retval = atcm_retrieve_SID_parameters(&BG96_ctxt, p_atp_ctxt);

  if (retval == ATSTATUS_OK)
  {
    /* new command: reset command context */
    atcm_reset_CMD_context(&BG96_ctxt.CMD_ctxt);

    /* For each SID, the sequence of AT commands to send id defined (it can be dynamic)
    * Determine and prepare the next command to send for this SID
    */

#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
    /* BG96 specific
    * BG96 modem enters in PSM autonomously and suspends its uart.
    * If the HOST has not required LowPower state, AT-Custom will wake-up the modem.
    * If an HOST request is received durint this wake-up time, suspend the request until we
    * receive the "APP READY" event from the modem.
    */
    if (p_atp_ctxt->step == 0U)
    {
      if ((ATC_BG96_is_modem_in_PSM_state() == true) && (ATC_BG96_is_waiting_modem_wakeup() == true))
      {
        PRINT_INFO("WARNING !!! modem is in PSM state and can not answer")

        /* waiting for RDY indication from modem */
        atcm_program_WAIT_EVENT(p_atp_ctxt, BG96_APP_RDY_TIMEOUT, INTERMEDIATE_CMD);
      }
      else
      {
        /* check if modem is resuming from PSM. In this case, disable echo */
        if (bg96_shared.modem_resume_from_PSM)
        {
          /* disable echo */
          BG96_ctxt.CMD_ctxt.command_echo = AT_FALSE;
          atcm_program_AT_CMD(&BG96_ctxt, p_atp_ctxt, ATTYPE_EXECUTION_CMD, (CMD_ID_t) CMD_ATE, INTERMEDIATE_CMD);
          bg96_shared.modem_resume_from_PSM = false;
        }
        else
        {
          /* nothing to do, skip the command */
          atcm_program_SKIP_CMD(p_atp_ctxt);
        }
      }
    }
    else
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */
    {
      retval = at_custom_SID_bg96(&BG96_ctxt, p_at_ctxt, p_ATcmdTimeout);
    }

    /* if no error, build the command to send */
    if (retval == ATSTATUS_OK)
    {
      retval = atcm_modem_build_cmd(&BG96_ctxt, p_atp_ctxt, p_ATcmdTimeout);
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
at_endmsg_t ATCustom_BG96_extractElement(atparser_context_t *p_atp_ctxt,
                                         const IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
#if (USE_SOCKETS_TYPE != USE_SOCKETS_MODEM)
  UNUSED(p_atp_ctxt);
#endif /* (USE_SOCKETS_TYPE != USE_SOCKETS_MODEM) */

  at_endmsg_t retval_msg_end_detected = ATENDMSG_NO;
  bool exit_loop;
  static bool first_colon_found = false;
  static bool inside_quotes = false;
  uint16_t *p_parseIndex = &(element_infos->current_parse_idx);

  PRINT_API("enter ATCustom_BG96_extractElement()")
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
      /* <CR><LF> sequence has been found, this is a command line */
      PRINT_DBG("cmd init sequence <CR><LF> found - break")
      *p_parseIndex = 2U;
    }

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    exit_loop = false;
    for (uint16_t idx = *p_parseIndex; (idx < (p_msg_in->size - 1U)) && (exit_loop == false); idx++)
    {
      if ((p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_QIRD) &&
          (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Payload) &&
          (BG96_ctxt.socket_ctxt.socket_RxData_state != SocketRxDataState_finished))
      {
        PRINT_DBG("receiving socket data (real size=%d)", SocketHeaderRX_getSize())
        element_infos->str_start_idx = 0U;
        element_infos->str_end_idx = (uint16_t) BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        element_infos->str_size = (uint16_t) BG96_ctxt.socket_ctxt.socket_rx_count_bytes_received;
        BG96_ctxt.socket_ctxt.socket_RxData_state = SocketRxDataState_finished;
        retval_msg_end_detected = ATENDMSG_YES;
        exit_loop = true;
      }
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
at_action_rsp_t ATCustom_BG96_analyzeCmd(at_context_t *p_at_ctxt,
                                         const IPC_RxMessage_t *p_msg_in,
                                         at_element_info_t *element_infos)
{
  atparser_context_t *p_atp_ctxt = &(p_at_ctxt->parser);
  at_action_rsp_t retval;

  PRINT_API("enter ATCustom_BG96_analyzeCmd()")

  /* Analyze data received from the modem and
    * search in LUT the ID corresponding to command received
  */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
  bool no_valid_command_found;

  if (BG96_ctxt.socket_ctxt.socket_receive_state == SocketRcvState_RequestData_Payload)
  {
    /* receiving data payload on a socket: do not analyze received date (ie do not search in the LUT) */
    no_valid_command_found = true;
  }
  else if (ATSTATUS_OK != atcm_searchCmdInLUT(&BG96_ctxt, p_atp_ctxt, p_msg_in, element_infos))
  {
    /* no matching command has been found in the LUT */
    no_valid_command_found = true;
  }
  else
  {
    /* a matching command has been successfully found in the LUT */
    no_valid_command_found = false;
  }

  if (no_valid_command_found)
#else
  if (ATSTATUS_OK != atcm_searchCmdInLUT(&BG96_ctxt, p_atp_ctxt, p_msg_in, element_infos))
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */
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
    retval = atcm_check_text_line_cmd(&BG96_ctxt, p_at_ctxt, p_msg_in, element_infos);

    /* 2nd STEP: search in specific modems commands if not found at 1st STEP
     *
     * This is the case in socket mode when receiving data.
     * The command is decomposed in 2 lines:
     * The 1st part of the response is analyzed by atcm_searchCmdInLUT:
     *   +QIRD: 522<CR><LF>
     * The 2nd part of the response, corresponding to the data, falls here:
     *   HTTP/1.1 200 OK<CR><LF><CR><LF>Date: Wed, 21 Feb 2018 14:56:54 GMT<CR><LF><CR><LF>Serve...
     */
    if (retval == ATACTION_RSP_NO_ACTION)
    {
      switch (p_atp_ctxt->current_atcmd.id)
      {
          /* ###########################  START CUSTOMIZED PART  ########################### */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
        case CMD_AT_QIRD:
          if (fRspAnalyze_QIRD_data_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos) != ATACTION_RSP_ERROR)
          {
            /* received a valid intermediate answer */
            retval = ATACTION_RSP_INTERMEDIATE;
          }
          break;

        case CMD_AT_QISTATE:
          (void) fRspAnalyze_QISTATE_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos);
          /* received a valid intermediate answer */
          retval = ATACTION_RSP_INTERMEDIATE;
          break;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

        case CMD_AT_QGMR:
          (void) fRspAnalyze_QGMR_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos);
          /* received a valid intermediate answer */
          retval = ATACTION_RSP_INTERMEDIATE;
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
      {
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
            BG96_ctxt.persist.modem_at_ready = AT_TRUE;
          }
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_ATE)
          {
            PRINT_DBG("Echo successfully disabled")
          }
        }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_PING_IP_ADDRESS)
        {
          /* PING requests for BG96 are asynchronous.
          * If the PING request is valid (no other ongoing ping request for example), the
          * modem will answer OK.
          * At this point, initialize the ping response structure.
          */
          PRINT_DBG("this is a valid PING request")
          atcm_validate_ping_request(&BG96_ctxt);
        }

        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_DIAL_COMMAND)
        {
          if (p_atp_ctxt->current_atcmd.id == (CMD_ID_t) CMD_AT_QIOPEN)
          {
            /* AT+QIOPEN: OK received */
            bg96_shared.QIOPEN_OK = AT_TRUE;
          }
        }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)*/

        retval = ATACTION_RSP_FRC_END;
        break;
      }

      case CMD_AT_NO_CARRIER:
      case CMD_AT_NO_ANSWER:
      {
        retval = ATACTION_RSP_ERROR;
        break;
      }

      case CMD_AT_RING:
      case CMD_AT_NO_DIALTONE:
      case CMD_AT_BUSY:
      {
        /* VALUES NOT MANAGED IN CURRENT IMPLEMENTATION BECAUSE NOT EXPECTED */
        retval = ATACTION_RSP_ERROR;
        break;
      }

      case CMD_AT_CONNECT:
      {
        PRINT_INFO("MODEM SWITCHES TO DATA MODE")
        retval = (at_action_rsp_t)(ATACTION_RSP_FLAG_DATA_MODE | ATACTION_RSP_FRC_END);
        break;
      }

      /* ###########################  START CUSTOMIZATION PART  ########################### */
      case CMD_AT_CEREG:
      case CMD_AT_CREG:
      case CMD_AT_CGREG:
      {
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
      }

      case CMD_AT_RDY_EVENT:
      {
        /* We received RDY event from the modem.
        * If received during Power ON or RESET, it is indicating that the modem is ready.
        * If received in another state, we report to upper layer a modem reboot event.
        */
        BG96_ctxt.persist.modem_at_ready = AT_TRUE;
        if ((p_atp_ctxt->current_SID == (at_msg_t) SID_CS_POWER_ON) ||
            (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_RESET))
        {
          /* ignore the RDY event during POWER ON or RESET */
          retval = ATACTION_RSP_URC_IGNORED;
        }
#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
        else  if (ATC_BG96_is_modem_in_PSM_state() == true)
        {
          /* ignore this event when modem is in PSM, we will act on APP RDY instead
           * with APP RDY, all modem services should be ready
           */
          retval = ATACTION_RSP_URC_IGNORED;
        }
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */
        else
        {
          /* if event is received in other states, it's an unexpected modem reboot */
          if (atcm_modem_event_received(&BG96_ctxt, CS_MDMEVENT_BOOT) == AT_TRUE)
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

      case CMD_AT_APP_RDY_EVENT:
      {
#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
        if (ATC_BG96_is_waiting_modem_wakeup() == true)
        {
          PRINT_INFO("APP RDY unlock event")
          /* UNLOCK the WAIT EVENT */
          retval = ATACTION_RSP_FRC_END;
        }
        else
        {
          retval = ATACTION_RSP_URC_IGNORED;
        }
        /* update LP automatons */
        ATC_BG96_low_power_event(EVENT_LP_MDM_LEAVE_PSM);
#else
        retval = ATACTION_RSP_URC_IGNORED;
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */
        break;
      }

      case CMD_AT_POWERED_DOWN_EVENT:
      {
        PRINT_DBG("MODEM POWERED DOWN EVENT DETECTED")
        if (atcm_modem_event_received(&BG96_ctxt, CS_MDMEVENT_POWER_DOWN) == AT_TRUE)
        {
          retval = ATACTION_RSP_URC_FORWARDED;
        }
        else
        {
          retval = ATACTION_RSP_URC_IGNORED;
        }
        break;
      }

      case CMD_AT_QPSMTIMER_EVENT:
      {
        PRINT_DBG("QPSMTIMER URC DETECTED")
        /* will forward negotiated PSM timers value if new values detected */
        retval = ATACTION_RSP_URC_FORWARDED;;
        break;
      }

      case CMD_AT_SEND_OK:
      {
        if (p_atp_ctxt->current_SID == (at_msg_t) SID_CS_SEND_DATA)
        {
          retval = ATACTION_RSP_FRC_END;
        }
        else
        {
          retval = ATACTION_RSP_ERROR;
        }
        break;
      }

      case CMD_AT_SEND_FAIL:
      {
        retval = ATACTION_RSP_ERROR;
        break;
      }

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
      case CMD_AT_SOCKET_PROMPT:
      {
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
      }

      case CMD_AT_QIURC:
      {
        /* retval will be override in analyze of +QUIRC content
        *  indeed, QIURC can be considered as an URC or a normal msg (for DNS request)
        */
        retval = ATACTION_RSP_INTERMEDIATE;
        break;
      }

      case CMD_AT_QIOPEN:
        /* now waiting for an URC  */
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

      case CMD_AT_QPING:
        retval = ATACTION_RSP_URC_FORWARDED;
        break;
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

      case CMD_AT_QIND:
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case CMD_AT_CFUN:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_CPIN:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_QCFG:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

      case CMD_AT_QUSIM:
        retval = ATACTION_RSP_URC_IGNORED;
        break;

      case CMD_AT_CPSMS:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

      case CMD_AT_CEDRXS:
        retval = ATACTION_RSP_INTERMEDIATE;
        break;

      case CMD_AT_CGEV:
        retval = ATACTION_RSP_URC_FORWARDED;
        break;

      case CMD_AT_PSM_POWER_DOWN_EVENT:
#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
        ATC_BG96_low_power_event(EVENT_LP_MDM_ENTER_PSM);
#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */
        retval =  ATACTION_RSP_IGNORED;
        break;

      /* ###########################  END CUSTOMIZATION PART  ########################### */

      case CMD_AT:
        retval = ATACTION_RSP_IGNORED;
        break;

      case CMD_AT_INVALID:
        retval = ATACTION_RSP_ERROR;
        break;

      case CMD_AT_ERROR:
        /* ERROR does not contains parameters, so call the analyze function explicitly
        *  otherwise it will not ne called
        */
        retval = fRspAnalyze_Error_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos);
        break;

      case CMD_AT_CME_ERROR:
      case CMD_AT_CMS_ERROR:
        /* do the analyze here because will not be called by parser if the command
        * has no parameters (+CME ERROR only without parameters)
        */
        retval = fRspAnalyze_Error_BG96(p_at_ctxt, &BG96_ctxt, p_msg_in, element_infos);
        break;

      default:
      {
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
at_action_rsp_t ATCustom_BG96_analyzeParam(at_context_t *p_at_ctxt,
                                           const IPC_RxMessage_t *p_msg_in,
                                           at_element_info_t *element_infos)
{
  at_action_rsp_t retval;
  PRINT_API("enter ATCustom_BG96_analyzeParam()")

  /* analyse parameters of the command we received:
  * call the corresponding function from the LUT
  */
  retval = (atcm_get_CmdAnalyzeFunc(&BG96_ctxt, element_infos->cmd_id_received))(p_at_ctxt,
                                                                                 &BG96_ctxt,
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
at_action_rsp_t ATCustom_BG96_terminateCmd(atparser_context_t *p_atp_ctxt, at_element_info_t *element_infos)
{
  at_action_rsp_t retval = ATACTION_RSP_IGNORED;
  PRINT_API("enter ATCustom_BG96_terminateCmd()")

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* additional tests */
  if (BG96_ctxt.socket_ctxt.socket_send_state != SocketSendState_No_Activity)
  {
    /* special case for SID_CS_SEND_DATA
    * indeed, this function is called when an AT cmd is finished
    * but for AT+QISEND, it is called a 1st time when prompt is received
    * and a second time when data have been sent.
    */
    if (p_atp_ctxt->current_SID != (at_msg_t) SID_CS_SEND_DATA)
    {
      /* reset socket_send_state */
      BG96_ctxt.socket_ctxt.socket_send_state = SocketSendState_No_Activity;
    }
  }

  if ((p_atp_ctxt->current_atcmd.id == (at_msg_t) CMD_ATD) ||
      (p_atp_ctxt->current_atcmd.id == (at_msg_t) CMD_ATO) ||
      (p_atp_ctxt->current_atcmd.id == (at_msg_t) CMD_AT_CGDATA))
  {
    if (element_infos->cmd_id_received == (at_msg_t) CMD_AT_CONNECT)
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
at_status_t ATCustom_BG96_get_rsp(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_BG96_get_rsp()")

  /* prepare response for a SID - common part */
  retval = atcm_modem_get_rsp(&BG96_ctxt, p_atp_ctxt, p_rsp_buf);

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
                               (uint16_t) sizeof(bg96_shared.QIURC_dnsgip_param.hostIPaddr),
                               (void *)bg96_shared.QIURC_dnsgip_param.hostIPaddr) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;

    case SID_CS_POWER_ON:
    case SID_CS_RESET:
      display_user_friendly_mode_and_bands_config();
      break;

    case SID_CS_POWER_OFF:
      /* reinit context for power off case */
      ATC_BG96_modem_reset(&BG96_ctxt);
      break;

#if (BG96_ACTIVATE_PING_REPORT == 1)
    case SID_CS_PING_IP_ADDRESS:
    {
      /* SID_CS_PING_IP_ADDRESS is waiting for a ping structure
       * For this modem, PING will be received later (as URC).
       * Just indicate that no ping report is available for now.
       */
      PRINT_DBG("Ping no report available yet - use PING_INVALID_INDEX")
      (void) memset((void *)&BG96_ctxt.persist.ping_resp_urc, 0, sizeof(CS_Ping_response_t));
      BG96_ctxt.persist.ping_resp_urc.index = PING_INVALID_INDEX;
      if (DATAPACK_writeStruct(p_rsp_buf,
                               (uint16_t) CSMT_URC_PING_RSP,
                               (uint16_t) sizeof(CS_Ping_response_t),
                               (void *)&BG96_ctxt.persist.ping_resp_urc) != DATAPACK_OK)
      {
        retval = ATSTATUS_OK;
      }
      break;
    }
#endif /* (BG96_ACTIVATE_PING_REPORT == 1) */

    default:
      break;
  }
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  /* reset SID context */
  atcm_reset_SID_context(&BG96_ctxt.SID_ctxt);

  /* reset socket context */
  atcm_reset_SOCKET_context(&BG96_ctxt);

  return (retval);
}

/**
  * @brief  Returns a buffer containing the URC received.
  * @param  p_at_ctxt Pointer to the structure of AT context.
  * @param  p_rsp_buf Pointer to buffer with the response to send.
  * @retval at_status_t.
  */
at_status_t ATCustom_BG96_get_urc(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_BG96_get_urc()")

  /* prepare response for an URC - common part */
  retval = atcm_modem_get_urc(&BG96_ctxt, p_atp_ctxt, p_rsp_buf);

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
at_status_t ATCustom_BG96_get_error(atparser_context_t *p_atp_ctxt, at_buf_t *p_rsp_buf)
{
  at_status_t retval;
  PRINT_API("enter ATCustom_BG96_get_error()")

  /* prepare response when an error occurred - common part */
  retval = atcm_modem_get_error(&BG96_ctxt, p_atp_ctxt, p_rsp_buf);

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
at_status_t ATCustom_BG96_hw_event(sysctrl_device_type_t deviceType, at_hw_event_t hwEvent, GPIO_PinState gstate)
{
  UNUSED(deviceType);
  UNUSED(hwEvent);
  UNUSED(gstate);

  at_status_t retval = ATSTATUS_OK;
  /* IMPORTANT: Do not add traces int this function of in functions called
   * (this function called under interrupt if GPIO event)
   */

  /* ###########################  START CUSTOMIZATION PART  ########################### */
  /* NO GPIO EVENT FOR THIS MODEM IN PSM */
  /* ###########################  END CUSTOMIZATION PART  ########################### */

  return (retval);
}

/**
  * @brief  Global reset of modem parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none.
  */
void ATC_BG96_modem_reset(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter bg96_modem_reset")

  /* common reset function (reset all contexts except SID) */
  atcm_modem_reset(p_modem_ctxt);

  /* modem specific actions if any */
}

/**
  * @brief  Reset modem shared parameters.
  * @retval none.
  */
void ATC_BG96_reset_variables(void)
{
  /* Set default values of BG96 specific variables after SWITCH ON or RESET */
  bg96_shared.mode_and_bands_config.nw_scanseq = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.nw_scanmode = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.iot_op_mode = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.gsm_bands = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.CatM1_bands_MsbPart = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.CatM1_bands_LsbPart = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.CatNB1_bands_MsbPart = 0xFFFFFFFFU;
  bg96_shared.mode_and_bands_config.CatNB1_bands_LsbPart = 0xFFFFFFFFU;

  /* other values */
#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
  bg96_shared.host_lp_state = HOST_LP_STATE_IDLE;
  bg96_shared.modem_lp_state = MDM_LP_STATE_IDLE;
  bg96_shared.modem_resume_from_PSM = false;
#endif /* (ENABLE_BG96_LOW_POWER_MODE == 1U) */
}

/**
  * @brief  Reinitialize state of Syntax Automaton used to manage characters received from modem.
  * @retval none.
  */
void ATC_BG96_reinitSyntaxAutomaton(atcustom_modem_context_t *p_modem_ctxt)
{
  p_modem_ctxt->state_SyntaxAutomaton = WAITING_FOR_INIT_CR;
}

#if (ENABLE_BG96_LOW_POWER_MODE == 1U)
/**
  * @brief  Initialization of modem low power parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval at_bool_t Returns true if Low Power is enabled.
  */
at_bool_t ATC_BG96_init_low_power(atcustom_modem_context_t *p_modem_ctxt)
{
  at_bool_t lp_enabled;

  if (p_modem_ctxt->SID_ctxt.init_power_config.low_power_enable == CS_TRUE)
  {
    /* this parameter is used in CGREG/CEREG to enable or not PSM urc.
     * BG96 specific case: CEREG returns ERROR or UNKNOWN when CEREG=4 and CPSMS=0.
     * To avoid this, never use CEREG=4.
     */
    p_modem_ctxt->persist.psm_urc_requested = AT_FALSE;

    /* PSM and EDRX parameters: need to populate SID_ctxt.set_power_config which is the structure
     * used to build PSM and EDRX AT commands.
     * Provide psm and edrx default parameters.
     *
     * BG96 specific: always set psm_mode to PSM_MODE_DISABLE, will be set to enable only after a sleep request.
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
    /* this parameter is used in CGREG/CEREG to enable or not PSM urc.  */
    p_modem_ctxt->persist.psm_urc_requested = AT_FALSE;

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
at_bool_t ATC_BG96_set_low_power(atcustom_modem_context_t *p_modem_ctxt)
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
  * @retval none.
  */
void ATC_BG96_low_power_event(ATCustom_BG96_LP_event_t event)
{
  uint8_t host_state_error = 1U;
  uint8_t mdm_state_error = 1U;

  /* ##### manage HOST LP state ##### */
  if (bg96_shared.host_lp_state == HOST_LP_STATE_IDLE)
  {
    /* Host events */
    if (event == EVENT_LP_HOST_SLEEP_REQ)
    {
      bg96_shared.host_lp_state = HOST_LP_STATE_LP_REQUIRED;
      host_state_error = 0U;
    }
    /* Modem events */
    else if (event == EVENT_LP_MDM_ENTER_PSM)
    {
      /* Host in not in Low Power state, force modem wake up */
      PRINT_INFO("modem is in PSM state but Host is not, force wake up")
      /* a non null delay is used when called just after PSM POWER DOWN indication,
      *  otherwise this wake up request is not taken into account by the modem.
      */
      bg96_shared.host_lp_state = HOST_LP_STATE_WAKEUP_REQUIRED;
      (void) SysCtrl_BG96_wakeup_from_PSM(1000U);
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_LEAVE_PSM)
    {
      /* host is already in idle state, no need to update host_lp_state */
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_WAKEUP_REQ)
    {
      /* not implemented yet
       * is needed if UART is disabled by host (in case of STM32 low-power)
       */
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (bg96_shared.host_lp_state == HOST_LP_STATE_LP_REQUIRED)
  {
    /* Host events */
    if (event == EVENT_LP_HOST_SLEEP_COMPLETE)
    {
      bg96_shared.host_lp_state = HOST_LP_STATE_LP_ONGOING;
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_HOST_SLEEP_CANCEL)
    {
      bg96_shared.host_lp_state = HOST_LP_STATE_IDLE;
      host_state_error = 0U;
    }
    /* Modem events */
    else if (event == EVENT_LP_MDM_ENTER_PSM)
    {
      /* Host has required Low Power state, do not force modem wake up */
      PRINT_INFO("modem entered in PSM state")
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_LEAVE_PSM)
    {
      /* Crossing case.
       * Modem has left PSM state (T3412 expiry) while host is requiring Low Power.
       * Modem will returns automatically in PSM after T3324 expiry.
       */
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_WAKEUP_REQ)
    {
      /* not implemented yet
       * is needed if UART is disabled by host (in case of STM32 low-power)
       */
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (bg96_shared.host_lp_state == HOST_LP_STATE_LP_ONGOING)
  {
    /* Host events */
    if (event == EVENT_LP_HOST_WAKEUP_REQ)
    {
      /* Host Wakeup request, check if modem is in PSM */
      if (bg96_shared.modem_lp_state == MDM_LP_STATE_PSM)
      {
        /* waiting for modem leaving PSM confirmation */
        bg96_shared.host_lp_state = HOST_LP_STATE_WAKEUP_REQUIRED;
        host_state_error = 0U;
      }
      else
      {
        /* modem not in PSM, no need to wait for modem confirmation*/
        bg96_shared.host_lp_state = HOST_LP_STATE_IDLE;
        host_state_error = 0U;
      }
    }
    /* Modem events */
    else if (event == EVENT_LP_MDM_ENTER_PSM)
    {
      /* Modem enters PSM (T3324 expiry) */
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_LEAVE_PSM)
    {
      /* Modem has left PSM state while HOST is in Low Power state.
       * Modem will returns in PSM after T3324 expiry.
       */
      host_state_error = 0U;
    }
    else if (event == EVENT_LP_MDM_WAKEUP_REQ)
    {
      /* not implemented yet
       * is needed if UART is disabled by host (in case of STM32 low-power)
       */
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (bg96_shared.host_lp_state == HOST_LP_STATE_WAKEUP_REQUIRED)
  {
    /* Modem events */
    if (event == EVENT_LP_MDM_LEAVE_PSM)
    {
      bg96_shared.host_lp_state = HOST_LP_STATE_IDLE;
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

  /* ##### manage MODEM LP state ##### */
  if (bg96_shared.modem_lp_state == MDM_LP_STATE_IDLE)
  {
    if (event == EVENT_LP_MDM_ENTER_PSM)
    {
      bg96_shared.modem_lp_state = MDM_LP_STATE_PSM;
      mdm_state_error = 0U;
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else if (bg96_shared.modem_lp_state == MDM_LP_STATE_PSM)
  {
    if (event == EVENT_LP_MDM_LEAVE_PSM)
    {
      bg96_shared.modem_resume_from_PSM = true;
      bg96_shared.modem_lp_state = MDM_LP_STATE_IDLE;
      mdm_state_error = 0U;
    }
    else
    {
      /* unexpected event in this state */
    }
  }
  else /* state equal MDM_LP_STATE_ERROR or unexpected */
  {
    /* error or unexpected state  */
    mdm_state_error = 2U;
  }

  /* --- report automaton status --- */
  if (host_state_error == 0U)
  {
    PRINT_INFO("LP HOST AUTOMATON new state=%d ", bg96_shared.host_lp_state)
  }
  else if (host_state_error == 1U)
  {
    PRINT_INFO("LP HOST AUTOMATON ignore event %d in state %d", event, bg96_shared.host_lp_state)
  }
  else
  {
    PRINT_INFO("LP HOST AUTOMATON ERROR: unexpected state %d", bg96_shared.host_lp_state)
  }

  if (mdm_state_error == 0U)
  {
    PRINT_INFO("LP MODEM AUTOMATON new state=%d ", bg96_shared.modem_lp_state)
  }
  else if (mdm_state_error == 1U)
  {
    PRINT_INFO("LP MODEM AUTOMATON ignore event %d in state %d", event, bg96_shared.modem_lp_state)
  }
  else
  {
    PRINT_INFO("LP MODEM AUTOMATON ERROR: unexpected state %d", bg96_shared.modem_lp_state)
  }
  /* error codes: 1 for ignored event, 2 for unexpected state */
  if ((mdm_state_error != 0U) && (host_state_error != 0U))
  {
    PRINT_INFO("LP AUTOMATONS WARNING ignored event %d (mdm_err=%d, host_err=%d)",
               event, mdm_state_error, host_state_error)
  }
}

/**
  * @brief  Get Modem PSM state
  * @retval bool Returns true if modem in PSM state.
  */
bool ATC_BG96_is_modem_in_PSM_state(void)
{
  return (bg96_shared.modem_lp_state == MDM_LP_STATE_PSM);
}

/**
  * @brief  Indicates if we are waiting for a modem WakeUp
  * @retval bool Returns true if modem in WakeUp required state.
  */
bool ATC_BG96_is_waiting_modem_wakeup(void)
{
  return (bg96_shared.host_lp_state == HOST_LP_STATE_WAKEUP_REQUIRED);
}

#endif /* ENABLE_BG96_LOW_POWER_MODE == 1U */

/**
  * @}
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SPECIFIC_Private_Functions AT_CUSTOM QUECTEL_BG96 SPECIFIC Private Functions
  * @{
  */

/**
  * @brief  Initialization of modem parameters.
  * @param  p_modem_ctxt Pointer to the structure of Modem context.
  * @retval none.
  */
static void bg96_modem_init(atcustom_modem_context_t *p_modem_ctxt)
{
  PRINT_API("enter bg96_modem_init")

  /* common init function (reset all contexts) */
  atcm_modem_init(p_modem_ctxt);

  /* modem specific actions if any */
}

/**
  * @brief  Reset header structure for RX socket data.
  * @retval none.
  */
static void socketHeaderRX_reset(void)
{
  (void) memset((void *)SocketHeaderDataRx_Buf, 0, 4U);
  SocketHeaderDataRx_Cpt = 0U;
  SocketHeaderDataRx_Cpt_Complete = 0U;
}

/**
  * @brief Add character received to the header.
  * @param rxchar Character received.
  * @retval none.
  */
static void SocketHeaderRX_addChar(CRC_CHAR_t *rxchar)
{
  if ((SocketHeaderDataRx_Cpt_Complete == 0U) && (SocketHeaderDataRx_Cpt < 4U))
  {
    (void) memcpy((void *)&SocketHeaderDataRx_Buf[SocketHeaderDataRx_Cpt], (void *)rxchar, sizeof(char));
    SocketHeaderDataRx_Cpt++;
  }
}

/**
  * @brief  Get Header size.
  * @retval uint16_t Header size.
  */
static uint16_t SocketHeaderRX_getSize(void)
{
  uint16_t retval = (uint16_t) ATutil_convertStringToInt((uint8_t *)SocketHeaderDataRx_Buf,
                                                         (uint16_t)SocketHeaderDataRx_Cpt);
  return (retval);
}

/**
  * @brief  Decode and display GSM bands
  * @param gsm_bands Bitmap for GSM bands.
  * @retval none.
  */
static void display_decoded_GSM_bands(uint32_t gsm_bands)
{
  PRINT_INFO("GSM BANDS config = 0x%lx", gsm_bands)

  if ((gsm_bands & QCFGBANDGSM_900) != 0U)
  {
    PRINT_INFO("GSM_900")
  }
  if ((gsm_bands & QCFGBANDGSM_1800) != 0U)
  {
    PRINT_INFO("GSM_1800")
  }
  if ((gsm_bands & QCFGBANDGSM_850) != 0U)
  {
    PRINT_INFO("GSM_850")
  }
  if ((gsm_bands & QCFGBANDGSM_1900) != 0U)
  {
    PRINT_INFO("GSM_1900")
  }
}

/**
  * @brief  Decode and display Cat.M1 bands
  * @param CatM1_bands_MsbPart MSB part of bitmap for Cat.M1 bands.
  * @param CatM1_bands_LsbPart LSB part of bitmap for Cat.M1 bands.
  * @retval none.
  */
static void display_decoded_CatM1_bands(uint32_t CatM1_bands_MsbPart, uint32_t CatM1_bands_LsbPart)
{
  PRINT_INFO("Cat.M1 BANDS config = 0x%lx%lx", CatM1_bands_MsbPart, CatM1_bands_LsbPart)

  /* LSB bitmap part: ........XXXXXXXX */
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B1_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B1")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B2_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B2")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B3_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B3")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B4_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B4")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B5_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B5")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B8_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B8")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B12_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B12")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B13_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B13")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B18_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B18")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B19_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B19")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B20_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B20")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B26_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B26")
  }
  if ((CatM1_bands_LsbPart & (uint32_t) QCFGBANDCATM1_B28_LSB) != 0U)
  {
    PRINT_INFO("CatM1_B28")
  }
  /* MSB bitmap part: XXXXXXXX........ */
  if ((CatM1_bands_MsbPart & (uint32_t) QCFGBANDCATM1_B39_MSB) != 0U)
  {
    PRINT_INFO("CatM1_B39")
  }
}

/**
  * @brief  Decode and display Cat.NB1 bands
  * @param CatM1_bands_MsbPart MSB part of bitmap for Cat.NB1 bands.
  * @param CatM1_bands_LsbPart LSB part of bitmap for Cat.NB1 bands.
  * @retval none.
  */
static void display_decoded_CatNB1_bands(uint32_t CatNB1_bands_MsbPart, uint32_t CatNB1_bands_LsbPart)
{
  UNUSED(CatNB1_bands_MsbPart); /* MSB part of NB1 band bitmap is not used */

  PRINT_INFO("Cat.NB1 BANDS config = 0x%lx%lx", CatNB1_bands_MsbPart, CatNB1_bands_LsbPart)

  /* LSB bitmap part: ........XXXXXXXX */
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B1_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B1")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B2_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B2")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B3_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B3")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B4_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B4")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B5_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B5")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B8_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B8")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B12_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B12")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B13_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B13")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B18_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B18")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B19_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B19")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B20_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B20")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B26_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B26")
  }
  if ((CatNB1_bands_LsbPart & (uint32_t) QCFGBANDCATNB1_B28_LSB) != 0U)
  {
    PRINT_INFO("CatNB1_B28")
  }
  /* MSB bitmap part: XXXXXXXX........ */
  /* no bands in MSB bitmap part */
}

/**
  * @brief Display Mode and Bands.
  * @retval none.
  */
static void display_user_friendly_mode_and_bands_config(void)
{
  uint8_t catM1_on = 0U;
  uint8_t catNB1_on = 0U;
  uint8_t gsm_on = 0U;
  ATCustom_BG96_QCFGscanseq_t scanseq_1st, scanseq_2nd, scanseq_3rd;

#if 0 /* for DEBUG */
  /* display modem raw values */
  display_decoded_CatM1_bands(bg96_shared.mode_and_bands_config.CatM1_bands);
  display_decoded_CatNB1_bands(bg96_shared.mode_and_bands_config.CatNB1_bands);
  display_decoded_GSM_bands(bg96_shared.mode_and_bands_config.gsm_bands);

  PRINT_INFO("nw_scanmode = 0x%x", bg96_shared.mode_and_bands_config.nw_scanmode)
  PRINT_INFO("iot_op_mode = 0x%x", bg96_shared.mode_and_bands_config.iot_op_mode)
  PRINT_INFO("nw_scanseq = 0x%x", bg96_shared.mode_and_bands_config.nw_scanseq)
#endif /* 0, for DEBUG */

  PRINT_INFO(">>>>> BG96 mode and bands configuration <<<<<")
  /* LTE bands */
  if ((bg96_shared.mode_and_bands_config.nw_scanmode == QCFGSCANMODE_AUTO) ||
      (bg96_shared.mode_and_bands_config.nw_scanmode == QCFGSCANMODE_LTEONLY))
  {
    if ((bg96_shared.mode_and_bands_config.iot_op_mode == QCFGIOTOPMODE_CATM1CATNB1) ||
        (bg96_shared.mode_and_bands_config.iot_op_mode == QCFGIOTOPMODE_CATM1))
    {
      /* LTE Cat.M1 active */
      catM1_on = 1U;
    }
    if ((bg96_shared.mode_and_bands_config.iot_op_mode == QCFGIOTOPMODE_CATM1CATNB1) ||
        (bg96_shared.mode_and_bands_config.iot_op_mode == QCFGIOTOPMODE_CATNB1))
    {
      /* LTE Cat.NB1 active */
      catNB1_on = 1U;
    }
  }

  /* GSM bands */
  if ((bg96_shared.mode_and_bands_config.nw_scanmode == QCFGSCANMODE_AUTO) ||
      (bg96_shared.mode_and_bands_config.nw_scanmode == QCFGSCANMODE_GSMONLY))
  {
    /* GSM active */
    gsm_on = 1U;
  }

  /* Search active techno */
  scanseq_1st = ((ATCustom_BG96_QCFGscanseq_t)bg96_shared.mode_and_bands_config.nw_scanseq &
                 (ATCustom_BG96_QCFGscanseq_t)0x00FF0000U) >> 16;
  scanseq_2nd = ((ATCustom_BG96_QCFGscanseq_t)bg96_shared.mode_and_bands_config.nw_scanseq &
                 (ATCustom_BG96_QCFGscanseq_t)0x0000FF00U) >> 8;
  scanseq_3rd = ((ATCustom_BG96_QCFGscanseq_t)bg96_shared.mode_and_bands_config.nw_scanseq &
                 (ATCustom_BG96_QCFGscanseq_t)0x000000FFU);
  PRINT_DBG("decoded scanseq: 0x%lx -> 0x%lx -> 0x%lx", scanseq_1st, scanseq_2nd, scanseq_3rd)

  uint8_t rank = 1U;
  /* display active bands by rank
   * rank is incremented only if a band is really active
   */
  /* 1st band */
  if (1U == display_if_active_band(scanseq_1st, rank, catM1_on, catNB1_on, gsm_on))
  {
    rank++;
  }
  /* 2nd band */
  if (1U == display_if_active_band(scanseq_2nd, rank, catM1_on, catNB1_on, gsm_on))
  {
    rank++;
  }
  /* 3rd band (do not need to increment rank as this is the last band */
  (void) display_if_active_band(scanseq_3rd, rank, catM1_on, catNB1_on, gsm_on);

  PRINT_INFO(">>>>> ................................. <<<<<")
}

/**
  * @brief Display band info if it is active.
  * @param rank Band rank.
  * @param catM1_on Band state.
  * @param catNB1_on Band state.
  * @param gsm_on Band state.
  * @retval uint8_t Returns 1 if at least one band info displayed.
  */
static uint8_t display_if_active_band(ATCustom_BG96_QCFGscanseq_t scanseq,
                                      uint8_t rank,
                                      uint8_t catM1_on,
                                      uint8_t catNB1_on,
                                      uint8_t gsm_on)
{
  UNUSED(rank);
  uint8_t retval = 0U;

  if (scanseq == (ATCustom_BG96_QCFGscanseq_t) QCFGSCANSEQ_GSM)
  {
    if (gsm_on == 1U)
    {
      PRINT_INFO("GSM band active (scan rank = %d)", rank)
      display_decoded_GSM_bands(bg96_shared.mode_and_bands_config.gsm_bands);

      retval = 1U;
    }
  }
  else if (scanseq == (ATCustom_BG96_QCFGscanseq_t) QCFGSCANSEQ_LTECATM1)
  {
    if (catM1_on == 1U)
    {
      PRINT_INFO("LTE Cat.M1 band active (scan rank = %d)", rank)
      display_decoded_CatM1_bands(bg96_shared.mode_and_bands_config.CatM1_bands_MsbPart,
                                  bg96_shared.mode_and_bands_config.CatM1_bands_LsbPart);
      retval = 1U;
    }
  }
  else if (scanseq == (ATCustom_BG96_QCFGscanseq_t) QCFGSCANSEQ_LTECATNB1)
  {
    if (catNB1_on == 1U)
    {
      PRINT_INFO("LTE Cat.NB1 band active (scan rank = %d)", rank)
      display_decoded_CatNB1_bands(bg96_shared.mode_and_bands_config.CatNB1_bands_MsbPart,
                                   bg96_shared.mode_and_bands_config.CatNB1_bands_LsbPart);
      retval = 1U;
    }
  }
  else
  {
    /* scanseq value not managed */
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
