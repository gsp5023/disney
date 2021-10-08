/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
socket.h

steamboat socket support
*/

#pragma once

#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Constant Integers
enum {
    /// Maximum length of host name from `getnameinfo`
    sb_getnameinfo_max_host = 1025,
    /// Maximum length of service name from `getnameinfo`
    sb_getnameinfo_max_service = 32,

    /// Code for a generic failure used to populate the following `...__error_base_offset` constants
    __sb_generic_system_failure = -0xFEED,
    /// Base offset used to populate the enum `sb_getaddrinfo_error_code_e` within a specific numeric range
    __sb_getaddrinfo_error_base_offset = __sb_generic_system_failure - 100,
    /// Base offset used to populate the enum `sb_socket_connect_error_e` within a specific numeric range
    __sb_socket_connect_error_base_offset = __sb_generic_system_failure - 200,
    /// Base offset used to populate the enum `sb_socket_accept_error_e` within a specific numeric range
    __sb_socket_accept_error_base_offset = __sb_generic_system_failure - 300,
    /// Base offset used to populate the enum `sb_socket_bind_error_e` within a specific numeric range
    __sb_socket_bind_error_base_offset = __sb_generic_system_failure - 400,
    /// Base offset used to populate the enum `sb_socket_listen_error_e` within a specific numeric range
    __sb_socket_listen_error_base_offset = __sb_generic_system_failure - 500,
    /// Base offset used to populate the enum `sb_socket_receive_error_e` within a specific numeric range
    __sb_socket_receive_error_base_offset = __sb_generic_system_failure - 600,
    /// Base offset used to populate the enum `sb_socket_send_error_e` within a specific numeric range
    __sb_socket_send_error_base_offset = __sb_generic_system_failure - 700,
};

/// Socket family
typedef enum sb_socket_family_e {
    /// Use the Internet Protocol version 4
    sb_socket_family_IPv4 = 4,
    /// Use the Internet Protocol version 6
    sb_socket_family_IPv6 = 6,
    /// No preference in what OSI network layer protocol is used
    sb_socket_family_dont_care = 0,
    /// A OSI network layer protocol not supported by the system
    sb_socket_family_unsupported = -1,
} sb_socket_family_e;

/// Socket type
typedef enum sb_socket_type_e {
    /// No socket type specified
    sb_socket_type_any = 0,
    /// A connection-based, stream type socket
    sb_socket_type_stream = 1,
    /// A connectionless datagram socket
    sb_socket_type_datagram = 2,
    /// A socket type not supported by the system
    sb_socket_type_unsupported = -1,
} sb_socket_type_e;

/// Socket protocol
typedef enum sb_socket_protocol_type_e {
    /// No socket protocol specified
    sb_socket_protocol_any = 0,
    /// User Datagram Protocol (UDP)
    sb_socket_protocol_udp = 1,
    /// Transmission Control Protocol (TCP)
    sb_socket_protocol_tcp = 2,
    /// A socket protocol not supported by the system
    sb_socket_protocol_unsupported = -1,
} sb_socket_protocol_type_e;

/// Bitmask options for configuring a socket *send* action
typedef enum sb_socket_send_flags_e {
    /// No socket send flags set
    sb_socket_send_flag_none = 0x0,
    /// Requests the to socket send its packets directly to their destination (i.e., bypass the routing table).
    sb_socket_send_flag_dont_route = 0x1,
    /// Requests the to socket send its packets out-of-band.  Out-of-band packets are delivered with a higher priority than regular data.  The sb_socket_type_e and sb_socket_protocol_type_e must support out-of-band for this to work.
    sb_socket_send_flag_out_of_band = 0x2,
    /// Request that a "broken pipe" signal is not sent when writing to a pipe whose read end has closed.  NOTE: Ignored on windows
    sb_socket_send_flag_no_sig_pipe = 0x4,
} sb_socket_send_flags_e;

/// Bitmask options for configuring a socket *receive* action
typedef enum sb_socket_receive_flags_e {
    /// No socket receive flags set
    sb_socket_receive_flag_none = 0x0,
    /// Return the received data without marking it as read (i.e., same data will still be returned from the next call to `sb_socket_receive(...)` or any other receive function)
    sb_socket_receive_flag_peek = 0x1,
    /// Requests data send as out-of-band
    sb_socket_receive_flag_out_of_band = 0x2,
    /// Requests that a receive call, on a `sb_socket_type_stream` socket, block until the full amount of data is available.
    sb_socket_receive_flag_wait_all = 0x4,
    /// Request that a "broken pipe" signal is not sent when sending side unexpectedly closes.  NOTE: Ignored on windows
    sb_socket_receive_flag_no_sig_pipe = 0x8,
} sb_socket_receive_flags_e;

