/**
  ******************************************************************************
  * @file    stm32l462e_cell1_qspi_in_module.c
  * @author  MCD Application Team
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

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_qspi_in_module.h"
#include "stm32l462e_cell1_qspi.h"

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 0)
#include "quadspi.h"
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 0) */

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_QSPI_INMODULE STM32L462E_CELL1 QSPI_INMODULE
  * @{
  */

/** @defgroup STM32L462E_CELL1_QSPI_INMODULE_Private_Functions_Prototypes
  *           STM32L462E_CELL1 QSPI_INMODULE Private Functions Prototypes
  * @{
  */
static int32_t QSPI_CheckID(void);
static int32_t QSPI_ResetMemory(QSPI_HandleTypeDef *handleQSPI);
static int32_t QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *handleQSPI);
static int32_t QSPI_QuadMode(QSPI_HandleTypeDef *handleQSPI, uint8_t Operation);
static int32_t QSPI_Erase_Block64K(uint32_t BlockAddress);
static int32_t QSPI_Erase_Block32K(uint32_t BlockAddress);
static int32_t QSPI_Erase_Sector(uint32_t SectorAddress);

/**
  * @}
  */


/** @defgroup STM32L462E_CELL1_QSPI_INMODULE_Exported_Functions
  *           STM32L462E_CELL1 QSPI_INMODULE Exported Functions
  * @{
  */

/**
  * @brief  Initializes the QSPI interface.
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_Init(void)
{
  int32_t ret = BSP_ERROR_NONE;

  hqspi.Instance = QUADSPI;

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
  /* Register the QSPI MSP Callbacks */
  if (BSP_QSPI_RegisterDefaultMspCallbacks() != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
#endif /* USE_HAL_QSPI_REGISTER_CALLBACKS */

    /* Call the DeInit function to reset the driver */
    if (HAL_QSPI_DeInit(&hqspi) != HAL_OK)
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
    else
    {
      /* System level initialization */
      uint32_t flashSize = (uint32_t)(POSITION_VAL(W25Q80EW_FLASH_SIZE) - 1U);
      if (BSP_QSPI_Configuration(&hqspi, flashSize) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        /* QSPI memory reset */
        if (QSPI_ResetMemory(&hqspi) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        /* QSPI quad enable */
        if (QSPI_QuadMode(&hqspi, W25Q80EW_QSPI_QUAD_ENABLE) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        /* Configuration of the dummy cucles on QSPI memory side */
        if (QSPI_DummyCyclesCfg(&hqspi) != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        /* Check QSPI ID */
        if (QSPI_CheckID() != BSP_ERROR_NONE)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
      }
    }
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
  }
#endif /* USE_HAL_QSPI_REGISTER_CALLBACKS */

  return (ret);
}

/**
  * @brief  De-Initializes the QSPI interface.
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_DeInit(void)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  hqspi.Instance = QUADSPI;

  /* Call the DeInit function to reset the driver */
  if (HAL_QSPI_DeInit(&hqspi) == HAL_OK)
  {
    ret  = BSP_ERROR_NONE;
  }

  return (ret);
}

/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  pData: Pointer to data to be read
  * @param  ReadAddr: Read start address
  * @param  Size: Size of data to read
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_Read(&hqspi, pData, ReadAddr, Size) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  pData: Pointer to data to be written
  * @param  WriteAddr: Write start address
  * @param  Size: Size of data to write
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_Write(&hqspi, pData, WriteAddr, Size) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  *         W25Q80EW supports 4K, 32K, 64K size block erase commands.
  * @param  BlockAddress Block address to erase
  * @param  BlockSize    Erase Block size
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseBlock(uint32_t BlockAddress, BSP_QSPI_Erase_t BlockSize)
{
  int32_t ret;

  if (BlockSize == BSP_QSPI_ERASE_64K)
  {
    ret = QSPI_Erase_Block64K(BlockAddress);
  }
  else if (BlockSize == BSP_QSPI_ERASE_32K)
  {
    ret = QSPI_Erase_Block32K(BlockAddress);
  }
  else if (BlockSize == BSP_QSPI_ERASE_4K)
  {
    /* for 4k block, convert BlockAddress to a Sector number*/
    ret = QSPI_Erase_Sector(BlockAddress / W25Q80EW_SECTOR_SIZE);
  }
  else
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }

  return ret;
}

