/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/telemetry/telemetry.h"

#include "source/adk/log/log.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"

#include <stdint.h>

#define TAG_TELEMETRY FOURCC(' ', 'T', 'L', 'M')

enum {
    telemetry_region_size = 8 * 1024 * 1024,
};

static struct {
    mem_region_t telemetry_region;
#ifdef GUARD_PAGE_SUPPORT
    mem_region_t guard_pages;
#endif
} statics;

typedef uint64_t telemetry_capture_mask_t;

typedef struct telemetry_system_name_to_mask_t {
    const char * name;
    telemetry_capture_mask_t mask;
} telemetry_system_name_to_mask_t;

static const telemetry_system_name_to_mask_t telemetry_system_name_to_mask_dictionary[] = {
    {"wasm_ffi", WASM_FFI_TRACE_MASK},
    {"wasm_fn", WASM_FN_TRACE_MASK},
    {"rhi", RHI_TRACE_MASK},
    {"app_thunk", APP_THUNK_TRACE_MASK},
    {"merlin", MERLIN_TRACE_MASK},
    {"cg", CG_TRACE_MASK},
    {"cg_font", CG_FONT_TRACE_MASK},
    {"cg_gl", CG_GL_TRACE_MASK},
    {"cg_image", CG_IMAGE_TRACE_MASK},
    {"cache", CACHE_TRACE_MASK},
    {"httpx", HTTPX_TRACE_MASK},
    {"websocket", WEBSOCKET_MINIMAL_TRACE_MASK},
    {"curl_common", CURL_COMMON_TRACE_MASK},
    {"runtime", RUNTIME_TRACE_MASK},
    {"json_deflate", JSON_DEFLATE_TRACE_MASK},
    {"glfw", GLFW_TRACE_MASK},
    {"app", APP_TRACE_MASK},
    {"websocket_full", WEBSOCKET_FULL_TRACE_MASK},

    {"canvas", CG_TRACE_MASK | CG_GL_TRACE_MASK | CG_FONT_TRACE_MASK | CG_IMAGE_TRACE_MASK},
    {"http", HTTPX_TRACE_MASK | CURL_COMMON_TRACE_MASK | JSON_DEFLATE_TRACE_MASK | HTTP_CURL_TRACE_MASK},
    {"websocket", WEBSOCKET_MINIMAL_TRACE_MASK | CURL_COMMON_TRACE_MASK},
    {"startup", MERLIN_TRACE_MASK | CACHE_TRACE_MASK | APP_THUNK_TRACE_MASK | RUNTIME_TRACE_MASK | GLFW_TRACE_MASK},
    {"wasm", WASM_FFI_TRACE_MASK | WASM_FN_TRACE_MASK},
    {"gfx", RHI_TRACE_MASK},
};

static void telemetry_set_capture_mask(const char * const telemetry_groups) {
    static const char * const delimiters = ", ";

    if (telemetry_groups != NULL) {
        telemetry_capture_mask_t capture_mask = 0;

        const char * p = telemetry_groups;
        while (*p != '\0') {
            const size_t length = strcspn(p, delimiters);

            for (size_t i = 0; i < ARRAY_SIZE(telemetry_system_name_to_mask_dictionary); ++i) {
                const telemetry_system_name_to_mask_t * const name_to_mask = &telemetry_system_name_to_mask_dictionary[i];
                if (strlen(name_to_mask->name) == length && memcmp(name_to_mask->name, p, length) == 0) {
                    capture_mask |= name_to_mask->mask;
                    break;
                }
            }

            p += length;
            p += strspn(p, delimiters);
        }

        tmSetCaptureMask(capture_mask);
    }
}

void telemetry_print_help() {
    debug_write_line("\nCore telemetry:");

    debug_write_line("\t--telemetry-server <IP:port>");
    debug_write_line("\t--telemetry <comma-delimited list of systems>");

    debug_write_line("\n\tTelemetry systems:");

    for (size_t i = 0; i < ARRAY_SIZE(telemetry_system_name_to_mask_dictionary); ++i) {
        const telemetry_system_name_to_mask_t * const name_to_mask = &telemetry_system_name_to_mask_dictionary[i];
        debug_write_line("\t* %s", name_to_mask->name);
    }
}

