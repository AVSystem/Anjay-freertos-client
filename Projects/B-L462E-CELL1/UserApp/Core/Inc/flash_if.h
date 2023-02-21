/**
  ******************************************************************************
  * @file    flash_if.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for FLASH Interface functionalities.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FLASH_IF_H
#define FLASH_IF_H

/** @addtogroup USER_APP User App Example
  * @{
  */

/** @addtogroup USER_APP_COMMON Common
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define FLASH_IF_MIN_WRITE_LEN (8U)  /* Flash programming by 64 bits */
/* Exported functions ------------------------------------------------------- */

HAL_StatusTypeDef FLASH_If_Init(void);
HAL_StatusTypeDef FLASH_If_Write(void *pDestination, const void *pSource, uint32_t uLength);
HAL_StatusTypeDef FLASH_If_Read(void *pDestination, const void *pSource, uint32_t uLength);
HAL_StatusTypeDef FLASH_If_Erase_Size(void *pStart, uint32_t uLength);
HAL_StatusTypeDef FLASH_INT_Init(void);
HAL_StatusTypeDef FLASH_INT_If_Write(void *pDestination, const void *pSource, uint32_t uLength);
HAL_StatusTypeDef FLASH_INT_If_Read(void *pDestination, const void *pSource, uint32_t uLength);
HAL_StatusTypeDef FLASH_INT_If_Erase_Size(void *pStart, uint32_t uLength);
HAL_StatusTypeDef FLASH_EXT_Init(void);
HAL_StatusTypeDef FLASH_EXT_If_Write(void *pDestination, const void *pSource, uint32_t uLength);
HAL_StatusTypeDef FLASH_EXT_If_Read(void *pDestination, const void *pSource, uint32_t uLength);
HAL_StatusTypeDef FLASH_EXT_If_Erase_Size(void *pStart, uint32_t uLength);


/**
  * @}
  */

/**
  * @}
  */

#endif  /* FLASH_IF_H */
