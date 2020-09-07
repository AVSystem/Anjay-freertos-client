/*
 * Copyright 2020 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
#    define PLF_SW_CONFIG_H

#    ifdef __cplusplus
extern "C" {
#    endif

/* Includes ------------------------------------------------------------------*/
#    include "plf_features.h"
#    include "plf_modem_config.h"

/* Exported constants --------------------------------------------------------*/
/* SIM PIN Code */
#    define CST_SIM_PINCODE \
        ((uint8_t *) "") // SET PIN CODE HERE (for example "1234")  if no PIN
                         // code, use an string empty ""

/* ======================= */
/* BEGIN - Miscellaneous   */
/* ======================= */

/* IPC config BEGIN */
#    define USER_DEFINED_IPC_MAX_DEVICES (1)
#    define USER_DEFINED_IPC_DEVICE_MODEM (IPC_DEVICE_0)
/* IPC config END */

#    define PPP_NETMASK_HEX 0x00FFFFFF /* 255.255.255.0 */

/* Polling modem period */
#    if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#        define CST_MODEM_POLLING_PERIOD (10000U) /* Polling period = 10s */
#    else
#        define CST_MODEM_POLLING_PERIOD \
            (0U) /* No polling for modem monitoring */
#    endif       /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) */

/* If activated then for USE_SOCKETS_TYPE == USE_SOCKETS_MODEM
   com_getsockopt with COM_SO_ERROR parameter return a value compatible with
   errno.h see com_sockets_err_compat.c for the conversion */
#    define COM_SOCKETS_ERRNO_COMPAT (0) /* 0: not activated, 1: activated */

/* If COM_SOCKETS_STATISTIC activated then sockets statitic displayed
   on command request and/or every COM_SOCKETS_STATISTIC_PERIOD minutes */
#    if !defined COM_SOCKETS_STATISTIC
#        define COM_SOCKETS_STATISTIC (1U) // 0: not activated, 1: activated
#    endif                                 /* !defined COM_SOCKETS_STATISTIC */
/*
if COM_SOCKETS_STATISTIC_PERIOD = 0:
sockets statistic displayed only on command request
if COM_SOCKETS_STATISTIC_PERIOD != 0:
sockets statistic displayed on command request
and every COM_SOCKETS_STATISTIC_PERIOD value in min.
*/
#    define COM_SOCKETS_STATISTIC_PERIOD (0U) /* in min. */

/* FLASH config mapping */
#    define FEEPROM_UTILS_FLASH_USED (1)
#    define FEEPROM_UTILS_LAST_PAGE_ADDR (FLASH_LAST_PAGE_ADDR)
#    define FEEPROM_UTILS_APPLI_MAX 5

/* behaviour at boot selection */
#    define USE_BOOT_BEHAVIOUR_CONFIG \
        0 /* 0: automatic boot - 1: boot behaviour selection by boot menu */
#    define USE_MODEM_VOUCHER \
        0 // 0: voucher management not included - 1: voucher management included

/* ======================= */
/* END - Miscellaneous     */
/* ======================= */

/* ================================================= */
/* BEGIN - Middleware components used (expert mode)  */
/* ================================================= */
#    define RTOS_USED (1)     /* DO NOT MODIFY THIS VALUE */
#    define USE_DATACACHE (1) /* DO NOT MODIFY THIS VALUE */

/* ================================================= */
/* END - Middleware components used                  */
/* ================================================= */

/* =====================*/
/* BEGIN - Trace flags  */
/* =====================*/

#    if !defined SW_DEBUG_VERSION
#        define SW_DEBUG_VERSION \
            (0U) // 0 for SW release version (no traces), 1 for SW debug version
#    endif       /* !defined SW_DEBUG_VERSION */

#    if (SW_DEBUG_VERSION == 1U)
/* ### SOFTWARE DEBUG VERSION :  traces activated ### */
/* trace channels: ITM - UART */
#        define TRACE_IF_TRACES_ITM \
            (1U) /* trace_interface module send traces to ITM */
#        define TRACE_IF_TRACES_UART \
            (1U) /* trace_interface module send traces to UART */
#        define USE_PRINTF \
            (0U) // if set to 1, use printf instead of trace_interface module

/* trace masks allowed */
/* P0, WARN and ERROR traces only */
#        define TRACE_IF_MASK \
            (uint16_t)(DBL_LVL_P0 | DBL_LVL_WARN | DBL_LVL_ERR)
