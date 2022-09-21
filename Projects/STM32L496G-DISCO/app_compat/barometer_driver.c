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
#define LOG(level, ...) avs_log(barometer_driver, level, __VA_ARGS__)

#include "sensor_drivers/basic_sensor_driver.h"
#include "x_nucleo_iks01a2.h"
#include "x_nucleo_iks01a2_pressure.h"

static void *g_sensor_barometer = NULL;

static int barometer_init(void) {
    uint8_t status = 0U;
    if (!BSP_PRESSURE_IsInitialized(g_sensor_barometer, &status)
            && status == 1U) {
        LOG(WARNING, "IKS01A2 barometer already initialized");
        return -1;
    }
    if (BSP_PRESSURE_Init(PRESSURE_SENSORS_AUTO, &g_sensor_barometer)) {
        LOG(WARNING, "IKS01A2 barometer could not be initialized");
        return -1;
    }
    if (BSP_PRESSURE_Sensor_Enable(g_sensor_barometer)) {
        LOG(ERROR, "IKS01A2 barometer could not be enabled");
        return -1;
    }
    return 0;
}

static int get_pressure(float *out_pressure) {
    if (!g_sensor_barometer) {
        LOG(ERROR, "IKS01A2 barometer not initialized");
        return -1;
    }
    float pressure;
    if (BSP_PRESSURE_Get_Press(g_sensor_barometer, &pressure)) {
        LOG(ERROR, "error getting current pressure");
        return -1;
    }
    // convert hPa to Pa
    *out_pressure = 100.0f * pressure;
    return 0;
}

const basic_sensor_driver_t BSP_BAROMETER_DRIVER = {
    .init = barometer_init,
    .read = get_pressure
};
