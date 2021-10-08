
/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sentry_tests.c
test reporting capabilities
*/

#include "source/adk/cncbus/cncbus_addresses.h"
#include "source/adk/log/private/log_p.h"
#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/reporting/adk_reporting_sentry_options.h"
#include "source/adk/reporting/adk_reporting_utils.h"
#include "source/adk/reporting/private/adk_reporting_send_queue.h"
#include "source/adk/reporting/private/adk_reporting_sentry.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

#define TAG_ADK_REPORTING_TEST FOURCC('R', 'E', 'T', 'E')

enum {
    sentry_heap_size = 4 * 1024 * 1024,
    httpx_heap_size = 2 * 1024 * 1024,
    httpx_fragment_buffer_size = 4 * 1024 * 1024,
    max_send_queue_size = 32
};

static struct {
    heap_t heap;
    mem_region_t sentry_region;
    mem_region_t httpx_region;
    mem_region_t fragment_buffer;
    adk_httpx_api_t * httpx_api;
    adk_httpx_client_t * httpx_client;
} statics;

typedef struct sent_status_data_t {
    int code;
    bool called;
} sent_status_data_t;
static void testing_reporting_sent_status_cb(bool success, const char * error_message, void * data) {
    sent_status_data_t * sent_status = (sent_status_data_t *)data;

    if (sent_status->code == 200) {
        assert_true(success);
        assert_null(error_message);
    }
    if (sent_status->code == 400) {
        assert_false(success);
        assert_string_equal(error_message, "The adk_reporting upload to sentry failed with HTTP code 400.  The event will NOT be resent.");
    }
    sent_status->called = true;
}

typedef struct override_send_data_t {
    const char * const endpoint_check;
    const char * const auth_header_check;
    const char * const body_contains_check;
    bool called;
} override_send_data_t;
static void testing_reporting_override_send_cb(adk_httpx_client_t * const client, const char * endpoint, const char ** headers, const char * body, void * data) {
    override_send_data_t * sent_data = (override_send_data_t *)data;

    assert_non_null(client);
    assert_non_null(endpoint);
    assert_non_null(headers);
    assert_non_null(body);

    assert_string_equal(endpoint, sent_data->endpoint_check);
    assert_string_equal(headers[2], sent_data->auth_header_check);
    assert_non_null(strstr(body, sent_data->body_contains_check));
    sent_data->called = true;
}

static void test_reporting_full_dsn_inspect_message(void ** ignored) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://baaaaaaaaaaaaaaaaaaaaaaaaaaaaaab@dev-sentry.bamgrid.com/101",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_debug,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);

    override_send_data_t override_data = {
        .called = false,
        .auth_header_check = "x-sentry-auth:Sentry sentry_key=baaaaaaaaaaaaaaaaaaaaaaaaaaaaaab,sentry_version=7,sentry_client=adk_reporting_sentry",
        .body_contains_check = "test_reporting_full_dsn_inspect_message: ADK_REPORTING_SET_MSG_DEBUG",
        .endpoint_check = "https://dev-sentry.bamgrid.com/api/101/store/"};
    adk_reporting_sentry_override_send_set(instance, &testing_reporting_override_send_cb, (void *)&override_data);

    adk_reporting_instance_push_tag(instance, "instance_ncp_version", "1.0");
    adk_reporting_instance_push_tag(instance, "instance_sentry_version", "test");

    // dup tag example. instance_ncp_version tag should be updated to 2.0
    adk_reporting_instance_push_tag(instance, "instance_ncp_version", "2.0");

    adk_reporting_key_val_t msg_tag = {
        .key = "msg_tag", .value = "msg_tag", .next = NULL};
    ADK_REPORTING_REPORT_MSG_DEBUG(instance, &msg_tag, "test_reporting_full_dsn_inspect_message: %s", "ADK_REPORTING_SET_MSG_DEBUG");
    while (adk_reporting_tick(instance)) {
        sb_thread_yield();
    }

    adk_reporting_instance_free(instance);
    assert_true(override_data.called);
}

