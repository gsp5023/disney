/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
algorithm_test.c

algorithm library test fixture
*/
#include "testapi.h"

#define ALGO_TYPE int
#include "source/adk/runtime/algorithm.h"
#define ALGO_IMPL int
static bool compare_less_int(const int a, const int b) {
    return a < b;
}
#include "source/adk/runtime/algorithm.h"

static bool is_sorted(const int * const begin, const int * const end, bool (*pred)(const int a, const int b)) {
    for (const int * i = begin; i < end - 1; ++i) {
        if (pred(i[1], i[0])) {
            return false;
        }
    }

    return true;
}

static int * random_ints(const int count) {
    int * ints = (int *)malloc(sizeof(int) * count);
    TRAP_OUT_OF_MEMORY(ints);

    for (int i = 0; i < count; ++i) {
        ints[i] = rand();
    }

    assert_false(is_sorted(ints, ints + count, compare_less_int));
    return ints;
}

static void test_algorithm_sort(void ** state) {
    enum { num_ints = 17872 };

    int * ints = random_ints(num_ints);
    sort_int(ints, ints + num_ints);
    assert_true(is_sorted(ints, ints + num_ints, compare_less_int));
    free(ints);
}

static void test_algorithm_swap(void ** state) {
    int x = 1;
    int y = 2;
    swap_int(&x, &y);
    assert_true(x == 2);
    assert_true(y == 1);
}

int test_algorithms() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_algorithm_swap, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_algorithm_sort, NULL, NULL),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}