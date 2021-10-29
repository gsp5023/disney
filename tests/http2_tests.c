/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 http2_tests.c

 http2 test fixture
*/

#include "source/adk/http/adk_http2.h"
#include "source/adk/http/websockets/adk_websocket_backend_selector.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

static const char test_message[] = "websocket test message";

static struct {
    struct {
        bool text_websocket_received_message;
        bool invalid_url_failed;

        mem_region_t ws_test_message;

        bool url_with_path_connected;
        bool url_with_path_has_read;
        bool send_multiple_messages_and_close;
    } test;

    bool use_proxy;
    const char * proxy_address;
    adk_websocket_backend_e backend;
} statics;

typedef struct ws_test_state_t {
    mem_region_t region;
} ws_test_state_t;

static int setup(void ** s) {
    ws_test_state_t * state = malloc(sizeof(ws_test_state_t));
    ZEROMEM(&statics.test);
    const mem_region_t region = {
        {{{.ptr = malloc(2 * 1024 * 1024)},
          .size = 2 * 1024 * 1024}}};
    TRAP_OUT_OF_MEMORY(region.ptr);

    const websocket_config_t websocket_config = (websocket_config_t){
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 4 * 1024,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .maximum_redirects = 10,
    };
    adk_http_init(adk_http_init_default_cert_path, region, statics.backend, websocket_config, unit_test_guard_page_mode, MALLOC_TAG);

    // pass in --http2_use_proxy to the tests to run this and run charles to demo its ability to proxy
    if (statics.use_proxy) {
        adk_http_set_proxy(statics.proxy_address ? statics.proxy_address : "127.0.0.1:8888");
    }

    state->region = region;
    *s = state;

    return 0;
}

static int teardown(void ** s) {
    ws_test_state_t * state = *s;

    adk_http_shutdown(MALLOC_TAG);
    free(state->region.ptr);
    free(state);

    return 0;
}

static void ws_message_write_success(adk_websocket_handle_t * ws_handle, const adk_websocket_callbacks_t * const callbacks) {
    // if you have stack allocated message, you should delete it here.
    // but we're only working with statically allocated data, so nothing needs to be freed.
    print_message("Websocket message sent. pointer: [%p] bytes: [%i]\n", test_message, (int)strlen(test_message));
}

static void ws_read_message(adk_websocket_handle_t * const ws_handle) {
    const_mem_region_t msg_region;
    const adk_websocket_message_type_e msg_type = adk_websocket_begin_read(ws_handle, &msg_region);
    if (msg_type <= adk_websocket_message_none) {
        return;
    }

    statics.test.text_websocket_received_message = true;
    assert_non_null(ws_handle);
    assert_int_equal(msg_type, adk_websocket_message_text);

    const char * const text_received = (char *)msg_region.ptr;
    print_message("Websocket [%p] on_message: received: [%.*s]\n", ws_handle, (int)msg_region.size, text_received);
    assert_int_equal(strncmp(text_received, test_message, msg_region.size), 0);
    adk_websocket_end_read(ws_handle, MALLOC_TAG);
    adk_websocket_close(ws_handle, MALLOC_TAG);
}

static void ws_on_open_success(adk_websocket_handle_t * ws_handle, const adk_websocket_callbacks_t * const callbacks) {
    assert_non_null(ws_handle);
    statics.test.ws_test_message.ptr = (void *)test_message;
    statics.test.ws_test_message.size = strlen(test_message);
    print_message("Websocket [%p] on_open: sending message: [%.*s]\n", ws_handle, (int)statics.test.ws_test_message.size, (char *)statics.test.ws_test_message.ptr);

    const adk_websocket_callbacks_t ws_write_callback = {.success = ws_message_write_success, .error = NULL, .user = NULL};
    adk_websocket_send(ws_handle, statics.test.ws_test_message.consted, adk_websocket_message_text, ws_write_callback);
}

static void ws_on_open_malformed_error(adk_websocket_handle_t * ws_handle, const adk_websocket_callbacks_t * const callbacks, const adk_websocket_status_e status) {
    adk_websocket_close(ws_handle, MALLOC_TAG);
    if (status == adk_websocket_status_connection_failed) {
        statics.test.invalid_url_failed = true;
    }
}

