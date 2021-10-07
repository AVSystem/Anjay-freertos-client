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

#include <stdbool.h>

#include <anjay/anjay.h>
#include <anjay/attr_storage.h>
#include <anjay/security.h>
#include <anjay/server.h>

#include <avsystem/commons/avs_list.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_prng.h>

#include "cellular_service_datacache.h"
#include "cmsis_os_misrac2012.h"
#include "dc_common.h"
#include "error_handler.h"

#include "lwm2m.h"
#include "menu.h"

#include "device_object.h"
#include "joystick_object.h"
#include "sensor_objects.h"

#include "lwip/sockets.h"

#define LOG(level, ...) avs_log(app, level, __VA_ARGS__)

static anjay_t *g_anjay;
static avs_crypto_prng_ctx_t *g_prng_ctx;

static osThreadId g_lwm2m_task_handle;

extern RNG_HandleTypeDef hrng;

// Used to communicate between datacache callback and lwm2m thread
static osMessageQId status_msg_queue;

static void dc_cellular_callback(dc_com_event_id_t dc_event_id,
                                 const void *user_arg) {
    (void) user_arg;

    if (dc_event_id == DC_CELLULAR_NIFMAN_INFO) {
        dc_nifman_info_t dc_nifman_info;
        (void) dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO,
                           (void *) &dc_nifman_info, sizeof(dc_nifman_info));
        if (dc_nifman_info.rt_state == DC_SERVICE_ON) {
            LOG(INFO, "network is up");
            (void) osMessagePut(status_msg_queue, (uint32_t) dc_event_id, 0);
        } else {
            anjay_event_loop_interrupt(g_anjay);
            LOG(INFO, "network is down");
        }
    } else if (dc_event_id == DC_CELLULAR_CONFIG) {
        dc_cellular_params_t dc_cellular_params;
        (void) dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG,
                           (void *) &dc_cellular_params,
                           sizeof(dc_cellular_params));
        if (dc_cellular_params.rt_state == DC_SERVICE_ON) {
            LOG(INFO, "cellular configuration is updated");
        }
    }
}

static void heartbeat_led_toggle(void) {
    HAL_GPIO_TogglePin(BSP_HEARTBEAT_LED_PORT, BSP_HEARTBEAT_LED);
}

static void lwm2m_notify_job(avs_sched_t *sched, const void *anjay_ptr) {
    static size_t cycle = 0;
    anjay_t *anjay = *(anjay_t *const *) anjay_ptr;

    device_object_update(anjay);
    joystick_object_update(anjay);

    three_axis_sensor_objects_update(anjay);
    if (cycle % 5 == 0) {
        basic_sensor_objects_update(anjay);
    }

    heartbeat_led_toggle();

    cycle++;
    AVS_SCHED_DELAYED(sched, NULL, avs_time_duration_from_scalar(1, AVS_TIME_S),
                      lwm2m_notify_job, &anjay, sizeof(anjay));
}

static void lwm2m_thread(void const *user_arg) {
    (void) user_arg;

    (void) osMessageGet(status_msg_queue, RTOS_WAIT_FOREVER);

    lwm2m_notify_job(anjay_get_scheduler(g_anjay), &g_anjay);
    // TODO handle connection lost
    anjay_event_loop_run(g_anjay, avs_time_duration_from_scalar(1, AVS_TIME_S));
}

static int
entropy_callback(unsigned char *out_buf, size_t out_buf_len, void *user_ptr) {
    uint32_t random_number;
    for (size_t i = 0; i < out_buf_len / sizeof(random_number); i++) {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random_number) != HAL_OK) {
            return -1;
        }
        memcpy(out_buf, &random_number, sizeof(random_number));
        out_buf += sizeof(random_number);
    }

    size_t last_chunk_size = out_buf_len % sizeof(random_number);
    if (last_chunk_size) {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random_number) != HAL_OK) {
            return -1;
        }
        memcpy(out_buf, &random_number, last_chunk_size);
    }

    return 0;
}

static int setup_security_object() {
    if (anjay_security_object_install(g_anjay)) {
        return -1;
    }

    const char *endpoint_name = config_get_endpoint_name();
    const char *psk = config_get_psk();

    const anjay_security_instance_t security_instance = {
        .ssid = 1,
        .server_uri = config_get_server_uri(),
        .security_mode = ANJAY_SECURITY_PSK,
        .public_cert_or_psk_identity = (uint8_t *) endpoint_name,
        .public_cert_or_psk_identity_size = strlen(endpoint_name),
        .private_cert_or_psk_key = (uint8_t *) psk,
        .private_cert_or_psk_key_size = strlen(psk)
    };

    anjay_iid_t security_instance_id = ANJAY_ID_INVALID;
    return anjay_security_object_add_instance(g_anjay, &security_instance,
                                              &security_instance_id);
}

static int setup_server_object() {
    if (anjay_server_object_install(g_anjay)) {
        return -1;
    }

    const anjay_server_instance_t server_instance = {
        .ssid = 1,
        .lifetime = 60,
        .default_min_period = -1,
        .default_max_period = -1,
        .disable_timeout = -1,
        .binding = "U"
    };

    anjay_iid_t server_instance_id = ANJAY_ID_INVALID;
    return anjay_server_object_add_instance(g_anjay, &server_instance,
                                            &server_instance_id);
}

void lwm2m_init(void) {
    menu_init();

    osMessageQDef(status_msg_queue, 1, uint32_t);
    status_msg_queue = osMessageCreate(osMessageQ(status_msg_queue), NULL);
    if (!status_msg_queue) {
        LOG(ERROR, "failed to create message queue");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }

    g_prng_ctx = avs_crypto_prng_new(entropy_callback, NULL);
    if (!g_prng_ctx) {
        LOG(ERROR, "failed to create PRNG ctx");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }

    anjay_configuration_t config = {
        .endpoint_name = config_get_endpoint_name(),
        .in_buffer_size = 2048,
        .out_buffer_size = 2048,
        .msg_cache_size = 2048,
        .prng_ctx = g_prng_ctx
    };

    if (!(g_anjay = anjay_new(&config))) {
        LOG(ERROR, "failed to create Anjay object");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }

    // Registration to datacache
    dc_com_reg_id_t reg =
            dc_com_register_gen_event_cb(&dc_com_db, dc_cellular_callback,
                                         NULL);
    if (reg == DC_COM_INVALID_ENTRY) {
        LOG(ERROR, "failed to subscribe to datacache events");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }

    if (setup_security_object() || setup_server_object()
            || anjay_attr_storage_install(g_anjay)
            || device_object_install(g_anjay)) {
        LOG(ERROR, "failed to setup required objects");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }

    basic_sensor_objects_install(g_anjay);
    three_axis_sensor_objects_install(g_anjay);
    joystick_object_install(g_anjay);
}

static uint32_t lwm2m_thread_stack_buffer[LWM2M_THREAD_STACK_SIZE];
static osStaticThreadDef_t lwm2m_thread_controlblock;
void lwm2m_start(void) {
    osThreadStaticDef(lwm2m_task, lwm2m_thread, LWM2M_THREAD_PRIO, 0,
                      LWM2M_THREAD_STACK_SIZE, lwm2m_thread_stack_buffer,
                      &lwm2m_thread_controlblock);
    g_lwm2m_task_handle = osThreadCreate(osThread(lwm2m_task), NULL);

    if (!g_lwm2m_task_handle) {
        LOG(ERROR, "failed to create thread");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }
}
