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

void generate_random_ints(int * const ints, const int count) {
    for (int i = 0; i < count; ++i) {
        ints[i] = rand();
    }
}

void generate_random_ints_without_boundary_values(int * const ints, const int count) {
    for (int i = 0; i < count; ++i) {
        ints[i] = rand();
        if ((ints[i] == INT32_MAX) || (ints[i] == INT32_MIN)) {
            i--; // Do Over, to get a non-boundary value
        }
    }
}

static void test_algorithm_sort(void ** state) {
    enum { num_ints = 17872 };
    int * const ints = (int *)malloc(sizeof(int) * num_ints);
    TRAP_OUT_OF_MEMORY(ints);

    // Generate random ints, ensuring they are not coincidentally sorted
    do {
        generate_random_ints(ints, num_ints);
    } while (is_sorted(ints, ints + num_ints, compare_less_int));

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

static void test_algorithm_heap(void ** state) {
    enum { num_ints = 17872 };
    int * const ints = (int *)malloc(sizeof(int) * num_ints);
    TRAP_OUT_OF_MEMORY(ints);

    // Generate random ints, ensuring they are not coincidentally sorted
    do {
        generate_random_ints(ints, num_ints);
    } while (is_sorted(ints, ints + num_ints, compare_less_int));

    make_heap_int(ints, ints + num_ints);
    sort_heap_int(ints, ints + num_ints);
    assert_true(is_sorted(ints, ints + num_ints, compare_less_int));

    free(ints);
}

static void test_algorithm_lower_bound(void ** state) {
    enum { num_ints = 17872 };
    int * ints = (int *)malloc(sizeof(int) * num_ints);
    TRAP_OUT_OF_MEMORY(ints);

    generate_random_ints_without_boundary_values(ints, num_ints);

    sort_int(ints, ints + num_ints);
    assert_true(is_sorted(ints, ints + num_ints, compare_less_int));

    assert_true(*lower_bound_int(ints, ints + num_ints, ints[0]) == ints[0]);
    assert_true(*lower_bound_int(ints, ints + num_ints, ints[num_ints - 1]) == ints[num_ints - 1]);
    assert_true(*lower_bound_int(ints, ints + num_ints, ints[num_ints / 2]) == ints[num_ints / 2]);
    assert_true(*lower_bound_int(ints, ints + num_ints, ints[num_ints / 2] - 1) <= ints[num_ints / 2]);
    assert_true(*lower_bound_int(ints, ints + num_ints, ints[num_ints / 2] + 1) > ints[num_ints / 2]);
    assert_true(*lower_bound_int(ints, ints + num_ints, ints[0] - 1) == ints[0]);
    assert_true(lower_bound_int(ints, ints + num_ints, ints[num_ints - 1] + 1) == ints + num_ints);
    // Same as original, but const
    assert_true(*lower_bound_const_int(ints, ints + num_ints, ints[0]) == ints[0]);
    assert_true(*lower_bound_const_int(ints, ints + num_ints, ints[num_ints - 1]) == ints[num_ints - 1]);
    assert_true(*lower_bound_const_int(ints, ints + num_ints, ints[num_ints / 2]) == ints[num_ints / 2]);
    assert_true(*lower_bound_const_int(ints, ints + num_ints, ints[num_ints / 2] - 1) <= ints[num_ints / 2]);
    assert_true(*lower_bound_const_int(ints, ints + num_ints, ints[num_ints / 2] + 1) > ints[num_ints / 2]);
    assert_true(*lower_bound_const_int(ints, ints + num_ints, ints[0] - 1) == ints[0]);
    assert_true(lower_bound_const_int(ints, ints + num_ints, ints[num_ints - 1] + 1) == ints + num_ints);
    // Same as original, but with predicate
    assert_true(*lower_bound_with_predicate_int(ints, ints + num_ints, ints[0], compare_less_int) == ints[0]);
    assert_true(*lower_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints - 1], compare_less_int) == ints[num_ints - 1]);
    assert_true(*lower_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints / 2], compare_less_int) == ints[num_ints / 2]);
    assert_true(*lower_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints / 2] - 1, compare_less_int) <= ints[num_ints / 2]);
    assert_true(*lower_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints / 2] + 1, compare_less_int) > ints[num_ints / 2]);
    assert_true(*lower_bound_with_predicate_int(ints, ints + num_ints, ints[0] - 1, compare_less_int) == ints[0]);
    assert_true(lower_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints - 1] + 1, compare_less_int) == ints + num_ints);

    free(ints);
}

