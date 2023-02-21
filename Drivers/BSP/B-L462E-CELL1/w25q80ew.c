/**
  ******************************************************************************
  * @file    w25q80ew.c
  * @author  MCD Application Team
  * @brief   This file includes a standard driver for the W25Q80EW QSPI
  *          memory mounted on Murata board.
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
  [..]
   (#) This driver is used to drive the W25Q80EW QSPI external
       memory mounted on Murata evaluation board.

   (#) This driver need a specific component driver (W25Q80EW) to be included with.

   (#) Initialization steps:
       (++) Initialize the QPSI external memory using the BSP_QSPI_Init() function. This
            function includes the MSP layer hardware resources initialization and the
            QSPI interface with the external memory. The BSP_QSPI_DeInit() can be used
            to deactivate the QSPI interface.

   (#) QSPI memory operations
       (++) QSPI memory can be accessed with read/write operations once it is
            initialized.
            Read/write operation can be performed with AHB access using the functions
            BSP_QSPI_Read()/BSP_QSPI_Write().
       (++) The function to the QSPI memory in memory-mapped mode is possible after
            the call of the function BSP_QSPI_EnableMemoryMappedMode().
       (++) The function BSP_QSPI_GetInfo() returns the configuration of the QSPI memory.
            (see the QSPI memory data sheet)
       (++) Perform erase block operation using the function BSP_QSPI_Erase_Block() and by
            specifying the block address. You can perform an erase operation of the whole
            chip by calling the function BSP_QSPI_Erase_Chip().
       (++) The function W25Q80EW_GetStatus() returns the current status of the QSPI memory.
            (see the QSPI memory data sheet)
       (++) Perform erase sector operation using the function BSP_QSPI_Erase_Sector()
            which is not blocking. So the function W25Q80EW_GetStatus() should be used
            to check if the memory is busy, and the functions BSP_QSPI_SuspendErase()/
            BSP_QSPI_ResumeErase() can be used to perform other operations during the
            sector erase.
       (++) Deep power down of the QSPI memory is managed with the call of the functions
            BSP_QSPI_EnterDeepPowerDown()/BSP_QSPI_LeaveDeepPowerDown()
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "w25q80ew.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup MURATA
  * @{
  */

/** @defgroup MURATA_TYPE_1_SE_QSPI MURATA TYPE 1 SE QSPI
  * @{
  */

/* Private variables ---------------------------------------------------------*/
/** @defgroup MURATA_TYPE_1_SE_QSPI_Private_Variables Private Variables
  * @{
  */
/**
  * @}
  */


/* Private functions ---------------------------------------------------------*/

/** @defgroup MURATA_TYPE_1_SE_QSPI_Private_Functions Private Functions
  * @{
  */
static uint8_t W25Q80EW_WriteEnable(QSPI_HandleTypeDef *hqspi);


/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/** @addtogroup MURATA_TYPE_1_SE_QSPI_Exported_Functions
  * @{
  */


