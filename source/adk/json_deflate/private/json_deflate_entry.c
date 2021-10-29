/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_operations.c

JSON deflate high-level operations (infer, prepare, view, parse).
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/cjson/adk_cjson_context.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_platform.h"

typedef struct json_deflate_parse_data_args_t {
    const_mem_region_t schema;
    const_mem_region_t data;
    mem_region_t buffer;
    json_deflate_parse_target_e target;
    void * on_deflate_complete;
    uint32_t offset;
    uint32_t end;
    json_deflate_parse_status_e status;
    uint32_t expected_size;
    uint32_t schema_hash;
} json_deflate_parse_data_args_t;

typedef struct json_deflate_parse_http_args_s {
    const_mem_region_t schema;
    adk_curl_handle_t * http;
    mem_region_t buffer;
    json_deflate_parse_target_e target;
    void * on_deflate_complete;
    uint32_t expected_size;
    uint32_t schema_hash;
} json_deflate_parse_http_args_t;

// ========================================

static void json_deflate_queue_wait() {
    // Use a mutex and condition variable to wait for a previous json deflate to complete
    sb_lock_mutex(json_deflate_get_parallel_mutex());
    sb_wait_condition(json_deflate_get_cv(), json_deflate_get_parallel_mutex(), sb_timeout_infinite);
    sb_unlock_mutex(json_deflate_get_parallel_mutex());
}

static void json_deflate_queue_continue() {
    sb_condition_wake_one(json_deflate_get_cv());
}

// ========================================

static void json_deflate_parse_data_async_adapter(void * args, thread_pool_t * const pool) {
    json_deflate_parse_data_args_t * const t_args = args;

    for (;;) {
        const json_deflate_parse_data_result_t data_result = json_deflate_parse_data(t_args->schema, t_args->data, t_args->buffer, t_args->target, t_args->expected_size, t_args->schema_hash);

        // If we couldn't allocate memory to complete this deflate, wait until another one finishes and try again
        if (data_result.do_retry == true) {
            json_deflate_queue_wait();
        } else {
            json_deflate_queue_continue();

            t_args->status = (int32_t)data_result.result.status;
            t_args->offset = data_result.result.offset;
            t_args->end = data_result.result.end;
            break;
        }
    }
}

static void json_deflate_parse_data_async_complete_adapter(void * args, thread_pool_t * const pool) {
    json_deflate_parse_data_args_t * const t_args = args;
    ASSERT(t_args->status == json_deflate_parse_status_success);
    ASSERT(t_args->offset != t_args->end);
    app_run_callback_once_iii(t_args->on_deflate_complete, t_args->status, t_args->offset, t_args->end);
    json_deflate_free(args);
}

void json_deflate_parse_data_async(
    const const_mem_region_t schema,
    const const_mem_region_t data,
    const mem_region_t buffer,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    void * const on_complete) {
    json_deflate_parse_data_args_t * const args = json_deflate_calloc(1, sizeof(json_deflate_parse_data_args_t));
    args->schema = schema;
    args->data = data;
    args->buffer = buffer;
    args->target = target;
    args->on_deflate_complete = on_complete;
    args->offset = 0;
    args->expected_size = expected_size;
    args->schema_hash = schema_hash;
    thread_pool_enqueue(json_deflate_get_pool(), json_deflate_parse_data_async_adapter, json_deflate_parse_data_async_complete_adapter, args);
}

// ========================================

static void json_deflate_parse_data_http_async_complete_adapter(void * args, thread_pool_t * const pool) {
    json_deflate_parse_data_args_t * const t_args = args;
    app_run_callback_once_iiii(t_args->on_deflate_complete, t_args->status, (int)adk_curl_result_ok, t_args->offset, t_args->end);
    json_deflate_free(args);
}

static void json_deflate_parse_http_async_adapter(void * const args) {
    json_deflate_parse_http_args_t * const t_args = args;
    const adk_curl_result_e http_result = adk_curl_async_get_result(t_args->http);
    if (http_result == adk_curl_result_ok) {
        const const_mem_region_t data = adk_curl_get_http_body(t_args->http);

        json_deflate_parse_data_args_t * const u_args = json_deflate_calloc(1, sizeof(json_deflate_parse_data_args_t));
        u_args->schema = t_args->schema;
        u_args->buffer = t_args->buffer;
        u_args->data = data;
        u_args->expected_size = t_args->expected_size;
        u_args->on_deflate_complete = t_args->on_deflate_complete;
        u_args->schema_hash = t_args->schema_hash;
        u_args->target = t_args->target;

        json_deflate_free(t_args);

        thread_pool_enqueue(json_deflate_get_pool(), json_deflate_parse_data_async_adapter, json_deflate_parse_data_http_async_complete_adapter, u_args);
    } else {
        app_run_callback_once_iiii(t_args->on_deflate_complete, json_deflate_parse_status_external_error, http_result, 0, 0);
    }
}

