/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
locale_tests.c

locale test fixture
*/

#include "source/adk/steamboat/sb_locale.h"
#include "testapi.h"

void locale_unit_test(void ** state) {
#if defined(_VADER) || defined(_LEIA)
    sb_locale_t locale = sb_get_locale();
    assert_true(strlen(locale.language) > 0);
    assert_true(strlen(locale.region) > 0);

    print_message("current language: %s\n", locale.language);
    print_message("current region: %s\n", locale.region);
#endif
}

int test_locale() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(locale_unit_test, NULL, NULL)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
