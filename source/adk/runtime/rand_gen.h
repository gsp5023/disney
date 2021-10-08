/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "extern/xoroshiro256plusplus/xoroshiro256plusplus.h"
#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#include <stdint.h>

FFI_EXPORT FFI_PUB_CRATE typedef struct adk_rand_generator_t {
    xoroshiro256plusplus_t state;
} adk_rand_generator_t;

FFI_EXPORT FFI_PUB_CRATE adk_rand_generator_t adk_rand_create_generator();
FFI_EXPORT FFI_PUB_CRATE adk_rand_generator_t adk_rand_create_generator_with_seed(const uint64_t seed);
FFI_EXPORT FFI_PUB_CRATE uint64_t adk_rand_next(FFI_PTR_WASM adk_rand_generator_t * const generator);