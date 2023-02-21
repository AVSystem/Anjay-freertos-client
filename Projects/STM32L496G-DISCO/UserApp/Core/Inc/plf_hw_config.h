/**
  ******************************************************************************
  * @file    plf_hw_config.h
  * @author  MCD Application Team
  * @brief   This file contains the hardware configuration of the platform
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PLF_HW_CONFIG_H
#define PLF_HW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* MISRAC messages linked to HAL include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l4xx_hal.h"
#include "stm32l4xx.h"
/*cstat +MISRAC2012-* */

#include "main.h"
#include "plf_modem_config.h"
#include "usart.h"

/* Exported constants --------------------------------------------------------*/

/* Platform defines ----------------------------------------------------------*/

/* MODEM configuration */
#if defined(CONFIG_MODEM_USE_STMOD_CONNECTOR)
#define MODEM_UART_HANDLE                huart1
#define MODEM_UART_INSTANCE              ((USART_TypeDef *)USART1)
#define MODEM_UART_AUTOBAUD              (1)
#define MODEM_UART_IRQN                  USART1_IRQn
#define MODEM_UART_ALTERNATE             GPIO_AF7_USART1
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

#define MODEM_TX_GPIO_PORT               ((GPIO_TypeDef *)USART1_TX_GPIO_Port)   /* for DiscoL496: GPIOB       */
#define MODEM_TX_PIN                     USART1_TX_Pin                           /* for DiscoL496: GPIO_PIN_6  */
#define MODEM_RX_GPIO_PORT               ((GPIO_TypeDef *)UART1_RX_GPIO_Port)    /* for DiscoL496: GPIOG       */
#define MODEM_RX_PIN                     UART1_RX_Pin                            /* for DiscoL496: GPIO_PIN_10 */
#define MODEM_CTS_GPIO_PORT              ((GPIO_TypeDef *)UART1_CTS_GPIO_Port)   /* for DiscoL496: GPIOG       */
#define MODEM_CTS_PIN                    UART1_CTS_Pin                           /* for DiscoL496: GPIO_PIN_11 */
#define MODEM_RTS_GPIO_PORT              ((GPIO_TypeDef *)UART1_RTS_GPIO_Port)   /* for DiscoL496: GPIOG       */
#define MODEM_RTS_PIN                    UART1_RTS_Pin                           /* for DiscoL496: GPIO_PIN_12 */

/* ---- MODEM other pins configuration ---- */
#if defined(CONFIG_MODEM_USE_STMOD_CONNECTOR)
/* output */
#define MODEM_RST_GPIO_PORT              ((GPIO_TypeDef *)MDM_RST_GPIO_Port)    /* for DiscoL496: GPIOB      */
#define MODEM_RST_PIN                    MDM_RST_Pin                            /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_PWR_EN_GPIO_PORT           ((GPIO_TypeDef *)MDM_PWR_EN_GPIO_Port) /* for DiscoL496: GPIOD      */
#define MODEM_PWR_EN_PIN                 MDM_PWR_EN_Pin                         /* for DiscoL496: GPIO_PIN_3 */
#define MODEM_DTR_GPIO_PORT              ((GPIO_TypeDef *)MDM_DTR_GPIO_Port)    /* for DiscoL496: GPIOA      */
#define MODEM_DTR_PIN                    MDM_DTR_Pin                            /* for DiscoL496: GPIO_PIN_0 */
/* input */
#define MODEM_RING_GPIO_PORT             ((GPIO_TypeDef *)STMOD_INT_GPIO_Port)  /* for DiscoL496: GPIOH      */
#define MODEM_RING_PIN                   STMOD_INT_Pin                          /* for DiscoL496: GPIO_PIN_2 */
#define MODEM_RING_IRQN                  EXTI2_IRQn
#else
#error Modem connector not specified or invalid for this board
#endif /* defined(CONFIG_MODEM_USE_STMOD_CONNECTOR) */

/* ---- MODEM SIM SELECTION pins ---- */
#define MODEM_SIM_SELECT_0_GPIO_PORT     MDM_SIM_SELECT_0_GPIO_Port
#define MODEM_SIM_SELECT_0_PIN           MDM_SIM_SELECT_0_Pin
#define MODEM_SIM_SELECT_1_GPIO_PORT     MDM_SIM_SELECT_1_GPIO_Port
#define MODEM_SIM_SELECT_1_PIN           MDM_SIM_SELECT_1_Pin

/* DEBUG INTERFACE CONFIGURATION */
#define TRACE_INTERFACE_UART_HANDLE      huart2
#define TRACE_INTERFACE_INSTANCE         ((USART_TypeDef *)USART2)

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* PLF_HW_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
