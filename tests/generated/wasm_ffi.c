/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

#include "source/adk/runtime/runtime.h"
#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

FFI_EXPORT static void wasm_test_func_void() {
}

FFI_EXPORT static void wasm_test_func_i(int32_t arg0) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_i because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
}

FFI_EXPORT static void wasm_test_func_ii(int32_t arg0, int32_t arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_ii because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
}

FFI_EXPORT static void wasm_test_func_iii(int32_t arg0, int32_t arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iii because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_iii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_iii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
}

FFI_EXPORT static void wasm_test_func_iiw(int32_t arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iiw because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_iiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_iiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
}

FFI_EXPORT static void wasm_test_func_iiww(int32_t arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2, const FFI_PTR_WASM int32_t * const arg3) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iiww because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_iiww because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_iiww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_iiww because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
}

FFI_EXPORT static void wasm_test_func_iI(int32_t arg0, int64_t arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iI because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_iI because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
}

FFI_EXPORT static void wasm_test_func_if(int32_t arg0, float arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_if because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_if because arg1 (float) was expected to be 2.0 but was %f.", arg1);
}

FFI_EXPORT static void wasm_test_func_iw(int32_t arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iw because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_iw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
}

FFI_EXPORT static void wasm_test_func_iwi(int32_t arg0, const FFI_PTR_WASM int32_t * const arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iwi because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_iwi because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_iwi because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
}

FFI_EXPORT static void wasm_test_func_iww(int32_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_iww because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_iww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_iww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
}

FFI_EXPORT static void wasm_test_func_I(int64_t arg0) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_I because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
}

FFI_EXPORT static void wasm_test_func_Ii(int64_t arg0, int32_t arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Ii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_Ii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
}

FFI_EXPORT static void wasm_test_func_Iii(int64_t arg0, int32_t arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_Iii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_Iii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
}

FFI_EXPORT static void wasm_test_func_Iiiw(int64_t arg0, int32_t arg1, int32_t arg2, const FFI_PTR_WASM int32_t * const arg3) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iiiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_Iiiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_Iiiw because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_Iiiw because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
}

FFI_EXPORT static void wasm_test_func_IiI(int64_t arg0, int32_t arg1, int64_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_IiI because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_IiI because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_IiI because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
}

FFI_EXPORT static void wasm_test_func_Iiw(int64_t arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_Iiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_Iiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
}

FFI_EXPORT static void wasm_test_func_II(int64_t arg0, int64_t arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_II because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_II because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
}

FFI_EXPORT static void wasm_test_func_IIiiiw(int64_t arg0, int64_t arg1, int32_t arg2, int32_t arg3, int32_t arg4, const FFI_PTR_WASM int32_t * const arg5) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_IIiiiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_IIiiiw because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_IIiiiw because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_IIiiiw because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(arg4 == 5, "Failed to call wasm_test_func_IIiiiw because arg4 (int32_t) was expected to be 5 but was " PRIu32 ".", arg4);
    VERIFY_MSG(*arg5 == 6, "Failed to call wasm_test_func_IIiiiw because arg5 (const FFI_PTR_WASM int32_t * const) was expected to be 6 but was " PRIu32 ".", *arg5);
}

FFI_EXPORT static void wasm_test_func_III(int64_t arg0, int64_t arg1, int64_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_III because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_III because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_III because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
}

FFI_EXPORT static void wasm_test_func_IIww(int64_t arg0, int64_t arg1, const FFI_PTR_WASM int32_t * const arg2, const FFI_PTR_WASM int32_t * const arg3) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_IIww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_IIww because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_IIww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_IIww because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
}

FFI_EXPORT static void wasm_test_func_If(int64_t arg0, float arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_If because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_If because arg1 (float) was expected to be 2.0 but was %f.", arg1);
}

FFI_EXPORT static void wasm_test_func_IF(int64_t arg0, double arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_IF because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_IF because arg1 (double) was expected to be 2.0 but was %f.", arg1);
}

FFI_EXPORT static void wasm_test_func_Iw(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
}

FFI_EXPORT static void wasm_test_func_Iwi(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwi because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwi because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_Iwi because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
}

