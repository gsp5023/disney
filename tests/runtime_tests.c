/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
runtime_tests.c

fringe runtime coverage test fixture

The following online CRC calculator was used to compare results
http://www.sunshine2k.de/coding/javascript/crc/crc_js.html

The following online NMEA Checksum Calculator was used to compare results
https://nmeachecksum.eqth.net/

*/
#include "source/adk/runtime/crc.h"
#include "source/adk/runtime/hosted_app.h"
#include "source/adk/runtime/memory.h"
#include "testapi.h"

static void crc_unit_test_2(void ** state) {
    static const char * test_string = "The only thing predictable about life is its unpredictability";
    static const char * test_update_string = ".  The End.";
    static const char * test_string_update_by_char = "."; // For fn that update one char at a time
    char temp_string[256];
    size_t test_string_len = strlen(test_string);
    size_t test_update_string_len = strlen(test_update_string);

    // crc_8
    const uint8_t expected_crc8 = 0xC2;
    assert_true(crc_8((uint8_t *)test_string, test_string_len) == expected_crc8);
    // update_crc_8 (one character append)
    ZEROMEM(temp_string);
    strcpy(temp_string, test_string);
    strcat(temp_string, test_string_update_by_char);
    assert_true(update_crc_8(expected_crc8, (uint8_t)(test_string_update_by_char[0])) == crc_8((uint8_t *)temp_string, test_string_len + 1));

    // crc_16
    const uint16_t expected_crc16 = 0xF9F4;
    assert_true(crc_16((uint8_t *)test_string, test_string_len) == expected_crc16);
    // update_crc_16 (one character append)
    ZEROMEM(temp_string);
    strcpy(temp_string, test_string);
    strcat(temp_string, test_string_update_by_char);
    assert_true(update_crc_16(expected_crc16, (uint8_t)(test_string_update_by_char[0])) == crc_16((uint8_t *)temp_string, test_string_len + 1));
    // crc_modbus
    const uint16_t expected_crc16_modbus = 0xB96F;
    assert_true(crc_modbus((uint8_t *)test_string, test_string_len) == expected_crc16_modbus);

    // crc_32
    const uint32_t expected_crc32 = 0x5F51349F;
    assert_true(crc_name_check(test_string, expected_crc32) == expected_crc32);
    assert_true(crc_str_32(test_string) == expected_crc32);
    assert_true(crc_32((uint8_t *)test_string, test_string_len) == expected_crc32);
    // update_crc_32 - crc 32 update function will not match, due to Final Xor (See ticket M5-2784)
    const uint32_t initial_crc32 = 0xA0AECB60; // test_string crc without the final Xor
    ZEROMEM(temp_string);
    strcpy(temp_string, test_string);
    strcat(temp_string, test_update_string);
    uint32_t expected_crc32_update = crc_32((uint8_t *)temp_string, test_string_len + test_update_string_len);
    assert_true(update_crc_str_32(initial_crc32, test_update_string) == expected_crc32_update);
    assert_true(update_crc_32(initial_crc32, (uint8_t *)test_update_string, test_update_string_len) == expected_crc32_update);

    // crc_64_ecma
    const uint64_t expected_crc64_ecma_182 = 0x6FEA9F81F907CC6D;
    assert_true(crc_64_ecma((uint8_t *)test_string, test_string_len) == expected_crc64_ecma_182);
    // crc_64_we
    const uint64_t expected_crc64_we = 0xF508EFD8CFCC9F73;
    assert_true(crc_64_we((uint8_t *)test_string, test_string_len) == expected_crc64_we);
    // update_crc_64_ecma (one character append)
    ZEROMEM(temp_string);
    strcpy(temp_string, test_string);
    strcat(temp_string, test_string_update_by_char);
    assert_true(update_crc_64_ecma(expected_crc64_ecma_182, (uint8_t)(test_string_update_by_char[0])) == crc_64_ecma((uint8_t *)temp_string, test_string_len + 1));

    // NEMA
    assert_true(checksum_NMEA(NULL, (uint8_t *)temp_string) == NULL);
    assert_true(checksum_NMEA((uint8_t *)"$GPGLL,5300.97914,N,00259.98174,E,125926,A", NULL) == NULL);
    assert_string_equal(checksum_NMEA((uint8_t *)"$GPGLL,5300.97914,N,00259.98174,E,125926,A", (uint8_t *)temp_string), "28");
}

