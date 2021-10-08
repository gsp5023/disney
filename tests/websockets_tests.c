/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/websockets/private/websocket_constants.h"
#include "source/adk/http/websockets/websockets.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_socket.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

extern const adk_api_t * api;

static websocket_error_e websocket_send_text(websocket_t * const ws, const char * const message) {
    return websocket_send(ws, websocket_message_text, CONST_MEM_REGION(.ptr = message, .size = strlen(message) + 1));
}

typedef struct websocket_test_messages_t {
    const char * small;
    char byte_threshold_msg[ws_byte_max_len];
    char byte_threshold_above_msg[ws_byte_max_len + 1];
    char byte_threshold_below_msg[ws_byte_max_len - 1];
#ifdef _WEBSOCKET_BULK_TEST
    char short_threshold_msg[short_max_val];
    char short_threshold_above_msg[short_max_val + 1];
    char short_threshold_below_msg[short_max_val - 1];
    char large_msg[100001];
#else
    char midsize_message[3 * 1024];
#endif
} websocket_test_messages_t;

typedef struct websocket_test_message_regions_t {
#ifdef _WEBSOCKET_BULK_TEST
    mem_region_t regions[8];
#else
    mem_region_t regions[5];
#endif
} websocket_test_message_regions_t;

typedef struct websocket_connections_test_t {
    websocket_test_messages_t test_messages;
    websocket_test_message_regions_t test_regions;
    int32_t curr_message_ind;
    websocket_t * websocket;
    const char * name;
    websocket_message_type_e message_type;
    bool complete;
    bool first_message_sent;
} websocket_connections_test_t;

static void fill_websocket_test_messages(websocket_test_messages_t * const out_test_messages, websocket_test_message_regions_t * const out_test_regions, const websocket_message_type_e message_type) {
    const char fill_counter = (message_type == websocket_message_binary) ? 0 : 'a';

    *out_test_messages = (websocket_test_messages_t){
        .small = "small echo message",
        .byte_threshold_msg = {fill_counter},
        .byte_threshold_above_msg = {fill_counter + 1},
        .byte_threshold_below_msg = {fill_counter + 2},
#ifdef _WEBSOCKET_BULK_TEST
        .short_threshold_msg = {fill_counter + 3},
        .short_threshold_above_msg = {fill_counter + 4},
        .short_threshold_below_msg = {fill_counter + 5},
        .large_msg = {fill_counter + 6},
#else
        .midsize_message = {fill_counter + 3},
#endif
    };

    *out_test_regions = (websocket_test_message_regions_t){.regions = {
                                                               MEM_REGION(.ptr = out_test_messages->small, .size = strlen(out_test_messages->small) + 1),
                                                               MEM_REGION(.ptr = out_test_messages->byte_threshold_msg, .size = sizeof(out_test_messages->byte_threshold_msg)),
                                                               MEM_REGION(.ptr = out_test_messages->byte_threshold_above_msg, .size = sizeof(out_test_messages->byte_threshold_above_msg)),
                                                               MEM_REGION(.ptr = out_test_messages->byte_threshold_below_msg, .size = sizeof(out_test_messages->byte_threshold_below_msg)),
#ifdef _WEBSOCKET_BULK_TEST
                                                               MEM_REGION(.ptr = out_test_messages->short_threshold_msg, .size = sizeof(out_test_messages->short_threshold_msg)),
                                                               MEM_REGION(.ptr = out_test_messages->short_threshold_above_msg, .size = sizeof(out_test_messages->short_threshold_above_msg)),
                                                               MEM_REGION(.ptr = out_test_messages->short_threshold_below_msg, .size = sizeof(out_test_messages->short_threshold_below_msg)),
                                                               MEM_REGION(.ptr = out_test_messages->large_msg, .size = sizeof(out_test_messages->large_msg)),
#else
                                                               MEM_REGION(.ptr = out_test_messages->midsize_message, .size = sizeof(out_test_messages->midsize_message)),
#endif
                                                           }};

    for (size_t i = 1; i < ARRAY_SIZE(out_test_regions->regions); ++i) {
        int fill = *out_test_regions->regions[i].byte_ptr;
        memset(out_test_regions->regions[i].ptr, fill, out_test_regions->regions[i].size);
    }
}

