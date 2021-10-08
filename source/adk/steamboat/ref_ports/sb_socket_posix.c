/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_socket_posix.c

steamboat socket implementation for posix
*/

#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_socket.h"

#ifndef __USE_GNU
#define __USE_GNU // so we get EAI_ADDRFAMILY
#endif

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int sb_sock_family_to_sys(const sb_socket_family_e domain) {
    int sys_domain = 0;
    switch (domain) {
        case sb_socket_family_IPv4:
            sys_domain = AF_INET;
            break;
        case sb_socket_family_IPv6:
            sys_domain = AF_INET6;
            break;
        case sb_socket_family_dont_care:
            sys_domain = AF_UNSPEC;
            break;
        default:
            TRAP("unsupported sb_socket_domain_e: %i", domain);
    }
    return sys_domain;
}

static sb_socket_family_e sys_to_sb_sock_family(const int sys_domain) {
    sb_socket_family_e domain = 0;
    switch (sys_domain) {
        case AF_INET:
            domain = sb_socket_family_IPv4;
            break;
        case AF_INET6:
            domain = sb_socket_family_IPv6;
            break;
        case AF_UNSPEC:
            domain = sb_socket_family_dont_care;
            break;
        default:
            domain = sb_socket_family_unsupported;
    }
    return domain;
}

static int sb_sock_type_to_sys(const sb_socket_type_e sock_type) {
    int sys_sock = 0;

    switch (sock_type) {
        case sb_socket_type_stream:
            sys_sock = SOCK_STREAM;
            break;
        case sb_socket_type_datagram:
            sys_sock = SOCK_DGRAM;
            break;
        case sb_socket_type_any:
            sys_sock = 0;
            break;
        default:
            TRAP("unsupported sb_socket_type_e: %i", sock_type);
    }
    return sys_sock;
}

static int sys_to_sb_sock_type(const int sys_sock) {
    sb_socket_type_e sock_type = 0;

    switch (sys_sock) {
        case SOCK_STREAM:
            sock_type = sb_socket_type_stream;
            break;
        case SOCK_DGRAM:
            sock_type = sb_socket_type_datagram;
            break;
        case 0:
            sock_type = sb_socket_type_any;
            break;
        default:
            sock_type = sb_socket_type_unsupported;
    }
    return sock_type;
}

static int sb_socket_protocol_to_sys(const sb_socket_protocol_type_e protocol) {
    int sys_protocol = 0;

    switch (protocol) {
        case sb_socket_protocol_tcp:
            sys_protocol = IPPROTO_TCP;
            break;
        case sb_socket_protocol_udp:
            sys_protocol = IPPROTO_UDP;
            break;
        case sb_socket_protocol_any:
            sys_protocol = 0;
            break;
        default:
            TRAP("unspported sb_socket_protocol_type_e, %i", protocol);
    }
    return sys_protocol;
}

static sb_socket_protocol_type_e sys_to_sb_socket_protocol(const int sys_protocol) {
    sb_socket_protocol_type_e protocol = 0;

    switch (sys_protocol) {
        case IPPROTO_TCP:
            protocol = sb_socket_protocol_tcp;
            break;
        case IPPROTO_UDP:
            protocol = sb_socket_protocol_udp;
            break;
        case 0:
            protocol = sb_socket_protocol_any;
            break;
        default:
            protocol = sb_socket_protocol_unsupported;
    }
    return protocol;
}

static int sb_to_sys_sock_flags(const sb_getaddrinfo_flags_e ai_flags) {
    return ai_flags == sb_getaddrinfo_flag_ai_passive ? AI_PASSIVE : 0;
}

static sb_getaddrinfo_flags_e sys_to_sb_sock_flags(const int sys_flags) {
    return sys_flags == AI_PASSIVE ? sb_getaddrinfo_flag_ai_passive : sb_getaddrinfo_flag_none;
}