/// Options for configuring a *getaddrinfo* action
typedef enum sb_getaddrinfo_flags_e {
    /// The socket is suitable for connecting to and sent data
    sb_getaddrinfo_flag_none = 0,
    /// The socket is suitable for binding and accepting connections
    sb_getaddrinfo_flag_ai_passive = 1,
} sb_getaddrinfo_flags_e;

/// Bitmask options for configuring a *getnameinfo* action
typedef enum sb_getnameinfo_flags_e {
    /// No getnameinfo flags set
    sb_getname_info_flag_none = 0x0,
    /// Request the service address in its numeric form
    sb_getnameinfo_numeric_service = 0x1,
    /// Request the hostname in its numeric form
    sb_getnameinfo_numeric_host = 0x2,
} sb_getnameinfo_flags_e;

/// Socket blocking mode
typedef enum sb_socket_blocking_e {
    /// A flag used to set a socket to blocking. All calls (send, receive, ...) to to a blocking socket will not be returned until all relevant data is available.
    sb_socket_blocking_enabled = 2,
    /// A flag used to set a socket to non-blocking. All calls (send, receive, ...) to a non-blocking socket will return immediately with whatever is data currently available.
    sb_socket_blocking_disabled = 3
} sb_socket_blocking_e;

/// Enable or disable tcp_nodelay (disable/enable Nagle's coalescing)
typedef enum sb_socket_tcp_delay_mode_e {
    /// disable Nagle algorithm (don't wait for acks to send data).
    sb_socket_tcp_delay_none = 2,
    /// use Nagle algorithm for send coalescing (wait for acks, and aggregate send inside the OS)
    sb_socket_tcp_delay_nagle = 3,
} sb_socket_tcp_delay_mode_e;

/// Set the socket's linger mode
typedef enum sb_socket_linger_mode_e {
    /// A flag used to enable linger mode on a socket.  Close or shutdown calls will not return until all queued messages are handled.
    sb_socket_linger = 2,
    /// A flag used to dosable linger mode on a socket. Close or shutdown calls will return immediately.
    sb_socket_no_linger = 3,
} sb_socket_linger_mode_e;

/// What to forcibly close when shutting down a socket
typedef enum sb_socket_shutdown_flags_e {
    /// No socket shutdown flags set
    sb_socket_shutdown_flag_none = 0x0,
    /// Shut down the ability to read from a socket
    sb_socket_shutdown_read = 0x1,
    /// Shut down the ability to write to a socket
    sb_socket_shutdown_write = 0x2,
    /// Shut down the ability to both read from and write to a socket
    sb_socket_shutdown_read_write = sb_socket_shutdown_read | sb_socket_shutdown_write,
} sb_socket_shutdown_flags_e;

/// Bitmask options for what operations are available on the sockets when select completes
typedef enum sb_socket_select_result_e {
    /// This is an empty mask
    sb_socket_select_flag_none = 0x0,
    /// Readable sockets are available from the seek
    sb_socket_select_flag_readable = 0x1,
    /// Writable sockets are available from the seek
    sb_socket_select_flag_writable = 0x2,
    /// Sockets with "exceptional conditions" are available from the seek
    sb_socket_select_flag_exception = 0x4,
} sb_socket_select_result_e;

/// Error classification for getting the address info of a socket
typedef enum sb_getaddrinfo_error_code_e {
    /// no error
    sb_getaddrinfo_success = 0,
    /// Host does not have any network addresses in the requested family
    sb_getaddrinfo_address_family = __sb_getaddrinfo_error_base_offset - 1,
    /// Name server returned temporary failure, try again later.
    sb_getaddrinfo_again = __sb_getaddrinfo_error_base_offset - 2,
    /// The requested service is not available for the requested socket type
    sb_getaddrinfo_service = __sb_getaddrinfo_error_base_offset - 3,
} sb_getaddrinfo_error_code_e;

/// Result of getting the address info of a socket (including a potential error)
typedef struct sb_getaddrinfo_result_t {
    /// The translated results
    sb_getaddrinfo_error_code_e result;
    /// The system specific return code from the underlying address info request
    int system_code;
} sb_getaddrinfo_result_t;

