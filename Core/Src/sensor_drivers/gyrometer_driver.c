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

#include <avsystem/commons/avs_log.h>

#include "x_nucleo_iks01a2.h"
#include "x_nucleo_iks01a2_gyro.h"

#include "sensor_drivers/three_axis_sensor_driver.h"

#define LOG(level, ...) avs_log(gyrometer_driver, level, __VA_ARGS__)

static void *g_sensor_gyrometer = NULL;

static int gyrometer_init(void) {
    uint8_t status = 0U;
    if (!BSP_GYRO_IsInitialized(g_sensor_gyrometer, &status) && status == 1U) {
        LOG(WARNING, "IKS01A2 gyrometer already initialized");
        return -1;
    }
    if (BSP_GYRO_Init(GYRO_SENSORS_AUTO, &g_sensor_gyrometer)) {
        LOG(WARNING, "IKS01A2 gyrometer could not be initialized");
        return -1;
    }
    if (BSP_GYRO_Sensor_Enable(g_sensor_gyrometer)) {
        LOG(ERROR, "IKS01A2 gyrometer could not be enabled");
        return -1;
    }
    return 0;
}

static int
get_angular_velocity(three_axis_sensor_values_t *out_angular_velocity) {
    if (!g_sensor_gyrometer) {
        LOG(ERROR, "IKS01A2 gyrometer not initialized");
        return -1;
    }
    SensorAxes_t angular_velocity;
    if (BSP_GYRO_Get_Axes(g_sensor_gyrometer, &angular_velocity)) {
        LOG(ERROR, "error getting current angular velocity");
        return -1;
    }
    // Convert from mdeg/s to deg/s
    *out_angular_velocity =
            three_axis_sensor_get_values_scaled(&angular_velocity, 0.001f);
    return 0;
}

const three_axis_sensor_driver_t IKS01A2_GYROMETER_DRIVER = {
    .init = gyrometer_init,
    .read = get_angular_velocity,
    .unit = "deg/s",
    .name = "gyrometer"
};
