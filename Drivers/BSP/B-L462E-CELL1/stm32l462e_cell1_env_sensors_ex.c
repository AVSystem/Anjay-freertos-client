/**
  ******************************************************************************
  * @file    stm32l462e_cell1_env_sensors_ex.c
  * @author  MCD Application Team
  * @brief   source file for the BSP environmental sensors extended driver
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
#include "stm32l462e_cell1_env_sensors_ex.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_ENV_SENSORS_EX STM32L462E_CELL1 ENV_SENSORS_EX
  * @{
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_EX_Exported_Variables STM32L462E_CELL1 ENV_SENSORS_EX Exported Variables
  * @{
  */

extern void *EnvCompObj[STM32L462E_CELL1_ENV_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_EX_Exported_Functions STM32L462E_CELL1 ENV_SENSORS_EX Exported Functions
  * @{
  */

/**
  * @brief  Get the data stored in FIFO (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Press the pressure data
  * @param  Temp the temperature data
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Get_Data(uint32_t Instance, float_t *Press, float_t *Temp)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Get_Data((LPS22HH_Object_t *)EnvCompObj[Instance], Press, Temp) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get FIFO THR status (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Status the pointer to the status
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Get_Fth_Status(uint32_t Instance, uint8_t *Status)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Get_FTh_Status((LPS22HH_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get FIFO Full status (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Status the pointer to the status
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Get_Full_Status(uint32_t Instance, uint8_t *Status)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Get_Full_Status((LPS22HH_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get the number of unread samples in FIFO (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  NumSamples the number of unread FIFO samples
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Get_Num_Samples(uint32_t Instance, uint8_t *NumSamples)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Get_Level((LPS22HH_Object_t *)EnvCompObj[Instance], NumSamples) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get FIFO OVR status (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Status the pointer to the status
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Get_Ovr_Status(uint32_t Instance, uint8_t *Status)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Get_Ovr_Status((LPS22HH_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Reset FIFO Interrupt (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Interrupt FIFO interrupt. Could be: FTH, FULL or OVR
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Reset_Interrupt(uint32_t Instance, uint8_t Interrupt)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Reset_Interrupt((LPS22HH_Object_t *)EnvCompObj[Instance], Interrupt) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Set FIFO Interrupt (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Interrupt FIFO interrupt. Could be: FTH, FULL or OVR
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Set_Interrupt(uint32_t Instance, uint8_t Interrupt)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Set_Interrupt((LPS22HH_Object_t *)EnvCompObj[Instance], Interrupt) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Set FIFO mode  (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Mode FIFO mode
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Set_Mode(uint32_t Instance, uint8_t Mode)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Set_Mode((LPS22HH_Object_t *)EnvCompObj[Instance], Mode) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Set FIFO watermark  (available only for LPS22HB sensor)
  * @param  Instance the device instance
  * @param  Watermark FIFO data level threshold
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_FIFO_Set_Watermark_Level(uint32_t Instance, uint8_t Watermark)
{
  int32_t ret;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_FIFO_Set_Watermark_Level((LPS22HH_Object_t *)EnvCompObj[Instance], Watermark) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get the status of data ready bit (available only for HTS221, LPS22HB sensors)
  * @param  Instance the device instance
  * @param  Function Environmental sensor function. Could be:
  *         - ENV_HUMIDITY or ENV_TEMPERATURE for instance STM32L462E_CELL1_HTS221_0
  *         - ENV_PRESSURE or ENV_TEMPERATURE for instance STM32L462E_CELL1_LPS22HB_0
  * @param  Status the pointer to the status
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Get_DRDY_Status(uint32_t Instance, uint32_t Function, uint8_t *Status)
{
  int32_t ret;

  switch (Instance)
  {
#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
    case STM32L462E_CELL1_HTS221_0:
      if ((Function & ENV_HUMIDITY) == ENV_HUMIDITY)
      {
        if (HTS221_HUM_Get_DRDY_Status((HTS221_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else
        {
          ret = BSP_ERROR_NONE;
        }
      }
      else if ((Function & ENV_TEMPERATURE) == ENV_TEMPERATURE)
      {
        if (HTS221_TEMP_Get_DRDY_Status((HTS221_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
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
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if ((Function & ENV_PRESSURE) == ENV_PRESSURE)
      {
        if (LPS22HH_PRESS_Get_DRDY_Status((LPS22HH_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else
        {
          ret = BSP_ERROR_NONE;
        }
      }
      else if ((Function & ENV_TEMPERATURE) == ENV_TEMPERATURE)
      {
        if (LPS22HH_TEMP_Get_DRDY_Status((LPS22HH_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
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
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get the register value (available only for HTS221, LPS22HB sensors)
  * @param  Instance the device instance
  * @param  Reg address to be read
  * @param  Data pointer where the value is written to
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Read_Register(uint32_t Instance, uint8_t Reg, uint8_t *Data)
{
  int32_t ret;

  switch (Instance)
  {
#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
    case STM32L462E_CELL1_HTS221_0:
      if (HTS221_Read_Reg((HTS221_Object_t *)EnvCompObj[Instance], Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_Read_Reg((LPS22HH_Object_t *)EnvCompObj[Instance], Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Set the register value (available only for HTS221, LPS22HB sensors)
  * @param  Instance the device instance
  * @param  Reg address to be read
  * @param  Data value to be written
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Write_Register(uint32_t Instance, uint8_t Reg, uint8_t Data)
{
  int32_t ret;

  switch (Instance)
  {
#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
    case STM32L462E_CELL1_HTS221_0:
      if (HTS221_Write_Reg((HTS221_Object_t *)EnvCompObj[Instance], Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_Write_Reg((LPS22HH_Object_t *)EnvCompObj[Instance], Reg, Data) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Set environmental sensor one shot mode
  * @param  Instance environmental sensor instance to be used
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Set_One_Shot(uint32_t Instance)
{
  int32_t ret;

  switch (Instance)
  {
#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
    case STM32L462E_CELL1_HTS221_0:
      if (HTS221_Set_One_Shot((HTS221_Object_t *)EnvCompObj[Instance]) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_Set_One_Shot((LPS22HH_Object_t *)EnvCompObj[Instance]) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  return ret;
}

/**
  * @brief  Get environmental sensor one shot status
  * @param  Instance environmental sensor instance to be used
  * @param  Status pointer to the one shot status (1 = measurements available, 0 = measurements not available yet)
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Get_One_Shot_Status(uint32_t Instance, uint8_t *Status)
{
  int32_t ret;

  switch (Instance)
  {
#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
    case STM32L462E_CELL1_HTS221_0:
      if (HTS221_Get_One_Shot_Status((HTS221_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_Get_One_Shot_Status((LPS22HH_Object_t *)EnvCompObj[Instance], Status) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

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
