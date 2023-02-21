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

#ifndef DEFAULT_CONFIG_H
#define DEFAULT_CONFIG_H

#define DEFAULT_SERVER_URI "coaps://eu.iot.avsystem.cloud:5684"
#define DEFAULT_PSK "psk"
#define DEFAULT_BOOTSTRAP "n"
#define DEFAULT_SECURITY "psk"
#define DEFAULT_APN "internet"
#define DEFAULT_APN_USERNAME "internet"
#define DEFAULT_APN_PASSWORD "internet"
#define FIRMWARE_VERSION "23.02"

void generate_default_endpoint_name(char *endpoint_name, size_t len);

#endif // DEFAULT_CONFIG_H
