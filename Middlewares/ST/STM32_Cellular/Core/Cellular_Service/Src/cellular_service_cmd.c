/**
  ******************************************************************************
  * @file    cellular_service_cmd.c
  * @author  MCD Application Team
  * @brief   This file defines cellular service console command
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>

#include "plf_config.h"

#if (USE_CMD_CONSOLE == 1)
#include "dc_common.h"

#include "cellular_control_api.h"

#include "cellular_service_datacache.h"
#include "cellular_service_os.h"
#include "cellular_service_utils.h"
#include "cellular_service_task.h"
#include "cellular_service_cmd.h"
#include "error_handler.h"
#include "cellular_runtime_custom.h"
#include "cellular_service_config.h"

#if defined(USE_MODEM_BG96)
/* MODEM BG96 */
#define CST_CMD_USE_MODEM_CONFIG     (1)
#define CST_CMD_MODEM_BG96           (1)
#define CST_CMD_MODEM_TYPE1SC        (0)
#define CST_CMD_USE_MODEM_CELL_GM01Q (0)
#elif defined(USE_MODEM_TYPE1SC)
/* MODEM TYPE1SC */
#define CST_CMD_USE_MODEM_CONFIG     (1)
#define CST_CMD_MODEM_BG96           (0)
#define CST_CMD_MODEM_TYPE1SC        (1)
#define CST_CMD_USE_MODEM_CELL_GM01Q (0)
#elif defined(USE_MODEM_GM01Q)
/* MODEM GM01Q */
#define CST_CMD_USE_MODEM_CONFIG     (1)
#define CST_CMD_MODEM_BG96           (0)
#define CST_CMD_MODEM_TYPE1SC        (0)
#define CST_CMD_USE_MODEM_CELL_GM01Q (1)
#else
/* NO MODEM DEFINED */
#define CST_CMD_USE_MODEM_CONFIG     (0)
#define CST_CMD_MODEM_BG96           (0)
#define CST_CMD_MODEM_TYPE1SC        (0)
#define CST_CMD_USE_MODEM_CELL_GM01Q (0)
#endif  /* USE_MODEM_BG96 */

#if (CST_CMD_MODEM_BG96 == 1)
#include "at_custom_modem_specific.h"
#endif  /* CST_CMD_MODEM_BG96 */

#include "cmd.h"

/* Private defines -----------------------------------------------------------*/
#define CST_MAX_STR_LEN         45U      /* Max len of string when converting constant integer to explicit string */

#define CST_ATCMD_SIZE_MAX      100U     /* AT CMD length max              */
#define CST_CMS_PARAM_MAX        13U     /* number max of cmd param        */
#define CST_AT_TIMEOUT         5000U     /* default AT cmd response timeout */


#if (CST_CMD_MODEM_BG96 == 1)
/* BG96 Modem constant definition */

/* To display scanmode config value */
/* Has to be consistent with the array CST_ScanmodeName_p */
#define CST_DISP_SCANMODE_AUTO 0U
#define CST_DISP_SCANMODE_GSM 1U
#define CST_DISP_SCANMODE_LTE 2U

/* Number of bands */
#define CST_CMD_MAX_BAND        16U
#define CST_CMD_GSM_BAND_NUMBER  6U
#define CST_CMD_M1_BAND_NUMBER  16U
#define CST_CMD_NB1_BAND_NUMBER 15U
#define CST_CMD_SCANSEQ_NUMBER  16U

/* Bit mask of scan sequences */
#define  CST_scanseq_NB1_M1  ((ATCustom_BG96_QCFGscanseq_t) 0x030202)
#define  CST_scanseq_NB1_GSM ((ATCustom_BG96_QCFGscanseq_t) 0x030101)
#define  CST_scanseq_M1_GSM  ((ATCustom_BG96_QCFGscanseq_t) 0x020101)
#define  CST_scanseq_M1_NB1  ((ATCustom_BG96_QCFGscanseq_t) 0x020303)
#define  CST_scanseq_GSM_M1  ((ATCustom_BG96_QCFGscanseq_t) 0x010202)
#define  CST_scanseq_GSM_NB1 ((ATCustom_BG96_QCFGscanseq_t) 0x010303)

#define  CST_scanseq_GSM     ((ATCustom_BG96_QCFGscanseq_t) 0x010101)
#define  CST_scanseq_M1      ((ATCustom_BG96_QCFGscanseq_t) 0x020202)
#define  CST_scanseq_NB1     ((ATCustom_BG96_QCFGscanseq_t) 0x030303)

#endif  /* (CST_CMD_MODEM_BG96 == 1)*/

#if (CST_CMD_MODEM_TYPE1SC == 1)
/*  ALTAIR Modem constant definition */
#define CST_CMD_BAND_MAX 12
#endif /* defined(CST_CMD_MODEM_TYPE1SC == 1) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
/*  MONARCH Modem constant definition */
/* Band number */
#define CST_CMD_BAND_MAX 12
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */


