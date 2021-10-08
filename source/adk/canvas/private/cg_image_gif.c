/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
cg_image_gif.c

canvas gif support
*/

#include "source/adk/canvas/cg.h"
#include "source/adk/imagelib/imagelib.h"
#include "source/adk/log/log.h"

#define TAG_CG_IMG_GIF FOURCC('I', 'M', 'G', 'F')

void destroy_gif(cg_image_t * const image, const char * const tag) {
    cg_async_image_t * const cg_async_image = &image->gif->async_image_data;
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

    LL_REMOVE(image, gif->prev_gif, gif->next_gif, ctx->gif_head, ctx->gif_tail);

    cg_free(&ctx->cg_heap_low, image->gif, tag);
    cg_free(&ctx->cg_heap_low, image, tag);
}

static void gif_decode_next_frame_job(void * void_user, thread_pool_t * const pool) {
    cg_image_t * const cg_image = void_user;
    cg_async_image_t * const cg_async_image = &cg_image->gif->async_image_data;
    cg_context_t * const ctx = cg_async_image->cg;

    const uint32_t req_frame_count = cg_image->gif->req_frame_count;
    const uint32_t frame_delta = min_uint32_t(req_frame_count - cg_image->gif->done_frame_count, 2);
    cg_image->gif->done_frame_count = req_frame_count;

    if (frame_delta < 1) {
        return;
    }

    const microseconds_t start_time = adk_read_microsecond_clock();

    render_conditional_flush_cmd_stream_and_wait_fence(ctx->gl->render_device, &cg_async_image->decode_cmd_stream, cg_async_image->upload_fence);
    for (uint32_t i = 0; i < frame_delta; ++i) {
        imagelib_gif_load_next_frame_from_memory(
            cg_async_image->resident_bytes.region,
            &cg_image->image,
            &cg_image->gif->next_frame_duration,
            cg_image->pixel_buffer.region,
            cg_async_image->working_buffer.region,
            (cg_image->image_animation_state == cg_image_animation_restart) ? imagelib_gif_force_restart : imagelib_gif_continue);
        cg_image->image_animation_state = cg_image_animation_running;
    }

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

    const microseconds_t end_time = adk_read_microsecond_clock();
    cg_image->gif->decoded_frame_time.us += end_time.us - start_time.us;
    cg_image->gif->decoded_frame_count += frame_delta;
}

static void gif_check_restart_job(void * void_user, thread_pool_t * const pool) {
    cg_image_t * const cg_image = void_user;
    cg_async_image_t * const cg_async_image = &cg_image->gif->async_image_data;
    cg_context_t * const ctx = cg_async_image->cg;

    if (cg_async_image->pending_destroy) {
        destroy_gif(cg_image, MALLOC_TAG);
        return;
    }

    if (cg_image->gif->decoded_frame_time.us > 1000 * 1000) {
        LOG_ALWAYS(TAG_CG_IMG_GIF, "[gif-decode-fps: %d", (int)((double)cg_image->gif->decoded_frame_count / ((double)cg_image->gif->decoded_frame_time.us / (1000.0 * 1000.0))));
        cg_image->gif->decoded_frame_count = 0;
        cg_image->gif->decoded_frame_time.us = 0;
    }

    if (cg_image->gif->req_frame_count != cg_image->gif->done_frame_count) {
        thread_pool_enqueue_front(ctx->thread_pool, gif_decode_next_frame_job, gif_check_restart_job, cg_image);
    } else {
        cg_async_image->decode_job_running = false;
    }
}

void cg_context_tick_gifs(cg_context_t * const ctx, const milliseconds_t delta_time) {
    for (cg_image_t * cg_image = ctx->gif_head; cg_image != NULL; cg_image = cg_image->gif->next_gif) {
        cg_async_image_t * const cg_async_image = &cg_image->gif->async_image_data;
        if (cg_image && (cg_image->status == cg_image_async_load_complete) && (cg_image->image_animation_state != cg_image_animation_stopped)) {
            cg_image->gif->frame_remaining_duration_in_ms -= (int32_t)delta_time.ms;

            if ((cg_image->gif->frame_remaining_duration_in_ms <= 0) && !cg_async_image->decode_job_running) {
                cg_image->gif->frame_remaining_duration_in_ms = cg_image->gif->next_frame_duration.ms;

                render_conditional_flush_cmd_stream_and_wait_fence(
                    ctx->gl->render_device,
                    &ctx->gl->render_device->default_cmd_stream,
                    cg_image->cg_texture.texture->resource.fence);

                ++cg_image->gif->req_frame_count;
                if (!cg_async_image->decode_job_running) {
                    cg_async_image->decode_job_running = true;
                    thread_pool_enqueue_front(ctx->thread_pool, gif_decode_next_frame_job, gif_check_restart_job, cg_image);
                }
            }
        }
    }
}
