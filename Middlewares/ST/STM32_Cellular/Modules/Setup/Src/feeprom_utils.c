/**
  ******************************************************************************
  * @file    feeprom_utils.c
  * @author  MCD Application Team
  * @brief   feeprom utils for setup management
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#include "feeprom_utils.h"
#include "app_select.h"
#include "menu_utils.h"

/* Private defines -----------------------------------------------------------*/
#define FEEPROM_UTILS_MAGIC_FLASH_CONFIG1 ((uint32_t)0x00000002)
#define FEEPROM_UTILS_MAGIC_FLASH_CONFIG2 ((uint32_t)0x5555AAAA)

/* Private macros ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* flash config header */
typedef struct
{
  uint32_t config_magic1;
  uint32_t config_magic2;
  uint16_t appli_code;
  uint16_t appli_version;
  uint32_t config_size;
} setup_config_header_t;

typedef struct
{
  setup_config_header_t  header;
  uint8_t                data;
} setup_config_t;


/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t feeprom_utils_flash_write(uint8_t *data_addr, uint8_t *flash_addr, uint32_t Lenght,
                                          uint32_t *byteswritten);
static void feeprom_utils_open_flash(void);
static void feeprom_utils_close_flash(void);

static uint32_t feeprom_utils_flash_erase(uint32_t page_number);

/*  get the flash bank associated to an application (appli_code) */
static uint32_t feeprom_utils_find_bank(setup_appli_code_t appli_code, setup_appli_version_t appli_version,
                                        setup_config_t **addr,
                                        uint32_t *page_number);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  open flash before write operation
  * @param  none
  * @retval none
  */
static void feeprom_utils_open_flash(void)
{
  /* Unlock the Flash to enable the flash control register access *************/
  (void)HAL_FLASH_Unlock();

  /* Erase the user Flash area
    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

  /* Clear OPTVERR bit set on virgin samples */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
}

/**
  * @brief  close flash after write operation
  * @param  none
  * @retval none
  */
static void feeprom_utils_close_flash(void)
{
  /* Lock the Flash to disable the flash control register access (recommended
   to protect the FLASH memory against possible unwanted operation) *********/
  (void)HAL_FLASH_Lock();
}

/**
  * @brief  Write "Lenght" bytes in feeprom at "flash_addr
  * @param  data_addr    data addr to write
  * @param  flash_addr   flash addr where write
  * @param  Lenght       lenght of data to write
  * @param  byteswritten (out) number of written bytes
  * @retval status (0=>write OK / 1=>write error)
  */
static uint32_t feeprom_utils_flash_write(uint8_t *data_addr, uint8_t *flash_addr, uint32_t Lenght,
                                          uint32_t *byteswritten)
{
  uint32_t tmp_lenght;
  uint32_t tmp_data_addr;
  uint32_t tmp_flash_addr;
  uint32_t ret;

  ret = 0U;
  tmp_lenght     = Lenght;
  tmp_data_addr  = (uint32_t)data_addr;
  tmp_flash_addr = (uint32_t)flash_addr;

  *byteswritten = 0U;
  if (tmp_lenght > FLASH_PAGE_SIZE)
  {
    tmp_lenght = FLASH_PAGE_SIZE;
  }

  while (*byteswritten < tmp_lenght)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, tmp_flash_addr, *((uint64_t *)tmp_data_addr)) == HAL_OK)
    {
      tmp_flash_addr = tmp_flash_addr + sizeof(uint64_t);
      tmp_data_addr  = tmp_data_addr  + sizeof(uint64_t);
      *byteswritten    = *byteswritten + sizeof(uint64_t);
    }
    else
    {
      PRINT_SETUP("HAL_FLASH_Program error")
      ret = 1U;
      break;
    }
  }
  return ret;
}


/* External functions BEGIN */