FFI_EXPORT static void wasm_test_func_IwIwIiii(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, int64_t arg2, const FFI_PTR_WASM int32_t * const arg3, int64_t arg4, int32_t arg5, int32_t arg6, int32_t arg7) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_IwIwIiii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_IwIwIiii because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_IwIwIiii because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_IwIwIiii because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(arg4 == 5000000000000, "Failed to call wasm_test_func_IwIwIiii because arg4 (int64_t) was expected to be 5000000000000 but was " PRIu64 ".", arg4);
    VERIFY_MSG(arg5 == 6, "Failed to call wasm_test_func_IwIwIiii because arg5 (int32_t) was expected to be 6 but was " PRIu32 ".", arg5);
    VERIFY_MSG(arg6 == 7, "Failed to call wasm_test_func_IwIwIiii because arg6 (int32_t) was expected to be 7 but was " PRIu32 ".", arg6);
    VERIFY_MSG(arg7 == 8, "Failed to call wasm_test_func_IwIwIiii because arg7 (int32_t) was expected to be 8 but was " PRIu32 ".", arg7);
}

FFI_EXPORT static void wasm_test_func_Iwffwiw(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, float arg2, float arg3, const FFI_PTR_WASM int32_t * const arg4, int32_t arg5, const FFI_PTR_WASM int32_t * const arg6) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwffwiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwffwiw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3.0, "Failed to call wasm_test_func_Iwffwiw because arg2 (float) was expected to be 3.0 but was %f.", arg2);
    VERIFY_MSG(arg3 == 4.0, "Failed to call wasm_test_func_Iwffwiw because arg3 (float) was expected to be 4.0 but was %f.", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_Iwffwiw because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
    VERIFY_MSG(arg5 == 6, "Failed to call wasm_test_func_Iwffwiw because arg5 (int32_t) was expected to be 6 but was " PRIu32 ".", arg5);
    VERIFY_MSG(*arg6 == 7, "Failed to call wasm_test_func_Iwffwiw because arg6 (const FFI_PTR_WASM int32_t * const) was expected to be 7 but was " PRIu32 ".", *arg6);
}

FFI_EXPORT static void wasm_test_func_Iwffwwiw(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, float arg2, float arg3, const FFI_PTR_WASM int32_t * const arg4, const FFI_PTR_WASM int32_t * const arg5, int32_t arg6, const FFI_PTR_WASM int32_t * const arg7) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwffwwiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwffwwiw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3.0, "Failed to call wasm_test_func_Iwffwwiw because arg2 (float) was expected to be 3.0 but was %f.", arg2);
    VERIFY_MSG(arg3 == 4.0, "Failed to call wasm_test_func_Iwffwwiw because arg3 (float) was expected to be 4.0 but was %f.", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_Iwffwwiw because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
    VERIFY_MSG(*arg5 == 6, "Failed to call wasm_test_func_Iwffwwiw because arg5 (const FFI_PTR_WASM int32_t * const) was expected to be 6 but was " PRIu32 ".", *arg5);
    VERIFY_MSG(arg6 == 7, "Failed to call wasm_test_func_Iwffwwiw because arg6 (int32_t) was expected to be 7 but was " PRIu32 ".", arg6);
    VERIFY_MSG(*arg7 == 8, "Failed to call wasm_test_func_Iwffwwiw because arg7 (const FFI_PTR_WASM int32_t * const) was expected to be 8 but was " PRIu32 ".", *arg7);
}

FFI_EXPORT static void wasm_test_func_Iww(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_Iww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
}

FFI_EXPORT static void wasm_test_func_Iwwi(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2, int32_t arg3) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwwi because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwwi because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_Iwwi because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_Iwwi because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
}

FFI_EXPORT static void wasm_test_func_Iwwiw(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2, int32_t arg3, const FFI_PTR_WASM int32_t * const arg4) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwwiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwwiw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_Iwwiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_Iwwiw because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_Iwwiw because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
}

FFI_EXPORT static void wasm_test_func_Iwww(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2, const FFI_PTR_WASM int32_t * const arg3) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_Iwww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_Iwww because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
}

FFI_EXPORT static void wasm_test_func_Iwwww(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2, const FFI_PTR_WASM int32_t * const arg3, const FFI_PTR_WASM int32_t * const arg4) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_Iwwww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_Iwwww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_Iwwww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_Iwwww because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_Iwwww because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
}

FFI_EXPORT static void wasm_test_func_f(float arg0) {
    VERIFY_MSG(arg0 == 1.0, "Failed to call wasm_test_func_f because arg0 (float) was expected to be 1.0 but was %f.", arg0);
}

