/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rhi_gl_shared.c

gl shared stuff
*/

#include _PCH
#include "rhi_gl_shared.h"

#include "source/adk/steamboat/sb_thread.h"

sb_atomic_int32_t gl_next_obj_id = {0};

/*
===============================================================================
opengl RHI heap
===============================================================================
*/

static struct {
    heap_t heap;
    sb_mutex_t * mutex;
} statics;

/*
=======================================
gl_init_heap
=======================================
*/

void gl_init_heap(const mem_region_t region, const system_guard_page_mode_e guard_page_mode) {
    statics.mutex = sb_create_mutex(MALLOC_TAG);
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&statics.heap, region.size, 8, 0, "rhi_gl_heap", guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(&statics.heap, region, 8, 0, "rhi_gl_heap");
    }
}

/*
=======================================
gl_destroy_heap
=======================================
*/

void gl_destroy_heap(const char * const tag) {
#ifndef _SHIP
    heap_debug_print_leaks(&statics.heap);
#endif
    heap_destroy(&statics.heap, MALLOC_TAG);
    sb_destroy_mutex(statics.mutex, MALLOC_TAG);
    ZEROMEM(&statics);
}

/*
=======================================
gl_dump_heap_usage
=======================================
*/

void gl_dump_heap_usage() {
    sb_lock_mutex(statics.mutex);
    heap_dump_usage(&statics.heap);
    sb_unlock_mutex(statics.mutex);
}

/*
=======================================
gl_unchecked_alloc
=======================================
*/

void * gl_unchecked_alloc(const size_t size, const char * const tag) {
    sb_lock_mutex(statics.mutex);
    void * const p = heap_unchecked_alloc(&statics.heap, size, tag);
    sb_unlock_mutex(statics.mutex);
    return p;
}

/*
=======================================
gl_alloc
=======================================
*/

void * gl_alloc(const size_t size, const char * const tag) {
    sb_lock_mutex(statics.mutex);
    void * const p = heap_alloc(&statics.heap, size, tag);
    sb_unlock_mutex(statics.mutex);
    return p;
}

/*
=======================================
gl_realloc
=======================================
*/

void * gl_realloc(void * const p, const size_t size, const char * const tag) {
    sb_lock_mutex(statics.mutex);
    void * const r = heap_realloc(&statics.heap, p, size, tag);
    sb_unlock_mutex(statics.mutex);
    return r;
}

/*
=======================================
gl_free
=======================================
*/

void gl_free(void * const p, const char * const tag) {
    sb_lock_mutex(statics.mutex);
    heap_free(&statics.heap, p, tag);
    sb_unlock_mutex(statics.mutex);
}

/*
===============================================================================
Opengl resource helpers
===============================================================================
*/

/*
=======================================
gl_resource_add_ref
=======================================
*/

static int gl_resource_add_ref(rhi_resource_t * const resource) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, 1, memory_order_relaxed);
    ASSERT(r > 0);
    return r;
}

/*
=======================================
gl_resource_release
=======================================
*/

static int gl_resource_release(rhi_resource_t * const resource, const char * const tag) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, -1, memory_order_relaxed);
    ASSERT(r > 0);
    gl_resource_t * const gl_resource = (gl_resource_t *)resource;
    if (r == 1) {
        rhi_resource_t * const outer = gl_resource->outer;
        if (gl_resource->vtable) {
            gl_resource->vtable->destroy(gl_resource, tag);
        }
        gl_free(gl_resource, MALLOC_TAG);
        if (outer) {
            rhi_release(outer, tag);
        }
    }

    return r;
}

static const rhi_resource_vtable_t gl_resource_rhi_resource_vtable = {
    .add_ref = gl_resource_add_ref,
    .release = gl_resource_release};

/*
=======================================
gl_create_resource
=======================================
*/

gl_resource_t * gl_create_resource(const size_t size, rhi_resource_t * const outer, const gl_resource_vtable_t * const vtable, const char * const tag) {
    ASSERT(size >= sizeof(gl_resource_t));

    gl_resource_t * const resource = (gl_resource_t *)gl_alloc(size, tag);
    memset(resource, 0, size);
    resource->outer = outer;
    resource->resource.ref_count.i32 = 1;
    resource->resource.instance_id = gl_get_next_obj_id();
    resource->resource.vtable = &gl_resource_rhi_resource_vtable;
    resource->resource.tag = tag;
    resource->vtable = vtable;

    if (outer) {
        rhi_add_ref(outer);
    }

    return resource;
}

/*
=======================================
gl_create_error
=======================================
*/

typedef struct gl_error_t {
    rhi_error_t resource;
    char * msg;
} gl_error_t;

/*
=======================================
gl_error_add_ref
=======================================
*/

static int gl_error_add_ref(rhi_resource_t * const resource) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, 1, memory_order_relaxed);
    ASSERT(r > 0);
    return r;
}

/*
=======================================
gl_error_release
=======================================
*/

static int gl_error_release(rhi_resource_t * const resource, const char * const tag) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, -1, memory_order_relaxed);
    ASSERT(r > 0);

    gl_error_t * const err = (gl_error_t *)resource;
    if (r == 1) {
        if (err->msg) {
            free(err->msg);
        }
        free(err);
    }

    return r;
}

static const rhi_resource_vtable_t gl_error_rhi_resource_vtable = {
    .add_ref = gl_error_add_ref,
    .release = gl_error_release};

static const char * gl_error_get_message(const rhi_error_t * const err) {
    return ((gl_error_t *)err)->msg;
}

static const rhi_error_vtable_t gl_error_vtable = {
    .get_message = gl_error_get_message};

/*
=======================================
gl_create_error
=======================================
*/

rhi_error_t * gl_create_error(const char * const msg) {
    gl_error_t * err = (gl_error_t *)malloc(sizeof(gl_error_t));
    TRAP_OUT_OF_MEMORY(err);
    ZEROMEM(err);
    err->resource.resource.instance_id = gl_get_next_obj_id();
    err->resource.resource.ref_count.i32 = 1;
    err->resource.resource.vtable = &gl_error_rhi_resource_vtable;
    err->resource.vtable = &gl_error_vtable;
#ifdef _MSC_VER
    err->msg = _strdup(msg);
#else
    err->msg = strdup(msg);
#endif
    TRAP_OUT_OF_MEMORY(err->msg);
    return &err->resource;
}