static void test_websocket_echo(void ** state) {
    adk_websocket_callbacks_t ws_callbacks = {
        .success = ws_on_open_success,
        .error = NULL};

    adk_http_header_list_t * const valid_handle_header_list = adk_http_append_header_list(NULL, "header-name-with-a-very-long-id:", "value", MALLOC_TAG);

    adk_websocket_handle_t * const valid_ws_handle = adk_websocket_create(
        "wss://libwebsockets.org",
        "lws-mirror-protocol",
        valid_handle_header_list,
        ws_callbacks,
        MALLOC_TAG);

    char long_header_for_allocation_checking[400] = {0};
    const size_t header_size = sizeof(long_header_for_allocation_checking) - 1;
    for (size_t i = 0; i < (header_size - 1); ++i) {
        long_header_for_allocation_checking[i] = 'c';
    }
    long_header_for_allocation_checking[header_size - 1] = ':';

    adk_websocket_callbacks_t ws_callbacks_malformed_url = {
        .success = NULL,
        .error = ws_on_open_malformed_error};

    // this call will deliberately induce warnings. its mostly a check to see if we complete at all.
    // if this test fails to finish in a sane time frame (<20s) then blame this line being broken.
    adk_websocket_handle_t * const invalid_ws_handle = adk_websocket_create(
        "wss://libwebsocfasdfgafgasdf34rasrgadfgvkets;org",
        "lws-mirror-protocol",
        NULL,
        ws_callbacks_malformed_url,
        MALLOC_TAG);

    while (adk_http_tick()) {
        if (!statics.test.text_websocket_received_message) {
            const adk_websocket_status_e ws_status = adk_websocket_get_status(valid_ws_handle);
            assert_true((ws_status == adk_websocket_status_connected) || (ws_status == adk_websocket_status_connecting));
            ws_read_message(valid_ws_handle);
        }

        // force override/check in case somehow we get black holed and we don't actually get an error on connecting (e.g. if running charles)
        if (!statics.test.invalid_url_failed) {
            const adk_websocket_status_e ws_status = adk_websocket_get_status(invalid_ws_handle);
            if (ws_status != adk_websocket_status_connected && ws_status != adk_websocket_status_connecting) {
                adk_websocket_close(invalid_ws_handle, MALLOC_TAG);
                statics.test.invalid_url_failed = true;
            }
        }

        sb_thread_sleep((milliseconds_t){1});
    }

    assert_true(statics.test.invalid_url_failed);
    assert_true(statics.test.text_websocket_received_message);
}

static void test_ws_long_header_name(void ** state) {
    char long_header_for_allocation_checking[400] = {0};
    const size_t header_size = sizeof(long_header_for_allocation_checking) - 1;
    for (size_t i = 0; i < (header_size - 1); ++i) {
        long_header_for_allocation_checking[i] = 'c';
    }
    long_header_for_allocation_checking[header_size - 1] = ':';

    adk_http_header_list_t * const long_header_list = adk_http_append_header_list(NULL, long_header_for_allocation_checking, "asd", MALLOC_TAG);

    adk_websocket_handle_t * const ws_handle_with_long_header_name = adk_websocket_create(
        "wss://libwebsockets.org:443",
        "lws-mirror-protocol",
        long_header_list,
        (adk_websocket_callbacks_t){0},
        MALLOC_TAG);

    bool long_header_name_connected = false;

    while (adk_http_tick()) {
        if (!long_header_name_connected) {
            const adk_websocket_status_e ws_status = adk_websocket_get_status(ws_handle_with_long_header_name);
            if (ws_status == adk_websocket_status_connected) {
                long_header_name_connected = true;
                adk_websocket_close(ws_handle_with_long_header_name, MALLOC_TAG);
            } else if (ws_status != adk_websocket_status_connecting) {
                adk_websocket_close(ws_handle_with_long_header_name, MALLOC_TAG);
            }
        }

        sb_thread_sleep((milliseconds_t){1});
    }

    assert_true(long_header_name_connected);
}

static void ws_with_path_read_message(adk_websocket_handle_t * const ws_handle) {
    const_mem_region_t msg_region;
    const adk_websocket_message_type_e msg_type = adk_websocket_begin_read(ws_handle, &msg_region);
    assert_int_not_equal(msg_type, adk_websocket_message_error);
    if (msg_type == adk_websocket_message_none) {
        return;
    }

    assert_non_null(ws_handle);
    assert_int_equal(msg_type, adk_websocket_message_text);

    const char * const text_received = (char *)msg_region.ptr;
    print_message("Websocket [%p] on_message: received: (%i) \n%.*s\n", ws_handle, (int)msg_region.size, (int)msg_region.size, text_received);

    statics.test.url_with_path_has_read = true;

    adk_websocket_end_read(ws_handle, MALLOC_TAG);
    adk_websocket_close(ws_handle, MALLOC_TAG);
}

