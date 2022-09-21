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
#ifndef CELLULAR_RUNTIME_STANDARD_H
#define CELLULAR_RUNTIME_STANDARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include <stdio.h>

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern uint8_t *crs_itoa(int32_t num, uint8_t *str, uint32_t base);
extern int32_t  crs_atoi(const uint8_t *string);
extern int32_t  crs_atoi_hex(const uint8_t *string);
extern uint32_t crs_strlen(const uint8_t *string);

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_RUNTIME_STANDARD_H_ */