/// Error classification for connecting on a socket
typedef enum sb_socket_connect_error_e {
    /// Successful connection
    sb_socket_connect_success = 0,
    /// request failed due to local firewall rule or user tried to connect to a broadcast address without having the socket broadcast flag enabled
    sb_socket_connect_access = __sb_socket_connect_error_base_offset - 1,
    /// local address already in use
    sb_socket_connect_address_in_use = __sb_socket_connect_error_base_offset - 2,
    /// the socket had not previously been bound to an address, upon attempting to bind to an ephemeral port it was determined that all ephemeral port numbers are in use
    sb_socket_connect_address_not_avail = __sb_socket_connect_error_base_offset - 3,
    /// passed address has an invalid address family for sin_family
    sb_socket_connect_no_support = __sb_socket_connect_error_base_offset - 4,
    /// for non blocking the connection cannot be completed immediately
    sb_socket_connect_would_block = __sb_socket_connect_error_base_offset - 5,
    /// the socket is nonblocking and the previous connection attempt has not been completed.
    sb_socket_connect_already = __sb_socket_connect_error_base_offset - 6,
    /// a connect() found no one listening on the remote address.
    sb_socket_connect_connection_refused = __sb_socket_connect_error_base_offset - 7,
    /// the socket is nonblocking and the connection cannot be completed immediately
    sb_socket_connect_in_progress = __sb_socket_connect_error_base_offset - 8,
    /// network is unreachable
    sb_socket_connect_network_unreachable = __sb_socket_connect_error_base_offset - 9,
    /// timeout while attempting connection
    sb_socket_connect_timed_out = __sb_socket_connect_error_base_offset - 10,
    /// the socket is already connected (connection-oriented sockets only)
    sb_socket_connect_is_connected = __sb_socket_connect_error_base_offset - 11,
} sb_socket_connect_error_e;

/// Result of a socket *connect* (including a potential error)
typedef struct sb_socket_connect_result_t {
    /// The translated results
    sb_socket_connect_error_e result;
    /// The system specific return code from the underlying connection request
    int system_code;
} sb_socket_connect_result_t;

/// Error classification for accepting on a socket
typedef enum sb_socket_accept_error_e {
    /// Successful acceptance
    sb_socket_accept_success = 0,
    /// the socket is marked nonblocking and no connections are present to be accepted
    sb_socket_accept_would_block = __sb_socket_accept_error_base_offset - 1,
    /// connection has been aborted
    sb_socket_accept_connection_aborted = __sb_socket_accept_error_base_offset - 2,
    /// socket is not listening for connections
    sb_socket_accept_invalid = __sb_socket_accept_error_base_offset - 3,
} sb_socket_accept_error_e;

/// Result of a socket *accept* (including a potential error)
typedef struct sb_socket_accept_result_t {
    /// The translated results
    sb_socket_accept_error_e result;
    /// The system specific return code from the accept request
    int system_code;
} sb_socket_accept_result_t;

/// Error classification for binding on a socket
typedef enum sb_socket_bind_error_e {
    /// Successful binding
    sb_socket_bind_success = 0,
    /// address is protected
    sb_socket_bind_access = __sb_socket_bind_error_base_offset - 1,
    /// address is already in use
    sb_socket_bind_in_use = __sb_socket_bind_error_base_offset - 2,
} sb_socket_bind_error_e;

/// Result of a socket *bind* (including a potential error)
typedef struct sb_socket_bind_result_t {
    /// The translated results
    sb_socket_bind_error_e result;
    /// The system specific return code from the bind request
    int system_code;
} sb_socket_bind_result_t;

/// Error classification for listening on a socket
typedef enum sb_socket_listen_error_e {
    /// Successful listen request
    sb_socket_listen_success = 0,
    /// another socket is already listening on the same port
    sb_socket_listen_address_in_use = __sb_socket_listen_error_base_offset - 1,
    /// socket is not of a type that supports listen()
    sb_socket_listen_not_supported = __sb_socket_listen_error_base_offset - 2,
} sb_socket_listen_error_e;

/// Result of a socket *listen* (including a potential error)
typedef struct sb_socket_listen_result_t {
    /// The translated results
    sb_socket_listen_error_e result;
    /// The system specific return code from the listen request
    int system_code;
} sb_socket_listen_result_t;

