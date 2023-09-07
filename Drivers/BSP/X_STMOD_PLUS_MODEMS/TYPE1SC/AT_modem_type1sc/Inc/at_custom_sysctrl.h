/**
  ******************************************************************************
  * @file    at_custom_sysctrl.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl_specific.c module for TYPE1SC
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
#ifndef SYSCTRL_TYPE1SC_H
#define SYSCTRL_TYPE1SC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_sysctrl.h"
#include "ipc_common.h"

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC AT_CUSTOM ALTAIR_T1SC
  * @{
  */

/** @addtogroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL AT_CUSTOM ALTAIR_T1SC SYSCTRL
  * @{
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Exported_Types AT_CUSTOM ALTAIR_T1SC SYSCTRL Exported Types
  * @{
  */
typedef enum
{
  SYSCTRL_HW_FLOW_CONTROL_NONE,          /* force Hw Flow Control to none */
  SYSCTRL_HW_FLOW_CONTROL_RTS_CTS,       /* force Hw Flow Control to RTS/CTS */
  SYSCTRL_HW_FLOW_CONTROL_USER_SETTING,  /* apply user setting for Hw Flow Control */

} SysCtrl_TYPE1SC_HwFlowCtrl_t;
/**
  * @}
  */

/** @defgroup AT_CUSTOM_ALTAIR_T1SC_SYSCTRL_Exported_Functions AT_CUSTOM ALTAIR_T1SC SYSCTRL Exported Functions
  * @{
  */
sysctrl_status_t SysCtrl_TYPE1SC_getDeviceDescriptor(sysctrl_device_type_t type, sysctrl_info_t *p_devices_list);
sysctrl_status_t SysCtrl_TYPE1SC_open_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_close_channel(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_power_on(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_power_off(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_reset(sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_sim_select(sysctrl_device_type_t type, sysctrl_sim_slot_t sim_slot);

/* Note: following functions are not part of standard sysctrl API
 * They are very specific to this modem and are called by at_custom files of this modem
 */
sysctrl_status_t SysCtrl_TYPE1SC_reinit_channel(IPC_Handle_t *ipc_handle, SysCtrl_TYPE1SC_HwFlowCtrl_t hwFC_status);
#if (ENABLE_T1SC_LOW_POWER_MODE != 0U)
sysctrl_status_t SysCtrl_TYPE1SC_request_suspend_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_complete_suspend_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_resume_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type,
                                                uint8_t modem_originated);
#endif /* (ENABLE_T1SC_LOW_POWER_MODE != 0U) */

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

#endif /* SYSCTRL_TYPE1SC_H */

