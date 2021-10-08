/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 cg_image.c

 canvas image support
 */

#include "cg_gzip.h"
#include "source/adk/bundle/bundle.h"
#include "source/adk/canvas/cg.h"
#include "source/adk/file/file.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/imagelib/imagelib.h"
#include "source/adk/log/log.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef _CG_IMAGE_TRACE
#include "source/adk/telemetry/telemetry.h"
#define CG_IMAGE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ##__VA_ARGS__)
#define CG_IMAGE_TIME_SPAN_END(_id) TRACE_TIME_SPAN_END(_id)
#else
#define CG_IMAGE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define CG_IMAGE_TIME_SPAN_END(_id)
#endif

#ifndef _SHIP
#define CG_IMAGE_TIME_LOGGING
#endif

#define TAG_CG_IMG FOURCC('C', 'I', 'M', 'G')

extern cg_statics_t cg_statics;

typedef struct stbi_alloc_user_t {
    const char * tag;
    cg_heap_t * cg_heap;
} stbi_alloc_user_t;

static const rhi_sampler_state_desc_t the_sampler_state = {
    .max_anisotropy = 1,
    .min_filter = rhi_min_filter_linear,
    .max_filter = rhi_max_filter_linear,
    .u_wrap_mode = rhi_wrap_mode_wrap,
    .v_wrap_mode = rhi_wrap_mode_wrap,
    .w_wrap_mode = rhi_wrap_mode_wrap,
    .border_color = {1, 1, 1, 1}};

void destroy_gif(cg_image_t * const image, const char * const tag);
void destroy_bif(cg_image_t * const image, const char * const tag);
static void cg_image_load_user_http_failure_cleanup(void * const void_user);

void cg_context_image_free(cg_image_t * const image, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();
    if ((image->status != cg_image_async_load_pending) || image->load_user) {
        if (image->load_user) {
            cg_image_load_user_http_failure_cleanup(image->load_user);
        }
        if (image->gif) {
            cg_async_image_t * const cg_async_image = &image->gif->async_image_data;
            if (cg_async_image->decode_job_running) {
                ASSERT(!cg_async_image->pending_destroy);
                cg_async_image->pending_destroy = true;
                return;
            }

            destroy_gif(image, tag);
            return;
        }
        if (image->bif) {
            cg_async_image_t * const cg_async_image = &image->bif->async_image_data;
            if (cg_async_image->decode_job_running) {
                ASSERT(!cg_async_image->pending_destroy);
                cg_async_image->pending_destroy = true;
                return;
            }

            destroy_bif(image, tag);
            return;
        }
        if (image->image_mask.data_len) {
            cg_gl_texture_free(image->cg_ctx->gl, &image->cg_texture_mask);
        }

        cg_gl_texture_free(image->cg_ctx->gl, &image->cg_texture);
        cg_free(&image->cg_ctx->cg_heap_low, image, tag);
    } else {
        image->status = cg_image_async_load_aborted;
    }
}

typedef struct free_pixels_user_t {
    cg_heap_t * cg_heap;
    cg_allocation_t * pixel_allocation;
    const char * tag;
} free_pixels_user_t;

static void async_free_pixels(rhi_device_t * _, void * const free_pixels_user) {
    free_pixels_user_t * const pixels_user = (free_pixels_user_t *)free_pixels_user;
    cg_free_alloc(*pixels_user->pixel_allocation, pixels_user->tag);
    ZEROMEM(pixels_user->pixel_allocation);
    cg_free(pixels_user->cg_heap, free_pixels_user, pixels_user->tag);
}

typedef enum image_load_type_e {
    image_load_type_file,
    image_load_type_url,
    image_load_type_bundle
} image_load_type_e;

typedef enum cg_image_type_e {
    cg_image_type_static = 0,
    cg_image_type_gif = 1,
    cg_image_type_bif = 2,
} cg_image_type_e;

typedef struct image_load_data_t {
    cg_image_t * cg_image;
    cg_heap_t * resource_heap;

    cg_image_type_e image_type;

    adk_curl_handle_t * curl_handle;
    cg_allocation_t image_bytes;
    cg_allocation_t header_bytes;

    size_t working_buffer_size;

    char * url;
    sb_file_t * file;

    cg_memory_region_e memory_region;
    image_load_type_e image_load_type;
    bool hit_oom;

#ifdef CG_IMAGE_TIME_LOGGING
    nanoseconds_t request_start;
    nanoseconds_t request_end;
    nanoseconds_t decode_delay_start;
    nanoseconds_t decode_delay_end;
    nanoseconds_t decode_start;
    nanoseconds_t decode_end;
    nanoseconds_t rhi_delay_start;
    nanoseconds_t rhi_delay_end;
    nanoseconds_t rhi_upload_start;
    nanoseconds_t rhi_upload_end;
#endif
} image_load_data_t;

#ifdef CG_IMAGE_TIME_LOGGING
static inline double nanosecond_to_ms_dt(const nanoseconds_t start, const nanoseconds_t end) {
    const double ns_to_ms = 1000000.0;
    return (double)(end.ns - start.ns) / ns_to_ms;
}

