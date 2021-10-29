/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/file/file.h"
#include "source/adk/interpreter/interp_common.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/wasm3/private/wasm3.h"

static const char * wasm3_get_callstack(void) {
    return m3_GetCallStack(wasm3_global_runtime);
}

static wasm_call_result_t wasm3_create_result(const char * const name, const M3Result details) {
    wasm_call_result_t result = {0};
    result.details = details;
    result.func_name = name;
    if (!details) {
        result.status = wasm_call_success;
    } else if (!strcmp(details, "function lookup failed")) {
        result.status = wasm_call_function_not_found;
    } else if (strstr(details, "[trap] unreachable executed")) {
        result.status = wasm_call_unreachable_executed;
    } else if (strstr(details, "[trap] out of bounds memory access")) {
        result.status = wasm_call_out_of_bounds_memory_access;
    } else {
        result.status = wasm_call_unknown_failure;
    }
    return result;
}

static wasm_call_result_t wasm3_call_void(const char * const name) {
    return wasm3_create_result(name, m3_Call_void(wasm3_global_runtime, name));
}

static wasm_call_result_t wasm3_call_i(const char * const name, const uint32_t arg0) {
    return wasm3_create_result(name, m3_Call_i(wasm3_global_runtime, name, arg0));
}

static wasm_call_result_t wasm3_call_ii(const char * const name, const uint32_t arg0, const uint32_t arg1) {
    return wasm3_create_result(name, m3_Call_ii(wasm3_global_runtime, name, arg0, arg1));
}

static wasm_call_result_t wasm3_call_iii(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2) {
    return wasm3_create_result(name, m3_Call_iii(wasm3_global_runtime, name, arg0, arg1, arg2));
}

static wasm_call_result_t wasm3_call_iiii(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3) {
    return wasm3_create_result(name, m3_Call_iiii(wasm3_global_runtime, name, arg0, arg1, arg2, arg3));
}

static wasm_call_result_t wasm3_call_iiiii(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3, const uint32_t arg4) {
    return wasm3_create_result(name, m3_Call_iiiii(wasm3_global_runtime, name, arg0, arg1, arg2, arg3, arg4));
}

static wasm_call_result_t wasm3_call_iI(const char * const name, const uint32_t arg0, const uint64_t arg1) {
    return wasm3_create_result(name, m3_Call_iI(wasm3_global_runtime, name, arg0, arg1));
}

static wasm_call_result_t wasm3_call_Ii(const char * const name, const uint64_t arg0, const uint32_t arg1) {
    return wasm3_create_result(name, m3_Call_Ii(wasm3_global_runtime, name, arg0, arg1));
}

static wasm_call_result_t wasm3_call_iiIi(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3) {
    return wasm3_create_result(name, m3_Call_iiIi(wasm3_global_runtime, name, arg0, arg1, arg2, arg3));
}

static wasm_call_result_t wasm3_call_iIi(const char * const name, const uint32_t arg0, const uint64_t arg1, const uint32_t arg2) {
    return wasm3_create_result(name, m3_Call_iIi(wasm3_global_runtime, name, arg0, arg1, arg2));
}

static wasm_call_result_t wasm3_call_iIIi(const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3) {
    return wasm3_create_result(name, m3_Call_iIIi(wasm3_global_runtime, name, arg0, arg1, arg2, arg3));
}

static wasm_call_result_t wasm3_call_iIIii(const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4) {
    return wasm3_create_result(name, m3_Call_iIIii(wasm3_global_runtime, name, arg0, arg1, arg2, arg3, arg4));
}

static wasm_call_result_t wasm3_call_ifI(const char * const name, const uint32_t arg0, const float arg1, const uint64_t arg2) {
    return wasm3_create_result(name, m3_Call_ifI(wasm3_global_runtime, name, arg0, arg1, arg2));
}

static wasm_call_result_t wasm3_call_ip(const char * const name, const uint32_t arg0, void * const arg1) {
    return wasm3_call_iI(name, arg0, (uint64_t)(uintptr_t)arg1);
}

static wasm_call_result_t wasm3_call_pi(const char * const name, void * const arg0, const uint32_t arg1) {
    return wasm3_call_Ii(name, (uint64_t)(uintptr_t)arg0, arg1);
}

static wasm_call_result_t wasm3_call_ipi(const char * const name, const uint32_t arg0, void * const arg1, const uint32_t arg2) {
    return wasm3_call_iIi(name, arg0, (uint64_t)(uintptr_t)arg1, arg2);
}

