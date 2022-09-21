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
#define LOG(level, ...) avs_log(thermometer_driver, level, __VA_ARGS__)

#include "sensor_drivers/basic_sensor_driver.h"
#include "x_nucleo_iks01a2.h"
#include "x_nucleo_iks01a2_temperature.h"

static void *g_sensor_thermometer = NULL;

static int thermometer_init(void) {
    uint8_t status = 0U;
    if (!BSP_TEMPERATURE_IsInitialized(g_sensor_thermometer, &status)
            && status == 1U) {
        LOG(WARNING, "IKS01A2 thermometer already initialized");
        return -1;
    }
    if (BSP_TEMPERATURE_Init(TEMPERATURE_SENSORS_AUTO, &g_sensor_thermometer)) {
        LOG(WARNING, "IKS01A2 thermometer could not be initialized");
        return -1;
    }
    if (BSP_TEMPERATURE_Sensor_Enable(g_sensor_thermometer)) {
        LOG(ERROR, "IKS01A2 thermometer could not be enabled");
        return -1;
    }
    return 0;
}

static int get_temperature(float *out_temperature) {
    if (!g_sensor_thermometer) {
        LOG(ERROR, "IKS01A2 thermometer not initialized");
        return -1;
    }
    if (BSP_TEMPERATURE_Get_Temp(g_sensor_thermometer, out_temperature)) {
        LOG(ERROR, "error getting current temperature");
        return -1;
    }
    return 0;
}

const basic_sensor_driver_t BSP_THERMOMETER_DRIVER = {
    .init = thermometer_init,
    .read = get_temperature
};
