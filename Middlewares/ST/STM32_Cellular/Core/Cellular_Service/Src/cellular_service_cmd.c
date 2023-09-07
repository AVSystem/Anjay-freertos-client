/**
  ******************************************************************************
  * @file    cellular_service_cmd.c
  * @author  MCD Application Team
  * @brief   This file defines cellular service console command
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

#define CST_ATCMD_SIZE_MAX      100U     /* AT CMD length max                 */
#define CST_AT_BAND_SIZE_MAX     10U     /* AT command 1 band part length max */
#define CST_CMS_PARAM_MAX        13U     /* number max of cmd param           */
#define CST_AT_TIMEOUT         5000U     /* default AT cmd response timeout   */


#if (CST_CMD_MODEM_BG96 == 1)
/* BG96 Modem constant definition */

/* To display scanmode config value */
/* Has to be consistent with the array CST_ScanmodeName_p */
#define CST_DISP_SCANMODE_AUTO 0U      /* Preferred network type : auto selection */
#define CST_DISP_SCANMODE_GSM 1U       /* Preferred network type : GSM only */
#define CST_DISP_SCANMODE_LTE 2U       /* Preferred network type : LTE only */

/* Number of bands */
#define CST_CMD_MAX_BAND        16U
#define CST_CMD_GSM_BAND_NUMBER  6U
#define CST_CMD_M1_BAND_NUMBER  16U
#define CST_CMD_NB1_BAND_NUMBER 15U
#define CST_CMD_SCANSEQ_NUMBER  16U

/* Bit mask of scan sequences */
#define  CST_SCANSEQ_NB1_M1  ((ATCustom_BG96_QCFGscanseq_t) 0x030202)
#define  CST_SCANSEQ_NB1_GSM ((ATCustom_BG96_QCFGscanseq_t) 0x030101)
#define  CST_SCANSEQ_M1_GSM  ((ATCustom_BG96_QCFGscanseq_t) 0x020101)
#define  CST_SCANSEQ_M1_NB1  ((ATCustom_BG96_QCFGscanseq_t) 0x020303)
#define  CST_SCANSEQ_GSM_M1  ((ATCustom_BG96_QCFGscanseq_t) 0x010202)
#define  CST_SCANSEQ_GSM_NB1 ((ATCustom_BG96_QCFGscanseq_t) 0x010303)

#define  CST_SCANSEQ_GSM     ((ATCustom_BG96_QCFGscanseq_t) 0x010101)
#define  CST_SCANSEQ_M1      ((ATCustom_BG96_QCFGscanseq_t) 0x020202)
#define  CST_SCANSEQ_NB1     ((ATCustom_BG96_QCFGscanseq_t) 0x030303)

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
#if (CST_CMD_MODEM_BG96 == 1)
/* Next defines are only used for BG96 */
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
#endif /* (CST_CMD_MODEM_BG96 == 1) */

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
/**
  * @brief  Converts integer access techno to an explicit human readable string
  * @param[in] state - the integer representing the access techno to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getAccessTechnoStr(uint8_t state, uint8_t *const str);

/**
  * @brief  Converts integer sim slot type to an explicit human readable string
  * @param[in] state - the integer representing the sim slot type to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getSimSlotTypeStr(uint8_t state, uint8_t *const str);

/**
  * @brief  Converts integer network registration mode to an explicit human readable string
  * @param[in] state - the integer representing the network registration mode to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getNetworkRegModeStr(uint8_t state, uint8_t *const str);

/**
  * @brief  Converts integer operator name format to an explicit human readable string
  * @param[in] state - the integer representing the operator name format to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getOperatorNameFormatStr(uint8_t state, uint8_t *const str);

/**
  * @brief  Converts integer operator name format to an explicit human readable string
  * @param[in] slotType - the integer representing the slot type to convert to ca_sim_slot_type_t
  * @retval - the slot type converted to ca_sim_slot_type_t
  */
static ca_sim_slot_type_t CST_simSlotTypeFromInt(uint8_t slotType);

/**
  * @brief  Cellular Service Task command line management
  * @param  cmd_line_p  command line
  * @retval -
  */
static void CST_cmd(uint8_t *cmd_line_p);

/**
  * @brief  AT command line processing
  * @param  cmd_line_p - command line
  * @retval bool - command result
  */
static bool cst_at_command_handle(uint8_t *cmd_line_p);

/**
  * @brief  Help command management
  * @param  -
  * @retval -
  */
static void CST_HelpCmd(void);

/**
  * @brief  displays help of atcmd commands
  * @param  -
  * @retval -
  */
static void cst_at_cmd_help(void);

#if (CST_CMD_USE_MODEM_CONFIG==1)
/**
  * @brief  displays help of modem commands
  * @param  -
  * @retval -
  */
static void CST_ModemHelpCmd(void);
#if ( CST_CMD_MODEM_BG96 == 1 )
/**
  * @brief  gets selected band from string
  * @param  band_descr - table containing the band description
  * @param  argv_p -         command arguments
  * @param  argc -           command arguments number
  * @param  band_result_MSB - (out) MSB value of matching band
  * @param  band_result_LSB - (out) LSB value of matching band
  * @retval bool - result
  */
static bool CST_CMD_get_band(const CST_band_descr_t *band_descr,
                             const uint8_t   *const *argv_p, uint32_t argc,
                             uint32_t *band_result_MSB, uint32_t *band_result_LSB);

/**
  * @brief  displays the name associated to the bitmap
  * @param  bitmap_MSB - bitmap (MSB)
  * @param  bitmap_LSB - bitmap (LSB)
  * @param  bitmap_descr - bitmap description table
  * @retval -
  */
static void  CST_CMD_display_bitmap_name(uint32_t bitmap_MSB, uint32_t bitmap_LSB,
                                         const CST_band_descr_t *bitmap_descr);
#endif /* ( CST_CMD_MODEM_BG96 == 1 ) */

#if (CST_CMD_USE_MODEM_CELL_GM01Q == 1)
/**
  * @brief  displays the name associated to the bitmap
  * @param  bitmap_MSB - bitmap (MSB)
  * @param  bitmap_LSB - bitmap (LSB)
  * @param  bitmap_descr - bitmap description table
  * @retval -
  */
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
  * @brief  Converts integer access techno to an explicit human readable string
  * @param[in] state - the integer representing the access techno to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getAccessTechnoStr(uint8_t state, uint8_t *const str)
{
  /* Modem state list  */
  switch (state)
  {
    case CA_ACT_GSM:
      CST_setTextToStr((uint8_t *)"GSM", str);
      break;
    case CA_ACT_E_UTRAN:
      CST_setTextToStr((uint8_t *)"E-UTRAN", str);
      break;
    case CA_ACT_E_UTRAN_NBS1:
      CST_setTextToStr((uint8_t *)"E UTRAN NBS1", str);
      break;
    default:
      CST_setTextToStr((uint8_t *)"Unknown Access techno", str);
      break;
  };
}

/**
  * @brief  Converts integer sim slot type to an explicit human readable string
  * @param[in] state - the integer representing the sim slot type to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getSimSlotTypeStr(uint8_t state, uint8_t *const str)
{
  /* Modem state list  */
  switch (state)
  {
    case CA_SIM_REMOVABLE_SLOT:
      CST_setTextToStr((uint8_t *)"REMOVABLE SLOT", str);
      break;
    case CA_SIM_EXTERNAL_MODEM_SLOT:
      CST_setTextToStr((uint8_t *)"EXTERNAL MODEM SLOT", str);
      break;
    case CA_SIM_INTERNAL_MODEM_SLOT:
      CST_setTextToStr((uint8_t *)"INTERNAL MODEM SLOT", str);
      break;
    case CA_SIM_NON_EXISTING_SLOT:
      CST_setTextToStr((uint8_t *)"Non existing slot", str);
      break;
    default:
      CST_setTextToStr((uint8_t *)"Unknown sim slot type", str);
      break;
  };
}

/**
  * @brief  Converts integer network registration mode to an explicit human readable string
  * @param[in] state - the integer representing the network registration mode to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getNetworkRegModeStr(uint8_t state, uint8_t *const str)
{
  /* Modem state list  */
  switch (state)
  {
    case CA_NTW_REGISTRATION_AUTO:
      CST_setTextToStr((uint8_t *)"Auto", str);
      break;
    case CA_NTW_REGISTRATION_MANUAL:
      CST_setTextToStr((uint8_t *)"Manual", str);
      break;
    case CA_NTW_REGISTRATION_DEREGISTER:
      CST_setTextToStr((uint8_t *)"Deregister", str);
      break;
    case CA_NTW_REGISTRATION_MANUAL_THEN_AUTO:
      CST_setTextToStr((uint8_t *)"Manual then auto", str);
      break;
    default:
      CST_setTextToStr((uint8_t *)"Unknown network registration mode", str);
      break;
  };
}

/**
  * @brief  Converts integer operator name format to an explicit human readable string
  * @param[in] state - the integer representing the operator name format to convert to string
  * @param[out] str  - The result string
  * @retval -
  */
static void CST_getOperatorNameFormatStr(uint8_t state, uint8_t *const str)
{
  /* Modem state list  */
  switch (state)
  {
    case CA_OPERATOR_NAME_FORMAT_LONG:
      CST_setTextToStr((uint8_t *)"Long", str);
      break;
    case CA_OPERATOR_NAME_FORMAT_SHORT:
      CST_setTextToStr((uint8_t *)"Short", str);
      break;
    case CA_OPERATOR_NAME_FORMAT_NUMERIC:
      CST_setTextToStr((uint8_t *)"Numeric", str);
      break;
    case CA_OPERATOR_NAME_FORMAT_NOT_PRESENT:
      CST_setTextToStr((uint8_t *)"Not present", str);
      break;
    default:
      CST_setTextToStr((uint8_t *)"Unknown operator name format", str);
      break;
  };
}

/**
  * @brief  Converts integer operator name format to an explicit human readable string
  * @param[in] slotType - the integer representing the slot type to convert to ca_sim_slot_type_t
  * @retval - the slot type converted to ca_sim_slot_type_t
  */
