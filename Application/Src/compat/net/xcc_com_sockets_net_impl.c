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

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_memory.h>
#include <avsystem/commons/avs_socket.h>
#include <avsystem/commons/avs_socket_v_table.h>
#include <avsystem/commons/avs_time.h>
#include <avsystem/commons/avs_utils.h>
#include <avsystem/commons/xcc_com_posix_compat.h>

#define LOG(level, ...) avs_log(netimpl, level, __VA_ARGS__)
#include <com_err.h>
#include <com_sockets.h>
#include <plf_modem_config.h>
#include <rtosal.h>

#include "xcc_com_sockets.h"

#define POLL_BUF_SIZE(socktype) \
    ((socktype) == COM_SOCK_STREAM ? 1 : CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE)

#define REMOTE_HOST_BUF_SIZE sizeof("255.255.255.255")
#define REMOTE_PORT_BUF_SIZE AVS_UINT_STR_BUF_SIZE(uint16_t)

struct xcc_net_socket_impl {
    const avs_net_socket_v_table_t *operations;
    xcc_net_socket_impl_t *self;
    int32_t socktype;
    int32_t fd;
    char remote_host[REMOTE_HOST_BUF_SIZE];
    char remote_port[REMOTE_PORT_BUF_SIZE];
    avs_time_duration_t recv_timeout;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    com_sockets_err_t poll_captured_recv_error;
    size_t buffered_by_poll_len;
    uint8_t buffered_by_poll[];
};

avs_error_t _avs_net_initialize_global_compat_state(void) {
    return AVS_OK;
}

void _avs_net_cleanup_global_compat_state(void) {}

// mapping similar to one provided by com_sockets_err_to_errno()
// if COM_SOCKETS_ERRNO_COMPAT is enabled (which requires LwIP)
static avs_errno_t com_sockets_err_to_avs_errno(com_sockets_err_t err) {
    switch (err) {
    case COM_SOCKETS_ERR_OK:
        return AVS_NO_ERROR;
    case COM_SOCKETS_ERR_DESCRIPTOR:
        return AVS_EIO;
    case COM_SOCKETS_ERR_PARAMETER:
        return AVS_EINVAL;
    case COM_SOCKETS_ERR_WOULDBLOCK:
        return AVS_EAGAIN;
    case COM_SOCKETS_ERR_NOMEMORY:
        return AVS_ENOMEM;
    case COM_SOCKETS_ERR_CLOSING:
        return AVS_ENOTCONN;
    case COM_SOCKETS_ERR_LOCKED:
        return AVS_EADDRINUSE;
    case COM_SOCKETS_ERR_TIMEOUT:
        return AVS_ETIMEDOUT;
    case COM_SOCKETS_ERR_INPROGRESS:
        return AVS_EINPROGRESS;
    case COM_SOCKETS_ERR_NONAME:
        return AVS_EHOSTUNREACH;
    case COM_SOCKETS_ERR_NONETWORK:
        return AVS_EHOSTUNREACH;
    case COM_SOCKETS_ERR_UNSUPPORTED:
        return AVS_ENOTSUP;
    case COM_SOCKETS_ERR_STATE:
        return AVS_EINVAL;
    default:
        return AVS_UNKNOWN_ERROR;
    }
}

static avs_error_t com_sockets_err_to_avs_error(com_sockets_err_t err) {
    return avs_errno(com_sockets_err_to_avs_errno(err));
}

static int prepare_stringified_host_port(xcc_net_socket_impl_t *sock,
                                         com_sockaddr_in_t *addr,
                                         uint16_t port) {
    // prepare stringified host and port; getpeername is unsupported, so
    // follow the logic used in com_translate_ip_address() internal function
    com_ip_addr_t com_ip_addr = {
        .addr = addr->sin_addr.s_addr
    };

    // HACK: PRIu8 macro expands to hhu format specifier unsupported in
    // reduced newlib, so we cast those numbers to unsigned int
    if (snprintf(sock->remote_host, sizeof(sock->remote_host), "%u.%u.%u.%u",
                 (unsigned int) COM_IP4_ADDR1(&com_ip_addr),
                 (unsigned int) COM_IP4_ADDR2(&com_ip_addr),
                 (unsigned int) COM_IP4_ADDR3(&com_ip_addr),
                 (unsigned int) COM_IP4_ADDR4(&com_ip_addr))
                    < 0
            || snprintf(sock->remote_port, sizeof(sock->remote_port),
                        "%" PRIu16, port)
                           < 0) {
        return -1;
    }

    return 0;
}

