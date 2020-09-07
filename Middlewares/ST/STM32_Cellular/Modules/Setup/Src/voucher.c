/**
  ******************************************************************************
  * @file    voucher.c
  * @author  MCD Application Team
  * @brief   modem voucher read and display
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "plf_config.h"
#if (USE_MODEM_VOUCHER == 1)
#include "app_select.h"
#include "menu_utils.h"
#include "voucher.h"
#include "i2c.h"
#include "cmsis_os_misrac2012.h"

/* Private defines -----------------------------------------------------------*/

#if defined(USE_MODEM_BG96)
/* MODEM BG96 */
#define VOUCHER_PRESENT     (1)
#define ADDR_USER_ZONE ((uint16_t)0xAE) /* new address: E2=E1=1, E0=0 */
#define ADDR_OTP_ZONE  ((uint16_t)0xBE) /* new address: E2=E1=1, E0=0 */
#elif defined(USE_MODEM_TYPE1SC)
/* MODEM TYPE1SC */
#define VOUCHER_PRESENT     (1)
#define ADDR_USER_ZONE ((uint16_t)0xA0) /* address: E2=E1=E0=0 */
#define ADDR_OTP_ZONE  ((uint16_t)0xB0) /* address: E2=E1=E0=0 */
#else
/* no EEPROM */
#define VOUCHER_PRESENT     (0)
#define ADDR_USER_ZONE ((uint16_t)0x0) /* address: E2=E1=E0=0 */
#define ADDR_OTP_ZONE  ((uint16_t)0x0) /* address: E2=E1=E0=0 */
#endif  /* USE_MODEM_BG96 */


#if (VOUCHER_PRESENT == 1)

#define VOUCHER_APP_SELECT_LABEL "Display Voucher code"

#define VOUCHER_DATA_I2C_ADDR 112U
#define VOUCHER_DATA_SIZE      17U

/* Private macros ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static bool voucher_display(void);

/* static function declarations */

/* voucher_read_modem_voucher_data()
 * this function read modem voucher data from EEPROM user zone (non-OTP)
 */
static void voucher_read_modem_voucher_data(uint16_t size, uint8_t *data)
{
  uint16_t i;

  /* reset data */
  (void)memset(data, 0, size);

  /* read bytes 1 by 1 (do not use read page !!! data can exceed 64 bytes) */
  for (i = 0; i < size; i++)
  {
    (void)HAL_I2C_Mem_Read(&hi2c1,
                           ADDR_USER_ZONE, VOUCHER_DATA_I2C_ADDR + i,
                           I2C_MEMADD_SIZE_16BIT,
                           (uint8_t *)(data + i),
                           1,
                           10000);
    (void) osDelay(5);
  }
}

static bool voucher_display(void)
{
  static uint8_t voucher_data[VOUCHER_DATA_SIZE];
  uint8_t car;

  PRINT_SETUP("\n\r\n\r")
  PRINT_SETUP("---------------\n\r")
  PRINT_SETUP("  Modem Voucher\n\r")
  PRINT_SETUP("---------------\n\r")

  voucher_read_modem_voucher_data(VOUCHER_DATA_SIZE, voucher_data);
  voucher_data[VOUCHER_DATA_SIZE - 1U] = 0U;
  PRINT_SETUP("Voucher code is: %s\n\r", voucher_data)
  PRINT_SETUP("\n\r")
  PRINT_SETUP("\n\r")
  PRINT_SETUP("Type any key to continue\n\r")
  /* Waiting for a key to reboot the board */
  (void)menu_utils_get_uart_char(&car);

  return false;
}
#endif /* (VOUCHER_PRESENT == 1) */

/**
  * @brief  setup component init
  * @param  none
  * @retval none
  */
void voucher_init(void)
{
#if (VOUCHER_PRESENT == 1)
  (void)app_select_record((AS_CHART_t *)VOUCHER_APP_SELECT_LABEL, voucher_display);
#endif /* (VOUCHER_PRESENT == 1) */
}
#endif /* (USE_MODEM_VOUCHER == 1) */


/* External functions BEGIN */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
