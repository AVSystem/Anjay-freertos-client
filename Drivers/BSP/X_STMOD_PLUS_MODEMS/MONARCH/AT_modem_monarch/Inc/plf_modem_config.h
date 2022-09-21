/**
  ******************************************************************************
  * @file    plf_modem_config.h
  * @author  MCD Application Team
  * @brief   This file contains the modem configuration for VZM20X/GM01Q
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
#ifndef PLF_MODEM_CONFIG_H
#define PLF_MODEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup AT_CUSTOM AT_CUSTOM
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH AT_CUSTOM SEQUANS_MONARCH
  * @{
  */

/** @addtogroup AT_CUSTOM_SEQUANS_MONARCH_CONFIG AT_CUSTOM SEQUANS_MONARCH CONFIG
  * @{
  */

/** @defgroup AT_CUSTOM_SEQUANS_MONARCH_CONFIG_Exported_Defines AT_CUSTOM SEQUANS_MONARCH CONFIG Exported Defines
  * @{
  */
#if defined(HWREF_GM01QDBA1)
/* already explicitly defined:
 * using HWREF_GM01QDBA1 directly on STMOD+ connector
 */
#else
/* set default config */
#define HWREF_GM01QDBA1
#endif /* HWREF_GM01QDBA1 */

/* Low Power */
#define ENABLE_MONARCH_PSM             (1U) /* active PSM: 1U, deactivate PSM: 0U */
#define ENABLE_MONARCH_LOW_POWER_MODE  (0U) /* Monarch Low Power mode (UART deactivation): not supported yet */

/* MODEM parameters */
#define USE_MODEM_GM01Q
#define CONFIG_MODEM_UART_BAUDRATE (115200U)
#define CONFIG_MODEM_USE_STMOD_CONNECTOR

#define UDP_SERVICE_SUPPORTED                (0U)
#define CONFIG_MODEM_UDP_SERVICE_CONNECT_IP  ((uint8_t *)"127.0.0.1")
#define CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE ((uint32_t)1460U)
#define CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE ((uint32_t)1500U)
#define CONFIG_MODEM_MAX_SIM_GENERIC_ACCESS_CMD_SIZE ((uint32_t)1460U)
#define CONFIG_MODEM_MIN_SIM_GENERIC_ACCESS_RSP_SIZE ((uint32_t)4U)

/* Ping URC received before or after Reply */
#define PING_URC_RECEIVED_AFTER_REPLY        (0U)

/* UART flow control settings */
#if defined(USER_FLAG_MODEM_FORCE_NO_FLOW_CTRL)
#define CONFIG_MODEM_UART_RTS_CTS  (0)
#elif defined(USER_FLAG_MODEM_FORCE_HW_FLOW_CTRL)
#define CONFIG_MODEM_UART_RTS_CTS  (1)
#else /* default FLOW CONTROL setting for GM01Q */
#define CONFIG_MODEM_UART_RTS_CTS  (1)
#endif /* user flag for modem flow control */

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

#endif /*_PLF_MODEM_CONFIG_H */

