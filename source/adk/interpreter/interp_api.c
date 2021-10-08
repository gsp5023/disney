/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/interpreter/interp_api.h"

wasm_interpreter_t * active_interpreter;

#define VERIFY_WASM_INTERPRETER

static void verify_interpreter_completeness(const wasm_interpreter_t * const interpreter) {
    VERIFY_MSG(interpreter, "[wasm] Interpreter cannot be NULL.");
#ifndef NDEBUG
#ifdef VERIFY_WASM_INTERPRETER
    const size_t size_of_interpreter = sizeof(*interpreter);
    ASSERT(size_of_interpreter % sizeof(void *) == 0);
    const size_t pointer_count = size_of_interpreter / sizeof(void *);
    const void * const * const first_pointer = (const void * const * const)interpreter;
    for (size_t i = 0; i < pointer_count; i++) {
        const void * const * const pointer = first_pointer[i];
        VERIFY_MSG(pointer, "[wasm] Interpreter is not completely integrated. Inspect the interpreter struct for NULL pointers.");
    }
#endif // VERIFY_INTERPRETER
#endif // NDEBUG
}

void set_active_wasm_interpreter(wasm_interpreter_t * const interpreter) {
    verify_interpreter_completeness(interpreter);
    active_interpreter = interpreter;
}

wasm_interpreter_t * get_active_wasm_interpreter(void) {
    return active_interpreter;
}

void * wasm_translate_ptr_wasm_to_native(wasm_ptr_t addr) {
    return active_interpreter->translate_ptr_wasm_to_native(addr);
}

wasm_ptr_t wasm_translate_ptr_native_to_wasm(void * addr) {
    return active_interpreter->translate_ptr_native_to_wasm(addr);
}
