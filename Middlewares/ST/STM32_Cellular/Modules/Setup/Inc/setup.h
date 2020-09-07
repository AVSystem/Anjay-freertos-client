/**
  ******************************************************************************
  * @file    setup.h
  * @author  MCD Application Team
  * @brief   Header for setup.c module
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
#ifndef SETUP_H
#define SETUP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "menu_utils.h"

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* List of application using setup configuration        */
/* Note: appli_code identifies applications in feeprom  */
/*       => these values must not be modified           */
typedef enum
{
  SETUP_APPLI_HTTP_CLIENT = 1,
  SETUP_APPLI_CST         = 4,
  SETUP_APPLI_PING_CLIENT = 5,
  SETUP_APPLI_ECHOCLIENT  = 6,
  SETUP_APPLI_MQTTCLIENT  = 7,
#if (USE_BOOT_BEHAVIOUR_CONFIG == 1)
  SETUP_BOOT_BEHAVIOUR    = 8,
#endif  /*  (USE_BOOT_BEHAVIOUR_CONFIG == 1) */
  /* Must be the last item */
  SETUP_APPLI_MAX
} setup_appli_code_t;

/* Exported constants --------------------------------------------------------*/
/* type of application version */
typedef uint16_t setup_appli_version_t;

/* Exported functions ------------------------------------------------------- */
void setup_set_config(setup_appli_code_t code_appli, setup_appli_version_t version_appli,
                      uint8_t **config, uint32_t config_size);

/* setup component init */
void setup_init(void);

/* apply all active setup configurations */
void setup_apply(void);

/* display help of version parameter */
void setup_version_help(void);

uint32_t setup_save_config_flash(setup_appli_code_t code_appli, setup_appli_version_t version_appli);

int32_t setup_record(setup_appli_code_t code_appli,
                     setup_appli_version_t version_appli,
                     uint8_t *label_appli,
                     uint32_t (*setup_fnct)(void),
                     void (*dump_fnct)(void),
                     void (*help_fnct)(void),
                     uint8_t **default_config,
                     uint32_t default_config_size);

const uint8_t *setup_get_label_appli(setup_appli_code_t code_appli);


#ifdef __cplusplus
}
#endif

#endif /* SETUP_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

