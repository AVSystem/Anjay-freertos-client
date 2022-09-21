/**
  ******************************************************************************
  * @file    cellular_runtime_custom.h
  * @author  MCD Application Team
  * @brief   Header for cellular_runtime_custom.c module
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
#ifndef CELLULAR_RUNTIME_CUSTOM_H
#define CELLULAR_RUNTIME_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef char CRC_CHAR_t;

/* Exported functions ------------------------------------------------------- */
extern uint32_t crc_get_ip_addr(uint8_t *string, uint8_t *addr, uint16_t *port);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_RUNTIME_CUSTOM_H_ */
