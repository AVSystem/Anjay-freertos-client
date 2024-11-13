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

#include <stdint.h>

#include <avsystem/commons/avs_utils.h>

#include "utils.h"

#if defined(STM32L496xx) || defined(STM32L462xx)
#include "stm32l4xx_hal.h"
#define get_uid_word(word) HAL_GetUIDw##word()
#endif // defined(STM32L496xx) || defined(STM32L462xx)

#if defined(STM32U585xx)
#ifdef DEBUG
#include "stm32u5xx_ll_utils.h"
#define get_uid_word(word) LL_GetUID_Word##word()
#else // DEBUG
#include "non_secure_to_secure.h"
#define get_uid_word(word) Secure_GetUID_Word(word)
#endif // DEBUG
#endif // defined(STM32U585xx)

void get_uid(device_id_t *out_id) {
    // Based on z_impl_hwinfo_get_device_id() function from Zephyr for getting
    // UID of STM32
    uint32_t uid_words[] = { avs_convert_be32(get_uid_word(2)),
                             avs_convert_be32(get_uid_word(1)),
                             avs_convert_be32(get_uid_word(0)) };
    avs_hexlify(out_id->value, sizeof(out_id->value), NULL, uid_words,
                sizeof(uid_words));
}

#ifdef USE_FW_UPDATE
void flash_aligned_writer_new(uint64_t *batch_buf,
                              size_t batch_buf_max_len_words,
                              flash_aligned_writer_cb_t *writer_cb,
                              flash_aligned_writer_t *out_writer) {
    assert(batch_buf);
    assert(batch_buf_max_len_words);
    assert(writer_cb);

    out_writer->batch_buf = batch_buf;
    out_writer->batch_buf_max_len_bytes =
            sizeof(*out_writer->batch_buf) * batch_buf_max_len_words;
    out_writer->batch_buf_len_bytes = 0;
    out_writer->write_offset_bytes = 0;
    out_writer->writer_cb = writer_cb;
}

int flash_aligned_writer_write(flash_aligned_writer_t *writer,
                               const uint8_t *data,
                               size_t length_bytes) {
    while (length_bytes > 0) {
        const size_t bytes_to_copy = AVS_MIN(
                writer->batch_buf_max_len_bytes - writer->batch_buf_len_bytes,
                length_bytes);
        memcpy((uint8_t *) writer->batch_buf + writer->batch_buf_len_bytes,
               data, bytes_to_copy);
        data += bytes_to_copy;
        length_bytes -= bytes_to_copy;
        writer->batch_buf_len_bytes += bytes_to_copy;

        if (writer->batch_buf_len_bytes == writer->batch_buf_max_len_bytes) {
            int res = writer->writer_cb(writer->batch_buf,
                                        writer->write_offset_bytes,
                                        writer->batch_buf_len_bytes);
            if (res) {
                return res;
            }
            writer->write_offset_bytes += writer->batch_buf_len_bytes;
            writer->batch_buf_len_bytes = 0;
        }
    }

    return 0;
}

int flash_aligned_writer_flush(flash_aligned_writer_t *writer) {
    if (writer->batch_buf_len_bytes == 0) {
        return 0;
    }
    if (writer->batch_buf_len_bytes % sizeof(*writer->batch_buf) != 0) {
        return -1;
    }

    int res = writer->writer_cb(writer->batch_buf, writer->write_offset_bytes,
                                writer->batch_buf_len_bytes);
    if (res) {
        return res;
    }
    writer->write_offset_bytes += writer->batch_buf_len_bytes;
    writer->batch_buf_len_bytes = 0;
    return 0;
}
#endif // USE_FW_UPDATE