void json_deflate_parse_http_async(
    const const_mem_region_t schema,
    adk_curl_handle_t * const http,
    const mem_region_t buffer,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    void * const on_deflate_complete) {
    ASSERT_MSG(
        adk_curl_get_buffering_mode(http) & adk_curl_handle_buffer_http_body,
        "[json_deflate] BUFFER_HTTP_BODY must be set in order to deserialize directly from an HTTP request");

    const adk_curl_result_e http_result = adk_curl_async_get_result(http);
    if (http_result == adk_curl_result_ok || http_result == adk_curl_result_busy) {
        json_deflate_parse_http_args_t * const args = json_deflate_calloc(1, sizeof(json_deflate_parse_http_args_t));
        args->schema = schema;
        args->http = http;
        args->buffer = buffer;
        args->target = target;
        args->on_deflate_complete = on_deflate_complete;
        args->expected_size = expected_size;
        args->schema_hash = schema_hash;

        if (http_result == adk_curl_result_ok) {
            json_deflate_parse_http_async_adapter(args);
        } else {
            adk_curl_set_json_deflate_callback(http, json_deflate_parse_http_async_adapter, args);
        }
    } else {
        app_run_callback_once_iiii(on_deflate_complete, json_deflate_parse_status_external_error, http_result, 0, 0);
    }
}

// ========================================

typedef struct json_deflate_parse_httpx_args_t {
    json_deflate_http_future_t * future;
    const_mem_region_t schema;
    mem_region_t buffer;
    json_deflate_parse_target_e target;
    uint32_t expected_size;
    uint32_t schema_hash;
} json_deflate_parse_httpx_args_t;

struct json_deflate_http_future_t {
    adk_future_status_e status;
    adk_httpx_response_t * response;
    json_deflate_parse_result_t result;
};

static void json_deflate_parse_data_httpx_async_complete_adapter(void * userdata, thread_pool_t * const pool) {
    json_deflate_parse_httpx_args_t * const args = userdata;
    ASSERT(args->future->result.status == json_deflate_parse_status_success);
    ASSERT(args->future->result.offset != args->future->result.end);

    args->future->status = adk_future_status_ready;

    json_deflate_free(args);
}

static void json_deflate_parse_httpx_async_adapter(void * const userdata, thread_pool_t * const pool) {
    json_deflate_parse_httpx_args_t * const args = userdata;

    for (;;) {
        const json_deflate_parse_data_result_t data_result = json_deflate_parse_data(
            args->schema, adk_httpx_response_get_body(args->future->response), args->buffer, args->target, args->expected_size, args->schema_hash);

        if (data_result.do_retry == true) {
            json_deflate_queue_wait();
        } else {
            json_deflate_queue_continue();

            args->future->result = data_result.result;
            break;
        }
    }
}

static void httpx_on_complete(adk_httpx_response_t * const response, void * userdata) {
    json_deflate_parse_httpx_args_t * const args = (json_deflate_parse_httpx_args_t *)userdata;

    args->future->response = response;
    thread_pool_enqueue(json_deflate_get_pool(), json_deflate_parse_httpx_async_adapter, json_deflate_parse_data_httpx_async_complete_adapter, args);
}

static json_deflate_parse_httpx_args_t * json_deflate_parse_httpx_args(
    const uint8_t * schema_layout,
    const size_t schema_layout_size,
    const uint8_t * buffer,
    const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    json_deflate_http_future_t * const future) {
    json_deflate_parse_httpx_args_t * const args = (json_deflate_parse_httpx_args_t *)json_deflate_malloc(sizeof(json_deflate_parse_httpx_args_t));
    ZEROMEM(args);

    args->future = future;
    args->schema = CONST_MEM_REGION(.byte_ptr = schema_layout, .size = schema_layout_size);
    args->buffer = MEM_REGION(.byte_ptr = buffer, .size = buffer_size);
    args->target = target;
    args->expected_size = expected_size;
    args->schema_hash = schema_hash;

    return args;
}

json_deflate_http_future_t * json_deflate_parse_httpx_async(
    const uint8_t * const schema_layout,
    const size_t schema_layout_size,
    adk_httpx_request_t * const request,
    const uint8_t * const buffer,
    const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash) {
    json_deflate_http_future_t * const future = (json_deflate_http_future_t *)json_deflate_malloc(sizeof(json_deflate_http_future_t));
    ZEROMEM(future);
    future->status = adk_future_status_pending;

    json_deflate_parse_httpx_args_t * const args = json_deflate_parse_httpx_args(
        schema_layout,
        schema_layout_size,
        buffer,
        buffer_size,
        target,
        expected_size,
        schema_hash,
        future);

    adk_httpx_request_set_on_complete(request, httpx_on_complete);
    adk_httpx_request_set_userdata(request, args);
    adk_httpx_send(request);

    return future;
}

