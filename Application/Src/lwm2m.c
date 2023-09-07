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

#include "application.h"
#include "config_persistence.h"
#include "lwm2m.h"
#include "menu.h"
#include "persistence.h"

#include "device_object.h"
#include "sensor_objects.h"

#include "joystick_object.h"

#ifdef USE_SIM_BOOTSTRAP
#include <anjay/bootstrapper.h>
#include <avsystem/commons/avs_errno.h>
#include <avsystem/commons/avs_stream.h>
#include <avsystem/commons/avs_stream_md5.h>

#include "sim_bootstrap.h"
#endif // USE_SIM_BOOTSTRAP

#ifdef USE_FW_UPDATE
#include "firmware_update.h"
#endif /* USE_FW_UPDATE */

#ifdef USE_AIBP
#include "ai_bridge.h"
#include "anomaly_detector_object.h"
#include "ml_model_object.h"
#include "pattern_detector_object.h"
#endif

#define LOG(level, ...) avs_log(app, level, __VA_ARGS__)

static anjay_t *volatile g_anjay;
static avs_crypto_prng_ctx_t *g_prng_ctx;

static osThreadId g_lwm2m_task_handle;
static avs_sched_handle_t lwm2m_notify_job_handle;

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
            anjay_t *anjay = g_anjay;
            if (anjay) {
                anjay_event_loop_interrupt(anjay);
            }
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

#ifdef USE_AIBP
    ml_model_object_update(anjay);

    if (ai_bridge_type == AI_BRIDGE_ANOMALY_TYPE) {
        anomaly_detector_object_update(anjay);
    }

    if (ai_bridge_type == AI_BRIDGE_CLASSIFIER_TYPE) {
        pattern_detector_object_update(anjay);
    }
#endif

    three_axis_sensor_objects_update(anjay);

    if (cycle % 5 == 0) {
        basic_sensor_objects_update(anjay);
    }

    if (menu_is_module_persistence_enabled()
            && persistence_mod_persist_if_required(anjay)) {
        LOG(ERROR, "Failed to persist modules");
    }

    heartbeat_led_toggle();

    cycle++;
    AVS_SCHED_DELAYED(sched, &lwm2m_notify_job_handle,
                      avs_time_duration_from_scalar(1, AVS_TIME_S),
                      lwm2m_notify_job, &anjay, sizeof(anjay));
}

static int setup_security_object_from_config(void) {
    anjay_security_instance_t security_instance = {
        .ssid = 1,
        .server_uri = g_config.server_uri,
    };
    if (strcmp(g_config.security, "psk") == 0) {
        security_instance.security_mode = ANJAY_SECURITY_PSK;
    } else if (strcmp(g_config.security, "cert") == 0) {
        security_instance.security_mode = ANJAY_SECURITY_CERTIFICATE;
    } else {
        security_instance.security_mode = ANJAY_SECURITY_NOSEC;
    }

    if (security_instance.security_mode != ANJAY_SECURITY_NOSEC) {
        security_instance.public_cert_or_psk_identity =
                (uint8_t *) g_config.public_cert_or_psk_identity;
        security_instance.public_cert_or_psk_identity_size =
                strlen(g_config.public_cert_or_psk_identity);
        security_instance.private_cert_or_psk_key =
                (uint8_t *) g_config.private_cert_or_psk_key;
        security_instance.private_cert_or_psk_key_size =
                strlen(g_config.private_cert_or_psk_key);
        if (strcmp(g_config.security, "cert") == 0) {
            security_instance.public_cert_or_psk_identity_size += 1;
            security_instance.private_cert_or_psk_key_size += 1;
        }
    }

    if (g_config.bootstrap[0] == 'y') {
        security_instance.bootstrap_server = true;
    } else {
        security_instance.bootstrap_server = false;
    }

    anjay_iid_t security_instance_id = ANJAY_ID_INVALID;
    return anjay_security_object_add_instance(g_anjay, &security_instance,
                                              &security_instance_id);
}

static const char *binding_mode_from_uri(const char *uri) {
    static const struct {
        const char *prefix;
        const char *mode;
    } mode_dict[] = { { "coap+tcp://", "T" }, { "coaps+tcp://", "T" } };

    for (size_t i = 0; i < AVS_ARRAY_SIZE(mode_dict); i++) {
        if (strncmp(mode_dict[i].prefix, uri, strlen(mode_dict[i].prefix))
                == 0) {
            return mode_dict[i].mode;
        }
    }

    return "U";
}

static int setup_server_object_from_config(void) {
    if (g_config.bootstrap[0] == 'y') {
        return 0;
    }
    const anjay_server_instance_t server_instance = {
        .ssid = 1,
        .lifetime = 60,
        .default_min_period = -1,
        .default_max_period = -1,
        .disable_timeout = -1,
        .binding = binding_mode_from_uri(g_config.server_uri)
    };

    anjay_iid_t server_instance_id = ANJAY_ID_INVALID;
    return anjay_server_object_add_instance(g_anjay, &server_instance,
                                            &server_instance_id);
}

