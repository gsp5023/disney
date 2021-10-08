/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 socket_tests.c

 socket test fixture
 */

#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET ((uint64_t)-1)
#endif

extern const adk_api_t * api;

enum {
    buffer_size = 255,
    client_max_retries = 10,
    client_sleep_duration = 500,
};

static struct {
    sb_mutex_t * print_mutex;
    const adk_api_t * api;
    sb_mutex_t * tcp_mutex;
    sb_condition_variable_t * tcp_cv;
    uint16_t tcp_server_listening_port;

    // Server socket for TCP and UDP tests
    sb_socket_t server_sock;

    // Connection socket for TCP tests
    sb_socket_t conn_sock;
} statics;

static void dump_addrinfos(const sb_addrinfo_t * const addrinfos, const uint32_t addrinfos_size, const char * const test_case_name, const char * const domain, const char * const port) {
    sb_lock_mutex(statics.print_mutex);
    print_message("[%s] got %i addrinfos for %s:%s\n", test_case_name, addrinfos_size, domain, port);
    for (uint32_t i = 0; i < addrinfos_size; ++i) {
        print_message("[%s] addr_info at index: %i ai_family: %i\n", test_case_name, i, addrinfos[i].ai_family);
        print_message("[%s] addr_info at index: %i ai_protocol: %i\n", test_case_name, i, addrinfos[i].ai_protocol);
        print_message("[%s] addr_info at index: %i ai_socktype: %i\n", test_case_name, i, addrinfos[i].ai_socktype);
        print_message("[%s] addr_info at index: %i ai_ddr::sin_port: %i\n", test_case_name, i, addrinfos[i].ai_addr.sin_port);
        char addr_buff[sb_getnameinfo_max_host];
        mem_region_t address;
        address.size = sizeof(addr_buff);
        address.ptr = addr_buff;
        char serv_buff[sb_getnameinfo_max_service];
        mem_region_t service;
        service.size = sizeof(serv_buff);
        service.ptr = serv_buff;
        sb_getnameinfo(&addrinfos[i].ai_addr, &address, &service, 0);
        print_message("[%s] addr_info at index: %i ai_addr::sin_addr: %s\n", test_case_name, i, addr_buff);
    }
    sb_unlock_mutex(statics.print_mutex);
}

static void stop_server_sock(sb_socket_t * sock, const char * const caller, const char * const sock_name) {
    if (sock->socket_id != INVALID_SOCKET) {
        sb_socket_t tmp_sock = *sock;
        sock->socket_id = INVALID_SOCKET;
        sb_close_socket(tmp_sock);
        print_message("[%s] closed %s socket: [%" PRIu64 "]\n", caller, sock_name, tmp_sock.socket_id);
    }
}

// Closes TCP or UDP server's socket, which should terminate the server thread on the next socket call.
// Used for normal and abnormal termination.
static void stop_test_server(const char * const caller) {
    stop_server_sock(&statics.conn_sock, caller, "connection");
    stop_server_sock(&statics.server_sock, caller, "server");
}

static bool is_little_endian() {
    static const int n = 1;
    return *(char *)&n == 1;
}

static uint16_t bswap_le(const uint16_t num) {
    return is_little_endian() ? __builtin_bswap16(num) : num;
}

