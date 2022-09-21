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

#include "b_u585i_iot02a.h"
#include "b_u585i_iot02a_env_sensors.h"
#include "sensor_drivers/basic_sensor_driver.h"

static bool g_sensor_thermometer;
static int thermometer_init(void) {
    if (BSP_ENV_SENSOR_Init(0, ENV_TEMPERATURE)
            || BSP_ENV_SENSOR_Enable(0, ENV_TEMPERATURE)) {
        LOG(ERROR, "Thermometer initialization failed");
        return -1;
    }
    g_sensor_thermometer = true;
    return 0;
}

static int get_temperature(float *out_temperature) {
    if (!g_sensor_thermometer) {
        LOG(ERROR, "Thermometer not initialized");
        return -1;
    }
    if (BSP_ENV_SENSOR_GetValue(0, ENV_TEMPERATURE, out_temperature)) {
        LOG(ERROR, "Couldn't read temperature");
        return -1;
    }
    return 0;
}

const basic_sensor_driver_t BSP_THERMOMETER_DRIVER = {
    .init = thermometer_init,
    .read = get_temperature
};
