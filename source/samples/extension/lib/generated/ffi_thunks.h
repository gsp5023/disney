/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

FFI_THUNK(0x10A, void, extension_println, (FFI_WASM_PTR const msg), {
    ASSERT_MSG(FFI_PIN_WASM_PTR(msg), "msg cannot be NULL");
    extension_println(FFI_PIN_WASM_PTR(msg));
})

FFI_THUNK(0x404, int32_t, extension_squared, (int32_t value), {
    return extension_squared(value);
})

