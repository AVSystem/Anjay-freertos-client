/**
  ******************************************************************************
  * @file    stm32l462e_cell1.h
  * @author  MCD Application Team
  * @brief   STM32L462E_CELL1 board support package header file.
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
#ifndef __STM32L462E_CELL1_H
#define __STM32L462E_CELL1_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l462e_cell1_conf.h"
#include "stm32l462e_cell1_errno.h"

/** @addtogroup BSP BSP
  * @{
  */

/** @addtogroup STM32L462E_CELL1 STM32L462E_CELL1
  * @{
  */

/** @addtogroup STM32L462E_CELL1_COMMON STM32L462E_CELL1 COMMON
  * @{
  */

/** @defgroup STM32L462E_CELL1_COMMON_Exported_Types STM32L462E_CELL1 COMMON Exported Types
  * @{
  */
typedef enum
{
  BSP_LED1 = 0x0,
  BSP_LED2 = 0x1,
  BSP_LED3 = 0x2,
  BSP_LED_NB,
  BSP_LED_GREEN = BSP_LED1,
  BSP_LED_RED   = BSP_LED2,
  BSP_LED_BLUE  = BSP_LED3,
} Led_TypeDef;


typedef enum
{
  BUTTON_USER = 0
} Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;


/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_COMMON_Exported_Constants STM32L462E_CELL1 COMMON Exported Constants
  * @{
  */

/**
  * @brief STM32L475E IOT01 BSP Driver version number
   */
#define __STM32L462E_CELL1_BSP_VERSION_MAIN   (0x02U) /*!< [31:24] main version */
#define __STM32L462E_CELL1_BSP_VERSION_SUB1   (0x00U) /*!< [23:16] sub1 version */
#define __STM32L462E_CELL1_BSP_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define __STM32L462E_CELL1_BSP_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define __STM32L462E_CELL1_BSP_VERSION        ((__STM32L462E_CELL1_BSP_VERSION_MAIN << 24)\
                                               |(__STM32L462E_CELL1_BSP_VERSION_SUB1 << 16)\
                                               |(__STM32L462E_CELL1_BSP_VERSION_SUB2 << 8 )\
                                               |(__STM32L462E_CELL1_BSP_VERSION_RC))

/**
  * @brief  Define for STM32L462E_CELL1 board
  */
#if !defined (USE_STM32L462E_CELL1)
#define USE_STM32L462E_CELL1
#endif /* !defined (USE_STM32L462E_CELL1) */

#define LED_NB                           ((uint8_t)3U)

#define LED1_PIN                         GPIO_PIN_6
#define LED1_GPIO_PORT                   GPIOC
#define LED1_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOC_CLK_ENABLE()
#define LED1_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOC_CLK_DISABLE()

#define LED2_PIN                         GPIO_PIN_15
#define LED2_GPIO_PORT                   GPIOB
#define LED2_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED2_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()

#define LED3_PIN                         GPIO_PIN_14
#define LED3_GPIO_PORT                   GPIOB
#define LED3_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED3_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()


#define LEDx_GPIO_CLK_ENABLE(__INDEX__)   do{ if((__INDEX__) == BSP_LED1) { LED1_GPIO_CLK_ENABLE();   } else \
                                              if((__INDEX__) == BSP_LED2) { LED2_GPIO_CLK_ENABLE();   } else \
                                              if((__INDEX__) == BSP_LED3) { LED3_GPIO_CLK_ENABLE(); } } while(0)

#define LEDx_GPIO_CLK_DISABLE(__INDEX__)  do{ if((__INDEX__) == BSP_LED1) { LED1_GPIO_CLK_DISABLE();   } else \
                                                if((__INDEX__) == BSP_LED2) { LED2_GPIO_CLK_DISABLE();   } else \
                                                if((__INDEX__) == BSP_LED3) { LED3_GPIO_CLK_DISABLE(); } } while(0)

/* Only one User/Wakeup button */
#define BUTTON_NB                            ((uint8_t)1)

/**
  * @brief Wakeup push-button
  */
