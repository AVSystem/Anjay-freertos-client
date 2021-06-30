/*
 * Copyright 2020-2021 AVSystem <avsystem@avsystem.com>
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
#include "stm32l462e_cell1_env_sensors.h"

static bool g_sensor_barometer;
static int barometer_init(void) {
    if (BSP_ENV_SENSOR_Init_Pressure()) {
        LOG(ERROR, "Barometer initialization failed");
        return -1;
    }
    g_sensor_barometer = true;
    return 0;
}

static int get_pressure(float *out_pressure) {
    if (!g_sensor_barometer) {
        LOG(ERROR, "Barometer not initialized");
        return -1;
    }
    if (BSP_ENV_SENSOR_Read_Pressure(out_pressure)) {
        LOG(ERROR, "Error getting current pressure");
        return -1;
    }
    return 0;
}

const basic_sensor_driver_t BSP_BAROMETER_DRIVER = {
    .init = barometer_init,
    .read = get_pressure,
    .unit = "hPa",
    .name = "barometer"
};