static ca_sim_slot_type_t CST_simSlotTypeFromInt(uint8_t slotType)
{
  ca_sim_slot_type_t ret; /* the return value */

  /* Set sim slot is not existing */
  ret = CA_SIM_NON_EXISTING_SLOT;
  if ((slotType == 0U) || (slotType == 1U) || (slotType == 2U) || (slotType == 99U))
  {
    /* Set now the sim slot to use */
    switch (slotType)
    {
      case 0:
        ret = CA_SIM_REMOVABLE_SLOT;
        break;
      case 1:
        ret = CA_SIM_EXTERNAL_MODEM_SLOT;
        break;
      case 2:
        ret = CA_SIM_INTERNAL_MODEM_SLOT;
        break;
      case 99:
        ret = CA_SIM_NON_EXISTING_SLOT;
        break;
      default:
        /* This case should never happen */
        ret = CA_SIM_NON_EXISTING_SLOT;
        break;
    }
  }
  return (ret);
}

/**
  * @brief  Help command management
  * @param  -
  * @retval -
  */
static void CST_HelpCmd(void)
{
  /* display common help header */
  CMD_print_help(CST_cmd_label);
  /* display help lines */
  PRINT_FORCE("%s help", (CRC_CHAR_t *)CST_cmd_label)
  PRINT_FORCE("%s state   (Displays the cellular and SIM state)", CST_cmd_label)
  PRINT_FORCE("%s config  (Displays the cellular configuration used)", CST_cmd_label)
  PRINT_FORCE("%s info    (Displays modem information)", CST_cmd_label)
  PRINT_FORCE("%s modem [off|sim|connected|modem|restart] (set modem state)", CST_cmd_label)
  PRINT_FORCE("%s polling [on|off]  (enable/disable periodical modem polling)", CST_cmd_label)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
  PRINT_FORCE("%s cmd  (switch to command mode)", CST_cmd_label)
  PRINT_FORCE("%s data  (switch to data mode)", CST_cmd_label)
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
  /* APN commands */
  PRINT_FORCE("%s simslotorder slot_type 1[ slot_type 2[ slot_type 3]] set sim slot priority order",  CST_cmd_label)
  PRINT_FORCE("%s   0 = Removable slot, 1 = e-sim, 2 = i-sim",  CST_cmd_label)
  PRINT_FORCE("%s apn [on|off] (use APN ON or OFF for active sim slot)", CST_cmd_label)
  PRINT_FORCE("%s apnconf [<apn> [<cid> [<username> <password>]]]  (update apn configuration of active sim slot)",
              CST_cmd_label)
  PRINT_FORCE("%s apnuser [on|off] (set apn user define for active sim slot)", CST_cmd_label)
  PRINT_FORCE("%s apnempty (Set APN to an empty string)", CST_cmd_label)
  /* End APN commands */
  PRINT_FORCE("%s operator  (operator selection)", CST_cmd_label)
  /* Techno commands */
  PRINT_FORCE("%s techno off", CST_cmd_label)
  PRINT_FORCE("%s techno on [ 0 (GSM) | 1 (E UTRAN) | 2 (E_UTRAN_NBS1) ]", CST_cmd_label)
  /* End techno commands */
}

/**
  * @brief  Config command management
  * @param  -
  * @retval -
  */
static void CST_ConfigCmd(void)
{
  cellular_operator_selection_t my_operator_selection;
  static dc_cellular_params_t   cst_cmd_cellular_params;
  uint8_t   myString[CST_MAX_STR_LEN];
  uint32_t  i;


  /* 'cst config' command: displays cellular configuration */
  PRINT_FORCE("Cellular Service Task Config")
  cellular_get_operator(&my_operator_selection);
  (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                    sizeof(cst_cmd_cellular_params));
  for (i = 0 ; i < cst_cmd_cellular_params.sim_slot_nb ; i++)
  {
    /* displays configuration for each sim slot defined */
    /* SIM slot type */
    CST_getSimSlotTypeStr((uint8_t)cst_cmd_cellular_params.sim_slot[i].sim_slot_type, myString);
    PRINT_FORCE("Sim Slot             : %ld (%s)", i, myString)
    /* APN */
    PRINT_FORCE("APN                  : \"%s\"", cst_cmd_cellular_params.sim_slot[i].apn)
    /* CID */
    PRINT_FORCE("CID                  : %d", cst_cmd_cellular_params.sim_slot[i].cid)
    /* APN user name */
    PRINT_FORCE("username             : %s", cst_cmd_cellular_params.sim_slot[i].username)
    /* APN password */
    PRINT_FORCE("password             : %s", cst_cmd_cellular_params.sim_slot[i].password)
  }
  /* Actual NFMC configuration */
  PRINT_FORCE("Attachment timeout   : %ld ms", cst_cmd_cellular_params.attachment_timeout)
  if (cst_cmd_cellular_params.nfmc_active == 0U)
  {
    PRINT_FORCE("nfmc mode            : %s", "Not active")
  }
  else if (cst_cmd_cellular_params.nfmc_active == 1U)
  {
    PRINT_FORCE("nfmc mode            : %s", "Active")
    /* NFMC feature active: displays list of temporisation values*/
    for (i = 0U; i < CA_NFMC_VALUES_MAX_NB ; i++)
    {
      PRINT_FORCE("nfmc value %ld       : %ld", i + 1U, cst_cmd_cellular_params.nfmc_value[i])
    }
  }
  else
  {
    PRINT_FORCE("nfmc mode            : %s", "Unknown value")
  }
  /* Operator registration configuration */
  CST_getNetworkRegModeStr((uint8_t)my_operator_selection.ntw_registration_mode, myString);
  PRINT_FORCE("Network register mode: %d = %s", my_operator_selection.ntw_registration_mode, myString);
  if (my_operator_selection.ntw_registration_mode != CA_NTW_REGISTRATION_AUTO)
  {
    /* If registration is not automatic, display also : */
    /*   Operator name format */
    CST_getOperatorNameFormatStr((uint8_t)my_operator_selection.operator_name_format, myString);
    PRINT_FORCE("Operator name format: %d (%s)", my_operator_selection.operator_name_format, myString);
    /*   Operator name */
    PRINT_FORCE("Operator name: %s\n\r", my_operator_selection.operator_name.value);
  }
  /* Access techno present or not */
  if (my_operator_selection.access_techno_present == CA_ACT_PRESENT)
  {
    PRINT_FORCE("Access techno present: Present");
    /* If access techno is present, display the techno to used */
    CST_getAccessTechnoStr((uint8_t)my_operator_selection.access_techno, myString);
    PRINT_FORCE("Network register mode: %d (%s)", my_operator_selection.access_techno, myString);
  }
  else
  {
    PRINT_FORCE("Access techno present: Not present");
  }
}

/**
  * @brief  Cellular Service Task command line management
  * @param  cmd_line_p  command line
  * @retval -
  */