static void tick_websocket_connection_test(websocket_connections_test_t * const connection_test) {
    websocket_message_t read_message = {0};

    // make sure we send the first message..
    if (!connection_test->first_message_sent) {
        connection_test->first_message_sent = true;
        print_message("[%s]  Sending message of size: {%" PRIu64 "}\n", connection_test->name, (uint64_t)connection_test->test_regions.regions[connection_test->curr_message_ind].size);
        const websocket_error_e send_error = websocket_send(connection_test->websocket, connection_test->message_type, connection_test->test_regions.regions[connection_test->curr_message_ind].consted);
        assert_int_equal(send_error, websocket_error_none);
    }

    if (websocket_read_message(connection_test->websocket, &read_message) == websocket_error_none) {
        print_message("[%s]  Received message of size: {%" PRIu64 "}\n", connection_test->name, (uint64_t)read_message.region.size);
        assert_non_null(read_message.region.ptr);
        assert_int_equal(read_message.message_type, connection_test->message_type);
        assert_int_equal(read_message.region.size, connection_test->test_regions.regions[connection_test->curr_message_ind].size);
        assert_int_equal(memcmp(read_message.region.ptr, connection_test->test_regions.regions[connection_test->curr_message_ind].ptr, connection_test->test_regions.regions[connection_test->curr_message_ind].size), 0);
        websocket_free_message(read_message, MALLOC_TAG);
        ++connection_test->curr_message_ind;
        if (connection_test->curr_message_ind < ARRAY_SIZE(connection_test->test_regions.regions)) {
            print_message("[%s]  Sending message of size: {%" PRIu64 "}\n", connection_test->name, (uint64_t)connection_test->test_regions.regions[connection_test->curr_message_ind].size);
            const websocket_error_e send_error = websocket_send(connection_test->websocket, connection_test->message_type, connection_test->test_regions.regions[connection_test->curr_message_ind].consted);
            assert_int_equal(send_error, websocket_error_none);
        }
    }
}

static void websocket_client_wait_till_shutdown(websocket_client_t * const client) {
    // give the websockets time to finish/drain and close normally.

    // give us 5s to shutdown, if we breach this there is a chance of a lifetime issue.
    nanoseconds_t timer = {5000000000};
    nanoseconds_t start = sb_read_nanosecond_clock();
    while (websocket_client_tick(client)) {
        sb_thread_yield();
        const nanoseconds_t end = sb_read_nanosecond_clock();
        const nanoseconds_t dt = {end.ns - start.ns};
        if (timer.ns < dt.ns) {
            TRAP("websocket client shutdown took an abnormally long time. this is likely a bug");
        } else {
            timer.ns -= dt.ns;
        }
        start = sb_read_nanosecond_clock();
    }

    websocket_client_shutdown(client, MALLOC_TAG);
}

static void queued_send_read_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 1400,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    websocket_test_messages_t test_messages;
    websocket_test_message_regions_t test_regions;
    fill_websocket_test_messages(&test_messages, &test_regions, websocket_message_text);
    websocket_t * const ws = websocket_create(websocket_client, "https://echo.websocket.org", "irrelevant-for-echos", NULL, config, MALLOC_TAG);
    websocket_message_t received_messages[ARRAY_SIZE(test_regions.regions)] = {0};
    int curr_received_message_ind = 0;
    for (size_t i = 0; i < ARRAY_SIZE(test_regions.regions); ++i) {
        print_message("Sending message of {%" PRIu64 "} bytes\n", (uint64_t)test_regions.regions[i].size);
        websocket_send(ws, websocket_message_text, test_regions.regions[i].consted);
    }
    while (websocket_client_tick(websocket_client)) {
        const websocket_error_e read_error = websocket_read_message(ws, &received_messages[curr_received_message_ind]);
        if (read_error == websocket_error_none) {
            print_message("Received message of {%" PRIu64 "} bytes\n", (uint64_t)received_messages[curr_received_message_ind].region.size);
            ++curr_received_message_ind;
            if (curr_received_message_ind == ARRAY_SIZE(received_messages)) {
                break;
            }
        } else if (read_error != websocket_error_no_readable_messages) {
            break;
        }
    }
    const websocket_error_e error = websocket_get_error(ws);
    if (error == websocket_error_connection_handshake_timeout) {
        print_message("[%p] %s\n", ws, websocket_error_to_str(error));
    } else {
        assert_int_equal(websocket_get_status(ws), websocket_status_connected);
        for (size_t i = 0; i < ARRAY_SIZE(test_regions.regions); ++i) {
            assert_int_equal(test_regions.regions[i].size, received_messages[i].region.size);
            assert_int_equal(websocket_message_text, received_messages[i].message_type);
            assert_int_equal(memcmp(test_regions.regions[i].ptr, received_messages[i].region.ptr, received_messages[i].region.size), 0);
            websocket_free_message(received_messages[i], MALLOC_TAG);
        }
    }
    websocket_close(ws, NULL);

    websocket_client_wait_till_shutdown(websocket_client);
}