#define GETHOSTBYNAME_RETRIES 20

static avs_error_t
net_connect(avs_net_socket_t *sock_, const char *host, const char *port) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    com_sockaddr_t addr;
    uint16_t parsed_port;
    int32_t res;

    assert(sock->fd < 0);

    if (sscanf(port, "%" PRIu16, &parsed_port) != 1) {
        return avs_errno(AVS_EINVAL);
    }

    // only supports IPv4 so we don't set any hints or so
    for (int i = GETHOSTBYNAME_RETRIES;
         i > 0 && (res = com_gethostbyname((const com_char_t *) host, &addr));
         i--) {
        LOG(WARNING, "Failed to resolve hostname. Result: %d Retries left: %d",
            (int) res, i);
    }
    if (res) {
        return avs_errno(AVS_ECONNREFUSED);
    }
    com_sockaddr_in_t *addr_in = (com_sockaddr_in_t *) &addr;

    // used API doesn't accept a port, set it manually
    addr_in->sin_port = COM_HTONS(parsed_port);

    if (prepare_stringified_host_port(sock, addr_in, parsed_port)) {
        return avs_errno(AVS_UNKNOWN_ERROR);
    }

    if ((res = com_socket(COM_AF_INET, sock->socktype,
                          sock->socktype == COM_SOCK_DGRAM ? COM_IPPROTO_UDP
                                                           : COM_IPPROTO_TCP))
            < 0) {
        return com_sockets_err_to_avs_error(res);
    }
    sock->fd = res;

    if ((res = com_connect(sock->fd, &addr, addr.sa_len))) {
        return com_sockets_err_to_avs_error(res);
    }

    return AVS_OK;
}

static avs_error_t
net_send(avs_net_socket_t *sock_, const void *buffer, size_t buffer_length) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;

    // in case of UDP, use COM_MSG_DONTWAIT to disable incorrect
    // behavior in XCC sockets that fragments the buffer and
    // sends it as multiple datagrams

    int32_t res = com_send(sock->fd, buffer, buffer_length,
                           sock->socktype == COM_SOCK_DGRAM ? COM_MSG_DONTWAIT
                                                            : COM_MSG_WAIT);
    if (res < 0 || res != buffer_length) {
        return avs_errno(AVS_EIO);
    }
    sock->bytes_sent += res;
    return AVS_OK;
}

static int32_t recv_with_timeout(xcc_net_socket_impl_t *sock,
                                 com_char_t *buf,
                                 int32_t len,
                                 int64_t timeout) {
    uint32_t timeout_opt;

    if (timeout == 0) {
        int32_t res = com_recv(sock->fd, buf, len, COM_MSG_DONTWAIT);

        if (res == COM_SOCKETS_ERR_GENERAL) {
            // behavior unique to COM_MSG_DONTWAIT
            res = COM_SOCKETS_ERR_TIMEOUT;
        }
        return res;
    }

    if (timeout < 0) {
        timeout_opt = RTOSAL_WAIT_FOREVER;
    } else {
        timeout_opt = (uint32_t) timeout;
        if (timeout_opt != timeout) {
            // accidentally truncated
            return COM_SOCKETS_ERR_GENERAL;
        }
    }

    int32_t res = com_setsockopt(sock->fd, COM_SOL_SOCKET, COM_SO_RCVTIMEO,
                                 &timeout_opt, sizeof(timeout_opt));
    if (res) {
        return res;
    }

    return com_recv(sock->fd, buf, len, COM_MSG_WAIT);
}

static int32_t recv_with_buffered_data(xcc_net_socket_impl_t *sock,
                                       com_char_t *buf,
                                       int32_t len) {
    AVS_STATIC_ASSERT(
            POLL_BUF_SIZE(COM_SOCK_STREAM) == 1,
            buffer_larger_than_one_for_stream_sockets_requires_handling_partial_reads);

    int32_t res = AVS_MIN(len, sock->buffered_by_poll_len);
    memcpy(buf, sock->buffered_by_poll, res);
    sock->buffered_by_poll_len = 0;

    // in case of TCP, if we've exhausted poll() implementation's
    // buffer, attempt to read more queued data with immediate timeout
    if (sock->socktype == COM_SOCK_STREAM && len > res) {
        int32_t tail_recv_res =
                recv_with_timeout(sock, buf + res, len - res, 0);

        // ignore recv error here, will be handled within next poll()
        // call
        if (tail_recv_res > 0) {
            res += tail_recv_res;
        }
    }

    return res;
}

