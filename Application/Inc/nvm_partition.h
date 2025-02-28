/*
 * Copyright 2020-2025 AVSystem <avsystem@avsystem.com>
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

#ifndef NVM_PARTITION_H
#define NVM_PARTITION_H

#include <assert.h>
#include <stdbool.h>

#include <avsystem/commons/avs_stream.h>

typedef enum {
    NVM_PARTITION_CONFIG,
    NVM_PARTITION_MODULES,
    NVM_PARTITION_CORE,
    _NVM_PARTITION_COUNT
} nvm_partition_t;

static inline void nvm_partition_assert(nvm_partition_t partition) {
    assert(partition >= 0 && partition < _NVM_PARTITION_COUNT);
}

int nvm_partition_mark_valid(nvm_partition_t partition);
int nvm_partition_clear(nvm_partition_t partition);
int nvm_partition_stream_output_open(nvm_partition_t partition,
                                     avs_stream_t **out_stream);
int nvm_partition_stream_input_open(nvm_partition_t partition,
                                    avs_stream_t **out_stream);
#endif // NVM_PARTITION_H