/// Error classification for receiving on a socket
typedef enum sb_socket_receive_error_e {
    /// Data was successfully received from the socket
    sb_socket_receive_success = 0,
    /// the socket is marked nonblocking and the receive operation would block, or receive timeout has been set and the timeout has expired before the data was received.
    sb_socket_receive_would_block = __sb_socket_receive_error_base_offset - 1,
    /// remote host refused the connection (service is most likely not running)
    sb_socket_receive_connection_refused = __sb_socket_receive_error_base_offset - 2,
    /// the socket is associated with a connection-oriented protocol and has not been connected.
    sb_socket_receive_not_connected = __sb_socket_receive_error_base_offset - 3,
} sb_socket_receive_error_e;

/// Result of a socket *receive* (including a potential error)
typedef struct sb_socket_receive_result_t {
    /// The translated results
    sb_socket_receive_error_e result;
    /// The system specific return code from the receive request
    int system_code;
} sb_socket_receive_result_t;

/// Error classification for sending on a socket
typedef enum sb_socket_send_error_e {
    /// The data was sent successfully
    sb_socket_send_success = 0,
    /// An attempt was made to send to a network/broadcast address as though it was a unicast address
    sb_socket_send_access = __sb_socket_send_error_base_offset - 1,
    /// The socket is marked nonblocking and requested operation would block.
    sb_socket_send_would_block = __sb_socket_send_error_base_offset - 2,
    /// Connection reset by peer
    sb_socket_send_connection_reset = __sb_socket_send_error_base_offset - 3,
    /// The socket type required the message to be sent atomically, but the size of the message made it impossible
    sb_socket_send_message_size = __sb_socket_send_error_base_offset - 4,
    /// The output queue for the network is full
    sb_socket_send_no_buffers = __sb_socket_send_error_base_offset - 5,
    /// The socket is not connected, and no target was specified
    sb_socket_send_not_connected = __sb_socket_send_error_base_offset - 6,
    /// The local end has been shut down on a connection oriented socket.
    sb_socket_send_shutdown = __sb_socket_send_error_base_offset - 7,
} sb_socket_send_error_e;

/// Result of a socket *send* (including a potential error)
typedef struct sb_socket_send_result_t {
    /// The translated results
    sb_socket_send_error_e result;
    /// The system specific return code from the send request
    int system_code;
} sb_socket_send_result_t;

/// A socket represented by an ID
typedef struct sb_socket_t {
    /// The ID number of the socket
    uint64_t socket_id;
} sb_socket_t;

/// Socket address (IPv4/6)
typedef struct sb_sockaddr_t {
    /// address family (defined by the OSI network layer protocol e.g., IPv4)
    sb_socket_family_e sin_family;
    /// Port number
    uint16_t sin_port;
    /// Where the actuall address is stored
    union {
        /// The IPv4 Address as an array of bytes
        uint8_t ipv4_u8[4];
        /// The IPv4 Address as an unsigned 32-bit integer
        uint32_t ipv4_u32;
        /// The IPv6 Address as an array of bytes
        uint8_t ipv6_u8[16];
        /// The IPv6 Address as an array of unsigned 16-bit integers
        uint16_t ipv6_u16[8];
        /// The IPv6 Address as an array of unsigned 32-bit integers
        uint32_t ipv6_u32[4];
    } sin_addr;
    /// IPv6 Flow Label
    uint32_t ipv6_flowinfo;
    /// Scope (aka Zone) ID
    uint32_t ipv6_scope_id;
} sb_sockaddr_t;

/// An internet address that can be used to bind/connect
typedef struct sb_addrinfo_t {
    /// Active or passive flags
    sb_getaddrinfo_flags_e ai_flags;
    /// OSI network layer protocol
    sb_socket_family_e ai_family;
    /// Connected stream or connectionless datagram
    sb_socket_type_e ai_socktype;
    /// OSI transport layer protocol
    sb_socket_protocol_type_e ai_protocol;
    /// The socket's address
    sb_sockaddr_t ai_addr;
} sb_addrinfo_t;

/// Returns one or more Internet addresses in `out_addrinfos` for the given `address` and `port`
///
/// * `address`: The internet address of the desired address structure
/// * `port`: The number of the desired address structure
/// * `hint`: can be either NULL or an sb_addrinfo_t structure for the type of service requested
/// * `out_addrinfos`: If non null: A preallocated backing array for the resulting list of address information. If null: indicates that the total number of required addresses should be specified in `out_Addrinfo_size`
/// * `out_addrinfo_size`: in/out paramter indicating the length of either the required size for all addresses to be supplied, or the maximal number of addresses desired.
///
/// Returns the results of the query
EXT_EXPORT sb_getaddrinfo_result_t sb_getaddrinfo(
    const char * const address,
    const char * const port,
    const sb_addrinfo_t * const hint,
    sb_addrinfo_t * const out_addrinfos,
    uint32_t * const out_addrinfo_size);

