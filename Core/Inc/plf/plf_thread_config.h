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
 * @file    plf_thread_config.h
 * @author  MCD Application Team
 * @brief   This file contains thread configuration
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
#ifndef PLF_THREAD_CONFIG_H
#    define PLF_THREAD_CONFIG_H

#    ifdef __cplusplus
extern "C" {
#    endif

/* Includes ------------------------------------------------------------------*/

#    include "plf_features.h"

/* Exported constants --------------------------------------------------------*/

/* ========================*/
/* BEGIN - Stack Priority  */
/* ========================*/
#    define TCPIP_THREAD_PRIO osPriorityBelowNormal
#    define PPPOSIF_CLIENT_THREAD_PRIO osPriorityHigh
#    define BOARD_BUTTONS_THREAD_PRIO osPriorityBelowNormal
#    define DC_TEST_THREAD_PRIO osPriorityNormal
#    define DC_EMUL_THREAD_PRIO osPriorityNormal
#    define ATCORE_THREAD_STACK_PRIO osPriorityNormal
#    define CELLULAR_SERVICE_THREAD_PRIO osPriorityNormal
#    define NIFMAN_THREAD_PRIO osPriorityNormal
#    define CTRL_THREAD_PRIO osPriorityAboveNormal

/* ========================*/
/* END - Stack Priority    */
/* ========================*/

/* ========================*/
/* BEGIN - Stack Size      */
/* ========================*/
#    define TCPIP_THREAD_STACK_SIZE (512U)
#    define DEFAULT_THREAD_STACK_SIZE (512U)
#    define FREERTOS_TIMER_THREAD_STACK_SIZE (256U)
#    define FREERTOS_IDLE_THREAD_STACK_SIZE (128U)

#    define ATCORE_THREAD_STACK_SIZE (384U)
#    define CELLULAR_SERVICE_THREAD_STACK_SIZE (512U)
#    define NIFMAN_THREAD_STACK_SIZE (384U)

#    if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#        define PPPOSIF_CLIENT_THREAD_STACK_SIZE (640U)
#    endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#    define BOARD_BUTTONS_THREAD_STACK_SIZE (256U)

/* ========================*/
/* END - Stack Size        */
/* ========================*/

#    define USED_ATCORE_THREAD_STACK_SIZE ATCORE_THREAD_STACK_SIZE
#    define USED_CELLULAR_SERVICE_THREAD_STACK_SIZE \
        CELLULAR_SERVICE_THREAD_STACK_SIZE
#    define USED_NIFMAN_THREAD_STACK_SIZE NIFMAN_THREAD_STACK_SIZE
#    define USED_DEFAULT_THREAD_STACK_SIZE DEFAULT_THREAD_STACK_SIZE
#    define USED_FREERTOS_TIMER_THREAD_STACK_SIZE \
        FREERTOS_TIMER_THREAD_STACK_SIZE
#    define USED_FREERTOS_IDLE_THREAD_STACK_SIZE FREERTOS_IDLE_THREAD_STACK_SIZE

#    define USED_ATCORE_THREAD 1
#    define USED_CELLULAR_SERVICE_THREAD 1
#    define USED_NIFMAN_THREAD 1
#    define USED_DEFAULT_THREAD 1
#    define USED_FREERTOS_TIMER_THREAD 1
#    define USED_FREERTOS_IDLE_THREAD 1

#    define USED_BOARD_BUTTONS_THREAD_STACK_SIZE BOARD_BUTTONS_THREAD_STACK_SIZE
#    define USED_BOARD_BUTTONS_THREAD 1

#    if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
/* check value in FreeRTOSConfig.h */
#        define USED_TCPIP_THREAD_STACK_SIZE TCPIP_THREAD_STACK_SIZE
#        define USED_TCPIP_THREAD 1
#    else
#        define USED_TCPIP_THREAD_STACK_SIZE 0U
#        define USED_TCPIP_THREAD 0
#    endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#    if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#        define USED_PPPOSIF_CLIENT_THREAD_STACK_SIZE \
            PPPOSIF_CLIENT_THREAD_STACK_SIZE
#        define USED_PPPOSIF_CLIENT_THREAD 1
#    else
#        define USED_PPPOSIF_CLIENT_THREAD_STACK_SIZE 0U
#        define USED_PPPOSIF_CLIENT_THREAD 0
#    endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) */

#    define USED_CUSTOMCLIENT_THREAD_STACK_SIZE CUSTOMCLIENT_THREAD_STACK_SIZE
#    define USED_CUSTOMCLIENT_THREAD CUSTOMCLIENT_THREAD

#    define USED_NET_CELLULAR_THREAD_STACK_SIZE 0U
#    define USED_NET_CELLULAR_THREAD 0

#    define USED_NOTIFICATION_THREAD_STACK_SIZE NOTIFICATION_THREAD_STACK_SIZE
#    define USED_NOTIFICATION_THREAD NOTIFICATION_THREAD

/* ============================================*/
/* BEGIN - Total Stack Size/Number Calculation */
/* ============================================*/

#    define TOTAL_THREAD_STACK_SIZE                                            \
        (size_t)(USED_TCPIP_THREAD_STACK_SIZE + USED_DEFAULT_THREAD_STACK_SIZE \
                 + USED_FREERTOS_TIMER_THREAD_STACK_SIZE                       \
                 + USED_FREERTOS_IDLE_THREAD_STACK_SIZE                        \
                 + USED_PPPOSIF_CLIENT_THREAD_STACK_SIZE                       \
                 + USED_BOARD_BUTTONS_THREAD_STACK_SIZE                        \
                 + USED_ATCORE_THREAD_STACK_SIZE                               \
                 + USED_CELLULAR_SERVICE_THREAD_STACK_SIZE                     \
                 + USED_NIFMAN_THREAD_STACK_SIZE                               \
                 + USED_CUSTOMCLIENT_THREAD_STACK_SIZE                         \
                 + USED_NET_CELLULAR_THREAD_STACK_SIZE                         \
                 + USED_NOTIFICATION_THREAD_STACK_SIZE)

#    define THREAD_NUMBER                                                  \
        (uint8_t)(USED_TCPIP_THREAD + USED_DEFAULT_THREAD                  \
                  + USED_FREERTOS_TIMER_THREAD + USED_FREERTOS_IDLE_THREAD \
                  + USED_PPPOSIF_CLIENT_THREAD + USED_BOARD_BUTTONS_THREAD \
                  + USED_ATCORE_THREAD + USED_CELLULAR_SERVICE_THREAD      \
                  + USED_NIFMAN_THREAD + USED_CUSTOMCLIENT_THREAD          \
                  + USED_NET_CELLULAR_THREAD + USED_NOTIFICATION_THREAD)

#    ifndef APPLICATION_HEAP_SIZE
#        define APPLICATION_HEAP_SIZE (0U)
#    endif /* APPLICATION_HEAP_SIZE */

/*
PARTIAL_HEAP_SIZE is used by:
- RTOS Timer/Mutex/Semaphore/Message object and extra pvPortMalloc call
*/
/* cost by:
   Mutex/Semaphore # 88 bytes
   Queue           # 96 bytes
   Thread          #104 bytes
   Timer           # 56 bytes
*/
#    define PARTIAL_HEAP_SIZE (THREAD_NUMBER * 600U)
#    define TOTAL_HEAP_SIZE                                            \
        ((TOTAL_THREAD_STACK_SIZE * 4U) + (size_t) (PARTIAL_HEAP_SIZE) \
         + (size_t) (APPLICATION_HEAP_SIZE))

/* ============================================*/
/* END - Total Stack Size/Number Calculation   */
/* ============================================*/

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#    ifdef __cplusplus
}
#    endif

#endif /* PLF_THREAD_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