/* Private macros ------------------------------------------------------------*/
/* Trace macro */
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)   (void)printf("" format "\n\r", ## args);
#endif  /* (USE_PRINTF == 0U) */

/* Private typedef -----------------------------------------------------------*/
/* Band description */
typedef struct
{
  uint8_t *name;      /* Name of Band                                  */
  uint32_t value_MSB; /* 32bits MSB part of the band: XXXXXXXX........ */
  uint32_t value_LSB; /* 32bits LSB part of the band: ........XXXXXXXX */
} CST_band_descr_t;

typedef struct
{
  uint8_t *name;    /* Sequence name           */
  uint32_t value;   /* associated bitmap value */
} CST_seq_descr_t;

/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/*  cellular service task commands prefix */
static uint8_t *CST_cmd_label = ((uint8_t *)"cst");

/*  at command prefix */
static uint8_t *CST_cmd_at_label = ((uint8_t *)"atcmd");


#if (CST_CMD_USE_MODEM_CONFIG==1)
/*  modem configuration command  prefix */
static uint8_t *CST_cmd_modem_label = ((uint8_t *)"modem");
#endif /* CST_CMD_USE_MODEM_CONFIG == 1 */

/* Current AT command timeout */
static uint32_t cst_at_timeout = CST_AT_TIMEOUT;

#if (( CST_CMD_MODEM_TYPE1SC == 1) || ( CST_CMD_USE_MODEM_CELL_GM01Q == 1 ))
static uint8_t CST_CMD_band_count = 2;
#endif /* (( CST_CMD_MODEM_TYPE1SC == 1) || ( CST_CMD_USE_MODEM_CELL_GM01Q == 1 )) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
static uint8_t CST_CMD_band_tab[CST_CMD_BAND_MAX] = {3, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */

#if (CST_CMD_MODEM_TYPE1SC == 1)
static uint8_t CST_CMD_band_tab[CST_CMD_BAND_MAX] = {13, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif /* (CST_CMD_MODEM_TYPE1SC == 1) */


/* Private function prototypes -----------------------------------------------*/
static cmd_status_t CST_cmd(uint8_t *cmd_line_p);
static cmd_status_t cst_at_command_handle(uint8_t *cmd_line_p);
static void CST_HelpCmd(void);
static void cst_at_cmd_help(void);

#if (CST_CMD_USE_MODEM_CONFIG==1)
static void CST_ModemHelpCmd(void);
#if ( CST_CMD_MODEM_BG96 == 1 )
static uint32_t CST_CMD_get_band(const CST_band_descr_t *band_descr,
                                 const uint8_t   *const *argv_p, uint32_t argc,
                                 uint32_t *band_result_MSB, uint32_t *band_result_LSB);
static void  CST_CMD_display_bitmap_name(uint32_t bitmap_MSB, uint32_t bitmap_LSB,
                                         const CST_band_descr_t *bitmap_descr);
#endif /* ( CST_CMD_MODEM_BG96 == 1 ) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
static void  CST_CMD_display_bitmap_name_sequans(void);
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */
#endif /* CST_CMD_USE_MODEM_CONFIG == 1 */


/* Private function Definition -----------------------------------------------*/

/**
  * @brief  Copy a text string to a destination string, and truncate text if destination string size is too short.
  * @param[in] text - the input text (usually hard coded text)
  * @param[out] str - The result string containing text truncated to str max size
  * @retval -
  */
static void CST_setTextToStr(const uint8_t *text, uint8_t *const str)
{
  uint8_t size;

  /* get size of hard coded text */
  size = (uint8_t)crs_strlen(text);
  /* if size greater than length of str, then truncate text size */
  if (size > (CST_MAX_STR_LEN - 1U))
  {
    /* set size to max possible length : max len minus '\0' */
    size = CST_MAX_STR_LEN - 1U;
  }
  /* copy text to str according to possible length */
  (void)memcpy(str, text, size);
  /* end copied string with a '\0' */
  str[size] = 0U;
}

/**
  * @brief  Converts integer state to an explicit human readable string
  * @param[in] state - the integer representing the state to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getModemStateStr(uint8_t state, uint8_t *const str)
{
  /* Modem state list  */
  switch (state)
  {
    case CA_MODEM_STATE_POWERED_ON:
      CST_setTextToStr((uint8_t *)"Modem powered on", str);
      break;
    case CA_MODEM_STATE_SIM_CONNECTED:
      CST_setTextToStr((uint8_t *)"Modem started and SIM is connected", str);
      break;
    case CA_MODEM_NETWORK_SEARCHING:
      CST_setTextToStr((uint8_t *)"Modem is searching network", str);
      break;
    case CA_MODEM_NETWORK_REGISTERED:
      CST_setTextToStr((uint8_t *)"Modem registered on network", str);
      break;
    case CA_MODEM_STATE_DATAREADY:
      CST_setTextToStr((uint8_t *)"Modem started and data is ready", str);
      break;
    case CA_MODEM_IN_FLIGHTMODE:
      CST_setTextToStr((uint8_t *)"Modem in flight mode", str);
      break;
    case CA_MODEM_REBOOTING:
      CST_setTextToStr((uint8_t *)"Modem is rebooting", str);
      break;
    case CA_MODEM_FOTA_INPROGRESS:
      CST_setTextToStr((uint8_t *)"Modem is in FOTA update", str);
      break;
    case CA_MODEM_POWER_OFF:
      CST_setTextToStr((uint8_t *)"Modem not started / power off", str);
      break;
    default:
      /* Nothing to do */
      __NOP();
      break;
  };
}

/**
  * @brief  Converts integer sim mode to an explicit human readable string
  * @param[in] state - the integer representing the sim mode to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getSimModeStr(uint8_t state, uint8_t *const str)
{
  /* Modem state list  */
  switch (state)
  {
    case CA_SIM_READY:
      CST_setTextToStr((uint8_t *)"SIM initialized and ready", str);
      break;
    case CA_SIM_STATUS_UNKNOWN:
      CST_setTextToStr((uint8_t *)"SIM status is not yet known", str);
      break;
    case CA_SIM_CONNECTION_ONGOING:
      CST_setTextToStr((uint8_t *)"SIM connection ongoing", str);
      break;
    case CA_SIM_PIN_OR_PUK_LOCKED:
      CST_setTextToStr((uint8_t *)"SIM locked with PIN or PUK", str);
      break;
    case CA_SIM_INCORRECT_PIN:
      CST_setTextToStr((uint8_t *)"SIM with an incorrect PIN password provided", str);
      break;
    case CA_SIM_BUSY:
      CST_setTextToStr((uint8_t *)"Sim busy - too many busy rsp during init", str);
      break;
    case CA_SIM_ERROR:
      CST_setTextToStr((uint8_t *)"SIM error - too many error rsp during init", str);
      break;
    case CA_SIM_NOT_INSERTED:
      CST_setTextToStr((uint8_t *)"Did not try any access to that sim", str);
      break;
    case CA_SIM_NOT_USED:
      CST_setTextToStr((uint8_t *)"Did not try any access to that sim slot", str);
      break;
    case CA_SIM_NOT_IMPLEMENTED:
      CST_setTextToStr((uint8_t *)"Not implemented on this hardware", str);
      break;
    default:
      /* Nothing to do */
      __NOP();
      break;
  };
}

/**
  * @brief  Help command management
  * @param  -
  * @retval -
  */
static void CST_HelpCmd(void)
{
  CMD_print_help(CST_cmd_label);
  PRINT_FORCE("%s help", (CRC_CHAR_t *)CST_cmd_label)
  PRINT_FORCE("%s state   (Displays the cellular and SIM state)", CST_cmd_label)
  PRINT_FORCE("%s config  (Displays the cellular configuration used)", CST_cmd_label)
  PRINT_FORCE("%s info    (Displays modem information)", CST_cmd_label)
  PRINT_FORCE("%s targetstate [off|sim|full|modem] (set modem state)", CST_cmd_label)
  PRINT_FORCE("%s polling [on|off]  (enable/disable periodical modem polling)", CST_cmd_label)
  PRINT_FORCE("%s cmd  (switch to command mode)", CST_cmd_label)
  PRINT_FORCE("%s data  (switch to data mode)", CST_cmd_label)
  PRINT_FORCE("%s apn [on|off] (use APN ON or OFF for active sim slot)", CST_cmd_label)
  PRINT_FORCE("%s apnconf [<apn> [<cid> [<username> <password>]]]  (update apn configuration of active sim slot)",
              CST_cmd_label)
  PRINT_FORCE("%s apnuser [on|off] (set apn user define for active sim slot)", CST_cmd_label)
  PRINT_FORCE("%s apnempty (Set APN to an empty string)", CST_cmd_label)
  PRINT_FORCE("%s power [on|off]  (modem switch ON or OFF)", CST_cmd_label)
  PRINT_FORCE("%s operator  (operator selection)", CST_cmd_label)
  PRINT_FORCE("%s techno off", CST_cmd_label)
  PRINT_FORCE("%s techno on [0 (GSM)|1 (GSM_COMPACT)|2 (UTRAN)|3(GSM EDGE)|4 (UTRAN HSDPA)|5 (UTRAN HSUPA)|\
                             6 (UTRAN HSDPA HSUPA)|7(E UTRAN)|8 (EC GSM IOT)|9 (E_UTRAN_NBS1)]", CST_cmd_label)
}

/**
  * @brief  Cellular Sercice Task command line management
  * @param  cmd_line_p  command line
  * @retval cmd_status_t command result
  */
static cmd_status_t CST_cmd(uint8_t *cmd_line_p)
{
  /* activated or not string */
  static uint8_t *CST_ActivateName_p[] =
  {
    (uint8_t *)"Not active",
    (uint8_t *)"Active"
  };

  /* Sim slot type list */
  static uint8_t *CST_SimSlotName_p[3] =
  {
    ((uint8_t *)"REMOVABLE SLOT"),
    ((uint8_t *)"EXTERNAL MODEM SLOT"),
    ((uint8_t *)"EXTERNAL MODEM SLOT")
  };

  /* Modem target state list  */
  static uint8_t *CST_TargetStateName_p[] =
  {
    ((uint8_t *)"OFF"),
    ((uint8_t *)"SIM_ONLY"),
    ((uint8_t *)"FULL"),
    ((uint8_t *)"MODEM_ONLY"),
  };


  /* To convert "network reg mode" integer code to a string human readable */
  static const uint8_t *CST_networkRegModeToString_p[4] =
  {
    ((uint8_t *)"Auto"),
    ((uint8_t *)"Manual"),
    ((uint8_t *)"Deregister"),
    ((uint8_t *)"Manual then auto")
  };
  /* To convert "operator name format" integer code to a string human readable */
  static const uint8_t *CST_operatorNameFormatToString_p[10] =
  {
    ((uint8_t *)"Long"),
    ((uint8_t *)"Short"),
    ((uint8_t *)"Numeric"),
    ((uint8_t *)""),
    ((uint8_t *)""),
    ((uint8_t *)""),
    ((uint8_t *)""),
    ((uint8_t *)""),
    ((uint8_t *)""),
    ((uint8_t *)"Not present")
  };
  /* To convert "access techno" integer code to a string human readable */
  static const uint8_t *CST_accessTechnoToString_p[10] =
  {
    ((uint8_t *)"GSM"),
    ((uint8_t *)"GSM compact"),
    ((uint8_t *)"UTRAN"),
    ((uint8_t *)"GSM edge"),
    ((uint8_t *)"UTRAN HSDPA"),
    ((uint8_t *)"UTRAN HSUPA"),
    ((uint8_t *)"UTRAN HSDPA HSUPA"),
    ((uint8_t *)"E-UTRAN"),
    ((uint8_t *)"EC GSM IOT"),
    ((uint8_t *)"E UTRAN NBS1")
  };

  static dc_cellular_info_t      cst_cmd_cellular_info;
  static dc_sim_info_t           cst_cmd_sim_info;
  static dc_cellular_params_t    cst_cmd_cellular_params;
  static dc_nfmc_info_t          cst_cmd_nfmc_info;
  static dc_cellular_target_state_t target_state;
  uint8_t   *argv_p[CST_CMS_PARAM_MAX];
  uint32_t  argc;
  uint8_t   *cmd_p;
  uint32_t  i;
  uint32_t  size;
  uint8_t   tmpConversion;
  uint8_t   myString[CST_MAX_STR_LEN];

  cellular_info_t p_my_cellular_info;
  cellular_signal_info_t p_my_signal_info;
  cellular_sim_info_t p_my_sim_info;

  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PRINT_FORCE("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  /* verify that it is a cst command */
  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CST_cmd_label,
               crs_strlen(cmd_p))
        == 0)
    {
      /* parameters parsing                     */
      for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
      {
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          /* end of argument list reached */
          break;
        }
      }

      if (argc == 0U)
      {
        /* no argument: displays help */
        CST_HelpCmd();
      }
      /* -- help ---------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],  "help",  crs_strlen(argv_p[0])) == 0)
      {
        /* help command: displays help */
        CST_HelpCmd();
      }
      /* -- pooling ------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "polling", crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst polling ...' command */
        if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* disables cst polling */
            CST_polling_active = false;
          }
          else
          {
            /* enables cst polling */
            CST_polling_active = true;
          }
        }

        /* displays cst polling state */
        if (CST_polling_active == 0U)
        {
          PRINT_FORCE("%s polling disable", CST_cmd_label)
        }
        else
        {
          PRINT_FORCE("%s polling enable", CST_cmd_label)
        }
      }
      /* -- targetstate --------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "targetstate", crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst targetstate ...' command */
        if (argc == 2U)
        {
          /* new mode mtarget state requested */
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to stop the modem. cmd acts as an application, so, use the API */
            (void)cellular_modem_stop();
            /* update target_state value that may have been modified in cellular_modem_stop */
            (void)dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "sim", crs_strlen(argv_p[1])) == 0)
          {
            /* 'cst targetstate sim' command: new modem state requested: modem manages sim but not data transfer */
            target_state.rt_state     = DC_SERVICE_ON;
            target_state.target_state = DC_TARGET_STATE_SIM_ONLY;
            target_state.callback = true;
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "full", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to start and attach the modem. cmd acts as an application, so, use the API */
            (void)cellular_connect();
            /* update target_state value that may have been modified in cellular_modem_start_and_connect */
            (void)dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));

          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "modem", crs_strlen(argv_p[1])) == 0)
          {
            /* 'cst targetstate modem only' command:  new modem state requested: modem manages full data transfer */
            target_state.rt_state     = DC_SERVICE_ON;
            target_state.target_state = DC_TARGET_STATE_MODEM_ONLY;
            target_state.callback = true;
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
          }
          else
          {
            /* Nothing to do */
          }
          PRINT_FORCE("New modem target state   : %s", CST_TargetStateName_p[target_state.target_state])
        }
      }
      /* -- apn ----------------------------------------------------------------------------------------------------- */
      /* command apn [on|off] */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apn", crs_strlen(argv_p[0])) == 0)
      {
        if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* disable use of APN */
            (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                              sizeof(cst_cellular_params));
            cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnSendToModem =
              (cellular_apn_send_to_modem_t)CA_APN_NOT_SEND_TO_MODEM;
            /* write new information to datacache */
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                               sizeof(cst_cellular_params));
          }
          if (memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0)
          {
            /* enables use of APN */
            (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                              sizeof(cst_cellular_params));
            cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnSendToModem =
              (cellular_apn_send_to_modem_t)CA_APN_SEND_TO_MODEM;
            /* write new information to datacache */
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                               sizeof(cst_cellular_params));
          }
        }

        /* displays cst polling state */
        if (cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnSendToModem ==
            (cellular_apn_send_to_modem_t)CA_APN_SEND_TO_MODEM)
        {
          PRINT_FORCE("%s apn set to on", CST_cmd_label)
        }
        else
        {
          PRINT_FORCE("%s apn set to off", CST_cmd_label)
        }
      }
      /* -- apnconf ------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apnconf", crs_strlen(argv_p[0])) == 0)
      {
        /* cst apnconf ...:  sets new apn configuration */

        /* reads current apn config */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                          sizeof(cst_cmd_cellular_params));
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_cmd_sim_info, sizeof(cst_cmd_sim_info));

        /* no username by default */
        cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].username[0] = 0U;
        cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].password[0] = 0U;

        if (argc >= 5U)
        {
          /* new password */
          size =  crs_strlen(argv_p[4]) + 1U;
          /* to avoid string overflow */
          if (size <= CA_PASSWORD_SIZE_MAX)
          {
            (void)memcpy(cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].password, argv_p[4], size);
          }
          else
          {
            /* Error: password string too long */
            PRINT_FORCE("password to long")
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }

        if (argc >= 4U)
        {
          /* new username */
          size =  crs_strlen(argv_p[3]) + 1U;
          /* to avoid string overflow */
          if (size <= CA_USERNAME_SIZE_MAX)
          {
            (void)memcpy(cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].username, argv_p[3], size);
          }
          else
          {
            /* Error: username string too long */
            PRINT_FORCE("username to long")
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
        if (argc >= 3U)
        {
          /* new cid */
          cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].cid = (uint8_t)crs_atoi(argv_p[2]);
        }

        if (argc >= 2U)
        {
          /* new apn */
          size =  crs_strlen(argv_p[1]) + 1U;
          /* to avoid string overflow */
          if (size <= CA_APN_SIZE_MAX)
          {
            (void)memcpy(cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].apn, argv_p[1], size);
            cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].apnChanged = true;
          }
          else
          {
            PRINT_FORCE("APN to long")
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }

        if (cmd_status == CMD_OK)
        {
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                             sizeof(cst_cmd_cellular_params));


          /* display the updated config */
          PRINT_FORCE("APN configuration values for the sim slot (%s):",
                      CST_SimSlotName_p[cst_cmd_sim_info.active_slot])
          PRINT_FORCE("APN                : %s", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].apn)
          PRINT_FORCE("CID                : %d", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].cid)
          PRINT_FORCE("username           : %s", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].username)
          PRINT_FORCE("password           : %s", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].password)
        }
      }
      /* -- apnuser ------------------------------------------------------------------------------------------------- */
      /* command apnuser
       [on|off] */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apnuser", crs_strlen(argv_p[0])) == 0)
      {
        if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* disable use of APN */
            (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                              sizeof(cst_cellular_params));
            cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnPresent = false;
            /* write new information to datacache */
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                               sizeof(cst_cellular_params));
          }
          if (memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0)
          {
            /* enables use of APN */
            (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                              sizeof(cst_cellular_params));
            cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnPresent = true;
            /* write new information to datacache */
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cellular_params,
                               sizeof(cst_cellular_params));
          }
        }

        /* displays cst apn user defined */
        if (cst_cellular_params.sim_slot[cst_context.sim_slot_index].apnPresent == true)
        {
          PRINT_FORCE("%s apn user set to on", CST_cmd_label)
        }
        else
        {
          PRINT_FORCE("%s apn user set to off", CST_cmd_label)
        }
      }
      /* -- apnempty ------------------------------------------------------------------------------------------------ */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apnempty", crs_strlen(argv_p[0])) == 0)
      {
        /* Set APN to empty string */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                          sizeof(cst_cmd_cellular_params));
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_cmd_sim_info, sizeof(cst_cmd_sim_info));

        cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].apn[0] = 0U;
        cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].apnChanged = true;
        /* display the updated config */
        PRINT_FORCE("APN configuration values for the sim slot (%s):",
                    CST_SimSlotName_p[cst_cmd_sim_info.active_slot])
        PRINT_FORCE("APN                : %s", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].apn)
        PRINT_FORCE("CID                : %d", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].cid)
        PRINT_FORCE("username           : %s", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].username)
        PRINT_FORCE("password           : %s", cst_cmd_cellular_params.sim_slot[cst_context.sim_slot_index].password)

        /* write new information to datacache */
        (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                           sizeof(cst_cmd_cellular_params));
      }
      /* -- state --------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "state",
                      crs_strlen(argv_p[0]))
               == 0)
      {
        /* 'cst state' command: displays cellular service state */
        PRINT_FORCE("Cellular Service Task State")

        /* Get SIM info from cellular API */
        cellular_get_sim_info(&p_my_sim_info);
        /* reads configuration in Data Cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                          sizeof(cst_cmd_cellular_params));
        PRINT_FORCE("Current State  : %s", CST_StateName[CST_get_state()])

        PRINT_FORCE("Sim Selected   : %s", CST_SimSlotName_p[p_my_sim_info.sim_slot_type[p_my_sim_info.sim_index]])
        for (i = 0; i < PLF_CELLULAR_SIM_SLOT_NB; i++)
        {
          CST_getSimModeStr((uint8_t)p_my_sim_info.sim_status[i], myString);
          PRINT_FORCE("Sim %s : %s", CST_SimSlotName_p[p_my_sim_info.sim_slot_type[i]], myString)
        }
        if (cst_cmd_cellular_params.nfmc_active != 0U)
        {
          /* NFMC feature active. Displays the temporisation list */
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_NFMC_INFO, (void *)&cst_cmd_nfmc_info, sizeof(cst_cmd_nfmc_info));
          for (i = 0U; i < CA_NFMC_VALUES_MAX_NB ; i++)
          {
            PRINT_FORCE("nfmc tempo %ld   : %ld", i + 1U, cst_cmd_nfmc_info.tempo[i])
          }
        }
      }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      /* -- data ---------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "data",
                      crs_strlen(argv_p[0]))
               == 0)
      {
        /* 'cst data' command: switch to data state */
        CS_Status_t cs_status ;
        cs_status = osCDS_resume_data();
        if (cs_status != CELLULAR_OK)
        {
          PRINT_FORCE("Switch to data state FAIL")
        }
        else
        {
          PRINT_FORCE("Switch to data state OK")
        }
      }
      /* -- cmd ----------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "cmd",
                      crs_strlen(argv_p[0]))
               == 0)
      {
        /* 'cst cmd' command: switch to cmd state */
        CS_Status_t cs_status ;
        cs_status = osCDS_suspend_data();
        if (cs_status != CELLULAR_OK)
        {
          PRINT_FORCE("Switch to cmd state FAIL")
        }
        else
        {
          PRINT_FORCE("Switch to cmd state OK")
        }
      }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
      /* -- valid --------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "valid",
                      crs_strlen(argv_p[0]))
               == 0)
      {
        if (memcmp((CRC_CHAR_t *)argv_p[1],
                   "netstate",
                   crs_strlen(argv_p[1]))
            == 0)
        {
          /* 'cst valid netstate'  automatic tests */

          /* reads configuration in Data Cache */
          TRACE_VALID("@valid@:cst:netstate:%s\n\r", CST_StateName[CST_get_state()])
        }

      }
      /* -- info ---------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "info",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst state' command: displays cellular service info supplied by modem */

        /* reads cellular info from cellular API */
        cellular_get_cellular_info(&p_my_cellular_info);
        /* reads signal info from cellular API */
        cellular_get_signal_info(&p_my_signal_info);
        /* reads more cellular info from Data Cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cmd_cellular_info, sizeof(cst_cmd_cellular_info));

        /* reads SIM info in Data Cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_cmd_sim_info, sizeof(cst_cmd_sim_info));
        PRINT_FORCE("Cellular Service Infos ")
        CST_getModemStateStr((uint8_t)p_my_cellular_info.modem_state, myString);
        PRINT_FORCE("Modem state          : %d (%s)", p_my_cellular_info.modem_state, myString)
        PRINT_FORCE("Signal Quality       : %d", p_my_signal_info.signal_strength.raw_value)
        PRINT_FORCE("Signal level(dBm)    : %ld", p_my_signal_info.signal_strength.db_value)

        PRINT_FORCE("Operator name        : %s", cst_cmd_cellular_info.mno_name)
        PRINT_FORCE("IMEI                 : %s", p_my_cellular_info.imei.value)
        PRINT_FORCE("Manuf name           : %s", p_my_cellular_info.identity.manufacturer_id.value)
        PRINT_FORCE("Model                : %s", p_my_cellular_info.identity.model_id.value)
        PRINT_FORCE("Revision             : %s", p_my_cellular_info.identity.revision_id.value)
        PRINT_FORCE("Serial Number        : %s", p_my_cellular_info.identity.serial_number_id.value)
        PRINT_FORCE("ICCID                : %s", cst_cmd_cellular_info.iccid)
        PRINT_FORCE("IMSI                 : %s", cst_cmd_sim_info.imsi)
        PRINT_FORCE("Network register mode: %d (%s)", p_my_signal_info.access_techno,
                    CST_accessTechnoToString_p[p_my_signal_info.access_techno]);

      }
      /* -- config -------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "config",
                      crs_strlen(argv_p[0]))
               == 0)
      {
        /* 'cst config' command: displays cellular configuration */
        PRINT_FORCE("Cellular Service Task Config")
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                          sizeof(cst_cmd_cellular_params));
        for (i = 0 ; i < cst_cmd_cellular_params.sim_slot_nb ; i++)
        {
          /* displays configuration for each sim stop defined */
          PRINT_FORCE("Sim Slot             : %ld (%s)", i,
                      CST_SimSlotName_p[cst_cmd_cellular_params.sim_slot[i].sim_slot_type])
          PRINT_FORCE("APN                  : \"%s\"", cst_cmd_cellular_params.sim_slot[i].apn)
          PRINT_FORCE("CID                  : %d", cst_cmd_cellular_params.sim_slot[i].cid)
          PRINT_FORCE("username             : %s", cst_cmd_cellular_params.sim_slot[i].username)
          PRINT_FORCE("password             : %s", cst_cmd_cellular_params.sim_slot[i].password)
        }

        PRINT_FORCE("Target state         : %s", CST_TargetStateName_p[cst_cmd_cellular_params.target_state])
        PRINT_FORCE("Attachment timeout   : %ld ms", cst_cmd_cellular_params.attachment_timeout)

        PRINT_FORCE("nfmc mode            : %s", CST_ActivateName_p[cst_cmd_cellular_params.nfmc_active])
        if (cst_cmd_cellular_params.nfmc_active != 0U)
        {
          /* NFMC feature active: displays list of temporisation values*/
          for (i = 0U; i < CA_NFMC_VALUES_MAX_NB ; i++)
          {
            PRINT_FORCE("nfmc value %ld       : %ld", i + 1U, cst_cmd_cellular_params.nfmc_value[i])
          }
        }
        PRINT_FORCE("Network register mode: %d = %s", cst_cmd_cellular_params.operator_selector.network_reg_mode,
                    CST_networkRegModeToString_p[cst_cmd_cellular_params.operator_selector.network_reg_mode]);
        if (cst_cmd_cellular_params.operator_selector.network_reg_mode != CA_NTW_REGISTRATION_AUTO)
        {
          PRINT_FORCE("Operator name format: %d (%s)",
                      cst_cmd_cellular_params.operator_selector.operator_name_format,
                      CST_operatorNameFormatToString_p[cst_cmd_cellular_params.operator_selector.operator_name_format]);
          PRINT_FORCE("Operator name: %s\n\r", cst_cmd_cellular_params.operator_selector.operator_name);
        }
        /* Access techno present or not */
        if (cst_cmd_cellular_params.operator_selector.access_techno_present == CA_ACT_PRESENT)
        {
          PRINT_FORCE("Access techno present: Present");
        }
        else
        {
          PRINT_FORCE("Access techno present: Not present");
        }
        if (cst_cmd_cellular_params.operator_selector.access_techno_present != CA_ACT_NOT_PRESENT)
        {
          PRINT_FORCE("Network register mode: %d (%s)", cst_cmd_cellular_params.operator_selector.access_techno,
                      CST_accessTechnoToString_p[cst_cmd_cellular_params.operator_selector.access_techno]);
        }
      }
      /* -- power --------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "power", crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst power ...' command */
        if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0)
          {
            /* power on the modem */
            PRINT_FORCE("modem power ON")
            if (osCDS_power_on() != CELLULAR_OK)
            {
              /* AT command processing failed */
              PRINT_FORCE("command fail\n\r")
              cmd_status = CMD_PROCESS_ERROR;
            }
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* power off the modem */
            PRINT_FORCE("modem power OFF")
            if (CST_modem_power_off() != CELLULAR_OK)
            {
              /* AT command processing failed */
              PRINT_FORCE("command fail\n\r")
              cmd_status = CMD_PROCESS_ERROR;
            }
          }
          else
          {
            /* nothing to do */
          }
        }
      }
      /* -- techno -------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "techno", crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst techno ...' command */
        if (argc == 3U)
        {
          /* get data from data cache */
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                            sizeof(cst_cmd_cellular_params));

          tmpConversion = (uint8_t)crs_atoi(argv_p[2]);
          /* tmpConversion is unsigned, no need to test its value is greater or equal than zero */
          if ((memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0) && (tmpConversion <= 9U))
          {
            /* Set access techno is present */
            cst_cmd_cellular_params.operator_selector.access_techno_present = CA_ACT_PRESENT;
            /* Set now the techno to use */
            cst_cmd_cellular_params.operator_selector.access_techno =
              cst_convert_access_techno((CS_AccessTechno_t)tmpConversion);
            /* Save data to data cache */
            (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                               sizeof(cst_cmd_cellular_params));
          }
          else
          {
            /* Bad cst command: displays help  */
            cmd_status = CMD_SYNTAX_ERROR;
            PRINT_FORCE("%s bad command. Usage:", cmd_p)
            CST_HelpCmd();
          }
        }
        else if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* Set access techno is not present : automatic mode use*/
            cst_cmd_cellular_params.operator_selector.access_techno_present = CA_ACT_NOT_PRESENT;
          }
          /* Save data to data cache */
          (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                             sizeof(cst_cmd_cellular_params));
        }
        else
        {
          /* Bad cst command: displays help  */
          cmd_status = CMD_SYNTAX_ERROR;
          PRINT_FORCE("%s bad command. Usage:", cmd_p)
          CST_HelpCmd();
        }
      }
      else
      {
        /* Bad cst command: displays help  */
        cmd_status = CMD_SYNTAX_ERROR;
        PRINT_FORCE("%s bad command. Usage:", cmd_p)
        CST_HelpCmd();
      }
    }
    else
    {
      /* Bad cst command: displays help  */
      cmd_status = CMD_SYNTAX_ERROR;
      PRINT_FORCE("Bad command. Usage:")
      CST_HelpCmd();
    }
  }
  return cmd_status;
}


