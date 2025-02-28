/*
 * Copyright 2020-2025 AVSystem <avsystem@avsystem.com>
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

#include "sensor_drivers/basic_sensor_driver.h"
#include "sensor_drivers/bsp_sensor_drivers.h"

typedef struct {
    anjay_oid_t oid;
    const basic_sensor_driver_t *driver;
    const char *unit;
    bool installed;
} sensor_context_t;

static sensor_context_t basic_sensors_def[] = {
    {
        .oid = 3303,
        .driver = &BSP_THERMOMETER_DRIVER,
        .unit = "Cel"
    },
    {
        .oid = 3304,
        .driver = &BSP_HYGROMETER_DRIVER,
        .unit = "%RH"
    },
    {
        .oid = 3315,
        .driver = &BSP_BAROMETER_DRIVER,
        .unit = "Pa"
    }
};

static int read_value(anjay_iid_t iid, void *_ctx, double *out_value) {
    const basic_sensor_driver_t *driver = ((sensor_context_t *) _ctx)->driver;

    float fvalue;
    if (driver->read(&fvalue)) {
        return -1;
    }

    *out_value = (double) fvalue;

    return 0;
}

void basic_sensor_objects_install(anjay_t *anjay) {
    for (int i = 0; i < AVS_ARRAY_SIZE(basic_sensors_def); i++) {
        sensor_context_t *ctx = &basic_sensors_def[i];

        if (ctx->driver->init()
                || anjay_ipso_basic_sensor_install(anjay, ctx->oid, 1)) {
            continue;
        }

        ctx->installed = !anjay_ipso_basic_sensor_instance_add(
                anjay, ctx->oid, 0,
                (anjay_ipso_basic_sensor_impl_t) {
                    .unit = ctx->unit,
                    .user_context = ctx,
                    .min_range_value = NAN,
                    .max_range_value = NAN,
                    .get_value = read_value
                });
    }
}

void basic_sensor_objects_update(anjay_t *anjay) {
    for (int i = 0; i < AVS_ARRAY_SIZE(basic_sensors_def); i++) {
        sensor_context_t *ctx = &basic_sensors_def[i];

        if (ctx->installed) {
            anjay_ipso_basic_sensor_update(anjay, ctx->oid, 0);
        }
    }
}
