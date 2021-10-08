/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rhi_gl_api.c

rhi_gl_api interface
*/

#include _PCH
#include "rhi_gl_device.h"
#include "rhi_gl_shared.h"

static bool gl_rhi_api_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, rhi_error_t ** const out_error);
static int gl_rhi_api_add_ref(rhi_resource_t * resource);
static int gl_rhi_api_release(rhi_resource_t * resource, const char * const tag);

static const rhi_resource_vtable_t gl_rhi_api_resource_vtable = {
    .add_ref = gl_rhi_api_add_ref,
    .release = gl_rhi_api_release};

static const rhi_api_vtable_t gl_rhi_api_vtable = {
    .init = gl_rhi_api_init,
    .create_device = gl_rhi_api_create_device};

rhi_api_t rhi_api_gl = {
    .resource = {.vtable = &gl_rhi_api_resource_vtable},
    .vtable = &gl_rhi_api_vtable};

/*
=======================================
gl_rhi_api_init
=======================================
*/

static bool gl_rhi_api_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, rhi_error_t ** const out_error) {
    if (out_error) {
        *out_error = NULL;
    }
    rhi_api_gl.resource.ref_count.i32 = 1;
    gl_init_heap(region, guard_page_mode);
    return true;
}

/*
=======================================
gl_rhi_api_shutdown
=======================================
*/

static void gl_rhi_api_shutdown(const char * const tag) {
    gl_destroy_heap(tag);
}

/*
=======================================
gl_rhi_api_add_ref
=======================================
*/

static int gl_rhi_api_add_ref(rhi_resource_t * resource) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, 1, memory_order_relaxed);
    ASSERT(r > 0);
    return r;
}

/*
=======================================
gl_rhi_api_release
=======================================
*/

static int gl_rhi_api_release(rhi_resource_t * resource, const char * const tag) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, -1, memory_order_relaxed);
    ASSERT(r > 0);
    if (r == 1) {
        gl_rhi_api_shutdown(tag);
    }

    return r;
}