/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  pData: Pointer to data to be read
  * @param  ReadAddr: Read start address
  * @param  Size: Size of data to read
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_Read(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the read command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_FAST_READ_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = ReadAddr;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = W25Q80EW_DUMMY_CYCLES_READ_QUAD;
  sCommand.NbData            = Size;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Reception of the data */
    if (HAL_QSPI_Receive(hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
  }

  return (ret);
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  pData: Pointer to data to be written
  * @param  WriteAddr: Write start address
  * @param  Size: Size of data to write
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_Write(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;
  uint32_t end_addr, current_size, current_addr;

  /* Calculation of the size between the write address and the end of the page */
  current_size = W25Q80EW_PAGE_SIZE - (WriteAddr % W25Q80EW_PAGE_SIZE);

  /* Check if the size of the data is less than the remaining place in the page */
  if (current_size > Size)
  {
    current_size = Size;
  }

  /* Initialize the adress variables */
  current_addr = WriteAddr;
  end_addr = WriteAddr + Size;

  /* Initialize the program command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_PAGE_PROG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Perform the write page by page */
  do
  {
    sCommand.Address = current_addr;
    sCommand.NbData  = current_size;

    /* Enable write operations */
    if (W25Q80EW_WriteEnable(hqspi) != W25Q80EW_QSPI_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Configure the command */
      if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
      else
      {
        /* Transmission of the data */
        if (HAL_QSPI_Transmit(hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
          ret = W25Q80EW_QSPI_ERROR;
        }
        else
        {
          /* Configure automatic polling mode to wait for end of program */
          if (W25Q80EW_AutoPollingMemReady(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != W25Q80EW_QSPI_OK)
          {
            ret = W25Q80EW_QSPI_ERROR;
          }
        }
      }
    }

    if (ret == W25Q80EW_QSPI_OK)
    {
      /* Update the address and size variables for next page programming */
      current_addr += current_size;
      pData += current_size;
      current_size = ((current_addr + W25Q80EW_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : W25Q80EW_PAGE_SIZE;
    }
  } while ((current_addr < end_addr) && (ret == W25Q80EW_QSPI_OK));

  return (ret);
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  * @param  BlockAddress: Block address to erase
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_Erase_Block64K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_BLOCK_ERASE_64K_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = BlockAddress;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Enable write operations */
  if (W25Q80EW_WriteEnable(hqspi) != W25Q80EW_QSPI_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Configure automatic polling mode to wait for end of erase */
      if (W25Q80EW_AutoPollingMemReady(hqspi, W25Q80EW_SUBBLOCK_ERASE_MAX_TIME) != W25Q80EW_QSPI_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
    }
  }

  return (ret);
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  * @param  BlockAddress: Block address to erase
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_Erase_Block32K(QSPI_HandleTypeDef *hqspi, uint32_t BlockAddress)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_BLOCK_ERASE_32K_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = BlockAddress;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Enable write operations */
  if (W25Q80EW_WriteEnable(hqspi) != W25Q80EW_QSPI_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Configure automatic polling mode to wait for end of erase */
      if (W25Q80EW_AutoPollingMemReady(hqspi, W25Q80EW_SUBBLOCK_ERASE_MAX_TIME) != W25Q80EW_QSPI_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
    }
  }

  return (ret);
}

/**
  * @brief  Erases the specified sector of the QSPI memory.
  * @param  Sector: Sector address to erase (0 to 255)
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_Erase_Sector(QSPI_HandleTypeDef *hqspi, uint32_t Sector)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  if (Sector >= (uint32_t)(W25Q80EW_FLASH_SIZE / W25Q80EW_SECTOR_SIZE))
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Initialize the erase command */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    sCommand.Instruction       = W25Q80EW_SECTOR_ERASE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
    sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
    sCommand.Address           = (Sector * W25Q80EW_SECTOR_SIZE);
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    if (W25Q80EW_WriteEnable(hqspi) != W25Q80EW_QSPI_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Send the command */
      if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
      else
      {
        /* Configure automatic polling mode to wait for end of erase */
        if (W25Q80EW_AutoPollingMemReady(hqspi, W25Q80EW_SECTOR_ERASE_MAX_TIME) != W25Q80EW_QSPI_OK)
        {
          ret = W25Q80EW_QSPI_ERROR;
        }
      }
    }
  }

  return (ret);
}

/**
  * @brief  Erases the entire QSPI memory.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_Erase_Chip(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the erase command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_CHIP_ERASE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Enable write operations */
  if (W25Q80EW_WriteEnable(hqspi) != W25Q80EW_QSPI_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Configure automatic polling mode to wait for end of erase */
      if (W25Q80EW_AutoPollingMemReady(hqspi, W25Q80EW_CHIP_ERASE_MAX_TIME) != W25Q80EW_QSPI_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
    }
  }

  return (ret);
}

/**
  * @brief  Reads current status of the QSPI memory.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_GetStatus(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;
  uint8_t reg;

  /* Initialize the read status 2 register command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_READ_STATUS_REG_2_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 1;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Reception of the data */
    if (HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Check the value of the register */
      if ((reg & W25Q80EW_SR2_SUS) != 0U)
      {
        ret = W25Q80EW_QSPI_SUSPENDED;
      }
    }
  }

  if (ret == W25Q80EW_QSPI_OK)
  {
    /* Initialize the read status register command */
    sCommand.Instruction = W25Q80EW_READ_STATUS_REG_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Reception of the data */
      if (HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
      else
      {
        /* Check the value of the register */
        if ((reg & W25Q80EW_SR_BUSY) != 0U)
        {
          ret = W25Q80EW_QSPI_BUSY;
        }
      }
    }
  }

  return (ret);
}

/**
  * @brief  ead Flash 3 Byte IDs.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_ReadID(QSPI_HandleTypeDef *hqspi, uint8_t *id)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the read status 2 register command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_READ_JEDEC_ID_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 3;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Reception of the data */
    if (HAL_QSPI_Receive(hqspi, id, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
  }

  return (ret);
}

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  pInfo: pointer on the configuration structure
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_GetInfo(W25Q80EW_QSPI_Info_t *pInfo)
{
  /* Configure the structure with the memory configuration */
  pInfo->FlashSize          = W25Q80EW_FLASH_SIZE;
  pInfo->EraseSectorSize    = W25Q80EW_SUBBLOCK_SIZE;
  pInfo->EraseSectorsNumber = (W25Q80EW_FLASH_SIZE / W25Q80EW_SUBBLOCK_SIZE);
  /* W25Q80EW_SUBBLOCK_SIZE = 32K but chip supports : erase block 64k / erase block 32k / erase sector 4k */
  /*
   * pInfo->EraseSectorSize       = W25Q80EW_BLOCK_SIZE;
   * pInfo->EraseSectorsNumber    = (W25Q80EW_FLASH_SIZE / W25Q80EW_BLOCK_SIZE);
   * pInfo->EraseSubSectorSize    = W25Q80EW_SUBBLOCK_SIZE;
   * pInfo->EraseSubSectorNumber  = (W25Q80EW_FLASH_SIZE / W25Q80EW_SUBBLOCK_SIZE);
   * pInfo->EraseSubSector1Size   = W25Q80EW_SECTOR_SIZE;
   * pInfo->EraseSubSector1Number = (W25Q80EW_FLASH_SIZE / W25Q80EW_SECTOR_SIZE);
   */
  pInfo->ProgPageSize       = W25Q80EW_PAGE_SIZE;
  pInfo->ProgPagesNumber    = (W25Q80EW_FLASH_SIZE / W25Q80EW_PAGE_SIZE);

  return W25Q80EW_QSPI_OK;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_EnableMemoryMappedMode(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef      sCommand;
  QSPI_MemoryMappedTypeDef sMemMappedCfg;

  /* Configure the command for the read instruction */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_FAST_READ_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_4_LINES;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = W25Q80EW_DUMMY_CYCLES_READ_QUAD;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the memory mapped mode */
  sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

  if (HAL_QSPI_MemoryMapped(hqspi, &sCommand, &sMemMappedCfg) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }

  return (ret);
}

/**
  * @brief  This function suspends an ongoing erase command.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_SuspendErase(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Check whether the device is busy (erase operation is
  in progress).
  */
  if (W25Q80EW_GetStatus(hqspi) == W25Q80EW_QSPI_BUSY)
  {
    /* Initialize the erase command */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    sCommand.Instruction       = W25Q80EW_ERASE_PROG_SUSPEND_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }

    if (W25Q80EW_GetStatus(hqspi) != W25Q80EW_QSPI_SUSPENDED)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
  }

  return (ret);
}

