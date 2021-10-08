/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once
#include "source/adk/http/adk_http2.h"
#include "source/adk/http/websockets/websockets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adk_websocket_vtable_t {
    void (*adk_http_init)(const char * const ssl_certificate_path, const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag);
    void (*adk_http_shutdown)(const char * const tag);
    void (*adk_http_dump_heap_usage)();
    void (*adk_http_set_proxy)(const char * const proxy);
    void (*adk_http_set_socks)(const char * const socks);
    bool (*adk_http_tick)();

    adk_http_header_list_t * (*adk_http_append_header_list)(adk_http_header_list_t * list, const char * const name, const char * const value, const char * const tag);
    adk_websocket_handle_t * (*adk_websocket_create)(const char * const url, const char * const supported_protocols, adk_http_header_list_t * const header_list, const adk_websocket_callbacks_t callbacks, const char * const tag);
    adk_websocket_handle_t * (*adk_websocket_create_with_ssl_ctx)(const char * const url, const char * const supported_protocols, adk_http_header_list_t * const header_list, const adk_websocket_callbacks_t callbacks, mem_region_t ssl_ca, mem_region_t ssl_client, const char * const tag);
    void (*adk_websocket_close)(adk_websocket_handle_t * const ws_handle, const char * const tag);
    adk_websocket_status_e (*adk_websocket_get_status)(adk_websocket_handle_t * const ws_handle);
    adk_websocket_status_e (*adk_websocket_send)(adk_websocket_handle_t * const ws_handle, const const_mem_region_t message, const adk_websocket_message_type_e message_type, const adk_websocket_callbacks_t write_status_callback);
    adk_websocket_message_type_e (*adk_websocket_begin_read)(adk_websocket_handle_t * const ws_handle, const_mem_region_t * const message);
    void (*adk_websocket_end_read)(adk_websocket_handle_t * const ws_handle, const char * const tag);
} adk_websocket_vtable_t;

typedef enum adk_websocket_backend_e {
    adk_websocket_backend_none,
    adk_websocket_backend_http2,
    adk_websocket_backend_websocket,
} adk_websocket_backend_e;

EXT_EXPORT void adk_websocket_backend_set(const adk_websocket_backend_e backend);
EXT_EXPORT void adk_websocket_backend_set_websocket_config(const websocket_config_t config);

#ifdef __cplusplus
}
#endif
