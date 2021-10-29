/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/extender/extender_status.h"
#include "source/adk/ffi/ffi.h"
#include "source/adk/interpreter/interp_api.h"
#include "source/adk/telemetry/telemetry.h"

#define TAG_APP FOURCC('A', 'P', 'P', '\0')

typedef uint64_t wasm_sig_mask;

EXT_EXPORT void wasm_sig(wasm_sig_mask mask, char * const out);

EXT_EXPORT void * wasm_translate_ptr_wasm_to_native(wasm_ptr_t addr);

FFI_EXPORT FFI_PTR_NATIVE const char * adk_get_wasm_call_stack(void);

typedef bool (*wasm_interpreter_initializer)(const wasm_memory_region_t wasm_memory, const size_t wasm_bytecode_size);

wasm_memory_region_t wasm_load_common(
    const sb_file_directory_e directory,
    const char * const wasm_filename,
    const size_t wasm_low_heap_size,
    const size_t wasm_high_heap_size,
    wasm_interpreter_initializer initializer,
    const size_t heap_allocation_threshold);

wasm_memory_region_t wasm_load_fp_common(
    void * const wasm_file,
    const wasm_fread_t fread_func,
    const size_t wasm_file_content_size,
    const size_t wasm_low_heap_size,
    const size_t wasm_high_heap_size,
    wasm_interpreter_initializer initializer,
    const size_t heap_allocation_threshold);

wasm_memory_region_t alloc_wasm_memory(const size_t wasm_bytecode_size, const size_t wasm_low_heap_size, const size_t wasm_high_heap_size);

void free_mem_region(const adk_mem_region_t region);

void zero_mem_region(adk_mem_region_t region);

wasm_call_result_t wasm_argv_call(wasm_interpreter_t * const interpreter, const char * const name, uint32_t * const ret, const uint32_t argc, const char ** const argv);
