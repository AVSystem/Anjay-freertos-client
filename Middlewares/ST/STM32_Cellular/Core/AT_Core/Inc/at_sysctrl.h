/**
  ******************************************************************************
  * @file    at_sysctrl.h
  * @author  MCD Application Team
  * @brief   Header for sysctrl.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AT_SYSCTRL_H
#define AT_SYSCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ipc_common.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_SYSCTRL AT_CORE SYSCTRL
  * @{
  */

/** @defgroup AT_CORE_SYSCTRL_Exported_Defines AT_CORE SYSCTRL Exported Defines
  * @{
  */
#define MAX_CONNECTED_DEVICES (2U) /* maximum number of hardwares devices (modem, wifi,...) connected */
/**
  * @}
  */

/** @defgroup AT_CORE_SYSCTRL_Exported_Types AT_CORE SYSCTRL Exported Types
  * @{
  */
typedef enum
{
  SCSTATUS_OK = 0,
  SCSTATUS_ERROR,
} sysctrl_status_t;

typedef enum
{
  DEVTYPE_MODEM_CELLULAR = 0,
  /*  DEVTYPE_WIFI, */
  /*  DEVTYPE_GPS, */
  /* etc... all modules using AT commands */

  /* --- */
  DEVTYPE_INVALID,      /* keep it last */
} sysctrl_device_type_t;

typedef enum
{
  SC_MODEM_SIM_SOCKET_0 = 0,    /* to select SIM card placed in SIM socket */
  SC_MODEM_SIM_ESIM_1,          /* to select integrated SIM in modem module */
  SC_STM32_SIM_2,               /* to select SIM in STM32 side (various implementations) */
} sysctrl_sim_slot_t;

typedef struct
{
  sysctrl_device_type_t       type;
  IPC_Device_t                ipc_device;
  IPC_Interface_t             ipc_interface;
} sysctrl_info_t;

typedef sysctrl_status_t (*SC_getDeviceDescriptor)(sysctrl_device_type_t device_type, sysctrl_info_t *p_devices_list);
typedef sysctrl_status_t (*SC_open_channel)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_close_channel)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_power_on)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_power_off)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_power_reset_device)(sysctrl_device_type_t device_type);
typedef sysctrl_status_t (*SC_sim_select)(sysctrl_device_type_t device_type, sysctrl_sim_slot_t sim_slot);

typedef struct
{
  uint8_t                     initialized;
  SC_getDeviceDescriptor      f_getDeviceDescriptor;
  SC_open_channel             f_open_channel;
  SC_close_channel            f_close_channel;
  SC_power_on                 f_power_on;
  SC_power_off                f_power_off;
  SC_power_reset_device       f_reset_device;
  SC_sim_select               f_sim_select;
} sysctrl_funcPtrs_t;
/**
  * @}
  */

/** @defgroup AT_CORE_SYSCTRL_Exported_Functions AT_CORE SYSCTRL Exported Functions
  * @{
  */
sysctrl_status_t SysCtrl_getDeviceDescriptor(sysctrl_device_type_t device_type, sysctrl_info_t *p_devices_list);
sysctrl_status_t SysCtrl_open_channel(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_close_channel(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_power_on(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_power_off(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_reset_device(sysctrl_device_type_t device_type);
sysctrl_status_t SysCtrl_sim_select(sysctrl_device_type_t device_type, sysctrl_sim_slot_t sim_slot);
void SysCtrl_delay(uint32_t timeMs);
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

#endif /* AT_SYSCTRL_H */
