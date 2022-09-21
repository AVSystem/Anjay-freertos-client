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
#define LOG(level, ...) avs_log(magnetometer_driver, level, __VA_ARGS__)

#include "sensor_drivers/three_axis_sensor_driver.h"
#include "x_nucleo_iks01a2.h"
#include "x_nucleo_iks01a2_magneto.h"

static void *g_sensor_magnetometer = NULL;

static int magnetometer_init(void) {
    uint8_t status = 0U;
    if (!BSP_MAGNETO_IsInitialized(g_sensor_magnetometer, &status)
            && status == 1U) {
        LOG(WARNING, "IKS01A2 magnetometer already initialized");
        return -1;
    }
    if (BSP_MAGNETO_Init(MAGNETO_SENSORS_AUTO, &g_sensor_magnetometer)) {
        LOG(WARNING, "IKS01A2 magnetometer could not be initialized");
        return -1;
    }
    if (BSP_MAGNETO_Sensor_Enable(g_sensor_magnetometer)) {
        LOG(ERROR, "IKS01A2 magnetometer could not be enabled");
        return -1;
    }
    return 0;
}

static int get_magnetism(three_axis_sensor_values_t *out_magnetism) {
    if (!g_sensor_magnetometer) {
        LOG(ERROR, "IKS01A2 magnetometer not initialized");
        return -1;
    }
    SensorAxes_t magnetism;
    if (BSP_MAGNETO_Get_Axes(g_sensor_magnetometer, &magnetism)) {
        LOG(ERROR, "error getting current magnetism");
        return -1;
    }
    // Convert from mG to T
    *out_magnetism = three_axis_sensor_get_values_scaled(&magnetism, 1e-7f);
    return 0;
}

const three_axis_sensor_driver_t BSP_MAGNETOMETER_DRIVER = {
    .init = magnetometer_init,
    .read = get_magnetism
};
