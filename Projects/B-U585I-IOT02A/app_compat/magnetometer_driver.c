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

#include <avsystem/commons/avs_log.h>
#define LOG(level, ...) avs_log(magnetometer_driver, level, __VA_ARGS__)

#include "b_u585i_iot02a.h"
#include "b_u585i_iot02a_motion_sensors.h"
#include "sensor_drivers/three_axis_sensor_driver.h"

static bool g_sensor_magnetometer;
static int magnetometer_init(void) {
    if (BSP_MOTION_SENSOR_Init(1, MOTION_MAGNETO)
            || BSP_MOTION_SENSOR_Enable(1, MOTION_MAGNETO)) {
        LOG(ERROR, "Magnetometer initialization failed");
        return -1;
    }
    g_sensor_magnetometer = true;
    return 0;
}

static int get_magnetism(three_axis_sensor_values_t *out_magnetism) {
    if (!g_sensor_magnetometer) {
        LOG(ERROR, "Magnetometer not initialized");
        return -1;
    }

    BSP_MOTION_SENSOR_Axes_t axes;
    if (BSP_MOTION_SENSOR_GetAxes(1, MOTION_MAGNETO, &axes)) {
        LOG(ERROR, "Couldn't read magnetism");
        return -1;
    }
    // Convert from mG to T
    *out_magnetism = three_axis_sensor_get_values_scaled(&axes, 1e-7f);
    return 0;
}

const three_axis_sensor_driver_t BSP_MAGNETOMETER_DRIVER = {
    .init = magnetometer_init,
    .read = get_magnetism
};