static void sys_sockaddr_to_adk(sb_sockaddr_t * const out_sb_sockaddr, const struct sockaddr * const sys_sockaddr) {
    ASSERT(out_sb_sockaddr);
    ASSERT(sys_sockaddr);

    switch (sys_sockaddr->sa_family) {
        case AF_INET: {
            struct sockaddr_in * ipv4_sockaddr = (struct sockaddr_in *)sys_sockaddr;
            out_sb_sockaddr->sin_family = sb_socket_family_IPv4;
            out_sb_sockaddr->sin_port = ipv4_sockaddr->sin_port;
            memcpy(&out_sb_sockaddr->sin_addr, &ipv4_sockaddr->sin_addr, sizeof(ipv4_sockaddr->sin_addr));
            break;
        }
        case AF_INET6: {
            const struct sockaddr_in6 * ipv6_sockaddr = (struct sockaddr_in6 *)sys_sockaddr;
            out_sb_sockaddr->sin_family = sb_socket_family_IPv6;
            out_sb_sockaddr->sin_port = ipv6_sockaddr->sin6_port;
            out_sb_sockaddr->ipv6_flowinfo = ipv6_sockaddr->sin6_flowinfo;
            out_sb_sockaddr->ipv6_scope_id = ipv6_sockaddr->sin6_scope_id;
            memcpy(&out_sb_sockaddr->sin_addr, &ipv6_sockaddr->sin6_addr, sizeof(ipv6_sockaddr->sin6_addr));
            break;
        }
        default:
            TRAP("Invalid socket family %i", sys_sockaddr->sa_family);
    }
}

static void sb_sockaddr_to_sys(const sb_sockaddr_t * const sb_sockaddr, struct sockaddr_storage * const out_sys_sockaddr) {
    ASSERT(sb_sockaddr);
    ASSERT(out_sys_sockaddr);
    memset(out_sys_sockaddr, 0, sizeof(struct sockaddr_storage));
    switch (sb_sockaddr->sin_family) {
        case sb_socket_family_IPv4: {
            struct sockaddr_in * ipv4_sockaddr = (struct sockaddr_in *)out_sys_sockaddr;
            ipv4_sockaddr->sin_family = AF_INET;
            ipv4_sockaddr->sin_port = sb_sockaddr->sin_port;
            memcpy(&ipv4_sockaddr->sin_addr, &sb_sockaddr->sin_addr, sizeof(ipv4_sockaddr->sin_addr));
            break;
        }
        case sb_socket_family_IPv6: {
            struct sockaddr_in6 * ipv6_sockaddr = (struct sockaddr_in6 *)out_sys_sockaddr;
            ipv6_sockaddr->sin6_family = AF_INET6;
            ipv6_sockaddr->sin6_port = sb_sockaddr->sin_port;
            ipv6_sockaddr->sin6_flowinfo = sb_sockaddr->ipv6_flowinfo;
            ipv6_sockaddr->sin6_scope_id = sb_sockaddr->ipv6_scope_id;
            memcpy(&ipv6_sockaddr->sin6_addr, &sb_sockaddr->sin_addr, sizeof(ipv6_sockaddr->sin6_addr));
            break;
        }
        default:
            TRAP("Invalid socket family %i", sb_sockaddr->sin_family);
    }
}

