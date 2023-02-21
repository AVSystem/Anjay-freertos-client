/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32l462e_cell1_motion_sensors.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef MOTION_SENSOR_Axes_t bsp_axes_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define BSP_MODEL_NUMBER "B_L462E_CELL1"

#define BSP_BUTTON      USER_BUTTON_Pin
#define BSP_BUTTON_PORT USER_BUTTON_GPIO_Port

#define BSP_HEARTBEAT_LED      LED1_Pin
#define BSP_HEARTBEAT_LED_PORT LED1_GPIO_Port

#define BSP_AXIS_X x
#define BSP_AXIS_Y y
#define BSP_AXIS_Z z
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ARD_D10_Pin GPIO_PIN_15
#define ARD_D10_GPIO_Port GPIOA
#define ST33_MOSI_Pin GPIO_PIN_12
#define ST33_MOSI_GPIO_Port GPIOC
#define ARD_D12_Pin GPIO_PIN_4
#define ARD_D12_GPIO_Port GPIOB
#define MDM_PWR_EN_OUT_Pin GPIO_PIN_7
#define MDM_PWR_EN_OUT_GPIO_Port GPIOB
#define ARD_D15_Pin GPIO_PIN_8
#define ARD_D15_GPIO_Port GPIOB
#define ST33_MISO_Pin GPIO_PIN_11
#define ST33_MISO_GPIO_Port GPIOC
#define MDM_RST_OUT_Pin GPIO_PIN_6
#define MDM_RST_OUT_GPIO_Port GPIOB
#define USER_BUTTON_Pin GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define USER_BUTTON_EXTI_IRQn EXTI15_10_IRQn
#define ARD_D2_Pin GPIO_PIN_10
#define ARD_D2_GPIO_Port GPIOA
#define MDM_UART_RTS_Pin GPIO_PIN_2
#define MDM_UART_RTS_GPIO_Port GPIOD
#define ARD_D11_Pin GPIO_PIN_5
#define ARD_D11_GPIO_Port GPIOB
#define ARD_D14_Pin GPIO_PIN_9
#define ARD_D14_GPIO_Port GPIOB
#define ARD_D8_Pin GPIO_PIN_9
#define ARD_D8_GPIO_Port GPIOA
#define ST33_SCK_Pin GPIO_PIN_10
#define ST33_SCK_GPIO_Port GPIOC
#define ARD_D7_Pin GPIO_PIN_8
#define ARD_D7_GPIO_Port GPIOA
#define MDM_UART_TX_Pin GPIO_PIN_4
#define MDM_UART_TX_GPIO_Port GPIOC
#define ARD_A1_Pin GPIO_PIN_1
#define ARD_A1_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_15
#define LED2_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_6
#define LED1_GPIO_Port GPIOC
#define MDM_DTR_OUT_Pin GPIO_PIN_8
#define MDM_DTR_OUT_GPIO_Port GPIOC
#define ARD_D13_Pin GPIO_PIN_5
#define ARD_D13_GPIO_Port GPIOA
#define ARD_D0_Pin GPIO_PIN_3
#define ARD_D0_GPIO_Port GPIOA
#define LED3_Pin GPIO_PIN_14
#define LED3_GPIO_Port GPIOB
#define MDM_UART_CTS_Pin GPIO_PIN_13
#define MDM_UART_CTS_GPIO_Port GPIOB
#define MDM_RING_Pin GPIO_PIN_12
#define MDM_RING_GPIO_Port GPIOB
#define MDM_RING_EXTI_IRQn EXTI15_10_IRQn
#define ST33_VCC_CTRL_Pin GPIO_PIN_2
#define ST33_VCC_CTRL_GPIO_Port GPIOB
#define MDM_UART_RX_Pin GPIO_PIN_5
#define MDM_UART_RX_GPIO_Port GPIOC
#define ST33_CS_Pin GPIO_PIN_4
#define ST33_CS_GPIO_Port GPIOA
#define ARD_D1_Pin GPIO_PIN_2
#define ARD_D1_GPIO_Port GPIOA
#define ARD_A0_Pin GPIO_PIN_0
#define ARD_A0_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
