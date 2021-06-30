/**
  ******************************************************************************
  * @file    stm32l462e_cell1_bus.c
  * @author  MCD Application Team
  * @brief   source file for the BSP BUS IO driver
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

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_bus.h"
#include "i2c.h"
#include "spi.h"
#if (USE_I2C1_SEMAPHORE == 1)
#include "rtosal.h"
#endif /* (USE_I2C1_SEMAPHORE == 1) */

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_BUS STM32L462E_CELL1 BUS
  * @{
  */

/** @defgroup STM32L462E_CELL1_BUS_Private_Variables STM32L462E_CELL1 BUS Private Variables
  * @{
  */

#if (USE_I2C1_SEMAPHORE == 1)
static osSemaphoreId I2C1_semaphore = NULL;
#endif /* (USE_I2C1_SEMAPHORE == 1) */
static uint32_t I2C1InitCounter = 0;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_BUS_Private_Defines STM32L462E_CELL1 BUS Private Defines
  * @{
  */

#if (USE_I2C1_SEMAPHORE == 1)
#define I2C1_TIMEOUT_MAX (5000U)
#endif /* (USE_I2C1_SEMAPHORE == 1) */

#if (USE_I2C1_SEMAPHORE == 1)
#define TAKE_SEMA_I2C1() if (I2C1_semaphore != NULL) {(void) rtosalSemaphoreAcquire(I2C1_semaphore, I2C1_TIMEOUT_MAX);}
#define RELEASE_SEMA_I2C1() if (I2C1_semaphore != NULL) {(void) rtosalSemaphoreRelease(I2C1_semaphore);}
#else
#define TAKE_SEMA_I2C1() __NOP();
#define RELEASE_SEMA_I2C1() __NOP();
#endif /* (USE_I2C1_SEMAPHORE == 1) */

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_BUS_Private_Functions_Prototypes STM32L462E_CELL1 BUS Private Functions Prototypes
  * @{
  */

static void I2C1_MspInit(I2C_HandleTypeDef *hI2c);
static void I2C1_MspDeInit(I2C_HandleTypeDef *hI2c);
static void SPI3_MspInit(SPI_HandleTypeDef *hspi);
static void SPI3_MspDeInit(SPI_HandleTypeDef *hspi);

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_BUS_Exported_Functions STM32L462E_CELL1 BUS Exported Functions
  * @{
  */

/* BUS IO driver over I2C Peripheral */
/*******************************************************************************
                            BUS OPERATIONS OVER I2C
  *******************************************************************************/
/**
  * @brief  Initialize I2C HAL
  * @retval BSP status
  */
int32_t BSP_I2C1_Init(void)
{
  int32_t ret = BSP_ERROR_NONE;

  hi2c1.Instance  = I2C1;

  if (I2C1InitCounter == 0U)
  {
    I2C1InitCounter++;

#if (USE_I2C1_SEMAPHORE == 1)
    if (I2C1_semaphore == NULL)
    {
      I2C1_semaphore = rtosalSemaphoreNew((const rtosal_char_t *)"I2C1_ACCESS_SEMA", 1U);
    }
#endif /* (USE_I2C1_SEMAPHORE == 1) */

    TAKE_SEMA_I2C1();
    if (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_RESET)
    {
      /* Init the I2C Msp */
      I2C1_MspInit(&hi2c1);

      /* Init the I2C */
      MX_I2C1_Init();

      if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
      {
        ret = BSP_ERROR_BUS_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
    RELEASE_SEMA_I2C1();
  }

  return ret;
}

/**
  * @brief  DeInitialize I2C HAL.
  * @retval BSP status
  */
int32_t BSP_I2C1_DeInit(void)
{
  int32_t ret = BSP_ERROR_NONE;

  TAKE_SEMA_I2C1();
  if (I2C1InitCounter > 0U)
  {
    I2C1InitCounter--;
    if (I2C1InitCounter == 0U)
    {
      /* DeInit the I2C */
      I2C1_MspDeInit(&hi2c1);

      /* DeInit the I2C */
      if (HAL_I2C_DeInit(&hi2c1) != HAL_OK)
      {
        ret = BSP_ERROR_BUS_FAILURE;
      }
    }
  }
  RELEASE_SEMA_I2C1();
  return ret;
}

/**
  * @brief  Write a value in a register (8 bits) of the device through BUS.
  * @param  DevAddr Device address on Bus.
  * @param  Reg    The target register address to write
  * @param  pData  Pointer to data buffer to write
  * @param  Length Data Length
  * @retval BSP status
  */

int32_t BSP_I2C1_WriteReg_8b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  TAKE_SEMA_I2C1();
  if (HAL_I2C_Mem_Write(&hi2c1, DevAddr, Reg, I2C_MEMADD_SIZE_8BIT, pData, Length, BUS_I2C1_POLL_TIMEOUT) != HAL_OK)
  {
    if (HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret =  BSP_ERROR_PERIPH_FAILURE;
    }
  }
  RELEASE_SEMA_I2C1();
  return ret;
}

/**
  * @brief  Read a register (8 bits) of the device through BUS
  * @param  DevAddr Device address on Bus.
  * @param  Reg    The target register address to read
  * @param  pData  Pointer to data buffer to read
  * @param  Length Data Length
  * @retval BSP status
  */
int32_t  BSP_I2C1_ReadReg_8b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  TAKE_SEMA_I2C1();
  if (HAL_I2C_Mem_Read(&hi2c1, DevAddr, Reg, I2C_MEMADD_SIZE_8BIT, pData, Length, BUS_I2C1_POLL_TIMEOUT) != HAL_OK)
  {
    if (HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }
  RELEASE_SEMA_I2C1();
  return ret;
}

/**
  * @brief  Write a value in a register (16 bits) of the device through BUS.
  * @param  DevAddr Device address on Bus.
  * @param  Reg    The target register address to write
  * @param  pData  Pointer to data buffer to write
  * @param  Length Data Length
  * @retval BSP status
  */

int32_t BSP_I2C1_WriteReg_16b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  TAKE_SEMA_I2C1();
  if (HAL_I2C_Mem_Write(&hi2c1, DevAddr, Reg, I2C_MEMADD_SIZE_16BIT, pData, Length, BUS_I2C1_POLL_TIMEOUT) != HAL_OK)
  {
    if (HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret =  BSP_ERROR_PERIPH_FAILURE;
    }
  }
  RELEASE_SEMA_I2C1();
  return ret;
}

/**
  * @brief  Read a register (16 bits) of the device through BUS
  * @param  DevAddr Device address on Bus.
  * @param  Reg    The target register address to read
  * @param  pData  Pointer to data buffer to read
  * @param  Length Data Length
  * @retval BSP status
  */
int32_t  BSP_I2C1_ReadReg_16b(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  TAKE_SEMA_I2C1();
  if (HAL_I2C_Mem_Read(&hi2c1, DevAddr, Reg, I2C_MEMADD_SIZE_16BIT, pData, Length, BUS_I2C1_POLL_TIMEOUT) != HAL_OK)
  {
    if (HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)
    {
      ret = BSP_ERROR_BUS_ACKNOWLEDGE_FAILURE;
    }
    else
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }
  RELEASE_SEMA_I2C1();
  return ret;
}

/**
  * @brief  Checks if target device is ready for communication.
  * @param  DevAddr Device address on Bus.
  * @param  Trials Number of trials
  * @retval BSP status
  */
int32_t BSP_I2C1_IsDeviceReady(uint16_t DevAddr, uint32_t Trials)
{
  int32_t ret = BSP_ERROR_NONE;

  TAKE_SEMA_I2C1();
  if (HAL_I2C_IsDeviceReady(&hi2c1, DevAddr, Trials, BUS_I2C1_POLL_TIMEOUT) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  RELEASE_SEMA_I2C1();
  return ret;
}

/* BUS IO driver over SPI Peripheral */
/*******************************************************************************
                            BUS OPERATIONS OVER SPI
  *******************************************************************************/
/**
  * @brief  Initializes SPI HAL.
  * @retval BSP status
  */
int32_t BSP_SPI3_Init(void)
{
  int32_t ret = BSP_ERROR_NONE;

  hspi3.Instance  = SPI3;

  if (HAL_SPI_GetState(&hspi3) == HAL_SPI_STATE_RESET)
  {
    /* Init the SPI Msp */
    SPI3_MspInit(&hspi3);

    /* Init the SPI */
    MX_SPI3_Init();
  }

  return ret;
}

/**
  * @brief  DeInitializes SPI HAL.
  * @retval BSP status
  */
int32_t BSP_SPI3_DeInit(void)
{
  int32_t ret  = BSP_ERROR_NONE;

  SPI3_MspDeInit(&hspi3);

  /* DeInit the SPI*/
  if (HAL_SPI_DeInit(&hspi3) == HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }

  return ret;
}


/**
  * @brief  Write Data through SPI BUS.
  * @param  pData  Pointer to data buffer to send
  * @param  Length Length of data in byte
  * @retval BSP status
  */
int32_t BSP_SPI3_Send(uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_NONE;

  if (HAL_SPI_Transmit(&hspi3, pData, Length, BUS_SPI3_TIMEOUT) == HAL_OK)
  {
    ret = BSP_ERROR_NONE;
  }
  return ret;
}

/**
  * @brief  Receive Data from SPI BUS
  * @param  pData  Pointer to data buffer to receive
  * @param  Length Length of data in byte
  * @retval BSP status
  */
int32_t  BSP_SPI3_Recv(uint8_t *pData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_UNKNOWN_FAILURE;
  uint32_t tx_data = 0xFFFFFFFFU;

  if (HAL_SPI_TransmitReceive(&hspi3, (uint8_t *)&tx_data, pData, Length, BUS_SPI3_TIMEOUT) == HAL_OK)
  {
    ret = BSP_ERROR_NONE;
  }
  return ret;
}

/**
  * @brief  Send and Receive data to/from SPI BUS (Full duplex)
  * @param  pTxData  Pointer to data buffer to send
  * @param  pRxData  Pointer to data buffer to receive
  * @param  Length   Length of data in byte
  * @retval BSP status
  */
int32_t BSP_SPI3_SendRecv(uint8_t *pTxData, uint8_t *pRxData, uint16_t Length)
{
  int32_t ret = BSP_ERROR_UNKNOWN_FAILURE;

  if (HAL_SPI_TransmitReceive(&hspi3, pTxData, pRxData, Length, BUS_SPI3_TIMEOUT) == HAL_OK)
  {
    ret = BSP_ERROR_NONE;
  }

  return ret;
}

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_BUS_Private_Functions STM32L462E_CELL1 BUS Private Functions
  * @{
  */

static void I2C1_MspInit(I2C_HandleTypeDef *hI2c)
{
  HAL_I2C_MspInit(hI2c);
}
static void I2C1_MspDeInit(I2C_HandleTypeDef *hI2c)
{
  HAL_I2C_MspDeInit(hI2c);
}

static void SPI3_MspInit(SPI_HandleTypeDef *hspi)
{
  HAL_SPI_MspInit(hspi);
}

static void SPI3_MspDeInit(SPI_HandleTypeDef *hspi)
{
  HAL_SPI_MspDeInit(hspi);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
