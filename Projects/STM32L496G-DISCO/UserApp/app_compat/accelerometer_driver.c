/*
 * Copyright 2020-2024 AVSystem <avsystem@avsystem.com>
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
#include "x_nucleo_iks01a2.h"
#include "x_nucleo_iks01a2_accelero.h"

static void *g_sensor_accelerometer = NULL;

static int accelerometer_init(void) {
    uint8_t status = 0U;
    if (!BSP_ACCELERO_IsInitialized(g_sensor_accelerometer, &status)
            && status == 1U) {
        LOG(WARNING, "IKS01A2 accelerometer already initialized");
        return -1;
    }
    if (BSP_ACCELERO_Init(ACCELERO_SENSORS_AUTO, &g_sensor_accelerometer)) {
        LOG(WARNING, "IKS01A2 accelerometer could not be initialized");
        return -1;
    }
    if (BSP_ACCELERO_Sensor_Enable(g_sensor_accelerometer)) {
        LOG(ERROR, "IKS01A2 accelerometer could not be enabled");
        return -1;
    }
    return 0;
}

static int get_acceleration(three_axis_sensor_values_t *out_acceleration) {
    if (!g_sensor_accelerometer) {
        LOG(ERROR, "IKS01A2 accelerometer not initialized");
        return -1;
    }
    SensorAxes_t acceleration;
    if (BSP_ACCELERO_Get_Axes(g_sensor_accelerometer, &acceleration)) {
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
