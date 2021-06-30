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

#include <stdio.h>
#include <string.h>

#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_utils.h>

#include "config_persistence.h"
#include "console.h"
#include "default_config.h"

#define LOG(level, ...) avs_log(menu, level, __VA_ARGS__)

static config_t g_config;

typedef enum {
    OPTION_SERVER_URI,
    OPTION_ENDPOINT_NAME,
    OPTION_PSK,
    OPTION_APN,
    OPTION_APN_USERNAME,
    OPTION_APN_PASSWORWD,
    OPTION_DISCARD_CHANGES,
    OPTION_FACTORY_RESET,
    OPTION_SAVE_EXIT,
    _OPTION_END
} menu_option_id_t;

typedef struct {
    const char *const description;
    char *const value;
    size_t value_capacity;
} menu_option_t;

static menu_option_t OPTIONS[] = {
    [OPTION_SERVER_URI] = { "LwM2M Server URI", g_config.server_uri,
                            sizeof(g_config.server_uri) },
    [OPTION_ENDPOINT_NAME] = { "Endpoint name", g_config.endpoint_name,
                               sizeof(g_config.endpoint_name) },
    [OPTION_PSK] = { "PSK", g_config.psk, sizeof(g_config.psk) },
    [OPTION_APN] = { "APN", g_config.apn, sizeof(g_config.apn) },
    [OPTION_APN_USERNAME] = { "APN username", g_config.apn_username,
                              sizeof(g_config.apn_username) },
    [OPTION_APN_PASSWORWD] = { "APN password", g_config.apn_password,
                               sizeof(g_config.apn_password) },
    [OPTION_DISCARD_CHANGES] = { "Discard changes", NULL, 0 },
    [OPTION_FACTORY_RESET] = { "Factory reset", NULL, 0 },
    [OPTION_SAVE_EXIT] = { "Save & Exit", NULL, 0 }
};

static void print_menu(void) {
    console_write("### Configuration menu ###\r\n");
    for (int i = 0; i < _OPTION_END; i++) {
        console_write("  %d. %-25s", i + 1, OPTIONS[i].description);
        if (OPTIONS[i].value) {
            console_write(" (%s)", OPTIONS[i].value);
        }
        console_write("\r\n");
    }
    console_write("Select option (1 - %d): ", _OPTION_END);
}

static int get_option_id(menu_option_id_t *option_id) {
    char buffer[2];
    int id;
    console_read_line(buffer, sizeof(buffer));
    if (sscanf(buffer, "%d", &id) != 1 || id < 1 || id > _OPTION_END) {
        return -1;
    }
    const int menu_option_id_offset = 1;
    *option_id = id - menu_option_id_offset;
    return 0;
}

static void get_value(menu_option_id_t option_id) {
    console_write("Enter value for `%s`: ", OPTIONS[option_id].description);
    console_read_line(OPTIONS[option_id].value,
                      OPTIONS[option_id].value_capacity);
}

static bool get_confirmation(void) {
    console_write("Are you sure? ('y' to confirm) ");
    char answer[2];
    console_read_line(answer, sizeof(answer));
    return answer[0] == 'y';
}

static void config_restore_defaults(void) {
    g_config = (config_t) {
        .server_uri = DEFAULT_SERVER_URI,
        .psk = DEFAULT_PSK,
        .apn = DEFAULT_APN,
        .apn_username = DEFAULT_APN_USERNAME,
        .apn_password = DEFAULT_APN_PASSWORD
    };
    generate_default_endpoint_name(g_config.endpoint_name,
                                   sizeof(g_config.endpoint_name));
}

static void enter_menu(void) {
    bool changed = false;
    bool menu_running = true;
    while (menu_running) {
        print_menu();
        menu_option_id_t option_id;
        if (get_option_id(&option_id)) {
            console_write("Invalid choice\r\n");
            continue;
        }
        if (OPTIONS[option_id].value) {
            get_value(option_id);
            changed = true;
        } else {
            switch (option_id) {
            case OPTION_DISCARD_CHANGES:
                if (get_confirmation()) {
                    console_write("Discarding changes...\r\n");
                    if (config_load(&g_config)) {
                        LOG(WARNING, "Could not restore current configuration");
                    } else {
                        changed = false;
                    }
                }
                break;
            case OPTION_FACTORY_RESET:
                if (get_confirmation()) {
                    changed = true;
                    config_restore_defaults();
                    console_write("Performing factory reset...\r\n");
                }
                break;
            case OPTION_SAVE_EXIT:
                menu_running = false;
                if (changed) {
                    console_write("Saving config...\r\n");
                    if (config_save(&g_config)) {
                        LOG(WARNING, "Could not save config");
                    }
                }
                console_write("Exiting menu...\r\n");
                break;
            default:
                AVS_UNREACHABLE("invalid option id");
            }
        }
    }
}

void menu_init(void) {
    if (config_load(&g_config)) {
        config_restore_defaults();
    }

    console_write("\r\nPress any key in 3 seconds to enter config menu...\r\n");
    if (console_wait_for_key_press(3000)) {
        enter_menu();
    }
}

const char *config_get_server_uri(void) {
    return g_config.server_uri;
}

const char *config_get_endpoint_name(void) {
    return g_config.endpoint_name;
}

const char *config_get_psk(void) {
    return g_config.psk;
}

const char *config_get_apn(void) {
    return g_config.apn;
}

const char *config_get_apn_username(void) {
    return g_config.apn_username;
}

const char *config_get_apn_password(void) {
    return g_config.apn_password;
}