#if ( CST_CMD_MODEM_BG96 == 1 )
/*---------------------------------------------------------*/
/* Specific command management for MODEM MONARCH and BG96 */
/*---------------------------------------------------------*/

/**
  * @brief  gets selected band from string
  * @param  band_descr - table containing the band description
  * @param  argv_p -         command arguments
  * @param  argc -           command arguments number
  * @param  band_result_MSB - (out) MSB value of matching band
  * @param  band_result_LSB - (out) LSB value of matching band
  * @retval uint32_t - result
  */
static uint32_t CST_CMD_get_band(const CST_band_descr_t *band_descr,
                                 const uint8_t   *const *argv_p, uint32_t argc,
                                 uint32_t *band_result_MSB, uint32_t *band_result_LSB)
{
  uint32_t i;
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint32_t band_value_MSB;
  uint32_t band_value_LSB;
  uint32_t ret;

  /* local variable init */
  ret = 0;
  band_value_MSB = 0;
  band_value_LSB = 0;

  nb_band = argc - 2U;

  for (j = 0U; j < nb_band; j++)
  {
    /* band argument begin at 2nd argument: add 2 as offset in argument number */
    current_arg = j + 2U;
    for (i = 0U ; band_descr[i].name != NULL ; i++)
    {
      /* find matching band in band_descr table */
      if (memcmp((const CRC_CHAR_t *)argv_p[current_arg],
                 (CRC_CHAR_t *)(band_descr[i].name),
                 crs_strlen((const uint8_t *)argv_p[current_arg]))
          == 0)
      {
        /* matching band found */
        band_value_MSB = band_value_MSB | band_descr[i].value_MSB;
        band_value_LSB = band_value_LSB | band_descr[i].value_LSB;
        break;
      }
    }

    if (band_descr[i].name == NULL)
    {
      /* no matching band found in band_descr */
      ret = 1;
    }
  }

  /* set output parameters (band value) */
  *band_result_MSB = band_value_MSB;
  *band_result_LSB = band_value_LSB;

  PRINT_FORCE("")
  return ret;
}