static inline void gpu_fetch_decode_upload_timing_printout(const image_load_data_t * const user) {
    if (!cg_statics.ctx->log_image_timing) {
        return;
    }

    LOG_ALWAYS(
        TAG_CG_IMG,
        "[%s]\n"
        "    fetch: [%f]ms decode: [%f]ms rhi_upload: [%f]ms\n"
        "    decode-delay: [%f]ms rhi-upload-delay: [%f]ms",
        user->url,
        nanosecond_to_ms_dt(user->request_start, user->request_end),
        nanosecond_to_ms_dt(user->decode_start, user->decode_end),
        nanosecond_to_ms_dt(user->rhi_upload_start, user->rhi_upload_end),
        nanosecond_to_ms_dt(user->decode_delay_start, user->decode_delay_end),
        nanosecond_to_ms_dt(user->rhi_delay_start, user->rhi_delay_end));
}
#endif

static void null_free_callback(rhi_device_t * _, void * const __) {
}

static bool parse_gpu_ready_image_format(image_load_data_t * const user) {
    // gpu-ready image formats bypass the normal decode pipeline because
    // they are essentially instant-parsed and can be queued for upload immediately

#if defined(_LEIA) //this code is stripped from a partner build and must be conditionally wrapped.
    if (imagelib_load_gnf_from_memory(user->image_bytes.consted.region, &user->cg_image->image)) {
        return true;
    }
#endif

    if (imagelib_load_pvr_from_memory(user->image_bytes.consted.region, &user->cg_image->image)) {
        const size_t alpha_size = user->image_bytes.consted.region.size - user->cg_image->image.data_len - 52; // sizeof(pvr_header_t)

        user->cg_image->image_mask.data_len = 0;

        if (alpha_size > 0) {
            // the alpha bits are the image width*height in size
            user->cg_image->image_mask.data = user->image_bytes.region.byte_ptr + user->image_bytes.region.size - alpha_size;
            user->cg_image->image_mask.data_len = (uint32_t)alpha_size;
            user->cg_image->image_mask.bpp = 1;
            user->cg_image->image_mask.width = user->cg_image->image.width;
            user->cg_image->image_mask.height = user->cg_image->image.height;
            user->cg_image->image_mask.depth = 1;
            user->cg_image->image_mask.encoding = image_encoding_uncompressed;
            user->cg_image->image_mask.x = 0;
            user->cg_image->image_mask.y = 0;
            user->cg_image->image_mask.pitch = 1;
            user->cg_image->image_mask.spitch = 0;
        }
        return true;
    }

    return false;
}

static bool upload_gpu_ready_image_format(image_load_data_t * const user) {
    const bool has_alpha_bytes = user->cg_image->image_mask.data_len > 0;

    user->cg_image->pixel_buffer = user->image_bytes;

    free_pixels_user_t * const free_pixels_user = (free_pixels_user_t *)cg_alloc(&user->cg_image->cg_ctx->cg_heap_low, sizeof(free_pixels_user_t), MALLOC_TAG);
    *free_pixels_user = (free_pixels_user_t){.cg_heap = &user->cg_image->cg_ctx->cg_heap_low, .pixel_allocation = &user->cg_image->pixel_buffer, .tag = MALLOC_TAG};

    if (!cg_gl_texture_from_memory(user->cg_image->cg_ctx->gl->render_device, &user->cg_image->cg_texture, the_sampler_state, user->cg_image->image, cg_gl_static_texture_usage, has_alpha_bytes ? null_free_callback : async_free_pixels, free_pixels_user)) {
        LOG_ERROR(TAG_CG_IMG, "The specified image_encoding [%d] is not supported by this platform.", (int)user->cg_image->image.encoding);
        cg_free(&user->cg_image->cg_ctx->cg_heap_low, free_pixels_user, MALLOC_TAG);
        return false;
    }

    if (has_alpha_bytes) {
        if (!cg_gl_texture_from_memory(user->cg_image->cg_ctx->gl->render_device, &user->cg_image->cg_texture_mask, the_sampler_state, user->cg_image->image_mask, cg_gl_static_texture_usage, async_free_pixels, free_pixels_user)) {
            LOG_ERROR(TAG_CG_IMG, "The specified image_encoding [%d] is not supported by this platform.", (int)user->cg_image->image.encoding);
            cg_free(&user->cg_image->cg_ctx->cg_heap_low, free_pixels_user, MALLOC_TAG);
            return false;
        }
    }
    return true;
}

