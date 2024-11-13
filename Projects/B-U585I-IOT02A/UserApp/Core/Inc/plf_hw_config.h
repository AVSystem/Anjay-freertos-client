/**
  ******************************************************************************
  * @file    plf_hw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the hardware configuration of the platform
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_HW_CONFIG_H
#define PLF_HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* MISRAC messages linked to HAL include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32u5xx_hal.h"
#include "stm32u5xx.h"
/*cstat +MISRAC2012-* */

#include "main.h"
#include "plf_modem_config.h"
#include "usart.h"

/* Exported constants --------------------------------------------------------*/

/* Platform defines ----------------------------------------------------------*/

/* MODEM configuration */
#if defined(CONFIG_MODEM_USE_STMOD_CONNECTOR)
#define MODEM_UART_HANDLE                huart3
#define MODEM_UART_INSTANCE              ((USART_TypeDef *)USART3)
#define MODEM_UART_AUTOBAUD              (1)
#define MODEM_UART_IRQN                  USART3_IRQn
/* #define MODEM_UART_ALTERNATE             GPIO_AF7_USART1 */
#else
#error Modem connector not specified or invalid for this board
#endif /* defined(CONFIG_MODEM_USE_STMOD_CONNECTOR) */

/* UART interface */
#define MODEM_UART_BAUDRATE              (CONFIG_MODEM_UART_BAUDRATE)
#define MODEM_UART_WORDLENGTH            UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS              UART_STOPBITS_1
#define MODEM_UART_PARITY                UART_PARITY_NONE
#define MODEM_UART_MODE                  UART_MODE_TX_RX

#if (CONFIG_MODEM_UART_RTS_CTS == 1)
#define MODEM_UART_HWFLOWCTRL            UART_HWCONTROL_RTS_CTS
#else
#define MODEM_UART_HWFLOWCTRL            UART_HWCONTROL_NONE
#endif /* (CONFIG_MODEM_UART_RTS_CTS == 1) */

#define MODEM_TX_GPIO_PORT               ((GPIO_TypeDef *)MODEM_UART_TX_GPIO_Port)   /* for DiscoL496: GPIOB       */
#define MODEM_TX_PIN                     MODEM_UART_TX_Pin                           /* for DiscoL496: GPIO_PIN_6  */
#define MODEM_RX_GPIO_PORT               ((GPIO_TypeDef *)MODEM_UART_RX_GPIO_Port)    /* for DiscoL496: GPIOG       */
#define MODEM_RX_PIN                     MODEM_UART_RX_Pin                            /* for DiscoL496: GPIO_PIN_10 */
#define MODEM_CTS_GPIO_PORT              ((GPIO_TypeDef *)MODEM_UART_CTS_GPIO_Port)   /* for DiscoL496: GPIOG       */
#define MODEM_CTS_PIN                    MODEM_UART_CTS_Pin                           /* for DiscoL496: GPIO_PIN_11 */
#define MODEM_RTS_GPIO_PORT              ((GPIO_TypeDef *)MODEM_UART_RTS_GPIO_Port)   /* for DiscoL496: GPIOG       */
#define MODEM_RTS_PIN                    MODEM_UART_RTS_Pin                           /* for DiscoL496: GPIO_PIN_12 */

/* ---- MODEM other pins configuration ---- */
#if defined(CONFIG_MODEM_USE_STMOD_CONNECTOR)
/* output */
#define MODEM_RST_GPIO_PORT              ((GPIO_TypeDef *)MODEM_RST_GPIO_Port)    /* for DiscoL496: GPIOB      */
#define MODEM_RST_PIN                    MODEM_RST_Pin                            /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_PWR_EN_GPIO_PORT           ((GPIO_TypeDef *)MODEM_PWR_EN_GPIO_Port) /* for DiscoL496: GPIOD      */
#define MODEM_PWR_EN_PIN                 MODEM_PWR_EN_Pin                         /* for DiscoL496: GPIO_PIN_3 */
#define MODEM_DTR_GPIO_PORT              ((GPIO_TypeDef *)MODEM_DTR_GPIO_Port)    /* for DiscoL496: GPIOA      */
#define MODEM_DTR_PIN                    MODEM_DTR_Pin                            /* for DiscoL496: GPIO_PIN_0 */
/* input */
#define MODEM_RING_GPIO_PORT             ((GPIO_TypeDef *)MODEM_RING_GPIO_Port)  /* for DiscoL496: GPIOH      */
#define MODEM_RING_PIN                   MODEM_RING_Pin                          /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_RING_IRQN                  EXTI6_IRQn
#else
#error Modem connector not specified or invalid for this board
#endif /* defined(CONFIG_MODEM_USE_STMOD_CONNECTOR) */

/* ---- MODEM SIM SELECTION pins ---- */
#define MODEM_SIM_SELECT_0_GPIO_PORT     MODEM_SIM_SELECT_0_GPIO_Port
#define MODEM_SIM_SELECT_0_PIN           MODEM_SIM_SELECT_0_Pin
#define MODEM_SIM_SELECT_1_GPIO_PORT     MODEM_SIM_SELECT_1_GPIO_Port
#define MODEM_SIM_SELECT_1_PIN           MODEM_SIM_SELECT_1_Pin

/* DEBUG INTERFACE CONFIGURATION */
#define TRACE_INTERFACE_UART_HANDLE      huart1
#define TRACE_INTERFACE_INSTANCE         ((USART_TypeDef *)USART1)

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_HW_CONFIG_H */
