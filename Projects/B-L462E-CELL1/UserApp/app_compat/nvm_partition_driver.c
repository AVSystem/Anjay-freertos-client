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

#include <nvm_partition_driver.h>

#include <assert.h>
#include <stdbool.h>

#include <avsystem/commons/avs_defs.h>

#include <stm32l462e_cell1_qspi_in_module.h>

#include <nvm_partition.h>

AVS_STATIC_ASSERT(sizeof(uint32_t) == sizeof(size_t), size_t_is_32_bit);

// we're using blocks 13, 14, 15 of 16 total (64K/each) of in-module QSPI flash
// to not interfere with FOTA secondary partition; also assume partition ==
// block

#define FIRST_BLOCK 13

AVS_STATIC_ASSERT(FIRST_BLOCK + _NVM_PARTITION_COUNT
                          <= W25Q80EW_FLASH_SIZE / W25Q80EW_BLOCK_SIZE,
                  partitions_fit);

static inline bool fits(size_t offset, size_t op_len) {
    return offset + op_len < W25Q80EW_BLOCK_SIZE;
}

static inline size_t partition_start(nvm_partition_t partition) {
    return (FIRST_BLOCK + partition) * W25Q80EW_BLOCK_SIZE;
}

int nvm_partition_driver_read(nvm_partition_t partition,
                              size_t offset,
                              void *out_data,
                              size_t out_data_len) {
    nvm_partition_assert(partition);
    if (!fits(offset, out_data_len)) {
        return -1;
    }
    return BSP_QSPI_Read((uint8_t *) out_data,
                         partition_start(partition) + offset,
                         out_data_len);
}

int nvm_partition_driver_write(nvm_partition_t partition,
                               size_t offset,
                               const void *data,
                               size_t data_len) {
    nvm_partition_assert(partition);
    if (!fits(offset, data_len)) {
        return -1;
    }
    return BSP_QSPI_Write(
            (uint8_t *) data, partition_start(partition) + offset, data_len);
}

int nvm_partition_driver_clear(nvm_partition_t partition) {
    nvm_partition_assert(partition);
    return BSP_QSPI_EraseBlock(partition_start(partition), BSP_QSPI_ERASE_64K);
}
