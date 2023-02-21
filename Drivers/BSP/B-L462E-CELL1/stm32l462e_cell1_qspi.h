/**
  ******************************************************************************
  * @file    stm32l462e_cell1_qspi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the QSPI driver.
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
#ifndef STM32L462E_CELL1_QSPI_H
#define STM32L462E_CELL1_QSPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_conf.h"
#include "stm32l462e_cell1_errno.h"
#include "stm32l4xx_hal_qspi.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_QSPI STM32L462E_CELL1 QSPI
  * @{
  */

/** @defgroup STM32L462E_CELL1_QSPI_Exported_Defines STM32L462E_CELL1 QSPI Exported Defines
  * @{
  */

/* base address for Memory Mapped mode */
#define BSP_QSPI_BASE_ADDR       ((uint32_t)0x90000000)
/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_QSPI_Exported_Types STM32L462E_CELL1 QSPI Exported Types
  * @{
  */

/* Definition for QSPI information structure */
typedef struct
{
  uint32_t FlashSize;          /*!< Size of the flash */
  uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
  uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
  uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
  uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
} BSP_QSPI_Info_t;

/* Definition for QSPI modes */
typedef enum
{
  BSP_QSPI_SPI_MODE = 0,                 /*!< 1-1-1 commands, Power on H/W default setting  */
  BSP_QSPI_SPI_1I2O_MODE,                /*!< 1-1-2 commands                                */
  BSP_QSPI_SPI_2IO_MODE,                 /*!< 1-2-2 commands                                */
  BSP_QSPI_SPI_1I4O_MODE,                /*!< 1-1-4 commands                                */
  BSP_QSPI_SPI_4IO_MODE,                 /*!< 1-4-4 commands                                */
  BSP_QSPI_DPI_MODE,                     /*!< 2-2-2 commands                                */
  BSP_QSPI_QPI_MODE                      /*!< 4-4-4 commands                                */
} BSP_QSPI_Interface_t;

/* Definition for QSPI transfer rates */
typedef enum
{
  BSP_QSPI_STR_TRANSFER = 0,             /*!< Single Transfer Rate                          */
  BSP_QSPI_DTR_TRANSFER                  /*!< Double Transfer Rate                          */
} BSP_QSPI_Transfer_t;

/* Definition for QSPI dual flash mode */
typedef enum
{
  BSP_QSPI_DUALFLASH_DISABLE = 0, /*!<  Single flash mode              */
  BSP_QSPI_DUALFLASH_ENABLE       /*!<  Dual flash mode                */
} BSP_QSPI_DualFlash_t;

/* QSPI erase types */
typedef enum
{
  BSP_QSPI_ERASE_4K = 0,                 /*!< 4K size Sector erase                          */
  BSP_QSPI_ERASE_32K,                    /*!< 32K size Block erase                          */
  BSP_QSPI_ERASE_64K,                    /*!< 64K size Block erase                          */
  BSP_QSPI_ERASE_CHIP                    /*!< Whole bulk erase                              */
} BSP_QSPI_Erase_t;

typedef enum
{
  BSP_QSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,              */
  BSP_QSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access      */
  BSP_QSPI_ACCESS_MMP                /*!<  Instance use Memory Mapped Mode read   */
} BSP_QSPI_Access_t;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_QSPI_Exported_Variables STM32L462E_CELL1 QSPI Exported Variables
  * @{
  */

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
extern QSPI_HandleTypeDef hqspi;
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */
/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_QSPI_Exported_Functions STM32L462E_CELL1 QSPI Exported Functions
  * @{
  */

int32_t BSP_QSPI_Configuration(QSPI_HandleTypeDef *handleQSPI, uint32_t flashSize);
int32_t BSP_QSPI_Init(void);
int32_t BSP_QSPI_DeInit(void);
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_QSPI_RegisterDefaultMspCallbacks(void);
void BSP_QSPI_MspInit(QSPI_HandleTypeDef *qspiHandle);
void BSP_QSPI_MspDeInit(QSPI_HandleTypeDef *qspiHandle);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */
int32_t BSP_QSPI_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
int32_t BSP_QSPI_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
int32_t BSP_QSPI_EraseBlock(uint32_t BlockAddress, BSP_QSPI_Erase_t BlockSize);
int32_t BSP_QSPI_EraseChip(void);
int32_t BSP_QSPI_GetStatus(void);
int32_t BSP_QSPI_GetInfo(BSP_QSPI_Info_t *pInfo);
int32_t BSP_QSPI_ReadID(uint8_t *Id);
int32_t BSP_QSPI_EnableMemoryMappedMode(void);
int32_t BSP_QSPI_DisableMemoryMappedMode(void);

/**
  * @}
  */

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

#endif /* STM32L462E_CELL1_QSPI_H */
