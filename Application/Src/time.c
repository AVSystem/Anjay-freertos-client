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

#include <stdio.h>

#include <avsystem/commons/avs_time.h>

#include "cmsis_os.h"
#include "task.h"

#include "circ_buffer_ipc.h"

#if defined(STM32L496xx) || defined(STM32L462xx)
#include "stm32l4xx_hal.h"
#endif // defined(STM32L496xx) || defined(STM32L462xx)

#if defined(STM32U585xx)
#include "stm32u5xx_hal.h"
#endif // defined(STM32U585xx)

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
#include "cellular_service_control.h"
#include "cellular_service_os.h"
#include "ipc_common.h"
#include "ipc_rxfifo.h"
#endif // (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

avs_time_monotonic_t avs_time_monotonic_now(void) {
    static int64_t prev_tick = 0;
    static int64_t base_tick = 0;

    taskENTER_CRITICAL();

    int64_t ticks = osKernelSysTick();

    if (ticks < prev_tick) {
        base_tick += (int64_t) UINT32_MAX + 1;
    }
    prev_tick = ticks;
    ticks += base_tick;

    taskEXIT_CRITICAL();

    const int64_t time_ms = ticks * 1000 / osKernelSysTickFrequency;
    return avs_time_monotonic_from_scalar(time_ms, AVS_TIME_MS);
}

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
static IPC_RxCallbackTypeDef g_orig_rx_callback;

typedef char cclk_response_t[sizeof("+CCLK: \"00/00/00,00:00:00+00\"")];
static cclk_response_t g_cclk_response;
static cclk_response_t g_cclk_response_candidate;

#define CCLK_RESPONSE_PREFIX "+CCLK: \""

static int cclk_check_and_parse_msg_from_buffer(char *msg_buffer) {
    if (strncmp(g_cclk_response_candidate, CCLK_RESPONSE_PREFIX,
                strlen(CCLK_RESPONSE_PREFIX))
            == 0) {
        memcpy(g_cclk_response, g_cclk_response_candidate,
               sizeof(g_cclk_response));
    }

    return 0;
}
static void read_modem_cclk_rx_callback(IPC_Handle_t *channel) {
    // CAUTION: We are in an ISR context here
    read_ipc_circ_buffer_and_handle_msg(channel, g_cclk_response_candidate,
                                        sizeof(g_cclk_response_candidate),
                                        cclk_check_and_parse_msg_from_buffer);

    if (g_orig_rx_callback) {
        g_orig_rx_callback(channel);
    }
}

typedef struct {
    unsigned short year;
    unsigned short month;
    unsigned short day;
    unsigned short hours;
    unsigned short minutes;
    unsigned short seconds;
    unsigned short tz_quarters_east;
} parsed_modem_cclk_t;

static int read_modem_cclk(parsed_modem_cclk_t *out) {
    IPC_Handle_t *channel =
            IPC_DevicesList[USER_DEFINED_IPC_DEVICE_MODEM].h_current_channel;

    memset(g_cclk_response, 0, sizeof(g_cclk_response));

    __disable_irq();
    // HACK: ST APIs don't allow reading the response to the command issued with
    // osCDS_direct_cmd(). So we temporarily change the callback that is called
    // by the IPC subsystem when new message is received.
    g_orig_rx_callback = channel->RxClientCallback;
    channel->RxClientCallback = read_modem_cclk_rx_callback;
    __enable_irq();

#define CCLK_COMMAND "AT+CCLK?"
    CS_direct_cmd_tx_t direct_cmd_tx = {
        .cmd_str = CCLK_COMMAND,
        .cmd_size = strlen(CCLK_COMMAND)
    };
#undef CCLK_COMMAND

    osCDS_direct_cmd(&direct_cmd_tx, NULL);

    __disable_irq();
    channel->RxClientCallback = g_orig_rx_callback;
    __enable_irq();

    if (strncmp(g_cclk_response, CCLK_RESPONSE_PREFIX,
                strlen(CCLK_RESPONSE_PREFIX))
            != 0) {
        // CCLK response not read
        return -1;
    }

    memset(out, 0, sizeof(*out));
    char tzsign;
    int scanf_result =
            sscanf(&g_cclk_response[strlen(CCLK_RESPONSE_PREFIX)],
                   "%hu/%hu/%hu,%hu:%hu:%hu%c%hu", &out->year, &out->month,
                   &out->day, &out->hours, &out->minutes, &out->seconds,
                   &tzsign, &out->tz_quarters_east);
    if (scanf_result < 6 || out->month < 1 || out->month > 12 || out->day < 1
            || out->day > 31 || out->hours > 23 || out->minutes > 59
            || out->seconds > 60) {
        return -1;
    } else if (scanf_result < 8) {
        // no timezone, this is legal
        out->tz_quarters_east = 0;
    } else if (tzsign == '-') {
        out->tz_quarters_east = -out->tz_quarters_east;
    } else if (tzsign != '+') {
        return -1;
    }

    return 0;
}

