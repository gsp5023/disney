/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
log_tests.c

logging test fixture
*/

#include "source/adk/cncbus/cncbus_addresses.h"
#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/log/private/log_p.h"
#include "source/adk/log/private/log_receiver.h"
#include "source/adk/runtime/app/app.h"
#include "testapi.h"

#define TAG_TEST FOURCC('T', 'E', 'S', 'T')

#ifdef NDEBUG
#define OUT_OF_RANGE_TEST_DISTANCE 0
#else
#define OUT_OF_RANGE_TEST_DISTANCE 1
#endif

extern const adk_api_t * api;

static struct {
    cncbus_t bus;
    struct bus_dispatcher_t {
        sb_thread_id_t thread_id;
        volatile bool run;
    } bus_dispatcher;
    bool using_bus;
    log_level_e previous_log_level;
} log_tests = {0};

static void log_tests_try_bus_single_dispatch() {
    if (log_tests.using_bus) {
        print_message(" <<< dispatching message >>>\n");
        cncbus_dispatch(&log_tests.bus, cncbus_dispatch_single_message);
    }
}

static void test_log_min_level_range(void ** state) {
    print_message("\n");
    print_message("num_log_levels = %d\n", num_log_levels);
    log_set_min_level(num_log_levels - 1); // max level, just wanting to init other than 0 before first step through loop
    log_level_t last_min_level = log_get_min_level();
    for (log_level_t i = 0; i < num_log_levels + OUT_OF_RANGE_TEST_DISTANCE; i++) {
        print_message("log_set_min_level(%d) ", i);
        if (i < num_log_levels) {
            log_set_min_level(i);
            last_min_level = log_get_min_level();
            assert_true(i == last_min_level);
            print_message("succeeded\n");
        } else {
            expect_assert_failure(log_set_min_level(i));
        }
    }
    print_message("\n");
}

static void test_log_get_names(void ** state) {
    print_message("\n");
    const char * name;
    for (log_level_t i = 0; i < num_log_levels + OUT_OF_RANGE_TEST_DISTANCE; i++) {
        print_message("log_get_level_name(%d)       ", i);
        if (i < num_log_levels) {
            name = log_get_level_name(i);
            assert_non_null(name);
            print_message("= %s\n", name);
        } else {
            expect_assert_failure(name = log_get_level_name(i));
        }
        print_message("log_get_level_short_name(%d) ", i);
        if (i < num_log_levels) {
            name = log_get_level_short_name(i);
            assert_non_null(name);
            print_message("= %s\n", name);
        } else {
            expect_assert_failure(name = log_get_level_short_name(i));
        }
        print_message("\n");
    }
}

static void test_log_message_levels(void ** state) {
    print_message("\n");
    for (log_level_t min_level = 0; min_level < num_log_levels; min_level++) {
        print_message("******************************************\n");
        print_message("with log_set_min_level(%d) %s\n", min_level, log_get_level_name(min_level));
        log_set_min_level(min_level);
        for (log_level_t level = min_level; level < num_log_levels + OUT_OF_RANGE_TEST_DISTANCE; level++) {
            print_message("\ncalling log_message_va() with level = %d, ", level);
            if (level >= num_log_levels) {
                print_message("(should assert)\n");
                expect_assert_failure(log_message_va(__FILE__, __LINE__, __func__, level, TAG_TEST, "this msg should assert"));
            } else if (level < min_level) {
                print_message("(a test msg SHOULD NOT be logged below)\n");
                log_message_va(__FILE__, __LINE__, __func__, level, TAG_TEST, "this msg SHOULD NOT be logged");
            } else {
                print_message("(a test msg SHOULD be logged below)\n");
                log_message_va(__FILE__, __LINE__, __func__, level, TAG_TEST, "this msg SHOULD be logged");
                log_tests_try_bus_single_dispatch();
            }
        }
        print_message("\n");
    }
}

static uint32_t expected_tags[] = {
    FOURCC('a', 'b', 'c', 'd'),
    FOURCC('1', '2', '3', '4'),
    FOURCC('.', '.', '.', '.'),
    FOURCC('-', '-', '-', '-'),
    0,
    '*',
    TAG_TEST};

