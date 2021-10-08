/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 tread_pool_tests.c

 test fixture for thread_pool
 */

#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_thread.h"
#include "testapi.h"

enum {
    test_job_id_array_size = 100
};

static void basic_atomic_counter_test(void * user, thread_pool_t * const pool) {
    sb_atomic_int32_t * const counter = user;
    sb_atomic_fetch_add(counter, 1, memory_order_acquire);
}

typedef struct wait_test_user_args_t {
    milliseconds_t sleep_time;
    bool completion_handler_called;
} wait_test_user_args_t;

static void basic_wait_test(void * user, thread_pool_t * const pool) {
    // normally you wouldn't sleep in a test, but this also allows the incremeters to finish before the completion handler is invoked signaling that the test has completed.
    sb_thread_sleep(((wait_test_user_args_t *)user)->sleep_time);
}

static void main_thread_completion_handler(void * user, thread_pool_t * const pool) {
    ASSERT_IS_MAIN_THREAD();
    print_message("main thread completion handler called\n");
    ((wait_test_user_args_t *)user)->completion_handler_called = true;
}

static void thread_pool_unit_test(void ** ignored) {
    const mem_region_t region = MEM_REGION(.ptr = malloc(256 * 1024), .size = 256 * 1024);
    TRAP_OUT_OF_MEMORY(region.ptr);

    print_message("Creating thread pool...\n");
    struct thread_pool_t * pool = thread_pool_emplace_init(region, thread_pool_max_threads, "tst_thr_pol_", MALLOC_TAG);
    print_message("Created thread pool [%p] with [%i] threads\n", pool, thread_pool_max_threads);
    static const int max_counter_increments = 10;
    sb_atomic_int32_t atomic_counter = {.i32 = 0};

    for (int i = 0; i < max_counter_increments; ++i) {
        thread_pool_enqueue(pool, basic_atomic_counter_test, NULL, &atomic_counter);
    }
    print_message("created [%i] atomic incrementer jobs\n", max_counter_increments);

    wait_test_user_args_t wait_test_user = {.sleep_time = (milliseconds_t){100}, .completion_handler_called = false};
    thread_pool_enqueue(pool, basic_wait_test, main_thread_completion_handler, &wait_test_user);
    print_message("created a sleep test with a completion handler job\n");

    while (!wait_test_user.completion_handler_called) {
        thread_pool_run_completion_callbacks(pool);
    }

    assert_int_equal(sb_atomic_load(&atomic_counter, memory_order_acquire), max_counter_increments);
    assert_true(wait_test_user.completion_handler_called);

    print_message("Shutting down thread pool...\n");
    thread_pool_shutdown(pool, MALLOC_TAG);
    free(region.ptr);
}

int test_thread_pool() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(thread_pool_unit_test, NULL, NULL)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}