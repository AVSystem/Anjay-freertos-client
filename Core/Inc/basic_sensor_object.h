/*
 * Copyright 2020 AVSystem <avsystem@avsystem.com>
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

#ifndef BASIC_SENSOR_OBJECT_H
#define BASIC_SENSOR_OBJECT_H

#include "sensor_drivers/basic_sensor_driver.h"

typedef struct {
    const anjay_dm_object_def_t *def_ptr;
    anjay_dm_object_def_t def;

    const basic_sensor_driver_t *driver;
    float current_value;
    float min_value;
    float max_value;
} basic_sensor_object_t;

int basic_sensor_object_install(anjay_t *anjay,
                                const basic_sensor_driver_t *driver,
                                anjay_oid_t oid,
                                basic_sensor_object_t *obj);

void basic_sensor_object_update(anjay_t *anjay, basic_sensor_object_t *obj);

#endif // BASIC_SENSOR_OBJECT_H
