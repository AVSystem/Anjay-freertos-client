/**
  ******************************************************************************
  * @file    flash_if.c
  * @author  MCD Application Team
  * @brief   FLASH Interface module.
  *          This file provides set of firmware functions to manage Flash
  *          Interface functionalities.
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

#ifdef USE_FW_UPDATE
/** @addtogroup USER_APP User App Example
  * @{
  */

/** @addtogroup USER_APP_COMMON Common
  * @{
  */
/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "string.h"
#include <stdio.h>

/* Uncomment the line below if you want some debug logs */
#define FLASH_IF_DBG
#ifdef FLASH_IF_DBG
#define FLASH_IF_TRACE printf
#else
#define FLASH_IF_TRACE(...)
#endif /* FLASH_IF_DBG */


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define NB_PAGE_SECTOR_PER_ERASE  2U    /*!< Nb page erased per erase */
#define EXTERNAL_FLASH_ADDRESS    0x90000000U


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
static uint32_t GetPage(uint32_t uAddr);
static uint32_t GetBank(uint32_t uAddr);
static uint32_t GetBankAddr(uint32_t bank);
static HAL_StatusTypeDef FLASH_INT_If_Clear_Error(void);

/* Public functions : wrapper ---------------------------------------------------------*/

/**
  * @brief  Initialize internal and external flash interface (OSPI/QSPI)
  * @param  none
  * @retval HAL status.
  */
HAL_StatusTypeDef FLASH_If_Init(void)
{
  HAL_StatusTypeDef e_ret_status = HAL_OK;

  e_ret_status = FLASH_INT_Init();
  if (e_ret_status == HAL_OK)
  {
    e_ret_status = FLASH_EXT_Init();
  }
  return e_ret_status;
}
/**
  * @brief  Depending on start address, this function will call internal or external (OSPI/QSPI) flash driver
  * @param  pStart: flash address to be erased
  * @param  uLength: number of bytes
  * @retval HAL status.
  */
HAL_StatusTypeDef FLASH_If_Erase_Size(void *pStart, uint32_t uLength)
{
  /* Check Flash start address */
  if ((uint32_t) pStart < EXTERNAL_FLASH_ADDRESS)
  {
    return FLASH_INT_If_Erase_Size(pStart, uLength);
  }
  else
  {
    return FLASH_EXT_If_Erase_Size(pStart, uLength);
  }
}

/**
  * @brief  Depending on destination address, this function will call internal or external (OSPI/QSPI) flash driver
  * @param  pDestination: flash address to write
  * @param  pSource: pointer on buffer with data to write
  * @param  uLength: number of bytes
  * @retval HAL Status.
  */
HAL_StatusTypeDef FLASH_If_Write(void *pDestination, const void *pSource, uint32_t uLength)
{
  /* Check Flash destination address */
  if ((uint32_t) pDestination < EXTERNAL_FLASH_ADDRESS)
  {
    return FLASH_INT_If_Write(pDestination, pSource, uLength);
  }
  else
  {
    return FLASH_EXT_If_Write(pDestination, pSource, uLength);
  }
}

/**
  * @brief  Depending on destination address, this function will call internal or external (OSPI/QSPI) flash driver
  * @brief  Depending on source address, this function will call internal or external (OSPI/QSPI) flash driver
  * @param  pDestination: pointer on buffer to store data
  * @param  pSource: flash address to read
  * @param  uLength: number of bytes
  * @retval HAL Status.
  */
HAL_StatusTypeDef FLASH_If_Read(void *pDestination, const void *pSource, uint32_t uLength)
{
  /* Check Flash source address */
  if ((uint32_t) pSource < EXTERNAL_FLASH_ADDRESS)
  {
    return FLASH_INT_If_Read(pDestination, pSource, uLength);
  }
  else
  {
    return FLASH_EXT_If_Read(pDestination, pSource, uLength);
  }
}

/* Public functions : internal flash --------------------------------------------------------- */
/**
  * @brief  This function initialize the internal flash interface if required
  * @param  none
  * @retval HAL status.
  */
HAL_StatusTypeDef FLASH_INT_Init(void)
{
  return HAL_OK;
}

/**
  * @brief  This function does an erase of n (depends on Length) pages in user flash area
  * @param  pStart: Start of user flash area
  * @param  uLength: number of bytes.
  * @retval HAL status.
  */