/// Initializes the socket `out_socket`
///
/// * `domain`: The internet layer protocol to be used by the socket
/// * `sock_type`: The socket type (stream or datagram)
/// * `protocol`: The transport layer to be used by the socket
/// * `out_socket`: The returned, initialized socket
///
/// Returns a platform error code (0 for success)
EXT_EXPORT int sb_create_socket(const sb_socket_family_e domain, const sb_socket_type_e sock_type, const sb_socket_protocol_type_e protocol, sb_socket_t * const out_socket);

/// Configures the socket `sock` to be blocking/non-blocking based on `blocking_mode`
///
/// * `sock`:  The socket to be configured
/// * `blocking_mode`: clocking or non-blocking
///
/// Returns true if successful
EXT_EXPORT bool sb_enable_blocking_socket(const sb_socket_t sock, const sb_socket_blocking_e blocking_mode);

/// Configures the socket `sock` to be no-delay/use-coalescing based on `tcp_delay_mode`
///
/// * `sock`: The socket to be configured
/// * `tcp_delay_mode`: no-delay or use coalescing
///
/// Returns true if successful
EXT_EXPORT bool sb_socket_set_tcp_no_delay(const sb_socket_t sock, const sb_socket_tcp_delay_mode_e tcp_delay_mode);

/// enables/disables as specified by `linger_mode` on the socket `sock` for `linger_timeout_in_seconds`
///
/// * `sock`: The socket to be configured
/// * `linger_mode`: Return immediately or after queued messages are handled
/// * `linger_timeout_in_seconds`:  Timeout in seconds
///
EXT_EXPORT bool sb_socket_set_linger(const sb_socket_t sock, const sb_socket_linger_mode_e linger_mode, const int linger_timeout_in_seconds);

/// Binds the socket `sock` to a socket address `addr` (assigning a 'name' to the socket)
///
/// * `sock`: The socket to be bound
/// * `addr`: Address onto which the socket is to be bound
///
/// Returns the resulting status
EXT_EXPORT sb_socket_bind_result_t sb_bind_socket(const sb_socket_t sock, const sb_sockaddr_t * const addr);

/// Marks the socket `sock` as a socket that will be used to accept incoming connections (via `accept`)
///
/// * `sock`: The socket to be configured
/// * `backlog`: The maximum length to which the queue of pending connections for `sock` may grow
///
/// Returns the resulting status
EXT_EXPORT sb_socket_listen_result_t sb_listen_socket(const sb_socket_t sock, const int backlog);

/// Creates a connection with the first pending connection request and provides that connection via the socket `out_sock`
///
/// * `sock`: The socket listening for connections
/// * `optional_sockaddr`: The address of the peer socket
/// * `out_sock`: The resulting connection
///
/// Returns the resulting status
EXT_EXPORT sb_socket_accept_result_t sb_accept_socket(const sb_socket_t sock, sb_sockaddr_t * const optional_sockaddr, sb_socket_t * const out_sock);

/// Connects the socket `sock` to the address `addr`
///
/// * `sock`: The socket to be connected
/// * `addr`: Address to which the socket will be connected
///
/// Returns the resulting status
EXT_EXPORT sb_socket_connect_result_t sb_connect_socket(const sb_socket_t sock, const sb_sockaddr_t * const addr);

/// Closes the socket `sock`
///
/// * `sock`: The socket to be closed
///
EXT_EXPORT void sb_close_socket(const sb_socket_t sock);

/// Forcibly terminates the socket
///
/// * `sock`: The socket to be shut down
/// * `shutdown_flags`: Bitmask flag signalling what is to close when shutting down the socket
///
EXT_EXPORT void sb_shutdown_socket(const sb_socket_t sock, const sb_socket_shutdown_flags_e shutdown_flags);

/// Returns the number of sockets that are ready for I/O operations
///
/// `select` allows for synchronous multiplexing of sockets, monitoring/waiting for sockets to become ready///
/// * `socket`: The socket to be monitored
/// * `timeout`: Maximum number of milliseconds for the operation to complete
///
/// Returns the resulting status
EXT_EXPORT sb_socket_select_result_e sb_socket_select(const sb_socket_t socket, milliseconds_t * const timeout);

