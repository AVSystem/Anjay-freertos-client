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

#include <stdint.h>

#include <avsystem/commons/avs_utils.h>

#include "utils.h"

#if defined(STM32L496xx) || defined(STM32L462xx)
#include "stm32l4xx_hal.h"
#define get_uid_word(word) HAL_GetUIDw##word()
#endif // defined(STM32L496xx) || defined(STM32L462xx)

#if defined(STM32U585xx)
#include "stm32u5xx_ll_utils.h"
#define get_uid_word(word) LL_GetUID_Word##word()
#endif

void get_uid(device_id_t *out_id) {
    // Based on z_impl_hwinfo_get_device_id() function from Zephyr for getting
    // UID of STM32
    uint32_t uid_words[] = { avs_convert_be32(get_uid_word(2)),
                             avs_convert_be32(get_uid_word(1)),
                             avs_convert_be32(get_uid_word(0)) };
    avs_hexlify(out_id->value, sizeof(out_id->value), NULL, uid_words,
                sizeof(uid_words));
}