/**
  * @brief  displays the name associated to the bitmap
  * @param  bitmap_MSB - bitmap (MSB)
  * @param  bitmap_LSB - bitmap (LSB)
  * @param  bitmap_descr - bitmap description table
  * @retval -
  */
static void  CST_CMD_display_bitmap_name(uint32_t bitmap_MSB, uint32_t bitmap_LSB, const CST_band_descr_t *bitmap_descr)
{
  uint32_t i;
  uint32_t bmask_msb;
  uint32_t bmask_lsb;
  uint32_t bitmap_value_msb;
  uint32_t bitmap_value_lsb;
  uint8_t *bitmap_name;

  /* parse bitmap_descr to find all matching band */
  for (i = 0U; bitmap_descr[i].name != NULL ; i++)
  {
    /* bitmask of current band in bitmap_descr */
    bitmap_value_msb = bitmap_descr[i].value_MSB;
    bitmap_value_lsb = bitmap_descr[i].value_LSB;
    /* name of current band in bitmap_descr */
    bitmap_name  = bitmap_descr[i].name;

    /* apply mask between bitmask and current band mask */
    bmask_msb = bitmap_MSB & bitmap_value_msb;
    bmask_lsb = bitmap_LSB & bitmap_value_lsb;

    if (((bmask_msb != 0U) || (bmask_lsb != 0U)) &&
        (bmask_msb == bitmap_value_msb) &&
        (bmask_lsb == bitmap_value_lsb))
    {
      /*  result mask  != 0 =>  current band matching: display its name */
      PRINT_FORCE("%s", bitmap_name)
    }
  }
}
#endif   /*  CST_CMD_MODEM_BG96 == 1 ) */

#if (CST_CMD_MODEM_BG96 == 1)
/* MODEM BG96 command management */

/**
  * @brief  displays the sequence name associated to the bitmap
  * @param  bitmap  - bitmap
  * @param  bitmap_descr - bitmap description table
  * @retval -
  */
static void  CST_CMD_display_seq_name(uint32_t bitmap, const CST_seq_descr_t *bitmap_descr)
{
  uint32_t i;

  /* find matching bitmap in bitmap list  */
  for (i = 0U; bitmap_descr[i].name != NULL ; i++)
  {
    if (bitmap == bitmap_descr[i].value)
    {
      /* bitmap match with current value in bitmap_descr => displays sequence name */
      PRINT_FORCE("%s", bitmap_descr[i].name)
    }
  }
}

/**
  * @brief  displays help of modem commands
  * @param  -
  * @retval -
  */
