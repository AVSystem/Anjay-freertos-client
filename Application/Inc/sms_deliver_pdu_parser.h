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

#pragma once

#ifdef USE_SMS_TRIGGER

#include <stdint.h>

#include <anjay/sms.h>

#define CMGL_RESPONSE_PREFIX "+CMGL: "
#define CMGL_RESPONSE_MAX_INDEX_FIELD "0000"
#define CMGL_RESPONSE_MAX_STAT_FIELD "0"
#define CMGL_RESPONSE_MAX_OPTIONAL_ALPHA_FIELD ""
#define CMGL_RESPONSE_MAX_LENGTH_FIELD "000"

// SCA (10o)
// TP-RP + TP-UDHI + TP-SRI + Padding + TP-LP + TP-MMS + TP-MTI (1o)
// TP-OA (12o)
// TP-PID (1o)
// TP-DCS (1o)
// TP-SCTS (7o)
// TP-UDL (1o)
#define MAX_SMS_SCA_FIELD_LEN_OCT 10
#define MAX_SMS_DELIVER_HEADER_LEN_OCT 23

#define MAX_SMS_DELIVER_MESSAGE_LEN_OCT 140

#define MAX_PDU_LENGTH_OCT                                      \
    (MAX_SMS_SCA_FIELD_LEN_OCT + MAX_SMS_DELIVER_HEADER_LEN_OCT \
     + MAX_SMS_DELIVER_MESSAGE_LEN_OCT)

typedef struct {
    char sender_addr_number[ANJAY_MSISDN_SIZE + 1];
    uint8_t message_length;
    uint8_t message[MAX_SMS_DELIVER_MESSAGE_LEN_OCT];
} sms_deliver_message_t;

/**
 * Parses the PDU.
 *
 * @param pdu_str The PDU in form of HEX string.
 *
 * @param out_sms_pdu The pointer to the users sms_deliver_message_t structure
 * that need to be filled out.
 *
 * @returns 0 on success, negative value in case of error.
 */
int sms_deliver_parse(const char *const pdu_str,
                      sms_deliver_message_t *out_sms_pdu);

#endif // USE_SMS_TRIGGER
