/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_http2.h"
#include "source/adk/steamboat/sb_platform.h"

static bool ffi_on_http_header_recv(adk_curl_handle_t * const handle, const const_mem_region_t bytes, const adk_curl_callbacks_t * const callbacks) {
    if (callbacks->user[0]) {
        return app_run_callback_vvi(callbacks->user[0], handle, (void *)bytes.ptr, (int)bytes.size) != 0;
    }

    return true;
}

static bool ffi_on_http_recv(adk_curl_handle_t * const handle, const const_mem_region_t bytes, const adk_curl_callbacks_t * const callbacks) {
    if (callbacks->user[1]) {
        return app_run_callback_vvi(callbacks->user[1], handle, (void *)bytes.ptr, (int)bytes.size) != 0;
    }

    return true;
}

static void ffi_http_on_complete(adk_curl_handle_t * const handle, const adk_curl_result_e result, const adk_curl_callbacks_t * const callbacks) {
    // make sure to drop other callbacks.

    if (callbacks->user[0]) {
        app_drop_callback(callbacks->user[0]);
    }
    if (callbacks->user[1]) {
        app_drop_callback(callbacks->user[1]);
    }

    void * const user = callbacks->user[2]; // callback may destroy handle/callbacks structure
    app_run_callback_vi(user, handle, (int)result);
    app_drop_callback(user);
}

void adk_curl_async_perform_raw(
    adk_curl_handle_t * const handle,
    void * const on_http_recv_header_ptr,
    void * const on_http_recv_ptr,
    void * const on_complete_ptr) {
    adk_curl_async_perform(
        handle,
        (adk_curl_callbacks_t){
            .on_http_header_recv = on_http_recv_header_ptr ? ffi_on_http_header_recv : NULL,
            .on_http_recv = on_http_recv_ptr ? ffi_on_http_recv : NULL,
            .on_complete = on_complete_ptr ? ffi_http_on_complete : NULL,
            .user = {
                [0] = on_http_recv_header_ptr,
                [1] = on_http_recv_ptr,
                [2] = on_complete_ptr}});
}

adk_curl_result_e adk_curl_get_info_i32(adk_curl_handle_t * const handle, const adk_curl_info_e info, int32_t * const out) {
    long out_long;
    const adk_curl_result_e result = adk_curl_get_info_long(handle, info, &out_long);
    *out = result == adk_curl_result_ok ? (int32_t)out_long : 0;
    return result;
}

void adk_curl_set_opt_slist(adk_curl_handle_t * const handle, const adk_curl_option_e opt, adk_curl_slist_t * const arg) {
    ASSERT_MSG(
        opt == adk_curl_opt_http_header || opt == adk_curl_opt_quote || opt == adk_curl_opt_pre_quote || opt == adk_curl_opt_http_200_aliases,
        "adk_set_opt_slist() can only be used with [adk_curl_opt_http_header, adk_curl_opt_quote, adk_curl_opt_pre_quote, adk_curl_opt_post_quote, adk_curl_opt_http_200_aliases]");
    adk_curl_set_opt_ptr(handle, opt, arg);
}

static void ffi_websocket_callback_success(
    adk_websocket_handle_t * const ws_handle,
    const struct adk_websocket_callbacks_t * const callbacks) {
    void * const user = callbacks->user[0];
    app_run_callback_once_v(user, ws_handle);

    if (callbacks->user[1]) {
        app_drop_callback(callbacks->user[1]);
    }
}

static void ffi_websocket_callback_error(
    adk_websocket_handle_t * const ws_handle,
    const struct adk_websocket_callbacks_t * const callbacks,
    const adk_websocket_status_e status) {
    void * const user = callbacks->user[1];
    app_run_callback_once_vi(user, ws_handle, status);

    if (callbacks->user[0]) {
        app_drop_callback(callbacks->user[0]);
    }
}

adk_websocket_handle_t * adk_websocket_create_raw(
    const char * const url,
    const char * const supported_protocols,
    adk_http_header_list_t * const header_list,
    void * success_callback,
    void * error_callback) {
    return adk_websocket_create(
        url,
        supported_protocols,
        header_list,
        (adk_websocket_callbacks_t){
            .success = success_callback ? ffi_websocket_callback_success : NULL,
            .error = error_callback ? ffi_websocket_callback_error : NULL,
            .user = {
                [0] = success_callback,
                [1] = error_callback}},
        MALLOC_TAG);
}

adk_websocket_status_e adk_websocket_send_raw(
    adk_websocket_handle_t * const ws_handle,
    void * const ptr,
    const int32_t size,
    const adk_websocket_message_type_e message_type,
    void * const success_callback,
    void * const error_callback) {
    adk_websocket_callbacks_t callbacks = {
        .success = success_callback ? ffi_websocket_callback_success : NULL,
        .error = error_callback ? ffi_websocket_callback_error : NULL,
        .user = {
            [0] = success_callback,
            [1] = error_callback}};

    const adk_websocket_status_e socket_status = adk_websocket_send(ws_handle, CONST_MEM_REGION(.ptr = ptr, .size = size), message_type, callbacks);
    if ((socket_status != adk_websocket_status_connected) && (socket_status != adk_websocket_status_connecting)) {
        if (callbacks.user[0]) {
            app_drop_callback(callbacks.user[0]);
        }
        if (callbacks.user[1]) {
            app_drop_callback(callbacks.user[1]);
        }
    }
    return socket_status;
}

native_slice_t adk_curl_get_http_body_bridge(
    adk_curl_handle_t * const handle) {
    const const_mem_region_t region = adk_curl_get_http_body(handle);
    return (native_slice_t){.ptr = (uint64_t)region.adr, .size = (int32_t)region.size};
}

native_slice_t adk_curl_get_http_header_bridge(
    adk_curl_handle_t * const handle) {
    const const_mem_region_t region = adk_curl_get_http_header(handle);
    return (native_slice_t){.ptr = (uint64_t)region.adr, .size = (int32_t)region.size};
}

adk_websocket_message_type_e adk_websocket_begin_read_bridge(
    adk_websocket_handle_t * const ws_handle,
    native_slice_t * const out_buffer) {
    const_mem_region_t message;

    const adk_websocket_message_type_e msg_type = adk_websocket_begin_read(ws_handle, &message);

    out_buffer->ptr = (uint64_t)message.adr;
    out_buffer->size = (int32_t)message.size;

    return msg_type;
}
