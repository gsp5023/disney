/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
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

#include "rhi.h"
#include "source/adk/imagelib/imagelib.h"

static inline int rhi_release(rhi_resource_t * const resource, const char * const tag) {
    return resource->vtable->release(resource, tag);
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
