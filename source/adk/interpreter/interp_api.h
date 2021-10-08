/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifndef FFI_GEN

#include "source/adk/extender/extender_status.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/file/file.h"
#include "source/adk/runtime/app/app.h"

typedef extender_status_e (*wasm_linker_t)(void * const wasm_interpreter_instance);

/// A region of memory that hosts a Wasm binary.
typedef struct wasm_memory_region_t {
    adk_mem_region_t wasm_mem_region;
    size_t wasm_bytecode_size;
} wasm_memory_region_t;

/// The result status code of a C -> Wasm call.
typedef enum wasm_call_result_e {
    /// Indicates successful execution of the called Wasm function.
    wasm_call_success = 0,

    /// Indicates a failure to execute the called Wasm function, that is not covered by
    /// other status codes. Inspect the `details` field of the enclosing result for a
    /// possible explanation.
    wasm_call_unknown_failure,

    /// Indicates a failure to find the requested Wasm function. This may be because
    /// the function does not exist in this binary, or it is not exported, or is
    /// exported under a different name (e.g. due to name mangling by the compiler
    /// of the app).
    wasm_call_function_not_found,

    /// Indicates that an 'unreachable' instruction was encountered during the
    /// execution of this function. This typically indicates an app panic.
    wasm_call_unreachable_executed,

    /// Indicates that an out-of-bounds memory access was attempted during the
    /// execution of this function. This typically indicates that the app
    /// dereferenced an invalid pointer.
    wasm_call_out_of_bounds_memory_access,
} wasm_call_result_e;

/// The result of a C -> Wasm call.
typedef struct wasm_call_result_t {
    /// The result status code of the call. This allows to differentiate between
    /// certain kinds of failure in an interpreter-agnotsic manner.
    wasm_call_result_e status;

    /// An optional explanation of the failure, if any. This is a string and it is
    /// specific to the interpreter through which the call was attempted.
    const char * details;

    /// The name of the function that was attempted to be called.
    const char * func_name;
} wasm_call_result_t;

typedef size_t (*wasm_fread_t)(void * const buffer, const size_t size, void * const file);

/// An interface that represents a Wasm interpreter.
typedef struct wasm_interpreter_t {
    /// A unique identifier for the interpreter.
    const char * name;

    /// Initializes the interpreter and loads the specified Wasm file.
    wasm_memory_region_t (*load)(const sb_file_directory_e directory, const char * const wasm_filename, const size_t sizeof_application_workingset);

    /// Initializes the interpreter and loads the specified Wasm binary from a reader function.
    wasm_memory_region_t (*load_fp)(void * const wasm_file, const wasm_fread_t fread_func, const size_t wasm_file_content_size, const size_t sizeof_application_workingset);

    /// Unloads the Wasm binary and deinitializes the interpreter.
    void (*unload)(wasm_memory_region_t wasm_memory);

    /// Gets a string that represents the current Wasm call stack.
    const char * (*get_callstack)(void);

    /// Translates a pointer from the Wasm address space into a pointer in the native address space.
    void * (*translate_ptr_wasm_to_native)(wasm_ptr_t wasm_ptr);

    /// Translates a pointer from the native address space into a pointer in the Wasm address space, if applicable.
    wasm_ptr_t (*translate_ptr_native_to_wasm)(void * native_ptr);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_void)(const char * const name);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_i)(const char * const name, const uint32_t arg0);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ii)(const char * const name, const uint32_t arg0, const uint32_t arg1);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iii)(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iiii)(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iiiii)(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint32_t arg2, const uint32_t arg3, const uint32_t arg4);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iI)(const char * const name, const uint32_t arg0, const uint64_t arg1);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ip)(const char * const name, const uint32_t arg0, void * const arg1);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_Ii)(const char * const name, const uint64_t arg0, const uint32_t arg1);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iiIi)(const char * const name, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iIi)(const char * const name, const uint32_t arg0, const uint64_t arg1, const uint32_t arg2);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ipi)(const char * const name, const uint32_t arg0, void * const arg1, const uint32_t arg2);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iIIi)(const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_iIIii)(const char * const name, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ifI)(const char * const name, const uint32_t arg0, const float arg1, const uint64_t arg2);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri)(const char * const name, uint32_t * const ret);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_i)(const char * const name, uint32_t * const ret, const uint32_t arg0);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_iI)(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_ip)(const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_iiIi)(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, const uint64_t arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_iipi)(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint32_t arg1, void * const arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_iIIi)(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_ippi)(const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_iIIii)(const char * const name, uint32_t * const ret, const uint32_t arg0, const uint64_t arg1, const uint64_t arg2, const uint32_t arg3, const uint32_t arg4);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_ippii)(const char * const name, uint32_t * const ret, const uint32_t arg0, void * const arg1, void * const arg2, const uint32_t arg3, const uint32_t arg4);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_ifI)(const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, const uint64_t arg2);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_ifp)(const char * const name, uint32_t * const ret, const uint32_t arg0, const float arg1, void * const arg2);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_ri_argv)(const char * const name, uint32_t * const ret, const uint32_t argc, const char ** const argv);

    /// Performs a C -> Wasm function call.
    wasm_call_result_t (*call_rI)(const char * const name, uint64_t * const ret);
} wasm_interpreter_t;

