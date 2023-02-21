/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
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
#ifdef USE_AIBP

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os_misrac2012.h"
#include "stm32l4xx_hal.h"

#include "ai_bridge.h"
#include "console.h"

#include "anomaly_detector_object.h"
#include "pattern_detector_object.h"

char **ai_bridge_classes;
int ai_bridge_num_of_classes;
int ai_bridge_type;

extern UART_HandleTypeDef *const g_ai_bridge_huart;

/******************* Non-Interrupt Based UART Functions *******************/

static char ai_bridge_read_character(void) {
    char ch = '\n';
    while (HAL_UART_Receive(g_ai_bridge_huart, (uint8_t *) &ch, sizeof(char),
                            HAL_MAX_DELAY)
           == HAL_OK) {
        if (ch != '\r') {
            break;
        }
    }
    return ch;
}

static void ai_bridge_read_line(char *buffer, size_t size) {
    char ch;
    size_t i = 0;
    while ((ch = ai_bridge_read_character()) != '\n') {
        if (i < size - 1) {
            buffer[i++] = ch;
        }
    }
    buffer[i] = '\0';
}

void ai_bridge_write(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[128];

    if (avs_simple_vsnprintf(buffer, sizeof(buffer), format, args) >= 0) {
        HAL_UART_Transmit(g_ai_bridge_huart, (uint8_t *) buffer, strlen(buffer),
                          HAL_MAX_DELAY);
    }

    va_end(args);
}

uint8_t ai_bridge_get_type() {
    int repeats = 10;
    char buffer[64];

    ai_bridge_type = AI_BRIDGE_UNKNOWN_TYPE;

    ai_bridge_write("T\n");

    while (repeats) {
        ai_bridge_read_line(buffer, sizeof(buffer));

        switch (buffer[0]) {
        case AI_BRIDGE_ANOMALY_HEADER:
            console_printf("AI ANOMALY DETECTION\r\n");
            ai_bridge_type = AI_BRIDGE_ANOMALY_TYPE;
            return AI_BRIDGE_ANOMALY_TYPE;
        case AI_BRIDGE_CLASSIFIER_HEADER:
            console_printf("AI CLASSIFIER\r\n");
            ai_bridge_type = AI_BRIDGE_CLASSIFIER_TYPE;
            return AI_BRIDGE_CLASSIFIER_TYPE;
        default:
            repeats--;
            break;
        }
    }

    console_printf("AI BRIDGE UNKNOWN TYPE\r\n");
    return AI_BRIDGE_UNKNOWN_TYPE;
}

uint8_t ai_bridge_get_classes() {
    int repeats = 10;
    char buffer[64];
    ai_bridge_write("D\n");

    while (repeats) {
        ai_bridge_read_line(buffer, sizeof(buffer));

        switch (buffer[0]) {
        case AI_BRIDGE_CLASSIFIER_CLASSES:

            ai_bridge_num_of_classes = atoi(&buffer[2]);
            ai_bridge_classes =
                    pvPortMalloc(ai_bridge_num_of_classes * sizeof(char *));

            if (ai_bridge_num_of_classes > 0) {
                int i = 2;
                while (buffer[i++] != ':')
                    ;
                int len = strlen(buffer);
                char *tmp = pvPortMalloc((len - i + 1) * sizeof(char));
                memcpy(tmp, buffer + i, len - i);

                tmp[len - i] = '\0';

                ai_bridge_classes[0] = tmp;

                int j = 1;
                for (int k = 0; k < (len - i); k++) {
                    if (tmp[k] == ':') {
                        tmp[k] = '\0';
                        ai_bridge_classes[j++] = tmp + k + 1;
                    }
                }
            }

            console_write("Number of classes:%d\r\n", ai_bridge_num_of_classes);

            for (int k = 0; k < ai_bridge_num_of_classes; k++) {
                console_printf("[%d] = %s\r\n", k, ai_bridge_classes[k]);
            }

            return AI_BRIDGE_CLASSIFIER_TYPE;
        default:
            repeats--;
            break;
        }
    }
    return AI_BRIDGE_UNKNOWN_TYPE;
}

void ai_bridge_learn() {
    ai_bridge_write("L;\n");
}

/******************* Interrupt Based UART Functions *******************/
#define AI_BRIDGE_LINE_MAX 128
char ai_bridge_ch;
char ai_bridge_line[AI_BRIDGE_LINE_MAX];
size_t ai_bridge_line_index;

void ai_bridge_it_process_line() {
    float anomaly;
    int class;

    switch (ai_bridge_line[0]) {
    case AI_BRIDGE_ANOMALY_MSG:
        anomaly = atof(&ai_bridge_line[2]);
        anomaly_detector_object_value_update(anomaly);
        break;
    case AI_BRIDGE_CLASSIFIER_MSG:
        class = atoi(&ai_bridge_line[2]);
        pattern_detector_object_value_update(class);
        break;

    default:
        break;
    }

    ai_bridge_line_index = 0;
}

void ai_bridge_start_it(void) {
    ai_bridge_line_index = 0;
    HAL_UART_Receive_IT(g_ai_bridge_huart, (uint8_t *) &ai_bridge_ch,
                        sizeof(char));
}

void ai_bridge_it_callback(void) {
    switch (ai_bridge_ch) {
    case '\r':
        break;
    case '\n':
        ai_bridge_line[ai_bridge_line_index] = 0;
        ai_bridge_it_process_line();
        break;
    default:
        if (ai_bridge_line_index < AI_BRIDGE_LINE_MAX - 1) {
            ai_bridge_line[ai_bridge_line_index++] = ai_bridge_ch;
        }
        break;
    }

    HAL_UART_Receive_IT(g_ai_bridge_huart, (uint8_t *) &ai_bridge_ch,
                        sizeof(char));
}

#endif
