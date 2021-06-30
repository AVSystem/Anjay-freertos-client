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

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"

#include "application.h"
#include "cellular_control_api.h"
#include "cellular_service_datacache.h"
#include "plf_config.h"

#if (!USE_DEFAULT_SETUP == 1)
#    include "app_select.h"
#    include "setup.h"
#    include "time_date.h"
#    if (USE_MODEM_VOUCHER == 1)
#        include "voucher.h"
#    endif /* (USE_MODEM_VOUCHER == 1) */
#endif     /* (!USE_DEFAULT_SETUP == 1) */

#include "lwm2m.h"

#include "board_buttons.h"

#include "menu.h"

#include <string.h>

#include <avsystem/commons/avs_log.h>

#include "trace_interface.h"

static void configure_modem(void) {
    dc_cellular_params_t cellular_params;
    memset(&cellular_params, 0, sizeof(cellular_params));
    dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, &cellular_params,
                sizeof(cellular_params));

    const uint8_t sim_slot_index = 0;
    const dc_sim_slot_t *sim_slot = &cellular_params.sim_slot[sim_slot_index];

    strcpy((char *) sim_slot->apn, config_get_apn());
    strcpy((char *) sim_slot->username, config_get_apn_username());
    strcpy((char *) sim_slot->password, config_get_apn_password());

    dc_com_write(&dc_com_db, DC_CELLULAR_CONFIG, &cellular_params,
                 sizeof(cellular_params));
}

static void utilities_init(void) {
    (void) board_buttons_init();
}

static void utilities_start(void) {
    (void) board_buttons_start();
}

static void
log_handler(avs_log_level_t level, const char *module, const char *message) {
    traceIF_uartPrintForce(DBG_CHAN_APPLICATION, (uint8_t *) message,
                           strlen(message));
    traceIF_uartPrintForce(DBG_CHAN_APPLICATION, (uint8_t *) "\r\n", 2);
}

void application_init() {

    avs_log_set_handler(log_handler);
    /* RandomNumberGenerator );*/
    srand(osKernelSysTick());

    cellular_init();
    lwm2m_init();
    utilities_init();

    configure_modem();

    cellular_start();
    lwm2m_start();
    lwm2m_notify_start();
    utilities_start();
}
