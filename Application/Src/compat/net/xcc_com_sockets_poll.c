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

#include <avsystem/commons/avs_time.h>
#include <avsystem/commons/xcc_com_posix_compat.h>

#include "xcc_com_sockets.h"

// HACK: there is no poll() call available on XCC com sockets, so we're
// implementing it by repeatedly polling individual sockets in a loop, in short
// bursts to throttle down the loop that would be otherwise a busy spin
//
// Most of the time there's only one socket to poll, so we do that by sleeping
// on a recv() call underneath; the result of that recv() call is buffered and
// returned on subsequent calls to avs_net_socket_receive()

static int64_t calculate_timeout(avs_time_monotonic_t deadline) {
    static const int64_t quantum_ms = 50;

    if (!avs_time_monotonic_valid(deadline)) {
        // for AVS_TIME_MONOTONIC_INVALID always return quantum to implement
        // infinite timeout
        return quantum_ms;
    }

    avs_time_duration_t until_deadline =
            avs_time_monotonic_diff(deadline, avs_time_monotonic_now());
    if (avs_time_duration_less(until_deadline, AVS_TIME_DURATION_ZERO)) {
        until_deadline = AVS_TIME_DURATION_ZERO;
    }

    int64_t until_deadline_ms;
    int res = avs_time_duration_to_scalar(
            &until_deadline_ms, AVS_TIME_MS, until_deadline);
    assert(!res);

    return AVS_MIN(until_deadline_ms, quantum_ms);
}

static int throttled_sweep(struct xcc_net_socket_pollfd *fds,
                           nfds_t nfds,
                           avs_time_monotonic_t deadline) {
    for (nfds_t i = 0; i < nfds; i++) {
        int res = xcc_net_socket_poll_single(fds[i].fd,
                                             calculate_timeout(deadline),
                                             fds[i].events,
                                             &fds[i].revents);
        // we can return early here for responsiveness; other ready sockets will
        // be captured by next nonblocking_sweep() call
        if (res) {
            return res;
        }
    }
    return 0;
}

static int nonblocking_sweep(struct xcc_net_socket_pollfd *fds, nfds_t nfds) {
    int ready = 0;
    for (nfds_t i = 0; i < nfds; i++) {
        int res = xcc_net_socket_poll_single(
                fds[i].fd, 0, fds[i].events, &fds[i].revents);
        if (res < 0) {
            return res;
        }

        ready += res;
    }
    return ready;
}

static int poll_multiple_sockets(struct xcc_net_socket_pollfd *fds,
                                 nfds_t nfds,
                                 int timeout_ms) {
    // treat AVS_TIME_MONOTIC_INVALID as infinite deadline
    const avs_time_monotonic_t deadline =
            timeout_ms < 0
                    ? AVS_TIME_MONOTONIC_INVALID
                    : avs_time_monotonic_add(avs_time_monotonic_now(),
                                             avs_time_duration_from_scalar(
                                                     timeout_ms, AVS_TIME_MS));

    // attempt to return immediately if some sockets are already ready, this
    // also collects sockets that could be missed in the last iteration of
    // throttled_sweep()
    int res = nonblocking_sweep(fds, nfds);
    if (res) {
        return res;
    }

    while (avs_time_monotonic_before(avs_time_monotonic_now(), deadline)) {
        res = throttled_sweep(fds, nfds, deadline);
        if (res) {
            return res;
        }
    }

    return 0;
}

static int poll_single_socket(struct xcc_net_socket_pollfd *fd,
                              int timeout_ms) {
    xcc_net_socket_impl_t *socket = fd->fd;
    return xcc_net_socket_poll_single(
            socket, timeout_ms, fd->events, &fd->revents);
}

int xcc_net_socket_poll(struct xcc_net_socket_pollfd *fds,
                        nfds_t nfds,
                        int timeout) {
    if (nfds == 0) {
        return 0;
    }
    return nfds == 1 ? poll_single_socket(fds, timeout)
                     : poll_multiple_sockets(fds, nfds, timeout);
}
