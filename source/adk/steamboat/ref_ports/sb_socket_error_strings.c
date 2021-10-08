/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_socket_error_strings.c

human readable/safe to print error strings for socket error categories
*/

#include _PCH

#include "source/adk/steamboat/sb_socket.h"

static const char * const success_str = "[success]: Success.";
static const char * const system_str = "[system]: Other system error, error code is platform specific error.";

const char * sb_getaddrinfo_error_str(const sb_getaddrinfo_error_code_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_getaddrinfo_success:
            str = success_str;
            break;
        case sb_getaddrinfo_address_family:
            str = "[address_family]: Host does not have any network addresses in the requested family.";
            break;
        case sb_getaddrinfo_again:
            str = "[again]: Name server returned temporary failure, try again later.";
            break;
        case sb_getaddrinfo_service:
            str = "[service]: The requested service is not available for the requested socket type.";
            break;
        default:
            str = system_str;
    }
    return str;
}

const char * sb_socket_connect_error_str(const sb_socket_connect_error_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_socket_connect_success:
            str = success_str;
            break;
        case sb_socket_connect_access:
            str = "[access]: Request failed due to local firewall rule or user tried to connect to a broadcast address without having the socket broadcast flag enabled.";
            break;
        case sb_socket_connect_address_in_use:
            str = "[address_in_use]: Local address already in use.";
            break;
        case sb_socket_connect_address_not_avail:
            str = "[address_not_avail]: The socket had not previously been bound to an adress, upon attempting to bind to an ephermal port it was determined that all ephermal port numbers are in use.";
            break;
        case sb_socket_connect_no_support:
            str = "[no_support]: The passed address has an invalid address family for sin_family.";
            break;
        case sb_socket_connect_would_block:
            str = "[would_block]: For non blocking the connection cannot be completed immediately.";
            break;
        case sb_socket_connect_already:
            str = "[already]: The socket is nonblocking and the previous connection attempt has not been completed.";
            break;
        case sb_socket_connect_connection_refused:
            str = "[connection_refused]: Found no one listening on the remote address.";
            break;
        case sb_socket_connect_in_progress:
            str = "[in_process]: The socket is nonblocking and the connection cannot be completed immediately.";
            break;
        case sb_socket_connect_network_unreachable:
            str = "[network_unreachable]: Network is unreachable.";
            break;
        case sb_socket_connect_timed_out:
            str = "[timed_out]: timeout while attempting connection.";
            break;
        default:
            str = system_str;
    }
    return str;
}

const char * sb_socket_accept_error_str(const sb_socket_accept_error_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_socket_accept_success:
            str = success_str;
            break;
        case sb_socket_accept_would_block:
            str = "[would_block]: The socket is marked nonblocking and no connections are present to be accepted.";
            break;
        case sb_socket_accept_connection_aborted:
            str = "[connection_aborted]: Connection has been aborted.";
            break;
        case sb_socket_accept_invalid:
            str = "[invalid]: The socket is not listening for connections.";
            break;
        default:
            str = system_str;
    }
    return str;
}

const char * sb_socket_bind_error_str(const sb_socket_bind_error_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_socket_bind_success:
            str = success_str;
            break;
        case sb_socket_bind_access:
            str = "[access]: The address is protected.";
            break;
        case sb_socket_bind_in_use:
            str = "[in_use]: The address is already in use.";
            break;
        default:
            str = system_str;
    }
    return str;
}

const char * sb_socket_listen_error_str(const sb_socket_listen_error_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_socket_listen_success:
            str = success_str;
            break;
        case sb_socket_listen_address_in_use:
            str = "[address_in_use]: Another socket is already listening on the same port.";
            break;
        case sb_socket_listen_not_supported:
            str = "[not_supported]: The socket is not of a type that supports listen().";
            break;
        default:
            str = system_str;
    }
    return str;
}

const char * sb_socket_receive_error_str(const sb_socket_receive_error_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_socket_receive_success:
            str = success_str;
            break;
        case sb_socket_receive_would_block:
            str = "[would_block]: The socket is marked nonblocking and the receive operation would block, or receive timeout has been set and the timeout has expired before the data was received.";
            break;
        case sb_socket_receive_connection_refused:
            str = "[connection_refused]: The remote host refused the connection (service is most likely not running).";
            break;
        case sb_socket_receive_not_connected:
            str = "[not_connected]: The socket is associated with a connection-oriented protocol and has not been connected.";
            break;
        default:
            str = system_str;
    }
    return str;
}

const char * sb_socket_send_error_str(const sb_socket_send_error_e err) {
    const char * str = NULL;
    switch (err) {
        case sb_socket_send_success:
            str = success_str;
            break;
        case sb_socket_send_access:
            str = "[access]: An attempt was made to send to a network/broadcast address as though it was a unicast address.";
            break;
        case sb_socket_send_would_block:
            str = "[would_block]: The socket is marked nonblocking and requested operation would block.";
            break;
        case sb_socket_send_connection_reset:
            str = "[connection_reset]: Connection reset by peer.";
            break;
        case sb_socket_send_message_size:
            str = "[message_size]: The socket type required the message to be sent atomically, but the size of the message made it impossible.";
            break;
        case sb_socket_send_no_buffers:
            str = "[no_buffers]: The output queue for the network is full.";
            break;
        case sb_socket_send_not_connected:
            str = "[not_connected]: The socket is not connected, and no target was specified.";
            break;
        case sb_socket_send_shutdown:
            str = "[shutdown]: The local end has been shut down on a connection oriented socket.";
            break;
        default:
            str = system_str;
    }
    return str;
}