static void ws_send_success(
    adk_websocket_handle_t * const ws_handle,
    const adk_websocket_callbacks_t * const callbacks) {
    print_message("websocket [%p] ws_send_success\n", ws_handle);
}

static void ws_send_error(
    adk_websocket_handle_t * ws_handle,
    const adk_websocket_callbacks_t * const callbacks,
    const adk_websocket_status_e status) {
    print_message("websocket [%p] ws_send_error\n", ws_handle);
}

static void test_ws_bamgrid_json(void ** state) {
    const char * const url = "wss://qa.global.edge.bamgrid.com:443/c800b6/connection?X-BAMSDK-Platform=rust%2Fm5%2Fcanalplus-orange&X-DSS-Edge-Accept=vnd.dss.edge%2Bjson%3B%20version%3D0&X-BAMSDK-Client-ID=disney-svod-3d9324fc&X-BAMSDK-Version=0.3";
    char * protocols = "vnd.dss.edge+json";

    adk_websocket_handle_t * const ws_handle = adk_websocket_create(
        url,
        protocols,
        NULL,
        (adk_websocket_callbacks_t){0},
        MALLOC_TAG);

    const char message_payload[] = "{\"id\":\"fcc35b13-c016-2b46-8f80-97ce3944e02d\",\"type\":\"urn:dss:event:edge:sdk:authentication\",\"source\":\"urn:dss:source:sdk:rust:m5:canalplus-orange\",\"time\":\"2020-07-15T18:47:59.129Z\",\"istest\":null,\"data\":{\"accessToken\":\"eyJraWQiOiI5NGE1ZTUyYy1hY2IyLTQ2NDEtYTViYy04YTYxYWVmNDZmZmEiLCJhbGciOiJFZERTQSJ9.eyJzdWIiOiI2ODk1YjY0OS0yMTNlLTRlOTAtYTY0OS1hNWQwYzg2OGRjMzgiLCJuYmYiOjE1OTQ4Mzg4NzgsInBhcnRuZXJOYW1lIjoiZGlzbmV5IiwiaXNzIjoidXJuOmJhbXRlY2g6c2VydmljZTp0b2tlbiIsImNvbnRleHQiOnsiYWN0aXZlX3Byb2ZpbGVfaWQiOiI2MDQ5NjNhYi04YzJjLTQ5OGMtYmE2OC0yMGQ1ZWMwYjgzOGUiLCJ1cGRhdGVkX29uIjoiMjAyMC0wNy0xNVQxODo0Nzo1OC45MDkrMDAwMCIsInN1YnNjcmlwdGlvbnMiOltdLCJjb3VudHJ5X3NldHRpbmdzIjp7ImNvZGUiOiJVUyIsInRpbWV6b25lIjp7InV0Y19vZmZzZXQiOiItMDQ6MDAiLCJuYW1lIjoiQW1lcmljYVwvTmV3X1lvcmsifX0sImV4cGlyZXNfb24iOiIyMDIwLTA3LTE1VDE4OjU3OjU4LjkwOSswMDAwIiwiZXhwZXJpbWVudHMiOnt9LCJwcm9maWxlcyI6W3sia2lkc19tb2RlX2VuYWJsZWQiOmZhbHNlLCJwbGF5YmFja19zZXR0aW5ncyI6eyJwcmVmZXJfMTMzIjpmYWxzZX0sImFjdGl2ZSI6dHJ1ZSwiaWQiOiI2MDQ5NjNhYi04YzJjLTQ5OGMtYmE2OC0yMGQ1ZWMwYjgzOGUiLCJhdmF0YXIiOnsiaWQiOiI0NDJhZjdkYi04NWY3LTVlMWQtOTZmMC1iMmM1MTdiZTQwODUifSwidHlwZSI6InVybjpiYW10ZWNoOnByb2ZpbGUiLCJsYW5ndWFnZV9wcmVmZXJlbmNlcyI6eyJhcHBfbGFuZ3VhZ2UiOiJlbi1HQiIsInBsYXliYWNrX2xhbmd1YWdlIjoiZW4tR0IiLCJzdWJ0aXRsZV9sYW5ndWFnZSI6ImVuLUdCIn19XSwiaXBfYWRkcmVzcyI6IjczLjE4MC4xLjE4OSIsInR5cGUiOiJBTk9OWU1PVVMiLCJ2ZXJzaW9uIjoiVjIuMC4wIiwiYmxhY2tvdXRzIjp7ImVudGl0bGVtZW50cyI6W10sImRhdGEiOnt9LCJydWxlcyI6eyJ2aW9sYXRlZCI6W119fSwicGFydG5lciI6eyJuYW1lIjoiZGlzbmV5In0sImxvY2F0aW9uIjp7ImNvdW50cnlfY29kZSI6IlVTIiwiY2l0eV9uYW1lIjoiZXVnZW5lIiwic3RhdGVfbmFtZSI6Im9yZWdvbiIsImRtYSI6ODAxLCJyZWdpb25fbmFtZSI6InBhY2lmaWMgbm9ydGh3ZXN0IiwidHlwZSI6IlpJUF9DT0RFIiwiYXNuIjo3OTIyLCJ6aXBfY29kZSI6Ijk3NDAxIn0sImdlbmVyYXRlZF9vbiI6IjIwMjAtMDctMTVUMTg6NDc6NTguOTA5KzAwMDAiLCJpZCI6ImI0MDRiNmQwLWM2Y2ItMTFlYS1hNmUzLTAyNDJhYzExMDAxMCIsIm1lZGlhX3Blcm1pc3Npb25zIjp7ImVudGl0bGVtZW50cyI6W10sImRhdGEiOnt9LCJydWxlcyI6eyJwYXNzZWQiOltdfX0sImRldmljZSI6eyJhcHBfcnVudGltZSI6Im01IiwicHJvZmlsZSI6ImNhbmFscGx1cy1vcmFuZ2UiLCJpZCI6IjY4OTViNjQ5LTIxM2UtNGU5MC1hNjQ5LWE1ZDBjODY4ZGMzOCIsInR5cGUiOiJ1cm46ZHNzOmRldmljZTppbnRlcm5hbCIsImZhbWlseSI6InJ1c3QiLCJwbGF0Zm9ybSI6ImNhbmFscGx1cy1vcmFuZ2UifSwic3VwcG9ydGVkIjp0cnVlfSwiZW52IjoicWEiLCJleHAiOjE1OTQ4Mzk0NzgsImlhdCI6MTU5NDgzODg3OCwianRpIjoiZmIzYWVlMDktOTZjYS00YmQ0LWE2ODMtNDcyMDQ5ODJlNGYxIn0.o20mwRFMxu_jzcp9PM2GJ0LI_579JUnZa0Y4Ue33ue42bxm4m54aFe7Evf38r1JqMm_NbafQoS27bfiMkkbIDA\"},\"specversion\":\"0.3\",\"envelopeversion\":null,\"schemaurl\":\"https://github.bamtech.co/schema-registry/schema-registry/blob/master/dss/event/edge/1.0.0/sdk/authentication.oas2.yaml\",\"datacontenttype\":\"application/json;charset=utf-8\",\"datacontentencoding\":null,\"subject\":\"sessionId=b404b6d0-c6cb-11ea-a6e3-0242ac110010\",\"partner\":null}";
    adk_websocket_send(
        ws_handle,
        CONST_MEM_REGION(.ptr = message_payload, .size = strlen(message_payload)),
        adk_websocket_message_text,
        (adk_websocket_callbacks_t){.success = ws_send_success, .error = ws_send_error});

    // NOTE: the expected response to the above message is 'access-token.invalid - auth.expired'

    while (adk_http_tick()) {
        if (!statics.test.url_with_path_connected) {
            const adk_websocket_status_e ws_status = adk_websocket_get_status(ws_handle);

            if (ws_status == adk_websocket_status_connected) {
                print_message("websocket [%p] connected to a url with a path component: [%s]\n", ws_handle, url);
                statics.test.url_with_path_connected = true;
            } else if (ws_status != adk_websocket_status_connecting) {
                print_message("websocket [%p] failed to connect to a url with a path component: [%s]\n", ws_handle, url);
                adk_websocket_close(ws_handle, MALLOC_TAG);
            }

        } else if (!statics.test.url_with_path_has_read) {
            ws_with_path_read_message(ws_handle);
        }

        sb_thread_sleep((milliseconds_t){1});
    }

    assert_true(statics.test.url_with_path_connected);
    assert_true(statics.test.url_with_path_has_read);
}

