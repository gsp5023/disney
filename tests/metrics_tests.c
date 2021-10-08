/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
metrics_tests.c
*/

#include "source/adk/cncbus/cncbus_addresses.h"
#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/runtime/app/app.h"
#include "testapi.h"

#define TAG_TEST FOURCC('M', 'E', 'T', 'R')

#ifdef NDEBUG
#define OUT_OF_RANGE_TEST_DISTANCE 0
#else
#define OUT_OF_RANGE_TEST_DISTANCE 1
#endif

extern const adk_api_t * api;

static struct {
    const adk_api_t * api;
    cncbus_t bus;
    struct bus_dispatcher_t {
        sb_thread_id_t thread_id;
        volatile bool run;
    } bus_dispatcher;
    bool using_bus;

    struct {
        struct {
            int int_value;
            float float_value;
            milliseconds_t delta_time;
            metric_time_to_first_interaction_t interaction_timestamps;
        } sent;
        struct {
            int int_value;
            float float_value;
            milliseconds_t delta_time;
            metric_time_to_first_interaction_t interaction_timestamps;
        } read;
    } metrics;
} metrics_tests;

static struct {
    cncbus_receiver_t bus_receiver;
    cncbus_receiver_vtable_t vtable;
} metrics_receiver;

// --------------------------------------------------

static const char * format_metric(const void * const metric, const metric_types_e metric_type, char * const output_buf, const size_t output_buf_size) {
    switch (metric_type) {
        case metric_type_int: {
            sprintf_s(output_buf, output_buf_size, "%i", *(int *)metric);
            break;
        }
        case metric_type_float: {
            sprintf_s(output_buf, output_buf_size, "%f", *(float *)metric);
            break;
        }
        case metric_type_delta_time_in_ms: {
            sprintf_s(output_buf, output_buf_size, "%d", ((milliseconds_t *)metric)->ms);
            break;
        }
        case metric_type_time_to_first_interaction: {
            const metric_time_to_first_interaction_t * const ttfi = metric;
            sprintf_s(output_buf, output_buf_size, "{ main_timestamp:{%d}, app_init_timestamp:{%d}, dismiss_system_splash_timestamp:{%d} }", ttfi->main_timestamp.ms, ttfi->app_init_timestamp.ms, ttfi->dimiss_system_splash_timestamp.ms);
            break;
        }
        default:
            break;
    }
    return output_buf;
}

