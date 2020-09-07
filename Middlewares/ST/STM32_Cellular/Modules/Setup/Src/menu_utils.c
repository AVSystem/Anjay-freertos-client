/**
  ******************************************************************************
  * @file    menu_utils.c
  * @author  MCD Application Team
  * @brief   utils for menu management
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
#include "menu_utils.h"
#include <string.h>
#include "cellular_runtime_custom.h"
#include "cellular_runtime_standard.h"
#include "cmsis_os_misrac2012.h"


/* Private macros ------------------------------------------------------------*/

/* Size max of a configuration = feeprom page size */

/* Private defines -----------------------------------------------------------*/


/* Private typedef -----------------------------------------------------------*/
/* contains setup parameters */
typedef struct
{
  menu_setup_source_t type;
  uint8_t            *buffer;
  uint32_t            index;
  uint32_t            default_index;
  uint32_t            size;
} menu_utils_src_config_t;

typedef struct
{
  uint8_t            buffer[MENU_UTILS_SETUP_CONFIG_SIZE_MAX];
  uint32_t           index;
} menu_utils_dst_config_t;

/* Private variables ---------------------------------------------------------*/

/* temporary structure of setup */
static menu_utils_src_config_t menu_utils_src_config = { MENU_SETUP_SOURCE_NONE, NULL, 0, 0, 0};

/* temporary buffer of new config */
static menu_utils_dst_config_t menu_utils_dst_config;

/* Global variables ----------------------------------------------------------*/
uint8_t menu_utils_dump_mode = 0U;

/* Private function prototypes -----------------------------------------------*/

/* get a char from current config source  */
static int32_t menu_utils_getchar(uint8_t *car);


/* Functions Definition ------------------------------------------------------*/
static int32_t menu_utils_getchar(uint8_t *car)
{
  int32_t ret;
  HAL_StatusTypeDef ret_hal;

  if ((menu_utils_src_config.type == MENU_SETUP_SOURCE_FEEPROM)
      || (menu_utils_src_config.type == MENU_SETUP_SOURCE_DEFAULT))
  {
    if (menu_utils_src_config.index < menu_utils_src_config.size)
    {
      if (menu_utils_src_config.buffer[menu_utils_src_config.index] == 0xaU)
      {
        menu_utils_src_config.index++;
      }
      *car = menu_utils_src_config.buffer[menu_utils_src_config.index];
      menu_utils_src_config.index++;

      ret = 1;
    }
    else
    {
      *car = 0U;
      ret  = 0;
    }
  }
  else if (menu_utils_src_config.type == MENU_SETUP_SOURCE_UART)
  {

    ret_hal = HAL_UART_Receive(&TRACE_INTERFACE_UART_HANDLE, car, 1U, 0xFFFFFFU);
    while (ret_hal == HAL_BUSY)
    {
      (void)osDelay(10U);
      ret_hal = HAL_UART_Receive(&TRACE_INTERFACE_UART_HANDLE, car, 1U, 0xFFFFFFU);
    }
    if (ret_hal == HAL_OK)
    {
      ret = 1;
    }
    else
    {
      *car = 0U;
      ret = 0;
    }
  }
  else
  {
    *car = 0U;
    ret = 0;
  }

  return ret;
}


/* External functions BEGIN */

/* get a char from uart with timeout  */
int32_t menu_utils_get_char_timeout(uint8_t *car, uint32_t timeout)
{
  HAL_StatusTypeDef ret_hal;
  int32_t ret;

  ret_hal = HAL_UART_Receive(&TRACE_INTERFACE_UART_HANDLE, car, 1U, timeout);
  while (ret_hal == HAL_BUSY)
  {
    (void)osDelay(10U);
    ret_hal = HAL_UART_Receive(&TRACE_INTERFACE_UART_HANDLE, car, 1U, timeout);
  }

  if (ret_hal == HAL_TIMEOUT)
  {
    ret = 0;
  }
  else
  {
    ret = 1;
  }
  return ret;
}

