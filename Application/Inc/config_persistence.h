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

#ifndef CONFIG_PERSISTENCE_H
#define CONFIG_PERSISTENCE_H

#ifdef USE_SMS_TRIGGER
#include <anjay/sms.h>
#endif // USE_SMS_TRIGGER

#include <avsystem/commons/avs_stream_md5.h>

#include "cellular_service_datacache.h"

#define APN_SIZE sizeof(((dc_sim_slot_t *) 0)->apn)
#define APN_USERNAME_SIZE sizeof(((dc_sim_slot_t *) 0)->username)
#define APN_PASSWORD_SIZE sizeof(((dc_sim_slot_t *) 0)->password)

typedef struct {
    char server_uri[128];
    char endpoint_name[64];
    char public_cert_or_psk_identity[2048];
    char private_cert_or_psk_key[1024];
    char apn[APN_SIZE];
    char apn_username[APN_USERNAME_SIZE];
    char apn_password[APN_PASSWORD_SIZE];
#ifdef USE_SMS_TRIGGER
    char use_sms_trigger[2];
    char local_msisdn[ANJAY_MSISDN_SIZE];
    char server_msisdn[ANJAY_MSISDN_SIZE];
#endif // USE_SMS_TRIGGER
    char firmware_version[32];
    char bootstrap[2];
    char security[5];
    char use_persistence[2];
    char sim_bs_data_md5[AVS_COMMONS_MD5_LENGTH];
    char use_sim_bootstrap[2];
} config_t;

int config_save(const config_t *in_config);
int config_load(config_t *out_config);

extern config_t g_config;
#endif // CONFIG_PERSISTENCE_H