static void CST_cmd(uint8_t *cmd_line_p)
{
  /* Define association arrays to display human readable string of internal data */
  /* Modem state list  */
  static uint8_t *CST_ModemStateName_p[] =
  {
    ((uint8_t *)"OFF"),        /* Modem off state */
    ((uint8_t *)"SIM"),        /* Modem SIM only mode */
    ((uint8_t *)"CONNECTED"),  /* Modem "full" connected mode */
    ((uint8_t *)"MODEM"),      /* Modem only mode */
  };

  /* Local variables */
  /* Data cache variables */
  static dc_cellular_info_t         cst_cmd_cellular_info;
  static dc_sim_info_t              cst_cmd_sim_info;
  static dc_cellular_params_t       cst_cmd_cellular_params;
  static dc_cellular_target_state_t target_state;
  /* Array of line parameters */
  uint8_t   *argv_p[CST_CMS_PARAM_MAX];
  /* Count of line parameters */
  uint32_t  argc;
  uint8_t   *cmd_p;
  uint32_t  i;
  uint32_t  size;
  uint8_t   tmpConversion;
  uint8_t   myString[CST_MAX_STR_LEN];
  uint8_t   myString2[CST_MAX_STR_LEN];
  /* Variables to use cellular API */
  cellular_info_t           my_cellular_info;
  cellular_signal_info_t    my_signal_info;
  cellular_sim_info_t       my_sim_info;
  cellular_sim_info_index_t my_sim_info_index;
  cellular_nfmc_info_t      my_nfmc_info;
  cellular_ip_info_t        my_ip_info;

  bool cmd_status;
  cmd_status = true;

  PRINT_FORCE("\n\r")

  /* Get first "word" of the command line. Here should be cst */
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  /* verify that it is a cst command */
  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CST_cmd_label,
               crs_strlen(cmd_p)) == 0)
    {
      /* First word is CST_cmd_label : "cst", now analyze the parameters */
      /* parameters parsing */
      for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
      {
        /* Fill argv_p, the parameter array */
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
      /* -- poling -------------------------------------------------------------------------------------------------- */
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
          /* Pooling is disable */
          PRINT_FORCE("%s polling disable", CST_cmd_label)
        }
        else
        {
          /* Pooling is enable */
          PRINT_FORCE("%s polling enable", CST_cmd_label)
        }
      }
      /* -- modem --------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "modem", crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst modem ...' command */
        if (argc == 2U)
        {
          /* new modem state requested */
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to stop the modem. cmd acts as an application, so, use the API */
            (void)cellular_modem_stop();
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "sim", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to set modem in sim only state. cmd acts as an application, so, use the API */
            (void)cellular_disconnect();
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "connected", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to start and attach the modem. cmd acts as an application, so, use the API */
            (void)cellular_connect();

          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to start the modem in power on only. cmd acts as an application, so, use the API */
            (void)cellular_modem_start();
          }
          else if (memcmp((CRC_CHAR_t *)argv_p[1], "restart", crs_strlen(argv_p[1])) == 0)
          {
            /* API call to stop the modem. cmd acts as an application, so, use the API */
            (void)cellular_modem_stop();
            /* API call to start and attach the modem. cmd acts as an application, so, use the API */
            (void)cellular_connect();
          }
          else
          {
            /* Unknown parameter, print an error */
            PRINT_FORCE("New modem requested state is unknown")
          }
          /* update target_state value that may have been modified in cellular_modem_start_and_connect */
          (void)dc_com_read(&dc_com_db, DC_CELLULAR_TARGET_STATE_CMD, (void *)&target_state, sizeof(target_state));
          PRINT_FORCE("New modem requested state   : %s", CST_ModemStateName_p[target_state.target_state])
        }
      }
      /* -- apn ----------------------------------------------------------------------------------------------------- */
      /* command apn [on|off] */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apn", crs_strlen(argv_p[0])) == 0)
      {
        cellular_sim_info_index_t sim_info_index;
        if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* disable use of APN */
            if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
            {
              sim_info_index.pdn.apn_send_to_modem = CA_APN_NOT_SEND_TO_MODEM;
              (void)cellular_set_pdn(sim_info_index.sim_slot_type, &(sim_info_index.pdn));
            }
            else
            {
              /* ERROR */
              PRINT_FORCE("Error with apn off")
            }
          }
          if (memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0)
          {
            /* enables use of APN */
            if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
            {
              sim_info_index.pdn.apn_send_to_modem = CA_APN_SEND_TO_MODEM;
              (void)cellular_set_pdn(sim_info_index.sim_slot_type, &(sim_info_index.pdn));
            }
            else
            {
              /* ERROR */
              PRINT_FORCE("Error with apn on")
            }
          }
        }

        if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
        {
          /* displays Sending APN to modem state */
          if (sim_info_index.pdn.apn_send_to_modem ==
              (cellular_apn_send_to_modem_t)CA_APN_SEND_TO_MODEM)
          {
            /* APN sent to modem */
            PRINT_FORCE("%s apn set to on", CST_cmd_label)
          }
          else
          {
            /* APN not sent to modem */
            PRINT_FORCE("%s apn set to off", CST_cmd_label)
          }
        }
        else
        {
          /* ERROR */
          PRINT_FORCE("Error with apn [on|off]")
        }
      }
      /* -- apnconf ------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apnconf", crs_strlen(argv_p[0])) == 0)
      {
        cellular_sim_info_index_t sim_info_index;
        /* cst apnconf ...:  sets new apn configuration */

        /* reads current apn config */
        if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
        {
          if (argc >= 5U)
          {
            /* new password */
            size =  crs_strlen(argv_p[4]) + 1U;
            /* to avoid string overflow */
            if (size <= CA_PASSWORD_SIZE_MAX)
            {
              /* Password string length is OK */
              sim_info_index.pdn.password.len = (uint8_t)(size - 1U);
              (void)memcpy(sim_info_index.pdn.password.value, argv_p[4], size);
            }
            else
            {
              /* Error: password string too long */
              PRINT_FORCE("password to long")
              cmd_status = false;
            }
          }

          if (argc >= 4U)
          {
            /* new username */
            size =  crs_strlen(argv_p[3]) + 1U;
            /* to avoid string overflow */
            if (size <= CA_USERNAME_SIZE_MAX)
            {
              /* Username string length is OK */
              sim_info_index.pdn.username.len = (uint8_t)(size - 1U);
              (void)memcpy(sim_info_index.pdn.username.value, argv_p[3], size);
            }
            else
            {
              /* Error: username string too long */
              PRINT_FORCE("username to long")
              cmd_status = false;
            }
          }
          if (argc >= 3U)
          {
            /* new cid */
            sim_info_index.pdn.cid = (uint8_t)crs_atoi(argv_p[2]);
          }

          if (argc >= 2U)
          {
            /* new apn */
            size =  crs_strlen(argv_p[1]) + 1U;
            /* to avoid string overflow */
            if (size <= CA_APN_SIZE_MAX)
            {
              /* APN string length is OK */
              sim_info_index.pdn.apn.len = (uint8_t)(size - 1U);
              (void)memcpy(sim_info_index.pdn.apn.value, argv_p[1], size);
            }
            else
            {
              /* Error: APN string too long */
              PRINT_FORCE("APN to long")
              cmd_status = false;
            }
          }
        }
        else
        {
          /* Error reading current APN config */
          PRINT_FORCE("ERROR reading actual APN configuration")
          cmd_status = false;
        }

        if (cmd_status == true)
        {
          cellular_result_t cellular_ret;
          /* If no error occurred before, write data to the data cache that apn has changed */
          cellular_ret = cellular_set_pdn(sim_info_index.sim_slot_type, &(sim_info_index.pdn));

          if (cellular_ret == CELLULAR_SUCCESS)
          {
            if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
            {
              /* display the updated config */
              /* display actual SIM slot used */
              CST_getSimSlotTypeStr((uint8_t)sim_info_index.sim_slot_type, myString);
              PRINT_FORCE("APN configuration values for the sim slot (%s):", myString)

              /* Display APN present */
              if (sim_info_index.pdn.apn_present == true)
              {
                PRINT_FORCE("user defined APN   : YES")
              }
              else
              {
                PRINT_FORCE("user defined APN   : NO")
              }
              /* Display APN name */
              PRINT_FORCE("APN                : %s", sim_info_index.pdn.apn.value)
              /* Display CID number */
              PRINT_FORCE("CID                : %d", sim_info_index.pdn.cid)
              /* Display APN used name */
              PRINT_FORCE("username           : %s", sim_info_index.pdn.username.value)
              /* Display APN password */
              PRINT_FORCE("password           : %s", sim_info_index.pdn.password.value)
            }
            else
            {
              PRINT_FORCE("Can't read apn configuration for display")
            }
          }
          else if (cellular_ret == CELLULAR_ERR_BADARGUMENT)
          {
            PRINT_FORCE("Can't set apn configuration. One of the parameters is our of range")
          }
          else
          {
            PRINT_FORCE("Can't set apn configuration. Internal error")
          }
        }
      }
      /* -- apnuser ------------------------------------------------------------------------------------------------- */
      /* command apnuser
       [on|off] */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apnuser", crs_strlen(argv_p[0])) == 0)
      {
        if (argc == 2U)
        {
          cellular_sim_info_index_t sim_info_index;
          cellular_result_t cellular_ret;
          bool parameter_ok = false;

          /* reads current apn config */
          if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
          {
            /* apnuser has one parameter. Now analyze the parameter */
            if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
            {
              sim_info_index.pdn.apn_present = false;
              parameter_ok = true;
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0)
            {
              sim_info_index.pdn.apn_present = true;
              parameter_ok = true;
            }
            else
            {
              PRINT_FORCE("Invalid apnuser parameter. See help")
            }
            if (parameter_ok == true)
            {
              /* write data to the data cache */
              cellular_ret = cellular_set_pdn(sim_info_index.sim_slot_type, &(sim_info_index.pdn));

              if (cellular_ret == CELLULAR_SUCCESS)
              {
                if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
                {
                  /* display the updated config */
                  /* display actual SIM slot used */
                  CST_getSimSlotTypeStr((uint8_t)sim_info_index.sim_slot_type, myString);
                  PRINT_FORCE("APN configuration values for the sim slot (%s):", myString)

                  /* Display APN present */
                  if (sim_info_index.pdn.apn_present == true)
                  {
                    PRINT_FORCE("user defined APN   : YES")
                  }
                  else
                  {
                    PRINT_FORCE("user defined APN   : NO")
                  }
                  /* Display APN name */
                  PRINT_FORCE("APN                : %s", sim_info_index.pdn.apn.value)
                  /* Display CID number */
                  PRINT_FORCE("CID                : %d", sim_info_index.pdn.cid)
                  /* Display APN used name */
                  PRINT_FORCE("username           : %s", sim_info_index.pdn.username.value)
                  /* Display APN password */
                  PRINT_FORCE("password           : %s", sim_info_index.pdn.password.value)
                }
                else
                {
                  PRINT_FORCE("Can't get apn configuration. Internal error")
                }
              }
              else if (cellular_ret == CELLULAR_ERR_BADARGUMENT)
              {
                PRINT_FORCE("Can't set apn configuration. One of the parameters is our of range")
              }
              else
              {
                PRINT_FORCE("Can't set apn configuration. Internal error")
              }
            }
          }
          else
          {
            PRINT_FORCE("Can't read apn configuration. Internal error")
          }
        }
      }
      /* -- apnempty ------------------------------------------------------------------------------------------------ */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "apnempty", crs_strlen(argv_p[0])) == 0)
      {
        cellular_sim_info_index_t sim_info_index;
        /* reads current apn config */
        if (cellular_get_sim_info_from_index(cst_context.sim_slot_index, &sim_info_index) == CELLULAR_SUCCESS)
        {
          /* Set APN to empty string */
          sim_info_index.pdn.apn.len = 0;
          sim_info_index.pdn.apn.value[0] = 0U;

          (void)cellular_set_pdn(sim_info_index.sim_slot_type, &(sim_info_index.pdn));

          /* Read needed data from control api */
          if (cellular_get_sim_info_from_index(cst_context.sim_slot_index,
                                               &sim_info_index)  == CELLULAR_SUCCESS)
          {
            /* display the updated config */
            /* display actual SIM slot used */
            CST_getSimSlotTypeStr((uint8_t)cst_cmd_sim_info.active_slot, myString);
            PRINT_FORCE("APN configuration values for the sim slot (%s):", myString)
            /* Display APN present */
            if (sim_info_index.pdn.apn_present == true)
            {
              PRINT_FORCE("user defined APN   : YES")
            }
            else
            {
              PRINT_FORCE("user defined APN   : NO")
            }
            /* Display APN name */
            PRINT_FORCE("APN                : %s", sim_info_index.pdn.apn.value)
            /* Display CID number */
            PRINT_FORCE("CID                : %d", sim_info_index.pdn.cid)
            /* Display APN used name */
            PRINT_FORCE("username           : %s", sim_info_index.pdn.username.value)
            /* Display APN password */
            PRINT_FORCE("password           : %s", sim_info_index.pdn.password.value)
          }
          else
          {
            PRINT_FORCE("Can't get apn configuration. Internal error")
          }
        }
        else
        {
          PRINT_FORCE("Can't get apn configuration. Internal error")
        }
      }
      /* -- simslotorder -------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "simslotorder", crs_strlen(argv_p[0])) == 0)
      {
        ca_sim_slot_type_t sim_slot_type[3U];
        uint8_t cst_cmd_nb_slots;
        cellular_result_t cellular_ret;

        cst_cmd_nb_slots = 0;
        /* Get and fill the third argument : the third slot in priority order */
        if (argc >= 4U)
        {
          sim_slot_type[2] = CST_simSlotTypeFromInt((uint8_t)crs_atoi(argv_p[3]));
          cst_cmd_nb_slots++;
        }
        /* Third argument do not exists, set third slot to non existing */
        else
        {
          sim_slot_type[2] = CA_SIM_NON_EXISTING_SLOT;
        }
        /* Get and fill the second argument : the second slot in priority order */
        if (argc >= 3U)
        {
          sim_slot_type[1] = CST_simSlotTypeFromInt((uint8_t)crs_atoi(argv_p[2]));
          cst_cmd_nb_slots++;
        }
        /* Second argument do not exists, set second slot to non existing */
        else
        {
          sim_slot_type[1] = CA_SIM_NON_EXISTING_SLOT;
        }
        /* Get and fill the first argument : the first slot in priority order */
        if (argc >= 2U)
        {
          sim_slot_type[0] = CST_simSlotTypeFromInt((uint8_t)crs_atoi(argv_p[1]));
          cst_cmd_nb_slots++;
        }

        if (cst_cmd_nb_slots > 0U)
        {
          cellular_ret = cellular_set_sim_slot_order(cst_cmd_nb_slots, sim_slot_type);
          if (cellular_ret == CELLULAR_ERR_BADARGUMENT)
          {
            PRINT_FORCE("Can't set sim slot order configuration. One of the parameters is our of range")
          }
          if (cellular_ret == CELLULAR_ERR_STATE)
          {
            PRINT_FORCE("Can't set sim slot order configuration. The actual cellular state is incompatible")
          }
          if (cellular_ret == CELLULAR_ERR_INTERNAL)
          {
            PRINT_FORCE("Can't set sim slot order configuration. Internal error")
          }
        }
        CST_ConfigCmd();
      }
      /* -- state --------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "state",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst state' command: displays cellular service state */
        PRINT_FORCE("Cellular Service Task State")

        /* reads configuration in Data Cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cst_cmd_cellular_params,
                          sizeof(cst_cmd_cellular_params));
        /* print actual state */
        PRINT_FORCE("Current State  : %s", CST_StateName[CST_get_state()])
        /* For each defined SIM slot, and in priority order, display : */
        /*   the slot interface */
        /*   the result of sim activation */
        /* Get current SIM info from cellular API */
        cellular_get_sim_info(&my_sim_info);
        CST_getSimModeStr((uint8_t)my_sim_info.sim_status, myString);
        /* print actual selected sim */
        CST_getSimSlotTypeStr((uint8_t)my_sim_info.sim_slot_type, myString2);
        PRINT_FORCE("Sim Selected : %s > %s", myString2, myString)
        /* For each defined SIM slot, and in priority order, display : */
        /*   the slot interface */
        /*   the result of sim activation */
        /*   the APN information */
        PRINT_FORCE("All sim information :")
        for (uint8_t n = 0U; n < my_sim_info.sim_slot_nb; n++)
        {
          if (cellular_get_sim_info_from_index(n, &my_sim_info_index) == CELLULAR_SUCCESS)
          {
            CST_getSimModeStr((uint8_t)my_sim_info_index.sim_status, myString);
            CST_getSimSlotTypeStr((uint8_t)my_sim_info_index.sim_slot_type, myString2);
            PRINT_FORCE("- Sim %s : %s", myString2, myString)
            if (my_sim_info_index.sim_status == CA_SIM_READY)
            {
              /* ICCID and IMSI are known only for sim initialized and ready */
              PRINT_FORCE("    ICCID : %s", my_sim_info.iccid.value)
              PRINT_FORCE("    IMSI : %s", my_sim_info.imsi.value)
            }
            PRINT_FORCE("    APN : %s", my_sim_info_index.pdn.apn.value)
            PRINT_FORCE("    APN username : %s", my_sim_info_index.pdn.username.value)
            PRINT_FORCE("    APN password : %s", my_sim_info_index.pdn.password.value)
            PRINT_FORCE("    CID : %d", my_sim_info_index.pdn.cid)
            if (my_sim_info_index.pdn.apn_send_to_modem == CA_APN_SEND_TO_MODEM)
            {
              PRINT_FORCE("    APN is sent to modem")
            }
            else
            {
              PRINT_FORCE("    APN is not sent to modem")
            }
          }
        }
        /* Display NFMC information */
        cellular_get_nfmc_info(&my_nfmc_info);
        if (my_nfmc_info.enable != 0U)
        {
          /* NFMC feature active. Displays the temporization list */
          /* Display all NFMC tempo */
          for (i = 0U; i < my_nfmc_info.tempo_nb ; i++)
          {
            PRINT_FORCE("nfmc tempo %ld   : %ld", i + 1U, my_nfmc_info.tempo_values[i])
          }
        }
        else
        {
          PRINT_FORCE("No nfmc tempo set");
        }
      }
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
      /* -- data ---------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "data",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst data' command: switch to data state */
        CS_Status_t cs_status ;
        cs_status = osCDS_resume_data();
        /* Check if resume data is OK */
        if (cs_status != CS_OK)
        {
          /* Data resume failed */
          PRINT_FORCE("Switch to data state FAIL")
        }
        else
        {
          /* Data resume success */
          PRINT_FORCE("Switch to data state OK")
        }
      }
      /* -- cmd ----------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "cmd",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst cmd' command: switch to cmd state : quit data state */
        CS_Status_t cs_status ;
        cs_status = osCDS_suspend_data();
        /* Check if switch to CMD state is OK */
        if (cs_status != CS_OK)
        {
          /* Data suspend failed */
          PRINT_FORCE("Switch to cmd state FAIL")
        }
        else
        {
          /* Data suspend success */
          PRINT_FORCE("Switch to cmd state OK")
        }
      }
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */
      /* -- valid --------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "valid",
                      crs_strlen(argv_p[0])) == 0)
      {
        if (memcmp((CRC_CHAR_t *)argv_p[1],
                   "netstate",
                   crs_strlen(argv_p[1])) == 0)
        {
          /* 'cst valid netstate' used with automatic tests */
          /* reads actual CST state, and display it in a particular format for automatic tests */
          TRACE_VALID("@valid@:cst:netstate:%s\n\r", CST_StateName[CST_get_state()])
        }

      }
      /* -- info ---------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "info",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst info' command: displays cellular service info supplied by modem */

        /* reads cellular info from cellular API */
        cellular_get_cellular_info(&my_cellular_info);
        /* reads signal info from cellular API */
        cellular_get_signal_info(&my_signal_info);
        /* reads IP address from cellular API */
        cellular_get_ip_info(&my_ip_info);
        /* reads more cellular info from Data Cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_INFO, (void *)&cst_cmd_cellular_info, sizeof(cst_cmd_cellular_info));

        /* reads SIM info in Data Cache */
        (void)dc_com_read(&dc_com_db, DC_CELLULAR_SIM_INFO, (void *)&cst_cmd_sim_info, sizeof(cst_cmd_sim_info));
        PRINT_FORCE("Cellular Service Infos ")
        CST_getModemStateStr((uint8_t)my_cellular_info.modem_state, myString);
        /* Dynamic modem related information */
        PRINT_FORCE("Modem state          : %d (%s)", my_cellular_info.modem_state, myString)
        PRINT_FORCE("Signal Quality       : %d", my_signal_info.signal_strength.raw_value)
        PRINT_FORCE("Signal level(dBm)    : %ld", my_signal_info.signal_strength.db_value)
        /* SIM related information */
        PRINT_FORCE("Operator name        : %s", cst_cmd_cellular_info.mno_name)
        PRINT_FORCE("IMEI                 : %s", my_cellular_info.imei.value)
        /* Modem hardware related information */
        PRINT_FORCE("Manuf name           : %s", my_cellular_info.identity.manufacturer_id.value)
        PRINT_FORCE("Model                : %s", my_cellular_info.identity.model_id.value)
        PRINT_FORCE("Revision             : %s", my_cellular_info.identity.revision_id.value)
        PRINT_FORCE("Serial Number        : %s", my_cellular_info.identity.serial_number_id.value)
        /* SIM hardware related information */
        PRINT_FORCE("ICCID                : %s", cst_cmd_cellular_info.iccid)
        PRINT_FORCE("IMSI                 : %s", cst_cmd_sim_info.imsi)
        /* Cellular access techno used */
        CST_getAccessTechnoStr((uint8_t)my_signal_info.access_techno, myString);
        PRINT_FORCE("Network bearer (AcT) : %d (%s)", my_signal_info.access_techno, myString)
        PRINT_FORCE("IP address           : %d.%d.%d.%d",
                    (uint8_t)(my_ip_info.ip_addr.addr & (uint8_t)0xFF),
                    (uint8_t)((my_ip_info.ip_addr.addr >> 8) & (uint8_t)0xFF),
                    (uint8_t)((my_ip_info.ip_addr.addr >> 16) & (uint8_t)0xFF),
                    (uint8_t)((my_ip_info.ip_addr.addr >> 24) & (uint8_t)0xFF))

      }
      /* -- config -------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0],
                      "config",
                      crs_strlen(argv_p[0])) == 0)
      {
        CST_ConfigCmd();
      }
      /* -- techno -------------------------------------------------------------------------------------------------- */
      else if (memcmp((CRC_CHAR_t *)argv_p[0], "techno", crs_strlen(argv_p[0])) == 0)
      {
        /* 'cst techno ...' command */
        cellular_operator_selection_t my_operator_selection;

        /* Get data */
        cellular_get_operator(&my_operator_selection);
        if (argc == 3U)
        {
          tmpConversion = (uint8_t)crs_atoi(argv_p[2]);
          /* tmpConversion is unsigned, no need to test its value is greater or equal than zero */
          if ((memcmp((CRC_CHAR_t *)argv_p[1], "on", crs_strlen(argv_p[1])) == 0) &&
              ((tmpConversion == 0U) || (tmpConversion == 1U) || (tmpConversion == 2U)))
          {
            /* Set access techno is present */
            my_operator_selection.access_techno_present = CA_ACT_PRESENT;
            /* Set now the techno to use */
            switch (tmpConversion)
            {
              case 0:
                my_operator_selection.access_techno = CA_ACT_GSM;
                break;
              case 1:
                my_operator_selection.access_techno = CA_ACT_E_UTRAN;
                break;
              case 2:
                my_operator_selection.access_techno = CA_ACT_E_UTRAN_NBS1;
                break;
              default:
                /* This case should never happen */
                my_operator_selection.access_techno = CA_ACT_GSM;
                break;
            }
            /* Print status */
            CST_getAccessTechnoStr((uint8_t)my_operator_selection.access_techno, myString);
            PRINT_FORCE("Network bearer (AcT) set to %d (%s)", my_operator_selection.access_techno, myString)
            PRINT_FORCE("  /!\\ Need to restart Modem to be apply. /!\\")
            /* Save data */
            (void)cellular_set_operator(&my_operator_selection);
          }
          else
          {
            /* Bad cst command: displays help  */
            PRINT_FORCE("%s bad command. Usage:", cmd_p)
            CST_HelpCmd();
          }
        }
        else if (argc == 2U)
        {
          if (memcmp((CRC_CHAR_t *)argv_p[1], "off", crs_strlen(argv_p[1])) == 0)
          {
            /* Set access techno is not present : automatic mode use*/
            my_operator_selection.access_techno_present = CA_ACT_NOT_PRESENT;
            /* Print status */
            PRINT_FORCE("Network bearer (AcT) not forced")
            PRINT_FORCE("  /!\\ Need to restart Modem to be apply. /!\\")
            /* Save data */
            (void)cellular_set_operator(&my_operator_selection);
          }
        }
        else
        {
          /* Bad cst command: displays help  */
          PRINT_FORCE("%s bad command. Usage:", cmd_p)
          CST_HelpCmd();
        }
      }
      else
      {
        /* Bad cst command: displays help  */
        PRINT_FORCE("%s bad command. Usage:", cmd_p)
        CST_HelpCmd();
      }
    }
    else
    {
      /* Bad cst command: displays help  */
      PRINT_FORCE("Bad command. Usage:")
      CST_HelpCmd();
    }
  }
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
  * @retval bool - result
  */