/**
  * @brief  This function resumes a paused erase command.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_ResumeErase(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Check whether the device is in suspended state */
  if (W25Q80EW_GetStatus(hqspi) == W25Q80EW_QSPI_SUSPENDED)
  {
    /* Initialize the erase command */
    sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    sCommand.Instruction       = W25Q80EW_ERASE_PROG_RESUME_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* When this command is executed, the status register busy bit is set to 1. This command is ignored
       * if the device is not in a suspended state.
      */
      if (W25Q80EW_GetStatus(hqspi) != W25Q80EW_QSPI_BUSY)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
    }
  }

  return (ret);
}

/**
  * @brief  This function enter the QSPI memory in deep power down mode.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_EnterDeepPowerDown(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the power down command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_POWER_DOWN_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }

  /* --- Memory takes 3us max to power down --- */

  return (ret);
}

/**
  * @brief  This function leave the QSPI memory from deep power down mode.
  * @retval QSPI memory status
  */
uint8_t W25Q80EW_LeaveDeepPowerDown(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Initialize the release power down command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_RELEASE_POWER_DOWN_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }

  /* --- Memory takes 3us max to leave power down --- */

  return (ret);
}

/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hqspi: QSPI handle
  * @retval None
  */
uint8_t W25Q80EW_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Enable write operations */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_WRITE_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Configure automatic polling mode to wait for write enabling */
    sConfig.Match           = W25Q80EW_SR_WEL;
    sConfig.Mask            = W25Q80EW_SR_WEL;
    sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
    sConfig.StatusBytesSize = 1;
    sConfig.Interval        = 0x10;
    sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    sCommand.Instruction    = W25Q80EW_READ_STATUS_REG_CMD;
    sCommand.DataMode       = QSPI_DATA_4_LINES;

    if (HAL_QSPI_AutoPolling(hqspi, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
  }

  return (ret);
}

/**
  * @brief  This function configure the dummy cycles on memory side.
  * @param  hqspi: QSPI handle
  * @retval None
  */
uint8_t W25Q80EW_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;
  uint8_t reg;

  /* Initialize the read volatile configuration register command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_SET_READ_PARAM_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 1;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Calculation of the value to set according the dummy cycles required */
    reg = (((W25Q80EW_DUMMY_CYCLES_READ_QUAD / 2U) - 1U) << 4);

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
  }

  return (ret);
}

/**
  * @brief  This function enables/disables the Quad mode of the memory.
  * @param  hqspi     : QSPI handle
  * @param  Operation : W25Q80EW_QSPI_QUAD_ENABLE or W25Q80EW_QSPI_QUAD_DISABLE mode
  * @retval None
  */
