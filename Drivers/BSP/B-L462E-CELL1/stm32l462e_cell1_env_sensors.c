/**
  ******************************************************************************
  * @file    stm32l462e_cell1_env_sensors.c
  * @author  MCD Application Team
  * @brief   source file for the BSP environmental sensors driver
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
#include "stm32l462e_cell1_env_sensors.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_ENV_SENSORS STM32L462E_CELL1 ENV_SENSORS
  * @{
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Exported_Variables STM32L462E_CELL1 ENV_SENSORS Exported Variables
  * @{
  */

extern void
*EnvCompObj[STM32L462E_CELL1_ENV_INSTANCES_NBR]; /* This "redundant" line is here to fulfil MISRA C-2012 rule 8.4 */
void *EnvCompObj[STM32L462E_CELL1_ENV_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Private_Variables STM32L462E_CELL1 ENV_SENSORS Private Variables
  * @{
  */

/* We define a jump table in order to get the correct index from the desired function. */
/* This table should have a size equal to the maximum value of a function plus 1.      */
static uint32_t FunctionIndexEnv[5] = {0, 0, 1, 1, 2};
static ENV_SENSOR_FuncDrv_t *EnvFuncDrv[STM32L462E_CELL1_ENV_INSTANCES_NBR][STM32L462E_CELL1_ENV_FUNCTIONS_NBR];
static ENV_SENSOR_CommonDrv_t *EnvDrv[STM32L462E_CELL1_ENV_INSTANCES_NBR];
static ENV_SENSOR_Ctx_t EnvCtx[STM32L462E_CELL1_ENV_INSTANCES_NBR];

/**
  * @}
  */


/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Private_Functions_Prototypes STM32L462E_CELL1 ENV_SENSORS Private Funct Proto
  * @{
  */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
static int32_t HTS221_0_Probe(uint32_t Functions);
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
static int32_t LPS22HH_0_Probe(uint32_t Functions);
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Exported_Functions STM32L462E_CELL1 ENV_SENSORS Exported Functions
  * @{
  */

/**
  * @brief  Initializes the Temperature environmental sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Init_Temperature(void)
{
  return (BSP_ENV_SENSOR_Init(STM32L462E_CELL1_HTS221_0, ENV_TEMPERATURE));
}

/**
  * @brief  Initializes the Pressure environmental sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Init_Pressure(void)
{
  return (BSP_ENV_SENSOR_Init(STM32L462E_CELL1_LPS22HH_0, ENV_PRESSURE));
}

/**
  * @brief  Initializes the Humidity environmental sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Init_Humidity(void)
{
  return (BSP_ENV_SENSOR_Init(STM32L462E_CELL1_HTS221_0, ENV_HUMIDITY));
}

/**
  * @brief  Read the temperature environmental sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @param  temperature pointer to temperature value
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Read_Temperature(float_t *temperature)
{
  return (BSP_ENV_SENSOR_GetValue(STM32L462E_CELL1_HTS221_0, ENV_TEMPERATURE, temperature));
}

/**
  * @brief  Read the pressure environmental sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @param  pressure pointer to pressure value
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Read_Pressure(float_t *pressure)
{
  return (BSP_ENV_SENSOR_GetValue(STM32L462E_CELL1_LPS22HH_0, ENV_PRESSURE, pressure));
}

/**
  * @brief  Read the humidity environmental sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @param  humidity pointer to humidity value
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_ReadT_Humidity(float_t *humidity)
{
  return (BSP_ENV_SENSOR_GetValue(STM32L462E_CELL1_HTS221_0, ENV_HUMIDITY, humidity));
}

/**
  * @brief  Initializes the environmental sensor
  * @param  Instance environmental sensor instance to be used
  * @param  Functions Environmental sensor functions. Could be :
  *         - ENV_TEMPERATURE and/or ENV_HUMIDITY for instance 0
  *         - ENV_TEMPERATURE and/or ENV_PRESSURE for instance 1
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Init(uint32_t Instance, uint32_t Functions)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t function = ENV_TEMPERATURE;
  uint32_t i;
  uint32_t component_functions = 0;
  ENV_SENSOR_Capabilities_t cap;

  switch (Instance)
  {
#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
    case STM32L462E_CELL1_HTS221_0:
      if (HTS221_0_Probe(Functions) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_NO_INIT;
      }
      else
      {
        if (EnvDrv[Instance]->GetCapabilities(EnvCompObj[Instance], (void *)&cap) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_UNKNOWN_COMPONENT;
        }
        else
        {
          if (cap.Temperature == 1U)
          {
            component_functions |= ENV_TEMPERATURE;
          }
          if (cap.Humidity == 1U)
          {
            component_functions |= ENV_HUMIDITY;
          }
          if (cap.Pressure == 1U)
          {
            component_functions |= ENV_PRESSURE;
          }
        }
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
    case STM32L462E_CELL1_LPS22HH_0:
      if (LPS22HH_0_Probe(Functions) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_NO_INIT;
      }
      else
      {
        if (EnvDrv[Instance]->GetCapabilities(EnvCompObj[Instance], (void *)&cap) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_UNKNOWN_COMPONENT;
        }
        else
        {
          if (cap.Temperature == 1U)
          {
            component_functions |= ENV_TEMPERATURE;
          }
          if (cap.Humidity == 1U)
          {
            component_functions |= ENV_HUMIDITY;
          }
          if (cap.Pressure == 1U)
          {
            component_functions |= ENV_PRESSURE;
          }
        }
      }
      break;
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  if (ret == BSP_ERROR_NONE)
  {
    for (i = 0; i < STM32L462E_CELL1_ENV_FUNCTIONS_NBR; i++)
    {
      if (((Functions & function) == function) && ((component_functions & function) == function))
      {
        /* protection to not exceed FunctionIndexEnv array size */
        if (function <= 4U)
        {
          if (EnvFuncDrv[Instance][FunctionIndexEnv[function]]->Enable(EnvCompObj[Instance]) != BSP_ERROR_NONE)
          {
            ret = BSP_ERROR_COMPONENT_FAILURE;
            break;
          }
        }
      }
      function = function << 1;
    }
  }
  return ret;
}

