/*
 * Copyright 2020-2024 AVSystem <avsystem@avsystem.com>
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

#ifdef USE_SMS_TRIGGER

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <anjay/sms.h>

#include <avsystem/commons/avs_errno.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_time.h>

#include <cellular_service_control.h>
#include <cellular_service_os.h>

#include "circ_buffer_ipc.h"
#include "lwm2m.h"
#include "sms_deliver_pdu_parser.h"
#include "sms_driver.h"

#ifdef DEBUG
#undef DEBUG
#endif

#define SMS_LOCAL_INBOX_SIZE 10
#define MAX_VALIDITY_MAP_VALUE ((1 << (SMS_LOCAL_INBOX_SIZE)) - 1)

#define CMGL_RESPONSE_PREFIX_MAX_LENGTH                 \
    (CMGL_RESPONSE_PREFIX CMGL_RESPONSE_MAX_INDEX_FIELD \
     "," CMGL_RESPONSE_MAX_STAT_FIELD                   \
     "," CMGL_RESPONSE_MAX_OPTIONAL_ALPHA_FIELD         \
     "," CMGL_RESPONSE_MAX_LENGTH_FIELD "\r\n")

#define CMGF_COMMAND_PDU_MODE "AT+CMGF=0"
#define CMGL_COMMAND_REC_UNREAD "AT+CMGL=0"
#define CMGD_COMMAND_DELETE_ALL_MESSAGES "AT+CMGD=0,4"

typedef char cmgl_response_t[AVS_MAX(sizeof(CMGL_RESPONSE_PREFIX_MAX_LENGTH),
                                     MAX_PDU_LENGTH_OCT)];

static IPC_RxCallbackTypeDef g_orig_rx_callback;

static struct {
    uint32_t validity_map;
    sms_deliver_message_t messages[SMS_LOCAL_INBOX_SIZE];
} sms_inbox;

AVS_STATIC_ASSERT(sizeof(sms_inbox.validity_map) * 8 >= SMS_LOCAL_INBOX_SIZE,
                  validity_map_too_small);

static const char *
iterate_through_response(const char *current_response_pointer,
                         const char *const last_valid_response_pointer,
                         size_t max_field_len,
                         char separator) {
    size_t i = 0;
    while (*current_response_pointer++ != separator && i++ < max_field_len)
        ;

    if (*(current_response_pointer - 1) != separator
            || (current_response_pointer > last_valid_response_pointer
                && separator != '\n')) {
        return NULL;
    }

    return current_response_pointer;
}

static int cmgl_sms_parse_prefix(const char *cmgl_response,
                                 sms_deliver_message_t *out_sms_response) {
    assert(cmgl_response && out_sms_response);

    const char *const cmgl_response_last_valid_byte =
            cmgl_response + strlen(cmgl_response) - 1;

    static const size_t len = sizeof(CMGL_RESPONSE_PREFIX) - 1;
    if (cmgl_response + len - 1 > cmgl_response_last_valid_byte) {
        return -1;
    }
    if (strncmp(cmgl_response, CMGL_RESPONSE_PREFIX, len) != 0) {
        return -1;
    }
    cmgl_response += len;
    // Skipping command name field without handling it

    const char *aux = cmgl_response;
    if (!(cmgl_response = iterate_through_response(
                  cmgl_response, cmgl_response_last_valid_byte,
                  sizeof(CMGL_RESPONSE_MAX_INDEX_FIELD) - 1, ','))) {
        return -1;
    }
    if (cmgl_response - aux <= 1) {
        return -1;
    }
    // Skipping index field without handling it

    aux = cmgl_response;
    if (!(cmgl_response = iterate_through_response(
                  cmgl_response, cmgl_response_last_valid_byte,
                  sizeof(CMGL_RESPONSE_MAX_STAT_FIELD) - 1, ','))) {
        return -1;
    }
    if (cmgl_response - aux <= 1) {
        return -1;
    }
    // Skipping stat field without handling it

    aux = cmgl_response;
    if (!(cmgl_response = iterate_through_response(
                  cmgl_response, cmgl_response_last_valid_byte,
                  sizeof(CMGL_RESPONSE_MAX_OPTIONAL_ALPHA_FIELD) - 1, ','))) {
        return -1;
    }
    // Skipping alpha field without handling it

    aux = cmgl_response;
    if (!(cmgl_response = iterate_through_response(
                  cmgl_response, cmgl_response_last_valid_byte,
                  sizeof(CMGL_RESPONSE_MAX_LENGTH_FIELD) - 1, '\r'))) {
        return -1;
    }

    uint16_t tpdu_len = 0;
    while (cmgl_response - aux > 1) {
        if (isdigit((unsigned char) *aux)) {
            tpdu_len *= 10;
            tpdu_len += *aux++ - '0';
        } else {
            return -1;
        }
    }

    if (tpdu_len > MAX_SMS_DELIVER_HEADER_LEN_OCT
                               + MAX_SMS_DELIVER_MESSAGE_LEN_OCT) {
        return -1;
    }

    if (!(cmgl_response = iterate_through_response(
                  cmgl_response, cmgl_response_last_valid_byte, 0, '\n'))) {
        return -1;
    }

    return 0;
}

static int find_free_inbox_slot(void) {
    int ret = -1;
    for (size_t i = 0; i < SMS_LOCAL_INBOX_SIZE; i++) {
        if (!(sms_inbox.validity_map & (1 << i))) {
            ret = i;
            break;
        }
    }

    return ret;
}

static int cmgl_check_and_parse_msg_from_buffer(char *msg_buffer) {
    static bool prefix_parsed = false;
    static int free_slot_index = -1;

    if (strncmp(msg_buffer, CMGL_RESPONSE_PREFIX,
                sizeof(CMGL_RESPONSE_PREFIX) - 1)
            == 0) {
        if (!prefix_parsed) {
            free_slot_index = find_free_inbox_slot();
            if (free_slot_index < 0) {
                avs_log(sms_trigger, ERROR, "The local inbox is full");
                return -1;
            }

            memset(&sms_inbox.messages[free_slot_index], 0,
                   sizeof(sms_inbox.messages[free_slot_index]));
            if (cmgl_sms_parse_prefix(msg_buffer,
                                      &sms_inbox.messages[free_slot_index])) {
                avs_log(sms_trigger, DEBUG,
                        "Damaged prefix or not a CMGL response");
                return -1;
            }

            prefix_parsed = true;
        }
    } else {
        if (prefix_parsed) {
            prefix_parsed = false;
            if (sms_deliver_parse(msg_buffer,
                                  &sms_inbox.messages[free_slot_index])) {
                avs_log(sms_trigger, DEBUG,
                        "Damaged pdu or not a CMGL response");
                return -1;
            }

            sms_inbox.validity_map |= (1 << free_slot_index);
            free_slot_index = -1;
        }
    }

    return 0;
}

static void read_modem_cmgl_rx_callback(IPC_Handle_t *channel) {
    // CAUTION: We are in an ISR context here
    cmgl_response_t cmgl_response_candidate = { 0 };
    read_ipc_circ_buffer_and_handle_msg(channel, cmgl_response_candidate,
                                        sizeof(cmgl_response_candidate),
                                        cmgl_check_and_parse_msg_from_buffer);

    if (g_orig_rx_callback) {
        g_orig_rx_callback(channel);
    }
}

static int pull_sms_from_modem(void) {
    IPC_Handle_t *channel =
            IPC_DevicesList[USER_DEFINED_IPC_DEVICE_MODEM].h_current_channel;

    __disable_irq();
    // HACK: ST APIs don't allow reading the response to the command issued with
    // osCDS_direct_cmd(). So we temporarily change the callback that is called
    // by the IPC subsystem when new message is received.
    g_orig_rx_callback = channel->RxClientCallback;
    channel->RxClientCallback = read_modem_cmgl_rx_callback;
    __enable_irq();

    {
        // Reading unread messages from the modem
        CS_direct_cmd_tx_t direct_cmd_tx = {
            .cmd_str = CMGL_COMMAND_REC_UNREAD,
            .cmd_size = sizeof(CMGL_COMMAND_REC_UNREAD) - 1
        };

        if (osCDS_direct_cmd(&direct_cmd_tx, NULL)) {
            avs_log(sms_trigger, ERROR, "Error while sending CMGL command");
            return -1;
        }
        // TODO: Consider handling OK response
    }

    __disable_irq();
    channel->RxClientCallback = g_orig_rx_callback;
    __enable_irq();

    return 0;
}

static int sms_send_dummy(anjay_smsdrv_t *driver,
                          const char *destination,
                          const void *data,
                          size_t data_size,
                          const anjay_smsdrv_multipart_info_t *multipart_info,
                          avs_time_duration_t timeout) {
    // This might be actually treated as critical error, but this driver is
    // intended to support SMS Trigger mechanism that is used only for downlink
    // operation and allows the Client not to respond to the received message
    // (not even an ACK)
    avs_log(sms_trigger, DEBUG, "SMS sending is not supported!");
    return 0;
}

static int clear_modem_inbox(void) {
    CS_direct_cmd_tx_t direct_cmd_tx = {
        .cmd_str = CMGD_COMMAND_DELETE_ALL_MESSAGES,
        .cmd_size = sizeof(CMGD_COMMAND_DELETE_ALL_MESSAGES) - 1
    };

    if (osCDS_direct_cmd(&direct_cmd_tx, NULL)) {
        avs_log(sms_trigger, ERROR, "Error while sending CMGD command");
        return -1;
    }
    // TODO: Consider handling OK response

    return 0;
}

static int sms_should_try_recv(anjay_smsdrv_t *driver,
                               avs_time_duration_t timeout) {
    (void) driver;

    avs_time_monotonic_t deadline =
            avs_time_monotonic_add(avs_time_monotonic_now(), timeout);
    uint32_t init_validity_map = sms_inbox.validity_map;
    do {
        if (sms_inbox.validity_map == MAX_VALIDITY_MAP_VALUE
                || sms_inbox.validity_map != init_validity_map) {
            break;
        }

        if (pull_sms_from_modem()) {
            return -1;
        }
    } while (avs_time_monotonic_before(avs_time_monotonic_now(), deadline));

    if (clear_modem_inbox()) {
        return -1;
    }

    return sms_inbox.validity_map > 0;
}

static int sms_recv_all(anjay_smsdrv_t *driver,
                        anjay_smsdrv_recv_all_cb_t *cb,
                        void *cb_arg) {
    (void) driver;
    assert(cb);

    if (!g_anjay) {
        return -1;
    }

    anjay_ssid_t ssid;
    bool should_remove;
    bool trigger_enabled;
    for (size_t current = 0; current < SMS_LOCAL_INBOX_SIZE; current++) {
        if (sms_inbox.validity_map & (1 << current)) {
            bool server_found = false;

            // numbers in inbox have '+' prefix added for socket URI matching
            // and security configs don't, so there is [1] offset
            if (!anjay_get_server_ssid_by_msisdn(
                        g_anjay,
                        &sms_inbox.messages[current].sender_addr_number[1],
                        &ssid)
                    && !anjay_get_sms_trigger(g_anjay, ssid,
                                              &trigger_enabled)) {
                server_found = true;
            }

            if (!server_found || !trigger_enabled) {
                avs_log(sms_trigger, WARNING,
                        "Unrecognized number or SMS Trigger is disabled");
                sms_inbox.validity_map &= ~(1 << current);
                continue;
            }

            cb(cb_arg,
               sms_inbox.messages[current].sender_addr_number,
               sms_inbox.messages[current].message,
               sms_inbox.messages[current].message_length,
               NULL,
               &should_remove);

            if (should_remove) {
                sms_inbox.validity_map &= ~(1 << current);
            }
        }
    }

    return 0;
}

static int sms_system_socket(anjay_smsdrv_t *driver, const void **out) {
    // SMS driver does not operate on an object representable with a file
    // descriptor, therefore we want to avoid it being handled by Anjay event
    // loop. Custom scheduler job handles the socket entry associated with
    // Trigger Connection instead.
    return -1;
}

static avs_errno_t sms_error(anjay_smsdrv_t *driver) {
    return AVS_NO_ERROR;
}

static void sms_free(anjay_smsdrv_t *driver) {
    memset(&sms_inbox, 0, sizeof(sms_inbox));
}

static anjay_smsdrv_t sms_driver = {
    .send = sms_send_dummy,
    .should_try_recv = sms_should_try_recv,
    .recv_all = sms_recv_all,
    .system_socket = sms_system_socket,
    .get_error = sms_error,
    .free = sms_free
};

static int set_pdu_mode(void) {
    CS_direct_cmd_tx_t direct_cmd_tx = {
        .cmd_str = CMGF_COMMAND_PDU_MODE,
        .cmd_size = sizeof(CMGF_COMMAND_PDU_MODE) - 1
    };

    if (osCDS_direct_cmd(&direct_cmd_tx, NULL)) {
        avs_log(sms_trigger, ERROR, "Error while sending CMGF command");
        return -1;
    }
    // TODO: Consider handling OK response

    return 0;
}

anjay_smsdrv_t *_anjay_freertos_sms_driver_create(void) {
    // TODO: if phone number is not set, execute AT+CNUM
    if (clear_modem_inbox() || set_pdu_mode()) {
        return NULL;
    }
    // TODO: Consider setupping of additional parameters, such a storage choice

    return &sms_driver;
}

#endif // USE_SMS_TRIGGER
