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
    const char * const expected_partner_name,
    const char * const expected_partner_guid,
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
        assert_string_equal(mapping.partner_name, expected_partner_name);
        assert_string_equal(mapping.partner_guid, expected_partner_guid);
    }
}

static void test_persona_mapping(void ** state) {
    print_message("===== TESTING EXPECTED FAILURE =====\n");
    test_persona_parsing("tests/persona/persona.json", "missing", false, "launchpad", "123", "", "", NULL, "explicit persona id not found");
    test_persona_parsing("tests/persona/persona-bad-default.json", "", false, "launchpad", "123", "", "", NULL, "default persona not found");
    test_persona_parsing("tests/persona/missing.json", "persona1", false, "launchpad", "123", "", "", NULL, "missing persona file");
    test_persona_parsing("tests/persona/persona-empty.json", "persona1", false, "launchpad", "123", "", "", NULL, "empty persona file");
    test_persona_parsing("tests/persona/persona-empty-default.json", "", false, "launchpad", "123", "", "", NULL, "empty default persona");
    test_persona_parsing("tests/persona/persona-empty-personas.json", "persona1", false, "launchpad", "123", "", "", NULL, "empty persona array");
    test_persona_parsing("tests/persona/persona-empty-partner.json", "persona1", false, "launchpad", "123", "", "", NULL, "empty parner");
    test_persona_parsing("tests/persona/persona-invalid.json", "persona1", false, "launchpad", "123", "", "", NULL, "invalid JSON in persona file");
    test_persona_parsing("tests/persona/persona-no-default.json", "", false, "launchpad", "123", "", "", NULL, "missing default field");
    test_persona_parsing("tests/persona/persona-no-id.json", "", false, "launchpad", "123", "", "", NULL, "missing id field");
    test_persona_parsing("tests/persona/persona-no-personas.json", "persona1", false, "launchpad", "123", "", "", NULL, "missing personas field");
    test_persona_parsing("tests/persona/persona-no-url.json", "persona1", false, "launchpad", "123", "", "", NULL, "missing manifest_url field");
    test_persona_parsing("tests/persona/persona-no-v1.json", "persona1", false, "launchpad", "123", "", "", NULL, "missing v1 field");
    test_persona_parsing("tests/persona/persona-no-partner.json", "persona1", false, "launchpad", "123", "", "", NULL, "missing parner field");

    print_message("===== TESTING EXPECTED SUCCESS =====\n");
    test_persona_parsing("tests/persona/persona.json", "", true, "launchpad", "123", "persona1", "manifest_url_success1", "default-locale-only\n", "default persona");
    test_persona_parsing("tests/persona/persona.json", "persona1", true, "launchpad", "123", "persona1", "manifest_url_success1", "default-locale-only\n", "explict persona id in first location");
    test_persona_parsing("tests/persona/persona.json", "persona2", true, "launchpad", "123", "persona2", "manifest_url_success2", "user-locale-no-defaults", "explict persona id in second location");
    test_persona_parsing("tests/persona/persona.json", "persona3", true, "launchpad", "123", "persona3", "manifest_url_success3", NULL, "explict persona id in third location");
    test_persona_parsing("tests/persona/persona.json", "persona4", true, "launchpad", "123", "persona4", "manifest_url_success4", NULL, "explict persona id in fourth location");
    test_persona_parsing("tests/persona/persona.json", "persona5", true, "launchpad", "123", "persona5", "manifest_url_success5", "old error_message supported", "explict persona id in fourth location");
    test_persona_parsing("tests/persona/persona.json", "persona6", true, "launchpad", "123", "persona6", "manifest_url_success6", "newer error overrides\n", "explict persona id in fourth location");
}

int test_persona() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_persona_mapping)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
