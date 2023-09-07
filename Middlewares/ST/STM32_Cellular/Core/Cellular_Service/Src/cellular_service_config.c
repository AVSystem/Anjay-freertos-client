/**
  ******************************************************************************
  * @file    cellular_service_config.c
  * @author  MCD Application Team
  * @brief   This file defines functions for Cellular Service Config
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

#include "dc_common.h"
#include "at_util.h"
#include "cellular_service.h"
#include "cellular_service_datacache.h"
#include "cellular_service_os.h"
#include "cellular_service_config.h"
#include "cellular_service_task.h"
#include "cellular_service_int.h"
#include "cellular_runtime_standard.h"
#include "cellular_runtime_custom.h"
#include "error_handler.h"

/* Private defines -----------------------------------------------------------*/
/* Cellular config default parameters */
#define CST_DEFAULT_SIM_INDEX_0           PLF_CELLULAR_SIM_INDEX_0
#define CST_DEFAULT_SIM_INDEX_1           PLF_CELLULAR_SIM_INDEX_1
#define CST_DEFAULT_SIM_INDEX_2           PLF_CELLULAR_SIM_INDEX_2
#define CST_DEFAULT_SIM_INDEX_3           PLF_CELLULAR_SIM_INDEX_3
#define CST_DEFAULT_APN_PRESENT           PLF_CELLULAR_APN_USER_DEFINED      /*!< APN is defined by user */
#define CST_DEFAULT_APN                   PLF_CELLULAR_APN                   /*!< APN */
#define CST_DEFAULT_CID                   PLF_CELLULAR_CID                   /*!< CID ("1"-"9") */
#define CST_DEFAULT_USERNAME              PLF_CELLULAR_USERNAME              /*!< User name  */
#define CST_DEFAULT_PASSWORD              PLF_CELLULAR_PASSWORD              /*!< Password   */
#define CST_DEFAULT_TARGET_STATE          PLF_CELLULAR_TARGET_STATE          /*!< Modem target state */
#define CST_DEFAULT_ATTACHMENT_TIMEOUT    PLF_CELLULAR_ATTACHMENT_TIMEOUT    /*!< Attachment timeout in ms */
#define CST_DEFAULT_NFMC_ACTIVATION       PLF_CELLULAR_NFMC_ACTIVATION       /*!< NFMC activation */
#define CST_DEFAULT_NFMC_TEMPO1           PLF_CELLULAR_NFMC_TEMPO1           /*!< NFMC value 1 */
#define CST_DEFAULT_NFMC_TEMPO2           PLF_CELLULAR_NFMC_TEMPO2           /*!< NFMC value 2 */
#define CST_DEFAULT_NFMC_TEMPO3           PLF_CELLULAR_NFMC_TEMPO3           /*!< NFMC value 3 */
#define CST_DEFAULT_NFMC_TEMPO4           PLF_CELLULAR_NFMC_TEMPO4           /*!< NFMC value 4 */
#define CST_DEFAULT_NFMC_TEMPO5           PLF_CELLULAR_NFMC_TEMPO5           /*!< NFMC value 5 */
#define CST_DEFAULT_NFMC_TEMPO6           PLF_CELLULAR_NFMC_TEMPO6           /*!< NFMC value 6 */
#define CST_DEFAULT_NFMC_TEMPO7           PLF_CELLULAR_NFMC_TEMPO7           /*!< NFMC value 6 */
#define CST_DEFAULT_NETWORK_REG_MODE      PLF_CELLULAR_NETWORK_REG_MODE
#define CST_DEFAULT_OPERATOR_NAME_FORMAT  PLF_CELLULAR_OPERATOR_NAME_FORMAT
#define CST_DEFAULT_OPERATOR_NAME         PLF_CELLULAR_OPERATOR_NAME
#define CST_DEFAULT_ACT_PRESENT           PLF_CELLULAR_ACT_PRESENT
#define CST_DEFAULT_ACCESS_TECHNO         PLF_CELLULAR_ACCESS_TECHNO
#define CST_DEFAULT_LP_INACTIVITY_TIMEOUT PLF_CELLULAR_LP_INACTIVITY_TIMEOUT /* Timeout before to enter in
                                                                                low power mode */

#define CST_DEFAULT_PARAMA_NB     21U                  /* Cellular parameter nb */

#define CST_SETUP_NFMC       (1)          /* NMFC code included */

#define CST_LABEL "Cellular Service"      /* Setup menu label */
#define CST_VERSION_APPLI     (uint16_t)7 /* V7: Adding low power time out timer duration*/