static avs_error_t net_receive(avs_net_socket_t *sock_,
                               size_t *out_bytes_received,
                               void *buffer,
                               size_t buffer_length) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;

    if (sock->poll_captured_recv_error) {
        // return captured error by recv call in poll() implementation
        com_sockets_err_t error = sock->poll_captured_recv_error;
        sock->poll_captured_recv_error = 0;
        return com_sockets_err_to_avs_error(error);
    }

    // assume we don't need support for zero-length datagrams
    if (buffer_length == 0) {
        return AVS_OK;
    }

    int32_t res;
    if (sock->buffered_by_poll_len > 0) {
        res = recv_with_buffered_data(sock, buffer, buffer_length);
    } else {
        int64_t timeout_ms;
        if (avs_time_duration_to_scalar(&timeout_ms, AVS_TIME_MS,
                                        sock->recv_timeout)) {
            // treat AVS_TIME_DURATION_INVALID as infinite timeout
            timeout_ms = -1;
        } else if (timeout_ms < 0) {
            timeout_ms = 0;
        }

        res = recv_with_timeout(sock, buffer, buffer_length, timeout_ms);
    }

    if (res < 0) {
        return com_sockets_err_to_avs_error(res);
    }

    *out_bytes_received = res;
    sock->bytes_received += res;
    // HACK: per Anjay's Minimal socket implementation guide, if MSG_TRUNC flag
    // is not available, we need to assume that if the size of read datagram is
    // equal to size of the buffer, the message got truncated
    if (sock->socktype == COM_SOCK_DGRAM && res == buffer_length) {
        return avs_errno(AVS_EMSGSIZE);
    }
    return AVS_OK;
}

static avs_error_t net_close(avs_net_socket_t *sock_) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    avs_error_t err = AVS_OK;
    if (sock->fd >= 0) {
        err = com_sockets_err_to_avs_error(com_closesocket(sock->fd));
        sock->fd = -1;
    }
    return err;
}

static avs_error_t net_cleanup(avs_net_socket_t **sock_ptr) {
    avs_error_t err = AVS_OK;
    if (sock_ptr && *sock_ptr) {
        err = net_close(*sock_ptr);
        avs_free(*sock_ptr);
        *sock_ptr = NULL;
    }
    return err;
}

// HACK: net_system_socket returns a pointer to the value that represents a
// system socket (sockfd_t), which is usually later dereferenced and that value
// is passed to select()/poll() method in Anjay's event loop implementation.
//
// Normally poll() accepts these numeric file descriptors, but there's no poll()
// (nor select()) method available in XCC Com sockets, so we're implementing it
// on our own.
//
// To implement the poll() method properly we need to access auxiliary data
// defined in xcc_net_socket_impl_t, so instead of mapping numeric file
// descriptors to xcc_net_socket_impl_t's (which would require maintaining some
// static map-like container in this implementation), we're defining sockfd_t to
// be a pointer to xcc_net_socket_impl_t to be able to access that auxiliary
// data easily.
//
// As net_system_socket expects a *pointer* to sockfd_t (so a double-pointer to
// xcc_net_socket_impl_t), we need to maintain some xcc_net_socket_impl_t*
// variable that we can point to and dereference it, xcc_net_socket_impl_t::self
// is just that.
static const void *net_system_socket(avs_net_socket_t *sock_) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    return sock->fd < 0 ? NULL : &sock->self;
}

static avs_error_t net_remote_host(avs_net_socket_t *sock_,
                                   char *out_buffer,
                                   size_t out_buffer_size) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    if (out_buffer_size < strlen(sock->remote_host) + 1) {
        return avs_errno(AVS_UNKNOWN_ERROR);
    }
    strcpy(out_buffer, sock->remote_host);
    return AVS_OK;
}

static avs_error_t net_remote_port(avs_net_socket_t *sock_,
                                   char *out_buffer,
                                   size_t out_buffer_size) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    if (out_buffer_size < strlen(sock->remote_port) + 1) {
        return avs_errno(AVS_UNKNOWN_ERROR);
    }
    strcpy(out_buffer, sock->remote_port);
    return AVS_OK;
}

