/**
  ******************************************************************************
  * @file    at_custom_sysctrl.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl_specific.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SYSCTRL_MONARCH_H
#define SYSCTRL_MONARCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_sysctrl.h"
#include "at_custom_modem_specific.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_SYSCTRL AT_CUSTOM SEQUANS_MONARCH SYSCTRL
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_SYSCTRL_Exported_Functions AT_CUSTOM SEQUANS_MONARCH SYSCTRL Exported Functions
  * @{
  */
sysctrl_status_t SysCtrl_MONARCH_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list);
sysctrl_status_t SysCtrl_MONARCH_open_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_close_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_power_on(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_power_off(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_reset(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot);

#if (ENABLE_SEQUANS_LOW_POWER_MODE == 1)
sysctrl_status_t SysCtrl_MONARCH_suspend_channel_request(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_suspend_channel_complete(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_resume_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_MONARCH_setup_LowPower_Int(uint8_t set);
#endif /* (ENABLE_SEQUANS_LOW_POWER_MODE == 1) */

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
#endif

#endif /* SYSCTRL_MONARCH_H */
