/**
  ******************************************************************************
  * @file    stm32l462e_cell1_motion_sensors.c
  * @author  MCD Application Team
  * @brief   source file for the BSP motion sensors driver
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
#include "stm32l462e_cell1_motion_sensors.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_MOTION_SENSORS STM32L462E_CELL1 MOTION_SENSORS
  * @{
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Exported_Variables STM32L462E_CELL1 MOTION_SENSORS Exported Variables
  * @{
  */

extern void
*MotionCompObj[STM32L462E_CELL1_MOTION_INSTANCES_NBR]; /* This "redundant" line is here for MISRA C-2012 rule 8.4 */
void *MotionCompObj[STM32L462E_CELL1_MOTION_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Private_Variables STM32L462E_CELL1 MOTION_SENSORS Private Variables
  * @{
  */

/* We define a jump table in order to get the correct index from the desired function. */
/* This table should have a size equal to the maximum value of a function plus 1.      */
static uint32_t FunctionIndexMotion[5] = {0, 0, 1, 1, 2};
static MOTION_SENSOR_FuncDrv_t
*MotionFuncDrv[STM32L462E_CELL1_MOTION_INSTANCES_NBR][STM32L462E_CELL1_MOTION_FUNCTIONS_NBR];
static MOTION_SENSOR_CommonDrv_t *MotionDrv[STM32L462E_CELL1_MOTION_INSTANCES_NBR];
static MOTION_SENSOR_Ctx_t MotionCtx[STM32L462E_CELL1_MOTION_INSTANCES_NBR];

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Private_Functions_Prototypes  MOTION_SENSORS Private Functions Protos
  * @{
  */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1)
static int32_t LSM303AGR_ACC_0_Probe(uint32_t Functions);
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
static int32_t LSM303AGR_MAG_0_Probe(uint32_t Functions);
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Exported_Functions STM32L462E_CELL1 MOTION_SENSORS Exported Functions
  * @{
  */


/**
  * @brief  Initializes the Accelerator motion sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Init_Acc(void)
{
  return (BSP_MOTION_SENSOR_Init(STM32L462E_CELL1_LSM303AGR_ACC_0, MOTION_ACCELERO));
}

/**
  * @brief  Initializes the Magnetometer motion sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Init_Mag(void)
{
  return (BSP_MOTION_SENSOR_Init(STM32L462E_CELL1_LSM303AGR_MAG_0, MOTION_MAGNETO));
}

/**
  * @brief  Read the acceleration motion sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @param  acceleration pointer to acceleration value
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Read_Acc(MOTION_SENSOR_Axes_t *acceleration)
{
  return (BSP_MOTION_SENSOR_GetAxes(STM32L462E_CELL1_LSM303AGR_ACC_0, MOTION_ACCELERO, acceleration));
}

/**
  * @brief  Read the magnetometer motion sensor
  * @note   This is a  part of High-Level API to ease use of sensors.
  * @param  magnetic_field pointer to magnetic_field value
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Read_Mag(MOTION_SENSOR_Axes_t *magnetic_field)
{
  return (BSP_MOTION_SENSOR_GetAxes(STM32L462E_CELL1_LSM303AGR_MAG_0, MOTION_MAGNETO, magnetic_field));
}

/**
  * @brief  Initializes the motion sensors
  * @param  Instance Motion sensor instance
  * @param  Functions Motion sensor functions. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Init(uint32_t Instance, uint32_t Functions)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t function = MOTION_GYRO;
  uint32_t i;
  uint32_t component_functions = 0;
  MOTION_SENSOR_Capabilities_t cap;

  switch (Instance)
  {

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_ACC_0:
      if (LSM303AGR_ACC_0_Probe(Functions) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_NO_INIT;
      }
      else
      {
        if (MotionDrv[Instance]->GetCapabilities(MotionCompObj[Instance], (void *)&cap) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_UNKNOWN_COMPONENT;
        }
        else
        {
          if (cap.Acc == 1U)
          {
            component_functions |= MOTION_ACCELERO;
          }
          if (cap.Gyro == 1U)
          {
            component_functions |= MOTION_GYRO;
          }
          if (cap.Magneto == 1U)
          {
            component_functions |= MOTION_MAGNETO;
          }
        }
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
    case STM32L462E_CELL1_LSM303AGR_MAG_0:
      if (LSM303AGR_MAG_0_Probe(Functions) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_NO_INIT;
      }
      else
      {
        if (MotionDrv[Instance]->GetCapabilities(MotionCompObj[Instance], (void *)&cap) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_UNKNOWN_COMPONENT;
        }
        else
        {
          if (cap.Acc == 1U)
          {
            component_functions |= MOTION_ACCELERO;
          }
          if (cap.Gyro == 1U)
          {
            component_functions |= MOTION_GYRO;
          }
          if (cap.Magneto == 1U)
          {
            component_functions |= MOTION_MAGNETO;
          }
        }
      }
      break;
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */

    default:
      ret = BSP_ERROR_WRONG_PARAM;
      break;
  }

  if (ret == BSP_ERROR_NONE)
  {
    for (i = 0; i < STM32L462E_CELL1_MOTION_FUNCTIONS_NBR; i++)
    {
      if (((Functions & function) == function) && ((component_functions & function) == function))
      {
        /* protection to not exceed FunctionIndexMotion array size */
        if (function <= 4U)
        {
          if (MotionFuncDrv[Instance][FunctionIndexMotion[function]]->Enable(MotionCompObj[Instance]) != BSP_ERROR_NONE)
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
  * @brief  Deinitialize Motion sensor
  * @param  Instance Motion sensor instance
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_DeInit(uint32_t Instance)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (MotionDrv[Instance]->DeInit(MotionCompObj[Instance]) != BSP_ERROR_NONE)
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
  * @brief  Get motion sensor instance capabilities
  * @param  Instance Motion sensor instance
  * @param  Capabilities pointer to motion sensor capabilities
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_GetCapabilities(uint32_t Instance, MOTION_SENSOR_Capabilities_t *Capabilities)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (MotionDrv[Instance]->GetCapabilities(MotionCompObj[Instance], Capabilities) != BSP_ERROR_NONE)
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
  * @param  Instance Motion sensor instance
  * @param  Id WHOAMI value
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_ReadID(uint32_t Instance, uint8_t *Id)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (MotionDrv[Instance]->ReadID(MotionCompObj[Instance], Id) != BSP_ERROR_NONE)
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
  * @brief  Enable Motion sensor
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Enable(uint32_t Instance, uint32_t Function)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->Enable(MotionCompObj[Instance]) != BSP_ERROR_NONE)
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
  * @brief  Disable Motion sensor
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_Disable(uint32_t Instance, uint32_t Function)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->Disable(MotionCompObj[Instance]) != BSP_ERROR_NONE)
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
  * @brief  Get motion sensor axes data
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Axes pointer to axes data structure
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_GetAxes(uint32_t Instance, uint32_t Function, MOTION_SENSOR_Axes_t *Axes)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->GetAxes(MotionCompObj[Instance], Axes) != BSP_ERROR_NONE)
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
  * @brief  Get motion sensor axes raw data
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Axes pointer to axes raw data structure
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_GetAxesRaw(uint32_t Instance, uint32_t Function, MOTION_SENSOR_AxesRaw_t *Axes)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->GetAxesRaw(MotionCompObj[Instance], Axes) != BSP_ERROR_NONE)
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
  * @brief  Get motion sensor sensitivity
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Sensitivity pointer to sensitivity read value
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_GetSensitivity(uint32_t Instance, uint32_t Function, float_t *Sensitivity)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->GetSensitivity(MotionCompObj[Instance],
                                                                                 Sensitivity) != BSP_ERROR_NONE)
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
  * @brief  Get motion sensor Output Data Rate
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Odr pointer to Output Data Rate read value
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_GetOutputDataRate(uint32_t Instance, uint32_t Function, float_t *Odr)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->GetOutputDataRate(MotionCompObj[Instance], Odr)
          != BSP_ERROR_NONE)
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
  * @brief  Get motion sensor Full Scale
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Fullscale pointer to Fullscale read value
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_GetFullScale(uint32_t Instance, uint32_t Function, int32_t *Fullscale)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->GetFullScale(MotionCompObj[Instance],
                                                                               Fullscale) != BSP_ERROR_NONE)
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
  * @brief  Set motion sensor Output Data Rate
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Odr Output Data Rate value to be set
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_SetOutputDataRate(uint32_t Instance, uint32_t Function, float_t Odr)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->SetOutputDataRate(MotionCompObj[Instance], Odr)
          != BSP_ERROR_NONE)
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
  * @brief  Set motion sensor Full Scale
  * @param  Instance Motion sensor instance
  * @param  Function Motion sensor function. Could be :
  *         - MOTION_GYRO and/or MOTION_ACCELERO for instance 0
  *         - MOTION_ACCELERO for instance 1
  *         - MOTION_MAGNETO for instance 2
  * @param  Fullscale Fullscale value to be set
  * @retval BSP status
  */
