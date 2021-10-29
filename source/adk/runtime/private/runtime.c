/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
runtime.c

Basic runtime stuff, asserts
*/

#include _PCH

#include "source/adk/runtime/runtime.h"

#include "source/adk/interpreter/interp_api.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/private/events.h"
#include "source/adk/steamboat/sb_display.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"
#include "source/adk/telemetry/telemetry.h"

/*
=======================================
ADK initialization
=======================================
*/

static adk_api_t api;

static adk_memory_map_t allocate_mmap(const system_guard_page_mode_e guard_page_mode, const adk_low_memory_reservations_t memory_reservations) {
    RUNTIME_TRACE_PUSH_FN();
    static const int num_systems = sizeof(adk_low_memory_reservations_t) / sizeof(uint32_t);
    const uint32_t * const sizes = (const uint32_t *)&memory_reservations;

    adk_memory_map_t mmap;
    ZEROMEM(&mmap);

    adk_mem_region_t * const spans = (adk_mem_region_t *)&mmap;

#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        // full guard pages means each subsystem maps it's own memory pages per-allocation
        // so just fill in the sizes here.
        for (int i = 0; i < num_systems; ++i) {
            const size_t size = sizes[i];
            ASSERT_MSG(sizes[i], "reservation size at [%i] is zero", i);
            spans[i].region.size = size;
        }
    } else if (guard_page_mode == system_guard_page_mode_minimal) {
        for (int i = 0; i < num_systems; ++i) {
            const size_t size = sizes[i];
            ASSERT_MSG(sizes[i], "reservation size at [%i] is zero", i);
            if (size > 0) {
                const size_t page_size = get_sys_page_size();
                const size_t page_aligned_size = PAGE_ALIGN_INT(size);
                const size_t num_pages = page_aligned_size / page_size;
                // no-access pages
                const mem_region_t pages = sb_map_pages((num_pages + 2) * page_size, system_page_protect_no_access);
                TRAP_OUT_OF_MEMORY(pages.ptr);
                ASSERT_PAGE_ALIGNED(pages.ptr);
                // unprotect the pages we can touch
                sb_protect_pages(
                    MEM_REGION(
                            .adr = pages.adr + page_size,
                            .size = num_pages * page_size),
                    system_page_protect_read_write);
                // figure out the user space
                const mem_region_t user_space = {
                    {{{.adr = pages.adr + ((num_pages + 1) * page_size) - size},
                      .size = size}}};

                spans[i].region = user_space;
                spans[i].internal.pages = pages;
            }
        }
    } else
#endif
    {
        size_t total_size = 0;

        for (int i = 0; i < num_systems; ++i) {
            ASSERT_MSG(sizes[i], "reservation size at [%i] is zero", i);
            total_size += ALIGN_INT(sizes[i], 8);
        }

        const size_t aligned_total_size = PAGE_ALIGN_INT(total_size);

        const mem_region_t membase = sb_map_pages(aligned_total_size, system_page_protect_read_write);
        TRAP_OUT_OF_MEMORY(membase.ptr);
        ASSERT_PAGE_ALIGNED(membase.ptr);

        size_t ofs = 0;

        for (int i = 0; i < num_systems; ++i) {
            const size_t size = ALIGN_INT(sizes[i], 8);
            ASSERT(size > 0);
            if (size > 0) {
                spans[i] = (adk_mem_region_t){
                    .region = MEM_REGION(
                            .adr = membase.adr + ofs,
                            .size = size)};
                ofs += size;
            }
        }

        mmap.internal.page_memory = membase;
    }

    RUNTIME_TRACE_POP();
    return mmap;
}

static void free_mmap(const adk_memory_map_t mmap) {
    RUNTIME_TRACE_PUSH_FN();
#ifdef GUARD_PAGE_SUPPORT
    static const int num_systems = sizeof(adk_low_memory_reservations_t) / sizeof(uint32_t);
    const adk_mem_region_t * const spans = (const adk_mem_region_t *)&mmap;

    for (int i = 0; i < num_systems; ++i) {
        if (spans[i].internal.pages.adr) {
            sb_unmap_pages(spans[i].internal.pages);
        }
    }
#endif

    if (mmap.internal.page_memory.adr) {
        sb_unmap_pages(mmap.internal.page_memory);
    }
    RUNTIME_TRACE_POP();
}

