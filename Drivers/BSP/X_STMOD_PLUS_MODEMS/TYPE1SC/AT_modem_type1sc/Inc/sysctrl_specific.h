/**
  ******************************************************************************
  * @file    sysctrl_specific.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl_specific.c module for TYPE1SC
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#ifndef SYSCTRL_TYPE1SC_H
#define SYSCTRL_TYPE1SC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sysctrl.h"
#include "ipc_common.h"

/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum
{
  SYSCTRL_HW_FLOW_CONTROL_NONE,          /* force Hw Flow Control to none */
  SYSCTRL_HW_FLOW_CONTROL_RTS_CTS,       /* force Hw Flow Control to RTS/CTS */
  SYSCTRL_HW_FLOW_CONTROL_USER_SETTING,  /* apply user setting for Hw Flow Control */

} SysCtrl_TYPE1SC_HwFlowCtrl_t;

/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
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
sysctrl_status_t SysCtrl_TYPE1SC_request_suspend_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_complete_suspend_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type);
sysctrl_status_t SysCtrl_TYPE1SC_resume_channel(IPC_Handle_t *ipc_handle, sysctrl_device_type_t type,
                                                uint8_t modem_originated);

#ifdef __cplusplus
}
#endif

#endif /* SYSCTRL_TYPE1SC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

