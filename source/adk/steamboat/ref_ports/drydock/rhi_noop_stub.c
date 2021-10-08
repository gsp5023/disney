/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "impl_tracking.h"
#include "source/adk/renderer/private/rhi.h"

static int rhi_stub_add_ref(struct rhi_resource_t * const resource) {
    UNUSED(resource);

    return 0;
}

static int rhi_stub_release(rhi_resource_t * resource, const char * const tag) {
    UNUSED(resource);
    UNUSED(tag);

    return 0;
}

static bool rhi_stub_init(
    const mem_region_t region,
    const system_guard_page_mode_e guard_page_mode,
    rhi_error_t ** const out_error) {
    UNUSED(region);
    UNUSED(out_error);

    return false;
}

static rhi_device_t * rhi_stub_create_device(
    struct sb_window_t * const window,
    rhi_error_t ** const out_error,
    const char * const tag) {
    UNUSED(window);
    UNUSED(out_error);
    UNUSED(tag);

    return NULL;
}

const rhi_resource_vtable_t rhi_stub_resource_vtable = {
    .add_ref = rhi_stub_add_ref,
    .release = rhi_stub_release};

const rhi_api_vtable_t rhi_stub_api_vtable = {
    .init = rhi_stub_init,
    .create_device = rhi_stub_create_device};

rhi_api_t stub_rhi_api = {
    .resource = {.vtable = &rhi_stub_resource_vtable},
    .vtable = &rhi_stub_api_vtable};
