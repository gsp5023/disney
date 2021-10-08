/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/file/file.h"
#include "source/adk/interpreter/interp_common.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/wasm3/private/wasm3.h"

extern heap_t wasm_heap;

// Initializes the wasm3 app loaded in a wasm memory region
static bool initialize_wasm3(const wasm_memory_region_t wasm_memory, const size_t wasm_bytecode_size) {
    wasm3_global_environment = m3_NewEnvironment();
    wasm3_global_runtime = m3_NewRuntime(wasm3_global_environment, 512 * 1024, NULL);
    wasm3_app_module = NULL;

    M3Result wasm3_result = m3_ParseModule(wasm3_global_environment, &wasm3_app_module, wasm_memory.wasm_mem_region.region.ptr, (uint32_t)wasm_bytecode_size);
    if (wasm3_result) {
        LOG_ERROR(TAG_APP, wasm3_result);
        return false;
    }

    M3Result wasm3_load_result = m3_LoadModule(wasm3_global_runtime, wasm3_app_module);
    if (wasm3_load_result) {
        LOG_ERROR(TAG_APP, wasm3_load_result);
        return false;
    }

    wasm3_run_all_linkers();

    return true;
}

wasm_memory_region_t load_wasm3(const sb_file_directory_e directory, const char * const wasm_filename, const size_t sizeof_application_workingset) {
    return wasm_load_common(directory, wasm_filename, sizeof_application_workingset, initialize_wasm3);
}

wasm_memory_region_t load_wasm3_fp(
    void * const wasm_file,
    const wasm_fread_t fread_func,
    const size_t wasm_file_content_size,
    const size_t sizeof_application_workingset) {
    return wasm_load_fp_common(wasm_file, fread_func, wasm_file_content_size, sizeof_application_workingset, initialize_wasm3);
}

void unload_wasm3(wasm_memory_region_t wasm_memory) {
    // Do not free the module. The runtime owns it.
    wasm3_app_module = NULL;

    m3_FreeRuntime(wasm3_global_runtime);
    wasm3_global_runtime = NULL;

    m3_FreeEnvironment(wasm3_global_environment);
    wasm3_global_environment = NULL;

    free_mem_region(wasm_memory.wasm_mem_region);

    heap_destroy(&wasm_heap, MALLOC_TAG);
}