FFI_EXPORT static void wasm_test_func_ffff(float arg0, float arg1, float arg2, float arg3) {
    VERIFY_MSG(arg0 == 1.0, "Failed to call wasm_test_func_ffff because arg0 (float) was expected to be 1.0 but was %f.", arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_ffff because arg1 (float) was expected to be 2.0 but was %f.", arg1);
    VERIFY_MSG(arg2 == 3.0, "Failed to call wasm_test_func_ffff because arg2 (float) was expected to be 3.0 but was %f.", arg2);
    VERIFY_MSG(arg3 == 4.0, "Failed to call wasm_test_func_ffff because arg3 (float) was expected to be 4.0 but was %f.", arg3);
}

FFI_EXPORT static void wasm_test_func_w(const FFI_PTR_WASM int32_t * const arg0) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_w because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
}

FFI_EXPORT static void wasm_test_func_wiiIw(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int32_t arg2, int64_t arg3, const FFI_PTR_WASM int32_t * const arg4) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wiiIw because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_wiiIw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_wiiIw because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4000000000000, "Failed to call wasm_test_func_wiiIw because arg3 (int64_t) was expected to be 4000000000000 but was " PRIu64 ".", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_wiiIw because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
}

FFI_EXPORT static void wasm_test_func_wiIiwiiiiw(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int64_t arg2, int32_t arg3, const FFI_PTR_WASM int32_t * const arg4, int32_t arg5, int32_t arg6, int32_t arg7, int32_t arg8, const FFI_PTR_WASM int32_t * const arg9) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wiIiwiiiiw because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_wiIiwiiiiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_wiIiwiiiiw because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_wiIiwiiiiw because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_wiIiwiiiiw because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
    VERIFY_MSG(arg5 == 6, "Failed to call wasm_test_func_wiIiwiiiiw because arg5 (int32_t) was expected to be 6 but was " PRIu32 ".", arg5);
    VERIFY_MSG(arg6 == 7, "Failed to call wasm_test_func_wiIiwiiiiw because arg6 (int32_t) was expected to be 7 but was " PRIu32 ".", arg6);
    VERIFY_MSG(arg7 == 8, "Failed to call wasm_test_func_wiIiwiiiiw because arg7 (int32_t) was expected to be 8 but was " PRIu32 ".", arg7);
    VERIFY_MSG(arg8 == 9, "Failed to call wasm_test_func_wiIiwiiiiw because arg8 (int32_t) was expected to be 9 but was " PRIu32 ".", arg8);
    VERIFY_MSG(*arg9 == 10, "Failed to call wasm_test_func_wiIiwiiiiw because arg9 (const FFI_PTR_WASM int32_t * const) was expected to be 10 but was " PRIu32 ".", *arg9);
}

FFI_EXPORT static void wasm_test_func_wiIwiiiiw(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int64_t arg2, const FFI_PTR_WASM int32_t * const arg3, int32_t arg4, int32_t arg5, int32_t arg6, int32_t arg7, const FFI_PTR_WASM int32_t * const arg8) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wiIwiiiiw because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_wiIwiiiiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_wiIwiiiiw because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_wiIwiiiiw because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(arg4 == 5, "Failed to call wasm_test_func_wiIwiiiiw because arg4 (int32_t) was expected to be 5 but was " PRIu32 ".", arg4);
    VERIFY_MSG(arg5 == 6, "Failed to call wasm_test_func_wiIwiiiiw because arg5 (int32_t) was expected to be 6 but was " PRIu32 ".", arg5);
    VERIFY_MSG(arg6 == 7, "Failed to call wasm_test_func_wiIwiiiiw because arg6 (int32_t) was expected to be 7 but was " PRIu32 ".", arg6);
    VERIFY_MSG(arg7 == 8, "Failed to call wasm_test_func_wiIwiiiiw because arg7 (int32_t) was expected to be 8 but was " PRIu32 ".", arg7);
    VERIFY_MSG(*arg8 == 9, "Failed to call wasm_test_func_wiIwiiiiw because arg8 (const FFI_PTR_WASM int32_t * const) was expected to be 9 but was " PRIu32 ".", *arg8);
}

FFI_EXPORT static void wasm_test_func_wiwiiw(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2, int32_t arg3, int32_t arg4, const FFI_PTR_WASM int32_t * const arg5) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wiwiiw because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_wiwiiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_wiwiiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_wiwiiw because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(arg4 == 5, "Failed to call wasm_test_func_wiwiiw because arg4 (int32_t) was expected to be 5 but was " PRIu32 ".", arg4);
    VERIFY_MSG(*arg5 == 6, "Failed to call wasm_test_func_wiwiiw because arg5 (const FFI_PTR_WASM int32_t * const) was expected to be 6 but was " PRIu32 ".", *arg5);
}