static void test_algorithm_upper_bound(void ** state) {
    enum { num_ints = 17872 };
    int * ints = (int *)malloc(sizeof(int) * num_ints);
    TRAP_OUT_OF_MEMORY(ints);

    generate_random_ints_without_boundary_values(ints, num_ints);

    sort_int(ints, ints + num_ints);
    assert_true(is_sorted(ints, ints + num_ints, compare_less_int));

    assert_true(*upper_bound_int(ints, ints + num_ints, ints[0]) > ints[0]);
    assert_true(upper_bound_int(ints, ints + num_ints, ints[num_ints - 1]) == ints + num_ints);
    assert_true(*upper_bound_int(ints, ints + num_ints, ints[num_ints / 2]) > ints[num_ints / 2]);
    assert_true(*upper_bound_int(ints, ints + num_ints, ints[num_ints / 2] - 1) == ints[num_ints / 2]);
    assert_true(*upper_bound_int(ints, ints + num_ints, ints[num_ints / 2] + 1) > ints[num_ints / 2]);
    assert_true(*upper_bound_int(ints, ints + num_ints, ints[0] - 1) == ints[0]);
    assert_true(*upper_bound_int(ints, ints + num_ints, ints[num_ints - 1] - 1) == ints[num_ints - 1]);
    // Same as original, but const
    assert_true(*upper_bound_const_int(ints, ints + num_ints, ints[0]) > ints[0]);
    assert_true(upper_bound_const_int(ints, ints + num_ints, ints[num_ints - 1]) == ints + num_ints);
    assert_true(*upper_bound_const_int(ints, ints + num_ints, ints[num_ints / 2]) > ints[num_ints / 2]);
    assert_true(*upper_bound_const_int(ints, ints + num_ints, ints[num_ints / 2] - 1) == ints[num_ints / 2]);
    assert_true(*upper_bound_const_int(ints, ints + num_ints, ints[num_ints / 2] + 1) > ints[num_ints / 2]);
    assert_true(*upper_bound_const_int(ints, ints + num_ints, ints[0] - 1) == ints[0]);
    assert_true(*upper_bound_const_int(ints, ints + num_ints, ints[num_ints - 1] - 1) == ints[num_ints - 1]);
    // Same as original, but with predicate
    assert_true(*upper_bound_with_predicate_int(ints, ints + num_ints, ints[0], compare_less_int) > ints[0]);
    assert_true(upper_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints - 1], compare_less_int) == ints + num_ints);
    assert_true(*upper_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints / 2], compare_less_int) > ints[num_ints / 2]);
    assert_true(*upper_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints / 2] - 1, compare_less_int) == ints[num_ints / 2]);
    assert_true(*upper_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints / 2] + 1, compare_less_int) > ints[num_ints / 2]);
    assert_true(*upper_bound_with_predicate_int(ints, ints + num_ints, ints[0] - 1, compare_less_int) == ints[0]);
    assert_true(*upper_bound_with_predicate_int(ints, ints + num_ints, ints[num_ints - 1] - 1, compare_less_int) == ints[num_ints - 1]);

    free(ints);
}

int test_algorithms() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_algorithm_swap),
        cmocka_unit_test(test_algorithm_sort),
        cmocka_unit_test(test_algorithm_heap),
        cmocka_unit_test(test_algorithm_lower_bound),
        cmocka_unit_test(test_algorithm_upper_bound),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
