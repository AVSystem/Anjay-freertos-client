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
 * Generated by anjay_codegen.py on 2021-08-24 13:23:11
 *
 * LwM2M Object: ML Model
 * ID: 33654, URN: urn:oma:lwm2m:ext:33654, Optional, Single
 *
 * This object is used to report the meta information of the ML model
 * used by the device.
 */
#ifdef USE_AIBP

#include <assert.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_memory.h>

#include "ai_bridge.h"

/**
 * Model Name: R, Single, Optional
 * type: string, range: N/A, unit: N/A
 * ML model name.
 */
#define RID_MODEL_NAME 2000

/**
 * Model version: R, Single, Optional
 * type: integer, range: N/A, unit: N/A
 * ML model version.
 */
#define RID_MODEL_VERSION 2001

/**
 * Learn: E, Single, Optional
 * type: N/A, range: N/A, unit: N/A
 * Perform online learning.
 */
#define RID_LEARN 2002

typedef struct ml_model_object_struct {
    const anjay_dm_object_def_t *def;

} ml_model_object_t;

static inline ml_model_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, ml_model_object_t, def);
}

static int list_resources(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_MODEL_NAME, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_MODEL_VERSION, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);

    if (ai_bridge_type == AI_BRIDGE_ANOMALY_TYPE) {
        anjay_dm_emit_res(ctx, RID_LEARN, ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT);
    }
    return 0;
}

static int resource_read(anjay_t *anjay,
                         const anjay_dm_object_def_t *const *obj_ptr,
                         anjay_iid_t iid,
                         anjay_rid_t rid,
                         anjay_riid_t riid,
                         anjay_output_ctx_t *ctx) {
    (void) anjay;

    ml_model_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    switch (rid) {
    case RID_MODEL_NAME:
        assert(riid == ANJAY_ID_INVALID);
        if (ai_bridge_type == AI_BRIDGE_ANOMALY_TYPE) {
            return anjay_ret_string(ctx, "Anomaly Detector");
        } else {
            return anjay_ret_string(ctx, "Classificator");
        }

    case RID_MODEL_VERSION:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_i32(ctx, 1);

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int resource_execute(anjay_t *anjay,
                            const anjay_dm_object_def_t *const *obj_ptr,
                            anjay_iid_t iid,
                            anjay_rid_t rid,
                            anjay_execute_ctx_t *arg_ctx) {
    (void) arg_ctx;

    ml_model_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid == 0);

    switch (rid) {
    case RID_LEARN:
        ai_bridge_learn();
        return 0;

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static const anjay_dm_object_def_t OBJ_DEF = {
    .oid = 33654,
    .handlers = {
        .list_instances = anjay_dm_list_instances_SINGLE,

        .list_resources = list_resources,
        .resource_read = resource_read,
        .resource_execute = resource_execute,

        .transaction_begin = anjay_dm_transaction_NOOP,
        .transaction_validate = anjay_dm_transaction_NOOP,
        .transaction_commit = anjay_dm_transaction_NOOP,
        .transaction_rollback = anjay_dm_transaction_NOOP
    }
};

static ml_model_object_t ML_MODEL_OBJECT = {
    .def = &OBJ_DEF
};

static const anjay_dm_object_def_t **OBJ_DEF_PTR = &ML_MODEL_OBJECT.def;

int ml_model_object_install(anjay_t *anjay) {
    return anjay_register_object(anjay, OBJ_DEF_PTR);
}

void ml_model_object_update(anjay_t *anjay) {
    if (!OBJ_DEF_PTR) {
        return;
    }
}

#endif