/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/websockets/adk_websocket_backend_selector.h"

#ifdef _ADK_WEBSOCKET_TRACE
#include "source/adk/telemetry/telemetry.h"
#define ADK_WEBSOCKET_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define ADK_WEBSOCKET_TRACE_POP() TRACE_POP()
#else
#define ADK_WEBSOCKET_TRACE_PUSH_FN()
#define ADK_WEBSOCKET_TRACE_POP()
#endif

static struct statics_t {
    adk_websocket_vtable_t vtable;
    adk_websocket_backend_e backend;
} statics;

extern adk_websocket_vtable_t adk_websocket_vtable_websocket;
extern adk_websocket_vtable_t adk_websocket_vtable_http2;
extern adk_websocket_vtable_t adk_websocket_vtable_null;

static void adk_websocket_backend_set(const adk_websocket_backend_e backend) {
    if (backend == adk_websocket_backend_http2) {
        statics.vtable = adk_websocket_vtable_http2;
    } else if (backend == adk_websocket_backend_websocket) {
        statics.vtable = adk_websocket_vtable_websocket;
    } else {
        statics.vtable = adk_websocket_vtable_null;
    }
    statics.backend = backend;
}

void ws_shim_set_websocket_config(const websocket_config_t config);

static void adk_websocket_backend_set_websocket_config(const websocket_config_t config) {
    ws_shim_set_websocket_config(config);
}

void adk_http_init(
    const char * const ssl_certificate_path,
    const mem_region_t region,
    const adk_websocket_backend_e backend,
    const websocket_config_t config,
    const system_guard_page_mode_e guard_page_mode,
    const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    adk_websocket_backend_set(backend);
    adk_websocket_backend_set_websocket_config(config);
    statics.vtable.adk_http_init(ssl_certificate_path, region, guard_page_mode, tag);
    ADK_WEBSOCKET_TRACE_POP();
}

void adk_http_shutdown(const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    statics.vtable.adk_http_shutdown(tag);
    adk_websocket_backend_set(adk_websocket_backend_null);
    ADK_WEBSOCKET_TRACE_POP();
}

void adk_http_dump_heap_usage() {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    statics.vtable.adk_http_dump_heap_usage();
    ADK_WEBSOCKET_TRACE_POP();
}

heap_metrics_t adk_http_get_heap_metrics() {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    const heap_metrics_t metrics = statics.vtable.adk_http_get_heap_metrics();
    ADK_WEBSOCKET_TRACE_POP();
    return metrics;
}

void adk_http_set_proxy(const char * const proxy) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    statics.vtable.adk_http_set_proxy(proxy);
    ADK_WEBSOCKET_TRACE_POP();
}

void adk_http_set_socks(const char * const socks) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    statics.vtable.adk_http_set_socks(socks);
    ADK_WEBSOCKET_TRACE_POP();
}

bool adk_http_tick() {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    const bool status = statics.vtable.adk_http_tick();
    ADK_WEBSOCKET_TRACE_POP();
    return status;
}

adk_http_header_list_t * adk_http_append_header_list(adk_http_header_list_t * list, const char * const name, const char * const value, const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    adk_http_header_list_t * const header_list = statics.vtable.adk_http_append_header_list(list, name, value, tag);
    ADK_WEBSOCKET_TRACE_POP();
    return header_list;
}

adk_websocket_handle_t * adk_websocket_create(const char * const url, const char * const supported_protocols, adk_http_header_list_t * const header_list, const adk_websocket_callbacks_t callbacks, const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    adk_websocket_handle_t * const ws_handle = statics.vtable.adk_websocket_create(url, supported_protocols, header_list, callbacks, tag);
    ADK_WEBSOCKET_TRACE_POP();
    return ws_handle;
}

adk_websocket_handle_t * adk_websocket_create_with_ssl_ctx(const char * const url, const char * const supported_protocols, adk_http_header_list_t * const header_list, const adk_websocket_callbacks_t callbacks, mem_region_t ssl_ca, mem_region_t ssl_client, const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    adk_websocket_handle_t * const ws_handle = statics.vtable.adk_websocket_create_with_ssl_ctx(url, supported_protocols, header_list, callbacks, ssl_ca, ssl_client, tag);
    ADK_WEBSOCKET_TRACE_POP();
    return ws_handle;
}

void adk_websocket_close(adk_websocket_handle_t * const ws_handle, const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    statics.vtable.adk_websocket_close(ws_handle, tag);
    ADK_WEBSOCKET_TRACE_POP();
}

adk_websocket_status_e adk_websocket_get_status(adk_websocket_handle_t * const ws_handle) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    const adk_websocket_status_e status = statics.vtable.adk_websocket_get_status(ws_handle);
    ADK_WEBSOCKET_TRACE_POP();
    return status;
}

adk_websocket_status_e adk_websocket_send(adk_websocket_handle_t * const ws_handle, const const_mem_region_t message, const adk_websocket_message_type_e message_type, const adk_websocket_callbacks_t write_status_callback) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    const adk_websocket_status_e status = statics.vtable.adk_websocket_send(ws_handle, message, message_type, write_status_callback);
    ADK_WEBSOCKET_TRACE_POP();
    return status;
}

adk_websocket_message_type_e adk_websocket_begin_read(adk_websocket_handle_t * const ws_handle, const_mem_region_t * const message) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    const adk_websocket_message_type_e message_type = statics.vtable.adk_websocket_begin_read(ws_handle, message);
    ADK_WEBSOCKET_TRACE_POP();
    return message_type;
}

void adk_websocket_end_read(adk_websocket_handle_t * const ws_handle, const char * const tag) {
    ADK_WEBSOCKET_TRACE_PUSH_FN();
    statics.vtable.adk_websocket_end_read(ws_handle, tag);
    ADK_WEBSOCKET_TRACE_POP();
}