static void test_reporting_bad_request(void ** ignored) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://baaaaaaaaaaaaaaaaaaaaaaaaaaaaaab@dev-sentry.bamgrid.com/101",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_debug,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);
    sent_status_data_t sent_data = {
        .called = false,
        .code = 400};
    adk_reporting_sentry_sent_status_set(instance, &testing_reporting_sent_status_cb, (void *)&sent_data);

    adk_reporting_key_val_t msg_tag = {
        .key = "msg_tag", .value = "msg_tag", .next = NULL};
    ADK_REPORTING_REPORT_MSG_DEBUG(instance, &msg_tag, "test_reporting_bad_request: %s", "ADK_REPORTING_SET_MSG_DEBUG");
    while (adk_reporting_tick(instance)) {
        sb_thread_yield();
    }

    adk_reporting_instance_free(instance);
    assert_true(sent_data.called);
}

static void test_reporting_good_request(void ** ignored) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://ca310f8f58fb42acb95e7cab154ab225@dev-sentry.bamgrid.com/28",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_debug,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);
    sent_status_data_t sent_data = {
        .called = false,
        .code = 200};
    adk_reporting_sentry_sent_status_set(instance, &testing_reporting_sent_status_cb, (void *)&sent_data);

    // Example using instance (global) tags
    adk_reporting_instance_push_tag(instance, "ncp_version", "1.0");
    adk_reporting_instance_push_tag(instance, "sentry_version", "test");

    adk_reporting_key_val_t msg_tag = {
        .key = "msg_tag", .value = "msg_tag", .next = NULL};

    // Example reporting event now
    ADK_REPORTING_REPORT_MSG_DEBUG(instance, &msg_tag, "test_reporting_good_request: %s", "ADK_REPORTING_SET_MSG_DEBUG");
    while (adk_reporting_tick(instance)) {
        sb_thread_yield();
    }

    adk_reporting_instance_free(instance);
    assert_true(sent_data.called);
}

static void test_reporting_adk_reporting_create_href(void ** ignored) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://ca310f8f58fb42acb95e7cab154ab225@dev-sentry.bamgrid.com/28",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_debug,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);

    adk_reporting_url_info_t t = {0};
    t.host_info.host = "dev-sentry.bamgrid.com";
    t.protocol = "https:";

    adk_reporting_create_href(instance, &t, MALLOC_TAG);
    assert_string_equal(t.href, "https://dev-sentry.bamgrid.com");

    adk_reporting_free(instance, t.href, MALLOC_TAG);
    adk_reporting_instance_free(instance);
}

static void test_reporting_parse_bad_url(void ** ignored) {
#ifdef _DEBUG
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://ca310f8f58fb42acb95e7cab154ab225@dev-sentry.bamgrid.com/28",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_debug,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);

    expect_assert_failure(adk_reporting_parse_href(instance, "https://user:pass@/path?query=string/#hash", MALLOC_TAG));
    expect_assert_failure(adk_reporting_parse_href(instance, "/path?query=string/#hash", MALLOC_TAG));
    expect_assert_failure(adk_reporting_parse_href(instance, "https:///path/api/v1/2", MALLOC_TAG));
    expect_assert_failure(adk_reporting_parse_href(instance, "/path/api/v1/2", MALLOC_TAG));
    expect_assert_failure(adk_reporting_parse_href(instance, NULL, MALLOC_TAG));
    expect_assert_failure(adk_reporting_parse_href(instance, "", MALLOC_TAG));

    adk_reporting_instance_free(instance);
#endif
}

static void * safe_cjson_malloc(void * const ctx, size_t const size) {
    adk_reporting_instance_t * const instance = (adk_reporting_instance_t *)ctx;
    return adk_reporting_malloc(instance, size, MALLOC_TAG);
}

static void safe_cjson_free(void * const ctx, void * const ptr) {
    adk_reporting_instance_t * const instance = (adk_reporting_instance_t *)ctx;
    adk_reporting_free(instance, ptr, MALLOC_TAG);
}

static void * cjson_malloc(void * const ctx, size_t const size) {
    heap_t * const heap = (heap_t *)ctx;
    return heap_alloc(heap, size, MALLOC_TAG);
}

static void cjson_free(void * const ctx, void * const ptr) {
    heap_t * const heap = (heap_t *)ctx;
    if (ptr) {
        heap_free(heap, ptr, MALLOC_TAG);
    }
}