static void concurrent_websocket_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 1400,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    websocket_connections_test_t websocket_connection_tests[] = {
        {.websocket = websocket_create(websocket_client, "http://echo.websocket.org", "irrelevant-for-echos", NULL, config, MALLOC_TAG),
         .name = "ws://echo.websocket.org"},
        {.websocket = websocket_create(websocket_client, "https://echo.websocket.org", "irrelevant-for-echos", NULL, config, MALLOC_TAG),
         .name = "wss://echo.websocket.org"}};
    for (size_t i = 0; i < ARRAY_SIZE(websocket_connection_tests); ++i) {
        fill_websocket_test_messages(&websocket_connection_tests[i].test_messages, &websocket_connection_tests[i].test_regions, i & 1 ? websocket_message_text : websocket_message_binary);
    };

    assert_int_equal(websocket_send(websocket_connection_tests[0].websocket, websocket_message_binary, CONST_MEM_REGION(.ptr = NULL, .size = SIZE_MAX)), websocket_error_allocation_failure);
    int completed_websockets = 0;
    bool bail = false;
    while (websocket_client_tick(websocket_client)) {
        for (size_t i = 0; i < ARRAY_SIZE(websocket_connection_tests); ++i) {
            if (websocket_connection_tests[i].curr_message_ind < ARRAY_SIZE(websocket_connection_tests[i].test_regions.regions)) {
                tick_websocket_connection_test(&websocket_connection_tests[i]);
                if (websocket_get_status(websocket_connection_tests[i].websocket) == websocket_status_closed) {
                    bail = true;
                    print_message("[%p] %s\n", websocket_connection_tests[i].websocket, websocket_error_to_str(websocket_get_error(websocket_connection_tests[i].websocket)));
                    break;
                }
            } else if (!websocket_connection_tests[i].complete) {
                websocket_close(websocket_connection_tests[i].websocket, NULL);
                websocket_connection_tests[i].complete = true;
                ++completed_websockets;
            }
            assert_true(websocket_connection_tests[i].complete || (websocket_get_status(websocket_connection_tests[i].websocket) != websocket_status_closed));
        }
        if (bail || (completed_websockets == ARRAY_SIZE(websocket_connection_tests))) {
            break;
        }
    }
    if (bail) {
        for (size_t i = 0; i < ARRAY_SIZE(websocket_connection_tests); ++i) {
            if (!websocket_connection_tests[i].complete) {
                websocket_close(websocket_connection_tests[i].websocket, NULL);
            }
        }
    }

    websocket_client_wait_till_shutdown(websocket_client);
}

