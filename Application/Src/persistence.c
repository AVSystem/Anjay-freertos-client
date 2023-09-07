/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
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

#include <persistence.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_stream.h>

#include <anjay/attr_storage.h>
#include <anjay/core.h>
#include <anjay/security.h>
#include <anjay/server.h>

#include <nvm_partition.h>

#define LOG(Level, ...) avs_log(persistence, Level, __VA_ARGS__)

typedef avs_error_t restore_fn_t(anjay_t *anjay, avs_stream_t *stream);
typedef avs_error_t persist_fn_t(anjay_t *anjay, avs_stream_t *stream);
typedef bool is_modified_fn_t(anjay_t *anjay);
typedef void purge_fn_t(anjay_t *anjay);

struct persistence_target {
    const char *name;
    restore_fn_t *restore;
    persist_fn_t *persist;
    is_modified_fn_t *is_modified;
    purge_fn_t *purge;
};

static bool previous_attempt_failed;

#define DECL_TARGET(Name)                          \
    {                                              \
        .name = AVS_QUOTE(Name),                   \
        .restore = anjay_##Name##_restore,         \
        .persist = anjay_##Name##_persist,         \
        .is_modified = anjay_##Name##_is_modified, \
        .purge = anjay_##Name##_purge              \
    }

static const struct persistence_target targets[] = {
    DECL_TARGET(security_object), DECL_TARGET(server_object),
    DECL_TARGET(attr_storage)
};

#undef DECL_TARGET

void persistence_clear(void) {
    if (nvm_partition_clear(NVM_PARTITION_MODULES)) {
        LOG(ERROR, "Failed to clear modules partition");
    }
}


static int try_restore(anjay_t *anjay) {
    avs_stream_t *stream;
    if (nvm_partition_stream_input_open(NVM_PARTITION_MODULES, &stream)) {
        LOG(ERROR, "Failed to open partition stream");
        return -1;
    }
    if (!stream) {
        LOG(WARNING, "Partition not marked valid");
        return -1;
    }

    bool restore_failed = false;
    for (size_t i = 0; i < AVS_ARRAY_SIZE(targets); i++) {
        if (avs_is_err(targets[i].restore(anjay, stream))) {
            LOG(ERROR, "Failed to restore %s", targets[i].name);
            restore_failed = true;
            break;
        } else {
            LOG(INFO, "Restored %s from persistence", targets[i].name);
        }
    }

    if (avs_is_err(avs_stream_cleanup(&stream))) {
        LOG(ERROR, "Failed to cleanup partition stream");
        return -1;
    }

    return restore_failed ? -1 : 0;
}

int persistence_mod_restore(anjay_t *anjay) {
    if (try_restore(anjay)) {
        for (size_t i = 0; i < AVS_ARRAY_SIZE(targets); i++) {
            LOG(INFO, "Purging %s", targets[i].name);
            targets[i].purge(anjay);
        }
        return -1;
    }
    return 0;
}

static bool any_changed(anjay_t *anjay) {
    for (size_t i = 0; i < AVS_ARRAY_SIZE(targets); i++) {
        if (targets[i].is_modified(anjay)) {
            return true;
        }
    }
    return false;
}

int persistence_mod_persist_if_required(anjay_t *anjay) {
    if (!previous_attempt_failed && !any_changed(anjay)) {
        return 0;
    }

    previous_attempt_failed = true;

    avs_stream_t *stream;
    if (nvm_partition_stream_output_open(NVM_PARTITION_MODULES, &stream)
            || !stream) {
        LOG(ERROR, "Failed to open partition stream");
        return -1;
    }

    bool persist_failed = false;
    for (size_t i = 0; i < AVS_ARRAY_SIZE(targets); i++) {
        if (avs_is_err(targets[i].persist(anjay, stream))) {
            LOG(ERROR, "Failed to persist %s", targets[i].name);
            persist_failed = true;
            break;
        }
    }

    if (avs_is_err(avs_stream_cleanup(&stream))) {
        LOG(ERROR, "Failed to cleanup partition stream");
        return -1;
    }

    if (persist_failed) {
        return -1;
    }

    if (nvm_partition_mark_valid(NVM_PARTITION_MODULES)) {
        LOG(ERROR, "Failed to mark the partition valid");
        return -1;
    }

    LOG(INFO, "Successfully persisted modules");
    previous_attempt_failed = false;
    return 0;
}