static void test_reporting_sentry_exception_json(void ** ignored) {
    cJSON_Env json_ctx = (cJSON_Env){
        .ctx = &statics.heap,
        .callbacks = {
            .malloc = cjson_malloc,
            .free = cjson_free,
        }};

    cJSON * json = cJSON_CreateObject(&json_ctx);

    uintptr_t frames[2] = {0x1, 0x2};
    uintptr_t * frames_ptr = frames;
    assert_true(adk_reporting_append_exception_json(&json_ctx, "errorType", "error_value", (void **)frames_ptr, 2, &json));
    char * json_string = cJSON_PrintUnformatted(&json_ctx, json);
    cJSON_Delete(&json_ctx, json);
    assert_string_equal(json_string, "{\"exception\":{\"values\":[{\"type\":\"errorType\",\"value\":\"error_value\",\"stacktrace\":{\"frames\":[{\"instruction_addr\":\"0x1\"},{\"instruction_addr\":\"0x2\"}]}}]}}");
    cJSON_free(&json_ctx, json_string);
}

static void test_reporting_sentry_exception_inspect(void ** ignored) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://baaaaaaaaaaaaaaaaaaaaaaaaaaaaaab@dev-sentry.bamgrid.com/101",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_fatal,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);

    override_send_data_t override_send_data = {
        .auth_header_check = "x-sentry-auth:Sentry sentry_key=baaaaaaaaaaaaaaaaaaaaaaaaaaaaaab,sentry_version=7,sentry_client=adk_reporting_sentry",
        .body_contains_check = "Unsafe cast from void* to : int",
        .endpoint_check = "https://dev-sentry.bamgrid.com/api/101/store/",
        .called = false};
    adk_reporting_sentry_override_send_set(instance, &testing_reporting_override_send_cb, (void *)&override_send_data);

    ADK_REPORTING_REPORT_EXCEPTION_FATAL(instance, NULL, NULL, 0, "TypeCastError", "Unsafe cast from void* to : %s", "int");
    while (adk_reporting_tick(instance)) {
        sb_thread_yield();
    }

    adk_reporting_instance_free(instance);
    assert_true(override_send_data.called);
}

static void test_send_queue(uint32_t send_queue_size) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://ca310f8f58fb42acb95e7cab154ab225@dev-sentry.bamgrid.com/28",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_debug,
        .heap = &statics.heap,
        .send_queue_size = send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);
    // Example using instance (global) tags
    adk_reporting_instance_push_tag(instance, "ncp_version", "1.0");
    adk_reporting_instance_push_tag(instance, "sentry_version", "test");

    assert_true(adk_reporting_is_send_queue_empty(instance));

    cJSON_Env json_ctx = (cJSON_Env){
        .ctx = instance,
        .callbacks = {
            .malloc = safe_cjson_malloc,
            .free = safe_cjson_free,
        }};

    // fill up queue
    for (uint32_t i = 0; i < send_queue_size; i++) {
        cJSON * event = cJSON_CreateObject(&json_ctx);
        cJSON_AddNumberToObject(&json_ctx, event, "Index", i);
        adk_reporting_enqueue_to_send(instance, event);
        assert_int_equal(instance->send_queue->length, i + 1);
    }

    assert_int_equal(instance->send_queue->length, send_queue_size);

    adk_reporting_send_node_t * head = adk_reporting_flush_send_queue(instance, regard_pause);
    assert_true(adk_reporting_is_send_queue_empty(instance));

    uint32_t count = 0;

    while (head) {
        adk_reporting_send_node_t * stale = head;
        head = head->next;
        int index = cJSON_GetObjectItem(stale->event, "Index")->valueint;
        assert_int_equal(index, count);
        adk_reporting_free_send_node(instance, stale);
        count++;
    }
    assert_int_equal(count, send_queue_size);

    // Overload the queue
    for (uint32_t i = 0; i < (send_queue_size * 2); i++) {
        cJSON * event = cJSON_CreateObject(&json_ctx);
        cJSON_AddNumberToObject(&json_ctx, event, "Index", i);
        adk_reporting_enqueue_to_send(instance, event);
    }
    assert_int_equal(instance->send_queue->length, send_queue_size);

    head = adk_reporting_flush_send_queue(instance, regard_pause);

    assert_true(adk_reporting_is_send_queue_empty(instance));

    count = 0;

    while (head) {
        adk_reporting_send_node_t * stale = head;
        head = head->next;
        int index = cJSON_GetObjectItem(stale->event, "Index")->valueint;
        assert_int_equal(index, count + send_queue_size);
        adk_reporting_free_send_node(instance, stale);
        count++;
    }
    assert_int_equal(count, send_queue_size);

    // TODO Test set delay - https://jira.disneystreaming.com/browse/M5-1966

    adk_reporting_instance_free(instance);
}

