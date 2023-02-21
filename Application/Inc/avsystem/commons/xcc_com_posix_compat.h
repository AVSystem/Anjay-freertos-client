/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
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

#ifndef XCC_COM_POSIX_COMPAT_H
#define XCC_COM_POSIX_COMPAT_H

#include <stddef.h>
#include <stdint.h>

#include <com_sockets_net_compat.h>

#define XCC_NET_SOCKET_POLLIN (1 << 0)
#define XCC_NET_SOCKET_POLLERR (1 << 3)

#define INVALID_SOCKET NULL

typedef struct xcc_net_socket_impl xcc_net_socket_impl_t;

typedef xcc_net_socket_impl_t *sockfd_t;
typedef size_t nfds_t;

struct xcc_net_socket_pollfd {
    sockfd_t fd;
    short events;
    short revents;
};

int xcc_net_socket_poll(struct xcc_net_socket_pollfd *fds,
                        nfds_t nfds,
                        int timeout);

// HACK: use following #defines instead of directly declaring and implementing
// these methods to not pollute global namespace if anyone decides to use LwIP
// in the same app, too.
#ifdef pollfd
#undef pollfd
#endif // pollfd
#define pollfd xcc_net_socket_pollfd

#ifdef POLLIN
#undef POLLIN
#endif // POLLIN
#define POLLIN XCC_NET_SOCKET_POLLIN

#ifdef POLLERR
#undef POLLERR
#endif // POLLERR
#define POLLERR XCC_NET_SOCKET_POLLERR

#ifdef poll
#undef poll
#endif // poll
#define poll xcc_net_socket_poll
#endif /* XCC_COM_POSIX_COMPAT_H */
