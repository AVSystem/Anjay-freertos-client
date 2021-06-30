/**
  ******************************************************************************
  * @file    plf_sw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the software configuration of the platform
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_SW_CONFIG_H
#define PLF_SW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_features.h"

/* Exported constants --------------------------------------------------------*/
/* ======================= */
/* BEGIN - Miscellaneous   */
/* ======================= */

/* IPC config BEGIN */
#define USER_DEFINED_IPC_MAX_DEVICES        (1)
#define USER_DEFINED_IPC_DEVICE_MODEM       (IPC_DEVICE_0)
/* IPC config END */

/* Polling modem period */
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#define CST_MODEM_POLLING_PERIOD            (10000U)  /* Polling period = 10s */
#else
#define CST_MODEM_POLLING_PERIOD            (0U)      /* No polling for modem monitoring */
#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

/* If activated then for USE_SOCKETS_TYPE == USE_SOCKETS_MODEM
   com_getsockopt with COM_SO_ERROR parameter return a value compatible with errno.h
   see com_sockets_err_compat.c for the conversion */
#define COM_SOCKETS_ERRNO_COMPAT            (0) /* 0: not activated, 1: activated */

/* If COM_SOCKETS_STATISTIC activated then sockets statistic displayed
   on command request and/or every COM_SOCKETS_STATISTIC_PERIOD minutes */
#if !defined COM_SOCKETS_STATISTIC
#define COM_SOCKETS_STATISTIC               (1U) /* 0: not activated, 1: activated */
#endif /* !defined COM_SOCKETS_STATISTIC */

/* ======================= */
/* END - Miscellaneous     */
/* ======================= */

/* ========================== */
/* BEGIN - Debug Trace flags  */
/* ========================== */

#if !defined SW_DEBUG_VERSION
#define SW_DEBUG_VERSION                    (0U)   /* 0 for SW release version (no traces),
                                                      1 for SW debug version */
#endif /* !defined SW_DEBUG_VERSION */

#if (SW_DEBUG_VERSION == 1U)
/* ### SOFTWARE DEBUG VERSION :  traces activated ### */
/* Trace channels: ITM, UART or Printf */
#define TRACE_IF_TRACES_ITM                 (1U) /* trace_interface module send traces to ITM */
#define TRACE_IF_TRACES_UART                (1U) /* trace_interface module send traces to UART */
#define USE_PRINTF                          (0U) /* if set to 1, use printf instead of trace_interface module */

/* Trace masks allowed */
/* by default: P0, WARN and ERROR traces only */
#define TRACE_IF_MASK                       (uint16_t)(DBL_LVL_P0 | DBL_LVL_WARN | DBL_LVL_ERR)
/* Full traces */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_P0 | DBL_LVL_P1 | DBL_LVL_P2 | DBL_LVL_WARN | DBL_LVL_ERR) */

/* Trace module flags : indicate which modules are generating traces */
#define USE_TRACE_SYSCTRL                   (1U)
#define USE_TRACE_ATCORE                    (1U)
#define USE_TRACE_ATPARSER                  (1U)
#define USE_TRACE_ATDATAPACK                (1U)
#define USE_TRACE_ATCUSTOM_MODEM            (1U)
#define USE_TRACE_ATCUSTOM_COMMON           (1U)
#define USE_TRACE_ATCUSTOM_SPECIFIC         (1U)
#define USE_TRACE_CELLULAR_SERVICE          (1U)
#define USE_TRACE_COMLIB                    (1U)
#define USE_TRACE_IPC                       (1U)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#define USE_TRACE_PPPOSIF                   (1U)
#else
#define USE_TRACE_PPPOSIF                   (0U)
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
#define USE_TRACE_ERROR_HANDLER             (1U)
#define USE_TRACE_TEST                      (1U)
#if !defined USE_TRACE_APPLICATION
#define USE_TRACE_APPLICATION               (0U) /* No application defined */
#endif /* !defined USE_TRACE_APPLICATION */

#else
/* ### SOFTWARE RELEASE VERSION : very limited traces  ### */
/* Trace channels: ITM, UART or Printf */
#define TRACE_IF_TRACES_ITM                 (1U) /* trace_interface module send traces to ITM */
#define TRACE_IF_TRACES_UART                (1U) /* trace_interface module send traces to UART */
#define USE_PRINTF                          (0U) /* if set to 1, use printf instead of trace_interface module */

/* Trace masks allowed */
#define TRACE_IF_MASK             (uint16_t)(0U) /* no trace except the one using PRINT_FORCE */

/* Trace module flags */
#define USE_TRACE_SYSCTRL                   (0U)
#define USE_TRACE_ATCORE                    (0U)
#define USE_TRACE_ATPARSER                  (0U)
#define USE_TRACE_ATDATAPACK                (0U)
#define USE_TRACE_ATCUSTOM_MODEM            (0U)
#define USE_TRACE_ATCUSTOM_COMMON           (0U)
#define USE_TRACE_ATCUSTOM_SPECIFIC         (0U)
#define USE_TRACE_CELLULAR_SERVICE          (0U)
#define USE_TRACE_COMLIB                    (0U)
#define USE_TRACE_IPC                       (0U)
#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#define USE_TRACE_PPPOSIF                   (0U)
#else
#define USE_TRACE_PPPOSIF                   (0U)
#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
#define USE_TRACE_ERROR_HANDLER             (0U)
#define USE_TRACE_TEST                      (0U)
#if !defined USE_TRACE_APPLICATION
#define USE_TRACE_APPLICATION               (0U)
#endif /* !defined USE_TRACE_APPLICATION */
#endif /* SW_DEBUG_VERSION*/

/* ========================== */
/* END   - Debug Trace flags  */
/* ========================== */

/* ================================= */
/* BEGIN - Internal functionalities  */
/* ================================= */

/* Reserved for future use. Do not activate ! */
#if !defined USE_LOW_POWER
#define USE_LOW_POWER                       (0) /* 0: not activated, 1: activated */
#endif  /* !defined USE_LOW_POWER */

/* ================================= */
/* END   - Internal functionalities  */
/* ================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_SW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
