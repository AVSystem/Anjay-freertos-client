/**
  ******************************************************************************
  * @file    feeprom_utils.h
  * @author  MCD Application Team
  * @brief   Header for feeprom_utils.c module
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
#ifndef FEEPROM_UTILS_H
#define FEEPROM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "setup.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#define FEEPROM_UTILS_MATCHING_BANK_FOUND         0U
#define FEEPROM_UTILS_INVALID_VERSION_BANK_FOUND  1U
#define FEEPROM_UTILS_NO_BANK_FOUND               2U
#define FEEPROM_UTILS_FREE_BANK_FOUND             3U

/* Exported functions ------------------------------------------------------- */
/*  erase a configuration in flash */
uint32_t feeprom_utils_setup_erase(setup_appli_code_t appli_code, setup_appli_version_t version_appli);

/*  erase all flash pages of the configuration flash bank */
void    feeprom_utils_flash_erase_all(void);

/*  save the configuration of an application in flash */
uint32_t feeprom_utils_save_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version,
                                         uint8_t *config_addr, uint32_t config_size);

/*  get the configuration of an application (appli_code) in flash */
uint32_t feeprom_utils_read_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version,
                                         uint8_t **config_addr, uint32_t *config_size);


#ifdef __cplusplus
}
#endif

#endif /* FEEPROM_UTILS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