static void rhi_upload_job_main_thread(void * void_user, thread_pool_t * const pool) {
    ASSERT_IS_MAIN_THREAD();

    image_load_data_t * const user = void_user;
    cg_context_t * const cg_ctx = user->cg_image->cg_ctx;

#ifdef CG_IMAGE_TIME_LOGGING
    user->rhi_delay_end = sb_read_nanosecond_clock();
    CG_IMAGE_TIME_SPAN_END(user->url);
    CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[RHI_upload] %s", user->url);

    user->rhi_upload_start = sb_read_nanosecond_clock();
#endif

    if (user->cg_image->status == cg_image_async_load_aborted) {
        if (user->image_bytes.region.ptr != NULL) {
            cg_free_alloc(user->image_bytes, MALLOC_TAG);
        }

        if (user->cg_image->pixel_buffer.region.ptr != NULL) {
            cg_free_alloc(user->cg_image->pixel_buffer, MALLOC_TAG);
            // pixel buffer may also be used by gifs; and if the image canceled is a gif/bif we could try to double free this allocation.
            // zero the memory region so we know it's gone and don't try to free it.
            user->cg_image->pixel_buffer.region = (mem_region_t){0};
        }

        cg_context_image_free(user->cg_image, MALLOC_TAG);
    } else if (user->hit_oom) {
        LOG_WARN(TAG_CG_IMG,
                 "Image loading failed due to out of memory.\n"
                 "Could not finish loading image: [%s]\n"
                 "File size:            [%" PRIu64
                 "]\n"
                 "Pixel buffer size:    [%" PRIu64
                 "]\n"
                 "Working buffer size:  [%" PRIu64 "]",
                 user->url,
                 (uint64_t)user->image_bytes.region.size,
                 (uint64_t)user->cg_image->pixel_buffer.region.size,
                 (uint64_t)user->working_buffer_size);
        user->cg_image->status = cg_image_async_load_out_of_memory;

    } else if ((user->image_load_type == image_load_type_file) && user->file == NULL) {
        LOG_WARN(TAG_CG_IMG, "Image loading failed. Could not open file: [%s]", user->url);
        user->cg_image->status = cg_image_async_load_file_error;

    } else if (user->cg_image->image.data == NULL) {
        // if pixels are NULL we did not recognize the format, or there was a decoding error, or the image was corrupted
        user->cg_image->status = cg_image_async_load_unrecognized_image_format;

        // file corruption or invalid/unrecognized format
        LOG_ERROR(TAG_CG_IMG, "Unrecognized image format [%s]", user->url);

    } else if (user->cg_image->status == cg_image_async_load_pending) {
        // enqueue a render command to upload the image in rhi
        // this is a cheap operation
        user->cg_image->status = cg_image_async_load_complete;

        if (user->image_type == cg_image_type_static) {
            // image_encoding_uncompressed
            if (user->cg_image->image.encoding == image_encoding_uncompressed) {
                free_pixels_user_t * free_pixels_user = (free_pixels_user_t *)cg_alloc(&cg_ctx->cg_heap_low, sizeof(free_pixels_user_t), MALLOC_TAG);
                *free_pixels_user = (free_pixels_user_t){.cg_heap = &cg_ctx->cg_heap_low, .pixel_allocation = &user->cg_image->pixel_buffer, .tag = MALLOC_TAG};
                // stbi image load paths have no non-terminal case for failure
                VERIFY(cg_gl_texture_from_memory(cg_ctx->gl->render_device, &user->cg_image->cg_texture, the_sampler_state, user->cg_image->image, cg_gl_static_texture_usage, async_free_pixels, free_pixels_user));
            } else if (!upload_gpu_ready_image_format(user)) {
                // gpu format unsupported on this platform, cleanup
                cg_free_alloc(user->image_bytes, MALLOC_TAG);
                user->cg_image->status = cg_image_async_load_unrecognized_image_format;
            }
        } else {
            VERIFY(cg_gl_texture_from_memory(cg_ctx->gl->render_device, &user->cg_image->cg_texture, the_sampler_state, user->cg_image->image, cg_gl_dynamic_texture_usage, null_free_callback, NULL));

            if (user->image_type == cg_image_type_bif) {
                render_init_cmd_stream(cg_ctx->gl->render_device, &user->cg_image->bif->async_image_data.decode_cmd_stream);

            } else {
                ASSERT(user->image_type == cg_image_type_gif);
                render_init_cmd_stream(cg_ctx->gl->render_device, &user->cg_image->gif->async_image_data.decode_cmd_stream);
                LL_ADD(user->cg_image, gif->prev_gif, gif->next_gif, cg_ctx->gif_head, cg_ctx->gif_tail);
            }
        }
    }

#ifdef CG_IMAGE_TIME_LOGGING
    user->rhi_upload_end = sb_read_nanosecond_clock();

    CG_IMAGE_TIME_SPAN_END(user->url);
    gpu_fetch_decode_upload_timing_printout(user);
#endif

    cg_free(&cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
    cg_free(&cg_ctx->cg_heap_low, user, MALLOC_TAG);
}

#if defined(_VADER) || defined(_LEIA)
void cg_image_expand4(image_t * const src, const mem_region_t dst) {
    if ((src->bpp == 4) || (dst.ptr == NULL)) {
        return;
    }

    const int pixel_count = src->data_len / src->bpp;

    uint8_t * dst_pixels = dst.byte_ptr + (pixel_count * 4) - 4;
    const uint8_t * src_pixels = (uint8_t *)src->data + (pixel_count * src->bpp) - src->bpp;
    if (src->bpp == 3) {
        for (int ind = 0; ind < pixel_count; ++ind) {
            // convert rgb to rgb alpha
            // cheat hack that actually saves us 30us since we're going to overwrite the alpha value anyway
            // we can load the rgb portion in a single operation and overwrite the alpha portion
            // timings are on a lenovo p2 on windows.
            // timings copying over each pixel: ~46us
            // timings copying over all, and overwriting the last: ~25us

            *(int32_t *)dst_pixels = *(int32_t *)src_pixels;
            dst_pixels[3] = 255;

            dst_pixels -= 4;
            src_pixels -= 3;
        }
    } else if (src->bpp == 2) {
        for (int ind = 0; ind < pixel_count; ++ind) {
            // convert red + alpha into rgb + alpha
            dst_pixels[0] = src_pixels[0];
            dst_pixels[1] = src_pixels[0];
            dst_pixels[2] = src_pixels[0];

            dst_pixels[3] = src_pixels[1];

            dst_pixels -= 4;
            src_pixels -= 2;
        }
    } else {
        for (int ind = 0; ind < pixel_count; ++ind) {
            // convert red to rgb + specified alpha value
            dst_pixels[0] = src_pixels[0];
            dst_pixels[1] = src_pixels[0];
            dst_pixels[2] = src_pixels[0];
            dst_pixels[3] = 255;

            dst_pixels -= 4;
            src_pixels -= 1;
        }
    }
    src->bpp = 4;
    src->pitch = src->width * 4;
    src->spitch = src->pitch * src->height;
}
#endif

bool conditional_oom_free_image_allocs(image_load_data_t * const user, const cg_allocation_t test_allocation, const cg_allocation_t working_buffer) {
    if (test_allocation.region.ptr == NULL) {
        if (user->image_bytes.region.ptr) {
            cg_free_alloc(user->image_bytes, MALLOC_TAG);
            ZEROMEM(&user->image_bytes);
        }
        if (user->cg_image->pixel_buffer.region.ptr) {
            cg_free_alloc(user->cg_image->pixel_buffer, MALLOC_TAG);
            ZEROMEM(&user->cg_image->pixel_buffer);
        }
        if (working_buffer.region.ptr) {
            cg_free_alloc(working_buffer, MALLOC_TAG);
        }
        user->hit_oom = true;
        return true;
    }
    return false;
}

static void image_decode_job(void * void_user, thread_pool_t * const pool) {
    image_load_data_t * const user = void_user;

#ifdef CG_IMAGE_TIME_LOGGING
    user->decode_delay_end = sb_read_nanosecond_clock();
    CG_IMAGE_TIME_SPAN_END(user->url);
#endif

    if (user->cg_image->status != cg_image_async_load_pending) {
        return;
    }

    if (user->image_load_type == image_load_type_file) {
#ifdef CG_IMAGE_TIME_LOGGING
        CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[fetch] %s", user->url);
        user->request_start = sb_read_nanosecond_clock();
#endif

        user->file = adk_fopen(sb_app_root_directory, user->url, "rb");

        // skip deleting the artifact's name (its path) so we can log what failed to load later in the rhi upload job
        if (!user->file) {
            return;
        }

        const sb_stat_result_t stat_result = adk_stat(sb_app_root_directory, user->url);
        if (stat_result.error != sb_stat_success) {
            adk_fclose(user->file);
            user->file = NULL;
            return;
        }

        user->image_bytes = cg_unchecked_alloc(user->resource_heap, stat_result.stat.size, MALLOC_TAG);
        if (user->image_bytes.region.ptr == NULL) {
            adk_fclose(user->file);
            user->hit_oom = true;
            return;
        }
        adk_fread(user->image_bytes.region.ptr, user->image_bytes.region.size, 1, user->file);
        adk_fclose(user->file);

#ifdef CG_IMAGE_TIME_LOGGING
        user->request_end = sb_read_nanosecond_clock();
        CG_IMAGE_TIME_SPAN_END(user->url);
#endif
    }
#ifdef CG_IMAGE_TIME_LOGGING
    CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[decode] %s", user->url);
    user->decode_start = sb_read_nanosecond_clock();
#endif

    // If gzipped, inflate and replace the image bytes
    cg_allocation_t gzip_allocation;
    const inflate_gzip_from_memory_errors_e gzip_status = inflate_gzip_from_memory(user->image_bytes.cg_heap, user->cg_image->cg_ctx, user->image_bytes.region, &gzip_allocation);
    if (gzip_status == inflate_gzip_from_memory_success) {
        // free compressed image data, point to the unzipped data
        cg_free_alloc(user->image_bytes, MALLOC_TAG);
        user->image_bytes = gzip_allocation;
    } else if (gzip_status == inflate_gzip_from_memory_out_of_memory) {
        // this will always be triggered at this point, but it's most similar to other calls..
        if (conditional_oom_free_image_allocs(user, gzip_allocation, (cg_allocation_t){0})) {
            return;
        }
    }

    size_t pixel_buffer_size, working_space_size;

    if (imagelib_read_png_header_from_memory(user->image_bytes.consted.region, &user->cg_image->image, &pixel_buffer_size, &working_space_size)) {
#if defined(_VADER) || defined(_LEIA)
        pixel_buffer_size = (pixel_buffer_size / user->cg_image->image.bpp) * 4;
#endif
        user->working_buffer_size = working_space_size;

        user->cg_image->pixel_buffer = cg_unchecked_alloc(user->resource_heap, pixel_buffer_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, user->cg_image->pixel_buffer, (cg_allocation_t){0})) {
            return;
        }

        const cg_allocation_t working_buff = cg_unchecked_alloc(user->resource_heap, working_space_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, working_buff, working_buff)) {
            return;
        }

        imagelib_load_png_from_memory(user->image_bytes.consted.region, &user->cg_image->image, user->cg_image->pixel_buffer.region, working_buff.region);

        cg_free_alloc(working_buff, MALLOC_TAG);
        // pixel data async freed once uploaded to rhi however we can free our image file's bytes.
        cg_free_alloc(user->image_bytes, MALLOC_TAG);
        user->image_bytes.region.ptr = NULL;

    } else if (imagelib_read_gif_header_from_memory(user->image_bytes.consted.region, &user->cg_image->image, &pixel_buffer_size, &working_space_size)) {
        // we're dealing with a gif, so let the rhi_upload know (so it can keep our data around for us).
        user->image_type = cg_image_type_gif;
        user->working_buffer_size = working_space_size;

        // allocate the gif context, as we'll need to keep it around now.
        user->cg_image->gif = cg_alloc(&user->cg_image->cg_ctx->cg_heap_low, sizeof(cg_image_gif_t), MALLOC_TAG);
        ZEROMEM(user->cg_image->gif);
        cg_async_image_t * const cg_async_image = &user->cg_image->gif->async_image_data;
        cg_async_image->cg = user->cg_image->cg_ctx;

        cg_heap_t * gif_heap = NULL;
        if (user->memory_region == cg_memory_region_high_to_low) {
            gif_heap = &user->cg_image->cg_ctx->cg_heap_low;
            const cg_allocation_t alloc = cg_unchecked_realloc(gif_heap, user->image_bytes, user->image_bytes.region.size, MALLOC_TAG);
            if (conditional_oom_free_image_allocs(user, alloc, cg_async_image->working_buffer)) {
                return;
            }
            user->image_bytes = alloc;
            cg_async_image->resident_bytes = alloc.consted;
        } else {
            gif_heap = user->resource_heap;
            cg_async_image->resident_bytes = user->image_bytes.consted;
        }
        ZEROMEM(&user->image_bytes.region);

        user->cg_image->pixel_buffer = cg_unchecked_alloc(gif_heap, pixel_buffer_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, user->cg_image->pixel_buffer, cg_async_image->working_buffer)) {
            return;
        }

        cg_async_image->working_buffer = cg_unchecked_alloc(gif_heap, working_space_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, cg_async_image->working_buffer, cg_async_image->working_buffer)) {
            return;
        }

        imagelib_gif_load_first_frame_from_memory(cg_async_image->resident_bytes.region, &user->cg_image->image, &user->cg_image->gif->next_frame_duration, user->cg_image->pixel_buffer.region, cg_async_image->working_buffer.region);

        user->cg_image->gif->frame_remaining_duration_in_ms = (int32_t)user->cg_image->gif->next_frame_duration.ms;

    } else if (imagelib_read_bif_header_from_memory(user->image_bytes.consted.region, &user->cg_image->image, &user->cg_image->num_frames, &pixel_buffer_size, &working_space_size)) {
#if defined(_VADER) || defined(_LEIA)
        pixel_buffer_size = (pixel_buffer_size / user->cg_image->image.bpp) * 4;
#endif
        user->image_type = cg_image_type_bif;
        user->working_buffer_size = working_space_size;

        user->cg_image->bif = cg_alloc(&user->cg_image->cg_ctx->cg_heap_low, sizeof(cg_image_bif_t), MALLOC_TAG);
        ZEROMEM(user->cg_image->bif);

        cg_async_image_t * const cg_async_image = &user->cg_image->bif->async_image_data;

        cg_async_image->cg = user->cg_image->cg_ctx;

        cg_heap_t * bif_heap = NULL;
        if (user->memory_region == cg_memory_region_high_to_low) {
            bif_heap = &user->cg_image->cg_ctx->cg_heap_low;
            const cg_allocation_t alloc = cg_unchecked_realloc(bif_heap, user->image_bytes, user->image_bytes.region.size, MALLOC_TAG);
            if (conditional_oom_free_image_allocs(user, alloc, cg_async_image->working_buffer)) {
                return;
            }
            user->image_bytes = alloc;
            cg_async_image->resident_bytes = alloc.consted;
        } else {
            bif_heap = user->resource_heap;
            cg_async_image->resident_bytes = user->image_bytes.consted;
        }
        ZEROMEM(&user->image_bytes.region);

        user->cg_image->pixel_buffer = cg_unchecked_alloc(bif_heap, pixel_buffer_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, user->cg_image->pixel_buffer, cg_async_image->working_buffer)) {
            return;
        }

        cg_async_image->working_buffer = cg_unchecked_alloc(bif_heap, working_space_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, cg_async_image->working_buffer, cg_async_image->working_buffer)) {
            return;
        }

        imagelib_load_bif_jpg_frame_from_memory(cg_async_image->resident_bytes.region, &user->cg_image->image, 0, user->cg_image->pixel_buffer.region, cg_async_image->working_buffer.region);

    } else if (parse_gpu_ready_image_format(user)) {
        // nothing to do here until we get to rhi_upload_job
    } else if (imagelib_read_jpg_header_from_memory(user->image_bytes.consted.region, &user->cg_image->image, &pixel_buffer_size, &working_space_size)) {
#if defined(_VADER) || defined(_LEIA)
        pixel_buffer_size = (pixel_buffer_size / user->cg_image->image.bpp) * 4;
#endif
        user->working_buffer_size = working_space_size;

        user->cg_image->pixel_buffer = cg_unchecked_alloc(user->resource_heap, pixel_buffer_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, user->cg_image->pixel_buffer, (cg_allocation_t){0})) {
            return;
        }

        const cg_allocation_t working_buff = cg_unchecked_alloc(user->resource_heap, working_space_size, MALLOC_TAG);
        if (conditional_oom_free_image_allocs(user, working_buff, working_buff)) {
            return;
        }

        imagelib_load_jpg_from_memory(user->image_bytes.consted.region, &user->cg_image->image, user->cg_image->pixel_buffer.region, working_buff.region);

        cg_free_alloc(working_buff, MALLOC_TAG);
        // pixel data async freed once uploaded to rhi however we can free our image file's bytes.
        cg_free_alloc(user->image_bytes, MALLOC_TAG);
        user->image_bytes.region.ptr = NULL;

    } else {
        // unsupported image format fall through/skip expanding on vader targets.
        // user->cg_image->image.data is nullptr at this point.
        // this will be picked up in the upload function, indicating that the image could not be decoded.
#ifdef CG_IMAGE_TIME_LOGGING
        user->decode_end = sb_read_nanosecond_clock();
        CG_IMAGE_TIME_SPAN_END(user->url);
        CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[rhi-upload-delay] %s", user->url);
        user->rhi_delay_start = sb_read_nanosecond_clock();
#endif
        return;
    }

