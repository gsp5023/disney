/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/adk_httpx.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

static struct {
    adk_httpx_api_t * api;
    mem_region_t region;
    mem_region_t fragment_buffers_region;
} statics;

enum {
    httpx_test_heap_size = 4 * 1024 * 1024,
    httpx_test_fragment_buffers_size = 4 * 1024 * 1024,
    httpx_test_concurrent_request_count = 16
};

#define HTTP_TEST_TRACE print_message

static void test_http_requests(void ** state) {
    adk_httpx_client_t * const client = adk_httpx_client_create(statics.api);

    adk_httpx_request_t * const get = adk_httpx_client_request(client, adk_httpx_method_get, "https://httpbin.org/get");
    adk_httpx_response_t * const getter = adk_httpx_send(get);

    adk_httpx_request_t * post = adk_httpx_client_request(client, adk_httpx_method_post, "https://httpbin.org/post");
    adk_httpx_request_set_header(post, "foo: bar");
    const char message[] = "Hello, world!";
    adk_httpx_request_set_body(post, (const uint8_t *)message, strlen(message));
    adk_httpx_response_t * const poster = adk_httpx_send(post);

    adk_httpx_response_t * const putter = adk_httpx_send(
        adk_httpx_client_request(client, adk_httpx_method_put, "https://httpbin.org/put"));

    adk_httpx_response_t * const not_found = adk_httpx_send(
        adk_httpx_client_request(client, adk_httpx_method_get, "https://httpbin.org/status/404"));

    while (adk_httpx_client_tick(client)) {
        sb_thread_yield();
    }

    assert_int_equal(adk_httpx_response_get_status(getter), adk_future_status_ready);
    assert_int_equal(adk_httpx_response_get_status(poster), adk_future_status_ready);
    assert_int_equal(adk_httpx_response_get_status(putter), adk_future_status_ready);
    assert_int_equal(adk_httpx_response_get_status(not_found), adk_future_status_ready);

    assert_int_equal(adk_httpx_response_get_response_code(getter), 200);
    assert_int_equal(adk_httpx_response_get_response_code(poster), 200);
    assert_int_equal(adk_httpx_response_get_response_code(putter), 200);
    assert_int_equal(adk_httpx_response_get_response_code(not_found), 404);

    HTTP_TEST_TRACE(
        "getter: %.*s",
        (int)adk_httpx_response_get_body(getter).size,
        adk_httpx_response_get_body(getter).byte_ptr);

    HTTP_TEST_TRACE(
        "poster: %.*s",
        (int)adk_httpx_response_get_body(poster).size,
        adk_httpx_response_get_body(poster).byte_ptr);

    HTTP_TEST_TRACE(
        "putter: %.*s",
        (int)adk_httpx_response_get_body(putter).size,
        adk_httpx_response_get_body(putter).byte_ptr);

    adk_httpx_response_free(getter);
    adk_httpx_response_free(poster);
    adk_httpx_response_free(putter);
    adk_httpx_response_free(not_found);

    adk_httpx_client_free(client);
}

static void test_concurrent_http_requests(void ** state) {
    adk_httpx_client_t * const client = adk_httpx_client_create(statics.api);
    adk_httpx_request_t * get_requests[httpx_test_concurrent_request_count];
    adk_httpx_response_t * get_responses[httpx_test_concurrent_request_count];

    for (size_t i = 0; i < httpx_test_concurrent_request_count; i++) {
        const char url[] = "https://prod-static.disney-plus.net/fed-container-configs/prod/connected/connected/3.0.2/output.json";
        get_requests[i] = adk_httpx_client_request(client, adk_httpx_method_get, url);
        get_responses[i] = adk_httpx_send(get_requests[i]);
    }

    uint64_t epoch = adk_get_milliseconds_since_epoch();
    while (adk_httpx_client_tick(client)) {
        sb_thread_yield();

        if (adk_get_milliseconds_since_epoch() - epoch > 1000) {
            size_t num_ready = 0;
            for (size_t i = 0; i < httpx_test_concurrent_request_count; i++) {
                if (adk_httpx_response_get_status(get_responses[i]) == adk_future_status_ready) {
                    num_ready += 1;
                }
            }

            HTTP_TEST_TRACE("Currently processing httpx request: %zu\n", num_ready);
            epoch = adk_get_milliseconds_since_epoch();
        }
    }

    for (size_t i = 0; i < httpx_test_concurrent_request_count; i++) {
        const char * const error = adk_httpx_response_get_error(get_responses[i]);
        if (error != NULL) {
            HTTP_TEST_TRACE("httpx response error: %s\n", error);
        }

        assert_int_equal(adk_httpx_response_get_result(get_responses[i]), adk_httpx_ok);
        assert_int_equal(adk_httpx_response_get_status(get_responses[i]), adk_future_status_ready);
        assert_int_equal(adk_httpx_response_get_response_code(get_responses[i]), 200);

        adk_httpx_response_free(get_responses[i]);
    }

    adk_httpx_client_free(client);
}