/**
  * @brief  Erases the entire QSPI memory.
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_EraseChip(void)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_Erase_Chip(&hqspi) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Reads current status of the QSPI memory.
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_GetStatus(void)
{
  int32_t ret;

  uint8_t w25q80ew_status = W25Q80EW_GetStatus(&hqspi);
  if (w25q80ew_status == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }
  else if (w25q80ew_status == W25Q80EW_QSPI_SUSPENDED)
  {
    ret = BSP_ERROR_QSPI_SUSPENDED;
  }
  else if (w25q80ew_status == W25Q80EW_QSPI_BUSY)
  {
    ret = BSP_ERROR_QSPI_BUSY;
  }
  else
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }

  return ret;
}

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  pInfo  pointer on the configuration structure
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_GetInfo(BSP_QSPI_Info_t *pInfo)
{
  int32_t ret;

  W25Q80EW_QSPI_Info_t tmpInfo;
  (void) W25Q80EW_GetInfo(&tmpInfo);
  /* recopy requested QSPI information to client structure */
  pInfo->FlashSize          = tmpInfo.FlashSize;
  pInfo->EraseSectorSize    = tmpInfo.EraseSectorSize;
  pInfo->EraseSectorsNumber = tmpInfo.EraseSectorsNumber;
  pInfo->ProgPageSize       = tmpInfo.ProgPageSize;
  pInfo->ProgPagesNumber    = tmpInfo.ProgPagesNumber;
  ret = BSP_ERROR_NONE;

  return ret;
}

/**
  * @brief  Get Flash 3 Byte IDs
  * @param  Id Pointer to flash ID bytes
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadID(uint8_t *Id)
{
  int32_t ret;

  if (W25Q80EW_ReadID(&hqspi, Id) != W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_EnableMemoryMappedMode(void)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_EnableMemoryMappedMode(&hqspi) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Exit from memory-mapped mode
  * @retval QSPI memory status
  */
int32_t BSP_QSPI_DisableMemoryMappedMode(void)
{
  int32_t ret;

  if (HAL_QSPI_Abort(&hqspi) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if (BSP_QSPI_DeInit() != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else if (BSP_QSPI_Init() != BSP_ERROR_NONE)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_QSPI_INMODULE_Private_Functions
  *           STM32L462E_CELL1 QSPI_INMODULE Private Functions
  * @{
  */

/**
  * @brief  Check Flash 3 Byte IDs
  * @note   Check that QSPI is accessed and is the expected one.
  * @retval QSPI memory status
  */
static int32_t QSPI_CheckID(void)
{
  int32_t ret;
  uint8_t id[3] = { 0U };

  if (W25Q80EW_ReadID(&hqspi, id) != W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    /* Read component ID composed of 3 bytes:
     * byte 0 = EFh (Winbond, manufcturer JEDED ID )
     * byte 1 = 60h (W25Q80EW device identifiation: memory type)
     * byte 2 = 14h (W25Q80EW device identifiation: capacity)
     */
    if ((id[0] == 0xEFU) && (id[1] == 0x60U) && (id[2] == 0x14U))
    {
      ret = BSP_ERROR_NONE;
    }
    else
    {
      ret = BSP_ERROR_COMPONENT_FAILURE;
    }
  }

  return ret;
}

/**
  * @brief  This function reset the QSPI memory.
  * @note   Because of the small package and the limitation on the number of pins, the W25Q80EW provide a
  *         software Reset instruction instead of a dedicated RESET pin
  * @param  handleQSPI: QSPI handle
  * @retval None
  */
static int32_t QSPI_ResetMemory(QSPI_HandleTypeDef *handleQSPI)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_ResetMemory(handleQSPI) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  This function configure the dummy cycles on memory side.
  * @param  handleQSPI: QSPI handle
  * @retval None
  */
static int32_t QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *handleQSPI)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_DummyCyclesCfg(handleQSPI) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  This function enables/disables the Quad mode of the memory.
  * @param  handleQSPI     : QSPI handle
  * @param  Operation : W25Q80EW_QSPI_QUAD_ENABLE or W25Q80EW_QSPI_QUAD_DISABLE mode
  * @retval None
  */
static int32_t QSPI_QuadMode(QSPI_HandleTypeDef *handleQSPI, uint8_t Operation)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_QuadMode(handleQSPI, Operation) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Erases the specified 64K block of the QSPI memory.
  * @param  BlockAddress: Block address to erase
  * @retval QSPI memory status
  */
static int32_t QSPI_Erase_Block64K(uint32_t BlockAddress)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_Erase_Block64K(&hqspi, BlockAddress) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Erases the specified 32K block of the QSPI memory.
  * @param  BlockAddress: Block address to erase
  * @retval QSPI memory status
  */
static int32_t QSPI_Erase_Block32K(uint32_t BlockAddress)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_Erase_Block32K(&hqspi, BlockAddress) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @brief  Erases the specified sector of the QSPI memory.
  * @param  Sector: Sector address to erase (0 to 255)
  * @retval QSPI memory status
  * @note This function is non blocking meaning that sector erase
  *       operation is started but not completed when the function
  *       returns. Application has to call BSP_QSPI_GetStatus()
  *       to know when the device is available again (i.e. erase operation
  *       completed).
  */
static int32_t QSPI_Erase_Sector(uint32_t SectorAddress)
{
  int32_t ret = BSP_ERROR_COMPONENT_FAILURE;

  if (W25Q80EW_Erase_Sector(&hqspi, SectorAddress) == W25Q80EW_QSPI_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

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

