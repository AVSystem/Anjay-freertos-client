/**
  ******************************************************************************
  * @file    stm32l462e_cell1_bus.c
  * @author  MCD Application Team
  * @brief   source file for the BSP BUS IO driver
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32l462e_cell1_bus.h"
#if (USE_I2C1_SEMAPHORE == 1)
#include "rtosal.h"
#endif /* (USE_I2C1_SEMAPHORE == 1) */

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 0)
#include "i2c.h"
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 0)
#include "spi.h"
#endif /* (USE_HAL_SPI_REGISTER_CALLBACKS == 1) */

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

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
I2C_HandleTypeDef hi2c1;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
SPI_HandleTypeDef hspi3;
#endif /* (USE_HAL_SPI_REGISTER_CALLBACKS == 1) */

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


static int32_t I2C1_Configuration(void);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
static int32_t BSP_I2C1_RegisterDefaultMspCallbacks(I2C_HandleTypeDef *handleI2C);
static void BSP_I2C1_MspInit(I2C_HandleTypeDef *hI2c);
static void BSP_I2C1_MspDeInit(I2C_HandleTypeDef *hI2c);
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

static int32_t SPI3_Configuration(void);
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
static int32_t BSP_SPI3_RegisterDefaultMspCallbacks(SPI_HandleTypeDef *handleSPI);
static void BSP_SPI3_MspInit(SPI_HandleTypeDef *hspi);
static void BSP_SPI3_MspDeInit(SPI_HandleTypeDef *hspi);
#endif /* (USE_HAL_SPI_REGISTER_CALLBACKS == 1) */

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
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
      if (BSP_I2C1_RegisterDefaultMspCallbacks(&hi2c1) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_MSP_FAILURE;
      }
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

      /* Init the I2C */
      if (I2C1_Configuration() != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_BUS_FAILURE;
      }
      else if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
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
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
    if (BSP_SPI3_RegisterDefaultMspCallbacks(&hspi3) != BSP_ERROR_NONE)
    {
      return BSP_ERROR_MSP_FAILURE;
    }
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

    /* Init the SPI */
    ret = SPI3_Configuration();
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

  /* DeInit the SPI*/
  if (HAL_SPI_DeInit(&hspi3) != HAL_OK)
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

  if (HAL_SPI_Transmit(&hspi3, pData, Length, BUS_SPI3_TIMEOUT) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
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
  int32_t ret = BSP_ERROR_NONE;
  uint32_t tx_data = 0xFFFFFFFFU;

  if (HAL_SPI_TransmitReceive(&hspi3, (uint8_t *)&tx_data, pData, Length, BUS_SPI3_TIMEOUT) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
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
  int32_t ret = BSP_ERROR_NONE;

  if (HAL_SPI_TransmitReceive(&hspi3, pTxData, pRxData, Length, BUS_SPI3_TIMEOUT) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }

  return ret;
}

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_BUS_Private_Functions STM32L462E_CELL1 BUS Private Functions
  * @{
  */
static int32_t I2C1_Configuration(void)
{
  int32_t ret;

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  /** Configure Analogue filter
    */
  else if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  /** Configure Digital filter
    */
  else if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return (ret);
}

#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
static int32_t BSP_I2C1_RegisterDefaultMspCallbacks(I2C_HandleTypeDef *handleI2C)
{
  static uint32_t IsI2c1MspCbValid = 0U;
  int32_t ret = BSP_ERROR_NONE;

  if (IsI2c1MspCbValid == 0U)
  {
    __HAL_I2C_RESET_HANDLE_STATE(handleI2C);

    /* Register default MspInit/MspDeInit Callback */
    if (HAL_I2C_RegisterCallback(handleI2C, HAL_I2C_MSPINIT_CB_ID, BSP_I2C1_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if (HAL_I2C_RegisterCallback(handleI2C, HAL_I2C_MSPDEINIT_CB_ID, BSP_I2C1_MspDeInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsI2c1MspCbValid = 1U;
    }
  }

  /* BSP status */
  return ret;
}

static void BSP_I2C1_MspInit(I2C_HandleTypeDef *hI2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  /**I2C1 GPIO Configuration
  PB8     ------> I2C1_SCL

  PB9     ------> I2C1_SDA
    */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* I2C1 clock enable */
  __HAL_RCC_I2C1_CLK_ENABLE();
}

static void BSP_I2C1_MspDeInit(I2C_HandleTypeDef *hI2c)
{
  /* Peripheral clock disable */
  __HAL_RCC_I2C1_CLK_DISABLE();

  /**I2C1 GPIO Configuration
  PB8     ------> I2C1_SCL
  PB9     ------> I2C1_SDA
    */
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);
}
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

static int32_t SPI3_Configuration(void)
{
  int32_t ret;

  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    ret = BSP_ERROR_BUS_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  return (ret);
}

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
static int32_t BSP_SPI3_RegisterDefaultMspCallbacks(SPI_HandleTypeDef *handleSPI)
{
  static uint32_t IsSpi3MspCbValid = 0U;

  if (IsSpi3MspCbValid == 0U)
  {
    __HAL_SPI_RESET_HANDLE_STATE(handleSPI);

    /* Register MspInit Callback */
    if (HAL_SPI_RegisterCallback(handleSPI, HAL_SPI_MSPINIT_CB_ID, BSP_SPI3_MspInit) != HAL_OK)
    {
      return BSP_ERROR_PERIPH_FAILURE;
    }

    /* Register MspDeInit Callback */
    if (HAL_SPI_RegisterCallback(handleSPI, HAL_SPI_MSPDEINIT_CB_ID, BSP_SPI3_MspDeInit) != HAL_OK)
    {
      return BSP_ERROR_PERIPH_FAILURE;
    }

    IsSpi3MspCbValid = 1U;
  }
  return BSP_ERROR_NONE;
}

static void BSP_SPI3_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* SPI3 clock enable */
  SPI3_CLOCK_ENABLE();

  SPI3_GPIO_CLOCK_ENABLE();
  SPI3_NSS_CLOCK_ENABLE();
  /**SPI3 GPIO Configuration
  PC12     ------> SPI3_MOSI
  PC11     ------> SPI3_MISO
  PC10     ------> SPI3_SCK
  PA4     ------> SPI3_NSS
    */
  GPIO_InitStruct.Pin = SPI3_MOSI_PIN | SPI3_MISO_PIN | SPI3_SCK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = SPI3_ALT_FUNCTION;
  HAL_GPIO_Init(SPI3_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPI3_NSS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = SPI3_ALT_FUNCTION;
  HAL_GPIO_Init(SPI3_NSS_PORT, &GPIO_InitStruct);
}

static void BSP_SPI3_MspDeInit(SPI_HandleTypeDef *hspi)
{
  /* Peripheral clock disable */
  SPI3_GPIO_CLOCK_DISABLE();

  /**SPI3 GPIO Configuration
  PC12     ------> SPI3_MOSI
  PC11     ------> SPI3_MISO
  PC10     ------> SPI3_SCK
  PA4     ------> SPI3_NSS
    */
  HAL_GPIO_DeInit(SPI3_PORT, SPI3_MOSI_PIN | SPI3_MOSI_PIN | SPI3_SCK_PIN);

  HAL_GPIO_DeInit(SPI3_NSS_PORT, SPI3_NSS_PIN);
}
#endif /* (USE_HAL_SPI_REGISTER_CALLBACKS == 1) */

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
