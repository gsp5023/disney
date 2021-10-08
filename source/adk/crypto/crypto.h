/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "extern/mbedtls/mbedtls/include/mbedtls/base64.h"
#include "extern/mbedtls/mbedtls/include/mbedtls/md.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    crypto_hmac256_base64_max_size = 64,
    crypto_hmac256_hex_size = 65,
    crypto_sha256_size = 32,
    crypto_encoded_uuid_length = 37
};

void crypto_generate_hmac(
    const const_mem_region_t key,
    const const_mem_region_t input,
    uint8_t output[crypto_sha256_size]);

typedef struct crypto_hmac_ctx_t {
    mbedtls_md_context_t ctx;
} crypto_hmac_ctx_t;

void crypto_hmac_ctx_init(crypto_hmac_ctx_t * const ctx, const const_mem_region_t key);

void crypto_hmac_ctx_update(crypto_hmac_ctx_t * const ctx, const const_mem_region_t input);

void crypto_hmac_ctx_finish(crypto_hmac_ctx_t * const ctx, uint8_t output[crypto_sha256_size]);

size_t crypto_encode_base64(const const_mem_region_t input, const mem_region_t output);

size_t crypto_decode_base64(const const_mem_region_t input, const mem_region_t output);

/// encodes input as ASCII hex characters, with trailing NUL
uint8_t * crypto_encode_hex(const const_mem_region_t input, const mem_region_t output);

FFI_EXPORT
typedef struct crypto_hmac_hex_t {
    uint8_t bytes[crypto_hmac256_hex_size];
} crypto_hmac_hex_t;

crypto_hmac_hex_t crypto_compute_hmac_hex(const const_mem_region_t buffer);

void crypto_encode_uuid_as_string(const sb_uuid_t * const uuid, char str[crypto_encoded_uuid_length]);

#ifdef __cplusplus
}
#endif
