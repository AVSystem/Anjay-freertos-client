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

#include <avsystem/commons/avs_log.h>
#define LOG(level, ...) avs_log(accelerometer_driver, level, __VA_ARGS__)

#include "sensor_drivers/three_axis_sensor_driver.h"
#include "stm32l462e_cell1_motion_sensors.h"

static bool g_sensor_accelerometer;
static int accelerometer_init(void) {
    if (BSP_MOTION_SENSOR_Init_Acc()) {
        LOG(ERROR, "Accelerometer initialization failed");
        return -1;
    }
    g_sensor_accelerometer = true;
    return 0;
}

static int get_acceleration(three_axis_sensor_values_t *out_acceleration) {
    if (!g_sensor_accelerometer) {
        LOG(ERROR, "Accelerometer not initialized");
        return -1;
    }
    MOTION_SENSOR_Axes_t acceleration;
    if (BSP_MOTION_SENSOR_Read_Acc(&acceleration)) {
        LOG(ERROR, "error getting current acceleration");
        return -1;
    }
    // Convert from mg to m/s^2
    *out_acceleration =
            three_axis_sensor_get_values_scaled(&acceleration, 1e-3 * G_TO_MS2);
    return 0;
}

const three_axis_sensor_driver_t BSP_ACCELEROMETER_DRIVER = {
    .init = accelerometer_init,
    .read = get_acceleration
};
