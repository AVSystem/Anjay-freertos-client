/**
  ******************************************************************************
  * @file    stm32l462e_cell1_conf.h
  * @author  MCD Application Team
  * @brief   STM32L462E_CELL1 configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L462E_CELL1_CONF_H__
#define __STM32L462E_CELL1_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stm32l4xx_hal.h"
#include "stm32l462e_cell1_bus.h"
#include "stm32l462e_cell1_errno.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_CONF STM32L462E_CELL1 CONF
  * @{
  */

/** @defgroup STM32L462E_CELL1_CONF_Exported_Constants STM32L462E_CELL1 CONF Exported Constants
  * @{
  */

/* USER CODE BEGIN 1 */
#define USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0                1U
#define USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0               1U

#define USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0      1U
#define USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0      1U
/* USER CODE END 1 */

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STM32L462E_CELL1_CONF_H__*/