typedef struct callback_state_t {
    size_t num_headers_received;
    size_t body_bytes;
    bool is_completed;
    int64_t response_code;
} callback_state_t;

bool test_on_header(adk_httpx_response_t * const response, const const_mem_region_t header, void * userdata) {
    callback_state_t * const state = (callback_state_t *)userdata;

    state->num_headers_received += 1;

    HTTP_TEST_TRACE("header: %.*s", (int)header.size, header.byte_ptr);

    // Verify that all headers (that aren't the first or last) have (at least one) colon
    if ((state->num_headers_received > 1) && (header.size > 2)) {
        bool has_colon = false;
        for (size_t i = 0; i < header.size; ++i) {
            if (header.byte_ptr[i] == ':') {
                has_colon = true;
                break;
            }
        }

        assert_true(has_colon);
    }

    assert_int_equal(header.byte_ptr[header.size - 1], '\n');
    return true;
}

bool test_on_body(adk_httpx_response_t * const response, const const_mem_region_t body, void * userdata) {
    callback_state_t * const state = (callback_state_t *)userdata;
    state->body_bytes += body.size;

    HTTP_TEST_TRACE("body: %.*s", (int)body.size, body.byte_ptr);
    return true;
}

void test_on_complete(adk_httpx_response_t * const response, void * userdata) {
    callback_state_t * const state = (callback_state_t *)userdata;
    state->response_code = adk_httpx_response_get_response_code(response);
    state->is_completed = true;
}

static void test_http_request_callbacks(void ** _state) {
    adk_httpx_client_t * const client = adk_httpx_client_create(statics.api);

    adk_httpx_request_t * const request = adk_httpx_client_request(
        client, adk_httpx_method_get, "https://httpbin.org/get");

    callback_state_t state = {0};
    adk_httpx_request_set_on_header(request, test_on_header);
    adk_httpx_request_set_on_body(request, test_on_body);
    adk_httpx_request_set_on_complete(request, test_on_complete);
    adk_httpx_request_set_userdata(request, (void *)&state);

    adk_httpx_response_t * const getter = adk_httpx_send(request);

    while (adk_httpx_client_tick(client)) {
        sb_thread_yield();
    }

    assert_true(state.is_completed);
    assert_true(state.num_headers_received > 2);
    assert_int_not_equal(state.body_bytes, 0);
    assert_int_equal(state.response_code, 200);

    // Content for headers and body are NOT buffered when callbacks are provided
    assert_null(adk_httpx_response_get_headers(getter).ptr);
    assert_null(adk_httpx_response_get_body(getter).ptr);

    adk_httpx_response_free(getter);
    adk_httpx_client_free(client);
}

static int setup(void ** state) {
    statics.region = MEM_REGION(malloc(httpx_test_heap_size), httpx_test_heap_size);
    TRAP_OUT_OF_MEMORY(statics.region.ptr);

    statics.fragment_buffers_region = MEM_REGION(malloc(httpx_test_fragment_buffers_size), httpx_test_fragment_buffers_size);
    TRAP_OUT_OF_MEMORY(statics.fragment_buffers_region.ptr);

    statics.api = adk_httpx_api_create(
        statics.region,
        statics.fragment_buffers_region,
        fragment_size,
        unit_test_guard_page_mode,
        adk_httpx_init_normal,
        "tests-httpx");

    return 0;
}

static int teardown(void ** state) {
    adk_httpx_api_free(statics.api);
    free(statics.region.ptr);
    free(statics.fragment_buffers_region.ptr);

    return 0;
}

int test_httpx() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_http_requests),
        cmocka_unit_test(test_http_request_callbacks),
        cmocka_unit_test(test_concurrent_http_requests),
    };

    return cmocka_run_group_tests(tests, setup, teardown);
}