sb_getaddrinfo_error_code_e sys_to_getddrinfo_error(const int sys_error) {
    sb_getaddrinfo_error_code_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
        case EAI_ADDRFAMILY:
            sb_err = sb_getaddrinfo_address_family;
            break;
        case EAI_AGAIN:
            sb_err = sb_getaddrinfo_again;
            break;
        case EAI_SERVICE:
            sb_err = sb_getaddrinfo_service;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static sb_socket_connect_error_e sys_to_connect_error(const int sys_error) {
    sb_socket_connect_error_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
        case EACCES:
            sb_err = sb_socket_connect_access;
            break;
        case EADDRINUSE:
            sb_err = sb_socket_connect_address_in_use;
            break;
        case EADDRNOTAVAIL:
            sb_err = sb_socket_connect_address_not_avail;
            break;
        case EAFNOSUPPORT:
            sb_err = sb_socket_connect_no_support;
            break;
        case EAGAIN:
            sb_err = sb_socket_connect_would_block;
            break;
        case EALREADY:
            sb_err = sb_socket_connect_already;
            break;
        case ECONNREFUSED:
            sb_err = sb_socket_connect_connection_refused;
            break;
        case EINPROGRESS:
            sb_err = sb_socket_connect_in_progress;
            break;
        case ENETUNREACH:
            sb_err = sb_socket_connect_network_unreachable;
            break;
        case ETIMEDOUT:
            sb_err = sb_socket_connect_timed_out;
            break;
        case EISCONN:
            sb_err = sb_socket_connect_is_connected;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static sb_socket_accept_error_e sys_to_accept_error(const int sys_error) {
    sb_socket_accept_error_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
        case ECONNABORTED:
            sb_err = sb_socket_accept_connection_aborted;
            break;
        case EINVAL:
            sb_err = sb_socket_accept_invalid;
            break;
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
            FALLTHROUGH;
#endif
        case EWOULDBLOCK:
            sb_err = sb_socket_accept_would_block;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static sb_socket_bind_error_e sys_to_bind_error(const int sys_error) {
    sb_socket_bind_error_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
        case EACCES:
            sb_err = sb_socket_bind_access;
            break;
        case EADDRINUSE:
            sb_err = sb_socket_bind_in_use;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static sb_socket_listen_error_e sys_to_listen_error(const int sys_error) {
    sb_socket_listen_error_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
        case EADDRINUSE:
            sb_err = sb_socket_listen_address_in_use;
            break;
        case EOPNOTSUPP:
            sb_err = sb_socket_listen_not_supported;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static sb_socket_receive_error_e sys_to_receive_error(const int sys_error) {
    sb_socket_receive_error_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
            FALLTHROUGH;
#endif
        case EWOULDBLOCK:
            sb_err = sb_socket_receive_would_block;
            break;
        case ECONNREFUSED:
            sb_err = sb_socket_receive_connection_refused;
            break;
        case ENOTCONN:
            sb_err = sb_socket_receive_not_connected;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static sb_socket_send_error_e sys_to_send_error(const int sys_error) {
    sb_socket_send_error_e sb_err = 0;
    switch (sys_error) {
        case 0:
            break;
        case EACCES:
            sb_err = sb_socket_send_access;
            break;
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
            FALLTHROUGH;
#endif
        case EWOULDBLOCK:
            sb_err = sb_socket_send_would_block;
            break;
        case ECONNRESET:
            sb_err = sb_socket_send_connection_reset;
            break;
        case EMSGSIZE:
            sb_err = sb_socket_send_message_size;
            break;
        case ENOBUFS:
            sb_err = sb_socket_send_no_buffers;
            break;
        case ENOTCONN:
            sb_err = sb_socket_send_not_connected;
            break;
        case EPIPE:
            sb_err = sb_socket_send_shutdown;
            break;
        default:
            sb_err = __sb_generic_system_failure;
    }
    return sb_err;
}

static int get_platform_error() {
    return errno;
}

sb_getaddrinfo_result_t sb_getaddrinfo(
    const char * const address,
    const char * const port,
    const sb_addrinfo_t * const hint,
    sb_addrinfo_t * const out_addrinfos,
    uint32_t * const out_addrinfo_size) {
    ASSERT(address || port);
    ASSERT(out_addrinfo_size);

    struct addrinfo hints = {0};
    sb_getaddrinfo_result_t err_codes = {0};
    if (hint) {
        hints.ai_family = sb_sock_family_to_sys(hint->ai_family);
        hints.ai_socktype = sb_sock_type_to_sys(hint->ai_socktype);
        hints.ai_protocol = sb_socket_protocol_to_sys(hint->ai_protocol);
    }

    struct addrinfo * addrinfo_link_list = NULL;
    err_codes.system_code = getaddrinfo(address, port, hint ? &hints : NULL, &addrinfo_link_list);
    if (err_codes.system_code) {
        err_codes.result = sys_to_getddrinfo_error(err_codes.system_code);
        return err_codes;
    }

    uint32_t num_sys_addrinfos = 0;
    for (struct addrinfo * curr = addrinfo_link_list; curr != NULL; curr = curr->ai_next, ++num_sys_addrinfos) {
    }

    if (!out_addrinfos) {
        freeaddrinfo(addrinfo_link_list);
        *out_addrinfo_size = num_sys_addrinfos;
        return err_codes;
    }

    *out_addrinfo_size = min_uint32_t(num_sys_addrinfos, *out_addrinfo_size);

    struct addrinfo * curr_sys_addrinfo = addrinfo_link_list;
    for (uint32_t curr_sb_addrinfo_ind = 0; (curr_sys_addrinfo != NULL) && (curr_sb_addrinfo_ind < *out_addrinfo_size); curr_sys_addrinfo = curr_sys_addrinfo->ai_next, ++curr_sb_addrinfo_ind) {
        sb_addrinfo_t * const curr_sb_addrinfo = &out_addrinfos[curr_sb_addrinfo_ind];
        curr_sb_addrinfo->ai_family = sys_to_sb_sock_family(curr_sys_addrinfo->ai_family);
        curr_sb_addrinfo->ai_socktype = sys_to_sb_sock_type(curr_sys_addrinfo->ai_socktype);
        curr_sb_addrinfo->ai_protocol = sys_to_sb_socket_protocol(curr_sys_addrinfo->ai_protocol);
        curr_sb_addrinfo->ai_flags = sys_to_sb_sock_flags(curr_sys_addrinfo->ai_flags);

        sys_sockaddr_to_adk(&curr_sb_addrinfo->ai_addr, curr_sys_addrinfo->ai_addr);
    }

    freeaddrinfo(addrinfo_link_list);
    return err_codes;
}

int sb_create_socket(const sb_socket_family_e domain, const sb_socket_type_e sock_type, const sb_socket_protocol_type_e protocol, sb_socket_t * const out_socket) {
    const int socket_id = socket(sb_sock_family_to_sys(domain), sb_sock_type_to_sys(sock_type), sb_socket_protocol_to_sys(protocol));

    if (socket_id < 0) {
        return get_platform_error();
    }

    out_socket->socket_id = socket_id;
    return 0;
}

bool sb_enable_blocking_socket(const sb_socket_t sock, const sb_socket_blocking_e blocking_mode) {
    ASSERT(blocking_mode == sb_socket_blocking_disabled || blocking_mode == sb_socket_blocking_enabled);
    int flags = fcntl(sock.socket_id, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    flags = blocking_mode == sb_socket_blocking_enabled ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(sock.socket_id, F_SETFL, flags) == 0;
}

bool sb_socket_set_tcp_no_delay(const sb_socket_t sock, const sb_socket_tcp_delay_mode_e tcp_delay_mode) {
    ASSERT((tcp_delay_mode == sb_socket_tcp_delay_none) || (tcp_delay_mode == sb_socket_tcp_delay_nagle));
    const int use_nodelay = tcp_delay_mode == sb_socket_tcp_delay_none ? 1 : 0;
    return setsockopt(sock.socket_id, IPPROTO_TCP, TCP_NODELAY, (void *)&use_nodelay, sizeof(use_nodelay)) == 0;
}

bool sb_socket_set_linger(const sb_socket_t sock, const sb_socket_linger_mode_e linger_mode, const int linger_timeout_in_seconds) {
    ASSERT((linger_mode == sb_socket_linger) || (linger_mode == sb_socket_no_linger));
    struct linger linger;
    linger.l_onoff = linger_mode == sb_socket_linger;
    linger.l_linger = linger_timeout_in_seconds;
    return setsockopt(sock.socket_id, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == 0;
}

sb_socket_bind_result_t sb_bind_socket(const sb_socket_t sock, const sb_sockaddr_t * const addr) {
    struct sockaddr_storage storage;
    sb_socket_bind_result_t err_codes = {0};
    sb_sockaddr_to_sys(addr, &storage);
    unsigned int addr_len = sizeof(storage);
    if (bind(sock.socket_id, (const struct sockaddr *)&storage, addr_len) == -1) {
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_bind_error(err_codes.system_code);
    }
    return err_codes;
}

sb_socket_listen_result_t sb_listen_socket(const sb_socket_t sock, const int backlog) {
    sb_socket_listen_result_t err_codes = {0};
    if (listen(sock.socket_id, backlog) == -1) {
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_listen_error(err_codes.system_code);
    }
    return err_codes;
}

sb_socket_accept_result_t sb_accept_socket(const sb_socket_t sock, sb_sockaddr_t * const optional_sockaddr, sb_socket_t * const out_sock) {
    ASSERT(out_sock);
    sb_socket_accept_result_t err_codes = {0};
    if (optional_sockaddr) {
        struct sockaddr_storage storage;
        unsigned int addrlen = sizeof(storage);
        *out_sock = (sb_socket_t){.socket_id = accept(sock.socket_id, (struct sockaddr *)&storage, &addrlen)};
        sys_sockaddr_to_adk(optional_sockaddr, (struct sockaddr *)&storage);

    } else {
        *out_sock = (sb_socket_t){.socket_id = accept(sock.socket_id, NULL, NULL)};
    }

    if (out_sock->socket_id == (uintptr_t)-1) {
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_accept_error(err_codes.system_code);
    }
    return err_codes;
}

sb_socket_connect_result_t sb_connect_socket(const sb_socket_t sock, const sb_sockaddr_t * const addr) {
    sb_socket_connect_result_t err_codes = {0};
    struct sockaddr_storage storage;
    sb_sockaddr_to_sys(addr, &storage);
    unsigned int addr_len = sizeof(storage);

    if (connect(sock.socket_id, (struct sockaddr *)&storage, addr_len) == -1) {
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_connect_error(err_codes.system_code);
    }
    return err_codes;
}

void sb_close_socket(const sb_socket_t sock) {
    const int err = close(sock.socket_id);
    if (err != 0) {
        TRAP("close socket error: %i", get_platform_error());
    }
}

void sb_shutdown_socket(const sb_socket_t sock, const sb_socket_shutdown_flags_e shutdown_flags) {
    int flags;
    if (shutdown_flags == sb_socket_shutdown_write) {
        flags = SHUT_WR;
    } else if (shutdown_flags == sb_socket_shutdown_read) {
        flags = SHUT_RD;
    } else {
        ASSERT(shutdown_flags == sb_socket_shutdown_read_write);
        flags = SHUT_RDWR;
    }

    const int err = shutdown(sock.socket_id, flags);
    if ((err != 0) && (get_platform_error() != ENOTCONN)) {
        TRAP("shutdown socket error: %i", get_platform_error());
    }
}

sb_socket_select_result_e sb_socket_select(const sb_socket_t socket, milliseconds_t * const timeout) {
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&except_set);

    FD_SET(socket.socket_id, &read_set);
    FD_SET(socket.socket_id, &write_set);
    FD_SET(socket.socket_id, &except_set);

    struct timeval time = {0};
    if (timeout) {
        time.tv_usec = (timeout->ms % 1000) * 1000;
        time.tv_sec = timeout->ms / 1000;
    }
    const int nfds = (int)socket.socket_id + 1; // highest-numbered file descriptor in any of the three sets, plus 1
    select(nfds, &read_set, &write_set, &except_set, timeout->ms ? &time : NULL);
    sb_socket_select_result_e select_flags = FD_ISSET(socket.socket_id, &read_set) ? sb_socket_select_flag_readable : 0;
    select_flags |= FD_ISSET(socket.socket_id, &write_set) ? sb_socket_select_flag_writable : 0;
    select_flags |= FD_ISSET(socket.socket_id, &except_set) ? sb_socket_select_flag_exception : 0;
    return select_flags;
}

static int sb_socket_receive_to_sys_flags(const sb_socket_receive_flags_e flags) {
    int sys_flags = 0;
    sys_flags |= flags & sb_socket_receive_flag_peek ? MSG_PEEK : 0;
    sys_flags |= flags & sb_socket_receive_flag_out_of_band ? MSG_OOB : 0;
    sys_flags |= flags & sb_socket_receive_flag_wait_all ? MSG_WAITALL : 0;
    sys_flags |= flags & sb_socket_receive_flag_no_sig_pipe ? MSG_NOSIGNAL : 0;
    return sys_flags;
}

sb_socket_receive_result_t sb_socket_receive(const sb_socket_t sock, const mem_region_t buffer, const sb_socket_receive_flags_e flags, int * const receive_size) {
    ASSERT(buffer.size > 0);
    ASSERT(receive_size);
    sb_socket_receive_result_t err_codes = {0};

    *receive_size = recv(sock.socket_id, buffer.ptr, (int)buffer.size, sb_socket_receive_to_sys_flags(flags));

    if (*receive_size == -1) {
        *receive_size = 0;
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_receive_error(err_codes.system_code);
    }
    return err_codes;
}

sb_socket_receive_result_t sb_socket_receive_from(const sb_socket_t sock, const mem_region_t buffer, const sb_socket_receive_flags_e flags, sb_sockaddr_t * const out_addr, int * const receive_size) {
    ASSERT(buffer.size > 0);
    ASSERT(receive_size);
    sb_socket_receive_result_t err_codes = {0};

    struct sockaddr_storage storage;
    unsigned int storage_len = sizeof(storage);
    *receive_size = recvfrom(sock.socket_id, buffer.ptr, buffer.size, sb_socket_receive_to_sys_flags(flags), out_addr == NULL ? NULL : (struct sockaddr *)&storage, &storage_len);

    if (*receive_size == -1) {
        *receive_size = 0;
        *out_addr = (sb_sockaddr_t){0};
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_receive_error(err_codes.system_code);
        return err_codes;
    }
    if (out_addr) {
        sys_sockaddr_to_adk(out_addr, (struct sockaddr *)&storage);
    }
    return err_codes;
}

static int sb_socket_send_to_sys_flags(const sb_socket_send_flags_e flags) {
    int sys_flags = 0;
    sys_flags |= flags & sb_socket_send_flag_dont_route ? MSG_DONTROUTE : 0;
    sys_flags |= flags & sb_socket_send_flag_out_of_band ? MSG_OOB : 0;
    sys_flags |= flags & sb_socket_send_flag_no_sig_pipe ? MSG_NOSIGNAL : 0;
    return sys_flags;
}

sb_socket_send_result_t sb_socket_send(const sb_socket_t sock, const const_mem_region_t message, const sb_socket_send_flags_e flags, int * const bytes_sent) {
    ASSERT(message.size > 0);
    ASSERT(bytes_sent);
    sb_socket_send_result_t err_codes = {0};
    *bytes_sent = send(sock.socket_id, message.ptr, message.size, sb_socket_send_to_sys_flags(flags) | MSG_NOSIGNAL);

    if (*bytes_sent == -1) {
        *bytes_sent = 0;
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_send_error(err_codes.system_code);
    }
    return err_codes;
}

sb_socket_send_result_t sb_socket_send_to(const sb_socket_t sock, const const_mem_region_t message, const sb_socket_send_flags_e flags, const sb_sockaddr_t * const addr, int * const bytes_sent) {
    ASSERT(message.size > 0);
    ASSERT(addr);
    ASSERT(bytes_sent);
    sb_socket_send_result_t err_codes = {0};
    struct sockaddr_storage storage;
    sb_sockaddr_to_sys(addr, &storage);
    *bytes_sent = sendto(sock.socket_id, message.ptr, message.size, sb_socket_send_to_sys_flags(flags) | MSG_NOSIGNAL, (const struct sockaddr *)&storage, sizeof(storage));

    if (*bytes_sent == -1) {
        *bytes_sent = 0;
        err_codes.system_code = get_platform_error();
        err_codes.result = sys_to_send_error(err_codes.system_code);
    }
    return err_codes;
}

static int sb_getnameinfo_to_sys(const sb_getnameinfo_flags_e sb_flags) {
    int sys_flags = 0;
    sys_flags |= sb_flags & sb_getnameinfo_numeric_service ? NI_NUMERICSERV : 0;
    sys_flags |= sb_flags & sb_getnameinfo_numeric_host ? NI_NUMERICHOST : 0;
    return sys_flags;
}

void sb_getnameinfo(const sb_sockaddr_t * const addr, mem_region_t * const host, mem_region_t * const service, const sb_getnameinfo_flags_e flags) {
    ASSERT(addr);
    ASSERT((host && host->size > 0) || (service && service->size > 0));

    struct sockaddr_storage sys_addr = {0};
    sb_sockaddr_to_sys(addr, &sys_addr);

    char * host_ptr = host ? host->ptr : NULL;
    const size_t host_size = host ? host->size : 0;
    char * service_ptr = service ? service->ptr : NULL;
    const size_t service_size = service ? service->size : 0;

    const int status = getnameinfo((const struct sockaddr *)&sys_addr, sizeof(sys_addr), host_ptr, host_size, service_ptr, service_size, sb_getnameinfo_to_sys(flags));
    VERIFY_MSG(status == 0, "sb_getnameinfo error: %i", status);
}

void sb_getsockname(const sb_socket_t sock, sb_sockaddr_t * const sockaddr) {
    struct sockaddr_storage storage;
    unsigned int len = sizeof(storage);
    VERIFY(getsockname(sock.socket_id, (struct sockaddr *)&storage, &len) == 0);
    sys_sockaddr_to_adk(sockaddr, (struct sockaddr *)&storage);
}

void sb_getpeername(const sb_socket_t sock, sb_sockaddr_t * const sockaddr) {
    struct sockaddr_storage storage;
    unsigned int len = sizeof(storage);
    VERIFY(getpeername(sock.socket_id, (struct sockaddr *)&storage, &len) == 0);
    sys_sockaddr_to_adk(sockaddr, (struct sockaddr *)&storage);
}

const char * sb_platform_socket_error_str(int err) {
    return gai_strerror(err);
}

unsigned int sb_socket_sys_get_sockaddr_size(const sb_socket_family_e family) {
    unsigned int addrsize = 0;

    switch (family) {
        case sb_socket_family_IPv4:
            addrsize = sizeof(struct sockaddr_in);
            break;

        case sb_socket_family_IPv6:
            addrsize = sizeof(struct sockaddr_in6);
            break;

        default:
            TRAP("unsupported sb_socket_family_e: %i", family);
    }

    return addrsize;
}