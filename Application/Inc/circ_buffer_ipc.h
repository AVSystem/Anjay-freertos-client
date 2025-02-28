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

#include <stddef.h>

#include <ipc_common.h>

typedef int ipc_circ_buffer_msg_handler(char *tab);

/**
 * Reads message form IPC buffer and handles it.
 *
 * @param channel Pointer to the IPC channel.
 * @param resp_buffer Pointer to the buffer, which will be filled and handled
 * internally.
 * @param resp_buffer_size Size of the resp_buffer.
 * @param handler Handler function that will be called for every received
 * message.
 *
 * @returns 0 on success, negative value in case of error
 */
int read_ipc_circ_buffer_and_handle_msg(IPC_Handle_t *channel,
                                        char *resp_buffer,
                                        size_t resp_buffer_size,
                                        ipc_circ_buffer_msg_handler *handler);
