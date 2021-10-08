/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/log/log.h"
#include "source/adk/persona/persona.h"
#include "testapi.h"

void test_persona_parsing(
    const char * const persona_file,
    const char * const persona_id,
    bool expected_success,
    const char * const expected_id,
    const char * const expected_url,
    const char * const expected_error_message,
    const char * const test_description) {
    print_message("===== %s =====\n", test_description);

    persona_mapping_t mapping;
    strcpy_s(mapping.file, ARRAY_SIZE(mapping.file), persona_file);
    strcpy_s(mapping.id, ARRAY_SIZE(mapping.id), persona_id);
    mapping.manifest_url[0] = 0;
    mapping.fallback_error_message[0] = 0;

    assert_true(get_persona_mapping(&mapping) == expected_success);

    if (expected_error_message) {
        assert_string_equal(mapping.fallback_error_message, expected_error_message);
    } else {
        assert_true(mapping.fallback_error_message[0] == 0);
    }

    if (expected_success) {
        assert_string_equal(mapping.id, expected_id);
        assert_string_equal(mapping.manifest_url, expected_url);
    }
}

static void test_persona_mapping(void ** state) {
    print_message("===== TESTING EXPECTED FAILURE =====\n");
    test_persona_parsing("tests/persona/persona.json", "missing", false, "", "", NULL, "explicit persona id not found");
    test_persona_parsing("tests/persona/persona-bad-default.json", "", false, "", "", NULL, "default persona not found");
    test_persona_parsing("tests/persona/missing.json", "persona1", false, "", "", NULL, "missing persona file");
    test_persona_parsing("tests/persona/persona-empty.json", "persona1", false, "", "", NULL, "empty persona file");
    test_persona_parsing("tests/persona/persona-empty-default.json", "", false, "", "", NULL, "empty default persona");
    test_persona_parsing("tests/persona/persona-empty-personas.json", "persona1", false, "", "", NULL, "empty persona array");
    test_persona_parsing("tests/persona/persona-invalid.json", "persona1", false, "", "", NULL, "invalid JSON in persona file");
    test_persona_parsing("tests/persona/persona-no-default.json", "", false, "", "", NULL, "missing default field");
    test_persona_parsing("tests/persona/persona-no-id.json", "", false, "", "", NULL, "missing id field");
    test_persona_parsing("tests/persona/persona-no-personas.json", "persona1", false, "", "", NULL, "missing personas field");
    test_persona_parsing("tests/persona/persona-no-url.json", "persona1", false, "", "", NULL, "missing manifest_url field");
    test_persona_parsing("tests/persona/persona-no-v1.json", "persona1", false, "", "", NULL, "missing v1 field");

    print_message("===== TESTING EXPECTED SUCCESS =====\n");
    test_persona_parsing("tests/persona/persona.json", "", true, "persona1", "manifest_url_success1", "test_message1", "default persona");
    test_persona_parsing("tests/persona/persona.json", "persona1", true, "persona1", "manifest_url_success1", "test_message1", "explict persona id in first location");
    test_persona_parsing("tests/persona/persona.json", "persona2", true, "persona2", "manifest_url_success2", "test_message2", "explict persona id in second location");
    test_persona_parsing("tests/persona/persona.json", "persona3", true, "persona3", "manifest_url_success3", NULL, "explict persona id in third location");
}

int test_persona() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_persona_mapping)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