// Include this in an individual interpreter implementation to make missing signatures a compile-time error.
#define WASM_CALLEE_REQUIRED_SIGNATURES  \
    WASM_CALLEE_SIGNATURE(void),         \
        WASM_CALLEE_SIGNATURE(i),        \
        WASM_CALLEE_SIGNATURE(ii),       \
        WASM_CALLEE_SIGNATURE(iii),      \
        WASM_CALLEE_SIGNATURE(iiii),     \
        WASM_CALLEE_SIGNATURE(iiiii),    \
        WASM_CALLEE_SIGNATURE(iI),       \
        WASM_CALLEE_SIGNATURE(ip),       \
        WASM_CALLEE_SIGNATURE(Ii),       \
        WASM_CALLEE_SIGNATURE(iIi),      \
        WASM_CALLEE_SIGNATURE(ipi),      \
        WASM_CALLEE_SIGNATURE(iiIi),     \
        WASM_CALLEE_SIGNATURE(iIIi),     \
        WASM_CALLEE_SIGNATURE(iIIii),    \
        WASM_CALLEE_SIGNATURE(ifI),      \
        WASM_CALLEE_SIGNATURE(ri),       \
        WASM_CALLEE_SIGNATURE(ri_i),     \
        WASM_CALLEE_SIGNATURE(ri_iI),    \
        WASM_CALLEE_SIGNATURE(ri_ip),    \
        WASM_CALLEE_SIGNATURE(ri_iiIi),  \
        WASM_CALLEE_SIGNATURE(ri_iipi),  \
        WASM_CALLEE_SIGNATURE(ri_iIIi),  \
        WASM_CALLEE_SIGNATURE(ri_ippi),  \
        WASM_CALLEE_SIGNATURE(ri_iIIii), \
        WASM_CALLEE_SIGNATURE(ri_ippii), \
        WASM_CALLEE_SIGNATURE(ri_ifI),   \
        WASM_CALLEE_SIGNATURE(ri_ifp),   \
        WASM_CALLEE_SIGNATURE(ri_argv),  \
        WASM_CALLEE_SIGNATURE(rI)

void set_active_wasm_interpreter(wasm_interpreter_t * const interpreter);
wasm_interpreter_t * get_active_wasm_interpreter(void);

void * wasm_translate_ptr_wasm_to_native(wasm_ptr_t addr);
wasm_ptr_t wasm_translate_ptr_native_to_wasm(void * addr);

#endif // FFI_GEN