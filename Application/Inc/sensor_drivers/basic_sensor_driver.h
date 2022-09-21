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

#ifndef BASIC_SENSOR_DRIVER_H
#define BASIC_SENSOR_DRIVER_H

#if defined(USE_STM32L496G_DISCO)
#include "sensor.h"
#endif

typedef struct {
    int (*init)(void);
    int (*read)(float *);
} basic_sensor_driver_t;

#endif // BASIC_SENSOR_DRIVER_H
