/**
  ******************************************************************************
  * @file    com_common.h
  * @author  MCD Application Team
  * @brief   COM module Common definition
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COM_COMMON_H
#define COM_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/** @addtogroup COM_Constants
  * @{
  */
#define COM_HANDLE_INVALID_ID ((int32_t)-1) /*!< Handle invalid Id */

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @addtogroup COM_Types
  * @{
  */

typedef uint8_t com_char_t;

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

#endif /* COM_COMMON_H */
