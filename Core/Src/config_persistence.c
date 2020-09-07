/*
 * Copyright 2020 AVSystem <avsystem@avsystem.com>
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

#include <avsystem/commons/avs_log.h>

#include <string.h>

#include "config_persistence.h"
#include "quadspi.h"

#define LOG(level, ...) avs_log(config, level, __VA_ARGS__)

#define MAGIC "cfgstart"
#define MAGIC_SIZE sizeof(MAGIC)
#define MAGIC_BASE_ADDR 0
#define CONFIG_BASE_ADDR (MAGIC_BASE_ADDR + MAGIC_SIZE)
#define CONFIG_BLOCK_ADDR 0

static int load_config_from_flash(config_t *out_config) {
    AVS_STATIC_ASSERT(sizeof(config_t) == 224, config_struct_change_guard);
    char magic[MAGIC_SIZE];
    if (BSP_QSPI_Read((uint8_t *) magic, MAGIC_BASE_ADDR, MAGIC_SIZE)
            || strcmp(magic, MAGIC)
            || BSP_QSPI_Read((uint8_t *) out_config,
                             CONFIG_BASE_ADDR,
                             sizeof(*out_config))) {
        return -1;
    }
    return 0;
}

int config_save(const config_t *in_config) {
    AVS_STATIC_ASSERT(sizeof(config_t) == 224, config_struct_change_guard);
    if (BSP_QSPI_Erase_Block(CONFIG_BLOCK_ADDR)
            || BSP_QSPI_Write((uint8_t *) MAGIC, MAGIC_BASE_ADDR, MAGIC_SIZE)
            || BSP_QSPI_Write((uint8_t *) in_config,
                              CONFIG_BASE_ADDR,
                              sizeof(*in_config))) {
        return -1;
    }
    return 0;
}

int config_load(config_t *out_config) {
    config_t loaded_config;
    if (load_config_from_flash(&loaded_config)) {
        return -1;
    }
    *out_config = loaded_config;
    return 0;
}
