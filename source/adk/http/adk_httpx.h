/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/// A client is used to create and send HTTP requests. The client is ticked in order to perform progress on the pending requests/responses.
FFI_ALWAYS_TYPED
typedef struct adk_httpx_client_t adk_httpx_client_t;

/// An HTTP request to send
FFI_ALWAYS_TYPED
typedef struct adk_httpx_request_t adk_httpx_request_t;

FFI_EXPORT FFI_ENUM_CLEAN_NAMES typedef enum adk_future_status_e {
    adk_future_status_pending,
    adk_future_status_ready,
} adk_future_status_e;

FFI_EXPORT FFI_ENUM_CLEAN_NAMES typedef enum adk_httpx_result_e {
    adk_httpx_ok,
    adk_httpx_timeout,
    adk_httpx_error,
} adk_httpx_result_e;

/// A response is the result of sending a request. The response is asynchronous and has a 'ready' status when completed.
FFI_ALWAYS_TYPED
typedef struct adk_httpx_response_t adk_httpx_response_t;

typedef bool (*adk_httpx_on_header_t)(adk_httpx_response_t * const response, const const_mem_region_t header, void * userdata);
typedef bool (*adk_httpx_on_body_t)(adk_httpx_response_t * const response, const const_mem_region_t body, void * userdata);
typedef void (*adk_httpx_on_complete_t)(adk_httpx_response_t * const response, void * userdata);

typedef enum adk_httpx_init_mode_e {
    adk_httpx_init_minimal = 0,
    adk_httpx_init_normal = 1
} adk_httpx_init_mode_e;

void adk_httpx_init_global_certs(bool use_global_certs);
void adk_httpx_free_global_certs();

typedef struct adk_httpx_http2_options {
    bool enabled;
    bool use_multiplexing;
    bool multiplex_wait_for_existing_connection;
} adk_httpx_http2_options;

void adk_httpx_enable_http2(adk_httpx_http2_options options);

adk_httpx_client_t * adk_httpx_client_create(
    const mem_region_t region,
    const mem_region_t fragments_region,
    const uint32_t fragment_size,
    const uint32_t pump_sleep_period,
    const system_guard_page_mode_e guard_page_mode,
    const adk_httpx_init_mode_e init_mode,
    const char * const name);

void adk_httpx_client_free(adk_httpx_client_t * const client);

bool adk_httpx_client_tick(adk_httpx_client_t * const client);

void adk_httpx_client_dump_heap_usage(adk_httpx_client_t * const client);

heap_metrics_t adk_httpx_client_get_heap_metrics(adk_httpx_client_t * const client);

FFI_EXPORT FFI_ENUM_CLEAN_NAMES typedef enum adk_httpx_method_e {
    adk_httpx_method_get,
    adk_httpx_method_post,
    adk_httpx_method_put,
    adk_httpx_method_patch,
    adk_httpx_method_delete,
    adk_httpx_method_head,
} adk_httpx_method_e;

adk_httpx_request_t * adk_httpx_client_request(
    adk_httpx_client_t * const client,
    const adk_httpx_method_e method,
    const char * const url);

FFI_EXPORT FFI_PUB_CRATE void adk_httpx_request_set_verbose(FFI_PTR_NATIVE adk_httpx_request_t * const request, bool verbose);
FFI_EXPORT FFI_PUB_CRATE void adk_httpx_request_set_follow_location(FFI_PTR_NATIVE adk_httpx_request_t * const request, bool follow_location);
FFI_EXPORT FFI_PUB_CRATE void adk_httpx_request_set_preferred_receive_buffer_size(FFI_PTR_NATIVE adk_httpx_request_t * const request, uint32_t preferred_receive_buffer_size);

FFI_EXPORT FFI_PUB_CRATE void adk_httpx_request_set_header(
    FFI_PTR_NATIVE adk_httpx_request_t * const request,
    FFI_PTR_WASM const char * const header);