static void test_send_queues(void ** ignored) {
    test_send_queue(1);
    test_send_queue(32);
}

static void test_reporting_through_log(void ** ignored) {
    adk_reporting_init_options_t options = {
        .sentry_dsn = "https://ca310f8f58fb42acb95e7cab154ab225@dev-sentry.bamgrid.com/28",
        .reporter_name = "reporting_tests",
        .httpx_client = statics.httpx_client,
        .min_report_level = event_level_error,
        .heap = &statics.heap,
        .send_queue_size = max_send_queue_size};
    adk_reporting_instance_t * instance = adk_reporting_instance_create(&options);
    sent_status_data_t sent_data = {
        .called = false,
        .code = 200};
    adk_reporting_sentry_sent_status_set(instance, &testing_reporting_sent_status_cb, (void *)&sent_data);

    log_init(NULL, CNCBUS_ADDRESS_LOGGER, CNCBUS_SUBNET_MASK_CORE, instance);
    LOG_ERROR(TAG_ADK_REPORTING_TEST, "test_reporting_through_log", "message");
    while (adk_reporting_tick(instance)) {
        sb_thread_yield();
    }

    log_shutdown();
    adk_reporting_instance_free(instance);
    assert_true(sent_data.called);
}

static int setup(void ** state) {
    statics.sentry_region = sb_map_pages(
        PAGE_ALIGN_INT(sentry_heap_size),
        system_page_protect_read_write);
    TRAP_OUT_OF_MEMORY(statics.sentry_region.ptr);
    heap_init_with_region(&statics.heap, statics.sentry_region, 0, 0, "reporting_tests_heap");

    const size_t aligned_httpx_region_size = PAGE_ALIGN_INT(httpx_heap_size);
    statics.httpx_region = MEM_REGION(.ptr = heap_alloc(&statics.heap, aligned_httpx_region_size, MALLOC_TAG), .size = aligned_httpx_region_size);
    statics.fragment_buffer = sb_map_pages(PAGE_ALIGN_INT(httpx_fragment_buffer_size), system_page_protect_read_write);

    statics.httpx_api = adk_httpx_api_create(
        statics.httpx_region,
        statics.fragment_buffer,
        fragment_size,
        unit_test_guard_page_mode,
        adk_httpx_init_normal,
        "adk-reporting-httpx-test");

    statics.httpx_client = adk_httpx_client_create(statics.httpx_api);

    return 0;
}

static int teardown(void ** state) {
    adk_httpx_client_free(statics.httpx_client);
    adk_httpx_api_free(statics.httpx_api);

    heap_free(&statics.heap, statics.httpx_region.ptr, MALLOC_TAG);
#ifndef NDEBUG
    heap_debug_print_leaks(&statics.heap);
#endif
    heap_destroy(&statics.heap, MALLOC_TAG);

    if (statics.sentry_region.ptr != NULL) {
        sb_unmap_pages(statics.sentry_region);
    }

    if (statics.fragment_buffer.ptr != NULL) {
        sb_unmap_pages(statics.fragment_buffer);
    }

    return 0;
}

int test_reporting() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_reporting_full_dsn_inspect_message),
        cmocka_unit_test(test_reporting_bad_request),
        cmocka_unit_test(test_reporting_good_request),
        cmocka_unit_test(test_reporting_adk_reporting_create_href),
        cmocka_unit_test(test_reporting_parse_bad_url),
        cmocka_unit_test(test_reporting_sentry_exception_json),
        cmocka_unit_test(test_reporting_sentry_exception_inspect),
        cmocka_unit_test(test_reporting_through_log),
        cmocka_unit_test(test_send_queues),
    };

    return cmocka_run_group_tests(tests, setup, teardown);
}