uint8_t W25Q80EW_QuadMode(QSPI_HandleTypeDef *hqspi, uint8_t Operation)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;

  /* Read status 2 register */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = W25Q80EW_READ_STATUS_REG_2_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.NbData            = 1;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    uint8_t reg;

    if (HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Enable volatile status register write operations */
      sCommand.Instruction = W25Q80EW_VOL_SR_WRITE_ENABLE_CMD;
      sCommand.DataMode    = QSPI_DATA_NONE;
      sCommand.NbData      = 0;

      if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
      else
      {
        /* Activate/deactivate the Quad mode */
        if (Operation == W25Q80EW_QSPI_QUAD_ENABLE)
        {
          SET_BIT(reg, W25Q80EW_SR2_QE);
        }
        else
        {
          CLEAR_BIT(reg, W25Q80EW_SR2_QE);
        }

        sCommand.Instruction = W25Q80EW_WRITE_STATUS_REG_2_CMD;
        sCommand.DataMode    = QSPI_DATA_1_LINE;
        sCommand.NbData      = 1;

        if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
          ret = W25Q80EW_QSPI_ERROR;
        }
        else
        {
          if (HAL_QSPI_Transmit(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
          {
            ret = W25Q80EW_QSPI_ERROR;
          }
        }
      }
    }

    if (ret == W25Q80EW_QSPI_OK)
    {
      /* Check the configuration has been correctly done */
      sCommand.Instruction = W25Q80EW_READ_STATUS_REG_2_CMD;

      if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
      else
      {
        if (HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        {
          ret = W25Q80EW_QSPI_ERROR;
        }
        else
        {
          if ((((reg & W25Q80EW_SR2_QE) == 0U) && (Operation == W25Q80EW_QSPI_QUAD_ENABLE)) ||
              (((reg & W25Q80EW_SR2_QE) != 0U) && (Operation == W25Q80EW_QSPI_QUAD_DISABLE)))
          {
            ret = W25Q80EW_QSPI_ERROR;
          }
        }
      }
    }

    if (ret == W25Q80EW_QSPI_OK)
    {
      /* Send the enable QPI command */
      sCommand.Instruction = W25Q80EW_ENABLE_QPI_CMD;
      sCommand.DataMode    = QSPI_DATA_NONE;
      sCommand.NbData      = 0;
      if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
    }
  }

  return (ret);
}

/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hqspi: QSPI handle
  * @param  Timeout: Timeout for auto-polling
  * @retval None
  */
uint8_t W25Q80EW_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Configure automatic polling mode to wait for memory ready */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_READ_STATUS_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_4_LINES;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  sConfig.Match           = 0;
  sConfig.Mask            = W25Q80EW_SR_BUSY;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling(hqspi, &sCommand, &sConfig, Timeout) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }

  return (ret);
}

/**
  * @brief  This function reset the QSPI memory.
  * @param  hqspi: QSPI handle
  * @retval None
  */
uint8_t W25Q80EW_ResetMemory(QSPI_HandleTypeDef *hqspi)
{
  uint8_t ret = W25Q80EW_QSPI_OK;
  QSPI_CommandTypeDef sCommand;
  uint8_t reg[3];

  /* --- Reset performed in QPI mode --- */
  /* Initialize the reset enable command */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_4_LINES;
  sCommand.Instruction       = W25Q80EW_RESET_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    ret = W25Q80EW_QSPI_ERROR;
  }
  else
  {
    /* Send the reset memory command */
    sCommand.Instruction = W25Q80EW_RESET_MEMORY_CMD;
    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }

    HAL_Delay(1);
  }

  if (ret == W25Q80EW_QSPI_OK)
  {
    /* --- Reset performed in SPI mode --- */
    /* Send the reset enable command */
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction     = W25Q80EW_RESET_ENABLE_CMD;

    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      /* Send the reset memory command */
      sCommand.Instruction = W25Q80EW_RESET_MEMORY_CMD;
      if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }

      HAL_Delay(1);
    }
  }

  if (ret == W25Q80EW_QSPI_OK)
  {
    /* --- Check that the memory is accessible by reading ID --- */
    sCommand.Instruction = W25Q80EW_READ_JEDEC_ID_CMD;
    sCommand.DataMode    = QSPI_DATA_1_LINE;
    sCommand.NbData      = 3;

    if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
      ret = W25Q80EW_QSPI_ERROR;
    }
    else
    {
      if (HAL_QSPI_Receive(hqspi, reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
      {
        ret = W25Q80EW_QSPI_ERROR;
      }
      else
      {
        if ((reg[0] != 0xEFU) || (reg[1] != 0x60U) || (reg[2] != 0x14U))
        {
          ret = W25Q80EW_QSPI_ERROR;
        }
      }
    }
  }

  return (ret);
}

/**
  * @}
  */

/** @addtogroup MURATA_QSPI_Private_Functions
  * @{
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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