FFI_EXPORT static void wasm_test_func_wiwiwiiiiw(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2, int32_t arg3, const FFI_PTR_WASM int32_t * const arg4, int32_t arg5, int32_t arg6, int32_t arg7, int32_t arg8, const FFI_PTR_WASM int32_t * const arg9) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wiwiwiiiiw because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_wiwiwiiiiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_wiwiwiiiiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_wiwiwiiiiw because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_wiwiwiiiiw because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
    VERIFY_MSG(arg5 == 6, "Failed to call wasm_test_func_wiwiwiiiiw because arg5 (int32_t) was expected to be 6 but was " PRIu32 ".", arg5);
    VERIFY_MSG(arg6 == 7, "Failed to call wasm_test_func_wiwiwiiiiw because arg6 (int32_t) was expected to be 7 but was " PRIu32 ".", arg6);
    VERIFY_MSG(arg7 == 8, "Failed to call wasm_test_func_wiwiwiiiiw because arg7 (int32_t) was expected to be 8 but was " PRIu32 ".", arg7);
    VERIFY_MSG(arg8 == 9, "Failed to call wasm_test_func_wiwiwiiiiw because arg8 (int32_t) was expected to be 9 but was " PRIu32 ".", arg8);
    VERIFY_MSG(*arg9 == 10, "Failed to call wasm_test_func_wiwiwiiiiw because arg9 (const FFI_PTR_WASM int32_t * const) was expected to be 10 but was " PRIu32 ".", *arg9);
}

FFI_EXPORT static void wasm_test_func_wI(const FFI_PTR_WASM int32_t * const arg0, int64_t arg1) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wI because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_wI because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
}

FFI_EXPORT static void wasm_test_func_wIii(const FFI_PTR_WASM int32_t * const arg0, int64_t arg1, int32_t arg2, int32_t arg3) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wIii because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_wIii because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_wIii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_wIii because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
}

FFI_EXPORT static void wasm_test_func_wf(const FFI_PTR_WASM int32_t * const arg0, float arg1) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wf because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_wf because arg1 (float) was expected to be 2.0 but was %f.", arg1);
}

FFI_EXPORT static void wasm_test_func_wfwwi(const FFI_PTR_WASM int32_t * const arg0, float arg1, const FFI_PTR_WASM int32_t * const arg2, const FFI_PTR_WASM int32_t * const arg3, int32_t arg4) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wfwwi because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_wfwwi because arg1 (float) was expected to be 2.0 but was %f.", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_wfwwi because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_wfwwi because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(arg4 == 5, "Failed to call wasm_test_func_wfwwi because arg4 (int32_t) was expected to be 5 but was " PRIu32 ".", arg4);
}

FFI_EXPORT static void wasm_test_func_ww(const FFI_PTR_WASM int32_t * const arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_ww because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
}

FFI_EXPORT static void wasm_test_func_wwf(const FFI_PTR_WASM int32_t * const arg0, const FFI_PTR_WASM int32_t * const arg1, float arg2) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_wwf because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_wwf because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3.0, "Failed to call wasm_test_func_wwf because arg2 (float) was expected to be 3.0 but was %f.", arg2);
}

FFI_EXPORT static void wasm_test_func_www(const FFI_PTR_WASM int32_t * const arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_www because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_www because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_www because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
}

