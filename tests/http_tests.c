/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
http_tests.c

http test fixture
 */

#include "source/adk/http/adk_http.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

static const char c_user_agent[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.183 Safari/537.36";

#define HTTP_TEST_TRACE print_message

#if defined(_LEIA) || defined(_VADER)
void adk_curl_api_set_mode_test(bool use_httpx);
#endif

static struct {
    mem_region_t region;
    mem_region_t fragment_buffers_region;
    size_t num_requests_completed;
    adk_curl_handle_buffer_mode_e buffer_mode;
    bool user_aborted_callback;
    adk_curl_slist_t * headers;
} statics;

static bool test_on_http_header_recv(adk_curl_handle_t * const handle, const const_mem_region_t data, const adk_curl_callbacks_t * const callbacks) {
    HTTP_TEST_TRACE(
        ">> test_on_http_header_recv url: [%s] [%8zu] bytes:\n%.*s\n",
        (const char *)callbacks->user[0],
        data.size,
        (int)data.size,
        (const char *)data.ptr);

    return true;
}

static bool test_on_http_recv(adk_curl_handle_t * const handle, const const_mem_region_t data, const adk_curl_callbacks_t * const callbacks) {
    HTTP_TEST_TRACE(
        ">> test_on_http_recv url: [%s] [%8zu] bytes:\n%.*s\n",
        (const char *)callbacks->user[0],
        data.size,
        (int)data.size,
        (const char *)data.ptr);

    return true;
}

static void test_on_http_complete(adk_curl_handle_t * const handle, const adk_curl_result_e result, const struct adk_curl_callbacks_t * const callbacks) {
    HTTP_TEST_TRACE(">> test_on_http_complete url: [%s], result: %d\n", (const char *)callbacks->user[0], (int)result);
    assert_true(result == adk_curl_result_ok);

    const const_mem_region_t header_data = adk_curl_get_http_header(handle);
    const const_mem_region_t body_data = adk_curl_get_http_body(handle);
    const size_t buffer_length = header_data.size + body_data.size;

    long response_code = 0;
    assert_int_equal(adk_curl_get_info_long(handle, adk_curl_info_response_code, &response_code), adk_curl_result_ok);
    assert_int_equal(response_code, 200);

    HTTP_TEST_TRACE(
        ">> test_on_http_complete(%ld) [%8zu] bytes:\nheader:\n%.*s\nbody:\n%.*s\n",
        response_code,
        buffer_length,
        (int)header_data.size,
        (const char *)header_data.ptr,
        (int)body_data.size,
        (const char *)body_data.ptr);

    // If the body of the response from httpbin contains the string "header",
    // we can be reasonably certain we didn't get back garbage.
    if ((statics.buffer_mode & adk_curl_handle_buffer_http_body) == adk_curl_handle_buffer_http_body) {
        assert_non_null(body_data.ptr);
        assert_non_null(strstr((const char *)body_data.ptr, "header"));
    }

    adk_curl_close_handle(handle);

    statics.num_requests_completed += 1;
}

static bool test_callback_ret_false(adk_curl_handle_t * const handle, const const_mem_region_t data, const adk_curl_callbacks_t * const callbacks) {
    assert_false(statics.user_aborted_callback);
    statics.user_aborted_callback = true;
    return false;
}

static void test_callback_ret_false_on_complete(adk_curl_handle_t * const handle, const adk_curl_result_e result, const struct adk_curl_callbacks_t * const callbacks) {
    assert_true(result == adk_curl_result_write_error);
    adk_curl_close_handle(handle);
}

static adk_curl_handle_t * open_handle(const char * const url) {
    adk_curl_handle_t * const handle = adk_curl_open_handle();

    adk_curl_set_opt_ptr(handle, adk_curl_opt_url, (void *)url);
    adk_curl_set_opt_long(handle, adk_curl_opt_follow_location, 1);
    adk_curl_set_opt_ptr(handle, adk_curl_opt_user_agent, c_user_agent);
    adk_curl_set_opt_ptr(handle, adk_curl_opt_http_header, statics.headers);

    adk_curl_set_buffering_mode(handle, statics.buffer_mode);

    return handle;
}

static void perform(adk_curl_handle_t * const handle, const char * const url) {
    const adk_curl_callbacks_t callbacks = {
        .on_http_header_recv = test_on_http_header_recv,
        .on_http_recv = test_on_http_recv,
        .on_complete = test_on_http_complete,
        .user = {(void *)url}};

    adk_curl_async_perform(handle, callbacks);
}

static void poll() {
    while (adk_curl_run_callbacks()) {
        sb_thread_yield();
    }
}

static void test_http_get(void ** state) {
    statics.num_requests_completed = 0;

    const char url[] = "https://httpbin.org/get";
    adk_curl_handle_t * const handle = open_handle(url);
    perform(handle, url);

    poll();

    assert_int_equal(statics.num_requests_completed, 1);
}

static void test_gzip_encoding(void ** state) {
    statics.num_requests_completed = 0;

    const char url[] = "https://httpbin.org/get";
    adk_curl_handle_t * const handle = open_handle(url);

    const char * const encoding = "gzip";
    adk_curl_set_opt_ptr(handle, adk_curl_opt_accept_encoding, (void *)encoding);

    perform(handle, url);

    poll();

    assert_int_equal(statics.num_requests_completed, 1);
}

static void test_http_post(void ** state) {
    statics.num_requests_completed = 0;

    const char url[] = "https://httpbin.org/post";

    {
        adk_curl_handle_t * const handle = open_handle(url);
        adk_curl_set_opt_long(handle, adk_curl_opt_post, 1);
        adk_curl_set_opt_long(handle, adk_curl_opt_post_field_size, 0);
        perform(handle, url);
    }

    {
        adk_curl_handle_t * const handle = open_handle(url);

        static const char message_body[] = "Hello, POST!";
        adk_curl_set_opt_long(handle, adk_curl_opt_post, 1);
        adk_curl_set_opt_ptr(handle, adk_curl_opt_post_fields, (void * const)message_body);
        adk_curl_set_opt_long(handle, adk_curl_opt_post_field_size, sizeof(message_body) - 1);

        perform(handle, url);
    }

    poll();

    assert_int_equal(statics.num_requests_completed, 2);
}

static void test_http_put(void ** state) {
    statics.num_requests_completed = 0;

    const char url[] = "https://httpbin.org/put";

    {
        adk_curl_handle_t * const handle = open_handle(url);
        adk_curl_set_opt_ptr(handle, adk_curl_opt_custom_request, (void *)"PUT");
        perform(handle, url);
    }

    poll();

    assert_int_equal(statics.num_requests_completed, 1);
}

static int group_setup(void ** state) {
    statics.region = MEM_REGION(.ptr = malloc(2 * 1024 * 1024), .size = 2 * 1024 * 1024);
    statics.fragment_buffers_region = MEM_REGION(.ptr = malloc(4 * 1024 * 1024), .size = 4 * 1024 * 1024);
    TRAP_OUT_OF_MEMORY(statics.region.ptr);
    TRAP_OUT_OF_MEMORY(statics.fragment_buffers_region.ptr);

    VERIFY(adk_curl_api_init(statics.region, statics.fragment_buffers_region, fragment_size, unit_test_guard_page_mode, adk_curl_http_init_normal));

    statics.headers = adk_curl_slist_append(NULL, "Content-Type: application/json");
    statics.headers = adk_curl_slist_append(statics.headers, "X-Powered-By: coffee");

    return 0;
}

static int group_teardown(void ** state) {
    adk_curl_slist_free_all(statics.headers);

    adk_curl_api_shutdown();

    free(statics.region.ptr);

    return 0;
}

static int set_buffered(void ** state) {
    (void)state;
    statics.buffer_mode = adk_curl_handle_buffer_http_header | adk_curl_handle_buffer_http_body;
    return 0;
}

static int set_unbuffered(void ** state) {
    (void)state;
    statics.buffer_mode = adk_curl_handle_buffer_off;
    return 0;
}

int test_http() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(test_http_get, set_buffered),
        cmocka_unit_test_setup(test_http_get, set_unbuffered),
        cmocka_unit_test_setup(test_gzip_encoding, set_buffered),
        cmocka_unit_test_setup(test_gzip_encoding, set_unbuffered),
        cmocka_unit_test_setup(test_http_post, set_buffered),
        cmocka_unit_test_setup(test_http_post, set_unbuffered),
        cmocka_unit_test_setup(test_http_put, set_buffered),
        cmocka_unit_test_setup(test_http_put, set_unbuffered),
    };

    int result = cmocka_run_group_tests(tests, group_setup, group_teardown);

#ifdef _CONSOLE_NATIVE
    adk_curl_api_set_mode_test(true);
    result += cmocka_run_group_tests(tests, group_setup, group_teardown);
    adk_curl_api_set_mode_test(false);
#endif

    return result;
}