static int install_required_objects(void) {
    if (anjay_security_object_install(g_anjay)
            || anjay_server_object_install(g_anjay)
            || device_object_install(g_anjay)) {
        LOG(ERROR, "failed to install required objects");
        return -1;
    }
    return 0;
}

#ifdef USE_SIM_BOOTSTRAP
static int try_bootstrap_from_sim(void) {
    if (!menu_is_module_persistence_enabled()) {
        if (avs_is_err(anjay_sim_bootstrap_perform(
                    g_anjay, sim_bootstrap_perform_command, NULL))) {
            LOG(ERROR, "failed to bootstrap from SIM");
            return -1;
        } else {
            LOG(INFO, "Successfully bootstrapped from SIM");
            return 0;
        }
    }

    avs_stream_t *stream =
            anjay_sim_bootstrap_stream_create(sim_bootstrap_perform_command,
                                              NULL);
    if (!stream) {
        goto fail;
    }
    uint8_t md5[AVS_COMMONS_MD5_LENGTH];
    if (!avs_is_ok(anjay_sim_bootstrap_calculate_md5(stream, &md5))) {
        LOG(ERROR, "Failed to calculate SIM Bootstrap data MD5 hash");
        goto fail;
    }

    if (!memcmp(md5, g_config.sim_bs_data_md5, AVS_COMMONS_MD5_LENGTH)) {
        LOG(INFO, "MD5 did not change, continue");
        goto fail;
    }

    LOG(INFO, "New MD5, bootstrapping from SIM");

    if (avs_is_ok(anjay_bootstrapper(g_anjay, stream))) {
        LOG(INFO, "Successfully bootstrapped from SIM");
        memcpy(g_config.sim_bs_data_md5, md5, AVS_COMMONS_MD5_LENGTH);
        if (config_save(&g_config)) {
            LOG(ERROR, "Failed to persist SIM Bootstrap data MD5");
        }
        avs_stream_cleanup(&stream);
        return 0;
    } else {
        LOG(ERROR, "Error during bootstrapping from SIM");
    }

fail:
    avs_stream_cleanup(&stream);
    return -1;
}
#endif // USE_SIM_BOOTSTRAP

static int setup_required_objects(void) {
#ifdef USE_SIM_BOOTSTRAP
    if (menu_is_sim_bootstrap_enabled() && !try_bootstrap_from_sim()) {
        return 0;
    }
#endif // USE_SIM_BOOTSTRAP

    if (menu_is_module_persistence_enabled()) {
        if (!persistence_mod_restore(g_anjay)) {
            LOG(INFO, "Restored Anjay modules from Persistence");
            return 0;
        }
        LOG(WARNING,
            "failed to restore modules from persistence, using fallback from "
            "console config");
        persistence_clear();
    }

    if (setup_security_object_from_config()
            || setup_server_object_from_config()) {
        LOG(ERROR, "failed to setup from console config");
        return -1;
    }
    return 0;
}

static anjay_t *create_and_setup_anjay(void) {
    anjay_configuration_t config = {
        .endpoint_name = g_config.endpoint_name,
        .in_buffer_size = 2048,
        .out_buffer_size = 2048,
        .msg_cache_size = 2048,
        .use_connection_id = true,
        .prng_ctx = g_prng_ctx
    };

    anjay_t *anjay = NULL;
    anjay = anjay_new(&config);
    if (!anjay) {
        LOG(ERROR, "failed to create Anjay object");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
        return NULL;
    }

    g_anjay = anjay;

    basic_sensor_objects_install(anjay);
    three_axis_sensor_objects_install(anjay);

    joystick_object_install(anjay);

#ifdef USE_FW_UPDATE
    fw_update_install(anjay);
#endif /* USE_FW_UPDATE */

#ifdef USE_AIBP
    ml_model_object_install(anjay);

    if (ai_bridge_type == AI_BRIDGE_ANOMALY_TYPE) {
        anomaly_detector_object_install(anjay);
    }

    if (ai_bridge_type == AI_BRIDGE_CLASSIFIER_TYPE) {
        pattern_detector_object_install(anjay);
    }
#endif

    return anjay;
}

static void lwm2m_thread(void const *user_arg) {
    (void) user_arg;

    (void) osMessageGet(status_msg_queue, RTOS_WAIT_FOREVER);

    int sync_time_result = avs_time_stm32_sync_time();
    if (sync_time_result) {
        LOG(WARNING, "failed to synchronize time");
    }

    anjay_t *anjay = create_and_setup_anjay();

    if (install_required_objects() || setup_required_objects()) {
        LOG(ERROR, "failed to install and setup required objects");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }

    lwm2m_notify_job(anjay_get_scheduler(anjay), &anjay);
    // TODO handle connection lost

    anjay_event_loop_run(anjay, avs_time_duration_from_scalar(1, AVS_TIME_S));

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

void lwm2m_init(void) {
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

    // Registration to datacache
    dc_com_reg_id_t reg =
            dc_com_register_gen_event_cb(&dc_com_db, dc_cellular_callback,
                                         NULL);
    if (reg == DC_COM_INVALID_ENTRY) {
        LOG(ERROR, "failed to subscribe to datacache events");
        ERROR_Handler(DBG_CHAN_APPLICATION, 0, ERROR_FATAL);
    }
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