/* Full traces */
/* #define TRACE_IF_MASK    (uint16_t)(DBL_LVL_P0 | DBL_LVL_P1 | DBL_LVL_P2 |
 * DBL_LVL_WARN | DBL_LVL_ERR) */

/* trace module flags : indicate which modules are generating traces */
#        define USE_TRACE_TEST (1U)
#        define USE_TRACE_SYSCTRL (1U)
#        define USE_TRACE_ATCORE (1U)
#        define USE_TRACE_ATCUSTOM_MODEM (1U)
#        define USE_TRACE_ATCUSTOM_COMMON (1U)
#        define USE_TRACE_ATDATAPACK (1U)
#        define USE_TRACE_ATPARSER (0U)
#        define USE_TRACE_CELLULAR_SERVICE (1U)
#        define USE_TRACE_ATCUSTOM_SPECIFIC (1U)
#        define USE_TRACE_COM_SOCKETS (1U)
#        if !defined USE_TRACE_CUSTOM_CLIENT
#            define USE_TRACE_CUSTOM_CLIENT (1U)
#        endif /* !defined USE_TRACE_CUSTOM_CLIENT */
#        define USE_TRACE_ECHO_CLIENT (1U)
#        define USE_TRACE_HTTP_CLIENT (1U)
#        define USE_TRACE_PING_CLIENT (1U)
#        define USE_TRACE_COM_CLIENT (1U)
#        define USE_TRACE_MQTT_CLIENT (1U)
#        define USE_TRACE_PPPOSIF (1U)
#        define USE_TRACE_IPC (1U)
#        define USE_TRACE_DCLIB (1U)
#        define USE_TRACE_DCMEMS (1U)
#        define USE_TRACE_ERROR_HANDLER (1U)
#        define USE_TRACE_CELLULAR_MNGT (1U)

#    else
/* ### SOFTWARE RELEASE VERSION : no traces  ### */
/* trace channels: ITM - UART */
#        define TRACE_IF_TRACES_ITM (1U)         /* DO NOT MODIFY THIS VALUE */
#        define TRACE_IF_TRACES_UART (1U)        /* DO NOT MODIFY THIS VALUE */
#        define USE_PRINTF (0U)                  /* DO NOT MODIFY THIS VALUE */

/* trace masks allowed */
/* P0, WARN and ERROR traces only */
#        define TRACE_IF_MASK (uint16_t)(0U)     /* DO NOT MODIFY THIS VALUE */

/* trace module flags */
#        define USE_TRACE_TEST (0U)              /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_SYSCTRL (0U)           /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ATCORE (0U)            /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ATCUSTOM_MODEM (0U)    /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ATCUSTOM_COMMON (0U)   /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ATDATAPACK (0U)        /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ATPARSER (0U)          /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_CELLULAR_SERVICE (0U)  /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ATCUSTOM_SPECIFIC (0U) // DO NOT MODIFY THIS VALUE
#        define USE_TRACE_COM_SOCKETS (0U)       /* DO NOT MODIFY THIS VALUE */
#        if !defined USE_TRACE_CUSTOM_CLIENT
#            define USE_TRACE_CUSTOM_CLIENT (0U) // DO NOT MODIFY THIS VALUE
#        endif /* !defined USE_TRACE_CUSTOM_CLIENT */
#        define USE_TRACE_ECHO_CLIENT (0U)   /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_HTTP_CLIENT (0U)   /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_PING_CLIENT (0U)   /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_COM_CLIENT (0U)    /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_MQTT_CLIENT (0U)   /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_PPPOSIF (0U)       /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_IPC (0U)           /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_DCLIB (0U)         /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_DCMEMS (0U)        /* DO NOT MODIFY THIS VALUE */
#        define USE_TRACE_ERROR_HANDLER (0U) /* DO NOT MODIFY THIS VALUE */
#    endif                                   /* SW_DEBUG_VERSION*/

/* ===================*/
/* END - Trace flags  */
/* ===================*/

/* ================================== */
/* BEGIN -  Internal functionalities  */
/* ================================== */

/* Reserved for future use. Do dot activate ! */
#    if !defined USE_LOW_POWER
#        define USE_LOW_POWER (0) /* 0: not activated, 1: activated */
#    endif                        /* !defined USE_LOW_POWER */

/* ================================== */
/* END   -  Internal functionalities  */
/* ================================== */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#    ifdef __cplusplus
}
#    endif

#endif /* PLF_SW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