static wasm_call_result_t wasm3_call_ippi(const char * const name, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3) {
    return wasm3_call_iIIi(name, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

static wasm_call_result_t wasm3_call_iipi(const char * const name, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3) {
    return wasm3_call_iiIi(name, arg0, arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

static wasm_call_result_t wasm3_call_ifp(const char * const name, const uint32_t arg0, const float arg1, void * const arg2) {
    return wasm3_call_ifI(name, arg0, arg1, (uint64_t)(uintptr_t)arg2);
}

static wasm_call_result_t wasm3_call_ri(const char * const name, uint32_t * const ret) {
    return wasm3_create_result(name, m3_Call_ri(wasm3_global_runtime, name, ret));
}

static wasm_call_result_t wasm3_call_ri_i(const char * const name, uint32_t * const ret, const uint32_t arg0) {
    return wasm3_create_result(name, m3_Call_ri_i(wasm3_global_runtime, name, ret, arg0));
}

static wasm_call_result_t wasm3_call_ri_iI(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1) {
    return wasm3_create_result(name, m3_Call_ri_iI(wasm3_global_runtime, name, ret, arg0, arg1));
}

static wasm_call_result_t wasm3_call_ri_iiIi(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3) {
    return wasm3_create_result(name, m3_Call_ri_iiIi(wasm3_global_runtime, name, ret, arg0, arg1, arg2, arg3));
}

static wasm_call_result_t wasm3_call_ri_iIIi(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3) {
    return wasm3_create_result(name, m3_Call_ri_iIIi(wasm3_global_runtime, name, ret, arg0, arg1, arg2, arg3));
}

static wasm_call_result_t wasm3_call_ri_iIIii(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4) {
    return wasm3_create_result(name, m3_Call_ri_iIIii(wasm3_global_runtime, name, ret, arg0, arg1, arg2, arg3, arg4));
}

static wasm_call_result_t wasm3_call_ri_ifI(const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, const uint64_t arg2) {
    return wasm3_create_result(name, m3_Call_ri_ifI(wasm3_global_runtime, name, ret, arg0, arg1, arg2));
}

static wasm_call_result_t wasm3_call_ri_ip(const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1) {
    return wasm3_call_ri_iI(name, ret, arg0, (uint64_t)(uintptr_t)arg1);
}

static wasm_call_result_t wasm3_call_ri_ippi(const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3) {
    return wasm3_call_ri_iIIi(name, ret, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

static wasm_call_result_t wasm3_call_ri_ippii(const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3, const uint32_t arg4) {
    return wasm3_call_ri_iIIii(name, ret, arg0, (uint64_t)(uintptr_t)arg1, (uint64_t)(uintptr_t)arg2, arg3, arg4);
}

static wasm_call_result_t wasm3_call_ri_iipi(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3) {
    return wasm3_call_ri_iiIi(name, ret, arg0, arg1, (uint64_t)(uintptr_t)arg2, arg3);
}

static wasm_call_result_t wasm3_call_ri_ifp(const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, void * const arg2) {
    return wasm3_call_ri_ifI(name, ret, arg0, arg1, (uint64_t)(uintptr_t)arg2);
}

static wasm_call_result_t wasm3_call_ri_argv(const char * const name, uint32_t * const ret, const uint32_t argc, const char ** const argv) {
    return wasm_argv_call(get_wasm3_interpreter(), name, ret, argc, argv);
}

static wasm_call_result_t wasm3_call_rI(const char * const name, uint64_t * const ret) {
    return wasm3_create_result(name, m3_Call_rI(wasm3_global_runtime, name, ret));
}

static void * wasm3_ptr_w2n(wasm_ptr_t ptr) {
    return m3_ConvertWasmPtrToNativePtr(wasm3_global_runtime, (uint32_t)ptr.ofs);
}

static wasm_ptr_t wasm3_ptr_n2w(void * ptr) {
    return (wasm_ptr_t){
        .ofs = m3_ConvertNativePtrToWasmPtr(wasm3_global_runtime, ptr)};
}

void * wasm3_convert_ptr_wasm_to_native(uint32_t ptr) {
    return wasm3_ptr_w2n((wasm_ptr_t){.ofs = ptr});
}

wasm_interpreter_t wasm3_interpreter = {
    .name = "wasm3",

    .load = load_wasm3,
    .load_fp = load_wasm3_fp,
    .unload = unload_wasm3,

    .get_callstack = wasm3_get_callstack,

    .translate_ptr_wasm_to_native = wasm3_ptr_w2n,
    .translate_ptr_native_to_wasm = wasm3_ptr_n2w,

#define WASM_CALLEE_SIGNATURE(_sig) TOKENPASTE(.call_, _sig) = TOKENPASTE(wasm3_call_, _sig)
    WASM_CALLEE_REQUIRED_SIGNATURES
#undef WASM_CALLEE_SIGNATURE
};

wasm_interpreter_t * get_wasm3_interpreter(void) {
    return &wasm3_interpreter;
}
