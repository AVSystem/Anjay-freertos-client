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

#include <assert.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_memory.h>

#include "basic_sensor_object.h"

#define BASIC_SENSOR_OBJ_LOG(...) avs_log(basic_sensor_obj, __VA_ARGS__)

/**
 * Min Measured Value: R, Single, Optional
 * type: float, range: N/A, unit: N/A
 * The minimum value measured by the sensor since power ON or reset.
 */
#define RID_MIN_MEASURED_VALUE 5601

/**
 * Max Measured Value: R, Single, Optional
 * type: float, range: N/A, unit: N/A
 * The maximum value measured by the sensor since power ON or reset.
 */
#define RID_MAX_MEASURED_VALUE 5602

/**
 * Reset Min and Max Measured Values: E, Single, Optional
 * type: N/A, range: N/A, unit: N/A
 * Reset the Min and Max Measured Values to Current Value.
 */
#define RID_RESET_MIN_AND_MAX_MEASURED_VALUES 5605

/**
 * Sensor Value: R, Single, Mandatory
 * type: float, range: N/A, unit: N/A
 * Last or Current Measured Value from the Sensor.
 */
#define RID_SENSOR_VALUE 5700

/**
 * Sensor Units: R, Single, Optional
 * type: string, range: N/A, unit: N/A
 * Measurement Units Definition.
 */
#define RID_SENSOR_UNITS 5701

#define BASIC_SENSOR_OBJ_DEF(Oid)                             \
    (anjay_dm_object_def_t) {                                 \
        .oid = (Oid),                                         \
        .handlers = {                                         \
            .list_instances = anjay_dm_list_instances_SINGLE, \
            .list_resources = basic_sensor_list_resources,    \
            .resource_read = basic_sensor_resource_read,      \
            .resource_execute = basic_sensor_resource_execute \
        }                                                     \
    }

static basic_sensor_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, basic_sensor_object_t, def_ptr);
}

static int
basic_sensor_list_resources(anjay_t *anjay,
                            const anjay_dm_object_def_t *const *obj_ptr,
                            anjay_iid_t iid,
                            anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_MIN_MEASURED_VALUE, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_MAX_MEASURED_VALUE, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_RESET_MIN_AND_MAX_MEASURED_VALUES,
                      ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_SENSOR_VALUE, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_SENSOR_UNITS, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    return 0;
}

static int
basic_sensor_resource_read(anjay_t *anjay,
                           const anjay_dm_object_def_t *const *obj_ptr,
                           anjay_iid_t iid,
                           anjay_rid_t rid,
                           anjay_riid_t riid,
                           anjay_output_ctx_t *ctx) {
    (void) anjay;

    basic_sensor_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    switch (rid) {
    case RID_MIN_MEASURED_VALUE:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_float(ctx, obj->min_value);

    case RID_MAX_MEASURED_VALUE:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_float(ctx, obj->max_value);

    case RID_SENSOR_VALUE:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_float(ctx, obj->current_value);

    case RID_SENSOR_UNITS:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_string(ctx, obj->driver->unit);

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int
basic_sensor_resource_execute(anjay_t *anjay,
                              const anjay_dm_object_def_t *const *obj_ptr,
                              anjay_iid_t iid,
                              anjay_rid_t rid,
                              anjay_execute_ctx_t *arg_ctx) {
    (void) arg_ctx;

    basic_sensor_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    switch (rid) {
    case RID_RESET_MIN_AND_MAX_MEASURED_VALUES:
        obj->max_value = obj->current_value;
        obj->min_value = obj->current_value;
        return 0;

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int read_value(const basic_sensor_driver_t *driver, float *out_value) {
    if (driver->read(out_value)) {
        BASIC_SENSOR_OBJ_LOG(ERROR, "Failed to read from %s", driver->name);
        return -1;
    }
    return 0;
}

int basic_sensor_object_install(anjay_t *anjay,
                                const basic_sensor_driver_t *driver,
                                anjay_oid_t oid,
                                basic_sensor_object_t *obj) {
    if (driver->init()) {
        return -1;
    }

    float initial_value;
    if (read_value(driver, &initial_value)) {
        BASIC_SENSOR_OBJ_LOG(ERROR, "Failed to initialize %s values",
                             driver->name);
        return -1;
    }

    obj->def = BASIC_SENSOR_OBJ_DEF(oid);
    obj->def_ptr = &obj->def;
    obj->driver = driver;
    obj->current_value = initial_value;
    obj->min_value = initial_value;
    obj->max_value = initial_value;

    return anjay_register_object(anjay, &obj->def_ptr);
}

void basic_sensor_object_update(anjay_t *anjay, basic_sensor_object_t *obj) {
    if (!obj->def_ptr) {
        return;
    }

    float sensor_value;
    if (read_value(obj->driver, &sensor_value)) {
        return;
    }

    if (obj->current_value != sensor_value) {
        obj->current_value = sensor_value;
        (void) anjay_notify_changed(anjay, obj->def.oid, 0, RID_SENSOR_VALUE);
    }

    if (obj->min_value > sensor_value) {
        obj->min_value = sensor_value;
        (void) anjay_notify_changed(anjay, obj->def.oid, 0,
                                    RID_MIN_MEASURED_VALUE);
    }

    if (obj->max_value < sensor_value) {
        obj->max_value = sensor_value;
        (void) anjay_notify_changed(anjay, obj->def.oid, 0,
                                    RID_MAX_MEASURED_VALUE);
    }
}