json_deflate_http_future_t * json_deflate_parse_httpx_response_async(
    const uint8_t * const schema_layout,
    const size_t schema_layout_size,
    adk_httpx_response_t * const response,
    const uint8_t * const buffer,
    const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash) {
    json_deflate_http_future_t * const future = (json_deflate_http_future_t *)json_deflate_malloc(sizeof(json_deflate_http_future_t));
    ZEROMEM(future);
    future->status = adk_future_status_pending;
    future->response = response;

    json_deflate_parse_httpx_args_t * const args = (json_deflate_parse_httpx_args_t *)json_deflate_malloc(sizeof(json_deflate_parse_httpx_args_t));
    ZEROMEM(args);

    args->future = future;
    args->schema = CONST_MEM_REGION(.byte_ptr = schema_layout, .size = schema_layout_size);
    args->buffer = MEM_REGION(.byte_ptr = buffer, .size = buffer_size);
    args->target = target;
    args->expected_size = expected_size;
    args->schema_hash = schema_hash;

    thread_pool_enqueue(json_deflate_get_pool(), json_deflate_parse_httpx_async_adapter, json_deflate_parse_data_httpx_async_complete_adapter, args);

    return future;
}

void json_deflate_parse_httpx_resize(
    json_deflate_http_future_t * const future,
    const uint8_t * const schema_layout,
    const size_t schema_layout_size,
    const uint8_t * const buffer,
    const size_t buffer_size,
    const json_deflate_parse_target_e target,
    const uint32_t expected_size,
    const uint32_t schema_hash) {
    ASSERT(adk_httpx_response_get_status(future->response) == adk_future_status_ready);

    json_deflate_parse_httpx_args_t * const args = json_deflate_parse_httpx_args(
        schema_layout,
        schema_layout_size,
        buffer,
        buffer_size,
        target,
        expected_size,
        schema_hash,
        future);

    // HTTP response has already completed, so execute completion handler directly
    httpx_on_complete(future->response, args);
}

adk_future_status_e json_deflate_http_future_get_status(const json_deflate_http_future_t * const future) {
    return future->status;
}

adk_httpx_response_t * json_deflate_http_future_get_response(const json_deflate_http_future_t * const future) {
    return future->response;
}

json_deflate_parse_result_t json_deflate_http_future_get_result(const json_deflate_http_future_t * const future) {
    return future->result;
}

void json_deflate_http_future_drop(json_deflate_http_future_t * const future) {
    adk_httpx_response_free(future->response);
    json_deflate_free(future);
}

// ========================================

void json_deflate_async(
    const uint8_t * const schema_layout,
    const uint32_t schema_layout_length,
    const uint8_t * const json_data,
    uint32_t json_data_length,
    uint8_t * const buffer,
    const uint32_t buffer_size,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    const json_deflate_parse_target_e target,
    void * const on_complete) {
    const_mem_region_t layout_region = {0};
    layout_region.byte_ptr = schema_layout;
    layout_region.size = (size_t)schema_layout_length;

    const_mem_region_t json_region = {0};
    json_region.byte_ptr = json_data;
    json_region.size = (size_t)json_data_length;

    mem_region_t output_region = {0};
    output_region.byte_ptr = buffer;
    output_region.size = (size_t)buffer_size;

    json_deflate_parse_data_async(
        layout_region,
        json_region,
        output_region,
        target,
        expected_size,
        schema_hash,
        on_complete);
}

void json_deflate_native_async(
    const uint8_t * const schema_layout,
    const uint32_t schema_layout_length,
    const uint8_t * const json_data,
    uint32_t json_data_length,
    uint8_t * const buffer,
    const uint32_t buffer_size,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    const json_deflate_parse_target_e target,
    void * const on_complete) {
    const_mem_region_t layout_region = {0};
    layout_region.byte_ptr = schema_layout;
    layout_region.size = (size_t)schema_layout_length;

    const_mem_region_t json_region = {0};
    json_region.byte_ptr = json_data;
    json_region.size = (size_t)json_data_length;

    mem_region_t output_region = {0};
    output_region.byte_ptr = buffer;
    output_region.size = (size_t)buffer_size;

    json_deflate_parse_data_async(
        layout_region,
        json_region,
        output_region,
        target,
        expected_size,
        schema_hash,
        on_complete);
}

void json_deflate_from_http_async(
    const uint8_t * const schema_layout,
    const uint32_t schema_layout_length,
    adk_curl_handle_t * const http,
    uint8_t * const buffer,
    const uint32_t buffer_size,
    const uint32_t expected_size,
    const uint32_t schema_hash,
    const json_deflate_parse_target_e target,
    void * const on_complete) {
    const_mem_region_t layout_region = {0};
    layout_region.byte_ptr = schema_layout;
    layout_region.size = (size_t)schema_layout_length;

    mem_region_t output_region = {0};
    output_region.byte_ptr = buffer;
    output_region.size = (size_t)buffer_size;

    json_deflate_parse_http_async(
        layout_region,
        http,
        output_region,
        target,
        expected_size,
        schema_hash,
        on_complete);
}
