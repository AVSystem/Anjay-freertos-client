/*
 * Copyright 2020-2024 AVSystem <avsystem@avsystem.com>
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
 * @file    board_buttons.c
 * @author  MCD Application Team
 * @brief   Implements functions for user buttons actions
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

#include <stdbool.h>
#include <stdint.h>

#include <cmsis_os.h>

#include <dc_common.h>
#include <error_handler.h>

#include "board_buttons.h"
#include "joystick_object.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define BOARD_BUTTONS_DEBOUNCE_TIMEOUT (200U) /* in millisec */
#define BOARD_BUTTONS_MSG_QUEUE_SIZE (uint32_t)(8)

/* Private variables ---------------------------------------------------------*/
static osMessageQId board_buttons_msg_queue;
static board_buttons_t board_buttons_sel;

static osTimerId DebounceTimerHandle;
static __IO uint8_t debounce_ongoing = 0U;

static bool sel_pressed;

/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t DC_BOARD_BUTTONS_SEL = DC_COM_INVALID_ENTRY;

/* Private function prototypes -----------------------------------------------*/
static void board_buttons_post_event_debounce(dc_com_event_id_t event_id);
static void board_buttons_debounce_timer_callback(void const *argument);
static void board_buttons_thread(void const *argument);

/* Functions Definition ------------------------------------------------------*/

/**
 * @brief  board_button thread
 * @param  argument (UNUSED)
 * @retval none
 */
void board_buttons_thread(void const *argument) {
    osEvent event;
    dc_com_event_id_t event_id;

    for (;;) {
        event = osMessageGet(board_buttons_msg_queue, osWaitForever);
        if (event.status == osEventMessage) {
            event_id = (dc_com_event_id_t) event.value.v;
            (void) dc_com_write_event(&dc_com_db, event_id);
        }
    }
}

static void buttons_callback(dc_com_event_id_t dc_event_id,
                             const void *user_arg) {
    (void) user_arg;

    if (dc_event_id == DC_BOARD_BUTTONS_SEL && sel_pressed) {
        joystick_object_increment_counter();
    }
}

/**
 * @brief  debounce timer management
 * @param  event_id            event id
 * @retval board_buttons_status_t    return status
 */
static void board_buttons_debounce_timer_callback(void const *argument) {
    (void) argument;
    debounce_ongoing = 0U;
}

/**
 * @brief  debounce event management
 * @param  event_id            event id
 * @retval board_buttons_status_t    return status
 */
static void board_buttons_post_event_debounce(dc_com_event_id_t event_id) {
    /* post event to the queue only if no ongoing debounce
     * limitation: all events with debounce are sharing same timer
     */
    if (debounce_ongoing == 0U) {
        if (osMessagePut(board_buttons_msg_queue, (uint32_t) event_id, 0U)
                != osOK) {
            ERROR_Handler(DBG_CHAN_APPLICATION, 7, ERROR_WARNING);
        } else {
            (void) osTimerStart(DebounceTimerHandle,
                                BOARD_BUTTONS_DEBOUNCE_TIMEOUT);
            debounce_ongoing = 1U;
        }
    }
}

void board_button_joy_sel_press(void) {
    sel_pressed = true;
    board_buttons_post_event_debounce(DC_BOARD_BUTTONS_SEL);
}

void board_button_joy_sel_release(void) {
    sel_pressed = false;
    board_buttons_post_event_debounce(DC_BOARD_BUTTONS_SEL);
}

/**
 * @brief  component initialisation
 * @param  -
 * @retval board_buttons_status_t     return status
 */
board_buttons_status_t board_buttons_init(void) {
    /* definition and creation of DebounceTimer */
    osTimerDef(DebounceTimer, board_buttons_debounce_timer_callback);
    DebounceTimerHandle =
            osTimerCreate(osTimer(DebounceTimer), osTimerOnce, NULL);

    /* queue creation */
    osMessageQDef(BOARD_BUTTONS_MSG_QUEUE, BOARD_BUTTONS_MSG_QUEUE_SIZE,
                  uint32_t);
    board_buttons_msg_queue =
            osMessageCreate(osMessageQ(BOARD_BUTTONS_MSG_QUEUE), NULL);

    DC_BOARD_BUTTONS_SEL =
            dc_com_register_serv(&dc_com_db, (void *) &board_buttons_sel,
                                 (uint16_t) sizeof(board_buttons_t));

    dc_com_register_gen_event_cb(&dc_com_db, buttons_callback, NULL);

    return BOARD_BUTTONS_OK;
}

static uint32_t buttons_thread_stack_buffer[BOARD_BUTTONS_THREAD_STACK_SIZE];
static osStaticThreadDef_t buttons_thread_controlblock;
/**
 * @brief  component start
 * @param  -
 * @retval board_buttons_status_t     return status
 */
board_buttons_status_t board_buttons_start(void) {
    static osThreadId board_buttons_thread_id;

    /* definition and creation of dc_CtrlEventTask */
    osThreadStaticDef(dc_CtrlTask, board_buttons_thread,
                      BOARD_BUTTONS_THREAD_PRIO, 0,
                      BOARD_BUTTONS_THREAD_STACK_SIZE,
                      buttons_thread_stack_buffer,
                      &buttons_thread_controlblock);
    board_buttons_thread_id = osThreadCreate(osThread(dc_CtrlTask), NULL);
    if (board_buttons_thread_id == NULL) {
        ERROR_Handler(DBG_CHAN_APPLICATION, 7, ERROR_WARNING);
    }

    if (DC_BOARD_BUTTONS_SEL != DC_COM_INVALID_ENTRY) {
        board_buttons_sel.rt_state = DC_SERVICE_ON;
    }

    return BOARD_BUTTONS_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
