/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/interpreter/interp_common.h"

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_platform.h"

heap_t wasm_heap;

void wasm_sig(wasm_sig_mask mask, char * const out) {
    while (!(mask & 0xF)) {
        mask >>= 4;
    }

    const char SIG_CHARS[16] = {
        ' ',
        'v',
        ' ',
        ' ',
        'i',
        ' ',
        ' ',
        ' ',
        'I',
        ' ',
        '*',
        ' ',
        ' ',
        'F',
        ' ',
        'f',
    };

    int c = 0;
    for (int i = 0; i <= 52 / 4; i++) {
        const uint8_t atom = (uint8_t)(mask >> (uint64_t)(52 - i * 4)) & 0xF;
        if (!atom) {
            continue;
        }
        if (!c) {
            out[c++] = SIG_CHARS[atom];
            out[c++] = '(';
        } else {
            out[c++] = SIG_CHARS[atom];
        }
    }
    out[c] = ')';
    out[c + 1] = 0;
}

const char * adk_get_wasm_call_stack(void) {
    return get_active_wasm_interpreter()->get_callstack();
}

wasm_call_result_t wasm_argv_call(wasm_interpreter_t * const interpreter, const char * const name, uint32_t * const ret, const uint32_t argc, const char ** const argv) {
    uint64_t wide_argv[128] = {0};
    VERIFY_MSG(argc <= ARRAY_SIZE(wide_argv), "[%s] Variadic call can handle up to %d arguments, but %d were provided", interpreter->name, ARRAY_SIZE(wide_argv), argc);

    for (uint32_t i = 0; i < argc; i++) {
        wide_argv[i] = (uint64_t)(uintptr_t)argv[i];
    }

    return interpreter->call_ri_ip(name, ret, argc, &wide_argv[0]);
}

static adk_mem_region_t allocate_adk_mem_region(const system_guard_page_mode_e guard_page_mode, const size_t alloc_size) {
    ASSERT(alloc_size > 0);
    adk_mem_region_t memory_region = {0};
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode != system_guard_page_mode_disabled) {
        const size_t page_aligned_size = PAGE_ALIGN_INT(alloc_size);
        const size_t num_pages = page_aligned_size / get_sys_page_size();
        // no-access pages
        const mem_region_t pages = sb_map_pages((num_pages + 2) * get_sys_page_size(), system_page_protect_no_access);
        TRAP_OUT_OF_MEMORY(pages.ptr);
        // unprotect the pages we can touch
        sb_protect_pages(
            MEM_REGION(
                    .adr = pages.adr + get_sys_page_size(),
                    .size = num_pages * get_sys_page_size()),
            system_page_protect_read_write);
        // figure out the user space
        const mem_region_t user_space = MEM_REGION(
                .adr = pages.adr + ((num_pages + 1) * get_sys_page_size()) - alloc_size,
                .size = alloc_size);

        memory_region.region = user_space;
        memory_region.internal.pages = pages;
    } else
#endif
    {
        const size_t aligned_total_size = PAGE_ALIGN_INT(alloc_size);
        const mem_region_t membase = sb_map_pages(aligned_total_size, system_page_protect_read_write);
        TRAP_OUT_OF_MEMORY(membase.ptr);
        memory_region.region = membase;
    }

    return memory_region;
}

// Allocates a region for the wasm bytecode followed by the working set, properly aligned
wasm_memory_region_t alloc_wasm_memory(const size_t wasm_bytecode_size, const size_t sizeof_application_workingset) {
    const size_t aligned_wasm_bytecode_size = ALIGN_INT(wasm_bytecode_size, 8);
    const size_t total_wasm_memory_required = aligned_wasm_bytecode_size + ALIGN_INT(sizeof_application_workingset, 8);
    return (wasm_memory_region_t){
        .wasm_bytecode_size = wasm_bytecode_size,
        .wasm_mem_region = allocate_adk_mem_region(system_guard_page_mode_enabled, total_wasm_memory_required)};
}