static void CST_ModemHelpCmd(void)
{
  CMD_print_help(CST_cmd_modem_label);

  PRINT_FORCE("Modem configuration commands are used to modify the modem band configuration.")
  PRINT_FORCE("Setting a new configuration is performed in two steps:")
  PRINT_FORCE("\n\r");

  PRINT_FORCE("- 1st step: enter the configuration parameters using the following commands:");
  PRINT_FORCE("%s config iotopmode [M1|NB1|ALL]  sets iotop mode)", CST_cmd_modem_label)
  PRINT_FORCE("%s config nwscanmode [GSM|LTE|AUTO]  (sets scan mode)", CST_cmd_modem_label)
  PRINT_FORCE("%s config gsmband [900] [1800] [850] [1900] [nochange] [any]   (sets the list of GSM bands to use)",
              CST_cmd_modem_label)
  PRINT_FORCE("%s config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28] [B39]",
              CST_cmd_modem_label)
  PRINT_FORCE("                 [nochange] [any]  (sets the list of M1 bands to use)")
  PRINT_FORCE("%s config nb1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28]",
              CST_cmd_modem_label)
  PRINT_FORCE("                 [nochange] [any]  (sets the list of NB1 bands to use)")
  PRINT_FORCE("%s config scanseq GSM_NB1_M1|GSM_M1_NB1|M1_GSM_NB1|M1_NB1_GSM|NB1_GSM_M1|NB1_M1_GSM|GSM|M1|NB1",
              CST_cmd_modem_label)
  PRINT_FORCE("                  (sets the sequence order to scan)")
  PRINT_FORCE("\n\r");

  PRINT_FORCE("- 2nd step: send the new configuration to the modem");
  PRINT_FORCE("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("Other commands:");
  PRINT_FORCE("%s config get (get current config from modem)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("    (Note: the result of this command displays trace of modem response)")
  PRINT_FORCE("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("Notes:");
  PRINT_FORCE("- To use these commands, it is advised to start firmware in 'Modem power on' mode");
  PRINT_FORCE("       (option '2' of the boot menu).");
  PRINT_FORCE("- The new modem configuration is taken into account only after target reboot.");
  PRINT_FORCE("\n\r");
  PRINT_FORCE("Example:");
  PRINT_FORCE("to reduce M1 and NB1 bands and set M1 scan first, type the following commands:");
  PRINT_FORCE("modem config iotopmode ALL");
  PRINT_FORCE("modem config nwscanmode AUTO");
  PRINT_FORCE("modem config gsmband any");
  PRINT_FORCE("modem config m1band B13");
  PRINT_FORCE("modem config nb1band B4");
  PRINT_FORCE("modem config scanseq M1_GSM_NB1");
  PRINT_FORCE("modem config send");
  PRINT_FORCE("reset");
}

/**
  * @brief  modem command line management
  * @param  cmd_line_p - command line
  * @retval cmd_status_t - command result
  */
static cmd_status_t CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint8_t            cst_cmd_nwscanmode_default     = QCFGSCANMODE_AUTO;
  static uint8_t            cst_display_nwscanmode_default = CST_DISP_SCANMODE_AUTO;
  static uint32_t           cst_cmd_iotopmode_default      = QCFGIOTOPMODE_CATM1;
  static uint32_t           cst_cmd_gsmband_MSB_default    = 0;
  static uint32_t           cst_cmd_gsmband_LSB_default    = QCFGBANDGSM_ANY;
  static uint32_t           cst_cmd_m1band_MSB_default     = QCFGBANDCATM1_B4_MSB | QCFGBANDCATM1_B13_MSB;
  static uint32_t           cst_cmd_m1band_LSB_default     = QCFGBANDCATM1_B4_LSB | QCFGBANDCATM1_B13_LSB;
  static uint32_t           cst_cmd_nb1band_MSB_default    = QCFGBANDCATNB1_ANY_MSB;
  static uint32_t           cst_cmd_nb1band_LSB_default    = QCFGBANDCATNB1_ANY_LSB;
  static uint32_t           cst_cmd_scanseq_default        = 0x020301;
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];

  /* list of scan modes */
  static const uint8_t *CST_ScanmodeName_p[] =
  {
    ((uint8_t *)"AUTO"),
    ((uint8_t *)"GSM"),
    ((uint8_t *)"LTE")
  };

  /* list of iotop modes */
  static const uint8_t *CST_IotopmodeName_p[] =
  {
    ((uint8_t *)"M1"),
    ((uint8_t *)"NB1"),
    ((uint8_t *)"ALL")
  };

  /* list of NB1 bands */
  static const CST_band_descr_t CST_Nb1band[] =
  {
    /* name                   value_MSB                     value_LSB    */
    {((uint8_t *)"B1"),       QCFGBANDCATNB1_B1_MSB,        QCFGBANDCATNB1_B1_LSB},
    {((uint8_t *)"B2"),       QCFGBANDCATNB1_B2_MSB,        QCFGBANDCATNB1_B2_LSB},
    {((uint8_t *)"B3"),       QCFGBANDCATNB1_B3_MSB,        QCFGBANDCATNB1_B3_LSB},
    {((uint8_t *)"B4"),       QCFGBANDCATNB1_B4_MSB,        QCFGBANDCATNB1_B4_LSB},
    {((uint8_t *)"B5"),       QCFGBANDCATNB1_B5_MSB,        QCFGBANDCATNB1_B5_LSB},
    {((uint8_t *)"B8"),       QCFGBANDCATNB1_B8_MSB,        QCFGBANDCATNB1_B8_LSB},
    {((uint8_t *)"B12"),      QCFGBANDCATNB1_B12_MSB,       QCFGBANDCATNB1_B12_LSB},
    {((uint8_t *)"B13"),      QCFGBANDCATNB1_B13_MSB,       QCFGBANDCATNB1_B13_LSB},
    {((uint8_t *)"B18"),      QCFGBANDCATNB1_B18_MSB,       QCFGBANDCATNB1_B18_LSB},
    {((uint8_t *)"B19"),      QCFGBANDCATNB1_B19_MSB,       QCFGBANDCATNB1_B19_LSB},
    {((uint8_t *)"B20"),      QCFGBANDCATNB1_B20_MSB,       QCFGBANDCATNB1_B20_LSB},
    {((uint8_t *)"B26"),      QCFGBANDCATNB1_B26_MSB,       QCFGBANDCATNB1_B26_LSB},
    {((uint8_t *)"B28"),      QCFGBANDCATNB1_B28_MSB,       QCFGBANDCATNB1_B28_LSB},
    {((uint8_t *)"nochange"), QCFGBANDCATNB1_NOCHANGE_MSB,  QCFGBANDCATNB1_NOCHANGE_LSB},
    {((uint8_t *)"any"),      QCFGBANDCATNB1_ANY_MSB,       QCFGBANDCATNB1_ANY_LSB},
    {NULL,      0, 0}   /* Mandatory: End of table */
  };

  /* list of M1 bands */
  static const CST_band_descr_t CST_M1band[] =
  {
    /* name                   value_MSB                    value_LSB    */
    {((uint8_t *)"B1"),       QCFGBANDCATM1_B1_MSB,        QCFGBANDCATM1_B1_LSB},
    {((uint8_t *)"B2"),       QCFGBANDCATM1_B2_MSB,        QCFGBANDCATM1_B2_LSB},
    {((uint8_t *)"B3"),       QCFGBANDCATM1_B3_MSB,        QCFGBANDCATM1_B3_LSB},
    {((uint8_t *)"B4"),       QCFGBANDCATM1_B4_MSB,        QCFGBANDCATM1_B4_LSB},
    {((uint8_t *)"B5"),       QCFGBANDCATM1_B5_MSB,        QCFGBANDCATM1_B5_LSB},
    {((uint8_t *)"B8"),       QCFGBANDCATM1_B8_MSB,        QCFGBANDCATM1_B8_LSB},
    {((uint8_t *)"B12"),      QCFGBANDCATM1_B12_MSB,       QCFGBANDCATM1_B12_LSB},
    {((uint8_t *)"B13"),      QCFGBANDCATM1_B13_MSB,       QCFGBANDCATM1_B13_LSB},
    {((uint8_t *)"B18"),      QCFGBANDCATM1_B18_MSB,       QCFGBANDCATM1_B18_LSB},
    {((uint8_t *)"B19"),      QCFGBANDCATM1_B19_MSB,       QCFGBANDCATM1_B19_LSB},
    {((uint8_t *)"B20"),      QCFGBANDCATM1_B20_MSB,       QCFGBANDCATM1_B20_LSB},
    {((uint8_t *)"B26"),      QCFGBANDCATM1_B26_MSB,       QCFGBANDCATM1_B26_LSB},
    {((uint8_t *)"B28"),      QCFGBANDCATM1_B28_MSB,       QCFGBANDCATM1_B28_LSB},
    {((uint8_t *)"B39"),      QCFGBANDCATM1_B39_MSB,       QCFGBANDCATM1_B39_LSB},
    {((uint8_t *)"nochange"), QCFGBANDCATM1_NOCHANGE_MSB,  QCFGBANDCATM1_NOCHANGE_LSB},
    {((uint8_t *)"any"),      QCFGBANDCATM1_ANY_MSB,       QCFGBANDCATM1_ANY_LSB},
    {NULL,      0, 0}  /* Mandatory: End of table */
  };

  /* list of GSM bands */
  static const CST_band_descr_t CST_GSMband[] =
  {
    /* name                   value_MSB      value_LSB    */
    {((uint8_t *)"900"),      0,             QCFGBANDGSM_900,},
    {((uint8_t *)"1800"),     0,             QCFGBANDGSM_1800},
    {((uint8_t *)"850"),      0,             QCFGBANDGSM_850},
    {((uint8_t *)"1900"),     0,             QCFGBANDGSM_1900},
    {((uint8_t *)"nochange"), 0,             QCFGBANDGSM_NOCHANGE},
    {((uint8_t *)"any"),      0,             QCFGBANDGSM_ANY},
    {NULL,      0, 0}  /* Mandatory: End of table */
  };

  /* list of scan sequences mask */
  static const CST_seq_descr_t CST_Scanseq[CST_CMD_SCANSEQ_NUMBER] =
  {
    /* name                        mask   */
    {((uint8_t *)"GSM_M1_NB1"),    QCFGSCANSEQ_GSM_M1_NB1},
    {((uint8_t *)"GSM_NB1_M1"),    QCFGSCANSEQ_GSM_NB1_M1},
    {((uint8_t *)"M1_GSM_NB1"),    QCFGSCANSEQ_M1_GSM_NB1},
    {((uint8_t *)"M1_NB1_GSM"),    QCFGSCANSEQ_M1_NB1_GSM},
    {((uint8_t *)"NB1_GSM_M1"),    QCFGSCANSEQ_NB1_GSM_M1},
    {((uint8_t *)"NB1_M1_GSM"),    QCFGSCANSEQ_NB1_M1_GSM},
    {((uint8_t *)"NB1_M1"),        CST_scanseq_NB1_M1    },
    {((uint8_t *)"NB1_GSM"),       CST_scanseq_NB1_GSM   },
    {((uint8_t *)"M1_GSM"),        CST_scanseq_M1_GSM    },
    {((uint8_t *)"M1_NB1"),        CST_scanseq_M1_NB1    },
    {((uint8_t *)"GSM_M1"),        CST_scanseq_GSM_M1    },
    {((uint8_t *)"GSM_NB1"),       CST_scanseq_GSM_NB1   },
    {((uint8_t *)"NB1"),           CST_scanseq_NB1       },
    {((uint8_t *)"M1"),            CST_scanseq_M1        },
    {((uint8_t *)"GSM"),           CST_scanseq_GSM       },
    {NULL,    0}   /* Mandatory: End of table */
  };

  /* list of scan sequences name */
  static const uint8_t *CST_ScanseqName_p[CST_CMD_SCANSEQ_NUMBER] =
  {
    ((uint8_t *)"GSM_M1_NB1"),
    ((uint8_t *)"GSM_NB1_M1"),
    ((uint8_t *)"M1_GSM_NB1"),
    ((uint8_t *)"M1_NB1_GSM"),
    ((uint8_t *)"NB1_GSM_M1"),
    ((uint8_t *)"NB1_M1_GSM"),
    ((uint8_t *)"NB1_M1"),
    ((uint8_t *)"NB1_GSM"),
    ((uint8_t *)"M1_GSM"),
    ((uint8_t *)"M1_NB1"),
    ((uint8_t *)"GSM_M1"),
    ((uint8_t *)"GSM_NB1"),
    ((uint8_t *)"NB1"),
    ((uint8_t *)"M1"),
    ((uint8_t *)"GSM"),
    NULL
  };

  uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  uint32_t    ret;
  uint8_t    *cmd_p;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PRINT_FORCE("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CST_cmd_modem_label,
               crs_strlen(cmd_p)) == 0)
    {
      /* parameters parsing                     */

      for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
      {
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          /* end of argument list reached */
          break;
        }
      }

      if (argc == 0U)
      {
        /* no argument: displays help */
        CST_ModemHelpCmd();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen((uint8_t *)argv_p[0])) == 0)
      {
        /* help command: displays help */
        CST_ModemHelpCmd();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                      "config",
                      crs_strlen((uint8_t *)argv_p[0]))
               == 0)
      {
        /* 'modem config ...' command */
        if (argc == 1U)
        {
          /* no argument: displays current config */
          PRINT_FORCE("scanmode  : (mask=0x%08x) %s", cst_cmd_nwscanmode_default,
                      CST_ScanmodeName_p[cst_display_nwscanmode_default])
          PRINT_FORCE("iotopmode : (mask=0x%08lx) %s", cst_cmd_iotopmode_default,
                      CST_IotopmodeName_p[cst_cmd_iotopmode_default])
          PRINT_FORCE("GSM bands : (mask=0x%lx%08lx)", cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default, CST_GSMband);
          PRINT_FORCE("M1 bands  : (mask=0x%lx%08lx)", cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default, CST_M1band);
          PRINT_FORCE("NB1 bands : (mask=0x%lx%08lx)", cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default, CST_Nb1band);

          PRINT_FORCE("Scan seq : (mask=0x%06lx)", cst_cmd_scanseq_default)
          CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "nwscanmode",
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config nwscanmode ...' command */
          if (argc == 3U)
          {
            /* 'modem config nwscanmode ...' command */
            if (memcmp((const CRC_CHAR_t *)argv_p[2],
                       "AUTO",
                       crs_strlen((uint8_t *)argv_p[2]))
                == 0)
            {
              /* 'modem config nwscanmode AUTO' command: set AUTO in nwscanmode config */
              cst_cmd_nwscanmode_default = (uint8_t)QCFGSCANMODE_AUTO;
              cst_display_nwscanmode_default = CST_DISP_SCANMODE_AUTO;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "GSM",
                            crs_strlen((uint8_t *)argv_p[2]))
                     == 0)
            {
              /* 'modem config nwscanmode GSM' command */
              cst_cmd_nwscanmode_default = (uint8_t)QCFGSCANMODE_GSMONLY;
              cst_display_nwscanmode_default = CST_DISP_SCANMODE_GSM;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "LTE",
                            crs_strlen((uint8_t *)argv_p[2])) == 0)
            {
              /* 'modem config nwscanmode LTE' command */
              cst_cmd_nwscanmode_default = (uint8_t)QCFGSCANMODE_LTEONLY;
              cst_display_nwscanmode_default = CST_DISP_SCANMODE_LTE;
            }
            else
            {
              /* argument not matching: displays help */
              PRINT_FORCE("%s %s Bad parameter: %s", (CRC_CHAR_t *)CST_cmd_modem_label, argv_p[1], argv_p[2])
              PRINT_FORCE("Usage: %s config nwscanmode [GSM|LTE|AUTO]", CST_cmd_modem_label)
              cmd_status = CMD_SYNTAX_ERROR;
            }
          }

          /* display current nwscanmode */
          PRINT_FORCE("scanmode: %s\n\r", CST_ScanmodeName_p[cst_display_nwscanmode_default])
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "iotopmode",
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config iotopmode ...' command */
          if (argc == 3U)
          {
            if (memcmp((const CRC_CHAR_t *)argv_p[2],
                       "M1",
                       crs_strlen((uint8_t *)argv_p[2]))
                == 0)
            {
              /* 'modem config iotopmode M1' command */
              cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATM1;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "NB1",
                            crs_strlen((uint8_t *)argv_p[2]))
                     == 0)
            {
              /* 'modem config iotopmode NB1' command */
              cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATNB1;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "ALL",
                            crs_strlen((uint8_t *)argv_p[2]))
                     == 0)
            {
              /* 'modem config iotopmode ALL' command */
              cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATM1CATNB1;
            }
            else
            {
              /* argument not matching: displays help */
              PRINT_FORCE("%s %s Bad parameter: %s", (CRC_CHAR_t *)CST_cmd_modem_label, argv_p[1], argv_p[2])
              PRINT_FORCE("Usage: %s config iotopmode [M1|NB1|ALL]", CST_cmd_modem_label)
              cmd_status = CMD_SYNTAX_ERROR;
            }
          }

          /* display current iotopmode */
          PRINT_FORCE("iotopmode: %s\n\r", CST_IotopmodeName_p[cst_cmd_iotopmode_default])
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "gsmband",
                        crs_strlen((uint8_t *)argv_p[1]))
                 == 0)
        {
          /* 'modem config gsmband ...' command */
          if (argc >= 3U)
          {
            uint32_t gsmband_value_msb;
            uint32_t gsmband_value_lsb;

            /* get defined bands from the command line */
            ret = CST_CMD_get_band(CST_GSMband, (const uint8_t **)argv_p,  argc, &gsmband_value_msb,
                                   &gsmband_value_lsb);
            if (ret != 0U)
            {
              /* bad band argument: display help */
              cmd_status = CMD_SYNTAX_ERROR;
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage:%s config gsmband [900] [1800] [850] [1900] [nochange] [any]", CST_cmd_modem_label)
            }
            else
            {
              /* valid bands defined. Set requested bands in the current config */
              cst_cmd_gsmband_MSB_default = gsmband_value_msb;
              cst_cmd_gsmband_LSB_default = gsmband_value_lsb;
            }
          }
          /* display current GSM band list */
          PRINT_FORCE("Gsm Bands: (mask=0x%lx%08lx)\n\r", cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default, CST_GSMband);
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "m1band",
                        crs_strlen((uint8_t *)argv_p[1]))
                 == 0)
        {
          /* 'modem config m1band ...' command */
          if (argc >= 3U)
          {
            uint32_t m1band_value_msb;
            uint32_t m1band_value_lsb;

            /* get defined bands from the command line */
            ret = CST_CMD_get_band(CST_M1band, (const uint8_t **)argv_p,  argc, &m1band_value_msb, &m1band_value_lsb);
            if (ret != 0U)
            {
              /* bad band argument: display help */
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage:%s config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26]",
                          CST_cmd_modem_label)
              PRINT_FORCE("                       [B28] [B39] [nchanche] [any]")
              cmd_status = CMD_SYNTAX_ERROR;
            }
            else
            {
              /* valid bands defined. Set requested bands in the current config */
              cst_cmd_m1band_MSB_default = m1band_value_msb;
              cst_cmd_m1band_LSB_default = m1band_value_lsb;
            }
          }
          /* display current M1 band list */
          PRINT_FORCE("M1 Bands: (mask=0x%lx%08lx)\n\r", cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default, CST_M1band);
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "nb1band",
                        crs_strlen((uint8_t *)argv_p[1]))
                 == 0)
        {
          /* 'modem config nb1band ...' command */
          if (argc >= 3U)
          {
            uint32_t nb1band_value_msb;
            uint32_t nb1band_value_lsb;

            /* get defined bands from the command line */
            ret = CST_CMD_get_band(CST_Nb1band, (const uint8_t **)argv_p, argc, &nb1band_value_msb, &nb1band_value_lsb);
            if (ret != 0U)
            {
              /* bad band argument: display help */
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage: modem config nb1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18]")
              PRINT_FORCE("                            [B19] [B20] [B26] [B28] [nchanche] [any]")
              cmd_status = CMD_SYNTAX_ERROR;
            }
            else
            {
              /* valid bands defined. Set requested bands in the current config */
              cst_cmd_nb1band_MSB_default = nb1band_value_msb;
              cst_cmd_nb1band_LSB_default = nb1band_value_lsb;
            }
          }

          /* display current NB1 band list */
          PRINT_FORCE("NB1 bands: (mask=0x%lx%08lx)", cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default, CST_Nb1band);
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "scanseq",
                        crs_strlen((uint8_t *)argv_p[1]))
                 == 0)
        {
          /* 'modem config scanseq ...' command */
          if (argc == 2U)
          {
            /* no argument: displays current scanseq config */
            PRINT_FORCE("Scan Seq : (%06lx)", cst_cmd_scanseq_default)
            CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
          }
          else if (argc == 3U)
          {
            uint32_t i;
            for (i = 0U ; i < CST_CMD_SCANSEQ_NUMBER ; i++)
            {
              /* find matching scanseq mask */
              if (memcmp((const CRC_CHAR_t *)argv_p[2],
                         (const CRC_CHAR_t *)CST_ScanseqName_p[i],
                         crs_strlen(CST_ScanseqName_p[i]))
                  == 0)
              {
                /* matching scanseq mask found: sets it in the current config */
                cst_cmd_scanseq_default = CST_Scanseq[i].value;
                break;
              }
            }

            if (i == CST_CMD_SCANSEQ_NUMBER)
            {
              /* no matching scanseq mask found */
              cmd_status = CMD_SYNTAX_ERROR;
              PRINT_FORCE("bad command: %s %s %s\n\r", cmd_p, argv_p[1], argv_p[2])
            }
            else
            {
              /* display new scanseq config  */
              PRINT_FORCE("Scan seq : (0x%06lx)", cst_cmd_scanseq_default)
              CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
            }
          }
          else
          {
            /* Bad command syntax: displays help  */
            cmd_status = CMD_SYNTAX_ERROR;
            PRINT_FORCE("Too many parameters command: %s %s \n\r", cmd_p, argv_p[1])
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config send' command: send condif to the modem */
          /* send scanseg set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanmode\",%d,1", cst_cmd_nwscanmode_default);
          ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send iotopmode set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"iotopmode\",%ld,1", cst_cmd_iotopmode_default);
          ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send nwscanseq set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanseq\",%06lx,1", cst_cmd_scanseq_default);
          ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send band list set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"band\",%lx%08lx,%lx%08lx,%lx%08lx,1",
                        cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default,
                        cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default,
                        cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default);
          ret  |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
          if (ret != 0U)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
            cmd_status = CMD_PROCESS_ERROR;
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config get' command: get condif from the modem */
          /* send scanseg get AT command  */
          PRINT_FORCE("nwscanmode:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanmode\"");
          ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send iotopmode get AT command  */
          PRINT_FORCE("iotopmode:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"iotopmode\"");
          ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send gsmband get AT command  */
          PRINT_FORCE("GSM Bands:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"band\"");
          ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send scanseg get AT command  */
          PRINT_FORCE("Scan Seq:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanseq\"");
          ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
          if (ret != 0U)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
            cmd_status = CMD_PROCESS_ERROR;
          }
        }
        else
        {
          /* Bad command argument: displays help  */
          cmd_status = CMD_SYNTAX_ERROR;
          PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
        }
      }
      else
      {
        /* Bad command argument: displays help  */
        cmd_status = CMD_SYNTAX_ERROR;
        PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      /* Bad command argument: displays help  */
      cmd_status = CMD_SYNTAX_ERROR;
      PRINT_FORCE("bad command\n\r")
    }
  }

  PRINT_FORCE("")
  return cmd_status;

}
#endif /* defined(CST_CMD_MODEM_BG96 == 1) */

