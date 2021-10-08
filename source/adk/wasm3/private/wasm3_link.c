/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/wasm3/wasm3_link.h"

static wasm_linker_t linkers[64] = {0};
static uint32_t linker_count = 0;

void wasm3_export_native_function(IM3Module module, const char * const name, const wasm_sig_mask signature, const M3RawCall func_ptr) {
    char tsig[32] = {0};
    wasm_sig(signature, tsig);
    m3_LinkRawFunction(wasm3_app_module, "*", name, tsig, func_ptr);
}

void wasm3_register_linker(const wasm_linker_t linker) {
    ASSERT_MSG(linker_count < ARRAY_SIZE(linkers) - 1, "[wasm3] Attempted to register too many linkers");
    linkers[linker_count++] = linker;
}

void wasm3_run_all_linkers(void) {
    for (uint32_t i = 0; i < linker_count; i++) {
        linkers[i](wasm3_app_module);
    }
}

#undef FFI_SET_NATIVE_PTR
#undef FFI_GET_NATIVE_PTR
#undef FFI_NATIVE_PTR
#undef FFI_ENUM
#undef FFI_SET_BOOL
#undef FFI_GET_BOOL
#undef FFI_BOOL
#undef FFI_SELECT_NATIVE_OR_WASM_VALUE
#undef FFI_WASM_PTR_OFFSET_TO_NATIVE_PTR
#undef FFI_WASM_PTR_OFFSET
#undef FFI_ASSERT_ALIGNED_WASM_PTR
#undef FFI_PIN_WASM_PTR
#undef FFI_WASM_PTR