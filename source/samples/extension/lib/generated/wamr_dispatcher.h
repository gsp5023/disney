/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

FFI_THUNK(0x10A, void, extension_println, (FFI_WASM_INTERPRETER_STATE runtime, FFI_WASM_PTR const msg), {
    _wamr_thunk_extension_println(msg);
})

FFI_THUNK(0x404, int32_t, extension_squared, (FFI_WASM_INTERPRETER_STATE runtime, int32_t value), {
    return _wamr_thunk_extension_squared(value);
})