#if (CST_CMD_MODEM_TYPE1SC == 1)
/*-----------------------------------------------*/
/* Specific command management for ALTAIR Modem */
/*-----------------------------------------------*/

/**
  * @brief  gets selected band from string
  * @param  argv_p - command arguments
  * @param  argc -   command arguments number
  * @retval uint32_t - result
  */
static uint32_t CST_CMD_get_band_altair(const uint8_t *const *argv_p, uint32_t argc)
{
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint8_t  band_value;
  uint32_t ret;
  bool leave;

  /* local variable init */
  ret = 0U;
  nb_band = argc - 2U;

  CST_CMD_band_count = 0U;
  (void)memset(CST_CMD_band_tab, 0, sizeof(CST_CMD_band_tab));

  /* parse all bands */
  leave = false;
  for (j = 0U ; (j < nb_band) && (leave == false) ; j++)
  {
    /* band argument begin at 2nd argument: add 2 as offset in argument number */
    current_arg = j + 2U;
    if (argv_p[current_arg][0] == (uint8_t)'B')
    {
      band_value = (uint8_t)crs_atoi(&argv_p[current_arg][1]);
      if (band_value == 0U)
      {
        /* bad argument. Returns no bands found  */
        CST_CMD_band_count = 0U;
        leave = true;
      }
      else
      {
        /* band found: adding it in the band table */
        CST_CMD_band_tab[CST_CMD_band_count] = band_value;
        CST_CMD_band_count++;
      }
    }
    else
    {
      /* wrong syntax (band name don't start by 'B' */
      leave = true;
    }
  }

  if (CST_CMD_band_count == 0U)
  {
    /* no band found */
    ret = 1U;
  }

  return ret;
}