/**
  * @brief  Deinitialize environmental sensor
  * @param  Instance environmental sensor instance to be used
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_DeInit(uint32_t Instance)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (EnvDrv[Instance]->DeInit(EnvCompObj[Instance]) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Get environmental sensor instance capabilities
  * @param  Instance Environmental sensor instance
  * @param  Capabilities pointer to Environmental sensor capabilities
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_GetCapabilities(uint32_t Instance, ENV_SENSOR_Capabilities_t *Capabilities)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (EnvDrv[Instance]->GetCapabilities(EnvCompObj[Instance], Capabilities) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Get WHOAMI value
  * @param  Instance environmental sensor instance to be used
  * @param  Id WHOAMI value
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_ReadID(uint32_t Instance, uint8_t *Id)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (EnvDrv[Instance]->ReadID(EnvCompObj[Instance], Id) != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Enable environmental sensor
  * @param  Instance environmental sensor instance to be used
  * @param  Function Environmental sensor function. Could be :
  *         - ENV_TEMPERATURE or ENV_HUMIDITY for instance 0
  *         - ENV_TEMPERATURE or ENV_PRESSURE for instance 1
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Enable(uint32_t Instance, uint32_t Function)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((EnvCtx[Instance].Functions & Function) == Function)
    {
      if (EnvFuncDrv[Instance][FunctionIndexEnv[Function]]->Enable(EnvCompObj[Instance]) != BSP_ERROR_NONE)
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
      ret = BSP_ERROR_WRONG_PARAM;
    }
  }

  return ret;
}

/**
  * @brief  Disable environmental sensor
  * @param  Instance environmental sensor instance to be used
  * @param  Function Environmental sensor function. Could be :
  *         - ENV_TEMPERATURE or ENV_HUMIDITY for instance 0
  *         - ENV_TEMPERATURE or ENV_PRESSURE for instance 1
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_Disable(uint32_t Instance, uint32_t Function)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((EnvCtx[Instance].Functions & Function) == Function)
    {
      if (EnvFuncDrv[Instance][FunctionIndexEnv[Function]]->Disable(EnvCompObj[Instance]) != BSP_ERROR_NONE)
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
      ret = BSP_ERROR_WRONG_PARAM;
    }
  }

  return ret;
}

/**
  * @brief  Get environmental sensor Output Data Rate
  * @param  Instance environmental sensor instance to be used
  * @param  Function Environmental sensor function. Could be :
  *         - ENV_TEMPERATURE or ENV_HUMIDITY for instance 0
  *         - ENV_TEMPERATURE or ENV_PRESSURE for instance 1
  * @param  Odr pointer to Output Data Rate read value
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_GetOutputDataRate(uint32_t Instance, uint32_t Function, float_t *Odr)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((EnvCtx[Instance].Functions & Function) == Function)
    {
      if (EnvFuncDrv[Instance][FunctionIndexEnv[Function]]->GetOutputDataRate(EnvCompObj[Instance], Odr) != BSP_ERROR_NONE)
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
      ret = BSP_ERROR_WRONG_PARAM;
    }
  }

  return ret;
}

/**
  * @brief  Set environmental sensor Output Data Rate
  * @param  Instance environmental sensor instance to be used
  * @param  Function Environmental sensor function. Could be :
  *         - ENV_TEMPERATURE or ENV_HUMIDITY for instance 0
  *         - ENV_TEMPERATURE or ENV_PRESSURE for instance 1
  * @param  Odr Output Data Rate value to be set
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_SetOutputDataRate(uint32_t Instance, uint32_t Function, float_t Odr)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((EnvCtx[Instance].Functions & Function) == Function)
    {
      if (EnvFuncDrv[Instance][FunctionIndexEnv[Function]]->SetOutputDataRate(EnvCompObj[Instance], Odr) != BSP_ERROR_NONE)
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
      ret = BSP_ERROR_WRONG_PARAM;
    }
  }

  return ret;
}

/**
  * @brief  Get environmental sensor value
  * @param  Instance environmental sensor instance to be used
  * @param  Function Environmental sensor function. Could be :
  *         - ENV_TEMPERATURE or ENV_HUMIDITY for instance 0
  *         - ENV_TEMPERATURE or ENV_PRESSURE for instance 1
  * @param  Value pointer to environmental sensor value
  * @retval BSP status
  */
