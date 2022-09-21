/**
  ******************************************************************************
  * @file    error_handler.h
  * @author  MCD Application Team
  * @brief   Header for error_handler.c module
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
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "trace_interface.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  ERROR_NO = 0,
  ERROR_DEBUG,
  ERROR_WARNING,
  ERROR_FATAL,
} error_gravity_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialize error handler module
  * @param  -
  * @retval -
  */
void ERROR_Handler_Init(void);

/**
  * @brief  Log an error
  * @param  chan    - channel/component in error
  * @param  errorId - error value to log
  * @param  gravity - error gravity to log - if equal to ERROR_FATAL then a SystemReset() is done
  * @retval -
  */
void ERROR_Handler(dbg_channels_t chan, int32_t errorId, error_gravity_t gravity);

/**
  * @brief  Dump all errors logged with a trace
  * @param  -
  * @retval -
  */
void ERROR_Dump_All(void);

/**
  * @brief  Dump the last error logged
  * @param  -
  * @retval -
  */
void ERROR_Dump_Last(void);


#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLER_H */
