/**
  ******************************************************************************
  * @file    stm32l462e_cell1_eeprom.h
  * @author  MCD Application Team
  * @brief   header file for the BSP EEPROM driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32L462E_CELL1_EEPROM_H
#define STM32L462E_CELL1_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_conf.h"
#include "stm32l462e_cell1_errno.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_EEPROM STM32L462E_CELL1 EEPROM
  * @{
  */

/** @defgroup STM32L462E_CELL1_EEPROM_Exported_Constants STM32L462E_CELL1 EEPROM Exported Constants
  * @{
  */
/* EEPROM M24128-DFMN6TP
   128Kbit (16Kbytes)
   page size is 64 bytes
   16 Kbytes = 16384 bits = 256 pages
*/
#define EEPROM_PAGE_NUMBER  256

/* EEPROM hardware address and page size */
#define EEPROM_I2C_ADDRESS           ((uint16_t)0xA0U)
#define EEPROM_I2C_ID_PAGE_ADDR      ((uint16_t)0xB0U) /** The Identification Page can be used to store sensitive
                                           * application parameters which can be (later) permanently
                                           * locked in Read-only mode.
                                           */
#define EEPROM_PAGESIZE             ((uint8_t)4U)

/* Maximum number of trials for BSP_EEPROM_WaitEepromStandbyState() function */
#define EEPROM_MAX_TRIALS           ((uint32_t)3000U)

#define EEPROM_OK                   ((uint32_t)0U)
#define EEPROM_FAIL                 ((uint32_t)1U)
#define EEPROM_TIMEOUT              ((uint32_t)2U)

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_EEPROM_Exported_Functions STM32L462E_CELL1 EEPROM Exported Functions
  * @{
  */
uint32_t BSP_EEPROM_Init(void);
uint32_t BSP_EEPROM_DeInit(void);
uint32_t BSP_EEPROM_ReadBuffer(uint8_t *pBuffer, uint16_t ReadAddr, uint16_t *NumByteToRead);
uint32_t BSP_EEPROM_WritePage(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t *NumByteToWrite);
uint32_t BSP_EEPROM_WriteBuffer(uint8_t *pBuffer, uint16_t WriteAddr, uint16_t NumByteToWrite);
uint32_t BSP_EEPROM_WaitEepromStandbyState(void);

/* USER Callbacks: This function is declared as __weak in EEPROM driver and
   should be implemented into user application.
   BSP_EEPROM_TIMEOUT_UserCallback() function is called whenever a timeout condition
   occurs during communication (waiting on an event that doesn't occur, bus
   errors, busy devices ...). */
void     BSP_EEPROM_TIMEOUT_UserCallback(void);

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
#endif /* __cplusplus */

#endif /* STM32L462E_CELL1_EEPROM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
