/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
text_to_speech_tests.c

Test for Text-to-Speech 
*/

#include "source/adk/steamboat/sb_platform.h"
#include "testapi.h"

static void text_to_speech_unit_test(void ** state) {
    const char * sample = "This is a test";
    print_message("Invoking text-to-speech on string: %s\n", sample);
    sb_text_to_speech(sample);
}

int test_text_to_speech() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(text_to_speech_unit_test, NULL, NULL)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