FFI_EXPORT static int32_t wasm_test_func_ri() {
    return 1000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_i(int32_t arg0) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_ri_i because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    return 2000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_ii(int32_t arg0, int32_t arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_ri_ii because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_ii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    return 3000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_iiw(int32_t arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_ri_iiw because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_iiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_ri_iiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_iw(int32_t arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_ri_iw because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ri_iw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    return 3000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_I(int64_t arg0) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_I because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    return 2000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_Ii(int64_t arg0, int32_t arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_Ii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_Ii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    return 3000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_Iiw(int64_t arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_Iiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_Iiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_ri_Iiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_II(int64_t arg0, int64_t arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_II because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_ri_II because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    return 3000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_IIi(int64_t arg0, int64_t arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_IIi because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_ri_IIi because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_IIi because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_IIii(int64_t arg0, int64_t arg1, int32_t arg2, int32_t arg3) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_IIii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_ri_IIii because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_IIii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_ri_IIii because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    return 5000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_Iw(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_Iw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ri_Iw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    return 3000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_Iwi(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_Iwi because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ri_Iwi because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_Iwi because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_Iwiiww(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, int32_t arg2, int32_t arg3, const FFI_PTR_WASM int32_t * const arg4, const FFI_PTR_WASM int32_t * const arg5) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_Iwiiww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ri_Iwiiww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_Iwiiww because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_ri_Iwiiww because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_ri_Iwiiww because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
    VERIFY_MSG(*arg5 == 6, "Failed to call wasm_test_func_ri_Iwiiww because arg5 (const FFI_PTR_WASM int32_t * const) was expected to be 6 but was " PRIu32 ".", *arg5);
    return 7000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_IwiI(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, int32_t arg2, int64_t arg3) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_IwiI because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ri_IwiI because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_IwiI because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4000000000000, "Failed to call wasm_test_func_ri_IwiI because arg3 (int64_t) was expected to be 4000000000000 but was " PRIu64 ".", arg3);
    return 5000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_Iww(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_ri_Iww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_ri_Iww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_ri_Iww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_w(const FFI_PTR_WASM int32_t * const arg0) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_ri_w because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    return 2000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_wii(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int32_t arg2) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_ri_wii because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_wii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_wii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_wiiI(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int32_t arg2, int64_t arg3) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_ri_wiiI because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_wiiI because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_ri_wiiI because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4000000000000, "Failed to call wasm_test_func_ri_wiiI because arg3 (int64_t) was expected to be 4000000000000 but was " PRIu64 ".", arg3);
    return 5000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_wiI(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int64_t arg2) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_ri_wiI because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_wiI because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_ri_wiI because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    return 4000;
}

FFI_EXPORT static int32_t wasm_test_func_ri_wiIi(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int64_t arg2, int32_t arg3) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_ri_wiIi because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_ri_wiIi because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_ri_wiIi because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_ri_wiIi because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    return 5000;
}

FFI_EXPORT static int64_t wasm_test_func_rI() {
    return 1000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_i(int32_t arg0) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_rI_i because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    return 2000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_iw(int32_t arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_rI_iw because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_rI_iw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    return 3000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_iww(int32_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_rI_iww because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_rI_iww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_rI_iww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_I(int64_t arg0) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_I because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    return 2000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_Iii(int64_t arg0, int32_t arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_Iii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_rI_Iii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_rI_Iii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_Iiiii(int64_t arg0, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_Iiiii because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_rI_Iiiii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_rI_Iiiii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    VERIFY_MSG(arg3 == 4, "Failed to call wasm_test_func_rI_Iiiii because arg3 (int32_t) was expected to be 4 but was " PRIu32 ".", arg3);
    VERIFY_MSG(arg4 == 5, "Failed to call wasm_test_func_rI_Iiiii because arg4 (int32_t) was expected to be 5 but was " PRIu32 ".", arg4);
    return 6000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_Iiw(int64_t arg0, int32_t arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_Iiw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_rI_Iiw because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_rI_Iiw because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_III(int64_t arg0, int64_t arg1, int64_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_III because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_rI_III because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_rI_III because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_Ifi(int64_t arg0, float arg1, int32_t arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_Ifi because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_rI_Ifi because arg1 (float) was expected to be 2.0 but was %f.", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_rI_Ifi because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_Iw(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_Iw because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_rI_Iw because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    return 3000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_Iww(int64_t arg0, const FFI_PTR_WASM int32_t * const arg1, const FFI_PTR_WASM int32_t * const arg2) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rI_Iww because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_rI_Iww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(*arg2 == 3, "Failed to call wasm_test_func_rI_Iww because arg2 (const FFI_PTR_WASM int32_t * const) was expected to be 3 but was " PRIu32 ".", *arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_w(const FFI_PTR_WASM int32_t * const arg0) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_rI_w because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    return 2000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_wi(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_rI_wi because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_rI_wi because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    return 3000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_wii(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int32_t arg2) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_rI_wii because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_rI_wii because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3, "Failed to call wasm_test_func_rI_wii because arg2 (int32_t) was expected to be 3 but was " PRIu32 ".", arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_wiI(const FFI_PTR_WASM int32_t * const arg0, int32_t arg1, int64_t arg2) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_rI_wiI because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2, "Failed to call wasm_test_func_rI_wiI because arg1 (int32_t) was expected to be 2 but was " PRIu32 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_rI_wiI because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    return 4000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_wIIwIiii(const FFI_PTR_WASM int32_t * const arg0, int64_t arg1, int64_t arg2, const FFI_PTR_WASM int32_t * const arg3, int64_t arg4, int32_t arg5, int32_t arg6, int32_t arg7) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_rI_wIIwIiii because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(arg1 == 2000000000000, "Failed to call wasm_test_func_rI_wIIwIiii because arg1 (int64_t) was expected to be 2000000000000 but was " PRIu64 ".", arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_rI_wIIwIiii because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_rI_wIIwIiii because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(arg4 == 5000000000000, "Failed to call wasm_test_func_rI_wIIwIiii because arg4 (int64_t) was expected to be 5000000000000 but was " PRIu64 ".", arg4);
    VERIFY_MSG(arg5 == 6, "Failed to call wasm_test_func_rI_wIIwIiii because arg5 (int32_t) was expected to be 6 but was " PRIu32 ".", arg5);
    VERIFY_MSG(arg6 == 7, "Failed to call wasm_test_func_rI_wIIwIiii because arg6 (int32_t) was expected to be 7 but was " PRIu32 ".", arg6);
    VERIFY_MSG(arg7 == 8, "Failed to call wasm_test_func_rI_wIIwIiii because arg7 (int32_t) was expected to be 8 but was " PRIu32 ".", arg7);
    return 9000;
}

FFI_EXPORT static int64_t wasm_test_func_rI_wwIww(const FFI_PTR_WASM int32_t * const arg0, const FFI_PTR_WASM int32_t * const arg1, int64_t arg2, const FFI_PTR_WASM int32_t * const arg3, const FFI_PTR_WASM int32_t * const arg4) {
    VERIFY_MSG(*arg0 == 1, "Failed to call wasm_test_func_rI_wwIww because arg0 (const FFI_PTR_WASM int32_t * const) was expected to be 1 but was " PRIu32 ".", *arg0);
    VERIFY_MSG(*arg1 == 2, "Failed to call wasm_test_func_rI_wwIww because arg1 (const FFI_PTR_WASM int32_t * const) was expected to be 2 but was " PRIu32 ".", *arg1);
    VERIFY_MSG(arg2 == 3000000000000, "Failed to call wasm_test_func_rI_wwIww because arg2 (int64_t) was expected to be 3000000000000 but was " PRIu64 ".", arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_rI_wwIww because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(*arg4 == 5, "Failed to call wasm_test_func_rI_wwIww because arg4 (const FFI_PTR_WASM int32_t * const) was expected to be 5 but was " PRIu32 ".", *arg4);
    return 6000;
}

FFI_EXPORT static float wasm_test_func_rf() {
    return 1000;
}

FFI_EXPORT static float wasm_test_func_rf_i(int32_t arg0) {
    VERIFY_MSG(arg0 == 1, "Failed to call wasm_test_func_rf_i because arg0 (int32_t) was expected to be 1 but was " PRIu32 ".", arg0);
    return 2000;
}

FFI_EXPORT static float wasm_test_func_rf_Iffwi(int64_t arg0, float arg1, float arg2, const FFI_PTR_WASM int32_t * const arg3, int32_t arg4) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rf_Iffwi because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    VERIFY_MSG(arg1 == 2.0, "Failed to call wasm_test_func_rf_Iffwi because arg1 (float) was expected to be 2.0 but was %f.", arg1);
    VERIFY_MSG(arg2 == 3.0, "Failed to call wasm_test_func_rf_Iffwi because arg2 (float) was expected to be 3.0 but was %f.", arg2);
    VERIFY_MSG(*arg3 == 4, "Failed to call wasm_test_func_rf_Iffwi because arg3 (const FFI_PTR_WASM int32_t * const) was expected to be 4 but was " PRIu32 ".", *arg3);
    VERIFY_MSG(arg4 == 5, "Failed to call wasm_test_func_rf_Iffwi because arg4 (int32_t) was expected to be 5 but was " PRIu32 ".", arg4);
    return 6000;
}

FFI_EXPORT static double wasm_test_func_rF_I(int64_t arg0) {
    VERIFY_MSG(arg0 == 1000000000000, "Failed to call wasm_test_func_rF_I because arg0 (int64_t) was expected to be 1000000000000 but was " PRIu64 ".", arg0);
    return 2000;
}

