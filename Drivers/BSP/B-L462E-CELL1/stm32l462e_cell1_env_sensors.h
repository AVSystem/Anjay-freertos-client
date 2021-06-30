/**
  ******************************************************************************
  * @file    stm32l462e_cell1_env_sensors.h
  * @author  MCD Application Team
  * @brief   header file for the BSP environmental sensors driver
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
#ifndef STM32L462E_CELL1_ENV_SENSORS_H
#define STM32L462E_CELL1_ENV_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_conf.h"
#include "env_sensor.h"
#include <math.h>

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_ENV_SENSORS STM32L462E_CELL1 ENV_SENSORS
  * @{
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Exported_Types STM32L462E_CELL1 ENV_SENSORS Exported Types
  * @{
  */

/* Environmental Sensor instance Info */
typedef struct
{
  uint8_t Temperature;
  uint8_t Pressure;
  uint8_t Humidity;
  uint8_t LowPower;
  float_t HumMaxOdr;
  float_t TempMaxOdr;
  float_t PressMaxOdr;
} ENV_SENSOR_Capabilities_t;

typedef struct
{
  uint32_t Functions;
} ENV_SENSOR_Ctx_t;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Exported_Constants STM32L462E_CELL1 ENV_SENSORS Exported Constants
  * @{
  */

#ifndef USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0
#define USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0          1
#endif /* USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 */

#ifndef USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0
#define USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0         1
#endif /* USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
#include "hts221.h"
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
#include "lps22hh.h"
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
#define STM32L462E_CELL1_HTS221_0  (0)
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
#define STM32L462E_CELL1_LPS22HH_0 (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0)
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

#ifndef ENV_TEMPERATURE
#define ENV_TEMPERATURE      1U
#endif /* ENV_TEMPERATURE */
#ifndef ENV_PRESSURE
#define ENV_PRESSURE         2U
#endif /* ENV_PRESSURE */
#ifndef ENV_HUMIDITY
#define ENV_HUMIDITY         4U
#endif /* ENV_HUMIDITY */

#define STM32L462E_CELL1_ENV_FUNCTIONS_NBR    3U
#define STM32L462E_CELL1_ENV_INSTANCES_NBR    (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 + \
                                               USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0)

#if (STM32L462E_CELL1_ENV_INSTANCES_NBR == 0)
#error "No environmental sensor instance has been selected"
#endif /* (STM32L462E_CELL1_ENV_INSTANCES_NBR == 0) */

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Exported_Functions STM32L462E_CELL1 ENV_SENSORS Exported Functions
  * @{
  */

/** High Level API : for basic sensors functionalities
  *  This API uses Low Level API and its goal is to ease use of sensors.
  */
int32_t BSP_ENV_SENSOR_Init_Temperature(void);
int32_t BSP_ENV_SENSOR_Init_Pressure(void);
int32_t BSP_ENV_SENSOR_Init_Humidity(void);
int32_t BSP_ENV_SENSOR_Read_Temperature(float_t *temperature);
int32_t BSP_ENV_SENSOR_Read_Pressure(float_t *pressure);
int32_t BSP_ENV_SENSOR_ReadT_Humidity(float_t *humidity);

/** Low Level API : for advanced sensors functionalities
 */
int32_t BSP_ENV_SENSOR_Init(uint32_t Instance, uint32_t Functions);
int32_t BSP_ENV_SENSOR_DeInit(uint32_t Instance);
int32_t BSP_ENV_SENSOR_GetCapabilities(uint32_t Instance, ENV_SENSOR_Capabilities_t *Capabilities);
int32_t BSP_ENV_SENSOR_ReadID(uint32_t Instance, uint8_t *Id);
int32_t BSP_ENV_SENSOR_Enable(uint32_t Instance, uint32_t Function);
int32_t BSP_ENV_SENSOR_Disable(uint32_t Instance, uint32_t Function);
int32_t BSP_ENV_SENSOR_GetOutputDataRate(uint32_t Instance, uint32_t Function, float_t *Odr);
int32_t BSP_ENV_SENSOR_SetOutputDataRate(uint32_t Instance, uint32_t Function, float_t Odr);
int32_t BSP_ENV_SENSOR_GetValue(uint32_t Instance, uint32_t Function, float_t *Value);

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

#endif /* STM32L462E_CELL1_ENV_SENSORS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
