/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rad_tools.c

rad tools telemetry integration for profiling function calls
*/

#include "source/adk/telemetry/telemetry.h"

#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"

#define TAG_TELEMETRY FOURCC(' ', 'R', 'A', 'D')

enum {
    telemetry_region_size = 8 * 1024 * 1024,
};

static struct {
    mem_region_t telemetry_region;
#ifdef GUARD_PAGE_SUPPORT
    mem_region_t guard_pages;
#endif
} statics;

void telemetry_init(const char * const address, const int port) {
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
        //const uintptr_t ublock = uptr + page_size + block_size - needed;
    }

    tmInitialize((tm_uint32)statics.telemetry_region.size, statics.telemetry_region.ptr);
    const tm_error tm_err = tmOpen(0, "m5", __DATE__ " "__TIME__, address, TMCT_TCP, (tm_uint16)port, TMOF_INIT_NETWORKING, 300);
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
            LOG_ALWAYS(TAG_TELEMETRY, "Uknown rad-tools error type: [%i]", tm_err);
            break;
    }
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