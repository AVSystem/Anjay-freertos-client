/**
  ******************************************************************************
  * @file    at_custom_sysctrl.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl_specific.c module for BG96
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
#ifndef SYSCTRL_BG96_H
#define SYSCTRL_BG96_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_sysctrl.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96 AT_CUSTOM QUECTEL_BG96
  * @{
  */

/** @addtogroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL AT_CUSTOM QUECTEL_BG96 SYSCTRL
  * @{
  */

/** @defgroup AT_CUSTOM_QUECTEL_BG96_SYSCTRL_Exported_Functions AT_CUSTOM QUECTEL_BG96 SYSCTRL Exported Functions
  * @{
  */
sysctrl_status_t SysCtrl_BG96_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list);
sysctrl_status_t SysCtrl_BG96_open_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_BG96_close_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_BG96_power_on(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_BG96_power_off(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_BG96_reset(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_BG96_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot);

/* Note: following functions are not part of standard sysctrl API
 * They are very specific to this modem and are called by at_custom files of this modem
 */
sysctrl_status_t SysCtrl_BG96_wakeup_from_PSM(uint32_t delay);

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

#endif /* SYSCTRL_BG96_H */

