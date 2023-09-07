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

#include <nvm_partition_driver.h>

#include <assert.h>
#include <stdbool.h>

#include <avsystem/commons/avs_defs.h>

#include <b_u585i_iot02a_ospi.h>

#include <nvm_partition.h>

AVS_STATIC_ASSERT(sizeof(uint32_t) == sizeof(size_t), size_t_is_32_bit);

// since we can erase only whole blocks and they're huge (64K/each),
// just assume partition == block

static inline bool fits(size_t offset, size_t op_len) {
    return offset + op_len < BSP_OSPI_NOR_BLOCK_64K;
}

static inline size_t partition_start(nvm_partition_t partition) {
    return partition * BSP_OSPI_NOR_BLOCK_64K;
}

int nvm_partition_driver_read(nvm_partition_t partition,
                              size_t offset,
                              void *out_data,
                              size_t out_data_len) {
    nvm_partition_assert(partition);
    if (!fits(offset, out_data_len)) {
        return -1;
    }
    return BSP_OSPI_NOR_Read(0, (uint8_t *) out_data,
                             partition_start(partition) + offset, out_data_len);
}

int nvm_partition_driver_write(nvm_partition_t partition,
                               size_t offset,
                               const void *data,
                               size_t data_len) {
    nvm_partition_assert(partition);
    if (!fits(offset, data_len)) {
        return -1;
    }
    return BSP_OSPI_NOR_Write(0, (uint8_t *) data,
                              partition_start(partition) + offset, data_len);
}

int nvm_partition_driver_clear(nvm_partition_t partition) {
    return BSP_OSPI_NOR_Erase_Block(0, partition_start(partition),
                                    BSP_OSPI_NOR_ERASE_64K);
}
