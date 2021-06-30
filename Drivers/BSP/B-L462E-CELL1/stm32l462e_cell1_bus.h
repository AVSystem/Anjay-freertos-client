/**
  ******************************************************************************
  * @file    stm32l462e_cell1_bus.h
  * @author  MCD Application Team
  * @brief   header file for the BSP BUS IO driver
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
#ifndef STM32L462E_CELL1_BUS_H
#define STM32L462E_CELL1_BUS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_conf.h"
#include "stm32l462e_cell1_errno.h"
#include "stm32l462e_cell1.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_BUS STM32L462E_CELL1 BUS
  * @{
  */

/** @defgroup STM32L462E_CELL1_BUS_Exported_Constants STM32L462E_CELL1 BUS Exported Constants
  * @{
  */
#define USE_I2C1_SEMAPHORE (1)

#ifndef BUS_I2C1_POLL_TIMEOUT
#define BUS_I2C1_POLL_TIMEOUT                ((uint32_t)0x1000U)
#endif /* BUS_I2C1_POLL_TIMEOUT */

/* I2C1 Frequeny in Hz  */
#ifndef BUS_I2C1_FREQUENCY
#define BUS_I2C1_FREQUENCY  1000000U /* Frequency of I2Cn = 100 KHz*/
#endif /* BUS_I2C1_FREQUENCY */

#ifndef BUS_SPI3_TIMEOUT
#define BUS_SPI3_TIMEOUT                  ((uint32_t)0x1000)
#endif /* BUS_SPI3_TIMEOUT */

#ifndef BUS_SPI1_BAUDRATE
#define BUS_SPI3_BAUDRATE  12500000    /* baud rate of SPIn = 12.5 Mbps*/
#endif /* BUS_SPI1_BAUDRATE */

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_BUS_Exported_Functions STM32L462E_CELL1 BUS Exported Functions
  * @{
  */

/* BUS IO driver over I2C Peripheral */
int32_t BSP_I2C1_Init(void);
int32_t BSP_I2C1_DeInit(void);
int32_t BSP_I2C1_WriteReg_8b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg_8b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_WriteReg_16b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg_16b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_IsDeviceReady(uint16_t DevAddr, uint32_t Trials);

/* BUS IO driver over SPI Peripheral */
int32_t BSP_SPI3_Init(void);
int32_t BSP_SPI3_DeInit(void);
int32_t BSP_SPI3_Send(uint8_t *pData, uint16_t Length);
int32_t BSP_SPI3_Recv(uint8_t *pData, uint16_t Length);
int32_t BSP_SPI3_SendRecv(uint8_t *pTxData, uint8_t *pRxData, uint16_t Length);

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

#endif /* STM32L462E_CELL1_BUS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
