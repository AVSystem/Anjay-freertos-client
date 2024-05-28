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

#include <stddef.h>
#include <stdint.h>

#include <avsystem/commons/avs_log.h>

#include <ipc_common.h>
#include <ipc_rxfifo.h>

#include "circ_buffer_ipc.h"

#ifdef DEBUG
#undef DEBUG
#endif

int read_ipc_circ_buffer_and_handle_msg(IPC_Handle_t *channel,
                                        char *out_buffer,
                                        size_t out_buffer_size,
                                        ipc_circ_buffer_msg_handler *handler) {
    uint16_t bytes_available =
            IPC_RXBUF_MAXSIZE - IPC_RXFIFO_getFreeBytes(channel);
    uint16_t bytes_read = 0;
    while (bytes_read + 2 < bytes_available) {
        uint8_t header1 =
                channel->RxQueue
                        .data[(channel->RxQueue.index_read + bytes_read++)
                              % IPC_RXBUF_MAXSIZE];
        uint8_t header2 =
                channel->RxQueue
                        .data[(channel->RxQueue.index_read + bytes_read++)
                              % IPC_RXBUF_MAXSIZE];
        uint16_t msg_size =
                ((header1 & IPC_RXMSG_HEADER_SIZE_MASK) << 8) + header2;
        if (bytes_read + msg_size > bytes_available) {
            return -1;
        }
        uint16_t truncated_msg_size = AVS_MIN(msg_size, out_buffer_size - 1);
        uint16_t start_idx =
                (channel->RxQueue.index_read + bytes_read) % IPC_RXBUF_MAXSIZE;
        uint16_t end_idx =
                (channel->RxQueue.index_read + bytes_read + truncated_msg_size)
                % IPC_RXBUF_MAXSIZE;
        if (start_idx <= end_idx) {
            memcpy(out_buffer,
                   &channel->RxQueue.data[start_idx],
                   end_idx - start_idx);
        } else {
            memcpy(out_buffer,
                   &channel->RxQueue.data[start_idx],
                   IPC_RXBUF_MAXSIZE - start_idx);
            memcpy(&out_buffer[IPC_RXBUF_MAXSIZE - start_idx],
                   channel->RxQueue.data, end_idx);
        }
        out_buffer[truncated_msg_size] = '\0';

        if (handler(out_buffer)) {
            avs_log(ipc_circ_buffer, DEBUG,
                    "Problem occured while handling circ buffer msg");
        }

        bytes_read += msg_size;
    }

    return 0;
}
