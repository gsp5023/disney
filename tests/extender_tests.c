/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/extender/extender.h"
#include "testapi.h"

static int extender_unit_test_setup(void ** state) {
    const char * const path = test_getargarg("--extensions");
    *state = (void *)path;
    return 0;
}

static void extender_unit_test(void ** state) {
    int some_event = 1977;
    assert_int_equal(extender_status_success, bind_extensions((const char *)(*state)));
    assert_int_equal(extender_status_success, start_extensions(NULL));
    assert_int_equal(extender_status_success, tick_extensions((void *)&some_event));
    assert_int_equal(extender_status_success, suspend_extensions());
    assert_int_equal(extender_status_success, resume_extensions());
    assert_int_equal(extender_status_success, stop_extensions());
    assert_int_equal(extender_status_success, unbind_extensions());
}

int test_extender() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(extender_unit_test)};

    return cmocka_run_group_tests(tests, extender_unit_test_setup, NULL);
}
