/**
  ******************************************************************************
  * @file    at_sysctrl.c
  * @author  MCD Application Team
  * @brief   This file provides code for generic System Control
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

/* Includes ------------------------------------------------------------------*/
#include "at_sysctrl.h"
#include "at_custom_modem_api.h"
#include "plf_config.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SYSCTRL AT_CORE SYSCTRL
  * @{
  */

/** @defgroup AT_CORE_SYSCTRL_Private_Macros AT_CORE SYSCTRL Private Macros
  * @{
  */
#if (USE_TRACE_SYSCTRL == 1U)
#if (USE_PRINTF == 0U)
#include "trace_interface.h"
#define PRINT_DBG(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_P1, "SysCtrl:" format "\n\r", ## args)
#define PRINT_ERR(format, args...)  TRACE_PRINT(DBG_CHAN_ATCMD, DBL_LVL_ERR, "SysCtrl ERROR:" format "\n\r", ## args)
#else
#include <stdio.h>
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(format, args...)   (void) printf("SysCtrl ERROR:" format "\n\r", ## args);
#endif /* USE_PRINTF */
#else
#define PRINT_DBG(...)   __NOP(); /* Nothing to do */
#define PRINT_ERR(...)   __NOP(); /* Nothing to do */
#endif /* USE_TRACE_SYSCTRL */
/**
  * @}
  */

/** @defgroup AT_CORE_SYSCTRL_Private_Variables AT_CORE SYSCTRL Private Variables
  * @{
  */
/* AT custom functions ptrs table */
static sysctrl_funcPtrs_t sysctrl_custom_func[DEVTYPE_INVALID] = {0U};
/**
  * @}
  */

/** @defgroup AT_CORE_SYSCTRL_Exported_Functions AT_CORE SYSCTRL Exported Functions
  * @{
  */
/**
  * @brief  SysCtrl_getDeviceDescriptor
  * @param  device_type
  * @param  p_devices_list
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_getDeviceDescriptor(sysctrl_device_type_t device_type, sysctrl_info_t *p_devices_list)
{
  sysctrl_status_t retval;

  /* check input parameters validity */
  if (p_devices_list == NULL)
  {
    retval = SCSTATUS_ERROR;
  }
  else
  {
    /* check if device is already initialized */
    if (sysctrl_custom_func[device_type].initialized == 0U)
    {
      /* Init SysCtrl functions pointers */
      (void) atcma_init_sysctrl_func_ptrs(&sysctrl_custom_func[device_type]);

      /* device is initialized now */
      sysctrl_custom_func[device_type].initialized = 1U;
    }
    retval = (*sysctrl_custom_func[device_type].f_getDeviceDescriptor)(device_type, p_devices_list);
  }

  return (retval);
}

/**
  * @brief  SysCtrl_open_channel
  * @param  device_type
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_open_channel(sysctrl_device_type_t device_type)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  /* check if device is initialized */
  if (sysctrl_custom_func[device_type].initialized == 1U)
  {
    retval = (*sysctrl_custom_func[device_type].f_open_channel)(device_type);
  }
  else
  {
    PRINT_ERR("Device type %d is not initialized", device_type)
  }

  return (retval);
}

/**
  * @brief  SysCtrl_close_channel
  * @param  device_type
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_close_channel(sysctrl_device_type_t device_type)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  /* check if device is initialized */
  if (sysctrl_custom_func[device_type].initialized == 1U)
  {
    retval = (*sysctrl_custom_func[device_type].f_close_channel)(device_type);
  }
  else
  {
    PRINT_ERR("Device type %d is not initialized", device_type)
  }

  return (retval);
}

/**
  * @brief  SysCtrl_power_on
  * @param  device_type
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_power_on(sysctrl_device_type_t device_type)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  /* check if device is initialized */
  if (sysctrl_custom_func[device_type].initialized == 1U)
  {
    retval = (*sysctrl_custom_func[device_type].f_power_on)(device_type);
  }
  else
  {
    PRINT_ERR("Device type %d is not initialized", device_type)
  }

  return (retval);
}

/**
  * @brief  SysCtrl_power_off
  * @param  device_type
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_power_off(sysctrl_device_type_t device_type)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  /* check if device is initialized */
  if (sysctrl_custom_func[device_type].initialized == 1U)
  {
    retval = (*sysctrl_custom_func[device_type].f_power_off)(device_type);
  }
  else
  {
    PRINT_ERR("Device type %d is not initialized", device_type)
  }

  return (retval);
}

/**
  * @brief  SysCtrl_reset_device
  * @param  device_type
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_reset_device(sysctrl_device_type_t device_type)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  /* check if device is initialized */
  if (sysctrl_custom_func[device_type].initialized == 1U)
  {
    retval = (*sysctrl_custom_func[device_type].f_reset_device)(device_type);
  }
  else
  {
    PRINT_ERR("Device type %d is not initialized", device_type)
  }

  return (retval);
}

/**
  * @brief  SysCtrl_sim_select
  * @param  device_type
  * @param  sim_slot
  * @retval sysctrl_status_t
  */
sysctrl_status_t SysCtrl_sim_select(sysctrl_device_type_t device_type, sysctrl_sim_slot_t sim_slot)
{
  sysctrl_status_t retval = SCSTATUS_ERROR;

  /* check if device is initialized */
  if (sysctrl_custom_func[device_type].initialized == 1U)
  {
    retval = (*sysctrl_custom_func[device_type].f_sim_select)(device_type, sim_slot);
  }
  else
  {
    PRINT_ERR("Device type %d is not initialized", device_type)
  }

  return (retval);
}

/**
  * @brief  SysCtrl_delay
  * @param  timeMs
  * @retval none
  */
void SysCtrl_delay(uint32_t timeMs)
{
  if (timeMs != 0U)
  {
    (void) rtosalDelay(timeMs);
  }
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