static void idle_connection_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {5000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 256,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    milliseconds_t idle_timer = {10000};
    // lws's echo server expects this protocol or will refuse to connect outright and doesn't bother sending us any handshake data back
    //   (we handle this, but its weird/confusing when trying to understand what is going on)
    // additionally, lws has issues with sending messages in general. any `continuation` frame becomes a final!? and anything larger than 4096 gets filled with zeros and echo'd back.
    // so we use lws's server here to verify we're connecting to multiple different endpoints across all ws tests.
    // http://libwebsockets.org returns us a 301 which we currently ignore, so handshake fails since we don't follow. so use https:// (443)
    //   (curl is only connecting us to the endpoint and doesn't do follow redirects for us here..)
    websocket_http_header_list_t * header_list = websocket_client_append_header_list(websocket_client, NULL, "a random header field:", "random value", MALLOC_TAG);
    header_list = websocket_client_append_header_list(websocket_client, header_list, "a second random field:", "other value", MALLOC_TAG);

    websocket_t * const ws = websocket_create(websocket_client, "wss://libwebsockets.org", "lws-mirror-protocol", header_list, config, MALLOC_TAG);
    milliseconds_t start = adk_read_millisecond_clock();
    print_message("Idling websocket connection for [%" PRIu32 "] ms\n", idle_timer.ms);
    mem_region_t ws_response_headers = {0};
    while (websocket_client_tick(websocket_client)) {
        const milliseconds_t end = adk_read_millisecond_clock();
        const milliseconds_t dt = {end.ms - start.ms};
        if (idle_timer.ms < dt.ms) {
            websocket_close(ws, NULL);
            break;
        } else {
            idle_timer.ms -= dt.ms;
        }
        if (ws_response_headers.ptr == NULL) {
            ws_response_headers = websocket_get_response_header_bytes(ws);
            if (ws_response_headers.ptr) {
                print_message("Websocket response headers:\n%s", (char *)ws_response_headers.ptr);
            }
        }
        if (websocket_get_status(ws) == websocket_status_closed) {
            break;
        }
        start = adk_read_millisecond_clock();
        sb_thread_sleep((milliseconds_t){1});
    }
    const websocket_error_e error = websocket_get_error(ws);
    if (error == websocket_error_connection_handshake_timeout) {
        print_message("[%p] %s\n", ws, websocket_error_to_str(error));
        websocket_close(ws, NULL);
    } else {
        assert_int_equal(websocket_get_status(ws), websocket_status_user_initiated_closing);
        assert_int_equal(error, websocket_error_none);
    }
    websocket_client_wait_till_shutdown(websocket_client);
}

static void redirect_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {5000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 256,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .maximum_redirects = 2,
        .max_handshake_timeout = {10 * 1000},
    };

    websocket_t * const ws = websocket_create(websocket_client, "ws://libwebsockets.org", "lws-mirror-protocol", NULL, config, MALLOC_TAG);
    while (websocket_client_tick(websocket_client)) {
        if (websocket_get_status(ws) != websocket_status_connecting) {
            break;
        }
    }
    const websocket_error_e error = websocket_get_error(ws);
    if (error == websocket_error_connection_handshake_timeout) {
        print_message("[%p] %s\n", ws, websocket_error_to_str(error));
    } else {
        assert_int_equal(websocket_get_status(ws), websocket_status_connected);
    }
    websocket_close(ws, NULL);
    websocket_client_wait_till_shutdown(websocket_client);
}

static void ping_timeout_connection_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    // generally unreasonably short ping timeouts/no activity to force us to die due to timeout(s) -- if you have a connection that hits these endpoints in 3ms attempt to come up with a better test?
    const websocket_config_t config = {
        .ping_timeout = {2},
        .no_activity_wait_period = {1},
        .receive_buffer_size = 1024,
        .send_buffer_size = 256,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    websocket_t * const ws = websocket_create(websocket_client, "http://echo.websocket.org", "irrelevant-for-echos", NULL, config, MALLOC_TAG);

    while (websocket_client_tick(websocket_client)) {
        if (websocket_get_status(ws) == websocket_status_closed) {
            break;
        }
    }
    const websocket_error_e error = websocket_get_error(ws);
    if (error == websocket_error_connection_handshake_timeout) {
        print_message("[%p] %s\n", ws, websocket_error_to_str(error));
    } else {
        assert_int_equal(error, websocket_error_timeout);
    }
    websocket_close(ws, NULL);

    websocket_client_shutdown(websocket_client, MALLOC_TAG);
}

