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

#ifndef CONFIG_PERSISTENCE_H
#define CONFIG_PERSISTENCE_H

#include "cellular_service_datacache.h"

#define MAGIC "cfgstart"
#define MAGIC_SIZE sizeof(MAGIC)
#define MAGIC_BASE_ADDR 0
#define CONFIG_BASE_ADDR (MAGIC_BASE_ADDR + MAGIC_SIZE)
#define CONFIG_BLOCK_ADDR 0

#define APN_SIZE sizeof(((dc_sim_slot_t *) 0)->apn)
#define APN_USERNAME_SIZE sizeof(((dc_sim_slot_t *) 0)->username)
#define APN_PASSWORD_SIZE sizeof(((dc_sim_slot_t *) 0)->password)

typedef struct {
    char server_uri[128];
    char endpoint_name[64];
    char psk[32];
    char apn[APN_SIZE];
    char apn_username[APN_USERNAME_SIZE];
    char apn_password[APN_PASSWORD_SIZE];
} config_t;

int config_save(const config_t *in_config);
int config_load(config_t *out_config);

#endif // CONFIG_PERSISTENCE_H
