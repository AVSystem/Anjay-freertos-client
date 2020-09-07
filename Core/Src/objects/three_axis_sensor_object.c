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

#include <assert.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_memory.h>

#include "three_axis_sensor_object.h"

#define THREE_AXIS_SENSOR_OBJ_LOG(...) \
    avs_log(three_axis_sensor_obj, __VA_ARGS__)

/**
 * Sensor Units: R, Single, Optional
 * type: string, range: N/A, unit: N/A
 * Measurement Units Definition.
 */
#define RID_SENSOR_UNITS 5701

/**
 * X Value: R, Single, Mandatory
 * type: float, range: N/A, unit: N/A
 * The measured value along the X axis.
 */
#define RID_X_VALUE 5702

/**
 * Y Value: R, Single, Optional
 * type: float, range: N/A, unit: N/A
 * The measured value along the Y axis.
 */
#define RID_Y_VALUE 5703

/**
 * Z Value: R, Single, Optional
 * type: float, range: N/A, unit: N/A
 * The measured value along the Z axis.
 */
#define RID_Z_VALUE 5704

#define THREE_AXIS_SENSOR_OBJ_DEF(Oid)                          \
    (anjay_dm_object_def_t) {                                   \
        .oid = (Oid),                                           \
        .handlers = {                                           \
            .list_instances = anjay_dm_list_instances_SINGLE,   \
            .list_resources = three_axis_sensor_list_resources, \
            .resource_read = three_axis_sensor_resource_read    \
        }                                                       \
    }

static inline three_axis_sensor_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, three_axis_sensor_object_t, def_ptr);
}

static int
three_axis_sensor_list_resources(anjay_t *anjay,
                                 const anjay_dm_object_def_t *const *obj_ptr,
                                 anjay_iid_t iid,
                                 anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_SENSOR_UNITS, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_X_VALUE, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_Y_VALUE, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_Z_VALUE, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    return 0;
}

static int
three_axis_sensor_resource_read(anjay_t *anjay,
                                const anjay_dm_object_def_t *const *obj_ptr,
                                anjay_iid_t iid,
                                anjay_rid_t rid,
                                anjay_riid_t riid,
                                anjay_output_ctx_t *ctx) {
    (void) anjay;

    three_axis_sensor_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);
    assert(riid == ANJAY_ID_INVALID);

    switch (rid) {
    case RID_SENSOR_UNITS:
        return anjay_ret_string(ctx, obj->driver->unit);

    case RID_X_VALUE:
        return anjay_ret_float(ctx, obj->values.x);

    case RID_Y_VALUE:
        return anjay_ret_float(ctx, obj->values.y);

    case RID_Z_VALUE:
        return anjay_ret_float(ctx, obj->values.z);

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int read_values(const three_axis_sensor_driver_t *driver,
                       three_axis_sensor_values_t *out_values) {
    if (driver->read(out_values)) {
        THREE_AXIS_SENSOR_OBJ_LOG(ERROR, "Failed to read from %s",
                                  driver->name);
        return -1;
    }
    return 0;
}

int three_axis_sensor_object_install(anjay_t *anjay,
                                     const three_axis_sensor_driver_t *driver,
                                     anjay_oid_t oid,
                                     three_axis_sensor_object_t *obj) {
    if (driver->init()) {
        return -1;
    }

    obj->def = THREE_AXIS_SENSOR_OBJ_DEF(oid);
    obj->def_ptr = &obj->def;
    obj->driver = driver;
    return anjay_register_object(anjay, &obj->def_ptr);
}

void three_axis_sensor_object_update(anjay_t *anjay,
                                     three_axis_sensor_object_t *obj) {
    if (!obj->def_ptr) {
        return;
    }

    three_axis_sensor_values_t sensor_values;
    if (read_values(obj->driver, &sensor_values)) {
        return;
    }

    if (obj->values.x != sensor_values.x) {
        obj->values.x = sensor_values.x;
        (void) anjay_notify_changed(anjay, obj->def.oid, 0, RID_X_VALUE);
    }

    if (obj->values.y != sensor_values.y) {
        obj->values.y = sensor_values.y;
        (void) anjay_notify_changed(anjay, obj->def.oid, 0, RID_Y_VALUE);
    }

    if (obj->values.z != sensor_values.z) {
        obj->values.z = sensor_values.z;
        (void) anjay_notify_changed(anjay, obj->def.oid, 0, RID_Z_VALUE);
    }
}