static void max_receivable_message_limit_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 256,
        .max_receivable_message_size = 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    char large_buf[4 * 1024];
    memset(large_buf, 0xff, sizeof(large_buf));
    websocket_t * const ws = websocket_create(websocket_client, "http://echo.websocket.org", "irrelevant-for-echos", NULL, config, MALLOC_TAG);
    websocket_send(ws, websocket_message_binary, CONST_MEM_REGION(.ptr = large_buf, .size = sizeof(large_buf)));
    while (websocket_client_tick(websocket_client)) {
        if (websocket_get_status(ws) == websocket_status_closed) {
            break;
        }
        websocket_message_t received_message;
        if (websocket_read_message(ws, &received_message) != websocket_error_no_readable_messages) {
            break;
        }
    }
    const websocket_error_e error = websocket_get_error(ws);
    if (error == websocket_error_connection_handshake_timeout) {
        print_message("[%p] %s\n", ws, websocket_error_to_str(error));
    } else {
        assert_int_equal(error, websocket_error_payload_too_large);
    }
    websocket_close(ws, NULL);

    websocket_client_shutdown(websocket_client, MALLOC_TAG);
}

static void immediate_close_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 256,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    websocket_t * const ws = websocket_create(websocket_client, "http://echo.websocket.org", "irrelevant-for-echos", NULL, config, MALLOC_TAG);
    char tiny_buf[1];
    websocket_send(ws, websocket_message_binary, CONST_MEM_REGION(.ptr = tiny_buf, .size = sizeof(tiny_buf)));
    websocket_close(ws, NULL);
    assert_int_equal(websocket_send(ws, websocket_message_binary, CONST_MEM_REGION(.ptr = tiny_buf, .size = sizeof(tiny_buf))), websocket_error_not_connected);
    websocket_client_wait_till_shutdown(websocket_client);
}

static void connection_failures_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 1400,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };
    websocket_config_t timeout_config = config;
    timeout_config.max_handshake_timeout = (milliseconds_t){10};

    websocket_http_header_list_t * header_list = websocket_client_append_header_list(websocket_client, NULL, "a random header field:", "random value", MALLOC_TAG);
    header_list = websocket_client_append_header_list(websocket_client, header_list, "a second random field:", "other value", MALLOC_TAG);

    websocket_t * const dns_failure_ws = websocket_create(websocket_client, "http://this_address_does_not_exist.com/some/non_existant/path", "asd", header_list, config, MALLOC_TAG);
    websocket_t * const handshake_failure_ws = websocket_create(websocket_client, "http://httpbin.org/status/200", "", NULL, config, MALLOC_TAG);
    websocket_t * const handshake_timeout_ws = websocket_create(websocket_client, "wss://libwebsockets.org", "lws-mirror-protocol", NULL, timeout_config, MALLOC_TAG);
    websocket_t * const websockets[] = {
        dns_failure_ws,
        handshake_failure_ws,
        handshake_timeout_ws};
    while (websocket_client_tick(websocket_client)) {
        uint32_t completed_count = 0;
        for (size_t i = 0; i < ARRAY_SIZE(websockets); ++i) {
            if (websocket_get_status(websockets[i]) == websocket_status_closed) {
                ++completed_count;
            }
        }
        if (completed_count == ARRAY_SIZE(websockets)) {
            break;
        }
    }
    assert_int_equal(websocket_get_error(dns_failure_ws), websocket_error_connection);
    assert_int_equal(websocket_get_error(handshake_failure_ws), websocket_error_upgrade_failed_handshake_failure);
    assert_int_equal(websocket_get_response_http_status_code(handshake_failure_ws), http_status_ok);
    assert_int_equal(websocket_get_error(handshake_timeout_ws), websocket_error_connection_handshake_timeout);

    for (size_t i = 0; i < ARRAY_SIZE(websockets); ++i) {
        websocket_close(websockets[i], NULL);
    }
    websocket_client_wait_till_shutdown(websocket_client);
}

