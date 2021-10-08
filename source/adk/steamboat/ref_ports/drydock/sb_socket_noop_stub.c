/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "source/adk/steamboat/ref_ports/drydock/impl_tracking.h"
#include "source/adk/steamboat/sb_socket.h"

static const char simulated_error_string[] = "not implemented";

void sb_socket_dump_heap_usage(void) {
    NOT_IMPLEMENTED_EX;
}

sb_getaddrinfo_result_t sb_getaddrinfo(
    const char * const address,
    const char * const port,
    const sb_addrinfo_t * const hint,
    sb_addrinfo_t * const out_addrinfos,
    uint32_t * const out_addrinfo_size) {
    sb_getaddrinfo_result_t r = {0};

    UNUSED(address);
    UNUSED(port);
    UNUSED(hint);
    UNUSED(out_addrinfos);
    UNUSED(out_addrinfo_size);

    NOT_IMPLEMENTED_EX;

    return r;
}

int sb_create_socket(
    const sb_socket_family_e domain,
    const sb_socket_type_e sock_type,
    const sb_socket_protocol_type_e protocol,
    sb_socket_t * const out_socket) {
    UNUSED(domain);
    UNUSED(sock_type);
    UNUSED(protocol);
    UNUSED(out_socket);

    NOT_IMPLEMENTED_EX;

    return 0;
}

bool sb_enable_blocking_socket(
    const sb_socket_t sock,
    const sb_socket_blocking_e blocking_mode) {
    UNUSED(sock);
    UNUSED(blocking_mode);

    NOT_IMPLEMENTED_EX;

    return false;
}

bool sb_socket_set_tcp_no_delay(
    const sb_socket_t sock,
    const sb_socket_tcp_delay_mode_e tcp_delay_mode) {
    UNUSED(sock);
    UNUSED(tcp_delay_mode);

    NOT_IMPLEMENTED_EX;

    return false;
}

bool sb_socket_set_linger(
    const sb_socket_t sock,
    const sb_socket_linger_mode_e linger_mode,
    const int linger_timeout_in_seconds) {
    UNUSED(sock);
    UNUSED(linger_mode);
    UNUSED(linger_timeout_in_seconds);

    NOT_IMPLEMENTED_EX;

    return false;
}

sb_socket_bind_result_t sb_bind_socket(
    const sb_socket_t sock,
    const sb_sockaddr_t * const addr) {
    sb_socket_bind_result_t r = {0};

    UNUSED(sock);
    UNUSED(addr);

    NOT_IMPLEMENTED_EX;

    return r;
}

sb_socket_listen_result_t sb_listen_socket(
    const sb_socket_t sock,
    const int backlog) {
    sb_socket_listen_result_t r = {0};

    UNUSED(sock);
    UNUSED(backlog);

    NOT_IMPLEMENTED_EX;

    return r;
}

sb_socket_accept_result_t sb_accept_socket(
    const sb_socket_t sock,
    sb_sockaddr_t * const optional_sockaddr,
    sb_socket_t * const out_sock) {
    sb_socket_accept_result_t r = {0};

    UNUSED(sock);
    UNUSED(optional_sockaddr);
    UNUSED(out_sock);

    NOT_IMPLEMENTED_EX;

    return r;
}

sb_socket_connect_result_t sb_connect_socket(
    const sb_socket_t sock,
    const sb_sockaddr_t * const addr) {
    sb_socket_connect_result_t r = {0};

    UNUSED(sock);
    UNUSED(addr);

    NOT_IMPLEMENTED_EX;

    return r;
}

void sb_close_socket(const sb_socket_t sock) {
    UNUSED(sock);

    NOT_IMPLEMENTED_EX;
}

void sb_shutdown_socket(
    const sb_socket_t sock,
    const sb_socket_shutdown_flags_e shutdown_flags) {
    UNUSED(sock);
    UNUSED(shutdown_flags);

    NOT_IMPLEMENTED_EX;
}

sb_socket_select_result_e sb_socket_select(
    const sb_socket_t socket,
    milliseconds_t * const timeout) {
    UNUSED(socket);
    UNUSED(timeout);

    NOT_IMPLEMENTED_EX;

    return -1;
}

sb_socket_receive_result_t sb_socket_receive(
    const sb_socket_t sock,
    const mem_region_t buffer,
    const sb_socket_receive_flags_e flags,
    int * const receive_size) {
    sb_socket_receive_result_t r = {0};

    UNUSED(sock);
    UNUSED(buffer);
    UNUSED(flags);
    UNUSED(receive_size);

    NOT_IMPLEMENTED_EX;

    return r;
}

sb_socket_receive_result_t sb_socket_receive_from(
    const sb_socket_t sock,
    const mem_region_t buffer,
    const sb_socket_receive_flags_e flags,
    sb_sockaddr_t * const out_addr,
    int * const receive_size) {
    sb_socket_receive_result_t r = {0};

    UNUSED(sock);
    UNUSED(buffer);
    UNUSED(flags);
    UNUSED(out_addr);
    UNUSED(receive_size);

    NOT_IMPLEMENTED_EX;

    return r;
}

sb_socket_send_result_t sb_socket_send(
    const sb_socket_t sock,
    const const_mem_region_t message,
    const sb_socket_send_flags_e flags,
    int * const bytes_sent) {
    sb_socket_send_result_t r = {0};

    UNUSED(sock);
    UNUSED(message);
    UNUSED(flags);
    UNUSED(bytes_sent);

    NOT_IMPLEMENTED_EX;

    return r;
}

sb_socket_send_result_t sb_socket_send_to(
    const sb_socket_t sock,
    const const_mem_region_t message,
    const sb_socket_send_flags_e flags,
    const sb_sockaddr_t * const addr,
    int * const bytes_sent) {
    sb_socket_send_result_t r = {0};

    UNUSED(sock);
    UNUSED(message);
    UNUSED(flags);
    UNUSED(addr);
    UNUSED(bytes_sent);

    NOT_IMPLEMENTED_EX;

    return r;
}

void sb_getnameinfo(
    const sb_sockaddr_t * const addr,
    mem_region_t * const host,
    mem_region_t * const service,
    const sb_getnameinfo_flags_e flags) {
    UNUSED(addr);
    UNUSED(host);
    UNUSED(service);
    UNUSED(flags);

    NOT_IMPLEMENTED_EX;
}

void sb_getsockname(
    const sb_socket_t sock,
    sb_sockaddr_t * const sockaddr) {
    UNUSED(sock);
    UNUSED(sockaddr);

    NOT_IMPLEMENTED_EX;
}

void sb_getpeername(
    const sb_socket_t sock,
    sb_sockaddr_t * const sockaddr) {
    UNUSED(sock);
    UNUSED(sockaddr);

    NOT_IMPLEMENTED_EX;
}

const char * sb_getaddrinfo_error_str(const sb_getaddrinfo_error_code_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_socket_connect_error_str(const sb_socket_connect_error_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_socket_accept_error_str(const sb_socket_accept_error_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_socket_bind_error_str(const sb_socket_bind_error_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_socket_listen_error_str(const sb_socket_listen_error_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_socket_receive_error_str(const sb_socket_receive_error_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_socket_send_error_str(const sb_socket_send_error_e err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}

const char * sb_platform_socket_error_str(int err) {
    UNUSED(err);

    NOT_IMPLEMENTED_EX;

    return simulated_error_string;
}
