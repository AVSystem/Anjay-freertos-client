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
#include "x_nucleo_iks01a2_humidity.h"

#include "sensor_drivers/basic_sensor_driver.h"

#define LOG(level, ...) avs_log(hygrometer_driver, level, __VA_ARGS__)

static void *g_sensor_hygrometer = NULL;

static int hygrometer_init(void) {
    uint8_t status = 0U;
    if (!BSP_HUMIDITY_IsInitialized(g_sensor_hygrometer, &status)
            && status == 1U) {
        LOG(WARNING, "IKS01A2 hygrometer already initialized");
        return -1;
    }
    if (BSP_HUMIDITY_Init(HUMIDITY_SENSORS_AUTO, &g_sensor_hygrometer)) {
        LOG(WARNING, "IKS01A2 hygrometer could not be initialized");
        return -1;
    }
    if (BSP_HUMIDITY_Sensor_Enable(g_sensor_hygrometer)) {
        LOG(ERROR, "IKS01A2 hygrometer could not be enabled");
        return -1;
    }
    return 0;
}

static int get_humidity(float *out_humidity) {
    if (!g_sensor_hygrometer) {
        LOG(ERROR, "IKS01A2 hygrometer not initialized");
        return -1;
    }
    if (BSP_HUMIDITY_Get_Hum(g_sensor_hygrometer, out_humidity)) {
        LOG(ERROR, "error getting current humidity");
        return -1;
    }
    return 0;
}

const basic_sensor_driver_t IKS01A2_HYGROMETER_DRIVER = {
    .init = hygrometer_init,
    .read = get_humidity,
    .unit = "%",
    .name = "hygrometer"
};
