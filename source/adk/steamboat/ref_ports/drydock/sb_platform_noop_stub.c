/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH

#include "impl_tracking.h"
#include "source/adk/renderer/private/rhi.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_platform.h"

bool sb_preinit(const int argc, const char * const * const argv) {
    UNUSED(argc);
    UNUSED(argv);
    NOT_IMPLEMENTED_EX;

    return true;
}

bool sb_init(adk_api_t * api, const int argc, const char * const * const argv, const system_guard_page_mode_e guard_page_mode) {
    if (api) {
        extern rhi_api_t stub_rhi_api; // Declared in rhi_noop_stub.c
        api->rhi = &stub_rhi_api;
    }

    UNUSED(argc);
    UNUSED(argv);
    UNUSED(guard_page_mode);

    NOT_IMPLEMENTED_EX;

    return true;
}

void sb_shutdown(void) {
    NOT_IMPLEMENTED_EX;
}

void sb_halt(const char * const message) {
    UNUSED(message);

    NOT_IMPLEMENTED_EX;
}

void sb_platform_dump_heap_usage(void) {
    NOT_IMPLEMENTED_EX;
}

heap_metrics_t sb_platform_get_heap_metrics(void) {
    NOT_IMPLEMENTED_EX;
    return (heap_metrics_t){0};
}

void sb_on_app_load_failure(void) {
    NOT_IMPLEMENTED_EX;
}

mem_region_t sb_map_pages(
    const size_t size,
    const system_page_protect_e protect) {
    mem_region_t m = {0};

    UNUSED(size);
    UNUSED(protect);

    NOT_IMPLEMENTED_EX;

    return m;
}

void sb_protect_pages(
    const mem_region_t pages,
    const system_page_protect_e protect) {
    UNUSED(pages);
    UNUSED(protect);

    NOT_IMPLEMENTED_EX;
}

void sb_unmap_pages(const mem_region_t pages) {
    UNUSED(pages);

    NOT_IMPLEMENTED_EX;
}

void sb_assert_failed(
    const char * const message,
    const char * const filename,
    const char * const function,
    const int line) {
    UNUSED(message);
    UNUSED(filename);
    UNUSED(function);
    UNUSED(line);

    NOT_IMPLEMENTED_EX;
}

void sb_notify_app_status(const sb_app_notify_e notify) {
    UNUSED(notify);

    NOT_IMPLEMENTED_EX;
}

void sb_get_system_metrics(adk_system_metrics_t * const out) {
    UNUSED(out);

    NOT_IMPLEMENTED_EX;
}

void sb_tick(
    const adk_event_t ** const head,
    const adk_event_t ** const tail) {
    UNUSED(head);
    UNUSED(tail);

    NOT_IMPLEMENTED_EX;
}

const_mem_region_t sb_get_deeplink_buffer(const sb_deeplink_handle_t * const handle) {
    UNUSED(handle);

    NOT_IMPLEMENTED_EX;

    return CONST_MEM_REGION(.ptr = NULL, .size = 0);
}

void sb_release_deeplink(sb_deeplink_handle_t * const handle) {
    UNUSED(handle);

    NOT_IMPLEMENTED_EX;
}

nanoseconds_t sb_read_nanosecond_clock() {
    NOT_IMPLEMENTED_EX;

    return (nanoseconds_t){0};
}

sb_time_since_epoch_t sb_get_time_since_epoch(void) {
    sb_time_since_epoch_t t = {0};

    NOT_IMPLEMENTED_EX;

    return t;
}

void sb_text_to_speech(const char * const text) {
    UNUSED(text);

    NOT_IMPLEMENTED_EX;
}

sb_uuid_t sb_generate_uuid() {
    NOT_IMPLEMENTED_EX;
    return (sb_uuid_t){0};
}

void sb_report_app_metrics(
    const char * const app_id,
    const char * const app_name,
    const char * const app_version) {
    UNUSED(app_id);
    UNUSED(app_name);
    UNUSED(app_version);

    NOT_IMPLEMENTED_EX;
}

int64_t sb_get_localtime_offset() {
    NOT_IMPLEMENTED_EX;
    return 0;
}
