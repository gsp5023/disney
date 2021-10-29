/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/adk_http2.h"
#include "source/adk/http/websockets/adk_websocket_backend_selector.h"
#include "source/adk/http/websockets/websockets.h"
#include "source/adk/log/log.h"
#include "source/adk/steamboat/sb_thread.h"

#define SHIM_NAME(NAME) ws_shim_##NAME
#define WS_SHIM_TAG FOURCC('W', 'S', 'H', 'M')
#define WS_SHIM_THREAD_SAFE

static struct {
    websocket_client_t * client;
    websocket_config_t websocket_config;
    // not a fan of this, but the new websocket backend does not currently support reading/writing/modifications from separate threads. it _could_ but does not right now. it is however quick.
#ifdef WS_SHIM_THREAD_SAFE
    sb_mutex_t * mutex;
#endif
} statics;

static void lock_mutex() {
#ifdef WS_SHIM_THREAD_SAFE
    sb_lock_mutex(statics.mutex);
#endif
}

static void unlock_mutex() {
#ifdef WS_SHIM_THREAD_SAFE
    sb_unlock_mutex(statics.mutex);
#endif
}

void SHIM_NAME(adk_http_init)(const char * const ssl_certificate_path_ignored, const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
#ifdef WS_SHIM_THREAD_SAFE
    statics.mutex = sb_create_mutex(MALLOC_TAG);
#endif
    statics.client = websocket_client_emplace_init(region, guard_page_mode, tag);
}

void ws_shim_set_websocket_config(const websocket_config_t config) {
    statics.websocket_config = config;
}

void SHIM_NAME(adk_http_shutdown)(const char * const tag) {
    websocket_client_shutdown(statics.client, tag);
#ifdef WS_SHIM_THREAD_SAFE
    sb_destroy_mutex(statics.mutex, MALLOC_TAG);
#endif
}

void SHIM_NAME(adk_http_dump_heap_usage)() {
    lock_mutex();
    websocket_client_dump_heap_usage(statics.client);
    unlock_mutex();
}

heap_metrics_t SHIM_NAME(adk_http_get_heap_metrics)() {
    lock_mutex();
    const heap_metrics_t metrics = websocket_client_get_heap_metrics(statics.client);
    unlock_mutex();
    return metrics;
}

bool SHIM_NAME(adk_http_tick)() {
    lock_mutex();
    const bool status = websocket_client_tick(statics.client);
    unlock_mutex();
    return status;
}

void SHIM_NAME(adk_http_set_proxy)(const char * const proxy) {
    LOG_ALWAYS(WS_SHIM_TAG, "Setting proxy via adk_http_set_proxy not supported for current websocket backend! please use environment variables");
}

void SHIM_NAME(adk_http_set_socks)(const char * const socks) {
    LOG_ALWAYS(WS_SHIM_TAG, "Setting socks proxy via adk_http_set_socks not supported for current websocket backend!");
}

adk_http_header_list_t * SHIM_NAME(adk_http_append_header_list)(adk_http_header_list_t * const list, const char * const name, const char * const value, const char * const tag) {
    lock_mutex();
    adk_http_header_list_t * const header_list = (adk_http_header_list_t *)websocket_client_append_header_list(statics.client, (websocket_http_header_list_t *)list, name, value, tag);
    unlock_mutex();
    return header_list;
}

adk_websocket_handle_t * SHIM_NAME(adk_websocket_create)(const char * const url, const char * const supported_protocols, adk_http_header_list_t * const header_list, const adk_websocket_callbacks_t callbacks, const char * const tag) {
    lock_mutex();
    adk_websocket_handle_t * const ws_handle = (adk_websocket_handle_t *)websocket_create(statics.client, url, supported_protocols, (websocket_http_header_list_t *)header_list, statics.websocket_config, tag);
    unlock_mutex();
    if (websocket_get_error((websocket_t *)ws_handle) == websocket_error_none) {
        if (callbacks.success) {
            callbacks.success(ws_handle, &callbacks);
        }
    } else {
        if (callbacks.error) {
            callbacks.error(ws_handle, &callbacks, adk_websocket_status_connection_closed);
        }
    }
    return ws_handle;
}

websocket_t * websocket_create_with_ssl_ctx(
    websocket_client_t * const websocket_client,
    const char * const url,
    const char * const optional_protocols,
    websocket_http_header_list_t * const optional_additional_headers,
    const websocket_config_t config,
    const mem_region_t ssl_ca,
    const mem_region_t ssl_client,
    const char * const tag);

adk_websocket_handle_t * SHIM_NAME(adk_websocket_create_with_ssl_ctx)(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    mem_region_t ssl_ca,
    mem_region_t ssl_client,
    const char * const tag) {
    lock_mutex();
    adk_websocket_handle_t * const ws_handle = (adk_websocket_handle_t *)websocket_create_with_ssl_ctx(
        statics.client,
        url,
        supported_protocols,
        (websocket_http_header_list_t *)header_list,
        statics.websocket_config,
        ssl_ca,
        ssl_client,
        tag);
    unlock_mutex();
    if (websocket_get_error((websocket_t *)ws_handle) == websocket_error_none) {
        if (callbacks.success) {
            callbacks.success(ws_handle, &callbacks);
        }
    } else {
        if (callbacks.error) {
            callbacks.error(ws_handle, &callbacks, adk_websocket_status_connection_closed);
        }
    }
    return ws_handle;
}