/**
  * @brief  get the flash bank associated to an application (appli_code)
  * @param  appli_code               code of owner of configuration
  * @param  appli_version            format version of configuration
  * @param  addr                     (out) addr of bank found
  * @param  page_number              (out) feeprom page number of bank found
  * @retval status                    FEEPROM_UTILS_NO_BANK_FOUND
  * @note                             FEEPROM_UTILS_INVALID_VERSION_BANK_FOUND
  * @note                             FEEPROM_UTILS_MATCHING_BANK_FOUND
  * @note                             FEEPROM_UTILS_FREE_BANK_FOUND
  */
static uint32_t feeprom_utils_find_bank(setup_appli_code_t appli_code, setup_appli_version_t appli_version,
                                        setup_config_t **addr,
                                        uint32_t *page_number)
{
  setup_config_t *setup_config;
  uint32_t  i;
  uint32_t   ret;
  const uint8_t  *label;

  ret  = FEEPROM_UTILS_NO_BANK_FOUND;
  *addr        = 0U;
  *page_number = 0U;

  /* find a free bank */
  for (i = 0U; i < (uint32_t)FEEPROM_UTILS_APPLI_MAX ; i++)
  {
    setup_config = (setup_config_t *)(FEEPROM_UTILS_LAST_PAGE_ADDR - (i * FLASH_PAGE_SIZE));
    if ((setup_config->header.config_magic1 == FEEPROM_UTILS_MAGIC_FLASH_CONFIG1)
        && (setup_config->header.config_magic2 == FEEPROM_UTILS_MAGIC_FLASH_CONFIG2))
    {
      /* used band found */
      if (setup_config->header.appli_code == (uint16_t)appli_code)
      {
        *addr        = setup_config;
        if (setup_config->header.appli_version != appli_version)
        {
          /* the band has an invalid format version */
          *page_number = (uint32_t)FLASH_LAST_PAGE_NUMBER - i;
          label = setup_get_label_appli(appli_code);
          PRINT_SETUP("WARNING : \"%s\"  FEEPROM Config Bad format : config erased\r\n", label)

          /* Invalid version of config for this application => the page is erased and made available */
          if (feeprom_utils_flash_erase(*page_number) == 0U)
          {
            ret = FEEPROM_UTILS_INVALID_VERSION_BANK_FOUND;
          }
        }
        else
        {
          /* matching bank found */
          *page_number = (uint32_t)FLASH_LAST_PAGE_NUMBER - i;
          ret = FEEPROM_UTILS_MATCHING_BANK_FOUND;
          break;
        }
      }
    }
    else
    {
      /* free bank found */
      if (*addr == 0U)
      {
        *addr        = setup_config;
        *page_number = (uint32_t)FLASH_LAST_PAGE_NUMBER - i;
        ret = FEEPROM_UTILS_FREE_BANK_FOUND;
      }
    }
  }
  return ret ;
}

/**
  * @brief  erase a flash page
  * @param  page_number             feeprom page number of bank to erase
  * @retval status (0=>erase OK / 1=>erase error)
  */
static uint32_t feeprom_utils_flash_erase(uint32_t page_number)
{
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PAGEError;
  HAL_StatusTypeDef hal_ret;
  uint32_t ret;
  ret = 0;

  feeprom_utils_open_flash();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Banks       = FLASH_BANK_NUMBER;
  EraseInitStruct.Page        = page_number;
  EraseInitStruct.NbPages     = (uint32_t)1;
  hal_ret = HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
  if (hal_ret != HAL_OK)
  {
    ret = 1;
    PRINT_SETUP("HAL_FLASHEx_Erase error")
  }
  feeprom_utils_close_flash();

  return ret;
}

/**
  * @brief  erase a setup configuration in flash
  * @param  appli_code          owner code of application to erase
  * @param  appli_version       format version of configuration
  * @retval status (0=>erase OK / 1=>erase error)
  */