/// Receives a message from `sock` and stores the content in the region `buffer` and the message size in `receive_size`
///
/// * `sock`: The socket from which to receive the message
/// * `buffer`: Where the message will be stored
/// * `flags`: Bitmask signaling how the message is to handled
/// * `receive_size`: Size in bites written to the `buffer`
///
/// Returns the resulting status
EXT_EXPORT sb_socket_receive_result_t sb_socket_receive(const sb_socket_t sock, const mem_region_t buffer, const sb_socket_receive_flags_e flags, int * const receive_size);

/// Receives a message from a connected or unconnected socket
///
/// If `address` is not NULL, the source address of the received message shall be stored in `out_addr`
///
/// * `sock`: The socket from which to receive the message
/// * `buffer`: Where the message will be stored
/// * `flags`: Bitmask signaling how the message is to handled
/// * `out_addr`: NULL or a `sb_sockaddr_t*` structure in which the sending address is to be stored
/// * `receive_size`: Size in bites written to the `buffer`
///
/// Returns the resulting status
EXT_EXPORT sb_socket_receive_result_t sb_socket_receive_from(const sb_socket_t sock, const mem_region_t buffer, const sb_socket_receive_flags_e flags, sb_sockaddr_t * const out_addr, int * const receive_size);

/// Transmits a `message` to the socket connected to `sock`
///
/// * `sock`: The socket on which the message will be sent
/// * `message`: The message to be sent
/// * `flags`: Bitmask signaling how the message is to be sent
/// * `bytes_sent`: Returned number of bytes actually sent
///
/// Returns the resulting status
EXT_EXPORT sb_socket_send_result_t sb_socket_send(const sb_socket_t sock, const const_mem_region_t message, const sb_socket_send_flags_e flags, int * const bytes_sent);

/// Transmits a `message` to the socket corresponding to the address `addr`
///
/// * `sock`: The socket on which the message will be sent
/// * `message`: The message to be sent
/// * `flags`: Bitmask signaling how the message is to be sent
/// * `addr`:  the destination address
/// * `bytes_sent`: Returned number of bytes actually sent
///
/// Returns the resulting status
EXT_EXPORT sb_socket_send_result_t sb_socket_send_to(const sb_socket_t sock, const const_mem_region_t message, const sb_socket_send_flags_e flags, const sb_sockaddr_t * const addr, int * const bytes_sent);

/// Converts a socket address to a corresponding host and service (independent of protocol)
///
/// * `addr`:  the socket address structure to be converted
/// * `host`: Where the returned host name will be stored
/// * `service`: Where the returned service name (i.e., port) will be stored
/// * `flags`: Bitmask signaling what is to be returned
///
EXT_EXPORT void sb_getnameinfo(const sb_sockaddr_t * const addr, mem_region_t * const host, mem_region_t * const service, const sb_getnameinfo_flags_e flags);

/// Returns the current address to which the socket `sock` is bound as `sockaddr`
///
/// * `sock`: The socket from which to extract the address
/// * `sockaddr`: Where the returned address will be stored
///
EXT_EXPORT void sb_getsockname(const sb_socket_t sock, sb_sockaddr_t * const sockaddr);

/// Returns the address of the peer connected to the socket `sock` in `sockaddr`
///
/// * `sock`: The socket from which to extract the address
/// * `sockaddr`: Where the returned address will be stored
///
EXT_EXPORT void sb_getpeername(const sb_socket_t sock, sb_sockaddr_t * const sockaddr);

/// Returns a readable-string representation of a *getaddrinfo* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_getaddrinfo_error_str(const sb_getaddrinfo_error_code_e err);

/// Returns a readable-string representation of a socket *connect* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_socket_connect_error_str(const sb_socket_connect_error_e err);

/// Returns a readable-string representation of a socket *accept* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_socket_accept_error_str(const sb_socket_accept_error_e err);

/// Returns a readable-string representation of a socket *bind* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_socket_bind_error_str(const sb_socket_bind_error_e err);

/// Returns a readable-string representation of a socket *listen* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_socket_listen_error_str(const sb_socket_listen_error_e err);

/// Returns a readable-string representation of a socket *receive* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_socket_receive_error_str(const sb_socket_receive_error_e err);

/// Returns a readable-string representation of a socket *send* error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_socket_send_error_str(const sb_socket_send_error_e err);

/// Returns a readable-string representation of a platform socket error
///
/// * `err`: Error code
///
/// Returns a human-readable string describing of the error
EXT_EXPORT const char * sb_platform_socket_error_str(int err);

#ifdef __cplusplus
}
#endif
