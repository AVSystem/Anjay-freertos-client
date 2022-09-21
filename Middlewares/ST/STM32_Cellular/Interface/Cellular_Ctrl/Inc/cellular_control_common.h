/**
  ******************************************************************************
  * @file    cellular_control_common.h
  * @author  MCD Application Team
  * @brief   Cellular control common definition
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
#ifndef CELLULAR_CONTROL_COMMON_H
#define CELLULAR_CONTROL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/* None */

/* Exported types ------------------------------------------------------------*/
/** @defgroup CELLULAR_CTRL_Types Types
  * @{
  */

typedef enum
{
  CELLULAR_SUCCESS                = (uint8_t)0x00, /*!< e.g: Operation was successful.                                */
  CELLULAR_ERR_BADARGUMENT        = (uint8_t)0x01, /*!< e.g: An argument value is incorrect.                          */
  CELLULAR_ERR_STATE              = (uint8_t)0x02, /*!< e.g: Operation cannot be proceed according to platform state. */
  CELLULAR_ERR_INPROGRESS         = (uint8_t)0x03, /*!< e.g: An operation in progress forbids the modification.       */
  CELLULAR_ERR_NOMEMORY           = (uint8_t)0x04, /*!< e.g: No more memory to do the request.                        */
  CELLULAR_ERR_INTERNAL           = (uint8_t)0x05, /*!< e.g: An internal cellular operation cannot be proceed.        */
  CELLULAR_ERR_NOTIMPLEMENTED     = (uint8_t)0xFF  /*!< e.g: Operation is not yet implemented.                        */
} cellular_result_t;

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/* None */

/* Exported macros -----------------------------------------------------------*/
/* None */

/* Exported functions ------------------------------------------------------- */
/* None */

#ifdef __cplusplus
}
#endif

#endif /* CELLULAR_CONTROL_COMMON_H */
