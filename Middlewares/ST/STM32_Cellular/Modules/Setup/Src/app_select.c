/**
  ******************************************************************************
  * @file    app_select.c
  * @author  MCD Application Team
  * @brief   application selection at boot
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
#include "app_select.h"
#include "plf_config.h"
#include <stdint.h>
#include "menu_utils.h"
#include "error_handler.h"
#include "cellular_version.h"
#if (FEEPROM_UTILS_FLASH_USED == 1)
#include "feeprom_utils.h"
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */

/* Private defines -----------------------------------------------------------*/

/* Number max of application recorded  */
#define APP_SELECT_NUMBER_MAX     10U

/* Timeout to display menu before to start the firmware */
#define APP_SELECT_TIMEOUT      4000U

#if (USE_BOOT_BEHAVIOUR_CONFIG == 1)
#define APP_SELECT_BOOT_BEHAVIOUR_LABEL "Boot behaviour selection"
#define APP_SELECT_BOOT_BEHAVIOUR_VERSION      1U
#endif  /*  (USE_BOOT_BEHAVIOUR_CONFIG == 1) */


/* Private macros ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/

/* data structure of recorded applications */
typedef struct
{
  uint8_t    *label;
  AS_HandlerCallback handler_callback;
} app_select_list_t;

/* Private variables ---------------------------------------------------------*/

/* Table of recorded applications */
static app_select_list_t app_select_list[APP_SELECT_NUMBER_MAX];
#if (USE_BOOT_BEHAVIOUR_CONFIG == 1)
#if (FEEPROM_UTILS_FLASH_USED == 1)
static uint8_t app_select_boot_behaviour_config_addr[8];
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
#endif  /*  (USE_BOOT_BEHAVIOUR_CONFIG == 1) */


/* number of recorded applications */
static uint16_t app_select_nb = 0U;

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
#if (USE_BOOT_BEHAVIOUR_CONFIG == 1)
#if (FEEPROM_UTILS_FLASH_USED == 1)
/**
  * @brief  boot behaviour selection
  * @param  none
  * @retval false
  */
static bool app_select_boot_behaviour_select(void)
{
  uint8_t car;
  uint32_t  count;
  car = 0u;

  PRINT_SETUP("\n\r\n\r")
  PRINT_SETUP("---------------------------\n\r")
  PRINT_SETUP("  Boot behaviour Selection\n\r")
  PRINT_SETUP("---------------------------\n\r")
  PRINT_SETUP("Automatic boot after delay (y/n) ")
  (void)menu_utils_get_uart_char(&car);

  if (car == 'n')
  {
    PRINT_SETUP("\n\rautomatic boot is set\n\r")

    (void)feeprom_utils_setup_erase(SETUP_BOOT_BEHAVIOUR, APP_SELECT_BOOT_BEHAVIOUR_VERSION);
  }
  else if (car == 'y')
  {
    count = feeprom_utils_save_config_flash(SETUP_BOOT_BEHAVIOUR,
                                            APP_SELECT_BOOT_BEHAVIOUR_VERSION,
                                            app_select_boot_behaviour_config_addr,
                                            sizeof(app_select_boot_behaviour_config_addr));
    if (count != 0)
    {
      PRINT_SETUP("\n\rwait for signal to boot is set\n\r")
    }
    else
    {
      PRINT_SETUP("\n\rERROR: boot behaviour cannot be set\n\r")
    }
  }
  else
  {
    PRINT_SETUP("\n\rboot behaviour not modified\n\r")
  }

  return false;
}
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
#endif  /*  (USE_BOOT_BEHAVIOUR_CONFIG == 1) */

/**
  * @brief  component init
  * @param  none
  * @retval none
  */
void app_select_init(void)
{
#if defined ( __GNUC__ )
  /* No more needed with SW4STM32 version 2.6.0 --
   * For SW4STM32-Eclipse only. This is needed to avoid problems when using printf(). */
  /*  setbuf(stdout, NULL); */
#endif /* __GNUC__ */
  app_select_nb = 0U;
}

/**
  * @brief  allows to add an application in menu
  * @param  app_label: label of appliocation to display
  * @param  handler_callback: application callback
  * @retval none
  */

/* allows to record an application */
int32_t app_select_record(AS_CHART_t *app_label, AS_HandlerCallback handler_callback)
{
  int32_t ret;
  if (app_select_nb >= APP_SELECT_NUMBER_MAX)
  {
    ERROR_Handler(DBG_CHAN_UTILITIES, 1, ERROR_WARNING);
    ret = -1;
  }
  else
  {
    app_select_list[app_select_nb].label = app_label;
    app_select_list[app_select_nb].handler_callback  = handler_callback;
    app_select_nb++;
    ret = 0;
  }
  return ret;
}

/**
  * @brief  component start -  display menu
  * @param  none
  * @retval application to start
  */
void app_select_start(void)
{
  uint16_t i;
  int32_t ret_car;
  uint8_t car;
  uint8_t num_appli;
  bool end;
  uint32_t timeout;

  car = 0U;

#if (USE_BOOT_BEHAVIOUR_CONFIG == 1)
#if (FEEPROM_UTILS_FLASH_USED == 1)
  uint8_t *behaviour_config_addr;
  uint32_t  behaviour_config_size;
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
#endif  /*  (USE_BOOT_BEHAVIOUR_CONFIG == 1) */

  timeout = APP_SELECT_TIMEOUT;

#if (USE_BOOT_BEHAVIOUR_CONFIG == 1)
#if (FEEPROM_UTILS_FLASH_USED == 1)
  (void)app_select_record((AS_CHART_t *)APP_SELECT_BOOT_BEHAVIOUR_LABEL, app_select_boot_behaviour_select);

  if (feeprom_utils_read_config_flash(SETUP_BOOT_BEHAVIOUR,
                                      APP_SELECT_BOOT_BEHAVIOUR_VERSION,
                                      &behaviour_config_addr,
                                      &behaviour_config_size) != 0U)
  {
    timeout = HAL_MAX_DELAY;
  }
#endif  /*  (FEEPROM_UTILS_FLASH_USED == 1) */
#endif  /*  (USE_BOOT_BEHAVIOUR_CONFIG == 1) */

  PRINT_SETUP("\n\r\n\r")
  PRINT_SETUP("=============================\n\r")
  PRINT_SETUP("    %s\n\r", CELLULAR_VERSION_FIRMWARE_NAME)
  PRINT_SETUP("    Version: %s\n\r", CELLULAR_VERSION_FIRMWARE_VERSION)
  PRINT_SETUP("=============================\n\r")

  do
  {
    PRINT_SETUP("Select the application to run:\n\r\n\r")
    for (i = (uint16_t)0; i < app_select_nb; i++)
    {
      PRINT_SETUP("%d: %s\n\r", (i + 1U), app_select_list[i].label)
    }
    PRINT_SETUP("\n\r")
    PRINT_SETUP("Or type any key to start\n\r")

    ret_car =  menu_utils_get_char_timeout(&car, timeout);
    if ((ret_car != 0) && (car >= 0x31U) && (car <= (0x30U + app_select_nb)))
    {
      num_appli = car - 0x30U - 1U;
      if ((uint16_t)num_appli < app_select_nb)
      {
        end = app_select_list[num_appli].handler_callback();
      }
      else
      {
        /* application code not match : starting firmware by default */
        end = true;
      }
    }
    else
    {
      /* application code not match : starting firmware by default */
      end = true;
    }
    timeout = HAL_MAX_DELAY;
    PRINT_SETUP("\n\r\n\r")
  } while (end == false);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