static uint32_t unexpected_tags[] = {
    FOURCC('\0', '\0', 'B', 'K'),
    FOURCC('F', 'R', '\0', '\0'),
    FOURCC('S', '\0', '\0', 'E'),
    0x0000FFFF,
    0xFFFF0000,
    INT32_MAX,
    UINT32_MAX};

static void test_log_tags(void ** state) {
    log_set_min_level(log_level_debug);

    print_message("\n******************************************\n");
    print_message("sampling of expected fourcc_tag values, all should be aligned with a 4-char tag width\n\n");
    for (int i = 0; i < ARRAY_SIZE(expected_tags); i++) {
        LOG_INFO(expected_tags[i], "via LOG_INFO macro with fourcc_tag = %d", expected_tags[i]);
        log_tests_try_bus_single_dispatch();
    }

    print_message("\n******************************************\n");
    print_message("sampling of unexpected fourcc_tag examples, most should still be aligned with a 4-char tag width\n\n");
    for (int i = 0; i < ARRAY_SIZE(unexpected_tags); i++) {
        LOG_INFO(unexpected_tags[i], "via LOG_INFO macro with fourcc_tag = %d", unexpected_tags[i]);
        log_tests_try_bus_single_dispatch();
    }

    print_message("\n");
}

static void test_log_message_args(void ** state) {
    print_message("\n");

    char buf[max_log_msg_length];
    memset(buf, '*', ARRAY_SIZE(buf) - 1);
    buf[max_log_msg_length - 1] = 0;

    LOG_ALWAYS(TAG_TEST, "this is an oversized log test message, will be truncated%s", buf);
    log_tests_try_bus_single_dispatch();
    print_message("\n");

    memcpy(buf, "again... ", 9);
    LOG_ALWAYS(TAG_TEST, buf);
    log_tests_try_bus_single_dispatch();
    print_message("\n");
}

static void test_log_macros(void ** state) {
    log_set_min_level(log_level_debug);

    print_message("\n");
    LOG_DEBUG(TAG_TEST, "This is a log test message via the LOG_DEBUG macro");
    log_tests_try_bus_single_dispatch();
    print_message("\n");
    LOG_INFO(TAG_TEST, "This is a log test message via the LOG_INFO macro");
    log_tests_try_bus_single_dispatch();
    print_message("\n");
    LOG_WARN(TAG_TEST, "This is a log test message via the LOG_WARN macro");
    log_tests_try_bus_single_dispatch();
    print_message("\n");
    LOG_ERROR(TAG_TEST, "This is a log test message via the LOG_ERROR macro");
    log_tests_try_bus_single_dispatch();
    print_message("\n");
    LOG_ALWAYS(TAG_TEST, "This is a log test message via the LOG_ALWAYS macro");
    log_tests_try_bus_single_dispatch();
    print_message("\n");
}

static void test_log_init(void ** state) {
    print_message("\n******************************************\n");
    assert_non_null(&log_tests.bus);
    print_message("calling log_init()\n\n");
    log_init(&log_tests.bus, CNCBUS_ADDRESS_LOGGER, CNCBUS_SUBNET_MASK_CORE, NULL);
    log_tests.using_bus = true;
}

static int log_tests_bus_setup(void ** state) {
    log_tests.previous_log_level = log_get_min_level();

    cncbus_init(&log_tests.bus, api->mmap.cncbus.region, unit_test_guard_page_mode);

    log_receiver_init(&log_tests.bus, CNCBUS_ADDRESS_LOGGER);

    return 0;
}

static int log_tests_bus_teardown(void ** state) {
    log_shutdown();

    cncbus_dispatch(&log_tests.bus, cncbus_dispatch_flush);

    log_receiver_shutdown(&log_tests.bus);

    cncbus_destroy(&log_tests.bus);

    log_set_min_level(log_tests.previous_log_level);

    return 0;
}

int test_log() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_log_min_level_range, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_get_names, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_message_levels, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_tags, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_message_args, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_macros, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_init, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_message_levels, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_message_args, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_log_macros, NULL, NULL),
    };

    return cmocka_run_group_tests(tests, log_tests_bus_setup, log_tests_bus_teardown);
}