#if defined(_VADER) || defined(_LEIA)
    cg_image_expand4(&user->cg_image->image, user->cg_image->pixel_buffer.region);
#endif

#ifdef CG_IMAGE_TIME_LOGGING
    user->decode_end = sb_read_nanosecond_clock();
    CG_IMAGE_TIME_SPAN_END(user->url);
    CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[rhi-upload-delay] %s", user->url);
    user->rhi_delay_start = sb_read_nanosecond_clock();
#endif
}

/*
The value of the "X-BAMTECH-ERROR" HTTP header (defined at
https://wiki.disneystreaming.com/display/SSDE/Ripcut+Custom+Error+Messages)
is a JSON encoded string of the form {"code": <ERROR CODE>, "message":
<HUMAN READABLE STRING>}.  The X-BAMTECH-ERROR code is currently defined as a
4-digit, positive number. The code parser handles signed numbers of
any character length for forward compatability.
This function will return -1 if there is no X-BAMTECH-ERROR header detected and
0 if it can't parse the X-BAMTECH-ERROR header.
*/
static int32_t check_for_ripcut_error(mem_region_t header) {
    static const char http_header_key_bamtech_error[] = "X-BAMTECH-ERROR:";
    const char * const ripcut_header = strstr((const char *)header.byte_ptr, http_header_key_bamtech_error);
    if (ripcut_header == NULL) { // no bamtech error header
        return -1;
    }
    static const char code_json_key[] = "\"code\":";
    const char * const code_value_str = strstr(ripcut_header, code_json_key);
    const char * const ripcut_header_end = strstr(ripcut_header, "\r\n");
    if ((code_value_str == NULL) || // no code key in header
        (ripcut_header_end == NULL) || // header is not complete
        (ripcut_header_end < code_value_str)) { // the code key is actually in a following header
        return 0;
    }
    return atoi(code_value_str + ARRAY_SIZE(code_json_key) - 1);
}

