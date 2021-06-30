/**
  ******************************************************************************
  * @file    stm32l462e_cell1.c
  * @author  MCD Application Team
  * @brief   STM32L462E_CELL1 board support package source file.
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
#include "stm32l462e_cell1.h"
#include "ssd1315.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_COMMON STM32L462E_CELL1 COMMON
  * @{
  */

/** @defgroup STM32L462E_CELL1_COMMON_Private_Variables STM32L462E_CELL1 COMMON Private Variables
  * @{
  */

static const uint16_t GPIO_PIN[BSP_LED_NB] = {LED1_PIN, LED2_PIN, LED3_PIN};
static GPIO_TypeDef *GPIO_PORT[BSP_LED_NB] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT};
static GPIO_TypeDef *BUTTON_PORT[BUTTON_NB] = {BSP_USER_BUTTON_GPIO_PORT};
static const uint16_t BUTTON_PIN[BUTTON_NB] = {BSP_USER_BUTTON_PIN};
static const uint16_t BUTTON_IRQn[BUTTON_NB] = {BSP_USER_BUTTON_EXTI_IRQ_N};

extern SPI_HandleTypeDef LCD_SPI_HANDLE;

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_COMMON_Exported_Functions STM32L462E_CELL1 COMMON Exported Functions
  * @{
  */

/**
  * @brief  This method returns the STM32L475E IOT01 BSP Driver revision
  * @retval version: 0xXYZR (8bits for each decimal, R for RC)
  */
int32_t BSP_GetVersion(void)
{
  return ((int32_t)__STM32L462E_CELL1_BSP_VERSION);
}

/**
  * @brief  Return system tick in ms
  * @retval Current HAL time base time stamp
  */
int32_t BSP_GetTick(void)
{
  return ((int32_t)HAL_GetTick());
}

/**
  * @brief  Configures LEDs.
  * @param  Led: LED to be configured.
  *         This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
  */
void BSP_LED_Init(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpio_init_structure;

  LEDx_GPIO_CLK_ENABLE(Led);
  /* Configure the GPIO_LED pin */
  gpio_init_structure.Pin   = GPIO_PIN[Led];
  gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull  = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;

  HAL_GPIO_Init(GPIO_PORT[Led], &gpio_init_structure);
  /* By default, turn off LED */
  HAL_GPIO_WritePin(
    GPIO_PORT[Led],
    (uint16_t)gpio_init_structure.Pin,
    GPIO_PIN_RESET);
}

/**
  * @brief  DeInit LEDs.
  * @param  Led: LED to be configured.
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
  */
void BSP_LED_DeInit(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpio_init_structure;

  /* DeInit the GPIO_LED pin */
  gpio_init_structure.Pin = GPIO_PIN[Led];

  /* Turn off LED */
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
  HAL_GPIO_DeInit(GPIO_PORT[Led], gpio_init_structure.Pin);
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: LED to be set on
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
   */
void BSP_LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: LED to be set off
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
   */
void BSP_LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: LED to be toggled
  *          This parameter can be one of the following values:
  *     @arg LED1
  *     @arg LED2
  *     @arg LED3
  * @retval None
   */
void BSP_LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

/**
  * @brief  Configures button GPIO and EXTI Line.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  * @param  ButtonMode: Button mode
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_MODE_GPIO: Button will be used as simple IO
  *            @arg  BUTTON_MODE_EXTI: Button will be connected to EXTI line
  *                                    with interrupt generation capability
  * @retval None
  */
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode)
{
  GPIO_InitTypeDef gpio_init_structure;

  BSP_USER_BUTTON_GPIO_CLK_ENABLE();

  if (ButtonMode == BUTTON_MODE_GPIO)
  {
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);
  }

  if (ButtonMode == BUTTON_MODE_EXTI)
  {
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio_init_structure.Mode = GPIO_MODE_IT_RISING_FALLING;
    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);

    HAL_NVIC_SetPriority((IRQn_Type)(BUTTON_IRQn[Button]), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  }
}