static mem_region_t get_wasm_interpreter_region(const wasm_memory_region_t wasm_memory, const size_t wasm_bytecode_size) {
    const size_t aligned_wasm_bytecode_size = ALIGN_INT(wasm_bytecode_size, 8);
    return MEM_REGION(
            .ptr = wasm_memory.wasm_mem_region.region.byte_ptr + aligned_wasm_bytecode_size,
            .size = wasm_memory.wasm_mem_region.region.size - aligned_wasm_bytecode_size);
}

static void initialize_wasm_heap(const wasm_memory_region_t wasm_memory, const size_t wasm_bytecode_size) {
    const mem_region_t wasm_interpreter_region = get_wasm_interpreter_region(wasm_memory, wasm_bytecode_size);

#ifdef GUARD_PAGE_SUPPORT
    if (the_app.guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&wasm_heap, wasm_interpreter_region.size, 8, 0, "wasm_heap", system_guard_page_mode_minimal, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(&wasm_heap, wasm_interpreter_region, 8, 0, "wasm_heap");
    }
}

wasm_memory_region_t wasm_load_common(const sb_file_directory_e directory, const char * const wasm_filename, const size_t sizeof_application_workingset, wasm_interpreter_initializer initializer) {
    // Identify the size of the Wasm artifact.
    const size_t wasm_bytecode_size = get_artifact_size(directory, wasm_filename);
    if (!wasm_bytecode_size) {
        return (wasm_memory_region_t){0};
    }

    // Calculate a region that is sufficient to host the artifact in alignment.
    wasm_memory_region_t wasm_memory = alloc_wasm_memory(wasm_bytecode_size, sizeof_application_workingset);

    const mem_region_t wasm_bytecode = MEM_REGION(
            .ptr = wasm_memory.wasm_mem_region.region.ptr,
            .size = wasm_bytecode_size);

    if (load_artifact_data(directory, wasm_bytecode, wasm_filename, 0)) {
        initialize_wasm_heap(wasm_memory, wasm_bytecode_size);
        if (!initializer(wasm_memory, wasm_bytecode_size)) {
            free_mem_region(wasm_memory.wasm_mem_region);
            return (wasm_memory_region_t){0};
        }
    } else {
        free_mem_region(wasm_memory.wasm_mem_region);
        wasm_memory.wasm_mem_region.region.ptr = NULL;
    }

    return wasm_memory;
}

wasm_memory_region_t wasm_load_fp_common(
    void * const wasm_file,
    const wasm_fread_t fread_func,
    const size_t wasm_file_content_size,
    const size_t sizeof_application_workingset,
    wasm_interpreter_initializer initializer) {
    if (!wasm_file_content_size) {
        return (wasm_memory_region_t){0};
    }

    wasm_memory_region_t wasm_memory = alloc_wasm_memory(wasm_file_content_size, sizeof_application_workingset);

    if (fread_func(wasm_memory.wasm_mem_region.region.ptr, wasm_file_content_size, wasm_file) == wasm_file_content_size) {
        initialize_wasm_heap(wasm_memory, wasm_file_content_size);
        if (!initializer(wasm_memory, wasm_file_content_size)) {
            free_mem_region(wasm_memory.wasm_mem_region);
            return (wasm_memory_region_t){0};
        }
    } else {
        free_mem_region(wasm_memory.wasm_mem_region);
        wasm_memory.wasm_mem_region.region.ptr = NULL; // return size for error handling
    }

    return wasm_memory;
}

// Returns the interpreter region within the previously allocated wasm memory region
void free_mem_region(const adk_mem_region_t region) {
#ifdef GUARD_PAGE_SUPPORT
    if (region.internal.pages.adr) {
        sb_unmap_pages(region.internal.pages);
    }
#else
    if (region.region.adr) {
        sb_unmap_pages(region.region);
    }
#endif
}

void zero_mem_region(adk_mem_region_t region) {
#ifdef GUARD_PAGE_SUPPORT
    if (region.internal.pages.adr) {
        ZEROMEM(&region.internal.pages);
    }
#else
    if (region.region.adr) {
        ZEROMEM(&region.region);
    }
#endif
}