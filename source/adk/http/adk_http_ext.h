/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/http/adk_http.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT void adk_curl_async_perform_raw(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle,
    FFI_WASM_CALLBACK void * const on_http_recv_header_ptr,
    FFI_WASM_CALLBACK void * const on_http_recv_ptr,
    FFI_WASM_CALLBACK void * const on_complete_ptr);

FFI_EXPORT FFI_NO_RUST_THUNK adk_curl_result_e adk_curl_get_info_i32(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle,
    const adk_curl_info_e info,
    FFI_PTR_WASM int32_t * const out);

FFI_EXPORT void adk_curl_set_opt_slist(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle,
    const adk_curl_option_e opt,
    FFI_PTR_NATIVE adk_curl_slist_t * const arg);

FFI_EXPORT FFI_NO_RUST_THUNK FFI_PTR_NATIVE adk_websocket_handle_t * adk_websocket_create_raw(
    FFI_PTR_WASM const char * const url,
    FFI_PTR_WASM const char * const supported_protocols,
    FFI_PTR_NATIVE FFI_CAN_BE_NULL adk_http_header_list_t * const header_list,
    FFI_WASM_CALLBACK void * success_callback,
    FFI_WASM_CALLBACK void * error_callback);

FFI_EXPORT FFI_NO_RUST_THUNK adk_websocket_status_e adk_websocket_send_raw(
    FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle,
    FFI_PTR_WASM FFI_SLICE void * const ptr,
    const int32_t size,
    const adk_websocket_message_type_e message_type,
    FFI_WASM_CALLBACK void * const success_callback,
    FFI_WASM_CALLBACK void * const error_callback);

FFI_EXPORT FFI_NAME(adk_curl_get_http_body) native_slice_t adk_curl_get_http_body_bridge(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle);

FFI_EXPORT FFI_NAME(adk_curl_get_http_header) native_slice_t adk_curl_get_http_header_bridge(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle);

FFI_EXPORT adk_websocket_message_type_e adk_websocket_begin_read_bridge(
    FFI_PTR_NATIVE adk_websocket_handle_t * const ws_handle,
    FFI_PTR_WASM native_slice_t * const out_buffer);

#ifdef __cplusplus
}
#endif