FFI_EXPORT FFI_PUB_CRATE void adk_httpx_request_set_timeout(
    FFI_PTR_NATIVE adk_httpx_request_t * const request,
    uint64_t timeout);

FFI_EXPORT FFI_PUB_CRATE void adk_httpx_request_set_body(
    FFI_PTR_NATIVE adk_httpx_request_t * const request,
    FFI_PTR_WASM FFI_SLICE const uint8_t * const body,
    FFI_TYPE_OVERRIDE(int32_t) const size_t body_size);

void adk_httpx_request_set_user_agent(
    adk_httpx_request_t * const request,
    const char * const user_agent);

void adk_httpx_request_set_on_header(adk_httpx_request_t * const request, adk_httpx_on_header_t on_header);
void adk_httpx_request_set_on_body(adk_httpx_request_t * const request, adk_httpx_on_body_t on_body);
void adk_httpx_request_set_on_complete(adk_httpx_request_t * const request, adk_httpx_on_complete_t on_complete);
void adk_httpx_request_set_userdata(adk_httpx_request_t * const request, void * const userdata);

typedef enum adk_httpx_buffering_mode_e {
    adk_httpx_buffering_mode_off = 0x0,
    adk_httpx_buffering_mode_body = 0x1,
    adk_httpx_buffering_mode_header = 0x2,
} adk_httpx_buffering_mode_e;

void adk_httpx_request_set_buffering_mode(adk_httpx_request_t * const request, adk_httpx_buffering_mode_e mode);

typedef enum adk_httpx_encoding_e {
    adk_httpx_encoding_none = 0x0,
    adk_httpx_encoding_gzip = 0x1,
} adk_httpx_encoding_e;

void adk_httpx_request_accept_encoding(adk_httpx_request_t * const request, adk_httpx_encoding_e encoding);

// NOTE: consumes (drops) `request`
FFI_EXPORT FFI_PUB_CRATE FFI_PTR_NATIVE adk_httpx_response_t * adk_httpx_send(
    FFI_PTR_NATIVE adk_httpx_request_t * const request);

FFI_EXPORT FFI_PUB_CRATE adk_future_status_e adk_httpx_response_get_status(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response);

FFI_EXPORT FFI_PUB_CRATE adk_httpx_result_e adk_httpx_response_get_result(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response);

FFI_EXPORT FFI_PUB_CRATE FFI_PTR_NATIVE FFI_CAN_BE_NULL const char * adk_httpx_response_get_error(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response);

FFI_EXPORT FFI_PUB_CRATE int64_t adk_httpx_response_get_response_code(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response);

const_mem_region_t adk_httpx_response_get_headers(const adk_httpx_response_t * const response);
const_mem_region_t adk_httpx_response_get_body(const adk_httpx_response_t * const response);

FFI_EXPORT FFI_PUB_CRATE void adk_httpx_response_free(FFI_PTR_NATIVE adk_httpx_response_t * const response);

typedef bool (*adk_httpx_fetch_on_header_t)(const const_mem_region_t header, void * userdata);
typedef bool (*adk_httpx_fetch_on_body_t)(const const_mem_region_t body, void * userdata);
typedef void (*adk_httpx_fetch_on_complete_t)(adk_httpx_result_e result, int32_t response_code, void * userdata);

typedef struct adk_httpx_fetch_callbacks_t {
    adk_httpx_fetch_on_header_t on_header;
    adk_httpx_fetch_on_body_t on_body;
    adk_httpx_fetch_on_complete_t on_complete;
    void * userdata;
} adk_httpx_fetch_callbacks_t;

/// Fetches the resource at `url`, waiting until complete.
void adk_httpx_fetch(
    heap_t * const heap,
    const char * const url,
    adk_httpx_fetch_callbacks_t callbacks,
    const char ** headers,
    const size_t num_headers,
    const seconds_t timeout);

#ifdef __cplusplus
}
#endif
