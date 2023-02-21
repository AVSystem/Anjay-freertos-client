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

#include <avsystem/commons/avs_log.h>

#include "b_u585i_iot02a_eeprom.h"
#include "config_persistence.h"

#define LOG(level, ...) avs_log(config_persistence, level, __VA_ARGS__)

static int eeprom_init(void) {
    static bool initialized;
    if (!initialized) {
        if (BSP_EEPROM_Init(0)) {
            return -1;
        }
        initialized = true;
    }
    return 0;
}

int config_save(const config_t *in_config) {
    if (eeprom_init()) {
        return -1;
    }
    if (BSP_EEPROM_WriteBuffer(0, (uint8_t *) MAGIC, MAGIC_BASE_ADDR,
                               MAGIC_SIZE)
            || BSP_EEPROM_IsDeviceReady(0)
            || BSP_EEPROM_WriteBuffer(0, (uint8_t *) in_config,
                                      CONFIG_BASE_ADDR, sizeof(*in_config))) {
        return -1;
    }
    return 0;
}

int config_load(config_t *out_config) {
    if (eeprom_init()) {
        return -1;
    }
    config_t loaded_config;
    char magic[MAGIC_SIZE];
    if (BSP_EEPROM_ReadBuffer(0, (uint8_t *) magic, MAGIC_BASE_ADDR, MAGIC_SIZE)
            || BSP_EEPROM_IsDeviceReady(0) || strcmp(magic, MAGIC)
            || BSP_EEPROM_ReadBuffer(0, (uint8_t *) &loaded_config,
                                     CONFIG_BASE_ADDR, sizeof(loaded_config))
            || BSP_EEPROM_IsDeviceReady(0)) {
        return -1;
    }

    *out_config = loaded_config;
    return 0;
}
