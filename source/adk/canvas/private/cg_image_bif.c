/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
cg_image_bif.c

canvas bif support
*/

#include "source/adk/canvas/cg.h"
#include "source/adk/imagelib/imagelib.h"

#if defined(_VADER) || defined(_LEIA)
void cg_image_expand4(image_t * const image_to_expand, const mem_region_t expanded_pixels_region);
#endif

void destroy_bif(cg_image_t * const image, const char * const tag) {
    cg_async_image_t * const cg_async_image = &image->bif->async_image_data;
    cg_context_t * const ctx = cg_async_image->cg;

    render_conditional_flush_cmd_stream_and_wait_fence(ctx->gl->render_device, &cg_async_image->decode_cmd_stream, cg_async_image->upload_fence);

    cg_gl_texture_free(ctx->gl, &image->cg_texture);

    if (cg_async_image->working_buffer.region.ptr) {
        cg_free_alloc(cg_async_image->working_buffer, tag);
    }

    if (image->pixel_buffer.region.ptr) {
        cg_free_alloc(image->pixel_buffer, tag);
    }

    if (cg_async_image->resident_bytes.region.ptr) {
        cg_free_const_alloc(cg_async_image->resident_bytes, tag);
    }

    cg_free(&ctx->cg_heap_low, image->bif, tag);
    cg_free(&ctx->cg_heap_low, image, tag);
}

static void bif_decode_next_frame_job(void * void_user, thread_pool_t * const pool) {
    cg_image_t * const cg_image = void_user;
    cg_async_image_t * const cg_async_image = &cg_image->bif->async_image_data;
    cg_context_t * const ctx = cg_async_image->cg;

    const uint32_t req_frame_index = cg_image->bif->req_frame_index;

    if (req_frame_index == cg_image->bif->decoded_frame_index) {
        return;
    }

    cg_image->bif->decoded_frame_index = req_frame_index;

    render_conditional_flush_cmd_stream_and_wait_fence(ctx->gl->render_device, &cg_async_image->decode_cmd_stream, cg_async_image->upload_fence);

    imagelib_load_bif_jpg_frame_from_memory(
        cg_async_image->resident_bytes.region,
        &cg_image->image,
        req_frame_index,
        cg_image->pixel_buffer.region,
        cg_async_image->working_buffer.region);

#if defined(_VADER) || defined(_LEIA)
    cg_image_expand4(&cg_image->image, cg_image->pixel_buffer.region);
#endif

    image_mips_t mipmaps;
    ZEROMEM(&mipmaps);

    mipmaps.num_levels = 1;
    mipmaps.levels[0] = cg_image->image;

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &cg_async_image->decode_cmd_stream,
        render_cmd_buf_write_upload_texture_indirect,
        &cg_image->cg_texture.texture->texture,
        mipmaps,
        MALLOC_TAG);

    // update fence so we don't cg_free the data before this command is processed
    cg_async_image->upload_fence = render_flush_cmd_stream(&cg_async_image->decode_cmd_stream, render_no_wait);
}

static void bif_check_restart_job(void * void_user, thread_pool_t * const pool) {
    cg_image_t * const cg_image = void_user;
    cg_async_image_t * const cg_async_image = &cg_image->bif->async_image_data;
    cg_context_t * const ctx = cg_async_image->cg;

    if (cg_async_image->pending_destroy) {
        destroy_bif(cg_image, MALLOC_TAG);
        return;
    }

    if (cg_image->bif->req_frame_index != cg_image->bif->decoded_frame_index) {
        thread_pool_enqueue_front(ctx->thread_pool, bif_decode_next_frame_job, bif_check_restart_job, cg_image);
    } else {
        cg_async_image->decode_job_running = false;
    }
}

void cg_context_set_image_frame_index(cg_image_t * const cg_image, const uint32_t image_index) {
    if ((cg_image->status != cg_image_async_load_complete) || !cg_image->bif) {
        return;
    }

    cg_image->bif->req_frame_index = clamp_int(image_index, 0, cg_image->num_frames - 1);
    cg_async_image_t * const cg_async_image = &cg_image->bif->async_image_data;

    if (!cg_async_image->decode_job_running) {
        cg_async_image->decode_job_running = true;
        thread_pool_enqueue_front(cg_async_image->cg->thread_pool, bif_decode_next_frame_job, bif_check_restart_job, cg_image);
    }
}

uint32_t cg_context_get_image_frame_count(const cg_image_t * const cg_image) {
    if (cg_image->status != cg_image_async_load_complete) {
        return 1;
    }
    return cg_image->num_frames;
}
