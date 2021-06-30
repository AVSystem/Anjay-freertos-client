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
#define LOG(level, ...) avs_log(gyrometer_driver, level, __VA_ARGS__)

#include "sensor_drivers/three_axis_sensor_driver.h"

static int gyrometer_init(void) {
    LOG(ERROR, "Gyrometer not supported");
    return -1;
}

static int get_angular_velocity(three_axis_sensor_values_t *out_acceleration) {
    LOG(ERROR, "Gyrometer not supported");
    return -1;
}

const three_axis_sensor_driver_t BSP_GYROMETER_DRIVER = {
    .init = gyrometer_init,
    .read = get_angular_velocity,
    .unit = "deg/s",
    .name = "gyrometer"
};
