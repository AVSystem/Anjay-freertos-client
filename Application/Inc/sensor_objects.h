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

#ifndef SENSOR_OBJECTS_H
#define SENSOR_OBJECTS_H

#include <anjay/dm.h>

int accelerometer_object_install(anjay_t *anjay);
void accelerometer_object_update(anjay_t *anjay);

int gyrometer_object_install(anjay_t *anjay);
void gyrometer_object_update(anjay_t *anjay);

int magnetometer_object_install(anjay_t *anjay);
void magnetometer_object_update(anjay_t *anjay);

int temperature_object_install(anjay_t *anjay);
void temperature_object_update(anjay_t *anjay);

int humidity_object_install(anjay_t *anjay);
void humidity_object_update(anjay_t *anjay);

int barometer_object_install(anjay_t *anjay);
void barometer_object_update(anjay_t *anjay);

#endif // SENSOR_OBJECTS_H
