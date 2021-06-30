/*
 * Copyright 2020-2021 AVSystem <avsystem@avsystem.com>
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

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32l4xx_hal.h"

#include "console.h"

#define CHAR_TAB 0x09
#define CHAR_ESC 0x1b
#define CHAR_DEL 0x7f

extern UART_HandleTypeDef *const g_console_huart;

static void console_ignore_characters(void) {
    uint8_t ch;
    const int ignore_timeout_ms = 1;
    // Receive remaining characters to empty the RX buffer
    while (HAL_UART_Receive(g_console_huart, &ch, sizeof(uint8_t),
                            ignore_timeout_ms)
           == HAL_OK)
        ;
}

static char console_read_character(void) {
    char ch = '\r';
    while (HAL_UART_Receive(g_console_huart, (uint8_t *) &ch, sizeof(char),
                            HAL_MAX_DELAY)
           == HAL_OK) {
        // Ignore escape sequences and tabs
        if (ch == CHAR_ESC) {
            console_ignore_characters();
        } else if (ch != CHAR_TAB) {
            break;
        }
    }
    return ch;
}

void console_read_line(char *buffer, size_t size) {
    char ch;
    size_t i = 0;
    while ((ch = console_read_character()) != '\r') {
        if (ch == CHAR_DEL) {
            if (i > 0) {
                buffer[--i] = '\0';
                console_write("\b \b");
            }
        } else if (i < size - 1) {
            buffer[i++] = ch;
            console_write("%c", ch);
        }
    }
    buffer[i] = '\0';
    console_write("\r\n");
}

void console_write(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    HAL_UART_Transmit(g_console_huart, (uint8_t *) buffer, strlen(buffer),
                      HAL_MAX_DELAY);
    va_end(args);
}

bool console_wait_for_key_press(uint32_t timeout_ms) {
    uint8_t ch;
    if (HAL_UART_Receive(g_console_huart, &ch, sizeof(uint8_t), timeout_ms)
            == HAL_OK) {
        if (ch == CHAR_ESC) {
            console_ignore_characters();
        }
        return true;
    }
    return false;
}
