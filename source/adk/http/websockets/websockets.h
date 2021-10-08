/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/http/http_status_codes.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum websocket_message_type_e {
    websocket_message_text,
    websocket_message_binary,
} websocket_message_type_e;

typedef enum websocket_status_e {
    /// websocket is the process of connecting
    websocket_status_connecting = 0,
    /// websocket is currently connected.
    websocket_status_connected,
    /// the remote end has sent us a close frame, and we are in the process of closing the websocket.
    websocket_status_closing,
    /// the websocket is closed (an error may have occurred, causing it close without sending/receiving a close frame)
    websocket_status_closed,
    // user initiated closing, not visible to well behaved users, primarily used for internal state tracking.
    websocket_status_user_initiated_closing,
    // user initiated closing, not visible to well behaved users, primarily used for internal state tracking
    websocket_status_user_initiated_closed,
} websocket_status_e;

typedef enum websocket_error_e {
    /// No errors/operation succeeded.
    websocket_error_none,
    /// Invalid action per protocol spec. Either missing data, data at wrong time, or other issues.
    websocket_error_protocol,
    /// Received too large of a payload for us to process.
    websocket_error_payload_too_large,
    /// Could not allocate sufficient memory for an operation (heap out of memory).
    websocket_error_allocation_failure,
    /// Too much time has passed since the last ping and the connection is assumed dead.
    websocket_error_timeout,
    /// Underlying socket send failed during initial handshake.
    websocket_error_upgrade_failed_send_failure,
    /// Underlying socket read failed during initial handshake.
    websocket_error_upgrade_failed_read_failure,
    /// Received a non 101 status code for upgrade. Or other handshake failure.
    websocket_error_upgrade_failed_handshake_failure,
    /// Generic connection failure during initial connection to the remote end.
    websocket_error_connection,
    /// Attempting to connect to the remote endpoint and exchange handshakes took too long.
    websocket_error_connection_handshake_timeout,
    /// Underlying socket read operation failure.
    websocket_error_socket_read_failure,
    /// Underlying socket send operation failure.
    websocket_error_socket_send_failure,
    /// Underlying socket exception.
    websocket_error_socket_exception,
    /// No readable messages in queue, try again later.
    websocket_error_no_readable_messages,
    /// Operation failed, websocket is not connected.
    websocket_error_not_connected,

    // not an error we emit, used for bookkeeping/indexing.
    websocket_error_last,
    FORCE_ENUM_SIGNED(websocket_error_e),
} websocket_error_e;

typedef struct websocket_t websocket_t;

typedef struct websocket_client_t websocket_client_t;

typedef struct websocket_message_t {
    websocket_client_t * client;
    websocket_message_type_e message_type;
    const_mem_region_t region;
} websocket_message_t;

typedef enum websocket_client_default_cert_usage_e {
    websocket_client_default_certs_load,
    websocket_client_default_certs_skip,
} websocket_client_default_cert_usage_e;

typedef enum websocket_client_additional_cert_type_e {
    websocket_client_additional_cert_type_file,
    websocket_client_additional_cert_type_mem_region
} websocket_client_additional_cert_type_e;

typedef struct websocket_client_additional_cert_t {
    websocket_client_additional_cert_type_e type;
    union {
        struct {
            const_mem_region_t cert_region;
            const char * cert_name;
        };
        const char * filepath;
    };
} websocket_client_additional_cert_t;

typedef struct websocket_http_header_list_t websocket_http_header_list_t;

websocket_http_header_list_t * websocket_client_append_header_list(websocket_client_t * const websocket_client, websocket_http_header_list_t * const list, const char * const name, const char * const value, const char * const tag);

websocket_client_t * websocket_client_emplace_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag);
websocket_client_t * websocket_client_emplace_init_custom_certs(
    const mem_region_t region,
    const system_guard_page_mode_e guard_page_mode,
    websocket_client_additional_cert_t * const additional_cert_array,
    const size_t additional_cert_array_size,
    const websocket_client_default_cert_usage_e default_cert_usage,
    const char * const tag);

void websocket_client_shutdown(websocket_client_t * const websokcet_client, const char * const tag);

void websocket_client_dump_heap_usage(const websocket_client_t * const websocket_client);

// returns true if there are any websockets left in internal list at end of tick.
bool websocket_client_tick(websocket_client_t * const websocket_client);

