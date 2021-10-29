/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char adk_http_init_default_cert_path[];
extern const char adk_http_init_default_charles_path[];

typedef enum adk_websocket_backend_e {
    adk_websocket_backend_null,
    adk_websocket_backend_http2,
    adk_websocket_backend_websocket,
} adk_websocket_backend_e;

struct websocket_config_t;

/// Initializes the HTTP system
EXT_EXPORT void adk_http_init(
    const char * const ssl_certificate_path,
    const mem_region_t region,
    const enum adk_websocket_backend_e backend,
    const struct websocket_config_t config,
    const system_guard_page_mode_e guard_page_mode,
    const char * const tag);

/// Shuts down the HTTP system
EXT_EXPORT void adk_http_shutdown(const char * const tag);

/// dumps internal heap usage
EXT_EXPORT void adk_http_dump_heap_usage();

/// gets internal heap metrics
EXT_EXPORT heap_metrics_t adk_http_get_heap_metrics();

/// enable and specify the http/https proxy to use.
/// format is: `host:ip` e.g. "myproxy.com:34" or "127.0.0.1.:8888"
FFI_EXPORT void adk_http_set_proxy(FFI_PTR_WASM const char * const proxy);

/// enable and specify the socks5 proxy to use.
/// format is: `host:ip` e.g. "myproxy.com:34" or "127.0.0.1.:8888"
FFI_EXPORT void adk_http_set_socks(FFI_PTR_WASM const char * const socks);

/// Performs an increment of work and returns *true* if events were processed
///
/// `tick` must be called periodically as work does not occur on separate threads
EXT_EXPORT bool adk_http_tick();

// websocket support

/// Handle to a websocket
FFI_DROP(adk_websocket_close)
FFI_CAN_BE_NULL
typedef struct adk_websocket_handle_t adk_websocket_handle_t;

/// Websocket message type
FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
typedef enum adk_websocket_message_type_e {
    adk_websocket_message_error = -1,
    adk_websocket_message_none = 0,
    adk_websocket_message_binary = 1,
    adk_websocket_message_text = 2,
} adk_websocket_message_type_e;

/// Status of the websocket
FFI_EXPORT FFI_NAME(adk_websocket_status_e)
    FFI_TYPE_MODULE(http2)
        FFI_ENUM_CLEAN_NAMES
    typedef enum adk_websocket_status_e {
        adk_websocket_status_internal_error = -4,
        adk_websocket_status_connection_failed = -3,

        adk_websocket_status_connection_closed = -2,
        adk_websocket_status_connection_closed_by_user = -1,

        adk_websocket_status_connected = 0,
        adk_websocket_status_connecting = 1,
    } adk_websocket_status_e;

/// Callbacks invoked on a websocket operation such as create and send
///
/// These callbacks are invoked by websocket operations whenever the operation completes or has encountered an error.
/// Callbacks are only called once and only one of the supplied callbacks will be called.
typedef struct adk_websocket_callbacks_t {
    void (*success)(adk_websocket_handle_t * const ws_handle, const struct adk_websocket_callbacks_t * const callbacks);
    void (*error)(adk_websocket_handle_t * const ws_handle, const struct adk_websocket_callbacks_t * const callbacks, const adk_websocket_status_e status);
    void * user[2];
} adk_websocket_callbacks_t;

/// a linked list of header name/values
typedef struct adk_http_header_list_t adk_http_header_list_t;

/// creates or appends a name and value to the list `list`
EXT_EXPORT FFI_EXPORT
    FFI_PTR_NATIVE adk_http_header_list_t *
    adk_http_append_header_list(FFI_PTR_NATIVE FFI_CAN_BE_NULL adk_http_header_list_t * list, FFI_PTR_WASM const char * const name, FFI_PTR_WASM const char * const value, FFI_MALLOC_TAG const char * const tag);

/// Creates a websocket connection with `url` and returns the handle to the websocket
///
/// - `url` example usage: wss://disney.com/some/arbitrary/path
/// - `supported_protocols` example usage: "send,chat,lws-mirror-protocol"
///   - a comma-delimited list of protocols we (the user of this library) will support
/// - header_list: a list created via adk_http_append_header_list
EXT_EXPORT adk_websocket_handle_t * adk_websocket_create(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    const char * const tag);

/// Creates a websocket connection with `url` and SSL context
/// and returns the handle to the websocket
///
/// - `url` example usage: wss://disney.com/some/arbitrary/path
/// - `supported_protocols` example usage: "send,chat,lws-mirror-protocol"
///   - a comma-delimited list of protocols we (the user of this library) will support
/// - header_list: a list created via adk_http_append_header_list
EXT_EXPORT adk_websocket_handle_t * adk_websocket_create_with_ssl_ctx(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    mem_region_t ssl_ca,
    mem_region_t ssl_client,
    const char * const tag);

/// Closes the websocket connection corresponding to `ws_handle`
///
/// After closing a websocket, performing subsequent operations on the handle is invalid/undefined.
EXT_EXPORT FFI_EXPORT void adk_websocket_close(FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle, FFI_MALLOC_TAG const char * const tag);

/// Returns the current status of the websocket corresponding to `ws_handle`
EXT_EXPORT FFI_EXPORT
    adk_websocket_status_e
    adk_websocket_get_status(FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle);

/// Enqueues a message for sending at the next available time and returns the current state of the websocket
///
/// The returned state shall indicate a failure if the websocket connection was terminated (and could not automatically reconnect)
EXT_EXPORT adk_websocket_status_e adk_websocket_send(adk_websocket_handle_t * const ws_handle, const const_mem_region_t message, const adk_websocket_message_type_e message_type, const adk_websocket_callbacks_t write_status_callback);

/// Returns the type of the next message and the content of the message in the region `message`
///
/// In the case where no message exists, `message` region will be zeroed.
EXT_EXPORT adk_websocket_message_type_e adk_websocket_begin_read(adk_websocket_handle_t * const ws_handle, const_mem_region_t * const message);

/// Indicates that the current message has been read and releases it
EXT_EXPORT FFI_EXPORT void adk_websocket_end_read(FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle, FFI_MALLOC_TAG const char * const tag);

#ifdef __cplusplus
}
#endif