static int on_metrics_msg_received(cncbus_receiver_t * const self, const cncbus_msg_header_t header, cncbus_msg_t * const msg) {
    metric_msg_header_t metric_header;
    cncbus_msg_read(msg, &metric_header, sizeof(metric_header));

    struct tm time_info;
    sb_seconds_since_epoch_to_localtime(metric_header.time_since_epoch.seconds, &time_info);

    char time_str[27];
    snprintf(time_str, ARRAY_SIZE(time_str), "%04d-%02d-%02d|%02d:%02d:%02d.%06d", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec, metric_header.time_since_epoch.microseconds);

    char metric_format_buf[1024];
    switch (metric_header.type) {
        case metric_type_int: {
            int32_t metric_int;
            cncbus_msg_read(msg, &metric_int, sizeof(metric_int));
            debug_write_line("[%s][METRIC] %s", time_str, format_metric(&metric_int, metric_type_int, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
            metrics_tests.metrics.read.int_value = metric_int;
            break;
        }
        case metric_type_float: {
            float metric_float;
            cncbus_msg_read(msg, &metric_float, sizeof(metric_float));
            debug_write_line("[%s][METRIC] %s", time_str, format_metric(&metric_float, metric_type_float, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
            metrics_tests.metrics.read.float_value = metric_float;
            break;
        }
        case metric_type_delta_time_in_ms: {
            milliseconds_t dt;
            cncbus_msg_read(msg, &dt, sizeof(dt));
            debug_write_line("[%s][METRIC] %s", time_str, format_metric(&dt, metric_type_delta_time_in_ms, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
            metrics_tests.metrics.read.delta_time = dt;
            break;
        }
        case metric_type_time_to_first_interaction: {
            metric_time_to_first_interaction_t ttfi;
            cncbus_msg_read(msg, &ttfi, sizeof(ttfi));
            debug_write_line("[%s][METRIC] %s", time_str, format_metric(&ttfi, metric_type_time_to_first_interaction, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
            metrics_tests.metrics.read.interaction_timestamps = ttfi;
            break;
        }
        case metric_types_last:
        default:
            break;
    }

    return 0;
}

void metrics_receiver_init(cncbus_t * const bus, const cncbus_address_t address) {
    if (bus) {
        metrics_receiver.vtable.on_msg_recv = on_metrics_msg_received;
        cncbus_init_receiver(&metrics_receiver.bus_receiver, &metrics_receiver.vtable, address);
        cncbus_connect(bus, &metrics_receiver.bus_receiver);
    }
}

void metrics_receiver_shutdown(cncbus_t * const bus) {
    if (bus) {
        cncbus_disconnect(bus, &metrics_receiver.bus_receiver);
    }
}

// --------------------------------------------------

static int metrics_tests_bus_setup(void ** state) {
    cncbus_init(&metrics_tests.bus, metrics_tests.api->mmap.cncbus.region, unit_test_guard_page_mode);
    metrics_init(&metrics_tests.bus, CNCBUS_ADDRESS_LOGGER, CNCBUS_SUBNET_MASK_CORE);
    metrics_tests.using_bus = true;
    metrics_receiver_init(&metrics_tests.bus, CNCBUS_ADDRESS_LOGGER);

    return 0;
}

static int metrics_tests_bus_teardown(void ** state) {
    metrics_init(NULL, CNCBUS_ADDRESS_LOGGER, CNCBUS_SUBNET_MASK_CORE);
    cncbus_dispatch(&metrics_tests.bus, cncbus_dispatch_flush);
    metrics_receiver_shutdown(&metrics_tests.bus);
    cncbus_destroy(&metrics_tests.bus);

    return 0;
}

// --------------------------------------------------

static void test_metrics_publish(void ** state) {
    char metric_format_buf[1024];

    metrics_tests.metrics.sent.int_value = 1234;
    metrics_tests.metrics.sent.float_value = 0.5768f;
    metrics_tests.metrics.sent.delta_time = adk_read_millisecond_clock();
    metrics_tests.metrics.sent.interaction_timestamps = (metric_time_to_first_interaction_t){adk_read_millisecond_clock(), adk_read_millisecond_clock(), adk_read_millisecond_clock()};

    debug_write_line("sent int_value: %s", format_metric(&metrics_tests.metrics.sent.int_value, metric_type_int, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
    debug_write_line("sent float_value: %s", format_metric(&metrics_tests.metrics.sent.float_value, metric_type_float, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
    debug_write_line("sent delta_time: %s", format_metric(&metrics_tests.metrics.sent.delta_time, metric_type_delta_time_in_ms, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
    debug_write_line("sent interaction_timestamps: %s", format_metric(&metrics_tests.metrics.sent.interaction_timestamps, metric_type_time_to_first_interaction, metric_format_buf, ARRAY_SIZE(metric_format_buf)));

    publish_metric(metric_type_int, &metrics_tests.metrics.sent.int_value, sizeof(metrics_tests.metrics.sent.int_value));
    publish_metric(metric_type_float, &metrics_tests.metrics.sent.float_value, sizeof(metrics_tests.metrics.sent.float_value));
    publish_metric(metric_type_delta_time_in_ms, &metrics_tests.metrics.sent.delta_time, sizeof(metrics_tests.metrics.sent.delta_time));
    publish_metric(metric_type_time_to_first_interaction, &metrics_tests.metrics.sent.interaction_timestamps, sizeof(metrics_tests.metrics.sent.interaction_timestamps));

    // Flush the cncbus to ensure the message is sent and received
    cncbus_dispatch(&metrics_tests.bus, cncbus_dispatch_flush);

    debug_write_line("read int_value: %s", format_metric(&metrics_tests.metrics.read.int_value, metric_type_int, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
    debug_write_line("read float_value: %s", format_metric(&metrics_tests.metrics.read.float_value, metric_type_float, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
    debug_write_line("read delta_time: %s", format_metric(&metrics_tests.metrics.read.delta_time, metric_type_delta_time_in_ms, metric_format_buf, ARRAY_SIZE(metric_format_buf)));
    debug_write_line("read interaction_timestamps: %s", format_metric(&metrics_tests.metrics.read.interaction_timestamps, metric_type_time_to_first_interaction, metric_format_buf, ARRAY_SIZE(metric_format_buf)));

    assert_true(memcmp(&metrics_tests.metrics.sent, &metrics_tests.metrics.read, sizeof(metrics_tests.metrics.sent)) == 0);
}

static void test_get_cpu_mem(void ** state) {
    sb_cpu_mem_status_t cpu_mem = sb_get_cpu_mem_status();

    debug_write_line("cpu utilization: %f %%", cpu_mem.cpu_utilization * 100);
    debug_write_line("memory used: %ul Bytes", cpu_mem.memory_used);
    debug_write_line("memory available: %ul Bytes", cpu_mem.memory_available);

#if !defined(_LEIA) && !defined(_VADER)
    assert_true(cpu_mem.cpu_utilization > 0);
#endif
    assert_true(cpu_mem.memory_used > 0);
    assert_true(cpu_mem.memory_available > 0);
}

int test_metrics() {
    metrics_tests.api = api;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_metrics_publish, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_get_cpu_mem, NULL, NULL)};

    return cmocka_run_group_tests(tests, metrics_tests_bus_setup, metrics_tests_bus_teardown);
}