static void ws_on_open_success_and_close(adk_websocket_handle_t * ws_handle, const adk_websocket_callbacks_t * const callbacks) {
    static const int k_messages = 16;
    assert_non_null(ws_handle);
    statics.test.ws_test_message.ptr = (void *)test_message;
    statics.test.ws_test_message.size = strlen(test_message);
    print_message("Websocket [%p]: sending %d messages: [%.*s]\n", ws_handle, k_messages, (int)statics.test.ws_test_message.size, (char *)statics.test.ws_test_message.ptr);

    const adk_websocket_callbacks_t ws_write_callback = {.success = NULL, .error = NULL, .user = NULL};
    // Send k messages
    for (int i = 0; i < k_messages; i++) {
        adk_websocket_send(ws_handle, statics.test.ws_test_message.consted, adk_websocket_message_text, ws_write_callback);
    }

    // Force close socket connection immediately
    adk_websocket_close(ws_handle, MALLOC_TAG);
    statics.test.send_multiple_messages_and_close = true;
}

static void test_close_connection_before_sending_all_msgs(void ** state) {
    adk_websocket_callbacks_t ws_callbacks = {
        .success = ws_on_open_success_and_close,
        .error = NULL};

    adk_websocket_handle_t * const open_send_close_ws_handle = adk_websocket_create(
        "wss://libwebsockets.org",
        "lws-mirror-protocol",
        NULL,
        ws_callbacks,
        MALLOC_TAG);
    assert_non_null(open_send_close_ws_handle);
    (void)open_send_close_ws_handle;

    while (adk_http_tick()) {
        sb_thread_sleep((milliseconds_t){1});
    }
    assert_true(statics.test.send_multiple_messages_and_close);
}

