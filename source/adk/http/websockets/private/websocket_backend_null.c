/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/adk_http2.h"
#include "source/adk/http/websockets/adk_websocket_backend_selector.h"

#define SHIM_NAME(NAME) null_##NAME

void SHIM_NAME(adk_http_init)(const char * const ssl_certificate_path_ignored, const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
}

void SHIM_NAME(adk_http_shutdown)(const char * const tag) {
}

void SHIM_NAME(adk_http_dump_heap_usage)() {
}

heap_metrics_t SHIM_NAME(adk_http_get_heap_metrics)() {
    return (heap_metrics_t){0};
}

bool SHIM_NAME(adk_http_tick)() {
    return false;
}

void SHIM_NAME(adk_http_set_proxy)(const char * const proxy) {
}

void SHIM_NAME(adk_http_set_socks)(const char * const socks) {
}

adk_http_header_list_t * SHIM_NAME(adk_http_append_header_list)(adk_http_header_list_t * const list, const char * const name, const char * const value, const char * const tag) {
    return NULL;
}

adk_websocket_handle_t * SHIM_NAME(adk_websocket_create)(const char * const url, const char * const supported_protocols, adk_http_header_list_t * const header_list, const adk_websocket_callbacks_t callbacks, const char * const tag) {
    return NULL;
}

adk_websocket_handle_t * SHIM_NAME(adk_websocket_create_with_ssl_ctx)(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    const adk_websocket_callbacks_t callbacks,
    mem_region_t ssl_ca,
    mem_region_t ssl_client,
    const char * const tag) {
    return NULL;
}

void SHIM_NAME(adk_websocket_close)(adk_websocket_handle_t * const ws_handle, const char * const tag) {
}

adk_websocket_status_e SHIM_NAME(adk_websocket_get_status)(adk_websocket_handle_t * const ws_handle) {
    return adk_websocket_status_internal_error;
}

adk_websocket_status_e SHIM_NAME(adk_websocket_send)(adk_websocket_handle_t * const ws_handle, const const_mem_region_t message, const adk_websocket_message_type_e message_type, const adk_websocket_callbacks_t write_status_callback) {
    return adk_websocket_status_internal_error;
}

adk_websocket_message_type_e SHIM_NAME(adk_websocket_begin_read)(adk_websocket_handle_t * const ws_handle, const_mem_region_t * const message) {
    return adk_websocket_message_error;
}

void SHIM_NAME(adk_websocket_end_read)(FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle, FFI_MALLOC_TAG const char * const tag) {
}

adk_websocket_vtable_t adk_websocket_vtable_null = {
    .adk_http_init = SHIM_NAME(adk_http_init),
    .adk_http_shutdown = SHIM_NAME(adk_http_shutdown),
    .adk_http_dump_heap_usage = SHIM_NAME(adk_http_dump_heap_usage),
    .adk_http_set_proxy = SHIM_NAME(adk_http_set_proxy),
    .adk_http_set_socks = SHIM_NAME(adk_http_set_socks),
    .adk_http_tick = SHIM_NAME(adk_http_tick),
    .adk_http_get_heap_metrics = SHIM_NAME(adk_http_get_heap_metrics),
    .adk_http_append_header_list = SHIM_NAME(adk_http_append_header_list),
    .adk_websocket_create = SHIM_NAME(adk_websocket_create),
    .adk_websocket_create_with_ssl_ctx = SHIM_NAME(adk_websocket_create_with_ssl_ctx),
    .adk_websocket_close = SHIM_NAME(adk_websocket_close),
    .adk_websocket_get_status = SHIM_NAME(adk_websocket_get_status),
    .adk_websocket_send = SHIM_NAME(adk_websocket_send),
    .adk_websocket_begin_read = SHIM_NAME(adk_websocket_begin_read),
    .adk_websocket_end_read = SHIM_NAME(adk_websocket_end_read),
};