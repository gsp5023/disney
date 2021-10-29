/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
json_deflate.h

JSON deflate library API.
*/

#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/telemetry/telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_JSON_DEFLATE FOURCC('J', 'S', 'O', 'N')

typedef struct json_deflate_hooks_t {
    void * (*malloc_hook)(const size_t size);
    void * (*calloc_hook)(const size_t nmemb, const size_t size);
    void * (*unchecked_calloc_hook)(const size_t nmemb, const size_t size);
    void * (*realloc_hook)(void * const ptr, const size_t size);
    void (*free_hook)(void * const ptr);
} json_deflate_options_t;

FFI_EXPORT FFI_ENUM_CLEAN_NAMES typedef enum json_deflate_parse_target_e {
    json_deflate_parse_target_wasm = 0,
    json_deflate_parse_target_native = 1,
} json_deflate_parse_target_e;

FFI_EXPORT FFI_ENUM_CLEAN_NAMES typedef enum json_deflate_parse_status_e {
    json_deflate_parse_status_success = 0,
    json_deflate_parse_status_external_error = -1,
    json_deflate_parse_status_out_of_target_memory = -2,
    json_deflate_parse_status_invalid_json = -3,
    json_deflate_parse_status_invalid_binary_layout = -4,
    FORCE_ENUM_INT32(json_deflate_parse_status_e)
} json_deflate_parse_status_e;

FFI_EXPORT typedef struct json_deflate_parse_result_t {
    json_deflate_parse_status_e status;
    uint32_t offset;
    uint32_t end;
} json_deflate_parse_result_t;

typedef struct json_deflate_parse_data_result_t {
    json_deflate_parse_result_t result;
    bool do_retry;
} json_deflate_parse_data_result_t;

void json_deflate_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, thread_pool_t * const thread_pool);
void json_deflate_init_hooks(const json_deflate_options_t * const memory_hooks);
void * json_deflate_calloc(const size_t nmemb, const size_t size);
void * json_deflate_unchecked_calloc(const size_t nmemb, const size_t size);
void * json_deflate_malloc(const size_t size);
void * json_deflate_realloc(void * const ptr, const size_t size);
void json_deflate_free(void * const ptr);
void json_deflate_shutdown(void);
void json_deflate_dump_heap_usage();
heap_metrics_t json_deflate_get_heap_metrics(void);

json_deflate_parse_data_result_t json_deflate_parse_data(const const_mem_region_t schema_layout, const const_mem_region_t data, const mem_region_t buffer, const json_deflate_parse_target_e target, const uint32_t expected_size, const uint32_t schema_hash);
void json_deflate_parse_data_async(const const_mem_region_t schema_layout, const const_mem_region_t data, const mem_region_t buffer, const json_deflate_parse_target_e target, const uint32_t expected_size, const uint32_t schema_hash, void * const on_complete);
void json_deflate_parse_http_async(const const_mem_region_t schema_layout, adk_curl_handle_t * const http, const mem_region_t buffer, const json_deflate_parse_target_e target, const uint32_t expected_size, const uint32_t schema_hash, void * const on_deflate_complete);

typedef struct json_deflate_http_future_t json_deflate_http_future_t;

FFI_EXPORT FFI_PTR_NATIVE json_deflate_http_future_t * json_deflate_parse_httpx_async(
    FFI_PTR_WASM FFI_SLICE const uint8_t * const schema_layout,
    FFI_TYPE_OVERRIDE(uint64_t) const size_t schema_layout_size,
    FFI_PTR_NATIVE adk_httpx_request_t * const request,
    FFI_PTR_WASM const uint8_t * const buffer,
    FFI_TYPE_OVERRIDE(uint64_t) const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash);

FFI_EXPORT FFI_PTR_NATIVE json_deflate_http_future_t * json_deflate_parse_httpx_response_async(
    FFI_PTR_WASM FFI_SLICE const uint8_t * const schema_layout,
    FFI_TYPE_OVERRIDE(uint64_t) const size_t schema_layout_size,
    FFI_PTR_NATIVE adk_httpx_response_t * const response,
    FFI_PTR_WASM const uint8_t * const buffer,
    FFI_TYPE_OVERRIDE(uint64_t) const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash);

FFI_EXPORT void json_deflate_parse_httpx_resize(
    FFI_PTR_NATIVE json_deflate_http_future_t * const future,
    FFI_PTR_WASM FFI_SLICE const uint8_t * const schema_layout,
    FFI_TYPE_OVERRIDE(uint64_t) const size_t schema_layout_size,
    FFI_PTR_WASM const uint8_t * const buffer,
    FFI_TYPE_OVERRIDE(uint64_t) const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash);

FFI_EXPORT adk_future_status_e json_deflate_http_future_get_status(
    FFI_PTR_NATIVE const json_deflate_http_future_t * const future);

FFI_EXPORT FFI_PTR_NATIVE adk_httpx_response_t * json_deflate_http_future_get_response(
    FFI_PTR_NATIVE const json_deflate_http_future_t * const future);

FFI_EXPORT json_deflate_parse_result_t json_deflate_http_future_get_result(
    FFI_PTR_NATIVE const json_deflate_http_future_t * const future);

FFI_EXPORT void json_deflate_http_future_drop(
    FFI_PTR_NATIVE json_deflate_http_future_t * const future);

FFI_EXPORT void json_deflate_native_async(
    FFI_PTR_WASM FFI_SLICE const uint8_t * const schema_layout,
    const uint32_t schema_layout_length,
    FFI_PTR_NATIVE const uint8_t * const json_data,
    uint32_t json_data_length,
    FFI_PTR_WASM FFI_SLICE uint8_t * const buffer,
    const uint32_t buffer_size,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    const json_deflate_parse_target_e target,
    FFI_WASM_CALLBACK void * const on_complete);

FFI_EXPORT void json_deflate_async(
    FFI_PTR_WASM FFI_SLICE const uint8_t * const schema_layout,
    const uint32_t schema_layout_length,
    FFI_PTR_WASM FFI_SLICE const uint8_t * const json_data,
    uint32_t json_data_length,
    FFI_PTR_WASM FFI_SLICE uint8_t * const buffer,
    const uint32_t buffer_size,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    const json_deflate_parse_target_e target,
    FFI_WASM_CALLBACK void * const on_complete);

FFI_EXPORT void json_deflate_from_http_async(
    FFI_PTR_WASM FFI_SLICE const uint8_t * const schema_layout,
    const uint32_t schema_layout_length,
    FFI_PTR_NATIVE adk_curl_handle_t * const http,
    FFI_PTR_WASM FFI_SLICE uint8_t * const buffer,
    const uint32_t buffer_size,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    const json_deflate_parse_target_e target,
    FFI_WASM_CALLBACK void * const on_complete);

#ifdef __cplusplus
}
#endif