static void cg_image_load_user_http_failure_cleanup(void * const void_user) {
    image_load_data_t * const user = void_user;
    cg_context_t * const cg_ctx = user->cg_image->cg_ctx;
    adk_curl_close_handle(user->curl_handle);
    if (user->image_bytes.region.ptr) {
        cg_free_alloc(user->image_bytes, MALLOC_TAG);
    }
    if (user->header_bytes.region.ptr) {
        cg_free_alloc(user->header_bytes, MALLOC_TAG);
    }
    cg_free(&cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
    cg_free(&cg_ctx->cg_heap_low, user, MALLOC_TAG);
}

static void url_image_http_fetch_complete(adk_curl_handle_t * const handle, const adk_curl_result_e result, const struct adk_curl_callbacks_t * const callbacks) {
    ASSERT_IS_MAIN_THREAD();

    image_load_data_t * const user = callbacks->user[0];
    user->cg_image->load_user = NULL;
#ifdef CG_IMAGE_TIME_LOGGING
    user->request_end = sb_read_nanosecond_clock();
    CG_IMAGE_TIME_SPAN_END(user->url);
#endif

    if (user->header_bytes.region.ptr) {
        cg_free_alloc(user->header_bytes, MALLOC_TAG);
    }
    long http_status_code = 0;
    adk_curl_get_info_long(handle, adk_curl_info_response_code, &http_status_code);
    long expected_http_codes[] = {
        200, // ok
        301, // moved permanently
        302, // found
        303, // see other
        307, // temporary redirect
        308 // permanent redirect
    };
    bool found_expected_http_status = false;
    for (size_t i = 0; i < ARRAY_SIZE(expected_http_codes); ++i) {
        if (expected_http_codes[i] == http_status_code) {
            found_expected_http_status = true;
            break;
        }
    }
    if ((result == adk_curl_result_ok) && (user->cg_image->status == cg_image_async_load_pending) && found_expected_http_status) {
        // check if we can upload via a fast-path

        if (parse_gpu_ready_image_format(user)) {
#ifdef CG_IMAGE_TIME_LOGGING
            CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[GPU_ready-upload] %s", user->url);
#endif
            if (upload_gpu_ready_image_format(user)) {
                user->cg_image->status = cg_image_async_load_complete;
            } else {
                // gpu format unsupported on this platform, cleanup
                user->cg_image->status = cg_image_async_load_unrecognized_image_format;
                cg_free_alloc(user->image_bytes, MALLOC_TAG);
            }
#ifdef CG_IMAGE_TIME_LOGGING
            CG_IMAGE_TIME_SPAN_END(user->url);
            gpu_fetch_decode_upload_timing_printout(user);
#endif
            cg_free(&user->cg_image->cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
            cg_free(&user->cg_image->cg_ctx->cg_heap_low, user, MALLOC_TAG);

        } else {
            // it's not a gpu-ready image format then do the slow decode pipeline
#ifdef CG_IMAGE_TIME_LOGGING
            CG_IMAGE_TIME_SPAN_BEGIN(user->url, "[decode-delay] %s", user->url);
            user->decode_delay_start = sb_read_nanosecond_clock();
#endif
            thread_pool_enqueue(user->cg_image->cg_ctx->thread_pool, image_decode_job, rhi_upload_job_main_thread, user);
        }

    } else {
        cg_context_t * const cg_ctx = user->cg_image->cg_ctx;

        if (user->cg_image->status == cg_image_async_load_ripcut_error) {
            LOG_WARN(TAG_CG_IMG, "Ripcut returned error [%i] while trying to fetch an image at [%s]", user->cg_image->ripcut_error_code, user->url);
        } else if (result != adk_curl_result_ok) {
            if (user->hit_oom) {
                LOG_WARN(TAG_CG_IMG, "Ran out of memory when attempting to download image at [%s]", user->url);
                user->cg_image->status = cg_image_async_load_out_of_memory;
            } else {
                LOG_WARN(TAG_CG_IMG, "Encountered an error while trying to fetch an image at [%s] adk_curl error code: [%i]", user->url, result);
                user->cg_image->status = cg_image_async_load_http_fetch_error;
            }
        } else {
            LOG_WARN(TAG_CG_IMG, "Encountered an error while trying to fetch an image at [%s] http status: [%i]", user->url, http_status_code);
            user->cg_image->status = cg_image_async_load_http_fetch_error;
        }

        if (user->image_bytes.region.ptr) {
            cg_free_alloc(user->image_bytes, MALLOC_TAG);
        }
        cg_free(&cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
        cg_free(&cg_ctx->cg_heap_low, user, MALLOC_TAG);
    }

    adk_curl_close_handle(handle);
}

static bool url_image_http_receive(adk_curl_handle_t * const handle, const const_mem_region_t bytes, const struct adk_curl_callbacks_t * const callbacks) {
    ASSERT_IS_MAIN_THREAD();
    image_load_data_t * const user = callbacks->user[0];
    ASSERT(!user->hit_oom);
    const cg_allocation_t allocation = cg_unchecked_realloc(user->resource_heap, user->image_bytes, user->image_bytes.region.size + bytes.size, MALLOC_TAG);
    if (!allocation.region.ptr) {
        if (user->image_bytes.region.ptr) {
            cg_free_alloc(user->image_bytes, MALLOC_TAG);
        }
        user->image_bytes.region = (mem_region_t){0};
        user->hit_oom = true;
        return false;
    }
    memcpy(allocation.region.byte_ptr + user->image_bytes.region.size, bytes.ptr, bytes.size);
    user->image_bytes = allocation;
    return true;
}

static bool url_image_http_header_receive(adk_curl_handle_t * const handle, const const_mem_region_t bytes, const struct adk_curl_callbacks_t * const callbacks) {
    ASSERT_IS_MAIN_THREAD();
    image_load_data_t * const user = callbacks->user[0];
    ASSERT(!user->hit_oom);
    // if this is a new allocation then add an additional space for nul, otherwise the space for nul is included..
    const size_t new_size = user->header_bytes.region.size == 0 ? (bytes.size + 1) : (user->header_bytes.region.size + bytes.size);
    const cg_allocation_t allocation = cg_unchecked_realloc(user->resource_heap, user->header_bytes, new_size, MALLOC_TAG);
    if (!allocation.region.ptr) {
        if (user->header_bytes.region.ptr) {
            cg_free_alloc(user->header_bytes, MALLOC_TAG);
        }
        user->header_bytes.region = (mem_region_t){0};
        user->hit_oom = true;
        return false;
    }

    // if we have prior bytes, we should overwrite the nul with our new bytes, then insert the nul at the end of the region.
    const size_t memcpy_offset = user->header_bytes.region.size != 0 ? user->header_bytes.region.size - 1 : 0;
    memcpy(allocation.region.byte_ptr + memcpy_offset, bytes.ptr, bytes.size);
    user->header_bytes = allocation;
    user->header_bytes.region.byte_ptr[user->header_bytes.region.size - 1] = 0;

    if (user->cg_image->ripcut_error_code <= 0) {
        user->cg_image->ripcut_error_code = check_for_ripcut_error(user->header_bytes.region);
        if (user->cg_image->ripcut_error_code >= 0) {
            user->cg_image->status = cg_image_async_load_ripcut_error;
            return false;
        }
    }
    return true;
}

cg_image_t * cg_context_load_image_async(const char * const file_location, const cg_memory_region_e memory_region, const cg_image_load_opts_e image_load_opts, const char * const tag) {
    // file loading steps are as follows:
    // 1. (main thread) receive a request, and build the appropriate domain and id
    //    copy the `file_location` to filename to extend the lifetime sufficiently
    // 2. (thread pool) decode the image and delay error handling until upload
    // 3. (main thread) upload the image to RHI

    // url loading steps are as follows:
    // 1. (main thread) we get a request for fetching an image
    // 2. (main thread) we enqueue a GET operation to the http library
    // 3. (main thread) we read the http body as its received and buffer it internally
    //    if image loading is aborted we cancel out and free the current state and indicate to the http library to abort the request
    // 4. (main thread) on completion of the GET we enqueue a decode job and an upload job (via a completion handler) to the thread pool
    // 5. (thread pool) the decode job is run
    //    if the request to process the image is aborted before the decode starts then we abort processing the image
    //    if an error is encountered during image processing we defer checking until upload.
    // 6. (main thread) the image is uploaded to RHI on the main thread
    //    if there was a decoding error we update the status and skip the rest of the upload process
    //    if the image was requested for abort (the last chance to async abort the request) we skip uploading to RHI and free the image

    cg_context_t * const ctx = cg_statics.ctx;
    image_load_data_t * const image_load_data = cg_alloc(&ctx->cg_heap_low, sizeof(image_load_data_t), tag);
    ZEROMEM(image_load_data);

    image_load_data->resource_heap = memory_region != cg_memory_region_low ? &ctx->cg_heap_high : &ctx->cg_heap_low;

    image_load_data->cg_image = cg_alloc(&ctx->cg_heap_low, sizeof(cg_image_t), MALLOC_TAG);
    ZEROMEM(image_load_data->cg_image);

    image_load_data->memory_region = memory_region;
    image_load_data->cg_image->cg_ctx = ctx;
    image_load_data->cg_image->num_frames = 1;
    image_load_data->cg_image->status = cg_image_async_load_pending;

    {
        // keep URL for error reporting
        const size_t name_length = strlen(file_location) + 1;
        image_load_data->url = cg_alloc(&ctx->cg_heap_low, name_length, MALLOC_TAG);
        memcpy((void *)image_load_data->url, file_location, name_length);
    }

    if (strstr(file_location, "://") != NULL) {
        image_load_data->image_load_type = image_load_type_url;

        adk_curl_handle_t * const handle = adk_curl_open_handle();
        adk_curl_set_opt_ptr(handle, adk_curl_opt_url, (void *)file_location);
        adk_curl_set_opt_long(handle, adk_curl_opt_follow_location, 1);
        if (image_load_opts & cg_image_load_opts_http_verbose) {
            adk_curl_set_opt_long(handle, adk_curl_opt_verbose, 1);
        }

        const adk_curl_callbacks_t callbacks = {
            .on_http_header_recv = url_image_http_header_receive,
            .on_http_recv = url_image_http_receive,
            .on_complete = url_image_http_fetch_complete,
            .user = {image_load_data}};
        image_load_data->cg_image->load_user = image_load_data;
        image_load_data->curl_handle = handle;
#ifdef CG_IMAGE_TIME_LOGGING
        CG_IMAGE_TIME_SPAN_BEGIN(image_load_data->url, "[fetch] %s", image_load_data->url);
        image_load_data->request_start = sb_read_nanosecond_clock();
#endif
        adk_curl_async_perform(handle, callbacks);
    } else {
        image_load_data->image_load_type = image_load_type_file;
#ifdef CG_IMAGE_TIME_LOGGING
        CG_IMAGE_TIME_SPAN_BEGIN(image_load_data->url, "[decode-delay] %s", image_load_data->url);
        image_load_data->decode_delay_start = sb_read_nanosecond_clock();
#endif
        thread_pool_enqueue(ctx->thread_pool, image_decode_job, rhi_upload_job_main_thread, image_load_data);
    }

    return image_load_data->cg_image;
}

cg_image_async_load_status_e cg_get_image_load_status(const cg_image_t * const image) {
    return image->status;
}

cg_rect_t cg_context_image_rect(const cg_image_t * const image) {
    // http://www.dii.uchile.cl/~daespino/files/Iso_C_1999_definition.pdf#page=102
    // c99 spec: 6.5.15 dictates that only the second or third operand of a conditional operator will be run.
    // msvc 2017/2019 (toolchains v141, v142?) will unconditionally run both branchs of a conditional operator in debug
    // gcc/clang actually perform a cmp + jmp first.
    // https://godbolt.org/z/79Yxoo
    // since we will attempt to deref a nullptr if the condition is false, we can't rely on a ?: here for msvc + debug

    if (image->status == cg_image_async_load_complete) {
        return (cg_rect_t){.x = 0.f, .y = 0.f, .width = (float)image->cg_texture.texture->width, .height = (float)image->cg_texture.texture->height};
    } else {
        return (cg_rect_t){.x = 0.f, .y = 0.f, .width = 1.f, .height = 1.f};
    }
}

void cg_context_image_set_repeat(cg_image_t * const image, const bool repeat_x, const bool repeat_y) {
    image->cg_texture.sampler_state.u_wrap_mode = repeat_x ? rhi_wrap_mode_wrap : rhi_wrap_mode_clamp_to_edge;
    image->cg_texture.sampler_state.v_wrap_mode = repeat_y ? rhi_wrap_mode_wrap : rhi_wrap_mode_clamp_to_edge;
}

void cg_context_set_image_animation_state(cg_image_t * const image, const cg_image_animation_state_e image_animation_state) {
    image->image_animation_state = image_animation_state;
}

int32_t cg_get_image_ripcut_error_code(const cg_image_t * const image) {
    return image->ripcut_error_code;
}
