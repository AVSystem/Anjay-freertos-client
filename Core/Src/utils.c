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

#include <stdint.h>

#include <avsystem/commons/avs_utils.h>

#include "stm32l4xx_hal.h"
#include "utils.h"

void get_uid(device_id_t *out_id) {
    // Based on z_impl_hwinfo_get_device_id() function from Zephyr for getting
    // UID of STM32
    uint32_t uid_words[] = { avs_convert_be32(HAL_GetUIDw2()),
                             avs_convert_be32(HAL_GetUIDw1()),
                             avs_convert_be32(HAL_GetUIDw0()) };
    avs_hexlify(out_id->value, sizeof(out_id->value), NULL, uid_words,
                sizeof(uid_words));
}