/**
  * @brief  displays the band list
  * @param  -
  * @retval -
  */
static void  CST_CMD_display_bitmap_name_altair(void)
{
  uint32_t i;

  for (i = 0U; i < CST_CMD_band_count  ; i++)
  {
    PRINT_FORCE("B%d", CST_CMD_band_tab[i])
  }
}

/**
  * @brief  Help of modem command management
  * @param  -
  * @retval -
  */
static void CST_ModemHelpCmd(void)
{
  CMD_print_help(CST_cmd_modem_label);

  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("1 - Modem band configuration commands")
  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("Modem configuration commands are used to modify the modem band configuration.")
  PRINT_FORCE("Setting a new configuration is performed in two steps:")
  PRINT_FORCE("\n\r");

  PRINT_FORCE("- 1st step: enter the configuration parameters using the following commands:");
  PRINT_FORCE("%s config band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B14] [B17] [B18] ", CST_cmd_modem_label);
  PRINT_FORCE("               [B19] [B20] [B26] [B28] [B39] [B66]  (gets/sets the bands to use(12 bands max))");
  PRINT_FORCE("\n\r");

  PRINT_FORCE("- 2nd step: send the new configuration to the modem");
  PRINT_FORCE("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("Other commands:");
  PRINT_FORCE("%s config get (get current config from modem)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("    (Note: the result of this command displays trace of modem response)")
  PRINT_FORCE("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("2 - Modem low power configuration     ")
  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("%s config lowpower  (configure modem with lowpower capabilities)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  PRINT_FORCE("------")
  PRINT_FORCE("Notes:");
  PRINT_FORCE("------")
  PRINT_FORCE("- To use these commands, it is advised to start firmware in 'Modem power on' mode");
  PRINT_FORCE("              (option '2' of the boot menu).");
  PRINT_FORCE("- The new modem configuration is taken into account only after target reboot.");
}


/**
  * @brief  modem command line management
  * @param  cmd_line_p - command line
  * @retval cmd_status_t - command result
  */
static cmd_status_t CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];

  const uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  uint32_t    ret;
  uint8_t     *cmd_p;
  uint8_t     i;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;

  PRINT_FORCE("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (memcmp((CRC_CHAR_t *)cmd_p,
             (CRC_CHAR_t *)CST_cmd_modem_label,
             crs_strlen(cmd_p)) == 0)
  {
    /* parameters parsing                     */

    for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        /* end of argument list reached */
        break;
      }
    }

    if (argc == 0U)
    {
      /* help command or no argument: displays help */
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
    {
      /* help command or no argument: displays help */
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                    "config",
                    crs_strlen(argv_p[0]))
             == 0)
    {
      /* 'modem config ...' command */
      if (argc == 1U)
      {
        PRINT_FORCE("bands : ")
        CST_CMD_display_bitmap_name_altair();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                      "bands",
                      crs_strlen(argv_p[1]))
               == 0)
      {
        /* 'modem config bands ...' command */
        if (argc >= 3U)
        {
          /* get defined bands from the command line */
          ret = CST_CMD_get_band_altair(argv_p,  argc);
          if (ret != 0U)
          {
            /*  parsing error: no band specified. Displays help  */
            PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
            PRINT_FORCE("Usage: config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B14] [B17]");
            PRINT_FORCE("                     [B18] [B19] [B20] [B26] [B28] [B39] [B66]  (12 bands max)");
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
        /* display current band list */
        CST_CMD_display_bitmap_name_altair();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
      {
        /* 'modem config send' command: send config to the modem */
        /* 1st create the AT command */
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETCFG=\"BAND\"");
        for (i = 0; i < CST_CMD_band_count; i++)
        {
          /* create the AT command to add the band */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "%s,\"%d\"", (CRC_CHAR_t *)CST_CMD_Command, CST_CMD_band_tab[i]);
        }

        /* send the AT command to modem */
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
        if (ret != 0U)
        {
          /* AT command processing failed */
          PRINT_FORCE("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "lowpower", crs_strlen(argv_p[1])) == 0)
      {
        /* set low power capabilities command  */
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETACFG=\"pm.hifc.mode,A\"");
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETACFG=\"pm.conf.sleep_mode,enable\"");
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETACFG=\"pm.conf.max_allowed_pm_mode,dh0\"");
        ret |= (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);

        if (ret != 0U)
        {
          /* AT command processing failed */
          PRINT_FORCE("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
        else
        {
          /* AT command processing OK */
          PRINT_FORCE("\n\r")
          PRINT_FORCE("Low power capabilities enabled\n\r")
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
      {
        /* 'modem config get' command: get condif from the modem */
        PRINT_FORCE("GSM Bands:")
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%GETCFG=\"BAND\"");
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
        if (ret != 0U)
        {
          /* AT command processing failed */
          PRINT_FORCE("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else
      {
        cmd_status = CMD_SYNTAX_ERROR;
        PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      /* Bad command argument: displays help  */
      cmd_status = CMD_SYNTAX_ERROR;
      PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
    }
  }

  PRINT_FORCE("")
  return cmd_status;
}
#endif /* defined(CST_CMD_MODEM_TYPE1SC == 1) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
/*------------------------------------------------*/
/* Specific command management for MODEM MONARCH */
/*------------------------------------------------*/

/**
  * @brief  Help of modem command management
  * @param  -
  * @retval -
  */
static void CST_ModemHelpCmd(void)
{
  CMD_print_help(CST_cmd_modem_label);

  PRINT_FORCE("Modem configuration commands are used to modify the modem band configuration.")
  PRINT_FORCE("Setting a new configuration is performed in two steps:")
  PRINT_FORCE("\n\r");

  PRINT_FORCE("- 1st step: enter the configuration parameters using the following commands:");
  PRINT_FORCE("%s config band  <1-256>...<1-256>   (gets/sets the bands to use)",
              CST_cmd_modem_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("- 2nd step: send the new configuration to the modem");
  PRINT_FORCE("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("Other command:");
  PRINT_FORCE("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PRINT_FORCE("\n\r");

  PRINT_FORCE("Notes:");
  PRINT_FORCE("- To use these commands, it is advised to start firmware in 'Modem power on' mode");
  PRINT_FORCE("              (option '2' of the boot menu).");
  PRINT_FORCE("- The new modem configuration is taken into account only after target reboot.");
}

/**
  * @brief  gets selected band from string
  * @param  argv_p - command arguments
  * @param  argc -   command arguments number
  * @retval uint32_t - result
  */
static uint32_t CST_CMD_get_band_sequans(const uint8_t *const *argv_p, uint32_t argc)
{
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint8_t  band_value;
  uint32_t ret;

  /* local variable init */
  ret = 0U;
  nb_band = argc - 2U;

  CST_CMD_band_count = 0U;
  (void)memset(CST_CMD_band_tab, 0, sizeof(CST_CMD_band_tab));

  for (j = 0U; j < nb_band; j++)
  {
    /* band argument begin at 2nd argument: add 2 as offset in argument number */
    current_arg = j + 2U;
    band_value = (uint8_t)crs_atoi(argv_p[current_arg]);
    if (band_value == 0U)
    {
      /* bad argument. Returns no bands found  */
      CST_CMD_band_count = 0;
      break;
    }

    /* band found: adding it in the band table */
    CST_CMD_band_tab[CST_CMD_band_count] = (band_value - 1U);
    CST_CMD_band_count++;
  }

  if (CST_CMD_band_count == 0U)
  {
    /* no band found */
    ret = 1U;
  }

  return ret;
}

/**
  * @brief  displays the band list
  * @param  -
  * @retval -
  */
static void  CST_CMD_display_bitmap_name_sequans(void)
{
  uint32_t i;

  /* display all bands of the string table */
  for (i = 0U; i < CST_CMD_band_count  ; i++)
  {
    PRINT_FORCE("%d", CST_CMD_band_tab[i] + 1U)
  }
}

/**
  * @brief  modem command line management
  * @param  cmd_line_p - command line
  * @retval cmd_status_t - command result
  */
static cmd_status_t CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];

  const uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  uint32_t    ret;
  uint8_t    *cmd_p;
  cmd_status_t cmd_status ;
  cmd_status = CMD_OK;
  uint32_t i;

  PRINT_FORCE("\n\r")

  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
  if (memcmp((CRC_CHAR_t *)cmd_p,
             (CRC_CHAR_t *)CST_cmd_modem_label,
             crs_strlen(cmd_p)) == 0)
  {
    /* parameters parsing                     */
    for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
    {
      argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
      if (argv_p[argc] == NULL)
      {
        /* end of argument list reached */
        break;
      }
    }

    if (argc == 0U)
    {
      /* no argument: displays help */
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
    {
      /* help command: displays help */
      CST_ModemHelpCmd();
    }
    else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                    "config",
                    crs_strlen(argv_p[0]))
             == 0)
    {
      /* 'modem config ...' command */
      if (argc == 1U)
      {
        /* no argument: only display default bands */
        PRINT_FORCE("bands : ")
        CST_CMD_display_bitmap_name_sequans();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                      "bands",
                      crs_strlen(argv_p[1]))
               == 0)
      {
        /* 'modem config band ...' command*/
        if (argc >= 3U)
        {
          /*  parse command to get specified bands  */
          ret = CST_CMD_get_band_sequans(argv_p,  argc);
          if (ret != 0U)
          {
            /*  parsing error: no band specified. Displays help  */
            PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
            PRINT_FORCE("Usage:%s config m1band <1-256>...<1-256>   (12 bands max)",
                        CST_cmd_modem_label)
            cmd_status = CMD_SYNTAX_ERROR;
          }
        }
        /* display selected bands*/
        CST_CMD_display_bitmap_name_sequans();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
      {
        /* send condif to the modem */
        /* Firstly: clear current modem configuration */
        ret = (uint32_t)cst_at_command_handle((uint8_t *)"AT!=\"clearscanconfig\"");
        if (ret != 0U)
        {
          /* AT command processing failed */
          PRINT_FORCE("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }

        /* secondary: send the list of the new bands */
        for (i = 0; i < CST_CMD_band_count; i++)
        {
          /* create the AT command to add the band */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT!=\"addScanBand band=%d\"", CST_CMD_band_tab[i] + 1U);
          /* send the AT command to the modem */
          ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
          if (ret != 0U)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
            cmd_status = CMD_PROCESS_ERROR;
            break;
          }
        }
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
      {
        /* get the current list of band from the modem */
        PRINT_FORCE("GSM Bands:")
        /* create the AT command to get list of bands */
        (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT!=addScanBand");
        /* send the AT command to the modem */
        ret = (uint32_t)cst_at_command_handle((uint8_t *)CST_CMD_Command);
        if (ret != 0U)
        {
          /* AT command processing failed */
          PRINT_FORCE("command fail\n\r")
          cmd_status = CMD_PROCESS_ERROR;
        }
      }
      else
      {
        /* Syntax error */
        cmd_status = CMD_SYNTAX_ERROR;
        PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      /* Syntax error */
      cmd_status = CMD_SYNTAX_ERROR;
      PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
    }
  }

  PRINT_FORCE("")
  return cmd_status;

}
#endif /* (CST_CMD_USE_MODEM_CELL_GM01Q == 1) */

/**
  * @brief  at processing callback
  * @param  direct_cmd_rx - rx command line
  * @retval -
  */
static void CST_cellular_direct_cmd_callback(CS_direct_cmd_rx_t direct_cmd_rx)
{
  UNUSED(direct_cmd_rx);
}


/**
  * @brief  displays help of atcmd commands
  * @param  -
  * @retval -
  */
static void cst_at_cmd_help(void)
{
  CMD_print_help(CST_cmd_at_label);

  PRINT_FORCE("%s timeout [<modem response timeout(ms) (default %d)>]", CST_cmd_at_label, CST_AT_TIMEOUT)
  PRINT_FORCE("%s <at command> (send an AT command to modem ex:atcmd AT+CSQ)", CST_cmd_at_label)
}

/**
  * @brief  AT command line processing
  * @param  cmd_line_p - command line
  * @retval cmd_status_t - command result
  */
static cmd_status_t cst_at_command_handle(uint8_t *cmd_line_p)
{
  uint32_t     size;
  cmd_status_t cmd_status;
  CS_Status_t  cs_status ;
  static CS_direct_cmd_tx_t CST_direct_cmd_tx;

  cmd_status = CMD_OK;

  size =  crs_strlen(cmd_line_p) + 1U;
  if (size <= MAX_DIRECT_CMD_SIZE)
  {
    (void)memcpy(&CST_direct_cmd_tx.cmd_str[0],
                 (CRC_CHAR_t *)cmd_line_p,
                 size);
  }

  CST_direct_cmd_tx.cmd_size    = (uint16_t)crs_strlen(cmd_line_p);
  CST_direct_cmd_tx.cmd_timeout = cst_at_timeout;

  /* send the AT command to the modem */
  cs_status = osCDS_direct_cmd(&CST_direct_cmd_tx, CST_cellular_direct_cmd_callback);
  if (cs_status != CELLULAR_OK)
  {
    /* AT command failed */
    PRINT_FORCE("\n\r%s command FAIL\n\r", cmd_line_p)
    cmd_status = CMD_PROCESS_ERROR;
  }
  return cmd_status;
}

/**
  * @brief  AT command line management
  * @param  cmd_line_p - command line
  * @retval cmd_status_t - command result
  */
static cmd_status_t CST_AtCmd(uint8_t *cmd_line_p)
{
  uint8_t  *argv_p[CST_CMS_PARAM_MAX];
  uint32_t i;
  uint32_t cmd_len;
  cmd_status_t cmd_status ;
  const uint8_t *cmd_p;

  cmd_status = CMD_OK;

  /* find an AT command */
  if (cmd_line_p != NULL)
  {
    cmd_len = crs_strlen(cmd_line_p);
    for (i = 0U ; i < cmd_len ; i++)
    {
      if (cmd_line_p[i] == (uint8_t)' ')
      {
        /* first blank found in the command line */
        break;
      }
    }
    i++;

    if (
      (i < cmd_len)
      &&
      (
        (memcmp((const CRC_CHAR_t *)&cmd_line_p[i], (const CRC_CHAR_t *)"at", 2) == 0)
        ||
        (memcmp((const CRC_CHAR_t *)&cmd_line_p[i], (const CRC_CHAR_t *)"AT", 2) == 0)
      )
    )
    {
      /* AT command to process */
      cmd_status = cst_at_command_handle(&cmd_line_p[i]);
    }
    else
    {
      /* Not an AT command */
      cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
      if (cmd_p != NULL)
      {
        if (memcmp((const CRC_CHAR_t *)cmd_p,
                   (const CRC_CHAR_t *)CST_cmd_at_label,
                   crs_strlen(cmd_p))
            == 0)
        {
          /* parameters parsing                     */
          argv_p[0] = (uint8_t *)strtok(NULL, " \t");
          if (argv_p[0] != NULL)
          {
            if (memcmp((CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
            {
              /* help command */
              cst_at_cmd_help();
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[0],
                            "timeout",
                            crs_strlen(argv_p[0]))
                     == 0)
            {
              /* timeout command */
              argv_p[1] = (uint8_t *)strtok(NULL, " \t");
              if (argv_p[1] != NULL)
              {
                /* timeout value */
                cst_at_timeout = (uint32_t)crs_atoi(argv_p[1]);
              }
              PRINT_FORCE("at timeout : %ld\n\r", cst_at_timeout)
            }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
            else if (memcmp((CRC_CHAR_t *)argv_p[0],
                            "suspenddata",
                            crs_strlen(argv_p[0]))
                     == 0)
            {
              /* suspenddata command: allows to switch modem to command mode */
              CS_Status_t cs_status ;
              cs_status = osCDS_suspend_data();
              if (cs_status != CELLULAR_OK)
              {
                PRINT_FORCE("\n\rsuspend data FAIL")
                cmd_status = CMD_PROCESS_ERROR;
              }
              else
              {
                PRINT_FORCE("\n\rsuspend data OK")
              }
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[0],
                            "resumedata",
                            crs_strlen(argv_p[0]))
                     == 0)
            {
              /* resumedata command: allows to switch modem data mode */
              CS_Status_t cs_status ;
              cs_status = osCDS_resume_data();
              if (cs_status != CELLULAR_OK)
              {
                PRINT_FORCE("\n\rresume data FAIL")
                cmd_status = CMD_PROCESS_ERROR;
              }
              else
              {
                PRINT_FORCE("\n\rresume data OK")
              }
            }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
            else
            {
              /* nothing to do */
            }
          }
        }
        else
        {
          /* no command matching: displays help */
          cst_at_cmd_help();
        }
      }
      else
      {
        /* no command matching: displays help */
        cst_at_cmd_help();
      }
    }
  }
  else
  {
    /* wrong command: displays help */
    cst_at_cmd_help();
  }
  return cmd_status;
}


/**
  * @brief  starts cellar command managememnt
  * @param  -
  * @retval CS_Status_t - function result
  */
CS_Status_t CST_cmd_cellular_service_start(void)
{
  CMD_Declare(CST_cmd_label, CST_cmd, (uint8_t *)"cellular service task management");
  CMD_Declare(CST_cmd_at_label, CST_AtCmd, (uint8_t *)"send an at command");
#if (CST_CMD_USE_MODEM_CONFIG == 1)
  CMD_Declare(CST_cmd_modem_label, CST_ModemCmd, (uint8_t *)"modem configuration management");
#endif  /* CST_CMD_USE_MODEM_CONFIG == 1 */

  return CELLULAR_OK;
}

#endif  /* (USE_CMD_CONSOLE == 1) */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

