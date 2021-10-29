/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/steamboat/sb_display.h"
#include "testapi.h"

static void test_invalid_display_index(void ** state) {
    sb_enumerate_display_modes_result_t display_result = {0};
    assert_false(sb_enumerate_display_modes(-1, 0, &display_result));

    assert_int_equal(display_result.display_mode.width, 0);
    assert_int_equal(display_result.display_mode.height, 0);
    assert_int_equal(display_result.display_mode.hz, 0);

    assert_int_equal(display_result.status, 0);
}

static void test_invalid_display_mode(void ** state) {
    sb_enumerate_display_modes_result_t display_result = {0};
    assert_false(sb_enumerate_display_modes(0, -1, &display_result));

    assert_int_equal(display_result.display_mode.width, 0);
    assert_int_equal(display_result.display_mode.height, 0);
    assert_int_equal(display_result.display_mode.hz, 0);

    assert_int_equal(display_result.status, 0);
}

static void test_enumerate_display_modes(void ** state) {
    sb_enumerate_display_modes_result_t display_result = {0};
    assert_true(sb_enumerate_display_modes(0, 0, &display_result));

    assert_true(display_result.display_mode.width > 0);
    assert_true(display_result.display_mode.height > 0);
    assert_true(display_result.display_mode.hz > 0);

    assert_true(display_result.status > 0);
}

// NOTE: order of tests matters due to `display_mode_count`

int test_display() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_invalid_display_index),
        cmocka_unit_test(test_invalid_display_mode),
        cmocka_unit_test(test_enumerate_display_modes),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
