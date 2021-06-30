/**
  ******************************************************************************
  * @file    stm32l462e_cell1_conf.h
  * @author  MCD Application Team
  * @brief   STM32L462E_CELL1 configuration file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