static bool CST_CMD_get_band(const CST_band_descr_t *band_descr,
                             const uint8_t   *const *argv_p, uint32_t argc,
                             uint32_t *band_result_MSB, uint32_t *band_result_LSB)
{
  /* Local variables */
  uint32_t i;
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint32_t band_value_MSB;
  uint32_t band_value_LSB;
  bool ret;

  /* local variable init */
  ret = true;
  band_value_MSB = 0;
  band_value_LSB = 0;

  nb_band = argc - 2U;

  /* Parse all the input bands */
  for (j = 0U; j < nb_band; j++)
  {
    /* band argument begin at 2nd argument: add 2 as offset in argument number */
    current_arg = j + 2U;
    for (i = 0U ; band_descr[i].name != NULL ; i++)
    {
      /* find matching band in band_descr table */
      if (memcmp((const CRC_CHAR_t *)argv_p[current_arg],
                 (CRC_CHAR_t *)(band_descr[i].name),
                 crs_strlen((const uint8_t *)argv_p[current_arg])) == 0)
      {
        /* matching band found, initialize the values to be used */
        band_value_MSB = band_value_MSB | band_descr[i].value_MSB;
        band_value_LSB = band_value_LSB | band_descr[i].value_LSB;
        break;
      }
    }

    if (band_descr[i].name == NULL)
    {
      /* no matching band found in band_descr */
      ret = false;
    }
  }

  /* set output parameters (band value) */
  *band_result_MSB = band_value_MSB;
  *band_result_LSB = band_value_LSB;

  PRINT_FORCE("")
  /* return if band has been found or not */
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
  /* Local variables */
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
      /*  result mask  != 0 => current band matching: display its name */
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
  /* Display help header */
  CMD_print_help(CST_cmd_modem_label);
  /* Display help lines */
  /* Introduction */
  /* ------------ */
  PRINT_FORCE("Modem configuration commands are used to modify the modem band configuration.")
  PRINT_FORCE("Setting a new configuration is performed in two steps:")
  PRINT_FORCE("\n\r");
  /* 1st step commands */
  /* ----------------- */
  PRINT_FORCE("- 1st step: enter the configuration parameters using the following commands:");
  /* iotop mode definition command */
  PRINT_FORCE("%s config iotopmode [M1|NB1|ALL]  sets iotop mode)", CST_cmd_modem_label)
  /* Scan mode definition command */
  PRINT_FORCE("%s config nwscanmode [GSM|LTE|AUTO]  (sets scan mode)", CST_cmd_modem_label)
  /* GSM band definition command */
  PRINT_FORCE("%s config gsmband [900] [1800] [850] [1900] [nochange] [any]   (sets the list of GSM bands to use)",
              CST_cmd_modem_label)
  /* M1 bands definition command */
  PRINT_FORCE("%s config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28] [B39]",
              CST_cmd_modem_label)
  PRINT_FORCE("                 [nochange] [any]  (sets the list of M1 bands to use)")
  /* NB1 band definition command */
  PRINT_FORCE("%s config nb1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26] [B28]",
              CST_cmd_modem_label)
  PRINT_FORCE("                 [nochange] [any]  (sets the list of NB1 bands to use)")
  /* Scan sequence command */
  PRINT_FORCE("%s config scanseq GSM_NB1_M1|GSM_M1_NB1|M1_GSM_NB1|M1_NB1_GSM|NB1_GSM_M1|NB1_M1_GSM|GSM|M1|NB1",
              CST_cmd_modem_label)
  PRINT_FORCE("                  (sets the sequence order to scan)")
  PRINT_FORCE("\n\r");
  /* 2nd step commands */
  /* ----------------- */
  PRINT_FORCE("- 2nd step: send the new configuration to the modem");
  /* Send config to modem */
  PRINT_FORCE("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  /* Other commands */
  /* -------------- */
  PRINT_FORCE("Other commands:");
  /* Command to display the actual config of the modem */
  PRINT_FORCE("%s config get (get current config from modem)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("    (Note: the result of this command displays trace of modem response)")
  /* Command to display the recoorded config that will be send with 'config send' */
  PRINT_FORCE("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  /* Example of usage */
  /* ---------------- */
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
  * @retval -
  */
static void CST_ModemCmd(uint8_t *cmd_line_p)
{
  /* Local variables */
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
    ((uint8_t *)"AUTO"),   /* Automatic selection */
    ((uint8_t *)"GSM"),    /* Force GSM mode      */
    ((uint8_t *)"LTE")     /* Force LTE mode      */
  };

  /* list of iotop modes */
  static const uint8_t *CST_IotopmodeName_p[] =
  {
    ((uint8_t *)"M1"),    /* M1 techno only     */
    ((uint8_t *)"NB1"),   /* NB1 techno only    */
    ((uint8_t *)"ALL")    /* may use any techno */
  };

  /* list of NB1 bands */
  static const CST_band_descr_t CST_Nb1band[] =
  {
    /* name                   value_MSB                     value_LSB                   Mhz   Common name             */
    {((uint8_t *)"B1"),       QCFGBANDCATNB1_B1_MSB,        QCFGBANDCATNB1_B1_LSB},  /* 1200  IMT                     */
    {((uint8_t *)"B2"),       QCFGBANDCATNB1_B2_MSB,        QCFGBANDCATNB1_B2_LSB},  /* 1900  PCS                     */
    {((uint8_t *)"B3"),       QCFGBANDCATNB1_B3_MSB,        QCFGBANDCATNB1_B3_LSB},  /* 1800  DCS                     */
    {((uint8_t *)"B4"),       QCFGBANDCATNB1_B4_MSB,        QCFGBANDCATNB1_B4_LSB},  /* 1700  AWS 1                   */
    {((uint8_t *)"B5"),       QCFGBANDCATNB1_B5_MSB,        QCFGBANDCATNB1_B5_LSB},  /*  850  Cellular (CLR)          */
    {((uint8_t *)"B8"),       QCFGBANDCATNB1_B8_MSB,        QCFGBANDCATNB1_B8_LSB},  /*  900  Extended GSM            */
    {((uint8_t *)"B12"),      QCFGBANDCATNB1_B12_MSB,       QCFGBANDCATNB1_B12_LSB}, /*  700  Lower SMH               */
    {((uint8_t *)"B13"),      QCFGBANDCATNB1_B13_MSB,       QCFGBANDCATNB1_B13_LSB}, /*  700  Upper SMH               */
    {((uint8_t *)"B18"),      QCFGBANDCATNB1_B18_MSB,       QCFGBANDCATNB1_B18_LSB}, /*  850  Lower 800 (Japan)       */
    {((uint8_t *)"B19"),      QCFGBANDCATNB1_B19_MSB,       QCFGBANDCATNB1_B19_LSB}, /*  850  Upper 800 (Japan)       */
    {((uint8_t *)"B20"),      QCFGBANDCATNB1_B20_MSB,       QCFGBANDCATNB1_B20_LSB}, /*  800  Digital Dividend (EU)   */
    {((uint8_t *)"B26"),      QCFGBANDCATNB1_B26_MSB,       QCFGBANDCATNB1_B26_LSB}, /*  850  Extended Cellular (CLR) */
    {((uint8_t *)"B28"),      QCFGBANDCATNB1_B28_MSB,       QCFGBANDCATNB1_B28_LSB}, /*  700  APT                     */
    {((uint8_t *)"nochange"), QCFGBANDCATNB1_NOCHANGE_MSB,  QCFGBANDCATNB1_NOCHANGE_LSB},
    {((uint8_t *)"any"),      QCFGBANDCATNB1_ANY_MSB,       QCFGBANDCATNB1_ANY_LSB},
    {NULL,      0, 0}   /* Mandatory: End of table */
  };

  /* list of M1 bands */
  static const CST_band_descr_t CST_M1band[] =
  {
    /* name                   value_MSB                    value_LSB                    Mhz   Common name             */
    {((uint8_t *)"B1"),       QCFGBANDCATM1_B1_MSB,        QCFGBANDCATM1_B1_LSB},    /* 1200  IMT                     */
    {((uint8_t *)"B2"),       QCFGBANDCATM1_B2_MSB,        QCFGBANDCATM1_B2_LSB},    /* 1900  PCS                     */
    {((uint8_t *)"B3"),       QCFGBANDCATM1_B3_MSB,        QCFGBANDCATM1_B3_LSB},    /* 1800  DCS                     */
    {((uint8_t *)"B4"),       QCFGBANDCATM1_B4_MSB,        QCFGBANDCATM1_B4_LSB},    /* 1700  AWS 1                   */
    {((uint8_t *)"B5"),       QCFGBANDCATM1_B5_MSB,        QCFGBANDCATM1_B5_LSB},    /*  850  Cellular (CLR)          */
    {((uint8_t *)"B8"),       QCFGBANDCATM1_B8_MSB,        QCFGBANDCATM1_B8_LSB},    /*  900  Extended GSM            */
    {((uint8_t *)"B12"),      QCFGBANDCATM1_B12_MSB,       QCFGBANDCATM1_B12_LSB},   /*  700  Lower SMH               */
    {((uint8_t *)"B13"),      QCFGBANDCATM1_B13_MSB,       QCFGBANDCATM1_B13_LSB},   /*  700  Upper SMH               */
    {((uint8_t *)"B18"),      QCFGBANDCATM1_B18_MSB,       QCFGBANDCATM1_B18_LSB},   /*  850  Lower 800 (Japan)       */
    {((uint8_t *)"B19"),      QCFGBANDCATM1_B19_MSB,       QCFGBANDCATM1_B19_LSB},   /*  850  Upper 800 (Japan)       */
    {((uint8_t *)"B20"),      QCFGBANDCATM1_B20_MSB,       QCFGBANDCATM1_B20_LSB},   /*  800  Digital Dividend (EU)   */
    {((uint8_t *)"B26"),      QCFGBANDCATM1_B26_MSB,       QCFGBANDCATM1_B26_LSB},   /*  850  Extended Cellular (CLR) */
    {((uint8_t *)"B28"),      QCFGBANDCATM1_B28_MSB,       QCFGBANDCATM1_B28_LSB},   /*  700  APT                     */
    {((uint8_t *)"B39"),      QCFGBANDCATM1_B39_MSB,       QCFGBANDCATM1_B39_LSB},   /* 1900  DCS IMT Gap             */
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
    {((uint8_t *)"GSM_M1_NB1"),    QCFGSCANSEQ_GSM_M1_NB1}, /* GSM then M1 then NB1 */
    {((uint8_t *)"GSM_NB1_M1"),    QCFGSCANSEQ_GSM_NB1_M1}, /* GSM then NB1 then M1 */
    {((uint8_t *)"M1_GSM_NB1"),    QCFGSCANSEQ_M1_GSM_NB1}, /* M1 then GSM then NB1 */
    {((uint8_t *)"M1_NB1_GSM"),    QCFGSCANSEQ_M1_NB1_GSM}, /* M1 then NB1 then GSM */
    {((uint8_t *)"NB1_GSM_M1"),    QCFGSCANSEQ_NB1_GSM_M1}, /* NB1 then GSM then M1 */
    {((uint8_t *)"NB1_M1_GSM"),    QCFGSCANSEQ_NB1_M1_GSM}, /* NB1 then M1 then GSM */
    {((uint8_t *)"NB1_M1"),        CST_SCANSEQ_NB1_M1    }, /* NB1 then M1  */
    {((uint8_t *)"NB1_GSM"),       CST_SCANSEQ_NB1_GSM   }, /* NB1 then GSM */
    {((uint8_t *)"M1_GSM"),        CST_SCANSEQ_M1_GSM    }, /* M1 then GSM  */
    {((uint8_t *)"M1_NB1"),        CST_SCANSEQ_M1_NB1    }, /* M1 then NB1  */
    {((uint8_t *)"GSM_M1"),        CST_SCANSEQ_GSM_M1    }, /* GSM then M1  */
    {((uint8_t *)"GSM_NB1"),       CST_SCANSEQ_GSM_NB1   }, /* GSM then NB1 */
    {((uint8_t *)"NB1"),           CST_SCANSEQ_NB1       }, /* MB1 only */
    {((uint8_t *)"M1"),            CST_SCANSEQ_M1        }, /* M1 only  */
    {((uint8_t *)"GSM"),           CST_SCANSEQ_GSM       }, /* GSM only */
    {NULL,    0}   /* Mandatory: End of table */
  };

  /* list of scan sequences name */
  static const uint8_t *CST_ScanseqName_p[CST_CMD_SCANSEQ_NUMBER] =
  {
    ((uint8_t *)"GSM_M1_NB1"), /* GSM then M1 then NB1 */
    ((uint8_t *)"GSM_NB1_M1"), /* GSM then NB1 then M1 */
    ((uint8_t *)"M1_GSM_NB1"), /* M1 then GSM then NB1 */
    ((uint8_t *)"M1_NB1_GSM"), /* M1 then NB1 then GSM */
    ((uint8_t *)"NB1_GSM_M1"), /* NB1 then GSM then M1 */
    ((uint8_t *)"NB1_M1_GSM"), /* NB1 then M1 then GSM */
    ((uint8_t *)"NB1_M1"),     /* NB1 then M1  */
    ((uint8_t *)"NB1_GSM"),    /* NB1 then GSM */
    ((uint8_t *)"M1_GSM"),     /* M1 then GSM  */
    ((uint8_t *)"M1_NB1"),     /* M1 then NB1  */
    ((uint8_t *)"GSM_M1"),     /* GSM then M1  */
    ((uint8_t *)"GSM_NB1"),    /* GSM then NB1 */
    ((uint8_t *)"NB1"),        /* MB1 only */
    ((uint8_t *)"M1"),         /* M1 only  */
    ((uint8_t *)"GSM"),        /* GSM only */
    NULL  /* Mandatory: End of table */
  };

  /* Local variables */
  uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  bool        ret;
  uint8_t    *cmd_p;

  PRINT_FORCE("\n\r")

  /* get the first "word" of the command line. here, it should be CST_cmd_modem_label = "modem" */
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CST_cmd_modem_label,
               crs_strlen(cmd_p)) == 0)
    {
      /* Cmd first word command is CST_cmd_modem_label (= "modem"). Now, parse parameters */
      for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
      {
        /* fill the array of parameters, and count the parameter number */
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
        /* help command: displays help -------------------------------------------------------------------------------*/
        CST_ModemHelpCmd();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                      "config",
                      crs_strlen((uint8_t *)argv_p[0])) == 0)
      {
        /* 'modem config ...' command --------------------------------------------------------------------------------*/
        if (argc == 1U)
        {
          /* no argument: displays current config */
          /* display scan mode*/
          PRINT_FORCE("scanmode  : (mask=0x%08x) %s", cst_cmd_nwscanmode_default,
                      CST_ScanmodeName_p[cst_display_nwscanmode_default])
          /* Display iotop mode */
          PRINT_FORCE("iotopmode : (mask=0x%08lx) %s", cst_cmd_iotopmode_default,
                      CST_IotopmodeName_p[cst_cmd_iotopmode_default])
          /* Display GSM bands */
          PRINT_FORCE("GSM bands : (mask=0x%lx%08lx)", cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default, CST_GSMband);
          /* Display M1 bands */
          PRINT_FORCE("M1 bands  : (mask=0x%lx%08lx)", cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default, CST_M1band);
          /* Display NB1 bands */
          PRINT_FORCE("NB1 bands : (mask=0x%lx%08lx)", cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default)
          CST_CMD_display_bitmap_name(cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default, CST_Nb1band);
          /* Display Scan sequence */
          PRINT_FORCE("Scan seq : (mask=0x%06lx)", cst_cmd_scanseq_default)
          CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "nwscanmode",
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config nwscanmode ...' command -------------------------------------------------------------------*/
          if (argc == 3U)
          {
            /* 'modem config nwscanmode ...' command */
            if (memcmp((const CRC_CHAR_t *)argv_p[2],
                       "AUTO",
                       crs_strlen((uint8_t *)argv_p[2])) == 0)
            {
              /* 'modem config nwscanmode AUTO' command: set AUTO in nwscanmode config */
              cst_cmd_nwscanmode_default = (uint8_t)QCFGSCANMODE_AUTO;
              cst_display_nwscanmode_default = CST_DISP_SCANMODE_AUTO;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "GSM",
                            crs_strlen((uint8_t *)argv_p[2])) == 0)
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
            }
          }

          /* display current nwscanmode */
          PRINT_FORCE("scanmode: %s\n\r", CST_ScanmodeName_p[cst_display_nwscanmode_default])
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "iotopmode",
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config iotopmode ...' command --------------------------------------------------------------------*/
          if (argc == 3U)
          {
            if (memcmp((const CRC_CHAR_t *)argv_p[2],
                       "M1",
                       crs_strlen((uint8_t *)argv_p[2])) == 0)
            {
              /* 'modem config iotopmode M1' command */
              cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATM1;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "NB1",
                            crs_strlen((uint8_t *)argv_p[2])) == 0)
            {
              /* 'modem config iotopmode NB1' command */
              cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATNB1;
            }
            else if (memcmp((const CRC_CHAR_t *)argv_p[2],
                            "ALL",
                            crs_strlen((uint8_t *)argv_p[2])) == 0)
            {
              /* 'modem config iotopmode ALL' command */
              cst_cmd_iotopmode_default = QCFGIOTOPMODE_CATM1CATNB1;
            }
            else
            {
              /* argument not matching: displays help */
              PRINT_FORCE("%s %s Bad parameter: %s", (CRC_CHAR_t *)CST_cmd_modem_label, argv_p[1], argv_p[2])
              PRINT_FORCE("Usage: %s config iotopmode [M1|NB1|ALL]", CST_cmd_modem_label)
            }
          }

          /* display current iotopmode */
          PRINT_FORCE("iotopmode: %s\n\r", CST_IotopmodeName_p[cst_cmd_iotopmode_default])
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "gsmband",
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config gsmband ...' command ----------------------------------------------------------------------*/
          if (argc >= 3U)
          {
            uint32_t gsmband_value_msb;
            uint32_t gsmband_value_lsb;

            /* get defined bands from the command line */
            ret = CST_CMD_get_band(CST_GSMband, (const uint8_t **)argv_p,  argc, &gsmband_value_msb,
                                   &gsmband_value_lsb);
            if (ret != true)
            {
              /* bad band argument: display help */
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
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config m1band ...' command -----------------------------------------------------------------------*/
          if (argc >= 3U)
          {
            uint32_t m1band_value_msb;
            uint32_t m1band_value_lsb;

            /* get defined bands from the command line */
            ret = CST_CMD_get_band(CST_M1band, (const uint8_t **)argv_p,  argc, &m1band_value_msb, &m1band_value_lsb);
            if (ret != true)
            {
              /* bad band argument: display help */
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage:%s config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18] [B19] [B20] [B26]",
                          CST_cmd_modem_label)
              PRINT_FORCE("                       [B28] [B39] [nchanche] [any]")
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
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config nb1band ...' command ----------------------------------------------------------------------*/
          if (argc >= 3U)
          {
            uint32_t nb1band_value_msb;
            uint32_t nb1band_value_lsb;

            /* get defined bands from the command line */
            ret = CST_CMD_get_band(CST_Nb1band, (const uint8_t **)argv_p, argc, &nb1band_value_msb, &nb1band_value_lsb);
            if (ret != true)
            {
              /* bad band argument: display help */
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage: modem config nb1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B18]")
              PRINT_FORCE("                            [B19] [B20] [B26] [B28] [nchanche] [any]")
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
                        crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config scanseq ...' command ----------------------------------------------------------------------*/
          if (argc == 2U)
          {
            /* no argument: displays current scanseq config */
            PRINT_FORCE("Scan Seq : (%06lx)", cst_cmd_scanseq_default)
            CST_CMD_display_seq_name(cst_cmd_scanseq_default, CST_Scanseq);
          }
          else if (argc == 3U)
          {
            uint32_t i;
            /* Parse all sequence mask to ... */
            for (i = 0U ; i < CST_CMD_SCANSEQ_NUMBER ; i++)
            {
              /* ... find matching scanseq mask */
              if (memcmp((const CRC_CHAR_t *)argv_p[2],
                         (const CRC_CHAR_t *)CST_ScanseqName_p[i],
                         crs_strlen(CST_ScanseqName_p[i])) == 0)
              {
                /* matching scanseq mask found: sets it in the current config */
                cst_cmd_scanseq_default = CST_Scanseq[i].value;
                break;
              }
            }

            if (i == CST_CMD_SCANSEQ_NUMBER)
            {
              /* no matching scanseq mask found */
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
            /* scanseq mask found */
            /* Bad command syntax: displays help  */
            PRINT_FORCE("Too many parameters command: %s %s \n\r", cmd_p, argv_p[1])
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config send' command: send condif to the modem ---------------------------------------------------*/
          /* send scanseg set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanmode\",%d,1", cst_cmd_nwscanmode_default);
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send iotopmode set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"iotopmode\",%ld,1", cst_cmd_iotopmode_default);
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;

          /* send nwscanseq set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanseq\",%06lx,1", cst_cmd_scanseq_default);
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;

          /* send band list set AT command  */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"band\",%lx%08lx,%lx%08lx,%lx%08lx,1",
                        cst_cmd_gsmband_MSB_default, cst_cmd_gsmband_LSB_default,
                        cst_cmd_m1band_MSB_default, cst_cmd_m1band_LSB_default,
                        cst_cmd_nb1band_MSB_default, cst_cmd_nb1band_LSB_default);
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;
          if (ret == false)
          {
            /* One of the AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen((uint8_t *)argv_p[1])) == 0)
        {
          /* 'modem config get' command: get condif from the modem ---------------------------------------------------*/
          /* send scanseg get AT command  */
          PRINT_FORCE("nwscanmode:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanmode\"");
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* send iotopmode get AT command  */
          PRINT_FORCE("iotopmode:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"iotopmode\"");
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;

          /* send gsmband get AT command  */
          PRINT_FORCE("GSM Bands:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"band\"");
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;

          /* send scanseg get AT command  */
          PRINT_FORCE("Scan Seq:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT+QCFG=\"nwscanseq\"");
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;
          if (ret == false)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }
        }
        else
        {
          /* Bad command argument: displays help  */
          PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
        }
      }
      else
      {
        /* Bad command argument: displays help  */
        PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
    else
    {
      /* Bad command argument: displays help  */
      PRINT_FORCE("bad command\n\r")
    }
  }

  PRINT_FORCE("")
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
  * @retval bool - result
  */
static bool CST_CMD_get_band_altair(const uint8_t *const *argv_p, uint32_t argc)
{
  /* local variables */
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint8_t  band_value;
  bool ret;
  bool leave;

  /* local variable init */
  ret = true;
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
    ret = false;
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
  /* Loop to parse the bands */
  for (i = 0U; i < CST_CMD_band_count  ; i++)
  {
    /* Display the band */
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
  /* Display help header */
  CMD_print_help(CST_cmd_modem_label);
  /* Display help lines */
  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("1 - Modem band configuration commands")
  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("Modem configuration commands are used to modify the modem band configuration.")
  PRINT_FORCE("Setting a new configuration is performed in two steps:")
  PRINT_FORCE("\n\r");
  /* 1st step commands */
  /* ----------------- */
  PRINT_FORCE("- 1st step: enter the configuration parameters using the following commands:");
  /* band definition command */
  PRINT_FORCE("%s config band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B14] [B17] [B18] ", CST_cmd_modem_label);
  PRINT_FORCE("               [B19] [B20] [B26] [B28] [B39] [B66]  (gets/sets the bands to use(12 bands max))");
  PRINT_FORCE("\n\r");
  /* 2nd step commands */
  /* ----------------- */
  PRINT_FORCE("- 2nd step: send the new configuration to the modem");
  /* Send config to modem */
  PRINT_FORCE("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  /* Other commands */
  /* -------------- */
  PRINT_FORCE("Other commands:");
  /* Command to display the actual config of the modem */
  PRINT_FORCE("%s config get (get current config from modem)", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("    (Note: the result of this command displays trace of modem response)")
  /* Command to display the recoorded config that will be send with 'config send' */
  PRINT_FORCE("%s config (display current config to be sent to modem)", CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  PRINT_FORCE("--------------------------------------")
  PRINT_FORCE("2 - Modem low power configuration     ")
  PRINT_FORCE("--------------------------------------")
  /* Command to configure modem lowpower */
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
  * @retval -
  */
static void CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];
  /* CST_CMD_Command_short is a temporary short copy of CST_CMD_Command to be able to add a string to CST_CMD_Command */
  /*   and avoid buffer overflow. CST_AT_BAND_SIZE_MAX is the max length of the string to be added to CST_CMD_Command */
  static uint8_t CST_CMD_Command_short[CST_ATCMD_SIZE_MAX - CST_AT_BAND_SIZE_MAX];

  /* Local variables */
  const uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  bool        ret;
  uint8_t     *cmd_p;
  uint8_t     i;

  PRINT_FORCE("\n\r")

  /* get the first "word" of the command line. here, it should be CST_cmd_modem_label = "modem" */
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");

  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CST_cmd_modem_label,
               crs_strlen(cmd_p)) == 0)
    {
      /* Cmd first word command is CST_cmd_modem_label (= "modem"). Now, parse parameters */
      for (argc = 0U ; argc < CST_CMS_PARAM_MAX ; argc++)
      {
        /* fill the array of parameters, and count the parameter number */
        argv_p[argc] = (uint8_t *)strtok(NULL, " \t");
        if (argv_p[argc] == NULL)
        {
          /* end of argument list reached */
          break;
        }
      }

      if (argc == 0U)
      {
        /* help command or no argument: displays help ----------------------------------------------------------------*/
        CST_ModemHelpCmd();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
      {
        /* help command or no argument: displays help */
        CST_ModemHelpCmd();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                      "config",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'modem config ...' command --------------------------------------------------------------------------------*/
        if (argc == 1U)
        {
          PRINT_FORCE("bands : ")
          CST_CMD_display_bitmap_name_altair();
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "bands",
                        crs_strlen(argv_p[1])) == 0)
        {
          /* 'modem config bands ...' command */
          if (argc >= 3U)
          {
            /* get defined bands from the command line */
            ret = CST_CMD_get_band_altair(argv_p,  argc);
            if (ret != true)
            {
              /*  parsing error: no band specified. Displays help  */
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage: config m1band [B1] [B2] [B3] [B4] [B5] [B8] [B12] [B13] [B14] [B17]");
              PRINT_FORCE("                     [B18] [B19] [B20] [B26] [B28] [B39] [B66]  (12 bands max)");
            }
          }
          /* display current band list */
          CST_CMD_display_bitmap_name_altair();
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
        {
          /* 'modem config send' command: send config to the modem ---------------------------------------------------*/
          /* 1st create the AT command */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETCFG=\"BAND\"");
          for (i = 0; i < CST_CMD_band_count; i++)
          {
            /* we will add to the CST_CMD_Command a string of at most CST_AT_BAND_SIZE_MAX characters. We want to */
            /*   avoid overlapping of this string */
            /* Copy actual string to a shorter one (CST_AT_BAND_SIZE_MAX characters less), to be able in a second */
            /*   time to add the needed CST_AT_BAND_SIZE_MAX characters without overlapping the CST_CMD_Command */
            /*   string */
            (void)memcpy(CST_CMD_Command_short, CST_CMD_Command, CST_ATCMD_SIZE_MAX - CST_AT_BAND_SIZE_MAX - 1U);
            /* be sure to ave a trailing \0 at the end of the string */
            CST_CMD_Command_short[CST_ATCMD_SIZE_MAX - CST_AT_BAND_SIZE_MAX - 1U] = (uint8_t)'\0';
            /* create the AT command to add the band by adding a band to the actual string */
            (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "%s,\"%hu\"", (CRC_CHAR_t *)CST_CMD_Command_short,
                          CST_CMD_band_tab[i]);
          }

          /* send the AT command to modem */
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);
          if (ret == false)
          {
            /* At least one AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "lowpower", crs_strlen(argv_p[1])) == 0)
        {
          /* set low power capabilities command ----------------------------------------------------------------------*/
          /* Configure HIFC mode to A. Specific command for ALTAIR modem */
          /* Can be send only once after modem firmware update. This configuration is stored at modem side */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETACFG=\"pm.hifc.mode,A\"");
          /* send AT command to modem */
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);

          /* Activate PSM mode */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETACFG=\"pm.conf.sleep_mode,enable\"");
          /* Send command to modem */
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;

          /* set max PSM level. DH0 is the optimum low power mode, the less consuming */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%SETACFG=\"pm.conf.max_allowed_pm_mode,dh0\"");
          /* Send command to modem */
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command) || ret;

          if (ret == false)
          {
            /* At least one AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }
          else
          {
            /* All AT command processing OK */
            PRINT_FORCE("\n\r")
            PRINT_FORCE("Low power capabilities enabled\n\r")
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
        {
          /* 'modem config get' command: get condif from the modem ---------------------------------------------------*/
          PRINT_FORCE("GSM Bands:")
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT%%GETCFG=\"BAND\"");
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);
          if (ret == false)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }
        }
        else
        {
          PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
        }
      }
      else
      {
        /* Bad command argument: displays help  */
        PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
  }
  PRINT_FORCE("")
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
  /* Display help header */
  CMD_print_help(CST_cmd_modem_label);
  /* Display help lines */
  PRINT_FORCE("Modem configuration commands are used to modify the modem band configuration.")
  PRINT_FORCE("Setting a new configuration is performed in two steps:")
  PRINT_FORCE("\n\r");
  /* 1st step commands */
  /* ----------------- */
  PRINT_FORCE("- 1st step: enter the configuration parameters using the following commands:");
  /* band definition command */
  PRINT_FORCE("%s config band  <1-256>...<1-256>   (gets/sets the bands to use)",
              CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  /* 2nd step commands */
  /* ----------------- */
  PRINT_FORCE("- 2nd step: send the new configuration to the modem");
  /* Send config to modem */
  PRINT_FORCE("%s config send", (CRC_CHAR_t *)CST_cmd_modem_label)
  PRINT_FORCE("\n\r");
  /* Other commands */
  /* -------------- */
  PRINT_FORCE("Other command:");
  /* Command to display the actual config of the modem */
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
  * @retval bool - result
  */
static bool CST_CMD_get_band_sequans(const uint8_t *const *argv_p, uint32_t argc)
{
  /* Local variables */
  uint32_t j;
  uint32_t nb_band;
  uint32_t current_arg;
  uint8_t  band_value;
  bool ret;

  /* local variable init */
  ret = true;
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
    ret = false;
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
  * @retval -
  */
static void CST_ModemCmd(uint8_t *cmd_line_p)
{
  static uint8_t CST_CMD_Command[CST_ATCMD_SIZE_MAX];

  const uint8_t    *argv_p[CST_CMS_PARAM_MAX];
  uint32_t    argc;
  bool        ret;
  uint8_t    *cmd_p;
  uint32_t    i;

  PRINT_FORCE("\n\r")

  /* get the first "word" of the command line. here, it should be CST_cmd_modem_label = "modem" */
  cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
  if (cmd_p != NULL)
  {
    if (memcmp((CRC_CHAR_t *)cmd_p,
               (CRC_CHAR_t *)CST_cmd_modem_label,
               crs_strlen(cmd_p)) == 0)
    {
      /* Cmd first word command is CST_cmd_modem_label (= "modem"). Now, parse parameters */
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
        /* help command: displays help -------------------------------------------------------------------------------*/
        CST_ModemHelpCmd();
      }
      else if (memcmp((const CRC_CHAR_t *)argv_p[0],
                      "config",
                      crs_strlen(argv_p[0])) == 0)
      {
        /* 'modem config ...' command --------------------------------------------------------------------------------*/
        if (argc == 1U)
        {
          /* no argument: only display default bands */
          PRINT_FORCE("bands : ")
          CST_CMD_display_bitmap_name_sequans();
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1],
                        "bands",
                        crs_strlen(argv_p[1])) == 0)
        {
          /* 'modem config band ...' command*/
          if (argc >= 3U)
          {
            /*  parse command to get specified bands  */
            ret = CST_CMD_get_band_sequans(argv_p,  argc);
            if (ret != true)
            {
              /*  parsing error: no band specified. Displays help  */
              PRINT_FORCE("%s Bad parameter", CST_cmd_modem_label)
              PRINT_FORCE("Usage:%s config m1band <1-256>...<1-256>   (12 bands max)",
                          CST_cmd_modem_label)
            }
          }
          /* display selected bands*/
          CST_CMD_display_bitmap_name_sequans();
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "send", crs_strlen(argv_p[1])) == 0)
        {
          /* send config to the modem --------------------------------------------------------------------------------*/
          /* Firstly: clear current modem configuration */
          ret = cst_at_command_handle((uint8_t *)"AT!=\"clearscanconfig\"");
          if (ret == false)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }

          /* secondary: send the list of the new bands */
          for (i = 0; i < CST_CMD_band_count; i++)
          {
            /* create the AT command to add the band */
            (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT!=\"addScanBand band=%d\"", CST_CMD_band_tab[i] + 1U);
            /* send the AT command to the modem */
            ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);
            if (ret == false)
            {
              /* AT command processing failed */
              PRINT_FORCE("command fail\n\r")
              break;
            }
          }
        }
        else if (memcmp((const CRC_CHAR_t *)argv_p[1], "get", crs_strlen(argv_p[1])) == 0)
        {
          /* get the current list of band from the modem -------------------------------------------------------------*/
          PRINT_FORCE("GSM Bands:")
          /* create the AT command to get list of bands */
          (void)sprintf((CRC_CHAR_t *)CST_CMD_Command, "AT!=addScanBand");
          /* send the AT command to the modem */
          ret = cst_at_command_handle((uint8_t *)CST_CMD_Command);
          if (ret == false)
          {
            /* AT command processing failed */
            PRINT_FORCE("command fail\n\r")
          }
        }
        else
        {
          /* Syntax error */
          PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
        }
      }
      else
      {
        /* Syntax error */
        PRINT_FORCE("bad command: %s %s\n\r", cmd_p, argv_p[0])
      }
    }
  }
  PRINT_FORCE("")
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
  * @retval bool - command result
  */
