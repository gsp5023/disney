/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  watchdog_tests.c

  Set of tests to varify the correct watchdog thread behaviour
 */

#include "source/adk/app_thunk/watchdog.h"
#include "testapi.h"

typedef void (*assert_failed_hook_t)(const char * const message, const char * const filename, const char * const function, const int line);

enum {
    default_suspend_threshold = 30u,
    default_warning_period_ms = 100u,
    default_fatal_period_ms = 3000u
};

static struct {
    /* 
       Watchdog test overrides the global hook configured in tests/main.c to capture the call. Can't use cmocka's 
       mock_assert cause that doesn't seem to work across multiple threads.
     */
    assert_failed_hook_t hook;

    bool assert_triggered;
    sb_mutex_t * mutex;
    sb_condition_variable_t * cv;
} statics;

extern assert_failed_hook_t __assert_failed_hook;

static void watchdog_mock_assert(const char * const message, const char * const filename, const char * const function, const int line) {
    statics.assert_triggered = true;
    sb_condition_wake_all(statics.cv);
}

static int watchdog_tests_setup(void ** state) {
    statics.hook = __assert_failed_hook;
    __assert_failed_hook = watchdog_mock_assert;

    statics.mutex = sb_create_mutex(MALLOC_TAG);
    statics.cv = sb_create_condition_variable(MALLOC_TAG);

    return 0;
}

static int watchdog_tests_teardown(void ** state) {
    __assert_failed_hook = statics.hook;

    sb_destroy_mutex(statics.mutex, MALLOC_TAG);
    sb_destroy_condition_variable(statics.cv, MALLOC_TAG);

    return 0;
}

static bool check_assert_triggered(const uint32_t wait_for_ms) {
    bool timeout_error = false;
    bool assert_triggered = false;

    sb_lock_mutex(statics.mutex);
    {
        while (statics.assert_triggered == false && !timeout_error) {
            timeout_error = !sb_wait_condition(statics.cv, statics.mutex, (milliseconds_t){.ms = wait_for_ms});
        }

        assert_triggered = statics.assert_triggered;
    }
    sb_unlock_mutex(statics.mutex);

    return !timeout_error && assert_triggered;
}

static void watchdog_pass(void ** state) {
    statics.assert_triggered = false;

    watchdog_t watchdog;
    watchdog_start(&watchdog, default_suspend_threshold, default_warning_period_ms, default_fatal_period_ms);

    sb_thread_sleep((milliseconds_t){.ms = 16u});
    watchdog_tick(&watchdog);

    watchdog_shutdown(&watchdog);

    assert_false(check_assert_triggered(1500u));
}

static void watchdog_traps_unresponsive_thread(void ** state) {
    statics.assert_triggered = false;

    watchdog_t watchdog;
    watchdog_start(&watchdog, default_suspend_threshold, default_warning_period_ms, default_fatal_period_ms);

    watchdog_tick(&watchdog);
    sb_thread_sleep((milliseconds_t){.ms = 3500u});

    watchdog_shutdown(&watchdog);

    assert_true(check_assert_triggered(1500u));
}

int test_watchdog() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(watchdog_pass),
        cmocka_unit_test(watchdog_traps_unresponsive_thread)};

    return cmocka_run_group_tests(tests, watchdog_tests_setup, watchdog_tests_teardown);
}
