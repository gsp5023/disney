/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wasm_ptr_t {
    int32_t ofs;
} wasm_ptr_t;

typedef struct wasm_bool_t {
    int32_t b;
} wasm_bool_t;

typedef uint64_t native_ptr_t;

#ifdef __cplusplus
}
#endif