static void crc_unit_test_1(void ** state) {
    static const char * test_string = "The only thing predictable about life is its unpredictability";
    size_t test_string_len = strlen(test_string);
    char temp_string[256];

    // calculate expected values of calculating the CRC on combined data + crc
    uint32_t test_mem = 0x00000000;
    uint8_t expected_crc8 = crc_8((uint8_t *)&test_mem, sizeof(test_mem));
    uint16_t expected_crc16 = crc_16((uint8_t *)&test_mem, sizeof(test_mem));
    uint32_t expected_crc32 = crc_32((uint8_t *)&test_mem, sizeof(test_mem));

    // calculate the crc8 of test_string, append it to test_string, and check
    // the crc8 of the combined string. This should be equal to crc8(0)
    uint8_t test_string_crc8 = crc_8((uint8_t *)test_string, test_string_len);
    ZEROMEM(temp_string);
    strcat(temp_string, test_string);
    memcpy(temp_string + test_string_len, (const char *)&test_string_crc8, sizeof(test_string_crc8));
    uint8_t crc_check8 = crc_8((uint8_t *)temp_string, test_string_len + sizeof(test_string_crc8));
    print_message("                crc 8:  calc = %d, init = %d\n", crc_check8, expected_crc8);
    assert_true(crc_check8 == expected_crc8);

    // calculate the crc16 of test_string, append it to test_string, and check
    // the crc16 of the combined string. This should be equal to crc16(0)
    uint16_t test_string_crc16 = crc_16((uint8_t *)test_string, test_string_len);
    ZEROMEM(temp_string);
    strcpy(temp_string, test_string);
    memcpy(temp_string + test_string_len, (const char *)&test_string_crc16, sizeof(test_string_crc16));
    uint16_t crc_check16 = crc_16((uint8_t *)temp_string, test_string_len + sizeof(test_string_crc16));
    print_message("                crc 16: calc = %d, init = %d\n", crc_check16, expected_crc16);
    assert_true(crc_check16 == expected_crc16);

    // calculate the crc32 of test_string, append it to test_string, and check
    // the crc32 of the combined string. This should be equal to crc32(0)
    uint32_t test_string_crc32 = crc_32((uint8_t *)test_string, test_string_len);
    ZEROMEM(temp_string);
    strcpy(temp_string, test_string);
    memcpy(temp_string + test_string_len, (const char *)&test_string_crc32, sizeof(test_string_crc32));
    uint32_t crc_check32 = crc_32((uint8_t *)temp_string, test_string_len + sizeof(test_string_crc32));
    print_message("                crc 32: calc = %d, init = %d\n", crc_check32, expected_crc32);
    assert_true(crc_check32 == expected_crc32);
}

static void adk_app_metrics_unit_test(void ** state) {
    adk_app_metrics_t app_info;

    // Expect no app
    assert_true(adk_app_metrics_get(&app_info) == adk_app_metrics_no_app);
    adk_app_metrics_report("AppIdAppIdAppIdAppIdAppId", "AppNameAppNameAppNameAppName", "AppVersionAppVersionAppVersion");
    assert_true(adk_app_metrics_get(&app_info) == adk_app_metrics_success);
    assert_string_equal(app_info.app_id, "AppIdAppIdAppIdAppIdAppId");
    assert_string_equal(app_info.app_name, "AppNameAppNameAppNameAppName");
    assert_string_equal(app_info.app_version, "AppVersionAppVersionAppVersion");
    // Reset back to no app
    adk_app_metrics_clear();
    assert_true(adk_app_metrics_get(&app_info) == adk_app_metrics_no_app);
}

static void strcat_s_unit_test(void ** state) {
    char str1[10] = "";

    strcat_s(str1, 10, "1234");
    strcat_s(str1, 10, "56789");
    assert_string_equal(str1, "123456789");
}

static void strcpy_s_unit_test(void ** state) {
    char str1[10] = "123456789", str2[10];

    strcpy_s(str2, 10, str1);
    assert_string_equal(str2, "123456789");
}

static void sprintf_s_unit_test(void ** state) {
    char str1[10] = "";

    assert_true(sprintf_s(str1, 10, "12345%d", 9876) == 9);
    assert_string_equal(str1, "123459876");
}

int test_runtime() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(crc_unit_test_1),
        cmocka_unit_test(crc_unit_test_2),
        cmocka_unit_test(adk_app_metrics_unit_test),
        cmocka_unit_test(strcat_s_unit_test),
        cmocka_unit_test(strcpy_s_unit_test),
        cmocka_unit_test(sprintf_s_unit_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