/**
  * @brief  Push Button DeInit.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  * @note PB DeInit does not disable the GPIO clock
  * @retval None
  */
void BSP_PB_DeInit(Button_TypeDef Button)
{
  GPIO_InitTypeDef gpio_init_structure;

  gpio_init_structure.Pin = BUTTON_PIN[Button];
  HAL_NVIC_DisableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  HAL_GPIO_DeInit(BUTTON_PORT[Button], gpio_init_structure.Pin);
}

/**
  * @brief  Returns the selected button state.
  * @param  Button: Button to be checked
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  * @retval The Button GPIO pin value (GPIO_PIN_RESET = button pressed)
  */
uint32_t BSP_PB_GetState(Button_TypeDef Button)
{
  return HAL_GPIO_ReadPin((GPIO_TypeDef *)BUTTON_PORT[Button], (uint16_t)BUTTON_PIN[Button]);
}

/*******************************************************************************
                            LINK OPERATIONS
  *******************************************************************************/

/******************************** LINK LCD ********************************/
/**
  * @brief  Initializes lcd low level.
  * @retval None
  */
void LCD_IO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RST_DISP_GPIO_PORT, LCD_RST_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);

  /* Configure GPIO pin : LCD_D_C_DISP_PIN */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = LCD_D_C_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_D_C_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_RESET);

  /* Configure GPIO pin : LCD_RST_DISP_PIN */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  GPIO_InitStruct.Pin = LCD_RST_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */          /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */ /* Already done in previous line */
  HAL_GPIO_Init(LCD_RST_DISP_GPIO_PORT, &GPIO_InitStruct);

  /* Configure GPIO pin : LCD_CS_DISP_PIN */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitStruct.Pin = LCD_CS_DISP_PIN;
  /* GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; */  /* Already done in previous line */
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */          /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */ /* Already done in previous line */
  HAL_GPIO_Init(LCD_CS_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_RESET);

  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_RST_DISP_GPIO_PORT, LCD_RST_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_RST_DISP_GPIO_PORT, LCD_RST_DISP_PIN, GPIO_PIN_SET);

  (void) BSP_SPI3_Init();

  GPIO_InitStruct.Pin = LCD_D_C_DISP_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  /* GPIO_InitStruct.Pull = GPIO_NOPULL; */  /* Already done in previous line */
  /* GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; */  /* Already done in previous line */
  HAL_GPIO_Init(LCD_D_C_DISP_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_RESET);
}

void LCD_IO_DeInit(void)
{
  if (HAL_SPI_GetState(&LCD_SPI_HANDLE) != HAL_SPI_STATE_RESET)
  {
    /* SPI Deinit */
    (void) BSP_SPI3_DeInit();

    HAL_GPIO_DeInit(GPIOC, (GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_10));
    __HAL_RCC_SPI3_FORCE_RESET();
    __HAL_RCC_SPI3_RELEASE_RESET();

    HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);

    /* Disable SPIx clock  */
    __HAL_RCC_GPIOC_CLK_DISABLE();
  }
}

void LCD_IO_WriteCommand(uint8_t Cmd)
{
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);

  (void) BSP_SPI3_Send(&Cmd, 1U);

  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

void LCD_IO_WriteData(uint8_t Value)
{
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);

  (void) BSP_SPI3_Send(&Value, 1U);;

  HAL_Delay(1);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size)
{
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_RESET);
  HAL_Delay(1U);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1U);

  (void) BSP_SPI3_Send(pData, (uint16_t)Size);

  HAL_Delay(1U);
  HAL_GPIO_WritePin(LCD_D_C_DISP_GPIO_PORT, LCD_D_C_DISP_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_DISP_GPIO_PORT, LCD_CS_DISP_PIN, GPIO_PIN_SET);
  HAL_Delay(1U);
}

void LCD_Delay(uint32_t delay)
{
  HAL_Delay(delay);
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