static bool cst_at_command_handle(uint8_t *cmd_line_p)
{
  uint32_t     size;
  bool         cmd_status;
  CS_Status_t  cs_status ;
  static CS_direct_cmd_tx_t CST_direct_cmd_tx;

  cmd_status = true;

  size =  crs_strlen(cmd_line_p) + 1U;
  if (size <= MAX_DIRECT_CMD_SIZE)
  {
    /* Command line length is OK */
    /* Initialize data structure to be send */
    /* AT command string */
    (void)memcpy(&CST_direct_cmd_tx.cmd_str[0],
                 (CRC_CHAR_t *)cmd_line_p,
                 size);
    /* AT command length */
    CST_direct_cmd_tx.cmd_size    = (uint16_t)crs_strlen(cmd_line_p);
    /* AT command timeout */
    CST_direct_cmd_tx.cmd_timeout = cst_at_timeout;

    /* send the AT command to the modem */
    cs_status = osCDS_direct_cmd(&CST_direct_cmd_tx, CST_cellular_direct_cmd_callback);
    if (cs_status != CS_OK)
    {
      /* AT command failed */
      PRINT_FORCE("\n\r%s command FAIL\n\r", cmd_line_p)
      cmd_status = false;
    }
  }
  else
  {
    /* AT command too long */
    PRINT_FORCE("\n\r%s command too long : FAIL\n\r", cmd_line_p)
    cmd_status = false;
  }
  return cmd_status;
}

