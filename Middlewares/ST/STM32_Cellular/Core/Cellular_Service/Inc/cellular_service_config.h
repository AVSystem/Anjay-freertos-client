/**
  ******************************************************************************
  * @file    cellular_service_config.h
  * @author  MCD Application Team
  * @brief   Header for cellular task configuration
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CELLULAR_SERVICE_CONFIG_H
#define CELLULAR_SERVICE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_modem_config.h"

/* Exported constants --------------------------------------------------------*/
#define CST_NFMC_TEMPO_NB          7U

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void CST_set_default_setup_config(dc_cellular_params_t *cellular_params);
CS_Status_t CST_config_init(void);
CS_PDN_conf_id_t  cst_get_cid_value(uint8_t cid_value);
#if (!USE_DEFAULT_SETUP == 1)
uint32_t CST_update_config_setup_handler(dc_apn_config_t *apn_config, dc_cs_sim_slot_type_t  active_slot);

#endif /* (!USE_DEFAULT_SETUP == 1) */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

