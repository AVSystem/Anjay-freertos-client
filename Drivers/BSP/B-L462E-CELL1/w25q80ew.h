/**
  ******************************************************************************
  * @file    w25q80ew.h
  * @author  MCD Application Team
  * @brief   This file contains all the description of the W25QEW QSPI memory.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                       opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W25Q80EW_H
#define W25Q80EW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "w25q80ew_conf.h"
//#include "stm32l4xx_hal.h"
#include <stdint.h>

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup W25Q80EW
  * @{
  */

/** @defgroup W25Q80EW_Exported_Types W25Q80EW Exported Types
  * @{
  */
typedef struct
{
  uint32_t FlashSize;              /*!< Size of the flash */
  uint32_t EraseSectorSize;        /*!< Size of sectors for the erase operation */
  uint32_t EraseSectorsNumber;     /*!< Number of sectors for the erase operation */
  uint32_t EraseSubSectorSize;     /*!< Size of subsector for the erase operation     */
  uint32_t EraseSubSectorNumber;   /*!< Number of subsector for the erase operation   */
  uint32_t EraseSubSector1Size;    /*!< Size of subsector 1 for the erase operation   */
  uint32_t EraseSubSector1Number;  /*!< Number of subsector 1 for the erase operation */
  uint32_t ProgPageSize;           /*!< Size of pages for the program operation */
  uint32_t ProgPagesNumber;        /*!< Number of pages for the program operation */
} W25Q80EW_QSPI_Info_t;

/**
  * @}
  */

/** @defgroup W25Q80EW_Exported_Constants W25Q80EW Exported Constants
  * @{
  */

/**
  * @brief  W25Q80EW Error Codes
  */
#define W25Q80EW_QSPI_OK            ((uint8_t)0x00)
#define W25Q80EW_QSPI_ERROR         ((uint8_t)0x01)
#define W25Q80EW_QSPI_BUSY          ((uint8_t)0x02)
#define W25Q80EW_QSPI_NOT_SUPPORTED ((uint8_t)0x04)
#define W25Q80EW_QSPI_SUSPENDED     ((uint8_t)0x08)

/**
  * @brief  W25Q80EW QUAD mode status
  */
#define W25Q80EW_QSPI_QUAD_DISABLE    ((uint8_t)0x0)
#define W25Q80EW_QSPI_QUAD_ENABLE     ((uint8_t)0x1)

/**
  * @brief  W25Q80EW Configuration
  */
#define W25Q80EW_FLASH_SIZE           ((uint32_t)0x100000) /* 8 MBits => 1 MByte */
#define W25Q80EW_BLOCK_SIZE           ((uint32_t)0x10000)  /* 16 blocks of 64 KBytes */
#define W25Q80EW_SUBBLOCK_SIZE        ((uint32_t)0x8000)   /* 32 blocks of 32 KBytes */
#define W25Q80EW_SECTOR_SIZE          ((uint32_t)0x1000)   /* 256 sectors of 4 kBytes */
#define W25Q80EW_PAGE_SIZE            ((uint32_t)0x100)    /* 4096 pages of 256 bytes */

#define W25Q80EW_DUMMY_CYCLES_READ           ((uint8_t)8)
#define W25Q80EW_DUMMY_CYCLES_READ_QUAD      ((uint8_t)4)

#define W25Q80EW_CHIP_ERASE_MAX_TIME         10000
#define W25Q80EW_BLOCK_ERASE_MAX_TIME        1000
#define W25Q80EW_SUBBLOCK_ERASE_MAX_TIME     800
#define W25Q80EW_SECTOR_ERASE_MAX_TIME       400

/**
  * @brief  W25Q80EW Commands
  */
/* Reset Operations */
#define W25Q80EW_RESET_ENABLE_CMD                     0x66
#define W25Q80EW_RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define W25Q80EW_READ_ID_CMD                          0x90
#define W25Q80EW_READ_ID_DUAL_CMD                     0x92
#define W25Q80EW_READ_ID_QUAD_CMD                     0x94

#define W25Q80EW_READ_JEDEC_ID_CMD                    0x9F
#define W25Q80EW_READ_UNIQUE_ID_CMD                   0x4B

/* Read Operations */
#define W25Q80EW_READ_DATA_CMD                        0x03
#define W25Q80EW_FAST_READ_CMD                        0x0B

#define W25Q80EW_FAST_READ_DUAL_OUT_CMD               0x3B
#define W25Q80EW_FAST_READ_DUAL_INOUT_CMD             0xBB

#define W25Q80EW_FAST_READ_QUAD_OUT_CMD               0x6B
#define W25Q80EW_FAST_READ_QUAD_INOUT_CMD             0xEB

#define W25Q80EW_BURST_READ_WITH_WRAP_CMD             0x0C