/**
  * @brief  AT command line management
  * @param  cmd_line_p - command line
  * @retval -
  */
static void CST_AtCmd(uint8_t *cmd_line_p)
{
  uint8_t  *argv_p[CST_CMS_PARAM_MAX];
  uint32_t i;
  uint32_t cmd_len;
  const uint8_t *cmd_p;


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
      /* AT command to process ---------------------------------------------------------------------------------------*/
      (void)cst_at_command_handle(&cmd_line_p[i]);
    }
    else
    {
      /* Not an AT command */
      cmd_p = (uint8_t *)strtok((CRC_CHAR_t *)cmd_line_p, " \t");
      if (cmd_p != NULL)
      {
        if (memcmp((const CRC_CHAR_t *)cmd_p,
                   (const CRC_CHAR_t *)CST_cmd_at_label,
                   crs_strlen(cmd_p)) == 0)
        {
          /* parameters parsing                     */
          argv_p[0] = (uint8_t *)strtok(NULL, " \t");
          if (argv_p[0] != NULL)
          {
            if (memcmp((CRC_CHAR_t *)argv_p[0], "help", crs_strlen(argv_p[0])) == 0)
            {
              /* help command ----------------------------------------------------------------------------------------*/
              cst_at_cmd_help();
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[0],
                            "timeout",
                            crs_strlen(argv_p[0])) == 0)
            {
              /* timeout command -------------------------------------------------------------------------------------*/
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
                            crs_strlen(argv_p[0])) == 0)
            {
              /* suspenddata command: allows to switch modem to command mode -----------------------------------------*/
              CS_Status_t cs_status ;
              cs_status = osCDS_suspend_data();
              if (cs_status != CS_OK)
              {
                PRINT_FORCE("\n\rsuspend data FAIL")
              }
              else
              {
                PRINT_FORCE("\n\rsuspend data OK")
              }
            }
            else if (memcmp((CRC_CHAR_t *)argv_p[0],
                            "resumedata",
                            crs_strlen(argv_p[0])) == 0)
            {
              /* resumedata command: allows to switch modem data mode ------------------------------------------------*/
              CS_Status_t cs_status ;
              cs_status = osCDS_resume_data();
              if (cs_status != CS_OK)
              {
                PRINT_FORCE("\n\rresume data FAIL")
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
}


/**
  * @brief  starts cellar command management
  * @param  -
  * @retval CS_Status_t - function result
  */
CS_Status_t CST_cmd_cellular_service_start(void)
{
  /* Declare and register to cmd the "cst" commands */
  CMD_Declare(CST_cmd_label, CST_cmd, (uint8_t *)"cellular service task management");
  /* Declare and register to cmd the "at" commands */
  CMD_Declare(CST_cmd_at_label, CST_AtCmd, (uint8_t *)"send an at command");
#if (CST_CMD_USE_MODEM_CONFIG == 1)
  /* Declare and register to cmd the "modem" commands */
  CMD_Declare(CST_cmd_modem_label, CST_ModemCmd, (uint8_t *)"modem configuration management");
#endif  /* CST_CMD_USE_MODEM_CONFIG == 1 */

  return CS_OK;
}

#endif  /* (USE_CMD_CONSOLE == 1) */
