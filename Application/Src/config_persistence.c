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

#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_persistence.h>

#include <string.h>

#include <config_persistence.h>
#include <nvm_partition.h>

#define LOG(level, ...) avs_log(config, level, __VA_ARGS__)

static int load_config_from_flash(config_t *out_config) {
    avs_stream_t *stream;
    if (nvm_partition_stream_input_open(NVM_PARTITION_CONFIG, &stream)
            || !stream) {
        return -1;
    }
    avs_error_t read_err =
            avs_stream_read_reliably(stream, out_config, sizeof(*out_config));

    avs_error_t cleanup_err = avs_stream_cleanup(&stream);
    return avs_is_ok(read_err) && avs_is_ok(cleanup_err) ? 0 : -1;
}

int config_save(const config_t *in_config) {
    avs_stream_t *stream;
    if (nvm_partition_stream_output_open(NVM_PARTITION_CONFIG, &stream)
            || !stream) {
        return -1;
    }
    avs_error_t write_err =
            avs_stream_write(stream, in_config, sizeof(*in_config));
    avs_error_t cleanup_err = avs_stream_cleanup(&stream);
    if (avs_is_err(write_err) || avs_is_err(cleanup_err)) {
        return -1;
    }

    return nvm_partition_mark_valid(NVM_PARTITION_CONFIG);
}

int config_load(config_t *out_config) {
    config_t loaded_config;
    if (load_config_from_flash(&loaded_config)) {
        return -1;
    }
    *out_config = loaded_config;
    return 0;
}