static int server_tcp_test(void * ignored) {
    const int backlog = 10;

    sb_addrinfo_t hints = {0};
    hints.ai_family = sb_socket_family_IPv4;
    hints.ai_socktype = sb_socket_type_stream;
    hints.ai_protocol = sb_socket_protocol_tcp;

    statics.server_sock.socket_id = INVALID_SOCKET;

    {
        sb_create_socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol, &statics.server_sock);
        print_message("[tcp server] created socket: [%" PRIu64 "]\n", statics.server_sock.socket_id);

        sb_sockaddr_t addr = {0};
        addr.sin_family = hints.ai_family;

        const sb_socket_bind_result_t bind_err = sb_bind_socket(statics.server_sock, &addr);
        if (bind_err.result == sb_socket_bind_success) {
            print_message("[tcp server] bound socket: [%" PRIu64 "]\n", statics.server_sock.socket_id);
        } else {
            print_message("[tcp server] failed to bind socket: [%" PRIu64 "] error: [%s]\n", statics.server_sock.socket_id, sb_socket_bind_error_str(bind_err.result));
            stop_test_server("tcp server");
            TRAP("[tcp server] failure");
        }

        const sb_socket_listen_result_t listen_err = sb_listen_socket(statics.server_sock, backlog);
        if (listen_err.result == sb_socket_listen_success) {
            sb_getsockname(statics.server_sock, &addr);
            print_message("[tcp server] listening on socket: [%" PRIu64 "] port: [%u] with a backlog of: [%i]\n", statics.server_sock.socket_id, bswap_le(addr.sin_port), backlog);

            sb_lock_mutex(statics.tcp_mutex);
            statics.tcp_server_listening_port = bswap_le(addr.sin_port);
            sb_condition_wake_all(statics.tcp_cv);
            sb_unlock_mutex(statics.tcp_mutex);

            // Check for valid port, as this is a test app
            if (addr.sin_port == 0) {
                print_message("[tcp server] sb_listen_socket() returned zero port number\n");
                stop_test_server("tcp server");
                TRAP("[tcp server] failure");
            }
        } else {
            print_message("[tcp server] failed to listen socket: [%" PRIu64 "] error: [%s]\n", statics.server_sock.socket_id, sb_socket_listen_error_str(listen_err.result));
            stop_test_server("tcp server");
            TRAP("[tcp server] failure");
        }
    }

    char buffer[buffer_size];
    int message_size;

    sb_sockaddr_t peer_addr = {0};

    char addr_buff[sb_getnameinfo_max_host];
    mem_region_t address_mem;
    address_mem.ptr = addr_buff;
    address_mem.size = sizeof(addr_buff);

    char serv_buff[sb_getnameinfo_max_service];
    mem_region_t service_mem;
    service_mem.ptr = serv_buff;
    service_mem.size = sizeof(serv_buff);

    for (;;) {
        mem_region_t buffer_mem_region;
        buffer_mem_region.ptr = buffer;
        buffer_mem_region.size = sizeof(buffer);

        statics.conn_sock.socket_id = 12345; // test: sb_accept_socket() should ignore this

        print_message("[tcp server] accepting connections on socket: [%" PRIu64 "]\n", statics.server_sock.socket_id);
        const sb_socket_accept_result_t accept_err = sb_accept_socket(statics.server_sock, &peer_addr, &statics.conn_sock);
        sb_getnameinfo(&peer_addr, &address_mem, &service_mem, 0);

        if (accept_err.result != sb_socket_accept_success) {
            print_message("[tcp server] accept err: [%s]\n", sb_socket_accept_error_str(accept_err.result));
            if (statics.conn_sock.socket_id != INVALID_SOCKET) {
                print_message("[tcp server] sb_accept_socket() failed with %u but returned valid socket: [%" PRIu64 "] error: [%s]\n", accept_err.result, statics.conn_sock.socket_id, sb_socket_accept_error_str(accept_err.result));
            }
            stop_test_server("tcp server");
            TRAP("[tcp server] failure");
        }
        print_message("[tcp server] accepted a connection from [%s:%s] on socket: [%" PRIu64 "]\n", (char *)address_mem.byte_ptr, (char *)service_mem.byte_ptr, statics.conn_sock.socket_id);
        sb_enable_blocking_socket(statics.conn_sock, sb_socket_blocking_enabled);
        print_message("[tcp server] set blocking on socket: [%" PRIu64 "]\n", statics.conn_sock.socket_id);

        const sb_socket_receive_result_t recv_err = sb_socket_receive(statics.conn_sock, buffer_mem_region, 0, &message_size);
        if (recv_err.result != sb_socket_receive_success) {
            print_message("[tcp server] receive error: [%s]\n", sb_socket_receive_error_str(recv_err.result));
            continue;
        }
        print_message("[tcp server] received: [%i] bytes from socket: [%" PRIu64 "]\n", message_size, statics.conn_sock.socket_id);

        sb_lock_mutex(statics.print_mutex);
        print_message("[tcp server] received: [%s]\n", buffer);
        sb_unlock_mutex(statics.print_mutex);

        const_mem_region_t message;
        message.ptr = buffer;
        message.size = strlen(buffer) + 1;
        int amount_sent;
        const sb_socket_send_result_t send_err = sb_socket_send(statics.conn_sock, message, 0, &amount_sent);
        if (send_err.result == sb_socket_send_success) {
            print_message("[tcp server] sent: [%i] bytes to socket: [%" PRIu64 "]\n", amount_sent, statics.conn_sock.socket_id);
            stop_test_server("tcp server");
            break; // only one echo call for the test case
        }
        print_message("[tcp server] failed to send a message, thus continuing to loop (probably an infinite loop)\n");
        sb_close_socket(statics.conn_sock);
        print_message("[tcp server] closed socket: [%" PRIu64 "]\n", statics.conn_sock.socket_id);
        statics.conn_sock.socket_id = INVALID_SOCKET;
    }
    return 0;
}

