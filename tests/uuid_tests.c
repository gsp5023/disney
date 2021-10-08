/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
uuid_tests.c

test fixture for UUID generation
*/

#include "source/adk/steamboat/sb_platform.h"
#include "testapi.h"

static void test_uuid_generation(void ** state) {
    sb_uuid_t uuid = sb_generate_uuid();

    bool found_non_zero = false;
    for (size_t i = 0; i < ARRAY_SIZE(uuid.id); ++i) {
        if (uuid.id[i] != 0) {
            found_non_zero = true;
            break;
        }
    }

    // cast/pointer access to defeat type-puning compiler error on 97425 in release
    const char * const uuid_b = (const char *)uuid.id;

    // format: 8-4-4-4-12
    print_message("Generated uuid: [%x-%x-%x-%x-%x%x]\n",
                  *(uint32_t *)(uuid_b + 0), // 8
                  *(uint16_t *)(uuid_b + 4), // 4
                  *(uint16_t *)(uuid_b + 6), // 4
                  *(uint16_t *)(uuid_b + 8), // 4
                  *(uint16_t *)(uuid_b + 10), // 12(4)
                  *(uint32_t *)(uuid_b + 12)); // 12(8)

    assert_true(found_non_zero);
}

int test_uuid() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_uuid_generation, NULL, NULL),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
