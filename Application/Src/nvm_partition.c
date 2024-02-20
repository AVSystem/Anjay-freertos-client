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

#include <nvm_partition.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_stream.h>
#include <avsystem/commons/avs_stream_simple_io.h>

#include <nvm_partition_driver.h>

// Last char denotes version
#define MAGIC_CONFIG "anjay_nvm_conf\3"
#define MAGIC_MODULES "anjay_nvm_mods\1"
#define MAGIC_CORE "anjay_nvm_core\1"
#define MAGIC_LEN (sizeof(MAGIC_CONFIG) - 1)

AVS_STATIC_ASSERT(sizeof(MAGIC_CONFIG) == sizeof(MAGIC_MODULES),
                  magic_size_config_neq_modules);
AVS_STATIC_ASSERT(sizeof(MAGIC_CONFIG) == sizeof(MAGIC_CORE),
                  magic_size_config_neq_core);

static size_t curr_offsets[_NVM_PARTITION_COUNT];

static inline nvm_partition_t partition_from_ctx(void *ctx) {
    nvm_partition_t partition = (nvm_partition_t) (uintptr_t) ctx;
    nvm_partition_assert(partition);
    return partition;
}

static inline void *partition_to_ctx(nvm_partition_t partition) {
    return (void *) (uintptr_t) partition;
}

static int
stream_writer(void *context, const void *buffer, size_t *inout_size) {
    nvm_partition_t partition = partition_from_ctx(context);
    if (nvm_partition_driver_write(partition, curr_offsets[partition], buffer,
                                   *inout_size)) {
        return -1;
    }
    curr_offsets[partition] += *inout_size;
    return 0;
}

static int stream_reader(void *context, void *buffer, size_t *inout_size) {
    nvm_partition_t partition = partition_from_ctx(context);
    if (nvm_partition_driver_read(partition, curr_offsets[partition], buffer,
                                  *inout_size)) {
        return -1;
    }
    curr_offsets[partition] += *inout_size;
    return 0;
}

static const char *partition_magic(nvm_partition_t partition) {
    nvm_partition_assert(partition);
    return (const char *[]) {
        [NVM_PARTITION_CONFIG] = MAGIC_CONFIG,
        [NVM_PARTITION_MODULES] = MAGIC_MODULES,
        [NVM_PARTITION_CORE] = MAGIC_CORE,
    }[partition];
}

int nvm_partition_clear(nvm_partition_t partition) {
    nvm_partition_assert(partition);

    // write dummy byte to invalidate magic in case
    // EEPROM is used for which clear is a no-op
    return nvm_partition_driver_clear(partition)
           || nvm_partition_driver_write(partition, 0,
                                         &(const uint8_t) { 0xFF }, 1);
}

int nvm_partition_mark_valid(nvm_partition_t partition) {
    nvm_partition_assert(partition);

    return nvm_partition_driver_write(partition, 0, partition_magic(partition),
                                      MAGIC_LEN);
}

int nvm_partition_stream_output_open(nvm_partition_t partition,
                                     avs_stream_t **out_stream) {
    nvm_partition_assert(partition);
    assert(out_stream);

    *out_stream = NULL;
    if (!nvm_partition_clear(partition)) {
        curr_offsets[partition] = MAGIC_LEN;
        *out_stream =
                avs_stream_simple_output_create(stream_writer,
                                                partition_to_ctx(partition));
    }
    return *out_stream ? 0 : -1;
}

int nvm_partition_stream_input_open(nvm_partition_t partition,
                                    avs_stream_t **out_stream) {
    nvm_partition_assert(partition);
    assert(out_stream);

    *out_stream = NULL;
    char magic[MAGIC_LEN];
    if (nvm_partition_driver_read(partition, 0, magic, sizeof(magic))) {
        return -1;
    }
    if (memcmp(partition_magic(partition), magic, sizeof(magic))) {
        return 0;
    }

    curr_offsets[partition] = MAGIC_LEN;
    *out_stream = avs_stream_simple_input_create(stream_reader,
                                                 partition_to_ctx(partition));
    return *out_stream ? 0 : -1;
}
