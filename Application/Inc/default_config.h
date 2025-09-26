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

#ifndef DEFAULT_CONFIG_H
#define DEFAULT_CONFIG_H

#define DEFAULT_SERVER_URI "coaps://eu.iot.avsystem.cloud:5684"
#define DEFAULT_PSK "psk"
#define DEFAULT_BOOTSTRAP "n"
#define DEFAULT_SECURITY "psk"
#define DEFAULT_APN "internet"
#define DEFAULT_APN_USERNAME "internet"
#define DEFAULT_APN_PASSWORD "internet"
#define DEFAULT_USE_SMS_TRIGGER "y"
#define DEFAULT_LOCAL_MSISDN "48000000000"
#define DEFAULT_SERVER_MSISDN "48000000000"
#define DEFAULT_USE_PERSISTENCE "0"
#ifdef USE_SIM_BOOTSTRAP
#define DEFAULT_USE_SIM_BOOTSTRAP "y"
#else // USE_SIM_BOOTSTRAP
#define DEFAULT_USE_SIM_BOOTSTRAP "n"
#endif // USE_SIM_BOOTSTRAP
#define FIRMWARE_VERSION "26.09"
#define MD5_OF_ZERO_BYTES_INITIALIZER                                     \
    {                                                                     \
        0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04, 0xE9, 0x80, 0x09, \
                0x98, 0xEC, 0xF8, 0x42, 0x7E                              \
    }
#define MD5_OF_ZERO_BYTES \
    ((const char[AVS_COMMONS_MD5_LENGTH]) MD5_OF_ZERO_BYTES_INITIALIZER)

void generate_default_endpoint_name(char *endpoint_name, size_t len);

#endif // DEFAULT_CONFIG_H