uint32_t feeprom_utils_setup_erase(setup_appli_code_t appli_code, setup_appli_version_t version_appli)
{
  uint32_t ret;
#if (FEEPROM_UTILS_FLASH_USED == 1)
  uint32_t page_number;
  setup_config_t *flash_addr;

  ret = feeprom_utils_find_bank(appli_code, version_appli, &flash_addr, &page_number);
  if (ret != FEEPROM_UTILS_NO_BANK_FOUND)
  {
    ret = feeprom_utils_flash_erase(page_number);
  }
#else
  ret = 0U;
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
  return ret;
}

/**
  * @brief  erase all flash pages of the configuration flash bank
  * @param  none
  * @retval none
  */
void feeprom_utils_flash_erase_all(void)
{
  uint32_t i;
  for (i = (uint32_t)0; i < (uint32_t)FEEPROM_UTILS_APPLI_MAX ; i++)
  {
    (void)feeprom_utils_flash_erase((uint32_t)FLASH_LAST_PAGE_NUMBER - i);
  }
}

/**
  * @brief  save the configuration of an application in flash
  * @param  appli_code               code of owner of configuration
  * @param  appli_version            format version of configuration
  * @param  config_addr              config addr to save
  * @param  config_size              config size to save
  * @retval status
  */
uint32_t feeprom_utils_save_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version,
                                         uint8_t *config_addr, uint32_t config_size)
{
  uint32_t  res;
  uint32_t  count;
  uint32_t byteswritten;
  uint32_t page_number;
  setup_config_header_t config_header;
  setup_config_t       *feeprom_config;

  config_header.config_magic1  = FEEPROM_UTILS_MAGIC_FLASH_CONFIG1;
  config_header.config_magic2  = FEEPROM_UTILS_MAGIC_FLASH_CONFIG2;
  config_header.appli_code    = (uint16_t)appli_code;
  config_header.appli_version = appli_version;
  config_header.config_size   = config_size;

  byteswritten = 0U;
  count = 0U;

  res = feeprom_utils_find_bank(appli_code, appli_version, &feeprom_config, &page_number);
  if (res != FEEPROM_UTILS_NO_BANK_FOUND)
  {
    res = feeprom_utils_flash_erase(page_number);
    if (res == 0U)
    {
      feeprom_utils_open_flash();

      res = feeprom_utils_flash_write(config_addr, &feeprom_config->data, config_size, &byteswritten);
      if (res == 0U)
      {
        count = byteswritten;
        res = feeprom_utils_flash_write((uint8_t *)&config_header, (uint8_t *)feeprom_config,
                                        sizeof(setup_config_header_t), &byteswritten);
        if (res != 0U)
        {
          count = 0U;
        }
        else
        {
          count += byteswritten;
        }
      }
      feeprom_utils_close_flash();
    }
  }
  else
  {
    PRINT_SETUP("Save config fail: no available bank found")
  }
  return count;
}

/**
  * @brief  get the configuration of an application (appli_code) in flash
  * @param  appli_code               code of owner of configuration
  * @param  appli_version            format version of configuration
  * @param  config_addr              config addr to save
  * @param  config_size              config size to save
  * @retval status                   0=>OK / 1=>KO
  */
uint32_t feeprom_utils_read_config_flash(setup_appli_code_t appli_code, setup_appli_version_t appli_version,
                                         uint8_t **config_addr, uint32_t *config_size)
{
  uint32_t ret;
  uint32_t ret_feeprom;
  uint32_t page_number;
  setup_config_t *setup_config;

  ret_feeprom = feeprom_utils_find_bank(appli_code, appli_version, &setup_config, &page_number);

  if (ret_feeprom == FEEPROM_UTILS_MATCHING_BANK_FOUND)
  {
    *config_size   = setup_config->header.config_size;
    *config_addr   = &setup_config->data;
    ret = 0U;
  }
  else
  {
    ret = 1U;
  }

  return ret;
}

/* External functions END */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

