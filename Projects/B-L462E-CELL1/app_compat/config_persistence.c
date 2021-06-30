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

#include <avsystem/commons/avs_log.h>

#include <string.h>

#include "config_persistence.h"
#include "stm32l462e_cell1_eeprom.h"

#define LOG(level, ...) avs_log(config, level, __VA_ARGS__)

static int eeprom_init() {
    static bool initialized = false;
    if (!initialized) {
        if (BSP_EEPROM_Init()) {
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
    if (BSP_EEPROM_WriteBuffer((uint8_t *) MAGIC, MAGIC_BASE_ADDR, MAGIC_SIZE)
            || BSP_EEPROM_WaitEepromStandbyState()
            || BSP_EEPROM_WriteBuffer((uint8_t *) in_config,
                                      CONFIG_BASE_ADDR,
                                      sizeof(*in_config))) {
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
    uint16_t magic_size = MAGIC_SIZE;
    uint16_t config_size = sizeof(loaded_config);
    if (BSP_EEPROM_ReadBuffer((uint8_t *) magic, MAGIC_BASE_ADDR, &magic_size)
            || BSP_EEPROM_WaitEepromStandbyState() || strcmp(magic, MAGIC)
            || BSP_EEPROM_ReadBuffer((uint8_t *) &loaded_config,
                                     CONFIG_BASE_ADDR,
                                     &config_size)
            || BSP_EEPROM_WaitEepromStandbyState()) {
        return -1;
    }

    *out_config = loaded_config;
    return 0;
}