static void qa_endpoint_test(void ** state) {
    websocket_client_t * const websocket_client = websocket_client_emplace_init(api->mmap.http2.region, unit_test_guard_page_mode, MALLOC_TAG);
    const websocket_config_t config = {
        .ping_timeout = {10000},
        .no_activity_wait_period = {50000},
        .receive_buffer_size = 1024,
        .send_buffer_size = 4096,
        .max_receivable_message_size = 1024 * 1024,
        .header_buffer_size = 2 * 1024,
        .max_handshake_timeout = {10 * 1000},
    };

    websocket_t * const qa_ws = websocket_create(
        websocket_client,
        "https://qa.global.edge.bamgrid.com/c800b6/connection?X-BAMSDK-Platform=rust%2Fm5%2Fcanalplus-orange&X-DSS-Edge-Accept=vnd.dss.edge%2Bjson%3B%20version%3D0&X-BAMSDK-Client-ID=disney-svod-3d9324fc&X-BAMSDK-Version=0.3",
        "vnd.dss.edge+json",
        NULL,
        config,
        MALLOC_TAG);
    const char message_payload[] = "{\"id\":\"fcc35b13-c016-2b46-8f80-97ce3944e02d\",\"type\":\"urn:dss:event:edge:sdk:authentication\",\"source\":\"urn:dss:source:sdk:rust:m5:canalplus-orange\",\"time\":\"2020-07-15T18:47:59.129Z\",\"istest\":null,\"data\":{\"accessToken\":\"eyJraWQiOiI5NGE1ZTUyYy1hY2IyLTQ2NDEtYTViYy04YTYxYWVmNDZmZmEiLCJhbGciOiJFZERTQSJ9.eyJzdWIiOiI2ODk1YjY0OS0yMTNlLTRlOTAtYTY0OS1hNWQwYzg2OGRjMzgiLCJuYmYiOjE1OTQ4Mzg4NzgsInBhcnRuZXJOYW1lIjoiZGlzbmV5IiwiaXNzIjoidXJuOmJhbXRlY2g6c2VydmljZTp0b2tlbiIsImNvbnRleHQiOnsiYWN0aXZlX3Byb2ZpbGVfaWQiOiI2MDQ5NjNhYi04YzJjLTQ5OGMtYmE2OC0yMGQ1ZWMwYjgzOGUiLCJ1cGRhdGVkX29uIjoiMjAyMC0wNy0xNVQxODo0Nzo1OC45MDkrMDAwMCIsInN1YnNjcmlwdGlvbnMiOltdLCJjb3VudHJ5X3NldHRpbmdzIjp7ImNvZGUiOiJVUyIsInRpbWV6b25lIjp7InV0Y19vZmZzZXQiOiItMDQ6MDAiLCJuYW1lIjoiQW1lcmljYVwvTmV3X1lvcmsifX0sImV4cGlyZXNfb24iOiIyMDIwLTA3LTE1VDE4OjU3OjU4LjkwOSswMDAwIiwiZXhwZXJpbWVudHMiOnt9LCJwcm9maWxlcyI6W3sia2lkc19tb2RlX2VuYWJsZWQiOmZhbHNlLCJwbGF5YmFja19zZXR0aW5ncyI6eyJwcmVmZXJfMTMzIjpmYWxzZX0sImFjdGl2ZSI6dHJ1ZSwiaWQiOiI2MDQ5NjNhYi04YzJjLTQ5OGMtYmE2OC0yMGQ1ZWMwYjgzOGUiLCJhdmF0YXIiOnsiaWQiOiI0NDJhZjdkYi04NWY3LTVlMWQtOTZmMC1iMmM1MTdiZTQwODUifSwidHlwZSI6InVybjpiYW10ZWNoOnByb2ZpbGUiLCJsYW5ndWFnZV9wcmVmZXJlbmNlcyI6eyJhcHBfbGFuZ3VhZ2UiOiJlbi1HQiIsInBsYXliYWNrX2xhbmd1YWdlIjoiZW4tR0IiLCJzdWJ0aXRsZV9sYW5ndWFnZSI6ImVuLUdCIn19XSwiaXBfYWRkcmVzcyI6IjczLjE4MC4xLjE4OSIsInR5cGUiOiJBTk9OWU1PVVMiLCJ2ZXJzaW9uIjoiVjIuMC4wIiwiYmxhY2tvdXRzIjp7ImVudGl0bGVtZW50cyI6W10sImRhdGEiOnt9LCJydWxlcyI6eyJ2aW9sYXRlZCI6W119fSwicGFydG5lciI6eyJuYW1lIjoiZGlzbmV5In0sImxvY2F0aW9uIjp7ImNvdW50cnlfY29kZSI6IlVTIiwiY2l0eV9uYW1lIjoiZXVnZW5lIiwic3RhdGVfbmFtZSI6Im9yZWdvbiIsImRtYSI6ODAxLCJyZWdpb25fbmFtZSI6InBhY2lmaWMgbm9ydGh3ZXN0IiwidHlwZSI6IlpJUF9DT0RFIiwiYXNuIjo3OTIyLCJ6aXBfY29kZSI6Ijk3NDAxIn0sImdlbmVyYXRlZF9vbiI6IjIwMjAtMDctMTVUMTg6NDc6NTguOTA5KzAwMDAiLCJpZCI6ImI0MDRiNmQwLWM2Y2ItMTFlYS1hNmUzLTAyNDJhYzExMDAxMCIsIm1lZGlhX3Blcm1pc3Npb25zIjp7ImVudGl0bGVtZW50cyI6W10sImRhdGEiOnt9LCJydWxlcyI6eyJwYXNzZWQiOltdfX0sImRldmljZSI6eyJhcHBfcnVudGltZSI6Im01IiwicHJvZmlsZSI6ImNhbmFscGx1cy1vcmFuZ2UiLCJpZCI6IjY4OTViNjQ5LTIxM2UtNGU5MC1hNjQ5LWE1ZDBjODY4ZGMzOCIsInR5cGUiOiJ1cm46ZHNzOmRldmljZTppbnRlcm5hbCIsImZhbWlseSI6InJ1c3QiLCJwbGF0Zm9ybSI6ImNhbmFscGx1cy1vcmFuZ2UifSwic3VwcG9ydGVkIjp0cnVlfSwiZW52IjoicWEiLCJleHAiOjE1OTQ4Mzk0NzgsImlhdCI6MTU5NDgzODg3OCwianRpIjoiZmIzYWVlMDktOTZjYS00YmQ0LWE2ODMtNDcyMDQ5ODJlNGYxIn0.o20mwRFMxu_jzcp9PM2GJ0LI_579JUnZa0Y4Ue33ue42bxm4m54aFe7Evf38r1JqMm_NbafQoS27bfiMkkbIDA\"},\"specversion\":\"0.3\",\"envelopeversion\":null,\"schemaurl\":\"https://github.bamtech.co/schema-registry/schema-registry/blob/master/dss/event/edge/1.0.0/sdk/authentication.oas2.yaml\",\"datacontenttype\":\"application/json;charset=utf-8\",\"datacontentencoding\":null,\"subject\":\"sessionId=b404b6d0-c6cb-11ea-a6e3-0242ac110010\",\"partner\":null}";

    websocket_send(qa_ws, websocket_message_text, CONST_MEM_REGION(.ptr = message_payload, .size = strlen(message_payload)));
    websocket_message_t received_message = {0};
    while (websocket_client_tick(websocket_client)) {
        const websocket_error_e read_error = websocket_read_message(qa_ws, &received_message);
        if (read_error != websocket_error_no_readable_messages) {
            break;
        }
    }
    print_message("qa headers:\n%s", (char *)websocket_get_response_header_bytes(qa_ws).ptr);
    // normally I'd check to see if we're still connected here, but QA may immediately send a shutdown message when it sends us the expired auth payload
    // and thus we could receive a close frame, start closing down, and close before we exit the above loop.
    // the only 'normal' route then is to make sure we haven't encountered an error since we shut down cleanly.
    assert_int_equal(websocket_get_error(qa_ws), websocket_error_none);
    print_message("qa response:\n%.*s\n", (int)received_message.region.size, (char *)received_message.region.ptr);
    // sometimes qa just closes the socket on us and doesn't give us any message to read either...
    if (received_message.region.ptr) {
        websocket_free_message(received_message, MALLOC_TAG);
    }

    websocket_close(qa_ws, NULL);
    websocket_client_wait_till_shutdown(websocket_client);
}

int test_websockets() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(redirect_test),
        cmocka_unit_test(max_receivable_message_limit_test),
        cmocka_unit_test(queued_send_read_test),
        cmocka_unit_test(concurrent_websocket_test),
        cmocka_unit_test(idle_connection_test),
        cmocka_unit_test(immediate_close_test),
        cmocka_unit_test(ping_timeout_connection_test),
        cmocka_unit_test(connection_failures_test),
        cmocka_unit_test(qa_endpoint_test),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
