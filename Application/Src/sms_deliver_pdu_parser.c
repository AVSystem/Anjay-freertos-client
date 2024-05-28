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

#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_utils.h>

#include "sms_deliver_pdu_parser.h"

#define SMSC_MAX_ADDRESS_OCTETS 10

#define SMS_TP_MTI_FIELD 0x03
#define SMS_DELIVER 0b00
#define SMS_VALID_ADDRESS_TYPE_MASK 0x80
#define SMS_8_BIT_CODING 0x04
#define SMS_TIME_STAMP_NIB 14

static int to_upper_even(uint8_t number, uint8_t *out_number) {
    assert(number != UINT8_MAX);
    *out_number = (number % 2) ? (number + 1) : number;
    return 0;
}

static void swap_half_bytes(uint8_t *tab, size_t tab_size) {
    for (size_t i = 0; i < tab_size; i++) {
        tab[i] = (tab[i] << 4) | (tab[i] >> 4);
    }
}

static int copy_swap_hexlify(const uint8_t *const in_ptr,
                             char *const out_ptr,
                             uint8_t count) {
    assert(in_ptr && out_ptr);

    uint8_t aux[ANJAY_MSISDN_SIZE / 2];
    (void) memcpy(aux, in_ptr, count);

    swap_half_bytes(aux, count);

    size_t hexlified_bytes;
    if (avs_hexlify(out_ptr, count * 2 + 1, &hexlified_bytes, aux, count)
            || hexlified_bytes < count) {
        return -1;
    }

    size_t i = 0;
    while (i < 2 * count - 1) {
        if (!isdigit((unsigned char) out_ptr[i++])) {
            return -1;
        }
    }

    if (toupper(out_ptr[i]) == 'F') {
        out_ptr[i] = '\0';
    } else if (!isdigit((unsigned char) out_ptr[i])) {
        return -1;
    }

    return 0;
}

static int sms_deliver_pdu_parse(const uint8_t *cont_pdu_ptr,
                                 const uint8_t *const pdu_last_valid_address,
                                 sms_deliver_message_t *out_sms_deliver_pdu) {
    assert(cont_pdu_ptr && pdu_last_valid_address && out_sms_deliver_pdu);

    if (cont_pdu_ptr > pdu_last_valid_address) {
        return -1;
    }
    uint8_t even_sender_addr_digit_count;
    if (*cont_pdu_ptr == 0
            || to_upper_even(*cont_pdu_ptr++, &even_sender_addr_digit_count)) {
        return -1;
    }

    if (cont_pdu_ptr > pdu_last_valid_address
            || even_sender_addr_digit_count > ANJAY_MSISDN_SIZE
            || !(*cont_pdu_ptr++ & SMS_VALID_ADDRESS_TYPE_MASK)) {
        return -1;
    }

    if (cont_pdu_ptr + even_sender_addr_digit_count / 2 - 1
            > pdu_last_valid_address) {
        return -1;
    }
    out_sms_deliver_pdu->sender_addr_number[0] = '+';
    if (copy_swap_hexlify(cont_pdu_ptr,
                          &out_sms_deliver_pdu->sender_addr_number[1],
                          even_sender_addr_digit_count / 2)) {
        return -1;
    }
    cont_pdu_ptr += even_sender_addr_digit_count / 2;
    avs_log(sms_trigger, INFO, "SMS received from number %s",
            out_sms_deliver_pdu->sender_addr_number);

    if (cont_pdu_ptr++ > pdu_last_valid_address) {
        return -1;
    }
    // Skipping PID field without handling it

    if (*cont_pdu_ptr++ != SMS_8_BIT_CODING) {
        return -1;
    }

    if (cont_pdu_ptr + SMS_TIME_STAMP_NIB / 2 - 1 > pdu_last_valid_address) {
        return -1;
    }

    cont_pdu_ptr += SMS_TIME_STAMP_NIB / 2;
    // Skipping SCTS field without handling it

    if (cont_pdu_ptr > pdu_last_valid_address) {
        return -1;
    }

    out_sms_deliver_pdu->message_length = *cont_pdu_ptr++;
    if (out_sms_deliver_pdu->message_length > MAX_SMS_DELIVER_MESSAGE_LEN_OCT) {
        return -1;
    }

    if (cont_pdu_ptr + out_sms_deliver_pdu->message_length - 1
            > pdu_last_valid_address) {
        return -1;
    }
    (void) memcpy(out_sms_deliver_pdu->message, cont_pdu_ptr,
                  out_sms_deliver_pdu->message_length);

    return 0;
}

int sms_deliver_parse(const char *const pdu_str,
                      sms_deliver_message_t *out_sms_pdu) {
    assert(pdu_str && out_sms_pdu);

    size_t pdu_str_size = strlen(pdu_str);
    if (pdu_str_size % 2 != 0 || pdu_str_size <= 2) {
        return -1;
    }
    pdu_str_size -= 2; // '\r' and '\n' at the end

    uint8_t pdu_bin[MAX_PDU_LENGTH_OCT];
    size_t unhexlified_bytes;
    if (avs_unhexlify(&unhexlified_bytes, pdu_bin, MAX_PDU_LENGTH_OCT, pdu_str,
                      pdu_str_size)
            || unhexlified_bytes < pdu_str_size / 2) {
        return -1;
    }

    const uint8_t *pdu_ptr = pdu_bin;
    const uint8_t *const pdu_last_valid_address =
            pdu_ptr + unhexlified_bytes - 1;

    if (*pdu_ptr > SMSC_MAX_ADDRESS_OCTETS
            || (pdu_ptr += *pdu_ptr) > pdu_last_valid_address) {
        return -1;
    }
    pdu_ptr++;
    // Skipping whole SCA field without handling it

    if (pdu_ptr > pdu_last_valid_address) {
        return -1;
    }

    if ((*pdu_ptr++ & SMS_TP_MTI_FIELD) != SMS_DELIVER) {
        return -1;
    }

    if (sms_deliver_pdu_parse(pdu_ptr, pdu_last_valid_address, out_sms_pdu)) {
        return -1;
    }

    return 0;
}

#endif // USE_SMS_TRIGGER
