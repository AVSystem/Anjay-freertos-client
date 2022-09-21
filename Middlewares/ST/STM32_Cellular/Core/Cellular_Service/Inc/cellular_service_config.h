/**
  ******************************************************************************
  * @file    cellular_service_config.h
  * @author  MCD Application Team
  * @brief   Header for cellular task configuration
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CELLULAR_SERVICE_CONFIG_H
#define CELLULAR_SERVICE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_modem_config.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void CST_set_default_setup_config(dc_cellular_params_t *cellular_params);
CS_Status_t CST_config_init(void);
CS_PDN_conf_id_t  cst_get_cid_value(uint8_t cid_value);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_SERVICE_CONFIG_H */

