/*
 * Copyright 2020-2025 AVSystem <avsystem@avsystem.com>
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

#include <anjay/anjay.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

#include "console.h"

#define CHAR_BS 0x08
#define CHAR_DEL 0x7f
#define CHAR_ESC 0x1b

#define BUF_SIZE 256
#define BUF_CHUNK_SIZE 16

typedef struct {
    uint8_t buf[BUF_SIZE];
    size_t in;
    size_t out;
    size_t available;
} circ_buf_t;

int circ_buf_get(circ_buf_t *c_buf, uint8_t *val) {
    size_t next = (c_buf->out + 1) % BUF_SIZE;
    if (next == c_buf->in || c_buf->available == 0) {
        return -1;
    }

    *val = c_buf->buf[next];
    c_buf->out = next;
    c_buf->available--;

    return 0;
}

static circ_buf_t cb;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance != g_console_huart->Instance) {
        return;
    }
    cb.available += Size;
    cb.in += Size;
    cb.in %= BUF_SIZE;

    size_t space_left = AVS_MIN(BUF_SIZE - cb.in, BUF_SIZE - cb.available);
    if (space_left == 0) {
        console_write(CONSOLE_OVERRUN_ERR, sizeof(CONSOLE_OVERRUN_ERR));
        HAL_NVIC_SystemReset();
    }

    HAL_UARTEx_ReceiveToIdle_DMA(g_console_huart, &cb.buf[cb.in],
                                 AVS_MIN(BUF_CHUNK_SIZE, space_left));
    __HAL_DMA_DISABLE_IT(g_console_huart->hdmarx, DMA_IT_HT);
}

void console_init(void) {
    cb.out = BUF_SIZE - 1;
    uint8_t rx_char;
    while (HAL_UART_Receive(g_console_huart, &rx_char, 1, 10) == HAL_OK) {
    }
    while (HAL_UARTEx_ReceiveToIdle_DMA(g_console_huart, cb.buf, BUF_CHUNK_SIZE)
           == HAL_BUSY) {
    }
    // By default HAL enabled Half-Transmission interrupts, they are not needed
    // in our case
    __HAL_DMA_DISABLE_IT(g_console_huart->hdmarx, DMA_IT_HT);
}

static char console_read_character(void) {
    uint8_t ch = '\r';

    while (circ_buf_get(&cb, &ch) != 0) {
#ifdef HAL_IWDG_MODULE_ENABLED
        WRITE_REG(IWDG->KR, IWDG_KEY_RELOAD);
#endif
    }
    return ch;
}

void console_read_line(char *buffer, size_t size, char end_char) {
    char ch;
    char last_char = '\0';
    size_t i = 0;

    while ((ch = console_read_character()) != end_char) {
        if (ch == CHAR_BS || ch == CHAR_DEL) {
            if (i > 0) {
                i--;
                console_write("\b \b", 3);
            }
        } else {
            if (i < size - 1) {
                if (last_char == '\r' && ch != '\n' && i < size - 2) {
                    buffer[i++] = '\n';
                    console_write("\n", 1);
                }
                buffer[i++] = ch;
                last_char = ch;
                console_write(&ch, 1);
            }
        }
    }
    buffer[i] = '\0';
    for (i--; i > 0; i--) {
        if (!isspace((int) buffer[i])) {
            break;
        }
        buffer[i] = '\0';
    }
    console_write("\r\n", 2);
}

void console_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    console_write(buffer, strlen(buffer));
    va_end(args);
}

void console_write(const char *buf, size_t len) {
    HAL_UART_AbortTransmit(g_console_huart);
    HAL_UART_Transmit(g_console_huart, (uint8_t *) buf, len, HAL_MAX_DELAY);
}

bool console_wait_for_key_press(uint32_t timeout_ms) {
    uint8_t ch;
    uint32_t tim = 0;
    while (1) {
        if (circ_buf_get(&cb, &ch) == 0) {
            return true;
        } else {
            HAL_Delay(10);
            tim += 10;
            if (tim > timeout_ms) {
                break;
            }
#ifdef HAL_IWDG_MODULE_ENABLED
            WRITE_REG(IWDG->KR, IWDG_KEY_RELOAD);
#endif
        }
    }
    return false;
}