int32_t BSP_MOTION_SENSOR_SetFullScale(uint32_t Instance, uint32_t Function, int32_t Fullscale)
{
  int32_t ret;

  if (Instance >= STM32L462E_CELL1_MOTION_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((MotionCtx[Instance].Functions & Function) == Function)
    {
      if (MotionFuncDrv[Instance][FunctionIndexMotion[Function]]->SetFullScale(MotionCompObj[Instance],
                                                                               Fullscale) != BSP_ERROR_NONE)
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

/** @defgroup STM32L462E_CELL1_MOTION_SENSORS_Private_Functions STM32L462E_CELL1 MOTION_SENSORS Private Functions
  * @{
  */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1)
/**
  * @brief  Register Bus IOs for instance 1 if component ID is OK
  * @retval error status
  */
static int32_t LSM303AGR_ACC_0_Probe(uint32_t Functions)
{
  LSM303AGR_IO_t                io_ctx;
  uint8_t                       id;
  static LSM303AGR_ACC_Object_t lsm303agr_acc_obj_0;
  LSM303AGR_Capabilities_t      cap;
  int32_t ret = BSP_ERROR_NONE;

  /* Configure the accelero driver */
  io_ctx.BusType     = LSM303AGR_I2C_BUS; /* I2C */
  io_ctx.Address     = LSM303AGR_I2C_ADD_XL;
  io_ctx.Init        = BSP_I2C1_Init;
  io_ctx.DeInit      = BSP_I2C1_DeInit;
  io_ctx.ReadReg     = BSP_I2C1_ReadReg_8b;
  io_ctx.WriteReg    = BSP_I2C1_WriteReg_8b;
  io_ctx.GetTick     = BSP_GetTick;

  if (LSM303AGR_ACC_RegisterBusIO(&lsm303agr_acc_obj_0, &io_ctx) != LSM303AGR_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (LSM303AGR_ACC_ReadID(&lsm303agr_acc_obj_0, &id) != LSM303AGR_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (id != (uint8_t)LSM303AGR_ID_XL)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    (void)LSM303AGR_ACC_GetCapabilities(&lsm303agr_acc_obj_0, &cap);
    MotionCtx[STM32L462E_CELL1_LSM303AGR_ACC_0].Functions = ((uint32_t)cap.Gyro) | ((uint32_t)cap.Acc << 1) | ((
                                                              uint32_t)cap.Magneto << 2);

    MotionCompObj[STM32L462E_CELL1_LSM303AGR_ACC_0] = &lsm303agr_acc_obj_0;
    /* The second cast (void *) is added to bypass Misra R11.3 rule */
    MotionDrv[STM32L462E_CELL1_LSM303AGR_ACC_0] = (MOTION_SENSOR_CommonDrv_t *)(void *)&LSM303AGR_ACC_COMMON_Driver;

    if ((ret == BSP_ERROR_NONE) && ((Functions & MOTION_ACCELERO) == MOTION_ACCELERO) && (cap.Acc == 1U))
    {
      /* The second cast (void *) is added to bypass Misra R11.3 rule */
      MotionFuncDrv[STM32L462E_CELL1_LSM303AGR_ACC_0][FunctionIndexMotion[MOTION_ACCELERO]] = (MOTION_SENSOR_FuncDrv_t *)(
            void *)&LSM303AGR_ACC_Driver;

      if (MotionDrv[STM32L462E_CELL1_LSM303AGR_ACC_0]->Init(MotionCompObj[STM32L462E_CELL1_LSM303AGR_ACC_0])
          != LSM303AGR_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & MOTION_GYRO) == MOTION_GYRO))
    {
      /* Return an error if the application try to initialize a function not supported by the component */
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & MOTION_MAGNETO) == MOTION_MAGNETO))
    {
      /* Return an error if the application try to initialize a function not supported by the component */
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}
#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_ACC_0 == 1) */

#if (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1)
/**
  * @brief  Register Bus IOs for instance 2 if component ID is OK
  * @retval error status
  */
static int32_t LSM303AGR_MAG_0_Probe(uint32_t Functions)
{
  LSM303AGR_IO_t                io_ctx;
  uint8_t                       id;
  static LSM303AGR_MAG_Object_t lsm303agr_mag_obj_0;
  LSM303AGR_Capabilities_t      cap;
  int32_t ret = BSP_ERROR_NONE;

  /* Configure the magneto driver */
  io_ctx.BusType     = LSM303AGR_I2C_BUS; /* I2C */
  io_ctx.Address     = LSM303AGR_I2C_ADD_MG;
  io_ctx.Init        = BSP_I2C1_Init;
  io_ctx.DeInit      = BSP_I2C1_DeInit;
  io_ctx.ReadReg     = BSP_I2C1_ReadReg_8b;
  io_ctx.WriteReg    = BSP_I2C1_WriteReg_8b;
  io_ctx.GetTick     = BSP_GetTick;

  if (LSM303AGR_MAG_RegisterBusIO(&lsm303agr_mag_obj_0, &io_ctx) != LSM303AGR_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (LSM303AGR_MAG_ReadID(&lsm303agr_mag_obj_0, &id) != LSM303AGR_OK)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else if (id != (uint8_t)LSM303AGR_ID_MG)
  {
    ret = BSP_ERROR_UNKNOWN_COMPONENT;
  }
  else
  {
    (void)LSM303AGR_MAG_GetCapabilities(&lsm303agr_mag_obj_0, &cap);
    MotionCtx[STM32L462E_CELL1_LSM303AGR_MAG_0].Functions = ((uint32_t)cap.Gyro) | ((uint32_t)cap.Acc << 1) | ((
                                                              uint32_t)cap.Magneto << 2);

    MotionCompObj[STM32L462E_CELL1_LSM303AGR_MAG_0] = &lsm303agr_mag_obj_0;
    /* The second cast (void *) is added to bypass Misra R11.3 rule */
    MotionDrv[STM32L462E_CELL1_LSM303AGR_MAG_0] = (MOTION_SENSOR_CommonDrv_t *)(void *)&LSM303AGR_MAG_COMMON_Driver;

    if ((ret == BSP_ERROR_NONE) && ((Functions & MOTION_MAGNETO) == MOTION_MAGNETO) && (cap.Magneto == 1U))
    {
      /* The second cast (void *) is added to bypass Misra R11.3 rule */
      MotionFuncDrv[STM32L462E_CELL1_LSM303AGR_MAG_0][FunctionIndexMotion[MOTION_MAGNETO]] = (MOTION_SENSOR_FuncDrv_t *)(
            void *)&LSM303AGR_MAG_Driver;

      if (MotionDrv[STM32L462E_CELL1_LSM303AGR_MAG_0]->Init(MotionCompObj[STM32L462E_CELL1_LSM303AGR_MAG_0])
          != LSM303AGR_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & MOTION_ACCELERO) == MOTION_ACCELERO))
    {
      /* Return an error if the application try to initialize a function not supported by the component */
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    if ((ret == BSP_ERROR_NONE) && ((Functions & MOTION_GYRO) == MOTION_GYRO))
    {
      /* Return an error if the application try to initialize a function not supported by the component */
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
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

#endif /* (USE_STM32L462E_CELL1_MOTION_SENSOR_LSM303AGR_MAG_0 == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
