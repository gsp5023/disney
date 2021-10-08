/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "crypto.h"

void crypto_generate_hmac(
    const const_mem_region_t key,
    const const_mem_region_t input,
    uint8_t output[crypto_sha256_size]) {
    const int status = mbedtls_md_hmac(
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
        key.byte_ptr,
        key.size,
        input.byte_ptr,
        input.size,
        output);

    VERIFY_MSG(status == 0, "Failed to generate HMAC: [%d]", status);
}

void crypto_hmac_ctx_init(crypto_hmac_ctx_t * const ctx, const const_mem_region_t key) {
    ASSERT(ctx != NULL);

    mbedtls_md_init(&ctx->ctx);

    const int setup_status = mbedtls_md_setup(&ctx->ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), true);
    VERIFY_MSG(setup_status == 0, "Failed to setup HMAC MD: [%d]", setup_status);

    const int starts_status = mbedtls_md_hmac_starts(&ctx->ctx, key.byte_ptr, key.size);
    VERIFY_MSG(starts_status == 0, "Failed to start HMAC MD: [%d]", starts_status);
}

void crypto_hmac_ctx_update(crypto_hmac_ctx_t * const ctx, const const_mem_region_t input) {
    ASSERT(ctx != NULL);

    const int status = mbedtls_md_hmac_update(&ctx->ctx, input.byte_ptr, input.size);
    VERIFY_MSG(status == 0, "Failed to update HMAC MD: [%d]", status);
}

void crypto_hmac_ctx_finish(crypto_hmac_ctx_t * const ctx, uint8_t output[crypto_sha256_size]) {
    ASSERT(ctx != NULL);

    const int status = mbedtls_md_hmac_finish(&ctx->ctx, output);
    VERIFY_MSG(status == 0, "Failed to finish HMAC MD: [%d]", status);

    mbedtls_md_free(&ctx->ctx);
}

size_t crypto_encode_base64(const const_mem_region_t input, const mem_region_t output) {
    size_t length;
    const int status = mbedtls_base64_encode(
        output.byte_ptr,
        output.size,
        &length,
        input.byte_ptr,
        input.size);

    VERIFY_MSG(status == 0, "Failed to encode as base64: [%d]", status);

    return length;
}

size_t crypto_decode_base64(const const_mem_region_t input, const mem_region_t output) {
    size_t length;
    const int status = mbedtls_base64_decode(
        output.byte_ptr,
        output.size,
        &length,
        input.byte_ptr,
        input.size);

    VERIFY_MSG(status == 0, "Failed to decode from base64: [%d]", status);

    return length;
}

#ifndef _DEVICE_ID_KEY
#define _DEVICE_ID_KEY "Dje2/XcY9UQTheBdIV5W1o47WcWLLPBf9pzGk6abKT3qLZYhdiocVxbGjQz8WDpeqqP4iwzCi7yuXKB4Fmkw8w=="
#endif

static const uint8_t device_signature_key[] = _DEVICE_ID_KEY;

uint8_t * crypto_encode_hex(const const_mem_region_t input, const mem_region_t output) {
    static const uint8_t codes[] = "0123456789abcdef";
    size_t index;

    ASSERT(output.size);
    ASSERT(((input.size * 2) + 1) <= output.size);
    output.byte_ptr[input.size * 2] = 0; // trailing NUL
    for (index = 0; index < input.size; index++) {
        output.byte_ptr[index * 2 + 0] = codes[(input.byte_ptr[index] >> 4) & 0x0F];
        output.byte_ptr[index * 2 + 1] = codes[(input.byte_ptr[index]) & 0x0F];
    }
    return output.byte_ptr;
}

crypto_hmac_hex_t crypto_compute_hmac_hex(const const_mem_region_t buffer) {
    const const_mem_region_t device_signature_key_region = CONST_MEM_REGION(.byte_ptr = device_signature_key, .size = ARRAY_SIZE(device_signature_key) - 1);
    uint8_t decoded_device_key[1024];
    const size_t decoded_length = crypto_decode_base64(
        device_signature_key_region,
        MEM_REGION(.byte_ptr = decoded_device_key, .size = ARRAY_SIZE(decoded_device_key)));
    VERIFY(decoded_length <= ARRAY_SIZE(decoded_device_key));

    uint8_t output[crypto_sha256_size];
    crypto_generate_hmac(
        CONST_MEM_REGION(.byte_ptr = decoded_device_key, .size = decoded_length),
        CONST_MEM_REGION(.byte_ptr = buffer.byte_ptr, .size = buffer.size),
        output);
    crypto_hmac_hex_t encoded;
    const mem_region_t encoded_region2 = MEM_REGION(.byte_ptr = encoded.bytes, .size = ARRAY_SIZE(encoded.bytes));
    const const_mem_region_t output_region2 = CONST_MEM_REGION(.byte_ptr = output, .size = ARRAY_SIZE(output));
    const uint8_t * p = crypto_encode_hex(output_region2, encoded_region2);

    VERIFY(p == encoded_region2.byte_ptr);

    return encoded;
}

void crypto_encode_uuid_as_string(const sb_uuid_t * const uuid, char str[crypto_encoded_uuid_length]) {
#define B(X) (uint8_t)(uuid->id[(X)])
    snprintf(
        str,
        crypto_encoded_uuid_length,
        "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        B(0),
        B(1),
        B(2),
        B(3),
        B(4),
        B(5),
        B(6),
        B(7),
        B(8),
        B(9),
        B(10),
        B(11),
        B(12),
        B(13),
        B(14),
        B(15));
#undef B
}
