/*
 * Copyright 2020 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 ******************************************************************************
 * @file    board_interrupts.c
 * @author  MCD Application Team
 * @brief   Implements HAL weak functions for Interrupts
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
#include "plf_config.h"

#include "board_buttons.h"
#include "ipc_uart.h"

#include "at_modem_api.h"

/* NOTE : this code is designed for FreeRTOS */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == MODEM_RING_PIN) {
        GPIO_PinState gstate =
                HAL_GPIO_ReadPin(MODEM_RING_GPIO_PORT, MODEM_RING_PIN);
        atcc_hw_event(DEVTYPE_MODEM_CELLULAR, HWEVT_MODEM_RING, gstate);
    } else if (GPIO_Pin == JOY_SEL_Pin) {
        GPIO_PinState state = HAL_GPIO_ReadPin(JOY_SEL_GPIO_Port, JOY_SEL_Pin);
        if (state) {
            board_button_joy_sel_press();
        } else {
            board_button_joy_sel_release();
        }

    } else {
        /* nothing to do */
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == MODEM_UART_INSTANCE) {
        IPC_UART_RxCpltCallback(huart);
    } else {
        /* Nothing to do */
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == MODEM_UART_INSTANCE) {
        IPC_UART_TxCpltCallback(huart);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == MODEM_UART_INSTANCE) {
        IPC_UART_ErrorCallback(huart);
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
