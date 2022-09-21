/*
 * Copyright 2020-2022 AVSystem <avsystem@avsystem.com>
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

#ifndef CONFIG_MBEDTLS_TIMING_ALT_H
#define CONFIG_MBEDTLS_TIMING_ALT_H

/**
 * Timing implementation for the STM board. See MBEDTLS_TIMING_ALT in
 * config/mbedtls.h .
 */

#include <stdint.h>

extern volatile int mbedtls_timing_alarmed;
void mbedtls_set_alarm(int seconds);

unsigned long mbedtls_timing_hardclock(void);

void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms);
int mbedtls_timing_get_delay(void *data);

struct mbedtls_timing_hr_time {
    unsigned char opaque[32];
};

typedef struct {
    struct mbedtls_timing_hr_time timer;
    uint32_t int_ms;
    uint32_t fin_ms;
} mbedtls_timing_delay_context;

unsigned long mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val,
                                       int reset);

#endif // CONFIG_MBEDTLS_TIMING_ALT_H