#undef CCLK_RESPONSE_PREFIX

static int32_t year_to_days(unsigned short year, bool *out_is_leap) {
    // This only supports two-digit dates between 2000 and 2099...

    static const int64_t LEAP_YEARS_BETWEEN_1970_AND_1999 = 7;

    *out_is_leap = (year % 4 == 0);
    int64_t leap_years_since_1970 = LEAP_YEARS_BETWEEN_1970_AND_1999
                                    + (*out_is_leap ? 0 : 1) + year / 4;
    return (year + 30) * 365 + leap_years_since_1970;
}

static int month_to_days(unsigned short month, bool is_leap) {
    static const uint16_t MONTH_LENGTHS[] = {
        31, 28 /* or 29 */, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int days = (is_leap && month > 2) ? 1 : 0;
    for (unsigned short i = 0;
         i < month - 1 && i < AVS_ARRAY_SIZE(MONTH_LENGTHS);
         ++i) {
        days += MONTH_LENGTHS[i];
    }
    return days;
}

static int64_t parsed_modem_cclk_to_timestamp(parsed_modem_cclk_t cclk) {
    bool is_leap;
    int32_t days = year_to_days(cclk.year, &is_leap)
                   + month_to_days(cclk.month, is_leap) + cclk.day - 1;
    int64_t time = 60 * (60 * cclk.hours + cclk.minutes) + cclk.seconds;
    int64_t result = days * 86400 + time;

    // NOTE: There is no consensus between modem manufacturers on the
    // interpretation of this time. It's universal time on at least Quectel and
    // Nordic, but local time on at least Murata and Sequans.
    //
    // This #ifdef will need to be updated if we ever support more modules than
    // TYPE1SC (Murata) and BG96 (Quectel).
#ifdef USE_TYPE1SC_MODEM
    result -= cclk.tz_quarters_east * 15 * 60;
#endif // USE_TYPE1SC_MODEM

    return result;
}

static avs_time_real_t read_current_time(void) {
    parsed_modem_cclk_t cclk;
    if (read_modem_cclk(&cclk)) {
        return AVS_TIME_REAL_INVALID;
    }

    return (avs_time_real_t) {
        .since_real_epoch.seconds = parsed_modem_cclk_to_timestamp(cclk)
    };
}
#endif // (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

static avs_time_duration_t REAL_TIME_OFFSET;

int avs_time_stm32_sync_time(void) {
#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)
    static const int SYNC_ATTEMPTS = 5;
    static const uint32_t SYNC_SLEEP_TIME_MS = 5000;

    avs_time_real_t current_time = AVS_TIME_REAL_INVALID;
    for (int i = 0; i < SYNC_ATTEMPTS; ++i) {
        current_time = read_current_time();
        if (avs_time_real_valid(current_time)) {
            break;
        } else {
            rtosalDelay(SYNC_SLEEP_TIME_MS);
        }
    }

    if (!avs_time_real_valid(current_time)) {
        return -1;
    }

    avs_time_duration_t offset = avs_time_duration_diff(
            current_time.since_real_epoch,
            avs_time_monotonic_now().since_monotonic_epoch);
    if (avs_time_duration_valid(offset)) {
        REAL_TIME_OFFSET = offset;
        return 0;
    }
#endif // (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

    return -1;
}

avs_time_real_t avs_time_real_now(void) {
    avs_time_real_t result = {
        .since_real_epoch = avs_time_duration_add(
                avs_time_monotonic_now().since_monotonic_epoch,
                REAL_TIME_OFFSET)
    };
    return result;
}