void init_http2(bool use_proxy, const char * proxy_address) {
    statics.use_proxy = use_proxy;
    statics.proxy_address = proxy_address;
}

static int websocket_backend_null_setup(void ** _) {
    adk_http_init(adk_http_init_default_cert_path, (mem_region_t){0}, adk_websocket_backend_null, (websocket_config_t){0}, unit_test_guard_page_mode, MALLOC_TAG);
    adk_http_set_proxy("null");
    adk_http_set_socks("null");
    return 0;
}

static int websocket_backend_null_teardown(void ** _) {
    adk_http_shutdown(MALLOC_TAG);
    return 0;
}

static void test_null_backend(void ** _) {
    adk_http_dump_heap_usage();
    const heap_metrics_t metrics = adk_http_get_heap_metrics();
    assert_int_equal(metrics.heap_size, 0);

    adk_http_header_list_t * const header_list = adk_http_append_header_list(NULL, "some fake", "value", MALLOC_TAG);
    assert_null(header_list);
    adk_websocket_handle_t * const ws_handle = adk_websocket_create("http://notarealurl.com", "none", header_list, (adk_websocket_callbacks_t){0}, MALLOC_TAG);
    assert_null(ws_handle);
    adk_websocket_handle_t * const ws_ssl_handle = adk_websocket_create_with_ssl_ctx("http://notarealurl.com", "none", header_list, (adk_websocket_callbacks_t){0}, (mem_region_t){0}, (mem_region_t){0}, MALLOC_TAG);
    assert_null(ws_ssl_handle);

    assert_false(adk_http_tick());
    while (adk_http_tick()) {
    }

    assert_int_equal(adk_websocket_get_status(ws_handle), adk_websocket_status_internal_error);
    assert_int_equal(adk_websocket_send(ws_handle, (const_mem_region_t){0}, adk_websocket_message_binary, (adk_websocket_callbacks_t){0}), adk_websocket_status_internal_error);
    assert_int_equal(adk_websocket_begin_read(ws_handle, NULL), adk_websocket_message_error);
    adk_websocket_end_read(ws_handle, MALLOC_TAG);

    adk_websocket_close(ws_handle, MALLOC_TAG);
    adk_websocket_close(ws_ssl_handle, MALLOC_TAG);

    while (adk_http_tick()) {
    }
}

int test_http2() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_websocket_echo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_ws_long_header_name, setup, teardown),
        cmocka_unit_test_setup_teardown(test_ws_bamgrid_json, setup, teardown),
        cmocka_unit_test_setup_teardown(test_close_connection_before_sending_all_msgs, setup, teardown),
    };
    const struct CMUnitTest null_backend_tests[] = {
        cmocka_unit_test_setup_teardown(test_null_backend, websocket_backend_null_setup, websocket_backend_null_teardown),
    };
    statics.backend = adk_websocket_backend_http2;
    print_message("Null tests:\n");
    const int null_tests = cmocka_run_group_tests(null_backend_tests, NULL, NULL);
    print_message("HTTP2 run:\n");
    const int http2_tests = cmocka_run_group_tests(tests, NULL, NULL);
    statics.backend = adk_websocket_backend_websocket;
    print_message("Websockets run:\n");
    return null_tests + http2_tests + cmocka_run_group_tests(tests, NULL, NULL);
}