void SHIM_NAME(adk_websocket_close)(adk_websocket_handle_t * const ws_handle, const char * const tag) {
    lock_mutex();
    websocket_close((websocket_t *)ws_handle, NULL);
    unlock_mutex();
}

static adk_websocket_status_e ws_shim_new_to_deprecated_status(websocket_t * const ws) {
    const websocket_status_e ws_status = websocket_get_status(ws);
    const websocket_error_e error = websocket_get_error(ws);
    if (ws_status == websocket_status_connecting) {
        return adk_websocket_status_connecting;
    } else if (ws_status == websocket_status_connected) {
        return adk_websocket_status_connected;
    }

    const websocket_error_e connection_failed_errors[] = {
        websocket_error_upgrade_failed_send_failure,
        websocket_error_upgrade_failed_read_failure,
        websocket_error_upgrade_failed_handshake_failure,
        websocket_error_connection,
        websocket_error_connection_handshake_timeout};
    for (size_t i = 0; i < ARRAY_SIZE(connection_failed_errors); ++i) {
        if (error == connection_failed_errors[i]) {
            return adk_websocket_status_connection_failed;
        }
    }
    return adk_websocket_status_internal_error;
}

adk_websocket_status_e SHIM_NAME(adk_websocket_get_status)(adk_websocket_handle_t * const ws_handle) {
    return ws_shim_new_to_deprecated_status((websocket_t *)ws_handle);
}

adk_websocket_status_e SHIM_NAME(adk_websocket_send)(adk_websocket_handle_t * const ws_handle, const const_mem_region_t message, const adk_websocket_message_type_e message_type, const adk_websocket_callbacks_t write_status_callback) {
    ASSERT(message_type == adk_websocket_message_binary || message_type == adk_websocket_message_text);
    lock_mutex();
    const websocket_error_e send_err = websocket_send((websocket_t *)ws_handle, message_type == adk_websocket_message_binary ? websocket_message_binary : websocket_message_text, message);
    const adk_websocket_status_e converted_status = ws_shim_new_to_deprecated_status((websocket_t *)ws_handle);
    unlock_mutex();
    if (send_err != websocket_error_none) {
        if (write_status_callback.error) {
            write_status_callback.error(ws_handle, &write_status_callback, converted_status);
        }
    }
    if (write_status_callback.success) {
        write_status_callback.success(ws_handle, &write_status_callback);
    }
    return converted_status;
}

adk_websocket_message_type_e SHIM_NAME(adk_websocket_begin_read)(adk_websocket_handle_t * const ws_handle, const_mem_region_t * const message) {
    websocket_message_t read_message;
    lock_mutex();
    const websocket_error_e read_error = websocket_read_message((websocket_t *)ws_handle, &read_message);
    unlock_mutex();
    if (read_error == websocket_error_no_readable_messages) {
        return adk_websocket_message_none;
    } else if (read_error != websocket_error_none) {
        return adk_websocket_message_error;
    }
    *message = read_message.region;
    return read_message.message_type == websocket_message_binary ? adk_websocket_message_binary : adk_websocket_message_text;
}

void ws_shim_support_free_message(websocket_t * ws, const char * const tag);
void SHIM_NAME(adk_websocket_end_read)(FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle, FFI_MALLOC_TAG const char * const tag) {
    lock_mutex();
    ws_shim_support_free_message((websocket_t *)ws_handle, tag);
    unlock_mutex();
}

adk_websocket_vtable_t adk_websocket_vtable_websocket = {
    .adk_http_init = SHIM_NAME(adk_http_init),
    .adk_http_shutdown = SHIM_NAME(adk_http_shutdown),
    .adk_http_dump_heap_usage = SHIM_NAME(adk_http_dump_heap_usage),
    .adk_http_set_proxy = SHIM_NAME(adk_http_set_proxy),
    .adk_http_set_socks = SHIM_NAME(adk_http_set_socks),
    .adk_http_tick = SHIM_NAME(adk_http_tick),

    .adk_http_append_header_list = SHIM_NAME(adk_http_append_header_list),
    .adk_websocket_create = SHIM_NAME(adk_websocket_create),
    .adk_websocket_create_with_ssl_ctx = SHIM_NAME(adk_websocket_create_with_ssl_ctx),
    .adk_websocket_close = SHIM_NAME(adk_websocket_close),
    .adk_websocket_get_status = SHIM_NAME(adk_websocket_get_status),
    .adk_websocket_send = SHIM_NAME(adk_websocket_send),
    .adk_websocket_begin_read = SHIM_NAME(adk_websocket_begin_read),
    .adk_websocket_end_read = SHIM_NAME(adk_websocket_end_read),
};