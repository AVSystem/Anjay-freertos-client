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

#ifndef THREE_AXIS_SENSOR_DRIVER_H
#define THREE_AXIS_SENSOR_DRIVER_H

#include "main.h"

#define G_TO_MS2 9.80665

typedef struct {
    float x;
    float y;
    float z;
} three_axis_sensor_values_t;

typedef struct {
    int (*init)(void);
    int (*read)(three_axis_sensor_values_t *);
} three_axis_sensor_driver_t;

static inline three_axis_sensor_values_t
three_axis_sensor_get_values_scaled(const bsp_axes_t *axes, float coeff) {
    return (three_axis_sensor_values_t) {
        .x = (float) axes->BSP_AXIS_X * coeff,
        .y = (float) axes->BSP_AXIS_Y * coeff,
        .z = (float) axes->BSP_AXIS_Z * coeff
    };
}

static inline three_axis_sensor_values_t
three_axis_sensor_get_values(const bsp_axes_t *axes) {
    return three_axis_sensor_get_values_scaled(axes, 1.0f);
}

#endif // THREE_AXIS_SENSOR_DRIVER_H
