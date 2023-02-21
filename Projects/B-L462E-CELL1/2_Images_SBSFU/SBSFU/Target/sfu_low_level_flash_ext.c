/**
  ******************************************************************************
  * @file    sfu_low_level_flash_ext.c
  * @author  MCD Application Team
  * @brief   SFU Flash Low Level Interface module
  *          This file provides set of firmware functions to manage SFU external
  *          flash low level interface.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "sfu_low_level_flash_ext.h"
#include "sfu_low_level_security.h"
#include "stm32l462e_cell1_qspi_in_module.h"

/* Private defines -----------------------------------------------------------*/
#define ERASE_BLOC_SIZE           0x10000U           /*!< 64 Kbytes */

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  This function initialize QSPI interface
  * @param  none
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_EXT_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	 __HAL_RCC_GPIOB_CLK_ENABLE();
	// Enable Modem GPIO pin
	GPIO_InitStruct.Pin = GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_Delay(10);


  if (BSP_QSPI_Init() != BSP_ERROR_NONE)
  {
    return SFU_ERROR;
  }
  else
  {
    return SFU_SUCCESS;
  }
}

/**
  * @brief  This function does an erase of n (depends on Length) pages in external OSPI flash
  * @param  pFlashStatus: SFU_FLASH Status pointer
  * @param  pStart: flash address to be erased
  * @param  Length: number of bytes
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_EXT_Erase_Size(SFU_FLASH_StatusTypeDef *pFlashStatus, uint8_t *pStart, uint32_t Length)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint32_t i;
  uint32_t address;

  /* flash address to erase is the offset from begin of external flash */
  address = (uint32_t) pStart - EXTERNAL_FLASH_ADDRESS;
  *pFlashStatus = SFU_FLASH_SUCCESS;

  /* Loop on 64KBytes block */
  for (i = 0 ; i < ((Length - 1U) / ERASE_BLOC_SIZE) + 1U; i++)
  {
    if (BSP_QSPI_EraseBlock(address, BSP_QSPI_ERASE_64K) != BSP_ERROR_NONE)
    {
      *pFlashStatus = SFU_FLASH_ERR_ERASE;
      e_ret_status = SFU_ERROR;
    }

    /* next 64KBytes block */
    address += ERASE_BLOC_SIZE;

    /* reload watch dog */
    SFU_LL_SECU_IWDG_Refresh();
  }
  return e_ret_status;
}

/**
  * @brief  This function writes a data buffer in external QSPI flash.
  * @param  pFlashStatus: FLASH_StatusTypeDef
  * @param  pDestination: flash address to write
  * @param  pSource: pointer on buffer with data to write
  * @param  Length: number of bytes
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_EXT_Write(SFU_FLASH_StatusTypeDef *pFlashStatus, uint8_t  *pDestination,
                                       const uint8_t *pSource, uint32_t Length)
{
  /* Do nothing if Length equal to 0 */
  if (Length == 0)
  {
    *pFlashStatus = SFU_FLASH_SUCCESS;
    return SFU_SUCCESS;
  }

  /* flash address to write is the offset from begin of external flash */
  if (BSP_QSPI_Write((uint8_t *) pSource, (uint32_t) pDestination - EXTERNAL_FLASH_ADDRESS, Length) != BSP_ERROR_NONE)
  {
    *pFlashStatus = SFU_FLASH_ERR_WRITING;
    return SFU_ERROR;
  }
  else
  {
    *pFlashStatus = SFU_FLASH_SUCCESS;
    return SFU_SUCCESS;
  }
}

/**
  * @brief  This function reads external QSPI flash
  * @param  pDestination: pointer on buffer to store data
  * @param  pSource: flash address to read
  * @param  Length: number of bytes
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_EXT_Read(uint8_t *pDestination, const uint8_t *pSource, uint32_t Length)
{
  /* ensure previous operation is finished (erase/write) : GetStatus()
     such verification is done (inside BSP driver) at the beginning of erase or write operation  but
     not for read operation ==> in order to maximise BSP driver execution timing efficiency */
  while (BSP_QSPI_GetStatus() !=  BSP_ERROR_NONE)
  {
  }

  /* flash address to read is the offset from begin of external flash */
  if (BSP_QSPI_Read((uint8_t *) pDestination, (uint32_t) pSource - EXTERNAL_FLASH_ADDRESS, Length) != BSP_ERROR_NONE)
  {
    return SFU_ERROR;
  }
  else
  {
    return SFU_SUCCESS;
  }
}

/**
  * @brief  This function compare a buffer with a flash area
  * @note   The flash area should not be located inside the secure area
  * @param  pFlash: address of the flash area
  * @param  Pattern1: first 32 bits pattern to be compared
  * @param  Pattern2: second 32 bits pattern to be compared
  * @param  Length: number of bytes to be compared
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_EXT_Compare(const uint8_t *pFlash, const uint32_t Pattern1, const uint32_t Pattern2, uint32_t Length)
{
  uint32_t a_flash_buffer[4U];
  uint32_t flash = (uint32_t) pFlash;
  uint32_t i;
  uint32_t j;

  /* ensure previous operation is finished (erase/write) : GetStatus()
     such verification is done (inside BSP driver) at the beginning of erase or write operation  but
     not for read operation ==> in order to maximise BSP driver execution timing efficiency */
  while (BSP_QSPI_GetStatus() !=  BSP_ERROR_NONE)
  {
  }

  /* by construction (AES_BLOCK_SIZE) firmware length is multiple of 16 bytes */
  if ((Length & 0x0000000FU) != 0U)
  {
    return SFU_ERROR;
  }

  /* flash address to read is the offset from begin of external flash */
  for ( i = 0U; i < Length; i += 16U)
  {
    if (BSP_QSPI_Read((uint8_t *)&a_flash_buffer[0], flash + i - EXTERNAL_FLASH_ADDRESS, 16U) == BSP_ERROR_NONE)
    {
      for (j = 0U; j < 4U; j++)
      {
        if ((a_flash_buffer[j] != Pattern1) && (a_flash_buffer[j] != Pattern2))
        {
          return SFU_ERROR;
        }
      }
    }
  }
  return SFU_SUCCESS;
}

/**
  * @brief  This function configure the flash in memory mapped mode to be able to execute code
  * @param  none
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LL_FLASH_EXT_Config_Exe(uint32_t SlotNumber)
{
  UNUSED(SlotNumber);
  /* no execution from external flash planned in this example
     this function is systematically called during boot execution then should return SFU_SUCCESS */
  return SFU_SUCCESS;
}