/* get a char from the console uart  */
int32_t menu_utils_get_uart_char(uint8_t *car)
{
  int32_t ret;
  HAL_StatusTypeDef ret_hal;

  ret_hal = HAL_UART_Receive(&TRACE_INTERFACE_UART_HANDLE, car, 1U, 0xFFFFFFU);
  while (ret_hal == HAL_BUSY)
  {
    (void)osDelay(10U);
    ret_hal = HAL_UART_Receive(&TRACE_INTERFACE_UART_HANDLE, car, 1U, 0xFFFFFFU);
  }

  if (ret_hal == HAL_OK)
  {
    ret = 1;
  }
  else
  {
    *car = 0U;
    ret  = 0;
  }

  return ret;
}


/* get the next line of the current config source  */
uint32_t  menu_utils_get_line(uint8_t string[], uint32_t size)
{
  uint32_t  i;
  int32_t   break_r;
  uint32_t  comment_flag = 0U;
  int32_t   ret;

  i = 0U;
  break_r = 1;
  while (break_r != 0)
  {
    ret = menu_utils_getchar(&string[i]);
    if (ret == 0)
    {
      /* no more char available */
      i = 0U;
      break_r = 0;
    }
    else if (string[i] == (uint8_t)'#')
    {
      /* commentary line : ignore the the following chars */
      comment_flag = 1U;
      i = 0U;
    }

    else if ((string[i] == 0xdU) || (i >= (size - 1U)))
    {
      /* "\r" encountered : end of line  */
      if (comment_flag == 1U)
      {
        /* end of comment line */
        comment_flag = 0U;
        i = 0U;
      }
      else
      {
        break_r = 0;
      }
    }
    else if (comment_flag == 0U)
    {
      /* normal configuration char : echo it */
      /*      PRINT_SETUP("%c", string[i]) */ /* no echo */
      i++;
    }
    else
    {
      /* Comment line : Nothing to do */
    }
  }
  string[i] = 0U;

  return i;
}

/**
  * @brief  get and analyse the next line of the current config source
  * @param  title   name to display of the string to get
  * @param  size    size max of the string to get
  * @retval none
  */
