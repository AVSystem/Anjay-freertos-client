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

/**
 * Generated by anjay_codegen.py on 2021-08-24 13:22:53
 *
 * LwM2M Object: Anomaly detector
 * ID: 33651, URN: urn:oma:lwm2m:ext:33651, Optional, Single
 *
 * This object is used to report the anomaly detected by the ML-based
 * algorithms and to count the number of times it has been detected.
 */
#ifdef USE_AIBP

#include <assert.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_memory.h>
#include <avsystem/commons/avs_mutex.h>

/**
 * Anomaly State: R, Single, Mandatory
 * type: boolean, range: N/A, unit: N/A
 * The current state of a detector.
 */
#define RID_ANOMALY_STATE 2000

/**
 * Anomaly Counter: R, Single, Mandatory
 * type: integer, range: N/A, unit: N/A
 * The cumulative value of anomalies detected.
 */
#define RID_ANOMALY_COUNTER 2001

/**
 * Anomaly Treshold: RW, Single, Optional
 * type: float, range: N/A, unit: N/A
 * Input data is treated as anomaly if anomaly score is higher that
 * threshold value.
 */
#define RID_ANOMALY_TRESHOLD 2002

typedef struct anomaly_detector_object_struct {
    const anjay_dm_object_def_t *def;

    float anomaly_score;

    float anomaly_treshold;

    avs_mutex_t *instance_state_mtx;

    bool detector_state;
    int32_t detector_counter;

    bool detector_cached_state;
    int32_t detector_cached_counter;
} anomaly_detector_object_t;

#define ANOMALY_TRESHOLD_INIT 1.0f

static inline anomaly_detector_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, anomaly_detector_object_t, def);
}

static int instance_reset(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid) {
    (void) anjay;

    anomaly_detector_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    return 0;
}

static int list_resources(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_ANOMALY_STATE, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_ANOMALY_COUNTER, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_ANOMALY_TRESHOLD, ANJAY_DM_RES_RW,
                      ANJAY_DM_RES_PRESENT);
    return 0;
}

static int resource_read(anjay_t *anjay,
                         const anjay_dm_object_def_t *const *obj_ptr,
                         anjay_iid_t iid,
                         anjay_rid_t rid,
                         anjay_riid_t riid,
                         anjay_output_ctx_t *ctx) {
    (void) anjay;

    anomaly_detector_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    /*
     * Implementation is lock-free as it was assumed that:
     * 1) reading the object state is atomic and
     * 2) the conditional statements are not used in the function.
     */

    switch (rid) {
    case RID_ANOMALY_STATE:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_bool(ctx, obj->detector_state);

    case RID_ANOMALY_COUNTER:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_i32(ctx, obj->detector_counter);

    case RID_ANOMALY_TRESHOLD:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_float(ctx, obj->anomaly_treshold);

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int resource_write(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_rid_t rid,
                          anjay_riid_t riid,
                          anjay_input_ctx_t *ctx) {
    (void) anjay;

    anomaly_detector_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    switch (rid) {
    case RID_ANOMALY_TRESHOLD: {
        assert(riid == ANJAY_ID_INVALID);

        return anjay_get_float(ctx, &obj->anomaly_treshold);
    }

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static const anjay_dm_object_def_t OBJ_DEF = {
    .oid = 33651,
    .handlers = {
        .list_instances = anjay_dm_list_instances_SINGLE,
        .instance_reset = instance_reset,

        .list_resources = list_resources,
        .resource_read = resource_read,
        .resource_write = resource_write,

        .transaction_begin = anjay_dm_transaction_NOOP,
        .transaction_validate = anjay_dm_transaction_NOOP,
        .transaction_commit = anjay_dm_transaction_NOOP,
        .transaction_rollback = anjay_dm_transaction_NOOP
    }
};

static anomaly_detector_object_t ANOMALY_DETECTOR_OBJECT = {
    .def = &OBJ_DEF
};

static const anjay_dm_object_def_t **OBJ_DEF_PTR = &ANOMALY_DETECTOR_OBJECT.def;

int anomaly_detector_object_install(anjay_t *anjay) {
    ANOMALY_DETECTOR_OBJECT.anomaly_treshold = ANOMALY_TRESHOLD_INIT;

    avs_mutex_create(&ANOMALY_DETECTOR_OBJECT.instance_state_mtx);

    return anjay_register_object(anjay, OBJ_DEF_PTR);
}

void anomaly_detector_object_update(anjay_t *anjay) {
    if (!OBJ_DEF_PTR) {
        return;
    }

    anomaly_detector_object_t *obj = &ANOMALY_DETECTOR_OBJECT;

    avs_mutex_lock(obj->instance_state_mtx);

    if (obj->detector_cached_state != obj->detector_state) {
        obj->detector_cached_state = obj->detector_state;
        anjay_notify_changed(anjay, obj->def->oid, 0, RID_ANOMALY_STATE);
    }

    if (obj->detector_cached_counter != obj->detector_counter) {
        obj->detector_cached_counter = obj->detector_counter;
        anjay_notify_changed(anjay, obj->def->oid, 0, RID_ANOMALY_COUNTER);
    }

    avs_mutex_unlock(obj->instance_state_mtx);
}

void anomaly_detector_object_value_update(float anomaly) {
    anomaly_detector_object_t *obj = get_obj(OBJ_DEF_PTR);

    avs_mutex_lock(obj->instance_state_mtx);

    obj->anomaly_score = anomaly;

    if (anomaly > obj->anomaly_treshold) {
        obj->detector_counter++;
        obj->detector_state = true;
    } else {
        obj->detector_state = false;
    }

    avs_mutex_unlock(obj->instance_state_mtx);
}

#endif