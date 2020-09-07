/**
  ******************************************************************************
  * @file    com_common.h
  * @author  MCD Application Team
  * @brief   COM module Common definition
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef COM_COMMON_H
#define COM_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "plf_config.h"

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
/** @note Next define is to ensure compatibility with previous release */
typedef bool com_bool_t;

typedef uint8_t com_char_t;


/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/** @addtogroup COM_Variables
  * @{
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @addtogroup COM_Macros
  * @{
  */

/**
  * @}
  */

/* Exported functions ------------------------------------------------------- */

/** @addtogroup COM_Functions
  * @{
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* COM_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
