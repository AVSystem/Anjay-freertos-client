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
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "cellular_control_api.h"
#include "cellular_service_datacache.h"
#include "cmsis_os.h"
#include "main.h"
#include "plf_config.h"
#include "task.h"
#include "trace_interface.h"

#include <avsystem/commons/avs_log.h>

#ifdef USE_AIBP
#include "ai_bridge.h"
#include "console.h"
#endif

#if (!USE_DEFAULT_SETUP == 1)
#include "app_select.h"
#include "setup.h"
#include "time_date.h"
#if (USE_MODEM_VOUCHER == 1)
#include "voucher.h"
#endif /* (USE_MODEM_VOUCHER == 1) */
#endif /* (!USE_DEFAULT_SETUP == 1) */

#include "application.h"
#include "board_buttons.h"
#include "config_persistence.h"
#include "lwm2m.h"
#include "menu.h"

static void configure_modem(void) {
    dc_cellular_params_t cellular_params;
    memset(&cellular_params, 0, sizeof(cellular_params));
    dc_com_read(&dc_com_db, DC_CELLULAR_CONFIG, &cellular_params,
                sizeof(cellular_params));

    const uint8_t sim_slot_index = 0;
    const dc_sim_slot_t *sim_slot = &cellular_params.sim_slot[sim_slot_index];

    strcpy((char *) sim_slot->apn, g_config.apn);
    strcpy((char *) sim_slot->username, g_config.apn_username);
    strcpy((char *) sim_slot->password, g_config.apn_password);

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

void application_init(void) {
    menu_init();

    avs_log_set_handler(log_handler);
    /* RandomNumberGenerator );*/
    srand(osKernelSysTick());

#ifdef USE_AIBP
    console_printf("### Detect AI BRIDGE type. ###\r\n");
    if (ai_bridge_get_type() == AI_BRIDGE_CLASSIFIER_TYPE) {
        ai_bridge_get_classes();
    }
    console_printf("### End of detection. ###\r\n");
#endif

    cellular_init();
    lwm2m_init();
    utilities_init();

    configure_modem();

    cellular_start();
    lwm2m_start();
    utilities_start();

#ifdef USE_AIBP
    ai_bridge_start_it();
#endif
}
