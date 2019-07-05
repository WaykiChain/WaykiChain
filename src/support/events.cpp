// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "events.h"
#include "commons/util.h"

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/util.h>
#include <event2/listener.h>

///////////////////////////////////////////////////////////////////////////////
// functions copy from libevent/http.c, and support to bind with IPV6_V6ONLY

#ifdef SOCK_NONBLOCK
#define EVUTIL_SOCK_NONBLOCK SOCK_NONBLOCK
#else
#define EVUTIL_SOCK_NONBLOCK 0x4000000
#endif
#ifdef SOCK_CLOEXEC
#define EVUTIL_SOCK_CLOEXEC SOCK_CLOEXEC
#else
#define EVUTIL_SOCK_CLOEXEC 0x80000000
#endif

/** copy from the implements of ev_http */
static struct evutil_addrinfo* make_addrinfo(const char* address, ev_uint16_t port) {
    struct evutil_addrinfo* ai = NULL;

    struct evutil_addrinfo hints;
    char strport[NI_MAXSERV];
    int ai_result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    /* turn NULL hostname into INADDR_ANY, and skip looking up any address
     * types we don't have an interface to connect to. */
    hints.ai_flags = EVUTIL_AI_PASSIVE | EVUTIL_AI_ADDRCONFIG;
    evutil_snprintf(strport, sizeof(strport), "%d", port);
    if ((ai_result = evutil_getaddrinfo(address, strport, &hints, &ai)) != 0) {
        if (ai_result == EVUTIL_EAI_SYSTEM) {
            LogPrint("ERROR", "make_addrinfo getaddrinfo error, address=%s\n", address);
        } else {
            LogPrint("ERROR", "make_addrinfo getaddrinfo error: %s, address=%s\n",
                     evutil_gai_strerror(ai_result), address);
        }
        return (NULL);
    }

    return (ai);
}

/* Faster version of evutil_make_socket_closeonexec for internal use.
 *
 * Requires that no F_SETFD flags were previously set on the fd.
 */
static int evutil_fast_socket_closeonexec(evutil_socket_t fd) {
#if !defined(_WIN32) && defined(EVENT__HAVE_SETFD)
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        LogPrint("ERROR", "evutil_fast_socket_closeonexec fcntl(%d, F_SETFD)\n", fd);
        return -1;
    }
#endif
    return 0;
}

/* Faster version of evutil_make_socket_nonblocking for internal use.
 *
 * Requires that no F_SETFL flags were previously set on the fd.
 */
static int evutil_fast_socket_nonblocking(evutil_socket_t fd) {
#ifdef _WIN32
    return evutil_make_socket_nonblocking(fd);
#else
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        LogPrint("ERROR", "evutil_fast_socket_nonblocking fcntl(%d, F_SETFL)\n", fd);
        return -1;
    }
    return 0;
#endif
}

/* Internal wrapper around 'socket' to provide Linux-style support for
 * syscall-saving methods where available.
 *
 * In addition to regular socket behavior, you can use a bitwise or to set the
 * flags EVUTIL_SOCK_NONBLOCK and EVUTIL_SOCK_CLOEXEC in the 'type' argument,
 * to make the socket nonblocking or close-on-exec with as few syscalls as
 * possible.
 */
evutil_socket_t evutil_socket_(int domain, int type, int protocol) {
    evutil_socket_t r;
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    r = socket(domain, type, protocol);
    if (r >= 0)
        return r;
    else if ((type & (SOCK_NONBLOCK | SOCK_CLOEXEC)) == 0)
        return -1;
#endif
#define SOCKET_TYPE_MASK (~(EVUTIL_SOCK_NONBLOCK | EVUTIL_SOCK_CLOEXEC))
    r = socket(domain, type & SOCKET_TYPE_MASK, protocol);
    if (r < 0) return -1;
    if (type & EVUTIL_SOCK_NONBLOCK) {
        if (evutil_fast_socket_nonblocking(r) < 0) {
            evutil_closesocket(r);
            return -1;
        }
    }
    if (type & EVUTIL_SOCK_CLOEXEC) {
        if (evutil_fast_socket_closeonexec(r) < 0) {
            evutil_closesocket(r);
            return -1;
        }
    }
    return r;
}

/* Create a non-blocking socket and bind it */
/* todo: rename this function */
static evutil_socket_t bind_socket_ai(struct evutil_addrinfo* ai, int reuse) {
    evutil_socket_t fd;

    int on = 1, r;

    /* Create listen socket */
    fd = evutil_socket_(ai ? ai->ai_family : AF_INET,
                        SOCK_STREAM | EVUTIL_SOCK_NONBLOCK | EVUTIL_SOCK_CLOEXEC, 0);
    if (fd == -1) {
        // event_sock_warn(-1, "socket");
        return (-1);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&on, sizeof(on)) < 0) {
        LogPrint("ERROR", "bind_socket_ai setsockopt(%d, SOL_SOCKET, SO_KEEPALIVE) failed\n", fd);
        evutil_closesocket(fd);
        return (-1);
    }

    if (evutil_make_listen_socket_reuseable(fd) < 0) {
        LogPrint("ERROR", "bind_socket_ai evutil_make_listen_socket_reuseable failed\n");
        evutil_closesocket(fd);
        return (-1);
    }

#if defined(IPV6_V6ONLY)
    int ai_family = ai ? ai->ai_family : AF_INET;
    // AF_INET6
    if (ai_family == PF_INET6) {
        on = 1;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&on, (ev_socklen_t)sizeof(on)) < 0) {
            LogPrint("ERROR", "bind_socket_ai evutil_make_listen_socket_ipv6only failed\n");
            return (-1);
        }
    }
#endif

    if (ai != NULL) {
        r = bind(fd, ai->ai_addr, (ev_socklen_t)ai->ai_addrlen);
        if (r == -1) {
            int serrno = EVUTIL_SOCKET_ERROR();
            LogPrint("ERROR", "bind_socket_ai bind failed, err: %s\n",
                     evutil_socket_error_to_string(serrno));
            evutil_closesocket(fd);
            return (-1);
        }
    }

    return (fd);
}

static evutil_socket_t bind_socket(const char* address, ev_uint16_t port, int reuse) {
    evutil_socket_t fd;
    struct evutil_addrinfo* aitop = NULL;

    /* just create an unbound socket */
    if (address == NULL && port == 0) return bind_socket_ai(NULL, 0);

    aitop = make_addrinfo(address, port);

    if (aitop == NULL) return (-1);

    fd = bind_socket_ai(aitop, reuse);

    evutil_freeaddrinfo(aitop);

    return (fd);
}

struct evhttp_bound_socket* evhttp_bind_accept_socket(struct evhttp* http, const char* address,
                                                      ev_uint16_t port) {
    evutil_socket_t fd;
    struct evhttp_bound_socket* bound;
    int serrno;

    if ((fd = bind_socket(address, port, 1 /*reuse*/)) == -1) return (NULL);

    if (listen(fd, 128) == -1) {
        serrno = EVUTIL_SOCKET_ERROR();
        LogPrint("ERROR", "evhttp_bind_accept_socket listen failed! err: \n",
                 evutil_socket_error_to_string(serrno));
        evutil_closesocket(fd);
        EVUTIL_SET_SOCKET_ERROR(serrno);
        return (NULL);
    }

    bound = evhttp_accept_socket_with_handle(http, fd);

    if (bound != NULL) {
        LogPrint("LIBEVENT",
                 "evhttp_bind_accept_socket Bound to port %d - Awaiting connections ... \n", port);
        return (bound);
    }

    return (NULL);
}