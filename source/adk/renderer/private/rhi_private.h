/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
rhi_private.h

RENDER HARDWARE INTERFACE

Private APIs for use by RHI implementations
*/

#include "rbcmd.h"
#include "rhi.h"
#include "source/adk/imagelib/imagelib.h"

static inline int rhi_release(rhi_resource_t * const resource, const char * const tag) {
    return resource->vtable->release(resource, tag);
}

static inline void render_cmd_release_rhi_resource(const render_cmd_release_rhi_resource_indirect_t * const cmd_args) {
    // NOTE: we can emit release commands that may try to release
    // resources that fail to create. instead of making the caller
    // block to wait-and-see if the resource fails and conditionally
    // releasing, just NULL check here and avoid complexity.

    const char * const tag =
#if ENABLE_RENDER_TAGS
        cmd_args->tag;
#else
        MALLOC_TAG;
#endif

    rhi_resource_t * const resource = *cmd_args->resource;
    if (resource) {
        rhi_release(*cmd_args->resource, tag);
    }
}

static inline void image_vertical_flip_in_place(const image_t * const img) {
    const int pitch = img->pitch;
    uint8_t * top = (uint8_t *)img->data;
    uint8_t * bottom = top + (img->pitch) * (img->height - 1);
    while (top < bottom) {
        int ofs = 0;
        for (int i = 0; i < img->pitch; ++i) {
            const uint8_t t = top[ofs];
            top[ofs] = bottom[ofs];
            bottom[ofs] = t;
            ++ofs;
        }
        top += pitch;
        bottom -= pitch;
    }
}

#ifdef __cplusplus
}
#endif
