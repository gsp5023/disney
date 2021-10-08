/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
coredump_tests.c

test fixture for core dump data collection
*/

#include "source/adk/coredump/coredump.h"
#include "testapi.h"

static void test_coredump_add_data(void ** state) {
    int coredump_stack_size = 16 * 1024 * 1024;
    adk_coredump_init(coredump_stack_size);

    // add inital value
    adk_coredump_add_data_public("name", "value");

    adk_coredump_data_t * data;
    adk_coredump_get_data(&data);

    assert_true(strcmp(data->name, "name") == 0);
    assert_true(strcmp(data->value, "value") == 0);

    // update value with same name
    adk_coredump_add_data_public("name", "newvalue");

    assert_true(strcmp(data->name, "name") == 0);
    assert_true(strcmp(data->value, "newvalue") == 0);

    // append data
    adk_coredump_add_data_public("test", "data");

    assert_true(strcmp(data->next->name, "test") == 0);
    assert_true(strcmp(data->next->value, "data") == 0);

    // append data and validate count
    adk_coredump_add_data_public("test1", "data");
    adk_coredump_add_data_public("test2", "data");
    adk_coredump_add_data_public("test1", "rdata");

    // add private data not shared with partners
    adk_coredump_add_data("private", "data");

    int count = 0;
    while (data != NULL) {
        count++;
        data = data->next;
    }

#ifdef SB_COREDUMP_PRIVATE_DATA_ENABLED
    assert_true(count == 5);
#else
    assert_true(count == 4);
#endif

    adk_coredump_shutdown();
}

int test_coredump() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_coredump_add_data),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