/* Write Operations */
#define W25Q80EW_WRITE_ENABLE_CMD                     0x06
#define W25Q80EW_VOL_SR_WRITE_ENABLE_CMD              0x50
#define W25Q80EW_WRITE_DISABLE_CMD                    0x04

/* Register Operations */
#define W25Q80EW_READ_STATUS_REG_CMD                  0x05
#define W25Q80EW_READ_STATUS_REG_2_CMD                0x35
#define W25Q80EW_WRITE_STATUS_REG_CMD                 0x01
#define W25Q80EW_WRITE_STATUS_REG_2_CMD               0x31

#define W25Q80EW_READ_SFDP_REG_CMD                    0x5A

#define W25Q80EW_ERASE_SECURITY_REG_CMD               0x44
#define W25Q80EW_PROG_SECURITY_REG_CMD                0x42
#define W25Q80EW_READ_SECURITY_REG_CMD                0x48

/* Program Operations */
#define W25Q80EW_PAGE_PROG_CMD                        0x02
#define W25Q80EW_QUAD_PAGE_PROG_CMD                   0x32

/* Erase Operations */
#define W25Q80EW_SECTOR_ERASE_CMD                     0x20
#define W25Q80EW_BLOCK_ERASE_32K_CMD                  0x52
#define W25Q80EW_BLOCK_ERASE_64K_CMD                  0xD8
#define W25Q80EW_CHIP_ERASE_CMD                       0xC7
#define W25Q80EW_CHIP_ERASE_CMD_1                     0x60

#define W25Q80EW_ERASE_PROG_SUSPEND_CMD               0x75
#define W25Q80EW_ERASE_PROG_RESUME_CMD                0x7A

/* Quad Operations */
#define W25Q80EW_ENABLE_QPI_CMD                       0x38
#define W25Q80EW_DISABLE_QPI_CMD                      0xFF

/* Power-down operations */
#define W25Q80EW_POWER_DOWN_CMD                       0xB9
#define W25Q80EW_RELEASE_POWER_DOWN_CMD               0xAB

/* Other operations */
#define W25Q80EW_SET_BURST_WRAP_CMD                   0x77
#define W25Q80EW_SET_READ_PARAM_CMD                   0xC0

/**
  * @brief  W25Q80EW Registers
  */
/* Status Register 1 */
#define W25Q80EW_SR_BUSY                     ((uint8_t)0x01)    /*!< Erase/Write in progress */
#define W25Q80EW_SR_WEL                      ((uint8_t)0x02)    /*!< Write enable latch */
#define W25Q80EW_SR_BP                       ((uint8_t)0x1C)    /*!< Block protected bits */
#define W25Q80EW_SR_TB                       ((uint8_t)0x20)    /*!< Top/Bottom protect */
#define W25Q80EW_SR_SEC                      ((uint8_t)0x40)    /*!< Sector protect */
#define W25Q80EW_SR_SRP                      ((uint8_t)0x80)    /*!< Status register protect */

/* Status Register 2 */
#define W25Q80EW_SR2_SRL                     ((uint8_t)0x01)    /*!< Status Register Lock */
#define W25Q80EW_SR2_QE                      ((uint8_t)0x02)    /*!< Quad Enable */
#define W25Q80EW_SR2_LB                      ((uint8_t)0x3C)    /*!< Security Register Lock bits */
#define W25Q80EW_SR2_CMP                     ((uint8_t)0x40)    /*!< Complement protect */
#define W25Q80EW_SR2_SUS                     ((uint8_t)0x80)    /*!< Suspend Status */

/**
  * @}
  */

/** @defgroup W25Q80EW_Exported_Functions W25Q80EW Exported Functions
  * @{
  */

uint8_t W25Q80EW_Read(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
uint8_t W25Q80EW_Write(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
uint8_t W25Q80EW_Erase_Block64K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);
uint8_t W25Q80EW_Erase_Block32K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress);
uint8_t W25Q80EW_Erase_Sector(QSPI_HandleTypeDef *hqspi, uint32_t Sector);
uint8_t W25Q80EW_Erase_Chip(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_GetStatus(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *id);
uint8_t W25Q80EW_GetInfo(W25Q80EW_QSPI_Info_t *pInfo);
uint8_t W25Q80EW_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_SuspendErase(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_ResumeErase(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_EnterDeepPowerDown(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_LeaveDeepPowerDown(QSPI_HandleTypeDef *hqspi);
//uint8_t W25Q80EW_WriteEnable(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi);
uint8_t W25Q80EW_QuadMode(QSPI_HandleTypeDef *hqspi, uint8_t Operation);
uint8_t W25Q80EW_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout);
uint8_t W25Q80EW_ResetMemory(QSPI_HandleTypeDef *hqspi);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* W25Q80EW_H */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