static avs_error_t net_get_opt(avs_net_socket_t *sock_,
                               avs_net_socket_opt_key_t option_key,
                               avs_net_socket_opt_value_t *out_option_value) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    switch (option_key) {
    case AVS_NET_SOCKET_OPT_RECV_TIMEOUT:
        out_option_value->recv_timeout = sock->recv_timeout;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_STATE:
        out_option_value->state = sock->fd < 0 ? AVS_NET_SOCKET_STATE_CLOSED
                                               : AVS_NET_SOCKET_STATE_CONNECTED;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_INNER_MTU:
        // this option only controls send and sendto calls, so use only max TX
        // size
        out_option_value->mtu = CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE;
        return AVS_OK;
    case AVS_NET_SOCKET_HAS_BUFFERED_DATA:
        out_option_value->flag = false;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_BYTES_SENT:
        out_option_value->bytes_sent = sock->bytes_sent;
        return AVS_OK;
    case AVS_NET_SOCKET_OPT_BYTES_RECEIVED:
        out_option_value->bytes_received = sock->bytes_received;
        return AVS_OK;
    default:
        return avs_errno(AVS_ENOTSUP);
    }
}

static avs_error_t net_set_opt(avs_net_socket_t *sock_,
                               avs_net_socket_opt_key_t option_key,
                               avs_net_socket_opt_value_t option_value) {
    xcc_net_socket_impl_t *sock = (xcc_net_socket_impl_t *) sock_;
    switch (option_key) {
    case AVS_NET_SOCKET_OPT_RECV_TIMEOUT:
        sock->recv_timeout = option_value.recv_timeout;
        return AVS_OK;
    default:
        return avs_errno(AVS_ENOTSUP);
    }
}

static const avs_net_socket_v_table_t NET_SOCKET_VTABLE = {
    .connect = net_connect,
    .send = net_send,
    .receive = net_receive,
    .close = net_close,
    .cleanup = net_cleanup,
    .get_system_socket = net_system_socket,
    .get_remote_host = net_remote_host,
    .get_remote_port = net_remote_port,
    .get_opt = net_get_opt,
    .set_opt = net_set_opt
};

static avs_error_t
net_create_socket(avs_net_socket_t **socket_ptr,
                  const avs_net_socket_configuration_t *configuration,
                  int32_t socktype) {
    assert(socket_ptr);
    assert(!*socket_ptr);
    (void) configuration;
    xcc_net_socket_impl_t *socket = (xcc_net_socket_impl_t *) avs_calloc(
            1, sizeof(*socket) + POLL_BUF_SIZE(socktype));
    if (!socket) {
        return avs_errno(AVS_ENOMEM);
    }
    socket->operations = &NET_SOCKET_VTABLE;
    socket->self = socket;
    socket->socktype = socktype;
    socket->fd = -1;
    socket->recv_timeout = avs_time_duration_from_scalar(30, AVS_TIME_S);
    *socket_ptr = (avs_net_socket_t *) socket;
    return AVS_OK;
}

avs_error_t _avs_net_create_udp_socket(avs_net_socket_t **socket_ptr,
                                       const void *configuration) {
    return net_create_socket(
            socket_ptr, (const avs_net_socket_configuration_t *) configuration,
            COM_SOCK_DGRAM);
}

avs_error_t _avs_net_create_tcp_socket(avs_net_socket_t **socket_ptr,
                                       const void *configuration) {
    return net_create_socket(
            socket_ptr, (const avs_net_socket_configuration_t *) configuration,
            COM_SOCK_STREAM);
}

int xcc_net_socket_poll_single(xcc_net_socket_impl_t *socket,
                               int64_t timeout_ms,
                               short events,
                               short *revents) {
    // this implementation is only suited to be used with Anjay's event loop,
    // so available flags are limited
    assert(!(events & ~(XCC_NET_SOCKET_POLLIN | XCC_NET_SOCKET_POLLERR)));

    if (socket->fd < 0) {
        return -1;
    }
    if (socket->poll_captured_recv_error) {
        *revents = XCC_NET_SOCKET_POLLERR;
        return 1;
    }
    if (socket->buffered_by_poll_len > 0) {
        *revents = events & XCC_NET_SOCKET_POLLIN;
        return !!*revents;
    }
    int32_t res =
            recv_with_timeout(socket, socket->buffered_by_poll,
                              POLL_BUF_SIZE(socket->socktype), timeout_ms);
    // assume we don't need support for zero-length datagrams
    if (res == COM_SOCKETS_ERR_TIMEOUT || res == 0) {
        *revents = 0;
        return 0;
    }
    if (res < 0) {
        // poll() spec says that POLLERR should be reported even if
        // this event is not listened for
        *revents = XCC_NET_SOCKET_POLLERR;
        socket->poll_captured_recv_error = res;
        return 1;
    }

    socket->buffered_by_poll_len = res;
    *revents = events & XCC_NET_SOCKET_POLLIN;
    return !!*revents;
}