#define BSP_USER_BUTTON_PIN                   GPIO_PIN_13
#define BSP_USER_BUTTON_GPIO_PORT             GPIOC
#define BSP_USER_BUTTON_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOC_CLK_ENABLE()
#define BSP_USER_BUTTON_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOC_CLK_DISABLE()
#define BSP_USER_BUTTON_EXTI_IRQ_N            EXTI15_10_IRQn

#define LCD_D_C_DISP_PIN                GPIO_PIN_11
#define LCD_D_C_DISP_GPIO_PORT          GPIOC
#define LCD_RST_DISP_PIN                GPIO_PIN_1
#define LCD_RST_DISP_GPIO_PORT          GPIOH
#define LCD_CS_DISP_PIN                 GPIO_PIN_2
#define LCD_CS_DISP_GPIO_PORT           GPIOB

#define SPI3_MOSI_PIN     GPIO_PIN_12
#define SPI3_MISO_PIN     GPIO_PIN_11
#define SPI3_PORT         GPIOC
#define SPI3_SCK_PIN      GPIO_PIN_10
#define SPI3_ALT_FUNCTION GPIO_AF6_SPI3
#define SPI3_GPIO_CLOCK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define SPI3_GPIO_CLOCK_DISABLE()     __HAL_RCC_GPIOC_CLK_DISABLE()
#define SPI3_CLOCK_ENABLE()           __HAL_RCC_SPI3_CLK_ENABLE()
#define SPI3_CLOCK_DISABLE()          __HAL_RCC_SPI3_CLK_DISABLE()
#define SPI3_FORCE_RESET()            __HAL_RCC_SPI3_FORCE_RESET()
#define SPI3_FORCE_RELEASE_RESET()    __HAL_RCC_SPI3_RELEASE_RESET()

/* LCD and ST33 share same SPI */
/* LCD configuration */
#define LCD_SPI_HANDLE                  hspi3
#define LCD_SPI_INSTANCE                SPI3
#define LCD_SPI_BAUDRATEPRESCALER       SPI_BAUDRATEPRESCALER_4
#define LCD_SPI_MOSI_PIN                SPI3_MOSI_PIN
#define LCD_SPI_MISO_PIN                0
#define LCD_SPI_PORT                    SPI3_PORT
#define LCD_SPI_CS_PIN                  GPIO_PIN_2
#define LCD_SPI_CS_PORT                 GPIOB
#define LCD_SPI_ALT_FUNCTION            SPI3_ALT_FUNCTION
#define LCD_SPI_SCK_PIN                 SPI3_SCK_PIN
#define LCD_SPI_GPIO_CLOCK_ENABLE()     SPI3_GPIO_CLOCK_ENABLE()
#define LCD_SPI_GPIO_CLOCK_DISABLE()    SPI3_GPIO_CLOCK_DISABLE()
#define LCD_SPI_CLOCK_ENABLE()          SPI3_CLOCK_ENABLE()
#define LCD_SPI_CLOCK_DISABLE()         SPI3_CLOCK_DISABLE()
#define LCD_SPI_FORCE_RESET()           SPI3_FORCE_RESET()
#define LCD_SPI_FORCE_RELEASE_RESET()   SPI3_FORCE_RELEASE_RESET()

/**
  * @}
  */

/** @defgroup STM32L462E_CELL1_COMMON_Exported_Functions STM32L462E_CELL1 COMMON Exported Functions
  * @{
  */
int32_t          BSP_GetVersion(void);
int32_t          BSP_GetTick(void);
void             BSP_LED_Init(Led_TypeDef Led);
void             BSP_LED_DeInit(Led_TypeDef Led);
void             BSP_LED_On(Led_TypeDef Led);
void             BSP_LED_Off(Led_TypeDef Led);
void             BSP_LED_Toggle(Led_TypeDef Led);
void             BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
void             BSP_PB_DeInit(Button_TypeDef Button);
uint32_t         BSP_PB_GetState(Button_TypeDef Button);

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

#endif /* __STM32L462E_CELL1_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
