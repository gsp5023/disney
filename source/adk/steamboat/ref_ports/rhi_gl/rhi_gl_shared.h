/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
rhi_gl_device.h

OpenGL rendering device
*/

#include "rhi_gl_config.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/steamboat/private/private_apis.h"
#include "source/adk/steamboat/sb_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

// all RHI objects created by the GL backend
// have an internal id to avoid collisions when
// objects are recycled in pools (i.e. comparing
// the address of 2 objects isn't a guarantee
// that it hasn't changed).
static inline int gl_get_next_obj_id() {
    extern sb_atomic_int32_t gl_next_obj_id;
    return sb_atomic_fetch_add(&gl_next_obj_id, 1, memory_order_relaxed) + 1;
}

/*
===============================================================================
gl_resource_t

All objects in the GL pipeline (buffers, shaders, anything with an id) are
represented with a gl_resource_t. A gl_resource_t can be contained inside
another rhi object, which it maintains a reference to. Typically this is used
when device objects are created, the objects maintain a reference to the device
and the device is not destroyed until all the objects are released.
===============================================================================
*/

struct gl_resource_t;

typedef struct gl_resource_vtable_t {
    void (*destroy)(struct gl_resource_t * const resource, const char * const tag);
} gl_resource_vtable_t;

typedef struct gl_resource_t {
    rhi_resource_t resource;
    const gl_resource_vtable_t * vtable;
    rhi_resource_t * outer;
} gl_resource_t;

gl_resource_t * gl_create_resource(const size_t size, rhi_resource_t * const outer, const gl_resource_vtable_t * const vtable, const char * const tag);

/*
===============================================================================
gl rhi memory routines

These allocate out of the GL RHI API heap and are thread safe.
===============================================================================
*/

void gl_init_heap(const mem_region_t region, const system_guard_page_mode_e guard_page_mode);
void gl_destroy_heap(const char * const tag);
void gl_dump_heap_usage();
void * gl_alloc(const size_t size, const char * const tag);
void * gl_unchecked_alloc(const size_t size, const char * const tag);
void * gl_realloc(void * const p, const size_t size, const char * const tag);
void gl_free(void * const p, const char * const tag);

/*
=======================================
gl_create_error

Creates an error object from the specified string.

NOTES: This uses the programs free store for allocation, you can safely
rhi_release() this object even if the API was not successfully initialized.
=======================================
*/

#define GL_CREATE_ERROR_PRINTF(_msg, ...) gl_create_error(VAPRINTF(MALLOC_TAG ": " _msg, __VA_ARGS__))
#define GL_CREATE_ERROR(_msg) gl_create_error(MALLOC_TAG ": " _msg)

rhi_error_t * gl_create_error(const char * const msg);

#ifdef __cplusplus
}
#endif