const adk_api_t * adk_init(const int argc, const char * const * const argv, const system_guard_page_mode_e guard_page_mode, const adk_memory_reservations_t memory_reservations, const char * const tag) {
    RUNTIME_TRACE_PUSH_FN();

    ASSERT_MSG(get_sys_page_size() > 0, "sb_preinit() must be called before adk_init.");
    ZEROMEM(&api);
    api.high_mem_reservations = memory_reservations.high;
    api.mmap = allocate_mmap(guard_page_mode, memory_reservations.low);
#ifndef _STB_NATIVE
    api.runtime_flags.headless = findarg("--headless", argc, argv) != -1;
#endif

    RUNTIME_TRACE_PUSH("sb_init");
    if (!sb_init(&api, argc, argv, guard_page_mode)) {
        free_mmap(api.mmap);
        RUNTIME_TRACE_POP();
        RUNTIME_TRACE_POP();
        return NULL;
    }
    RUNTIME_TRACE_POP();

    {
        RUNTIME_TRACE_PUSH("rhi_init");
        rhi_error_t * err = NULL;
        VERIFY(rhi_init(api.rhi, api.mmap.rhi.region, guard_page_mode, &err));
        if (err) {
            rhi_error_release(err, MALLOC_TAG);
        }
        RUNTIME_TRACE_POP();
    }

    RUNTIME_TRACE_POP();
    return &api;
}

// Temporary hook for initializing the NVE simple player
const adk_api_t * nve_adk_initialize() {
    if (!sb_preinit(0, NULL)) {
        return NULL;
    }

    return adk_init(
        0,
        NULL,
        system_guard_page_mode_disabled,
        adk_get_default_memory_reservations(),
        MALLOC_TAG);
}

void adk_shutdown(const char * const tag) {
    RUNTIME_TRACE_PUSH_FN();

    api.rhi->resource.vtable->release(&api.rhi->resource, tag);
    sb_destroy_main_window();
    sb_shutdown();
    free_mmap(api.mmap);

    RUNTIME_TRACE_POP();
}

void adk_get_system_metrics(adk_system_metrics_t * const out) {
    RUNTIME_TRACE_PUSH_FN();

    ZEROMEM(out);
    sb_get_system_metrics(out);
    adk_runtime_override_system_metrics(out);
#if defined(BRCM_DEVICE_OVERRIDE_HIGH) 
    // Force metrics device alignment if we are overriding
    VERIFY(0 == strcpy_s(out->device, ARRAY_SIZE(out->device), "97xx_high"));
#elif defined(BRCM_DEVICE_OVERRIDE_MID) 
    // Force metrics device alignment if we are overriding
    VERIFY(0 == strcpy_s(out->device, ARRAY_SIZE(out->device), "97xx_mid"));
#elif defined(BRCM_DEVICE_OVERRIDE_LOW)
    // Force metrics device alignment if we are overriding
    VERIFY(0 == strcpy_s(out->device, ARRAY_SIZE(out->device), "97xx_low"));
#endif    
    VERIFY(0 == strcpy_s(out->core_version, ARRAY_SIZE(out->core_version), ADK_VERSION_STRING));
#if defined(_SHIP)
#if defined(NDEBUG)
    VERIFY(0 == strcpy_s(out->config, ARRAY_SIZE(out->config), "ship"));
#else // !NDEBUG
    VERIFY(0 == strcpy_s(out->config, ARRAY_SIZE(out->config), "ship-dbg"));
#endif // END if(NDEBUG)
#else // !_SHIP
#if defined(NDEBUG)
    VERIFY(0 == strcpy_s(out->config, ARRAY_SIZE(out->config), "dev"));
#else // !NDEBUG
    VERIFY(0 == strcpy_s(out->config, ARRAY_SIZE(out->config), "debug"));
#endif // END if(NDEBUG)
#endif // END if(_SHIP)
    RUNTIME_TRACE_POP();
}