void telemetry_init(const char * const address, const int port, const char * const telemetry_groups) {
    {
        const size_t needed = ALIGN_INT(telemetry_region_size, 8);
        const size_t page_size = get_sys_page_size();
        const size_t block_size = ALIGN_INT(needed, page_size);
#ifdef GUARD_PAGE_SUPPORT
        const size_t total_size = block_size + page_size * 2;

        statics.guard_pages = debug_sys_map_pages(total_size, system_page_protect_no_access, MALLOC_TAG);
        VERIFY(statics.guard_pages.ptr);

        const uintptr_t uptr = statics.guard_pages.adr;
        statics.telemetry_region = MEM_REGION(.adr = uptr + page_size, .size = block_size);
        debug_sys_protect_pages(statics.telemetry_region, system_page_protect_read_write);
#else
        statics.telemetry_region = sb_map_pages(block_size, system_page_protect_read_write);
#endif
    }

    adk_system_metrics_t metrics = {0};
    adk_get_system_metrics(&metrics);

    static char program_name[128];
    sprintf_s(program_name, ARRAY_SIZE(program_name), "m5-%s-%s-%s", metrics.core_version, metrics.config, metrics.device);

    tmInitialize((tm_uint32)statics.telemetry_region.size, statics.telemetry_region.ptr);
    const tm_error tm_err = tmOpen(0, program_name, __DATE__ " "__TIME__, address, TMCT_TCP, (tm_uint16)port, TMOF_INIT_NETWORKING, 300);
    switch (tm_err) {
        case TM_OK:
            LOG_ALWAYS(TAG_TELEMETRY, "telemetry (rad tools telemetry) is enabled and running");
            break;
        case TMERR_DISABLED:
            LOG_ALWAYS(TAG_TELEMETRY, "Telemetry is disabled via #define NTELEMETRY");
            break;
        case TMERR_UNINITIALIZED:
            LOG_ALWAYS(TAG_TELEMETRY, "tmInitialize failed or was not called");
            break;
        case TMERR_NETWORK_NOT_INITIALIZED:
            LOG_ALWAYS(TAG_TELEMETRY, "WSAStartup was not called before tmOpen! Call WSAStartup or pass TMOF_INIT_NETWORKING.");
            break;
        case TMERR_NULL_API:
            LOG_ALWAYS(TAG_TELEMETRY, "There is no Telemetry API (the DLL isn't in the EXE's path)!");
            break;
        case TMERR_COULD_NOT_CONNECT:
            LOG_ALWAYS(TAG_TELEMETRY, "There is no Telemetry server running\nServer must be started for telemetry to run and may be found at `ncp-m5/extern/rad-tools-telemetry/telemetry/tm3/<platform>/.../tools_<platform>/rad_telemetry`\nVerify the key is in the same folder as your executable.");
            break;
        default:
            LOG_ALWAYS(TAG_TELEMETRY, "Unknown rad-tools error type: [%i]", tm_err);
            break;
    }

    telemetry_set_capture_mask(telemetry_groups);
}

void telemetry_shutdown() {
    tmClose(0);
    tmShutdown();

#ifdef GUARD_PAGE_SUPPORT
    if (statics.guard_pages.ptr) {
        debug_sys_unmap_pages(statics.guard_pages, MALLOC_TAG);
    }
#else
    if (statics.telemetry_region.ptr) {
        sb_unmap_pages(statics.telemetry_region);
    }
#endif
}

void app_telemetry_push(const char * const name) {
    tmEnter(APP_TRACE_MASK, 0, name);
}

void app_telemetry_pop(void) {
    tmLeave(APP_TRACE_MASK);
}

void app_telemetry_span_begin(const uint64_t id, const char * const str) {
    tmBeginTimeSpan(APP_TRACE_MASK, (tm_uint64)id, TMZF_NONE, "%s", str);
}

void app_telemetry_span_end(const uint64_t id) {
    tmEndTimeSpan(APP_TRACE_MASK, (tm_uint64)id);
}
