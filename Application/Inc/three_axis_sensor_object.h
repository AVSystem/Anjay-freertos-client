/*
 * Copyright 2020-2021 AVSystem <avsystem@avsystem.com>
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

#ifndef THREE_AXIS_SENSOR_OBJECT_H
#define THREE_AXIS_SENSOR_OBJECT_H

#include "sensor_drivers/three_axis_sensor_driver.h"

typedef struct {
    const anjay_dm_object_def_t *def_ptr;
    anjay_dm_object_def_t def;

    const three_axis_sensor_driver_t *driver;
    three_axis_sensor_values_t values;
} three_axis_sensor_object_t;

int three_axis_sensor_object_install(anjay_t *anjay,
                                     const three_axis_sensor_driver_t *driver,
                                     anjay_oid_t oid,
                                     three_axis_sensor_object_t *obj);

void three_axis_sensor_object_update(anjay_t *anjay,
                                     three_axis_sensor_object_t *obj);

#endif // THREE_AXIS_SENSOR_OBJECT_H
