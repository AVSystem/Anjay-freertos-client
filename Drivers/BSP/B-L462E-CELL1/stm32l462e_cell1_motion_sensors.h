/**
  ******************************************************************************
  * @file    stm32l462e_cell1_motion_sensors.h
  * @author  MCD Application Team
  * @brief   header file for the BSP motion sensors driver
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
#ifndef STM32L462E_CELL1_MOTION_SENSOR_H
#define STM32L462E_CELL1_MOTION_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_conf.h"
#include "motion_sensor.h"
#include <math.h>

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_MOTION_SENSORS STM32L462E_CELL1 MOTION_SENSORS
  * @{
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Exported_Types STM32L462E_CELL1 MOTION_SENSORS Exported Types
  * @{
  */

typedef struct
{
  int32_t x;
  int32_t y;
  int32_t z;
} MOTION_SENSOR_Axes_t;

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t z;
} MOTION_SENSOR_AxesRaw_t;

/* Motion Sensor instance Info */
typedef struct
{
  uint8_t  Acc;
  uint8_t  Gyro;
  uint8_t  Magneto;
  uint8_t  LowPower;
  uint32_t GyroMaxFS;
  uint32_t AccMaxFS;
  uint32_t MagMaxFS;
  float_t    GyroMaxOdr;
  float_t    AccMaxOdr;
  float_t    MagMaxOdr;
} MOTION_SENSOR_Capabilities_t;

typedef struct
{
  uint32_t Functions;
} MOTION_SENSOR_Ctx_t;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Exported_Constants STM32L462E_CELL1 MOTION_SENSORS Exported Constants
  * @{
  */

#ifndef USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0
#define USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0    1
#endif /* USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 */

#ifndef USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0
#define USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0    1
#endif /* USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 */

#if ((USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) || \
  (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1))
#include "lsm303agr.h"
#endif /* USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 or USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1)
#define STM32L462E_CELL1_LSM303AGR_ACC_0 (0)
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
#define STM32L462E_CELL1_LSM303AGR_MAG_0 (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0)
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */

#ifndef MOTION_GYRO
#define MOTION_GYRO             1U
#endif /* MOTION_GYRO */
#ifndef MOTION_ACCELERO
#define MOTION_ACCELERO         2U
#endif /* MOTION_ACCELERO */
#ifndef MOTION_MAGNETO
#define MOTION_MAGNETO          4U
#endif /* MOTION_MAGNETO */

#define STM32L462E_CELL1_MOTION_FUNCTIONS_NBR    3U
#define STM32L462E_CELL1_MOTION_INSTANCES_NBR    (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 + \
                                                  USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0)

#if (STM32L462E_CELL1_MOTION_INSTANCES_NBR == 0)
#error "No motion sensor instance has been selected"
#endif /* (STM32L462E_CELL1_MOTION_INSTANCES_NBR == 0) */

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Exported_Functions STM32L462E_CELL1 MOTION_SENSORS Exported Functions
  * @{
  */

/** High Level API : for basic sensors functionalities
  *  This API uses Low Level API and its goal is to ease use of sensors.
  */
int32_t BSP_MOTION_SENSOR_Init_Acc(void);
int32_t BSP_MOTION_SENSOR_Init_Mag(void);
int32_t BSP_MOTION_SENSOR_Read_Acc(MOTION_SENSOR_Axes_t *acceleration);
int32_t BSP_MOTION_SENSOR_Read_Mag(MOTION_SENSOR_Axes_t *magnetic_field);

/** Low Level API : for advanced sensors functionalities
 */
int32_t BSP_MOTION_SENSOR_Init(uint32_t Instance, uint32_t Functions);
int32_t BSP_MOTION_SENSOR_DeInit(uint32_t Instance);
int32_t BSP_MOTION_SENSOR_GetCapabilities(uint32_t Instance, MOTION_SENSOR_Capabilities_t *Capabilities);
int32_t BSP_MOTION_SENSOR_ReadID(uint32_t Instance, uint8_t *Id);
int32_t BSP_MOTION_SENSOR_Enable(uint32_t Instance, uint32_t Function);
int32_t BSP_MOTION_SENSOR_Disable(uint32_t Instance, uint32_t Function);
int32_t BSP_MOTION_SENSOR_GetAxes(uint32_t Instance, uint32_t Function, MOTION_SENSOR_Axes_t *Axes);
int32_t BSP_MOTION_SENSOR_GetAxesRaw(uint32_t Instance, uint32_t Function, MOTION_SENSOR_AxesRaw_t *Axes);
int32_t BSP_MOTION_SENSOR_GetSensitivity(uint32_t Instance, uint32_t Function, float_t *Sensitivity);
int32_t BSP_MOTION_SENSOR_GetOutputDataRate(uint32_t Instance, uint32_t Function, float_t *Odr);
int32_t BSP_MOTION_SENSOR_SetOutputDataRate(uint32_t Instance, uint32_t Function, float_t Odr);
int32_t BSP_MOTION_SENSOR_GetFullScale(uint32_t Instance, uint32_t Function, int32_t *Fullscale);
int32_t BSP_MOTION_SENSOR_SetFullScale(uint32_t Instance, uint32_t Function, int32_t Fullscale);

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

#endif /* STM32L462E_CELL1_MOTION_SENSOR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
