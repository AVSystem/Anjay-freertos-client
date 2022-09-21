/**
  ******************************************************************************
  * @file    at_datapack.h
  * @author  MCD Application Team
  * @brief   Header for at_datapack.c module
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
#ifndef AT_DATAPACK_H
#define AT_DATAPACK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "at_core.h"

/** @addtogroup AT_CORE AT_CORE
  * @{
  */

/** @addtogroup AT_CORE_DATAPACK AT_CORE DATAPACK
  * @{
  */

/** @defgroup AT_CORE_DATAPACK_Private_Defines AT_CORE DATAPACK Private Defines
  * @{
  */
#define DATAPACK_MAX_BUF_SIZE (ATCMD_MAX_BUF_SIZE)
/**
  * @}
  */

/** @defgroup AT_CORE_DATAPACK_Private_Types AT_CORE DATAPACK Private Types
  * @{
  */
typedef enum
{
  DATAPACK_OK = 0,
  DATAPACK_ERROR,
} DataPack_Status_t;

typedef struct
{
  void *structptr;
} datapack_structptr_t;
/**
  * @}
  */

/** @defgroup AT_CORE_DATAPACK_Exported_Functions AT_CORE DATAPACK Exported Functions
  * @{
  */
DataPack_Status_t DATAPACK_writePtr(uint8_t *p_buf, uint16_t msgtype, void *p_data);
DataPack_Status_t DATAPACK_writeStruct(uint8_t *p_buf, uint16_t msgtype, uint16_t size, void *p_data);
DataPack_Status_t DATAPACK_readPtr(uint8_t *p_buf, uint16_t msgtype, void **p_data);
DataPack_Status_t DATAPACK_readStruct(uint8_t *p_buf, uint16_t msgtype, uint16_t size, void *p_data);
uint16_t               DATAPACK_readMsgType(uint8_t *p_buf);
uint16_t               DATAPACK_readSize(uint8_t *p_buf);

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

#endif /* AT_DATAPACK_H */