static int client_tcp_test(void * ignored) {
    const char * address = "127.0.0.1";

    sb_addrinfo_t hints = {0};
    hints.ai_family = sb_socket_family_IPv4;
    hints.ai_socktype = sb_socket_type_stream;
    hints.ai_protocol = sb_socket_protocol_tcp;

    // Give the tcp server some time to actually startup so when we later attempt to connect we
    // actually have a peer to connect to.  Server not starting in a reasonable time is considered
    // a test failure.
    sb_lock_mutex(statics.tcp_mutex);
    print_message("[tcp client] waiting for TCP server port\n");
    const milliseconds_t timeout = {3 * 1000};
    sb_wait_condition(statics.tcp_cv, statics.tcp_mutex, timeout);
    sb_unlock_mutex(statics.tcp_mutex);
    if (statics.tcp_server_listening_port == 0) {
        TRAP("[tcp client] timed out after %u msecs waiting for server\n", timeout.ms);
    }

    char port[buffer_size];
    sprintf_s(port, buffer_size, "%u", statics.tcp_server_listening_port);

    sb_socket_t sock = {0};
    {
        sb_addrinfo_t addrinfo_array[10] = {0};
        uint32_t addrinfo_active_count = 0;
        sb_getaddrinfo(address, port, &hints, NULL, &addrinfo_active_count);

        if (addrinfo_active_count == 0) {
            TRAP("[tcp client] test failed");
        }

        sb_getaddrinfo(address, port, &hints, addrinfo_array, &addrinfo_active_count);

        dump_addrinfos(addrinfo_array, addrinfo_active_count, "tcp client", address, port);

        // immediately lock and unlock our mutex so we don't write during the addrinfo dumping (not actually needed in a normal program)
        sb_lock_mutex(statics.print_mutex);
        sb_unlock_mutex(statics.print_mutex);

        uint32_t i = 0;

        for (; i < addrinfo_active_count; ++i) {
            const sb_addrinfo_t * const addrinfo = &addrinfo_array[i];
            int platform_error = sb_create_socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol, &sock);
            if (platform_error != 0) {
                print_message("[tcp client] encountered platform error [%s] trying to create a socket\n", sb_platform_socket_error_str(platform_error));
                continue;
            }
            print_message("[tcp client] created socket: [%" PRIu64 "]\n", sock.socket_id);
            sb_socket_connect_result_t connect_err;
            print_message("[tcp client] attempting to connect on socket: [%" PRIu64 "]\n", sock.socket_id);
            bool connected = false;
            for (int count = 0; count < client_max_retries; ++count) {
                connect_err = sb_connect_socket(sock, &addrinfo->ai_addr);
                if (connect_err.result == sb_socket_connect_success) {
                    char addr_buff[sb_getnameinfo_max_host];
                    mem_region_t address_mem;
                    address_mem.ptr = addr_buff;
                    address_mem.size = sizeof(addr_buff);

                    char serv_buff[sb_getnameinfo_max_service];
                    mem_region_t service_mem;
                    service_mem.ptr = serv_buff;
                    service_mem.size = sizeof(serv_buff);

                    sb_getnameinfo(&addrinfo->ai_addr, &address_mem, &service_mem, 0);

                    print_message("[tcp client] connected on socket: [%" PRIu64 "] to server: [%s:%s]\n", sock.socket_id, addr_buff, serv_buff);

                    sb_enable_blocking_socket(sock, sb_socket_blocking_enabled);
                    print_message("[tcp client] enabled blocking on socket: [%" PRIu64 "]\n", sock.socket_id);
                    connected = true;
                    break;
                } else {
                    print_message("[tcp client] max retries remaining: [%i]\n", client_max_retries - count - 1);
                    sb_thread_sleep((milliseconds_t){client_sleep_duration});
                }
            }
            if (connected) {
                break;
            }
            print_message("[tcp client] failed to connect on socket: [%" PRIu64 "]\n", sock.socket_id);
            sb_close_socket(sock);
            print_message("[tcp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
            sock.socket_id = INVALID_SOCKET;
        }

        if (i == addrinfo_active_count) {
            print_message("[tcp client] found no valid address to connect to\n");
            TRAP("[tcp client] test failed");
        }
    }

    const char initial_message[] = "client ping to server";

    const_mem_region_t message;
    message.ptr = initial_message;
    message.size = sizeof(initial_message);
    int message_size;
    const sb_socket_send_result_t send_err = sb_socket_send(sock, message, 0, &message_size);
    if (send_err.result != sb_socket_send_success) {
        print_message("[tcp client] send error: [%s]\n", sb_socket_send_error_str(send_err.result));
        sb_close_socket(sock);
        print_message("[tcp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
        TRAP("[tcp client] test failed\n");
    }
    print_message("[tcp client] sent: [%i] bytes to socket: [%" PRIu64 "]\n", message_size, sock.socket_id);

    char buff[buffer_size];
    mem_region_t buffer;
    buffer.ptr = buff;
    buffer.size = sizeof(buff);

    const sb_socket_receive_result_t recv_err = sb_socket_receive(sock, buffer, 0, &message_size);
    if (recv_err.result != sb_socket_receive_success) {
        print_message("[tcp client] receive error: [%s]\n", sb_socket_receive_error_str(recv_err.result));
        sb_close_socket(sock);
        print_message("[tcp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
        TRAP("[tcp client] test failed\n");
    }
    print_message("[tcp client] received: [%i] bytes from socket: [%" PRIu64 "]\n", message_size, sock.socket_id);

    sb_lock_mutex(statics.print_mutex);
    print_message("[tcp client] received: [%s]\n", buff);
    sb_unlock_mutex(statics.print_mutex);

    sb_close_socket(sock);
    print_message("[tcp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
    return 0;
}

static int server_udp_test(const char * port) {
    sb_addrinfo_t hints;
    hints.ai_family = sb_socket_family_IPv4;
    hints.ai_socktype = sb_socket_type_datagram;
    hints.ai_protocol = sb_socket_protocol_udp;
    hints.ai_flags = sb_getaddrinfo_flag_ai_passive;

    const char * address = NULL;

    statics.server_sock.socket_id = INVALID_SOCKET;

    {
        sb_addrinfo_t addrinfo_array[10] = {0};
        uint32_t addrinfo_active_count = 0;
        sb_getaddrinfo(address, port, &hints, NULL, &addrinfo_active_count);

        if (addrinfo_active_count == 0) {
            TRAP("[udp server] test failed\n");
        }

        sb_getaddrinfo(address, port, &hints, addrinfo_array, &addrinfo_active_count);
        dump_addrinfos(addrinfo_array, addrinfo_active_count, "udp server", address, port);

        // immediately lock and unlock our mutex so we don't write during the addrinfo dumping (not actually needed in a normal program)
        sb_lock_mutex(statics.print_mutex);
        sb_unlock_mutex(statics.print_mutex);

        uint32_t i = 0;
        for (; i < addrinfo_active_count; ++i) {
            const sb_addrinfo_t * const addrinfo = &addrinfo_array[i];
            int platform_error = sb_create_socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol, &statics.server_sock);
            if (platform_error != 0) {
                print_message("[udp server] encountered platform error [%s] trying to create a socket\n", sb_platform_socket_error_str(platform_error));
                continue;
            }
            print_message("[udp server] created socket: [%" PRIu64 "]\n", statics.server_sock.socket_id);
            sb_socket_bind_result_t bind_err = sb_bind_socket(statics.server_sock, &addrinfo->ai_addr);
            if (bind_err.result == sb_socket_bind_success) {
                char addr_buff[sb_getnameinfo_max_host];
                mem_region_t address_mem;
                address_mem.ptr = addr_buff;
                address_mem.size = sizeof(addr_buff);
                char serv_buff[sb_getnameinfo_max_service];
                mem_region_t service_mem;
                service_mem.ptr = serv_buff;
                service_mem.size = sizeof(serv_buff);

                sb_getnameinfo(&addrinfo->ai_addr, &address_mem, &service_mem, 0);

                print_message("[udp server] bound socket: [%" PRIu64 "] to address: [%s:%s] index: [%i]\n", statics.server_sock.socket_id, addr_buff, serv_buff, i);
                break;
            }
            print_message("[udp server] failed to bind on socket: [%" PRIu64 "] error: [%s]\n", statics.server_sock.socket_id, sb_socket_bind_error_str(bind_err.result));
            sb_close_socket(statics.server_sock);
            print_message("[udp server] closed socket: [%" PRIu64 "]\n", statics.server_sock.socket_id);
            statics.server_sock.socket_id = INVALID_SOCKET;
        }

        if (i >= addrinfo_active_count) {
            print_message("[udp server] failed to find a valid socket to bind to\n");
            TRAP("[udp server] test failed\n");
        }
    }

    for (;;) {
        char buff[buffer_size];
        mem_region_t buffer;
        buffer.ptr = buff;
        buffer.size = sizeof(buff);
        sb_sockaddr_t sockaddr;
        int message_size;

        print_message("[udp server] receiving on socket: [%" PRIu64 "]\n", statics.server_sock.socket_id);
        const sb_socket_receive_result_t receive_error = sb_socket_receive_from(statics.server_sock, buffer, 0, &sockaddr, &message_size);

        if (receive_error.result != sb_socket_receive_success) {
            if (receive_error.result != sb_socket_receive_would_block) {
                print_message("[udp server] receive error: [%s] on socket: [%" PRIu64 "]\n", sb_socket_receive_error_str(receive_error.result), statics.server_sock.socket_id);
                stop_test_server("udp server");
                TRAP("[udp server] failure");
            }
            continue;
        }

        char addr_buff[sb_getnameinfo_max_host];
        mem_region_t address_mem;
        address_mem.ptr = addr_buff;
        address_mem.size = sizeof(addr_buff);

        char serv_buff[sb_getnameinfo_max_service];
        mem_region_t service_mem;
        service_mem.ptr = serv_buff;
        service_mem.size = sizeof(serv_buff);

        sb_getnameinfo(&sockaddr, &address_mem, &service_mem, 0);

        print_message("[udp server] received: [%i] bytes from address: [%s:%s]\n", message_size, addr_buff, serv_buff);
        print_message("[udp server] received: [%s]\n", buff);

        const sb_socket_send_result_t send_error = sb_socket_send_to(statics.server_sock, buffer.consted, 0, &sockaddr, &message_size);
        if (send_error.result != sb_socket_send_success) {
            print_message("[udp server] error: [%s] on sending data to [%s:%s]\n", sb_socket_send_error_str(send_error.result), addr_buff, serv_buff);
            stop_test_server("udp server");
            TRAP("[udp server] failure");
        }
        print_message("[udp server] sent: [%i] bytes to address: [%s:%s]\n", message_size, addr_buff, serv_buff);
        break;
    }
    stop_test_server("udp server");
    return 0;
}

static int client_udp_test(const char * port) {
    // give the udp server some time to actually startup so when we later attempt to connect we actually have a peer to connect to.
    sb_thread_sleep((milliseconds_t){2000}); // 2s sleep for rpi.
    sb_addrinfo_t hints;
    hints.ai_family = sb_socket_family_IPv4;
    hints.ai_socktype = sb_socket_type_datagram;
    hints.ai_protocol = sb_socket_protocol_udp;

    const char * address = "127.0.0.1";

    sb_socket_t sock = {0};

    {
        sb_addrinfo_t addrinfo_array[10] = {0};
        uint32_t addrinfo_active_count = 0;
        sb_getaddrinfo(address, port, &hints, NULL, &addrinfo_active_count);

        if (addrinfo_active_count == 0) {
            TRAP("[udp client] test failed");
        }

        sb_getaddrinfo(address, port, &hints, addrinfo_array, &addrinfo_active_count);

        dump_addrinfos(addrinfo_array, addrinfo_active_count, "udp client", address, port);

        // immediately lock and unlock our mutex so we don't write during the addrinfo dumping (not actually needed in a normal program)
        sb_lock_mutex(statics.print_mutex);
        sb_unlock_mutex(statics.print_mutex);

        uint32_t i = 0;

        for (; i < addrinfo_active_count; ++i) {
            const sb_addrinfo_t * const addrinfo = &addrinfo_array[i];
            int error = sb_create_socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol, &sock);
            if (error != 0) {
                print_message("[udp client] encountered platform error [%s] trying to create a socket\n", sb_platform_socket_error_str(error));
                continue;
            }
            print_message("[udp client] created socket: [%" PRIu64 "]\n", sock.socket_id);
            print_message("[udp client] attempting to connect on socket: [%" PRIu64 "]\n", sock.socket_id);
            sb_socket_connect_result_t connect_error;
            bool connected = false;

            for (int count = 0; count < client_max_retries; ++count) {
                connect_error = sb_connect_socket(sock, &addrinfo->ai_addr);
                if (connect_error.result == sb_socket_connect_success) {
                    char addr_buff[sb_getnameinfo_max_host];

                    mem_region_t address_mem;
                    address_mem.ptr = addr_buff;
                    address_mem.size = sizeof(addr_buff);
                    char serv_buff[sb_getnameinfo_max_service];

                    mem_region_t service_mem;
                    service_mem.ptr = serv_buff;
                    service_mem.size = sizeof(serv_buff);

                    sb_getnameinfo(&addrinfo->ai_addr, &address_mem, &service_mem, 0);

                    print_message("[udp client] connect socket: [%" PRIu64 "] to address: [%s:%s] index: [%i]\n", sock.socket_id, addr_buff, serv_buff, i);
                    connected = true;
                    break;
                } else {
                    print_message("[udp client] max retries remaining: [%i]\n", client_max_retries - count - 1);
                    sb_thread_sleep((milliseconds_t){client_sleep_duration});
                }
            }
            if (connected) {
                break;
            }
            print_message("[udp client] failed to connect on socket: [%" PRIu64 "] error: [%s]\n", sock.socket_id, sb_socket_connect_error_str(connect_error.result));
            sb_close_socket(sock);
            print_message("[udp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
            sock.socket_id = INVALID_SOCKET;
        }

        if (i == addrinfo_active_count) {
            print_message("[udp client] failed to find a valid socket to connect to\n");
            TRAP("[udp client] test failed");
        }
    }

    const char initial_message[] = "client ping to server";
    const_mem_region_t message;
    message.size = sizeof(initial_message);
    message.ptr = initial_message;

    int bytes_sent;
    const sb_socket_send_result_t send_error = sb_socket_send(sock, message, 0, &bytes_sent);
    if (send_error.result != sb_socket_send_success) {
        print_message("[udp client] send error: [%s] on socket: [%" PRIu64 "]\n", sb_socket_send_error_str(send_error.result), sock.socket_id);
        sb_close_socket(sock);
        print_message("[udp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
        TRAP("[udp client] test failed\n");
    }
    print_message("[udp client] sent: [%i] bytes on socket: [%" PRIu64 "]\n", bytes_sent, sock.socket_id);

    char buff[buffer_size];
    mem_region_t buffer;
    buffer.size = sizeof(buff);
    buffer.ptr = buff;
    int amount_received;
    const sb_socket_receive_result_t receive_error = sb_socket_receive(sock, buffer, 0, &amount_received);
    if (receive_error.result != sb_socket_receive_success) {
        print_message("[udp client] receive error: [%s] on socket: [%" PRIu64 "]\n", sb_socket_receive_error_str(receive_error.result), sock.socket_id);
        sb_close_socket(sock);
        print_message("[udp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
        TRAP("[udp client] test failed\n");
    }

    print_message("[udp client] received: [%i] bytes from socket: [%" PRIu64 "]\n", amount_received, sock.socket_id);
    print_message("[udp client] received: [%s]\n", buff);

    sb_close_socket(sock);
    print_message("[udp client] closed socket: [%" PRIu64 "]\n", sock.socket_id);
    return 0;
}

static void socket_unit_test(void ** ignored) {
    statics.print_mutex = sb_create_mutex(MALLOC_TAG);
    statics.tcp_mutex = sb_create_mutex(MALLOC_TAG);
    statics.tcp_cv = sb_create_condition_variable(MALLOC_TAG);
    statics.server_sock.socket_id = INVALID_SOCKET;
    statics.conn_sock.socket_id = INVALID_SOCKET;

    sb_thread_id_t server_thread = NULL;
    sb_thread_id_t client_thread = NULL;

    server_thread = sb_create_thread("sck_srv_tst", sb_thread_default_options, (int (*)(void *))server_tcp_test, NULL, MALLOC_TAG);
    if (server_thread) {
        client_thread = sb_create_thread("sck_clnt_tst", sb_thread_default_options, (int (*)(void *))client_tcp_test, NULL, MALLOC_TAG);
        if (client_thread) {
            sb_join_thread(client_thread);
            sb_thread_sleep((milliseconds_t){1000}); // give server some time to close normally
        } else {
            print_message("[socket test] failed to create client_tcp_test thread\n");
        }
        stop_test_server("socket test");
        sb_join_thread(server_thread);
    } else {
        print_message("[socket test] failed to create server_tcp_test thread\n");
    }

    const char * const udp_port = "8000";
    server_thread = sb_create_thread("sck_srv_tst", sb_thread_default_options, (int (*)(void *))server_udp_test, (void *)udp_port, MALLOC_TAG);
    if (server_thread) {
        client_thread = sb_create_thread("sck_clnt_tst", sb_thread_default_options, (int (*)(void *))client_udp_test, (void *)udp_port, MALLOC_TAG);
        if (client_thread) {
            sb_join_thread(client_thread);
            sb_thread_sleep((milliseconds_t){1000}); // give server some time to close normally
        } else {
            print_message("[socket test] failed to create client_udp_test thread\n");
        }
        stop_test_server("socket test");
        sb_join_thread(server_thread);
    } else {
        print_message("[socket test] failed to create server_udp_test thread\n");
    }

    sb_destroy_mutex(statics.tcp_mutex, MALLOC_TAG);
    sb_destroy_condition_variable(statics.tcp_cv, MALLOC_TAG);

    sb_destroy_mutex(statics.print_mutex, MALLOC_TAG);
}

int test_socket() {
    statics.api = api;
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(socket_unit_test, NULL, NULL)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