#define CST_TEMP_STRING_SIZE    64

/* Private macros ------------------------------------------------------------*/
#if (USE_PRINTF == 0U)
/* Trace macro definition */
#include "trace_interface.h"
#define PRINT_FORCE(format, args...) \
  TRACE_PRINT_FORCE(DBG_CHAN_CELLULAR_SERVICE, DBL_LVL_P0, "" format "", ## args)
#else
#include <stdio.h>
#define PRINT_FORCE(format, args...)                (void)printf(format , ## args);
#endif   /* (USE_PRINTF == 0U)*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void CST_local_setup_handler(void);

/* Global variables ----------------------------------------------------------*/

/* Private function Definition -----------------------------------------------*/

/**
  * @brief  convert cid integer value to enum value
  * @param  target_state_value    - cid integer value
  * @retval CS_PDN_conf_id_t  - cid enum value
  */
CS_PDN_conf_id_t  cst_get_cid_value(uint8_t cid_value)
{
  CS_PDN_conf_id_t enum_value;
  switch (cid_value)
  {
    case CS_PDN_PREDEF_CONFIG:
    {
      enum_value = CS_PDN_PREDEF_CONFIG;
      break;
    }
    case CS_PDN_USER_CONFIG_1:
    {
      enum_value = CS_PDN_USER_CONFIG_1;
      break;
    }
    case CS_PDN_USER_CONFIG_2:
    {
      enum_value = CS_PDN_USER_CONFIG_2;
      break;
    }
    case CS_PDN_USER_CONFIG_3:
    {
      enum_value = CS_PDN_USER_CONFIG_3;
      break;
    }
    case CS_PDN_USER_CONFIG_4:
    {
      enum_value = CS_PDN_USER_CONFIG_4;
      break;
    }
    case CS_PDN_USER_CONFIG_5:
    {
      enum_value = CS_PDN_USER_CONFIG_5;
      break;
    }
    case CS_PDN_CONFIG_DEFAULT:
    {
      enum_value = CS_PDN_CONFIG_DEFAULT;
      break;
    }
    case CS_PDN_NOT_DEFINED:
    {
      enum_value = CS_PDN_NOT_DEFINED;
      break;
    }
    case CS_PDN_ALL:
    {
      enum_value = CS_PDN_ALL;
      break;
    }
    default:
    {
      enum_value = CS_PDN_PREDEF_CONFIG;
      break;
    }

  }
  return enum_value;
}

/**
  * @brief  default setup config menu handler
  * @note
  * @param  none
  * @retval none
  */
static void CST_local_setup_handler(void)
{
  dc_cellular_params_t cellular_params;
  uint32_t size_apn;
  uint32_t size_user;
  uint32_t size_password;
  uint32_t size_operator;

  /* initialize the SIM slots */
  cellular_params.sim_slot_nb = 99U;


  cellular_params.sim_slot[0].sim_slot_type = (ca_sim_slot_type_t)CST_DEFAULT_SIM_INDEX_0;
  cellular_params.sim_slot[1].sim_slot_type = (ca_sim_slot_type_t)CST_DEFAULT_SIM_INDEX_1;
  cellular_params.sim_slot[2].sim_slot_type = (ca_sim_slot_type_t)CST_DEFAULT_SIM_INDEX_2;

  for (uint8_t idx = 0U; idx < DC_SIM_SLOT_NB; idx++)
  {
    /* If sim slot is applicable */
    if (cellular_params.sim_slot[idx].sim_slot_type != CA_SIM_NON_EXISTING_SLOT)
    {
      /* SIM slot parameters BEGIN*/
      /* APN present parameter */
      cellular_params.sim_slot[idx].apnPresent = CST_DEFAULT_APN_PRESENT;
      /* APN parameter */
      size_apn = crs_strlen(CST_DEFAULT_APN) + 1U;
      /* to avoid string overflow */
      if (size_apn <= CA_APN_SIZE_MAX)
      {
        (void)memcpy((CRC_CHAR_t *)cellular_params.sim_slot[idx].apn,
                     (CRC_CHAR_t *)CST_DEFAULT_APN,
                     size_apn);
      }

      /* CID parameter */
      cellular_params.sim_slot[idx].cid = CST_DEFAULT_CID;

      /* username parameter */
      size_user = crs_strlen(CST_DEFAULT_USERNAME) + 1U;
      /* to avoid string overflow */
      if (size_user <= CA_USERNAME_SIZE_MAX)
      {
        (void)memcpy((CRC_CHAR_t *)cellular_params.sim_slot[idx].username,
                     (CRC_CHAR_t *)CST_DEFAULT_USERNAME,
                     size_user);
      }

      /* password parameter */
      size_password = crs_strlen(CST_DEFAULT_PASSWORD) + 1U;
      /* to avoid string overflow */
      if (size_password <= CA_PASSWORD_SIZE_MAX)
      {
        (void)memcpy((CRC_CHAR_t *)cellular_params.sim_slot[idx].password,
                     (CRC_CHAR_t *)CST_DEFAULT_PASSWORD,
                     size_password);
      }
      /* SIM slot parameters END*/
    }
    else /* Sim Slot not applicable */
    {
      /* if sim_slot_nb not defined */
      if (cellular_params.sim_slot_nb == 99U)
      {
        cellular_params.sim_slot_nb = idx;
      }
    }
  }
  /* if no simslot on board, update corresponding values */
#if (DC_SIM_SLOT_NB == 0)
  /* if sim_slot_nb still not defined, that is, 4 applicable sim slots */
  if (cellular_params.sim_slot_nb == 99U)
  {
    cellular_params.sim_slot_nb = DC_SIM_SLOT_NB;
  }
#endif /* (DC_SIM_SLOT_NB == 0) */

  /* modem target state parameter */
  cellular_params.target_state = (dc_cs_target_state_t)CST_DEFAULT_TARGET_STATE;

  /* attachment timeout parameter */
  cellular_params.attachment_timeout = (uint32_t)CST_DEFAULT_ATTACHMENT_TIMEOUT;

  cellular_params.operator_selector.network_reg_mode      = CST_DEFAULT_NETWORK_REG_MODE;

  cellular_params.operator_selector.operator_name_format  = CST_DEFAULT_OPERATOR_NAME_FORMAT;

  size_operator = crs_strlen(CST_DEFAULT_OPERATOR_NAME) + 1U;
  (void)memcpy(cellular_params.operator_selector.operator_name, CST_DEFAULT_OPERATOR_NAME, size_operator);

  cellular_params.operator_selector.access_techno_present = CST_DEFAULT_ACT_PRESENT;

  cellular_params.operator_selector.access_techno         = CST_DEFAULT_ACCESS_TECHNO;

  /* low power inactivity timeout parameter */
  cellular_params.lp_inactivity_timeout = (uint32_t)CST_DEFAULT_LP_INACTIVITY_TIMEOUT;

#if (CST_SETUP_NFMC == 1)
  /* NMFC parameters */
  cellular_params.nfmc_active = CST_DEFAULT_NFMC_ACTIVATION;

  /* Set all NFMC tempo values */
  for (uint32_t i = 0U; i < CA_NFMC_VALUES_MAX_NB ; i++)
  {
    cellular_params.nfmc_value[i] = (uint32_t)CST_DEFAULT_NFMC_TEMPO1;
  }

#else /* CST_SETUP_NFMC == 1 */
  cellular_params.nfmc_active = 1;
  /* NMFC tempos */
  cellular_params.nfmc_value[0] = (uint32_t)CST_DEFAULT_NFMC_TEMPO1;
  cellular_params.nfmc_value[1] = (uint32_t)CST_DEFAULT_NFMC_TEMPO2;
  cellular_params.nfmc_value[2] = (uint32_t)CST_DEFAULT_NFMC_TEMPO3;
  cellular_params.nfmc_value[3] = (uint32_t)CST_DEFAULT_NFMC_TEMPO4;
  cellular_params.nfmc_value[4] = (uint32_t)CST_DEFAULT_NFMC_TEMPO5;
  cellular_params.nfmc_value[5] = (uint32_t)CST_DEFAULT_NFMC_TEMPO6;
  cellular_params.nfmc_value[6] = (uint32_t)CST_DEFAULT_NFMC_TEMPO7;
#endif /* (CST_SETUP_NFMC == 1) */

  /* set Data Cache entry valid */
  cellular_params.rt_state = DC_SERVICE_ON;

  /* write Entry to Data Cache */
  (void)dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, (void *)&cellular_params, sizeof(cellular_params));
}

/* Public function Definition -----------------------------------------------*/

/**
  * @brief  initialize the cellular configuration service
  * @note  this function is called by cellular service task init
  * @param  cellular_params   - cellular configuration
  * @retval error code (O:OK)
  */

CS_Status_t CST_config_init(void)
{
  /* In case on default setup (without menu) calls default configuration setting */
  CST_local_setup_handler();

  return CS_OK;
}