void menu_utils_get_string(uint8_t *title, uint8_t string[], uint32_t size)
{
  uint32_t  i;
  uint32_t  len;

  if (menu_utils_src_config.type == MENU_SETUP_SOURCE_UART)
  {
    PRINT_SETUP("Enter %s ", title)

    if (menu_utils_src_config.default_index < menu_utils_src_config.size)
    {
      PRINT_SETUP("(")
      for (i = 0U ; menu_utils_src_config.buffer[menu_utils_src_config.default_index + i] != (uint8_t)'\r' ; i++)
      {
        PRINT_SETUP("%c", menu_utils_src_config.buffer[menu_utils_src_config.default_index + i])
      }
      PRINT_SETUP(")")
    }
    PRINT_SETUP(": ")
  }
  else
  {
    PRINT_SETUP("%s", title)
  }

  len = menu_utils_get_line(string, size);

  if (menu_utils_src_config.type == MENU_SETUP_SOURCE_UART)
  {
    /* configuration from uart : display default configuration */
    if (len == 0U)
    {
      for (i = 0U;
           (menu_utils_src_config.buffer[menu_utils_src_config.default_index + i] != (uint8_t)'\r')
           && ((menu_utils_src_config.default_index + i) < menu_utils_src_config.size) ;
           i++)
      {
        string[i] = menu_utils_src_config.buffer[menu_utils_src_config.default_index + i];
      }
      string[i] = 0U;
      PRINT_SETUP("\n\r%s", string)
    }
    else
    {
      /* configuration from flash : parse default configuration until next parameter */
      for (i = 0U;
           (menu_utils_src_config.buffer[menu_utils_src_config.default_index + i] != (uint8_t)'\r')
           && ((menu_utils_src_config.default_index + i) < menu_utils_src_config.size) ;
           i++)
      { /* find the end of the parameter : nothing to process */ }
    }
    menu_utils_src_config.default_index += i + 1U;

    /* Set the current parameter in configuration with the line got from uart */

    len = crs_strlen(string) + 1U;
    /* to avoid string overflow */
    if ((len + menu_utils_dst_config.index) <= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
    {
      (void)memcpy((CRC_CHAR_t *)&menu_utils_dst_config.buffer[menu_utils_dst_config.index],
                   (const CRC_CHAR_t *)string,
                   len);
      menu_utils_src_config.index += crs_strlen(string);
      menu_utils_dst_config.index += crs_strlen(string);
      menu_utils_dst_config.buffer[menu_utils_dst_config.index] = 0xdU;
      menu_utils_src_config.index++;
      menu_utils_dst_config.index++;
    }
  }

  PRINT_SETUP("\n\r")
}


/**
  * @brief  get and analyse the next line of the current config source
  * @note   default value must be ignored
  * @param  title   name to display of the string to get
  * @param  size    size max of the string to get
  * @retval none
  */
void   menu_utils_get_string_without_default_value(uint8_t *title, uint8_t string[], uint32_t size)
{
  string[0] = 0;
  uint32_t len;

  if (menu_utils_src_config.type == MENU_SETUP_SOURCE_UART)
  {
    PRINT_SETUP("Enter %s ", title)
    (void)menu_utils_get_line(string, size);

    /* Set the current parameter in configuration with the line got from uart */

    len = crs_strlen(string) + 1U;
    /* to avoid string overflow */
    if ((len + menu_utils_dst_config.index) <= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
    {
      (void)memcpy((CRC_CHAR_t *)&menu_utils_dst_config.buffer[menu_utils_dst_config.index],
                   (const CRC_CHAR_t *)string,
                   len);

      menu_utils_dst_config.index += crs_strlen(string);
      menu_utils_dst_config.buffer[menu_utils_dst_config.index] = 0xdU;
      menu_utils_dst_config.index++;
      PRINT_SETUP("\n\r")
    }
  }
}

/**
  * @brief  adding a default value in config
  * @param  string   default value to add in vonfig
  * @retval none
  */
void menu_utils_set_sefault_value(uint8_t string[])
{
  uint32_t size;

  if (menu_utils_src_config.type == MENU_SETUP_SOURCE_UART)
  {
    /* Set the current parameter in configuration with the line got from uart */
    size = crs_strlen(string) + 1U;
    /* to avoid string overflow */
    if ((size + menu_utils_dst_config.index) <= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
    {
      (void)memcpy((CRC_CHAR_t *)&menu_utils_dst_config.buffer[menu_utils_dst_config.index],
                   (const CRC_CHAR_t *)string,
                   size);
      menu_utils_dst_config.index += crs_strlen(string);
      menu_utils_dst_config.buffer[menu_utils_dst_config.index] = 0xdU;
      menu_utils_dst_config.index++;
    }
  }
}

/**
  * @brief  get  the next default value from current configuration
  * @param  default_value_string   (out) defautlt value returned
  * @param  update                 if update == 1 the default value is flushed from the current config
  * @retval none
  */
uint32_t menu_utils_get_next_default_value(uint8_t *default_value_string, uint8_t update)
{
  uint16_t i;

  for (i = 0U;
       (menu_utils_src_config.buffer[menu_utils_src_config.default_index + i] != (uint8_t)'\r')
       && ((menu_utils_src_config.default_index + i) < menu_utils_src_config.size) ;
       i++)
  {
    default_value_string[i] = menu_utils_src_config.buffer[menu_utils_src_config.default_index + i];
  }
  default_value_string[i] = 0;

  if (update == 1U)
  {
    /* default value is flushed from the current config */
    menu_utils_src_config.default_index = menu_utils_src_config.default_index + i + 1U;
    menu_utils_src_config.index = menu_utils_src_config.default_index;
  }

  return i;
}

/**
  * @brief  copy next config value from source config to destination config
  * @param  none
  * @retval none
  */
uint32_t menu_utils_copy_next_value(void)
{
  uint16_t i;

  for (i = 0U;
       (menu_utils_src_config.buffer[menu_utils_src_config.index + i] != (uint8_t)'\r')
       && ((menu_utils_src_config.index + i) < menu_utils_src_config.size) ;
       i++)
  {
    menu_utils_dst_config.buffer[menu_utils_dst_config.index + i] =
      menu_utils_src_config.buffer[menu_utils_src_config.index + i];
  }

  if (i != 0U)
  {
    menu_utils_dst_config.buffer[menu_utils_dst_config.index + i] = 0xdU;
    menu_utils_src_config.index = menu_utils_src_config.index + i + 1U;
    menu_utils_dst_config.index = menu_utils_dst_config.index + i + 1U;
  }

  return i;
}

/**
  * @brief  replace  the next default value from current configuration by new_value
  * @param  new_value   new value to set
  * @retval lenght of new value
  */
uint32_t menu_utils_replace_next_value(uint8_t *new_value)
{
  uint16_t i;
  uint32_t new_len;
  uint32_t old_len;

  old_len = 0U;
  new_len = crs_strlen(new_value);

  for (i = 0U ; (menu_utils_src_config.buffer[menu_utils_src_config.index + i] != (uint8_t)'\r')
       && ((menu_utils_src_config.index + i) < menu_utils_src_config.size) ; i++)
  {
    old_len++;
  }

  /* to avoid string overflow */
  if ((new_len + menu_utils_dst_config.index) < MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
  {
    (void)memcpy(&menu_utils_dst_config.buffer[menu_utils_dst_config.index], new_value, new_len);
    menu_utils_dst_config.buffer[menu_utils_dst_config.index + new_len] = (uint8_t)'\r';
    menu_utils_src_config.index = menu_utils_src_config.index + old_len + 1U;
    menu_utils_dst_config.index = menu_utils_dst_config.index + new_len + 1U;
  }
  else
  {
    new_len = 0;
  }

  return new_len;
}

/**
  * @brief  insert a value in current configuration
  * @param  default_value_string   value to insert
  * @retval status: 0 => insertion OK
  */
uint32_t menu_utils_insert_default_value(uint8_t *default_value_string)
{
  uint32_t size;
  size = crs_strlen(default_value_string) + 1U;

  /* to avoid string overflow */
  if ((size + menu_utils_src_config.index) <= MENU_UTILS_SETUP_CONFIG_SIZE_MAX)
  {
    (void)memcpy((CRC_CHAR_t *)&menu_utils_dst_config.buffer[menu_utils_src_config.index],
                 (const CRC_CHAR_t *)default_value_string,
                 size);
    menu_utils_dst_config.index += crs_strlen(default_value_string);
    menu_utils_dst_config.buffer[menu_utils_dst_config.index] = 0xdU;
    menu_utils_dst_config.index++;
  }
  return 0;
}

/**
  * @brief  flush the next nb default values from current config
  * @param  nb   number of value to flush
  * @retval none
  */
void menu_utils_flush_next_default_value(int16_t nb)
{
  uint16_t i;
  uint16_t j;
  for (j = 0U; j < (uint16_t)nb; j++)
  {
    for (i = 0U ; (menu_utils_src_config.buffer[menu_utils_src_config.default_index + i] != (uint8_t)'\r')
         && ((menu_utils_src_config.default_index + i) < menu_utils_src_config.size) ; i++)
    {
      /* Nothing to do */
    }

    menu_utils_src_config.default_index = menu_utils_src_config.default_index + i + 1U;
  }
}

/**
  * @brief  set the new configuration to parse
  * @param  config_addr   configuration addr
  * @param  config_size   configuration size
  * @retval none
  */
void menu_utils_get_new_config(uint8_t **config_addr, uint32_t  *config_size)
{
  *config_addr = menu_utils_dst_config.buffer;
  *config_size = menu_utils_dst_config.index;
}

/**
  * @brief  type          source of configuration
  * @param  config        configuration addr
  * @param  config_size   configuration size
  * @retval none
  */
void menu_utils_config_init(menu_setup_source_t type, uint8_t *config, uint32_t  config_size)
{
  menu_utils_src_config.type          = type;
  menu_utils_src_config.buffer        = config;
  menu_utils_src_config.size          = config_size;
  menu_utils_src_config.index         = 0U;
  menu_utils_src_config.default_index = 0U;

  menu_utils_dst_config.index         = 0U;
}

/* External functions END */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
