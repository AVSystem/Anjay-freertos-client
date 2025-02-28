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
#ifdef USE_FW_UPDATE

#ifndef HAL_IWDG_MODULE_ENABLED
#error "Firmware Update works only with Internal Watchdog enabled."
#endif /* HAL_IWDG_MODULE_ENABLED */

#include <anjay/anjay.h>
#include <anjay/fw_update.h>
#include <avsystem/commons/avs_log.h>

#include "config_persistence.h"
#include "default_config.h"
#include "firmware_update.h"
#include "lwm2m.h"
#include "utils.h"

#define SFU_APP_NEW_IMAGE_C
#define SFU_FWIMG_COMMON_C

#include "flash_if.h"
#include "se_def.h"
#include "se_interface_application.h"
#include "sfu_fwimg_regions.h"
#include "sfu_new_image.h"
#include "stm32l496g_discovery.h"
#include "stm32l4xx_hal.h"

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#include "mapping_fwimg.h"
#include "mapping_sbsfu.h"
#elif defined(__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#endif /* __CC_ARM || __ARMCC_VERSION */

#define REQUIRED_FLASH_ALIGNMENT 8

static bool update_initialized;
static size_t downloaded_bytes;

static uint8_t fw_header_dwl_slot[SE_FW_HEADER_TOT_LEN];
static uint64_t writer_buf[64];
// HACK: see flash_aligned_writer_t's docs on why we're not
// writing the data directly into flash memory
static flash_aligned_writer_t writer;

static int
flash_aligned_writer_cb(uint64_t *src, size_t offset_bytes, size_t len_bytes) {
    return FLASH_If_Write((void *) (SlotStartAdd[SLOT_DWL_1] + offset_bytes),
                          src, len_bytes);
}

static int fw_stream_open(void *user_ptr,
                          const char *package_uri,
                          const struct anjay_etag *package_etag) {
    (void) user_ptr;
    (void) package_uri;
    (void) package_etag;

    /* Cleanup the memory for the firmware download */
    if (FLASH_If_Erase_Size((void *) (SlotStartAdd[SLOT_DWL_1]),
                            SLOT_SIZE(SLOT_DWL_1))
            != HAL_OK) {
        avs_log(fw_update, ERROR, "Init not successful");

        return -1;
    }

    if (flash_aligned_writer_new(writer_buf, AVS_ARRAY_SIZE(writer_buf),
                                 flash_aligned_writer_cb, &writer,
                                 REQUIRED_FLASH_ALIGNMENT)) {
        avs_log(fw_update, ERROR, "Buffer has wrong size");
        return -1;
    }

    downloaded_bytes = 0;
    update_initialized = true;
    avs_log(fw_update, INFO, "Init successful");

    return 0;
}

static int fw_stream_write(void *user_ptr, const void *data, size_t length) {
    (void) user_ptr;

    assert(update_initialized);

    int res = flash_aligned_writer_write(&writer, data, length);
    if (res) {
        return res;
    }

    downloaded_bytes += length;

    avs_log(fw_update, INFO, "Max size %lu bytes, downloaded %lu bytes.",
            SLOT_SIZE(SLOT_DWL_1), (unsigned long) downloaded_bytes);

    return 0;
}

static int fw_stream_finish(void *user_ptr) {
    (void) user_ptr;

    assert(update_initialized);
    update_initialized = false;

    int res = flash_aligned_writer_flush(&writer);
    if (res) {
        avs_log(fw_update, ERROR,
                "Failed to finish download: flash aligned writer flush failed, "
                "result: %d",
                res);
        return -1;
    }

    /* Read header in download slot */
    (void) FLASH_If_Read(fw_header_dwl_slot,
                         (void *) SlotStartAdd[SLOT_DWL_1],
                         SE_FW_HEADER_TOT_LEN);

    return 0;
}

static void fw_reset(void *user_ptr) {
    (void) user_ptr;

    update_initialized = false;
}

static void fw_update_reboot(avs_sched_t *sched, const void *data) {
    avs_log(fw_update, INFO, "Rebooting...");
    HAL_Delay(1000U);
    NVIC_SystemReset();
}

static int fw_perform_upgrade(void *anjay) {
    (void) anjay;

    /* Ask for installation at next reset */
    (void) SFU_APP_InstallAtNextReset((uint8_t *) fw_header_dwl_slot);
    avs_log(fw_update, INFO, "Firmware will be updated at next device reset");

    return AVS_SCHED_DELAYED(anjay_get_scheduler(anjay), NULL,
                             avs_time_duration_from_scalar(1, AVS_TIME_S),
                             fw_update_reboot, NULL, 0);
}

static const anjay_fw_update_handlers_t handlers = {
    .stream_open = fw_stream_open,
    .stream_write = fw_stream_write,
    .stream_finish = fw_stream_finish,
    .reset = fw_reset,
    .perform_upgrade = fw_perform_upgrade
};

int fw_update_install(anjay_t *anjay) {
    anjay_fw_update_initial_state_t state = { 0 };

    if (strcmp(FIRMWARE_VERSION, g_config.firmware_version) != 0) {
        state.result = ANJAY_FW_UPDATE_INITIAL_SUCCESS;
        avs_log(fw_update, INFO, "Firmware updated from version '%s' to '%s'",
                g_config.firmware_version, FIRMWARE_VERSION);
        strncpy(g_config.firmware_version, FIRMWARE_VERSION, 32);
        config_save(&g_config);
    }

    return anjay_fw_update_install(anjay, &handlers, anjay, &state);
}
#endif /* USE_FW_UPDATE */
