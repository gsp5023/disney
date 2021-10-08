/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"
#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT FFI_UNSAFE static inline void memcpy_n2r(
    FFI_SLICE FFI_PTR_WASM void * const dst,
    FFI_PTR_NATIVE const void * const src,
    const int32_t offset,
    const int32_t num_bytes) {
    memcpy(dst, (char *)src + offset, num_bytes);
}

FFI_EXPORT FFI_NO_RUST_THUNK static inline int32_t cstrlen(FFI_PTR_NATIVE const char * const str) {
    return (int32_t)strlen(str);
}

FFI_EXPORT FFI_NO_RUST_THUNK static inline int32_t strcpy_n2r(
    FFI_PTR_WASM char * const dst,
    const int32_t dst_buff_size,
    FFI_PTR_NATIVE const char * const src) {
    const int32_t amount_written = sprintf_s(dst, dst_buff_size, "%s", src);
    ASSERT(amount_written != -1);
    return amount_written;
}

FFI_EXPORT FFI_NO_RUST_THUNK static inline int32_t strcpy_n2r_upto(
    FFI_PTR_WASM char * const dst,
    const int32_t dst_buff_size,
    FFI_PTR_NATIVE const char * const src,
    const int32_t max_copy_len) {
    const int32_t amount_written = sprintf_s(dst, dst_buff_size, "%.*s", max_copy_len, src);
    ASSERT(amount_written != -1);
    return amount_written;
}

#ifdef __cplusplus
}
#endif
