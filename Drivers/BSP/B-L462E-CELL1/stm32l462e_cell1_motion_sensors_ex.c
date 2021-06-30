/**
  ******************************************************************************
  * @file    stm32l462e_cell1_motion_sensors_ex.c
  * @author  MCD Application Team
  * @brief   source file for the BSP motion sensors extended driver
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

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_motion_sensors_ex.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_MOTION_SENSORS_EX STM32L462E_CELL1 MOTION_SENSORS_EX
  * @{
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_EX_Exported_Variables STM32L462E_CELL1 MOTION_SENSORS_EX Exported Vars
  * @{
  */

extern void *MotionCompObj[STM32L462E_CELL1_MOTION_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_EX_Exported_Functions STM32L462E_CELL1 MOTION_SENSORS_EX Exported Function
  * @{
  */

/**
  * @brief  Get the status of data ready bit (available only for LSM6DSL, LSM303AGR, ASM330LHH sensors)
  * @param  Instance the device instance
  * @param  Function Motion sensor function. Could be:
  *         - MOTION_ACCELERO or MOTION_GYRO for instance STM32L462E_CELL1_LSM6DSL_0
  *         - MOTION_ACCELERO for instance STM32L462E_CELL1_LSM303AGR_ACC_0
  *         - MOTION_MAGNETO for instance STM32L462E_CELL1_LSM303AGR_MAG_0
  * @param  Status the pointer to the status
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Get_DRDY_Status(uint32_t Instance, uint32_t Function, uint8_t *Status)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_MAG_0:
      if ((Function & MOTION_MAGNETO) == MOTION_MAGNETO)
      {
        if (LSM303AGR_MAG_Get_DRDY_Status(
              (LSM303AGR_MAG_Object_t *)(MotionCompObj[Instance]),
              Status) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else
        {
          ret = BSP_ERROR_NONE;
        }
      }
      else
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */
    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get the register value (available only for LSM6DSL, LSM303AGR, ASM330LHH sensors)
  * @param  Instance the device instance
  * @param  Reg address to be read
  * @param  Data pointer where the value is written to
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Read_Register(uint32_t Instance, uint8_t Reg, uint8_t *Data)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_ACC_0:
      if (LSM303AGR_ACC_Read_Reg((LSM303AGR_ACC_Object_t *)(MotionCompObj[Instance]), Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_MAG_0:
      if (LSM303AGR_MAG_Read_Reg((LSM303AGR_MAG_Object_t *)(MotionCompObj[Instance]), Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Set the register value (available only for LSM6DSL, LSM303AGR, ASM330LHH sensors)
  * @param  Instance the device instance
  * @param  Reg address to be read
  * @param  Data value to be written
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Write_Register(uint32_t Instance, uint8_t Reg, uint8_t Data)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_ACC_0:
      if (LSM303AGR_ACC_Write_Reg((LSM303AGR_ACC_Object_t *)(MotionCompObj[Instance]), Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_MAG_0:
      if (LSM303AGR_MAG_Write_Reg((LSM303AGR_MAG_Object_t *)(MotionCompObj[Instance]), Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
