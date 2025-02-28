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

/*
 * This software is based on STM32CubeU5 SBSFU example, check:
 * https://github.com/STMicroelectronics/STM32CubeU5/tree/v1.0.2/Projects/B-U585I-IOT02A/Applications/SBSFU
 */

#ifdef USE_FW_UPDATE

#include "stm32u5xx_hal.h"

#include <anjay/anjay.h>
#include <anjay/fw_update.h>
#include <avsystem/commons/avs_log.h>

#include "Driver_Flash.h"
#include "config_persistence.h"
#include "default_config.h"
#include "firmware_update.h"
#include "region_defs.h"
#include "utils.h"

#define LOG(level, ...) avs_log(firmware_update, level, __VA_ARGS__)

#define REQUIRED_FLASH_ALIGNMENT 16

typedef struct {
    uint32_t MaxSizeInBytes; /*!< The maximum allowed size for the FwImage in
                                User Flash (in Bytes) */
    uint32_t DownloadAddr;   /*!< The download address for the FwImage in
                                UserFlash */
    uint32_t StartAddress;
    size_t DownloadedBytes;
} SFU_FwImageFlashTypeDef;

extern ARM_DRIVER_FLASH LOADER_FLASH_DEV_NAME;

static SFU_FwImageFlashTypeDef fw_image_dwl_area;

static const uint32_t MagicTrailerValue[] = {
    0xf395c277,
    0x7fefd260,
    0x0f505235,
    0x8079b62c,
};

// HACK: see flash_aligned_writer_t's docs on why we're not
// writing the data directly into flash memory
static flash_aligned_writer_t writer;
static uint64_t writer_buf[64];

static int
flash_aligned_writer_cb(uint64_t *src, size_t offset_bytes, size_t len_bytes) {
    fw_image_dwl_area.DownloadAddr += len_bytes;
    return LOADER_FLASH_DEV_NAME.ProgramData(
            (uint32_t) (fw_image_dwl_area.StartAddress + offset_bytes), src,
            len_bytes);
}

static int fw_stream_open(void *user_ptr,
                          const char *package_uri,
                          const struct anjay_etag *package_etag) {
    uint32_t sector_address;
    int32_t ret_arm;
    uint32_t m_uFlashSectorSize;

    ARM_FLASH_INFO *data = LOADER_FLASH_DEV_NAME.GetInfo();
    assert(data->sector_size == REQUIRED_FLASH_ALIGNMENT);

    (void) user_ptr;
    (void) package_uri;
    (void) package_etag;

    fw_image_dwl_area.DownloadAddr = FLASH_AREA_2_OFFSET;
    fw_image_dwl_area.MaxSizeInBytes = FLASH_AREA_2_SIZE;
    fw_image_dwl_area.StartAddress = FLASH_AREA_2_OFFSET;
    fw_image_dwl_area.DownloadedBytes = 0;

    m_uFlashSectorSize = data->sector_size;

    for (sector_address = fw_image_dwl_area.DownloadAddr;
         sector_address
         < fw_image_dwl_area.DownloadAddr + fw_image_dwl_area.MaxSizeInBytes;
         sector_address += m_uFlashSectorSize) {
        ret_arm = LOADER_FLASH_DEV_NAME.EraseSector(sector_address);
        if (ret_arm < 0) {
            LOG(ERROR,
                "Failed to erase sector, ret_val: %d, sector_address: %04x",
                (int) ret_arm, (unsigned int) sector_address);
            return -1;
        }
    }

    if (flash_aligned_writer_new(writer_buf, AVS_ARRAY_SIZE(writer_buf),
                                 flash_aligned_writer_cb, &writer,
                                 REQUIRED_FLASH_ALIGNMENT)) {
        LOG(ERROR, "Buffer has wrong size");
        return -1;
    }

    LOG(INFO, "Successfully initialized");

    return 0;
}

static int fw_stream_write(void *user_ptr, const void *data, size_t length) {
    (void) user_ptr;

    int32_t ret =
            flash_aligned_writer_write(&writer, (const uint8_t *) data, length);
    if (ret) {
        LOG(ERROR, "Failed to flash_aligned_writer_write, ret_val: %d",
            (int) ret);
        return ret;
    }
    fw_image_dwl_area.DownloadedBytes += length;
    LOG(INFO, "Write attempt, packet length: %u, total size: %u ",
        (unsigned int) length,
        (unsigned int) fw_image_dwl_area.DownloadedBytes);

    return 0;
}

static int fw_stream_finish(void *user_ptr) {
    uint32_t magic_address = 0;

    (void) user_ptr;

    int res = flash_aligned_writer_flush(&writer);
    if (res) {
        LOG(ERROR,
            "Failed to finish download: flash aligned writer flush failed, "
            "result: %d",
            res);
        return -1;
    }

    /* write the magic to trigger installation at next reset */
    magic_address =
            fw_image_dwl_area.StartAddress
            + (fw_image_dwl_area.MaxSizeInBytes - sizeof(MagicTrailerValue));

    res = LOADER_FLASH_DEV_NAME.ProgramData(magic_address, MagicTrailerValue,
                                            sizeof(MagicTrailerValue));
    if (res != ARM_DRIVER_OK) {
        LOG(ERROR, "Failed to trigger installation, error code: %d", (int) res);
        return -1;
    }

    LOG(INFO, "Installation trigger success");

    return 0;
}

static void fw_reset(void *user_ptr) {
    (void) user_ptr;
}

static void fw_update_reboot(avs_sched_t *sched, const void *data) {
    LOG(INFO, "Rebooting...");
    HAL_Delay(2000U);
    NVIC_SystemReset();
}

static int fw_perform_upgrade(void *anjay) {
    (void) anjay;

    LOG(INFO, "Firmware will be updated at next device reset");

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

    if (strcmp(FIRMWARE_VERSION, g_config.firmware_version)) {
        state.result = ANJAY_FW_UPDATE_INITIAL_SUCCESS;
        LOG(INFO, "Firmware updated from version '%s' to '%s'",
            g_config.firmware_version, FIRMWARE_VERSION);
        strncpy(g_config.firmware_version, FIRMWARE_VERSION, 32);
        config_save(&g_config);
    }

    return anjay_fw_update_install(anjay, &handlers, anjay, &state);
}
#endif /* USE_FW_UPDATE */
