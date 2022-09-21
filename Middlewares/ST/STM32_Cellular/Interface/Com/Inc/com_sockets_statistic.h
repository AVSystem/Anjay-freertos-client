/**
  ******************************************************************************
  * @file    com_sockets_statistic.h
  * @author  MCD Application Team
  * @brief   This file implements Socket statistic
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
#ifndef COM_SOCKETS_STATISTIC_H
#define COM_SOCKETS_STATISTIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"

#if (USE_COM_SOCKETS == 1)

/* Exported constants --------------------------------------------------------*/
/* None */


/* Exported types ------------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Types
  * @{
  */

/* Internal usage only: use by com_sockets_ip_modem to update statistics */
typedef enum
{
  COM_SOCKET_STAT_CRE_OK = 0,
  COM_SOCKET_STAT_CRE_NOK,
  COM_SOCKET_STAT_CNT_OK,
  COM_SOCKET_STAT_CNT_NOK,
  COM_SOCKET_STAT_SND_OK,
  COM_SOCKET_STAT_SND_NOK,
  COM_SOCKET_STAT_RCV_OK,
  COM_SOCKET_STAT_RCV_NOK,
  COM_SOCKET_STAT_CLS_OK,
  COM_SOCKET_STAT_CLS_NOK,
  COM_SOCKET_STAT_NWK_UP,
  COM_SOCKET_STAT_NWK_DWN
} com_sockets_stat_update_t;

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/* None */

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions ------------------------------------------------------- */
/** @addtogroup COM_SOCKETS_Functions_Other
  * @{
  */

/**
  * @brief  Display com sockets statistics
  * @note   COM_SOCKETS_STATISTIC and USE_TRACE_COM_SOCKETS must be set to 1
  * @param  -
  * @retval -
  */
void com_sockets_statistic_display(void);

/**
  * @}
  */

/*** Component Initialization *************************************************/
/*** Used by com_sockets module - Not an User Interface ***********************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  -
  * @retval -
  */
void com_sockets_statistic_init(void);

/*** Component Statistic Update ***********************************************/
/*** Used by com_sockets_* module - Not an User Interface *********************/

/**
  * @brief  Managed com sockets statistic update and print
  * @note   -
  * @param  stat - to know what the function has to do
  * @note   statistic init, update or print
  * @retval -
  */
void com_sockets_statistic_update(com_sockets_stat_update_t stat);

#endif /* USE_COM_SOCKETS == 1 */

#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_STATISTIC_H */
