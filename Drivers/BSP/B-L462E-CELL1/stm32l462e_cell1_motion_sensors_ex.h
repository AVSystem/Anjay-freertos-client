/**
  ******************************************************************************
  * @file    stm32l462e_cell1_motion_sensors_ex.h
  * @author  MCD Application Team
  * @brief   header file for the BSP motion sensors extended driver
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
#ifndef STM32L462E_CELL1_MOTION_SENSOR_EX_H
#define STM32L462E_CELL1_MOTION_SENSOR_EX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_motion_sensors.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_MOTION_SENSORS_EX STM32L462E_CELL1 MOTION_SENSORS_EX
  * @{
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_EX_Exported_Types STM32L462E_CELL1 MOTION_SENSORS_EX Exported Types
  * @{
  */
typedef enum
{
  STM32L462E_CELL1_MOTION_SENSOR_INT1_PIN = 0,
  STM32L462E_CELL1_MOTION_SENSOR_INT2_PIN
} STM32L462E_CELL1_MOTION_SENSOR_IntPin_t;

/*
typedef struct
{
  unsigned int FreeFallStatus_t : 1;
  unsigned int TapStatus_t : 1;
  unsigned int DoubleTapStatus_t : 1;
  unsigned int WakeUpStatus_t : 1;
  unsigned int StepStatus_t : 1;
  unsigned int TiltStatus_t : 1;
  unsigned int D6DOrientationStatus_t : 1;
  unsigned int SleepStatus_t : 1;
} STM32L462E_CELL1_MOTION_SENSOR_Event_Status_t;
*/

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_EX_Exported_Functions STM32L462E_CELL1 MOTION_SENSORS_EX Exported Function
  * @{
  */

int32_t BSP_MOTION_SENSOR_Get_DRDY_Status(uint32_t Instance, uint32_t Function, uint8_t *Status);
int32_t BSP_MOTION_SENSOR_Read_Register(uint32_t Instance, uint8_t Reg, uint8_t *Data);
int32_t BSP_MOTION_SENSOR_Write_Register(uint32_t Instance, uint8_t Reg, uint8_t Data);

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

#endif /* STM32L462E_CELL1_MOTION_SENSOR_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
