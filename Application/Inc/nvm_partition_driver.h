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

#ifndef NVM_PARTITION_DRIVER_H
#define NVM_PARTITION_DRIVER_H

#include <nvm_partition.h>

int nvm_partition_driver_read(nvm_partition_t partition,
                              size_t offset,
                              void *out_data,
                              size_t out_data_len);
int nvm_partition_driver_write(nvm_partition_t partition,
                               size_t offset,
                               const void *data,
                               size_t data_len);
int nvm_partition_driver_clear(nvm_partition_t partition);

#endif // NVM_PARTITION_DRIVER_H