/// Additional websocket configuration beyond typical websocket spec requirements, refer to each field for details.
typedef struct websocket_config_t {
    /// If we are uncertain a connection is alive, and this amount of time has passed since we've sent a ping the connection will be considered dead.
    milliseconds_t ping_timeout;
    /// A connection is assumed alive if we have received activity on the socket in this time frame.
    /// If we have not received network IO in this duration the underlying websocket will send a ping frame to end point.
    milliseconds_t no_activity_wait_period;
    /// When trying to initially connect to the remote end point and exchange handshakes take at most this long.
    /// If we take longer than this fail the websocket connection.
    /// Note: redirects do not reset the timeout.
    /// Set to zero to disable handshake timeout.
    milliseconds_t max_handshake_timeout;
    /// Max allowed receivable message in bytes, anything larger than this will close the websocket down with a `payload_too_large` error
    uint32_t max_receivable_message_size;
    /// Limits the maximum size of the receive buffer, generally this is only used for control frames,
    /// and should be large enough to fit the largest control frame message.
    /// Effectively this should be sufficiently large to read in the http close status + message (if supplied) or the size of the ping frame's optional message.
    /// If the buffer is insufficient in size, then we websocket will be closed with a `payload_too_large` error.
    uint32_t receive_buffer_size;
    /// Maximum size of a message allowed before websocket fragmentation occurs, this size is increased by maximum size of the websocket frame (14 bytes)
    /// Small values may pessimize network traffic.
    /// Too large of values may increase packet loss on the tcp layer causing more frequent resends.
    uint32_t send_buffer_size;
    /// This value must be sufficiently large to store our headers we will send for the handshake, and sufficiently large to receive headers back from the remote end.
    /// It would be reasonable to consult common http servers (nginx/others) to figure out what their defaults are.
    /// 4096 should be well beyond what is required for websockets under normal usage. and may infact be able to go far lower.
    uint32_t header_buffer_size;
    /// The maximum allowed redirects when attempting to connect.
    /// Set to zero to deny redirects.
    uint32_t maximum_redirects;
} websocket_config_t;

/// creates a websocket
/// `url` the url to connect the websocket to
/// `optional_protocols` if specified indicates a comma deliminated list of allowed protocols for this websocket.
///    If specified the user should check `websocket_get_accepted_protocols()` for the accepted protocols.
/// `optional_additional_headers` if specified is a list of additional headers to send with initial request
///    If headers need to be inspected the user should check `websocket_get_response_header_bytes()` for the headers.
/// `config` is additional websocket configurations to apply to the websocket, field should be non zero.
websocket_t * websocket_create(
    websocket_client_t * const websocket_client,
    const char * const url,
    const char * const optional_protocols,
    websocket_http_header_list_t * const optional_additional_headers,
    const websocket_config_t config,
    const char * const tag);

/// Enqueues a message to be sent on the websocket.
/// Messages may be enqueued while the websocket is connecting.
/// Will return a websocket_error_e indicating success/failure of the send enqueu.
websocket_error_e websocket_send(websocket_t * const ws, const websocket_message_type_e message_type, const const_mem_region_t message);

/// any successfully read messages must be freed by the user by calling websocket_free_message.
websocket_error_e websocket_read_message(websocket_t * const ws, websocket_message_t * const out_message);
/// frees the underlying websocket message
void websocket_free_message(const websocket_message_t message, const char * const tag);

/// Closes websocket, with an optional http status code, if no status code is provided defaults to 1001 (going away)
/// If the user has initiaed a close, defers the close operation until websocket_client_tick() has completed normal closing operations for this websocket.
/// If the websocket status is already 'closed' frees the underlying websocket.
void websocket_close(websocket_t * const ws, const http_status_code_e * const optional_status_code);

/// Gets the current websocket status, if the websocekt is labeled as `closed` there may be an error, check `websocket_get_error()`
websocket_status_e websocket_get_status(const websocket_t * const ws);
/// Returns the current error state of the websocket.
websocket_error_e websocket_get_error(const websocket_t * const ws);
/// Returns a statically allocated human readable error message for the specified websocket error.
const char * websocket_error_to_str(const websocket_error_e ws_error);
/// Returns the accepted protocols for the websocket.
/// Will return NULL if the websocket is connecting, or if no header field was found in the response headers.
const char * websocket_get_accepted_protocols(const websocket_t * const ws);
/// Returns a memory region to the underlying response header bytes, this should _not_ be manually freed.
mem_region_t websocket_get_response_header_bytes(const websocket_t * const ws);
/// Returns the initial response's http status code.
/// Will return 0 if the websocket is still connecting.
/// Not all http status codes are mapped (or can be), only common ones are mapped to `http_status_code_e` however whatever the status code response was will be returned.
http_status_code_e websocket_get_response_http_status_code(const websocket_t * const ws);

#ifdef __cplusplus
}
#endif