/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/crypto/crypto.h"
#include "testapi.h"

static void test_hmac(void ** state) {
    const uint8_t key[] = "secret";
    const uint8_t input[] = "hello, world!";
    uint8_t output[32];
    crypto_generate_hmac(
        CONST_MEM_REGION(.byte_ptr = key, .size = ARRAY_SIZE(key) - 1),
        CONST_MEM_REGION(.byte_ptr = input, .size = ARRAY_SIZE(input) - 1),
        output);

    uint8_t encoded[64];
    const size_t encoded_length = crypto_encode_base64(
        CONST_MEM_REGION(.byte_ptr = output, .size = ARRAY_SIZE(output)),
        MEM_REGION(.byte_ptr = encoded, .size = ARRAY_SIZE(encoded)));

    const uint8_t expected[] = "iRs1BMF9MFyn7aJs1FgmZCiBE9SOyWqcu01SmrcW8M4=";
    assert_memory_equal(encoded, expected, encoded_length);
}

static void test_hmac_incremental(void ** state) {
    const uint8_t key[] = "secret";
    const uint8_t input[] = "hello, world!";

    crypto_hmac_ctx_t ctx = {0};

    crypto_hmac_ctx_init(&ctx, CONST_MEM_REGION(.byte_ptr = key, .size = ARRAY_SIZE(key) - 1));

    crypto_hmac_ctx_update(&ctx, CONST_MEM_REGION(.byte_ptr = input, .size = ARRAY_SIZE(input) - 1));
    crypto_hmac_ctx_update(&ctx, (const_mem_region_t){0});
    crypto_hmac_ctx_update(&ctx, CONST_MEM_REGION(.byte_ptr = input, .size = ARRAY_SIZE(input) - 1));

    uint8_t output[32];
    crypto_hmac_ctx_finish(&ctx, output);

    uint8_t encoded[64];
    const size_t encoded_length = crypto_encode_base64(
        CONST_MEM_REGION(.byte_ptr = output, .size = ARRAY_SIZE(output)),
        MEM_REGION(.byte_ptr = encoded, .size = ARRAY_SIZE(encoded)));

    const uint8_t expected[] = "53B69fRcXVGFkbGhdvLsPP6lfguksFJ2jfn8zyVghY0=";
    assert_memory_equal(encoded, expected, encoded_length);
}

static void test_uuid_encoding(void ** state) {
    const sb_uuid_t uuid = {.id = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};

    char encoded_uuid[crypto_encoded_uuid_length];
    crypto_encode_uuid_as_string(&uuid, encoded_uuid);

    print_message("Encoded UUID: %s\n", encoded_uuid);

    const uint8_t expected[] = "01020304-0506-0708-090a-0b0c0d0e0f10";
    assert_string_equal(encoded_uuid, expected);
}

int test_crypto() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_hmac),
        cmocka_unit_test(test_hmac_incremental),
        cmocka_unit_test(test_uuid_encoding),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