adk_memory_reservations_t adk_get_default_memory_reservations() {
    return (adk_memory_reservations_t){
        .low = {
#if defined(_VADER) || defined(_LEIA)
            // resolve unit test failure during directory tests
            .runtime = 512 * 1024,
#else
            .runtime = 16 * 1024,
#endif
            .rhi = 256 * 1024,
            // space for rendering device + 64kb for heap
            .render_device = GET_RENDER_DEVICE_SIZE(32, 64 * 1024, 1) + 64 * 1024,
#if defined(_VADER) || defined(_LEIA)
            .curl = 64 * 1024 * 1024,
#else
            .curl = 2 * 1024 * 1024,
#endif
            .curl_fragment_buffers = 4 * 1024 * 1024,
            .bundle = 128 * 1024,
            .canvas = 14 * 1024 * 1024,
            .canvas_font_scratchpad = 1 * 1024 * 1024,
            .cncbus = 2 * 1024 * 1024,
            .json_deflate = 10 * 1024 * 1024,
            .default_thread_pool = 256 * 1024,
            .ssl = 10 * 1024 * 1024,
            .http2 = 2 * 1024 * 1024,
            .httpx = 2 * 1024 * 1024,
            .httpx_fragment_buffers = 4 * 1024 * 1024,
            .reporting = 2 * 1024 * 1024,
        },
        .high = {
            .canvas = 48 * 1024 * 1024,
        }};
}

/*
=======================================
DEBUG
=======================================
*/

void (*__assert_failed_hook)(const char * const message, const char * const filename, const char * const function, const int line);

void assert_failed(const char * const message, const char * const filename, const char * const function, const int line) {
    if (__assert_failed_hook) {
        __assert_failed_hook(message, filename, function, line);
    } else {
        sb_assert_failed(message, filename, function, line);
    }
}

void debug_write(const char * const msg, ...) {
    va_list args;
    va_start(args, msg);
    sb_vadebug_write(msg, args);
    va_end(args);
}

void debug_write_line(const char * const msg, ...) {
    va_list args;
    va_start(args, msg);
    sb_vadebug_write_line(msg, args);
    va_end(args);
}

/*
=======================================
command line arg parsing
=======================================
*/

int findarg(const char * arg, const int argc, const char * const * const argv) {
    for (int i = 0; i < argc; ++i) {
        if (!strcasecmp(arg, argv[i])) {
            return i;
        }
    }

    return -1;
}

const char * getargarg(const char * arg, const int argc, const char * const * const argv) {
    const int i = findarg(arg, argc, argv);
    if ((i != -1) && ((i + 1) < argc)) {
        return argv[i + 1];
    }

    return NULL;
}

/*
=======================================
portable string functions
=======================================
*/

int sb_vsprintf_s(char * buffer, size_t size, const char * format, va_list argptr) {
#if (defined(_WIN32) || defined(_WIN64))
    return vsprintf_s(buffer, size, format, argptr);
#else
    ASSERT(buffer != NULL);
    ASSERT(format != NULL);
    ASSERT(size > 0);

    const int bytes_written = vsnprintf(buffer, size, format, argptr);

    ASSERT_MSG(bytes_written >= 0, "vsnprintf error has occured");
    ASSERT_MSG((int)size > bytes_written + 1, "attempted buffer overflow");

    return bytes_written;
#endif
}

int sb_vsnprintf_s(char * buffer, size_t size, size_t count, const char * format, va_list argptr) {
#if (defined(_WIN32) || defined(_WIN64))
    return vsnprintf_s(buffer, size, count, format, argptr);
#else
    // Not attempting to write anything, just return
    if (count == 0) {
        return 0;
    }

    ASSERT(buffer != NULL);
    ASSERT(format != NULL);
    ASSERT(count > 0);
    ASSERT(size > 0);

    size_t max_length = (count + 1) < size ? (count + 1) : size;
    const int bytes_written = vsnprintf(buffer, max_length, format, argptr);

    ASSERT_MSG(bytes_written >= 0, "vsnprintf error has occured");

    if (bytes_written + 1 > (int)size && count >= size) {
        ASSERT_MSG((int)size > bytes_written + 1, "attempted buffer overflow");
        return -1;
    }

    return bytes_written < (int)max_length - 1 ? bytes_written : (int)max_length - 1;
#endif
}

const char * adk_get_env(const char * const env_name) {
#if defined(_VADER) || defined(_LEIA)
    return NULL;
#else
    ASSERT(env_name);

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996) // "this function may be considered unsafe
#endif
    RUNTIME_TRACE_PUSH_FN();
    const char * const env_ret = getenv(env_name);
    RUNTIME_TRACE_POP();
    return env_ret;
#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
}
