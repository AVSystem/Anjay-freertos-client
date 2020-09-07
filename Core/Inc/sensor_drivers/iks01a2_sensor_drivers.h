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

#ifndef IKS01A2_SENSOR_DRIVERS_H
#define IKS01A2_SENSOR_DRIVERS_H

#include "basic_sensor_driver.h"
#include "three_axis_sensor_driver.h"

extern const three_axis_sensor_driver_t IKS01A2_ACCELEROMETER_DRIVER;
extern const three_axis_sensor_driver_t IKS01A2_GYROMETER_DRIVER;
extern const three_axis_sensor_driver_t IKS01A2_MAGNETOMETER_DRIVER;

extern const basic_sensor_driver_t IKS01A2_THERMOMETER_DRIVER;
extern const basic_sensor_driver_t IKS01A2_HYGROMETER_DRIVER;
extern const basic_sensor_driver_t IKS01A2_BAROMETER_DRIVER;

#endif // IKS01A2_SENSOR_DRIVERS_H