HAL_StatusTypeDef FLASH_INT_If_Erase_Size(void *pStart, uint32_t uLength)
{
  uint32_t page_error = 0U;
  uint32_t uStart = (uint32_t)pStart;
  FLASH_EraseInitTypeDef x_erase_init;
  HAL_StatusTypeDef e_ret_status = HAL_ERROR;
  uint32_t first_page = 0U;
  uint32_t nb_pages = 0U;
  uint32_t chunk_nb_pages;
  uint32_t erase_command = 0U;
  uint32_t bank_number = 0U;

  /* Clear error flags raised during previous operation */
  e_ret_status = FLASH_INT_If_Clear_Error();

  if (e_ret_status == HAL_OK)
  {
    /* Unlock the Flash to enable the flash control register access *************/
    if (HAL_FLASH_Unlock() == HAL_OK)
    {
      do
      {
        /* Get the 1st page to erase */
        first_page = GetPage(uStart);
        bank_number = GetBank(uStart);
        if (GetBank(uStart + uLength - 1U) == bank_number)
        {
          /* Get the number of pages to erase from 1st page */
          nb_pages = GetPage(uStart + uLength - 1U) - first_page + 1U;

          /* Fill EraseInit structure*/
          x_erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
          x_erase_init.Banks = bank_number;

          /* Erase flash per NB_PAGE_SECTOR_PER_ERASE to avoid watch-dog */
          do
          {
            chunk_nb_pages = (nb_pages >= NB_PAGE_SECTOR_PER_ERASE) ? NB_PAGE_SECTOR_PER_ERASE : nb_pages;
            x_erase_init.Page = first_page;
            x_erase_init.NbPages = chunk_nb_pages;
            first_page += chunk_nb_pages;
            nb_pages -= chunk_nb_pages;
            if (HAL_FLASHEx_Erase(&x_erase_init, &page_error) != HAL_OK)
            {
              HAL_FLASH_GetError();
              e_ret_status = HAL_ERROR;
            }
            /* Refresh Watchdog */
            WRITE_REG(IWDG->KR, IWDG_KEY_RELOAD);
          } while (nb_pages > 0);
          erase_command = 1U;
        }
        else
        {
          uint32_t startbank2 = GetBankAddr(FLASH_BANK_2);
          nb_pages = GetPage(startbank2 - 1U) - first_page + 1U;
          x_erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
          x_erase_init.Banks       = bank_number;
          uLength = uLength  - (startbank2 - uStart);
          uStart = startbank2;

          /* Erase flash per NB_PAGE_SECTOR_PER_ERASE to avoid watch-dog */
          do
          {
            chunk_nb_pages = (nb_pages >= NB_PAGE_SECTOR_PER_ERASE) ? NB_PAGE_SECTOR_PER_ERASE : nb_pages;
            x_erase_init.Page = first_page;
            x_erase_init.NbPages = chunk_nb_pages;
            first_page += chunk_nb_pages;
            nb_pages -= chunk_nb_pages;
            if (HAL_FLASHEx_Erase(&x_erase_init, &page_error) != HAL_OK)
            {
              HAL_FLASH_GetError();
              e_ret_status = HAL_ERROR;
            }
            /* Refresh Watchdog */
            WRITE_REG(IWDG->KR, IWDG_KEY_RELOAD);
          } while (nb_pages > 0);
        }
      } while (erase_command == 0);
      /* Lock the Flash to disable the flash control register access (recommended
      to protect the FLASH memory against possible unwanted operation) *********/
      HAL_FLASH_Lock();

    }
    else
    {
      e_ret_status = HAL_ERROR;
    }
  }

  return e_ret_status;
}

/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  pDestination: Start address for target location. It has to be 8 bytes aligned.
  * @param  pSource: pointer on buffer with data to write
  * @param  uLength: Length of data buffer in bytes. It has to be 8 bytes aligned.
  * @retval HAL Status.
  */
