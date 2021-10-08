/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/http/adk_httpx.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT FFI_PUB_CRATE FFI_NAME(adk_httpx_client_tick) static inline bool adk_httpx_client_tick_const(
    FFI_PTR_NATIVE const adk_httpx_client_t * const client) {
    adk_httpx_client_t * const c = (adk_httpx_client_t * const)client;
    return adk_httpx_client_tick(c);
}

FFI_EXPORT FFI_PUB_CRATE FFI_NAME(adk_httpx_client_request) FFI_PTR_NATIVE adk_httpx_request_t * adk_httpx_client_request_const(
    FFI_PTR_NATIVE const adk_httpx_client_t * const client,
    const adk_httpx_method_e method,
    FFI_PTR_WASM const char * const url) {
    adk_httpx_client_t * const c = (adk_httpx_client_t * const)client;
    return adk_httpx_client_request(c, method, url);
}

FFI_EXPORT FFI_PUB_CRATE static inline FFI_PTR_NATIVE adk_httpx_client_t * adk_httpx_client_new() {
    extern adk_app_t the_app;
    return adk_httpx_client_create(the_app.httpx);
}

FFI_EXPORT FFI_PUB_CRATE FFI_TYPE_OVERRIDE(uint32_t) static inline size_t adk_httpx_response_get_headers_size(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response) {
    return adk_httpx_response_get_headers(response).size;
}

FFI_EXPORT FFI_PUB_CRATE FFI_TYPE_OVERRIDE(uint32_t) static inline size_t adk_httpx_response_get_headers_copy(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response,
    FFI_PTR_WASM FFI_SLICE uint8_t * const buffer,
    FFI_TYPE_OVERRIDE(uint32_t) const size_t buffer_size) {
    ASSERT(buffer != NULL);

    const const_mem_region_t region = adk_httpx_response_get_headers(response);
    const size_t size = min_size_t(region.size, buffer_size);
    memcpy(buffer, region.ptr, size);
    return size;
}

FFI_EXPORT FFI_PUB_CRATE FFI_TYPE_OVERRIDE(uint32_t) static inline size_t adk_httpx_response_get_body_size(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response) {
    return adk_httpx_response_get_body(response).size;
}

FFI_EXPORT FFI_PUB_CRATE FFI_TYPE_OVERRIDE(uint32_t) static inline size_t adk_httpx_response_get_body_copy(
    FFI_PTR_NATIVE const adk_httpx_response_t * const response,
    FFI_PTR_WASM FFI_SLICE uint8_t * const buffer,
    FFI_TYPE_OVERRIDE(uint32_t) const size_t buffer_size) {
    ASSERT(buffer != NULL);

    const const_mem_region_t region = adk_httpx_response_get_body(response);
    const size_t size = min_size_t(region.size, buffer_size);
    memcpy(buffer, region.ptr, size);
    return size;
}

#ifdef __cplusplus
}
#endif