int32_t BSP_ENV_SENSOR_GetValue(uint32_t Instance, uint32_t Function, float_t *Value)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_ENV_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((EnvCtx[Instance].Functions & Function) == Function)
    {
      if (EnvFuncDrv[Instance][FunctionIndexEnv[Function]]->GetValue(EnvCompObj[Instance], Value) != BSP_ERROR_NONE)
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
      ret = BSP_ERROR_WRONG_PARAM;
    }
  }

  return ret;
}

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_ENV_SENSORS_Private_Functions STM32L462E_CELL1 ENV_SENSORS Private Functions
  * @{
  */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1)
/**
  * @brief  Register Bus IOs for instance 0 if component ID is OK
  * @param  Functions Environmental sensor functions. Could be :
  *         - ENV_TEMPERATURE and/or ENV_HUMIDITY
  * @retval BSP status
  */
static int32_t HTS221_0_Probe(uint32_t Functions)
{
  HTS221_IO_t            io_ctx;
  uint8_t                id;
  int32_t                ret = BSP_ERROR_NONE;
  static HTS221_Object_t hts221_obj_0;
  HTS221_Capabilities_t  cap;

  /* Configure the environmental sensor driver */
  io_ctx.BusType     = HTS221_I2C_BUS; /* I2C */
  io_ctx.Address     = HTS221_I2C_ADDRESS;
  io_ctx.Init        = BSP_I2C1_Init;
  io_ctx.DeInit      = BSP_I2C1_DeInit;
  io_ctx.ReadReg     = BSP_I2C1_ReadReg_8b;
  io_ctx.WriteReg    = BSP_I2C1_WriteReg_8b;
  io_ctx.GetTick     = BSP_GetTick;

  if (HTS221_RegisterBusIO(&hts221_obj_0, &io_ctx) != HTS221_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (HTS221_ReadID(&hts221_obj_0, &id) != HTS221_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (id != HTS221_ID)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    (void)HTS221_GetCapabilities(&hts221_obj_0, &cap);
    EnvCtx[STM32L462E_CELL1_HTS221_0].Functions = ((uint32_t)cap.Temperature) | ((uint32_t)cap.Pressure << 1) | ((
                                                    uint32_t)cap.Humidity << 2);

    EnvCompObj[STM32L462E_CELL1_HTS221_0] = &hts221_obj_0;
    /* The second cast (void *) is added to bypass Misra R11.3 rule */
    EnvDrv[STM32L462E_CELL1_HTS221_0] = (ENV_SENSOR_CommonDrv_t *)(void *)&HTS221_COMMON_Driver;

    if ((ret == BSP_ERROR_NONE) && ((Functions & ENV_TEMPERATURE) == ENV_TEMPERATURE) && (cap.Temperature == 1U))
    {
      /* The second cast (void *) is added to bypass Misra R11.3 rule */
      EnvFuncDrv[STM32L462E_CELL1_HTS221_0][FunctionIndexEnv[ENV_TEMPERATURE]] =
        (ENV_SENSOR_FuncDrv_t *)(void *)&HTS221_TEMP_Driver;

      if (EnvDrv[STM32L462E_CELL1_HTS221_0]->Init(EnvCompObj[STM32L462E_CELL1_HTS221_0]) != HTS221_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & ENV_HUMIDITY) == ENV_HUMIDITY) && (cap.Humidity == 1U))
    {
      /* The second cast (void *) is added to bypass Misra R11.3 rule */
      EnvFuncDrv[STM32L462E_CELL1_HTS221_0][FunctionIndexEnv[ENV_HUMIDITY]] =
        (ENV_SENSOR_FuncDrv_t *)(void *)&HTS221_HUM_Driver;

      if (EnvDrv[STM32L462E_CELL1_HTS221_0]->Init(EnvCompObj[STM32L462E_CELL1_HTS221_0]) != HTS221_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & ENV_PRESSURE) == ENV_PRESSURE))
    {
      /* Return an error if the application try to initialize a function not supported by the component */
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_HTS221_0 == 1) */

#if (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1)
/**
  * @brief  Register Bus IOs for instance 1 if component ID is OK
  * @param  Functions Environmental sensor functions. Could be :
  *         - ENV_TEMPERATURE and/or ENV_PRESSURE
  * @retval BSP status
  */
static int32_t LPS22HH_0_Probe(uint32_t Functions)
{
  LPS22HH_IO_t            io_ctx;
  uint8_t                 id;
  int32_t                 ret = BSP_ERROR_NONE;
  static LPS22HH_Object_t lps22hh_obj_0;
  LPS22HH_Capabilities_t  cap;

  /* Configure the pressure driver */
  io_ctx.BusType     = LPS22HH_I2C_BUS; /* I2C */
  io_ctx.Address     = LPS22HH_I2C_ADD_H;
  io_ctx.Init        = BSP_I2C1_Init;
  io_ctx.DeInit      = BSP_I2C1_DeInit;
  io_ctx.ReadReg     = BSP_I2C1_ReadReg_8b;
  io_ctx.WriteReg    = BSP_I2C1_WriteReg_8b;
  io_ctx.GetTick     = BSP_GetTick;

  if (LPS22HH_RegisterBusIO(&lps22hh_obj_0, &io_ctx) != LPS22HH_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (LPS22HH_ReadID(&lps22hh_obj_0, &id) != LPS22HH_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (id != LPS22HH_ID)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    (void)LPS22HH_GetCapabilities(&lps22hh_obj_0, &cap);

    EnvCtx[STM32L462E_CELL1_LPS22HH_0].Functions = ((uint32_t)cap.Temperature) | ((uint32_t)cap.Pressure << 1) | ((
                                                     uint32_t)cap.Humidity << 2);

    EnvCompObj[STM32L462E_CELL1_LPS22HH_0] = &lps22hh_obj_0;
    /* The second cast (void *) is added to bypass Misra R11.3 rule */
    EnvDrv[STM32L462E_CELL1_LPS22HH_0] = (ENV_SENSOR_CommonDrv_t *)(void *)&LPS22HH_COMMON_Driver;

    if ((ret == BSP_ERROR_NONE) && ((Functions & ENV_TEMPERATURE) == ENV_TEMPERATURE) && (cap.Temperature == 1U))
    {
      /* The second cast (void *) is added to bypass Misra R11.3 rule */
      EnvFuncDrv[STM32L462E_CELL1_LPS22HH_0][FunctionIndexEnv[ENV_TEMPERATURE]] =
        (ENV_SENSOR_FuncDrv_t *)(void *)&LPS22HH_TEMP_Driver;

      if (EnvDrv[STM32L462E_CELL1_LPS22HH_0]->Init(EnvCompObj[STM32L462E_CELL1_LPS22HH_0]) != LPS22HH_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & ENV_PRESSURE) == ENV_PRESSURE) && (cap.Pressure == 1U))
    {
      /* The second cast (void *) is added to bypass Misra R11.3 rule */
      EnvFuncDrv[STM32L462E_CELL1_LPS22HH_0][FunctionIndexEnv[ENV_PRESSURE]] =
        (ENV_SENSOR_FuncDrv_t *)(void *)&LPS22HH_PRESS_Driver;

      if (EnvDrv[STM32L462E_CELL1_LPS22HH_0]->Init(EnvCompObj[STM32L462E_CELL1_LPS22HH_0]) != LPS22HH_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & ENV_HUMIDITY) == ENV_HUMIDITY))
    {
      /* Return an error if the application try to initialize a function not supported by the component */
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }
  return ret;
}
#endif /* (USE_STM32L462E_CELL1_ENV_SENSOR_LPS22HH_0 == 1) */

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
