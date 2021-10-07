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

#include <anjay/anjay.h>
#include <anjay/ipso_objects.h>
#include <avsystem/commons/avs_defs.h>

#include "sensor_drivers/bsp_sensor_drivers.h"
#include "sensor_drivers/three_axis_sensor_driver.h"

typedef struct {
    anjay_oid_t oid;
    const three_axis_sensor_driver_t *driver;
} sensor_context_t;

static sensor_context_t three_axis_sensors_def[] = {
    {
        .oid = 3313,
        .driver = &BSP_ACCELEROMETER_DRIVER
    },
    {
        .oid = 3314,
        .driver = &BSP_MAGNETOMETER_DRIVER
    },
    {
        .oid = 3334,
        .driver = &BSP_GYROMETER_DRIVER
    }
};

static int read_values(anjay_iid_t iid,
                       void *_ctx,
                       double *x_value,
                       double *y_value,
                       double *z_value) {
    const three_axis_sensor_driver_t *driver =
            ((sensor_context_t *) _ctx)->driver;

    three_axis_sensor_values_t out_values;
    if (driver->read(&out_values)) {
        return -1;
    }

    *x_value = (double) out_values.x;
    *y_value = (double) out_values.y;
    *z_value = (double) out_values.z;

    return 0;
}

void three_axis_sensor_objects_install(anjay_t *anjay) {
    for (int i = 0; i < AVS_ARRAY_SIZE(three_axis_sensors_def); i++) {
        sensor_context_t *ctx = &three_axis_sensors_def[i];

        if (ctx->driver->init()
                || anjay_ipso_3d_sensor_install(anjay, ctx->oid, 1)) {
            continue;
        }

        anjay_ipso_3d_sensor_instance_add(anjay,
                                          ctx->oid,
                                          0,
                                          (anjay_ipso_3d_sensor_impl_t) {
                                              .unit = ctx->driver->unit,
                                              .use_y_value = true,
                                              .use_z_value = true,
                                              .user_context = ctx,
                                              .min_range_value = NAN,
                                              .max_range_value = NAN,
                                              .get_values = read_values
                                          });
    }
}

void three_axis_sensor_objects_update(anjay_t *anjay) {
    for (int i = 0; i < AVS_ARRAY_SIZE(three_axis_sensors_def); i++) {
        anjay_ipso_3d_sensor_update(anjay, three_axis_sensors_def[i].oid, 0);
    }
}