HAL_StatusTypeDef FLASH_INT_If_Write(void *pDestination, const void *pSource, uint32_t uLength)
{
  HAL_StatusTypeDef e_ret_status = HAL_ERROR;
  uint32_t pdata = (uint32_t)pSource;

  /* Clear error flags raised during previous operation */
  e_ret_status = FLASH_INT_If_Clear_Error();

  if (e_ret_status == HAL_OK)
  {
    /* Unlock the Flash to enable the flash control register access *************/
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
      return HAL_ERROR;
    }
    else
    {
      /* DataLength must be a multiple of 64 bit */
      for (uint32_t i = 0U; i < uLength; i += 8U)
      {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
        be done by word */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)pDestination,  *((uint64_t *)(pdata + i)))
            == HAL_OK)
        {
          /* Check the written value */
          if (*(uint64_t *)pDestination != *(uint64_t *)(pdata + i))
          {
            /* Flash content doesn't match SRAM content */
            e_ret_status = HAL_ERROR;
            break;
          }
          /* Increment FLASH Destination address */
          pDestination = (void *)((uint32_t)pDestination + 8U);
        }
        else
        {
          /* Error occurred while writing data in Flash memory */
          e_ret_status = HAL_ERROR;
          break;
        }
      }
      /* Lock the Flash to disable the flash control register access (recommended
      to protect the FLASH memory against possible unwanted operation) *********/
      HAL_FLASH_Lock();
    }
  }
  return e_ret_status;
}

/**
  * @brief  This function reads flash
  * @param  pDestination: Start address for target location
  * @param  pSource: flash address to read
  * @param  uLength: number of bytes
  * @retval HAL Status.
  */
HAL_StatusTypeDef FLASH_INT_If_Read(void *pDestination, const void *pSource, uint32_t uLength)
{
  memcpy(pDestination, pSource, uLength);
  return HAL_OK;
}

/* Public functions : external flash ---------------------------------------------------------*/

HAL_StatusTypeDef FLASH_EXT_Init(void)
{
  return HAL_OK;
}

/* No external flash available on this product
   ==> return SFU_ERROR */

HAL_StatusTypeDef FLASH_EXT_If_Erase_Size(void *pStart, uint32_t uLength)
{
  return HAL_ERROR;
}

HAL_StatusTypeDef FLASH_EXT_If_Write(void  *pDestination, const void *pSource, uint32_t uLength)
{
  return HAL_ERROR;
}

HAL_StatusTypeDef FLASH_EXT_If_Read(void *pDestination, const void *pSource, uint32_t uLength)
{
  return HAL_ERROR;
}

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Clear error flags raised during previous operation
  * @param  None
  * @retval HAL Status.
  */
HAL_StatusTypeDef FLASH_INT_If_Clear_Error(void)
{
  HAL_StatusTypeDef ret = HAL_ERROR;

  /* Unlock the Program memory */
  if (HAL_FLASH_Unlock() == HAL_OK)
  {

    /* Clear all FLASH flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    /* Unlock the Program memory */
    if (HAL_FLASH_Lock() == HAL_OK)
    {
      ret = HAL_OK;
    }
#ifdef FLASH_IF_DBG
    else
    {
      FLASH_IF_TRACE("[FLASH_IF] Lock failure\r\n");
    }
#endif /* FLASH_IF_DBG */
  }
#ifdef FLASH_IF_DBG
  else
  {
    FLASH_IF_TRACE("[FLASH_IF] Unlock failure\r\n");
  }
#endif /* FLASH_IF_DBG */
  return ret;
}

/**
  * @brief  Gets the page of a given address
  * @param  uAddr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t uAddr)
{
  uint32_t page = 0U;

  if (uAddr < (FLASH_BASE + (FLASH_BANK_SIZE)))
  {
    /* Bank 1 */
    page = (uAddr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (uAddr - (FLASH_BASE + (FLASH_BANK_SIZE))) / FLASH_PAGE_SIZE;
  }

  return page;
}
/**
  * @brief  Gets the bank of a given address
  * @param  uAddr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t uAddr)
{
  uint32_t bank = 0U;

  if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0U)
  {
    /* No Bank swap */
    if (uAddr < (FLASH_BASE + (FLASH_BANK_SIZE)))
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
    /* Bank swap */
    if (uAddr < (FLASH_BASE + (FLASH_BANK_SIZE)))
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }

  return bank;
}


/**
  * @brief  Gets the address of a bank
  * @param  Bank: Bank ID
  * @retval Address of the bank
  */
static uint32_t GetBankAddr(uint32_t Bank)
{
  if (Bank == FLASH_BANK_2)
  {
    return  FLASH_BASE + (FLASH_BANK_SIZE);
  }
  else
  {
    return FLASH_BASE;
  }
}

/**
  * @}
  */

/**
  * @}
  */
#endif /* USE_FW_UPDATE */
