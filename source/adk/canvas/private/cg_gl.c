/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
cg_gl.c

canvas rendering backend
*/

#include "source/adk/canvas/cg.h"
#include "source/adk/log/log.h"
#include "source/shaders/compiled/canvas.frag.gen.h"
#include "source/shaders/compiled/canvas.vert.gen.h"
#include "source/shaders/compiled/canvas_alpha_mask.frag.gen.h"
#include "source/shaders/compiled/canvas_alpha_test.frag.gen.h"
#include "source/shaders/compiled/canvas_alpha_test_rgb_fill_alpha_red.frag.gen.h"
#include "source/shaders/compiled/canvas_rgb_fill_alpha_red.frag.gen.h"
#include "source/shaders/compiled/canvas_video.frag.gen.h"

#ifdef _CG_GL_TRACE
#include "source/adk/telemetry/telemetry.h"
#define CG_GL_TRACE_PUSH_FN() TRACE_PUSH_FN()
#define CG_GL_TRACE_PUSH(_name) TRACE_PUSH(_name)
#define CG_GL_TRACE_POP() TRACE_POP()
#else
#define CG_GL_TRACE_PUSH_FN()
#define CG_GL_TRACE_PUSH(_name)
#define CG_GL_TRACE_POP()
#endif

#ifdef _CANVAS_EXPERIMENTAL
#include "source/adk/experimental/canvas/cg_experimental.h"
#endif

#define TAG_CG_GL FOURCC('C', 'G', 'G', 'L')

/* ===========================================================================
 * Canvas RHI interface
 * ==========================================================================*/

static void cg_gl_upload_uniform(cg_gl_state_t * const state, const rhi_program_uniform_e name, const void * data, const int size, const char * const tag) {
    CG_GL_TRACE_PUSH_FN();

    ASSERT(name >= 0);
    ASSERT(name < rhi_program_num_uniforms);
    ASSERT_ALIGNED_16(size);

    r_uniform_buffer_t * ub = state->uniforms[name];
    if (!ub) {
        // create the uniform buffer and set its binding point

        ub = render_create_uniform_buffer(state->render_device, (rhi_uniform_buffer_desc_t){.size = size, .usage = rhi_usage_dynamic}, data, tag);

        state->uniforms[name] = ub;

        RENDER_ENSURE_WRITE_CMD_STREAM(
            &state->render_device->default_cmd_stream,
            render_cmd_buf_write_set_uniform_binding,
            (rhi_uniform_binding_indirect_t){
                .buffer = &ub->uniform_buffer,
                .index = name},
            tag);
    }

    render_cmd_stream_upload_uniform_buffer_data(
        &state->render_device->default_cmd_stream,
        ub,
        CONST_MEM_REGION(
                .ptr = data,
                .size = size),
        0,
        tag);

    CG_GL_TRACE_POP();
}

static void cg_gl_set_uniform_float(cg_gl_state_t * const state, const rhi_program_uniform_e name, const float f, const char * const tag) {
    CG_GL_TRACE_PUSH_FN();
    const float paddto4floats[4] = {f, 0, 0, 0};
    cg_gl_upload_uniform(state, name, paddto4floats, sizeof(paddto4floats), tag);
    CG_GL_TRACE_POP();
}

static void cg_gl_set_uniform_vec2(cg_gl_state_t * const state, const rhi_program_uniform_e name, const float * const v, const char * const tag) {
    CG_GL_TRACE_PUSH_FN();
    const float paddto4floats[4] = {v[0], v[1], 0, 0};
    cg_gl_upload_uniform(state, name, paddto4floats, sizeof(paddto4floats), tag);
    CG_GL_TRACE_POP();
}

static void cg_gl_set_uniform_vec4(cg_gl_state_t * const state, const rhi_program_uniform_e name, const float * const v, const char * const tag) {
    CG_GL_TRACE_PUSH_FN();
    cg_gl_upload_uniform(state, name, v, sizeof(float) * 4, tag);
    CG_GL_TRACE_POP();
}

void cg_gl_set_uniform_mat4(cg_gl_state_t * const state, const rhi_program_uniform_e name, const float * const m, const char * const tag) {
    CG_GL_TRACE_PUSH_FN();
    cg_gl_upload_uniform(state, name, m, sizeof(float) * 16, tag);
    CG_GL_TRACE_POP();
}

static void cg_gl_set_uniform_color(cg_gl_state_t * const state, const rhi_program_uniform_e name, const cg_color_t * const color, const char * const tag) {
    CG_GL_TRACE_PUSH_FN();
    STATIC_ASSERT(sizeof(cg_color_t) == sizeof(float) * 4);
    cg_gl_upload_uniform(state, name, color, sizeof(cg_color_t), tag);
    CG_GL_TRACE_POP();
}

/* ------------------------------------------------------------------------- */

static rhi_pixel_format_e get_rhi_pixel_format(const int channels, const image_encoding_e encoding) {
    ASSERT((encoding != image_encoding_uncompressed) || ((channels >= 1) && (channels <= 4)));

    static const rhi_pixel_format_e channels_to_rhi_format[] = {
        [1] = rhi_pixel_format_r8_unorm,
        [2] = rhi_pixel_format_ra8_unorm,
        [3] = rhi_pixel_format_rgb8_unorm,
        [4] = rhi_pixel_format_rgba8_unorm};

    switch (encoding) {
        case image_encoding_uncompressed:
            return channels_to_rhi_format[channels];
#if defined(_RPI) || defined(_STB_NATIVE)
        case image_encoding_etc1:
            return rhi_pixel_format_etc1;
#endif
#if defined(_VADER) || defined(_LEIA)
        case image_encoding_gnf1:
            return rhi_pixel_format_gnf;
#endif
        default:
            break;
    }

    return -1;
}

bool cg_gl_texture_from_memory(
    render_device_t * const render_device,
    cg_gl_texture_t * const tex,
    const rhi_sampler_state_desc_t sampler_state,
    const image_t image,
    const cg_gl_texture_usage usage,
    void (*free_mem_callback_called_from_render_thread)(rhi_device_t * device, void * arg),
    void * arg) {
    CG_GL_TRACE_PUSH_FN();

    ASSERT((image.encoding != image_encoding_uncompressed) || ((image.bpp >= 1) && (image.bpp <= 4)));

    ZEROMEM(tex);
    tex->sampler_state = sampler_state;

    image_mips_t mipmaps;
    ZEROMEM(&mipmaps);
    mipmaps.num_levels = 1;
    mipmaps.levels[0] = image;

    const rhi_usage_e rhi_usage = (usage == cg_gl_dynamic_texture_usage) ? rhi_usage_dynamic : rhi_usage_default;
    const rhi_pixel_format_e pixel_format = get_rhi_pixel_format(image.bpp, image.encoding);
    if (pixel_format < 0) {
        CG_GL_TRACE_POP();
        // invalid pixel format
        return false;
    }

    if (!image.data) {
        // create texture without data, reserves gpu memory
        tex->texture = render_create_texture_2d(
            render_device,
            mipmaps,
            pixel_format,
            rhi_usage,
            sampler_state,
            MALLOC_TAG);
    } else if (free_mem_callback_called_from_render_thread) {
        // simple case just create the texture
        // and queue a free-callback
        tex->texture = render_create_texture_2d(render_device, mipmaps, pixel_format, rhi_usage, tex->sampler_state, MALLOC_TAG);
        RENDER_ENSURE_WRITE_CMD_STREAM(
            &render_device->default_cmd_stream,
            render_cmd_buf_write_callback,
            free_mem_callback_called_from_render_thread,
            arg,
            MALLOC_TAG);
    } else {
        // this is a little complicated since i want to push the texture-data inline
        // this can only be done for tiny images
        render_cmd_stream_blocking_latch_cmd_buf(&render_device->default_cmd_stream);
        // must be capable of fitting inline
        VERIFY(render_device->default_cmd_stream.buf->hlba.size > mipmaps.levels[0].data_len + 1024);
        render_mark_cmd_buf(render_device->default_cmd_stream.buf);

        mipmaps.levels[0].data = render_cmd_buf_unchecked_alloc(render_device->default_cmd_stream.buf, 8, image.data_len);
        tex->texture = !mipmaps.levels[0].data ? NULL : render_create_texture_2d_no_flush(render_device, mipmaps, pixel_format, rhi_usage, sampler_state, MALLOC_TAG);

        if (!(mipmaps.levels[0].data && tex->texture)) {
            render_unwind_cmd_buf(render_device->default_cmd_stream.buf);
            render_flush_cmd_stream(&render_device->default_cmd_stream, render_no_wait);
            render_cmd_stream_blocking_latch_cmd_buf(&render_device->default_cmd_stream);

            // must succeed
            mipmaps.levels[0].data = render_cmd_buf_alloc(render_device->default_cmd_stream.buf, 8, image.data_len);
            tex->texture = render_create_texture_2d_no_flush(render_device, mipmaps, pixel_format, rhi_usage, tex->sampler_state, MALLOC_TAG);
            VERIFY(tex->texture);
        }

        memcpy(mipmaps.levels[0].data, image.data, image.data_len);
    }

    CG_GL_TRACE_POP();
    return true;
}

void cg_gl_texture_init_with_color(cg_gl_state_t * const state, cg_gl_texture_t * const tex, const cg_color_packed_t * const color) {
    CG_GL_TRACE_PUSH_FN();

    const rhi_sampler_state_desc_t sampler_state = {
        .max_anisotropy = 1,
        .min_filter = rhi_min_filter_linear,
        .max_filter = rhi_max_filter_linear,
        .u_wrap_mode = rhi_wrap_mode_wrap,
        .v_wrap_mode = rhi_wrap_mode_wrap,
        .w_wrap_mode = rhi_wrap_mode_wrap,
        .border_color = {1, 1, 1, 1}};

    const image_t image = {
        .encoding = image_encoding_uncompressed,
        .width = 1,
        .height = 1,
        .depth = 1,
        .bpp = 4,
        .pitch = 1,
        .spitch = 4,
        .data_len = 4,
        .data = (void *)&color->rgba};

    VERIFY(cg_gl_texture_from_memory(state->render_device, tex, sampler_state, image, cg_gl_static_texture_usage, NULL, NULL));

    CG_GL_TRACE_POP();
}

void cg_gl_texture_bind_raw(cg_gl_state_t * const state, rhi_texture_t * const * const tex) {
    CG_GL_TRACE_PUSH_FN();

    rhi_texture_bindings_indirect_t bindings;
    bindings.num_textures = 1;
    bindings.textures[0] = tex;

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_texture_bindings_indirect,
        bindings,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_texture_bind_raw_two(cg_gl_state_t * const state, rhi_texture_t * const * const tex0, rhi_texture_t * const * const tex1) {
    CG_GL_TRACE_PUSH_FN();

    rhi_texture_bindings_indirect_t bindings;
    bindings.num_textures = 2;
    bindings.textures[0] = tex0;
    bindings.textures[1] = tex1;

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_texture_bindings_indirect,
        bindings,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_texture_bind(cg_gl_state_t * const state, const cg_gl_texture_t * const tex) {
    CG_GL_TRACE_PUSH_FN();

    cg_gl_texture_bind_raw(state, &tex->texture->texture);

    // update fence so we don't cg_free the data before this command is processed
    tex->texture->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

void cg_gl_texture_bind_two(cg_gl_state_t * const state, const cg_gl_texture_t * const tex0, const cg_gl_texture_t * const tex1) {
    CG_GL_TRACE_PUSH_FN();

    cg_gl_texture_bind_raw_two(state, &tex0->texture->texture, &tex1->texture->texture);

    // update fence so we don't cg_free the data before this command is processed
    tex0->texture->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);
    tex1->texture->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

void cg_gl_texture_update_sampler_state(cg_gl_state_t * const state, cg_gl_texture_t * const tex) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_texture_sampler_state_indirect,
        &tex->texture->texture,
        tex->sampler_state,
        MALLOC_TAG);

    // update fence so we don't cg_free the data before this command is processed
    tex->texture->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

/* ------------------------------------------------------------------------- */

void cg_gl_texture_free(cg_gl_state_t * const state, cg_gl_texture_t * const tex) {
    CG_GL_TRACE_PUSH_FN();
    if (tex->texture) {
        render_release(&tex->texture->resource, MALLOC_TAG);
        tex->texture = NULL;
    }
    CG_GL_TRACE_POP();
}

void cg_gl_texture_update(cg_gl_state_t * const state, cg_gl_texture_t * const tex, void * const pixels) {
    CG_GL_TRACE_PUSH_FN();

    const int data_len = tex->texture->width * tex->texture->height * tex->texture->channels;

    image_mips_t mipmaps;
    ZEROMEM(&mipmaps);

    mipmaps.num_levels = 1;
    mipmaps.levels[0].bpp = tex->texture->channels;
    mipmaps.levels[0].width = tex->texture->width;
    mipmaps.levels[0].height = tex->texture->height;
    mipmaps.levels[0].depth = 1;
    mipmaps.levels[0].pitch = tex->texture->width * tex->texture->channels;
    mipmaps.levels[0].spitch = data_len;
    mipmaps.levels[0].encoding = image_encoding_uncompressed;
    mipmaps.levels[0].data = pixels;
    mipmaps.levels[0].data_len = data_len;

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_upload_texture_indirect,
        &tex->texture->texture,
        mipmaps,
        MALLOC_TAG);

    // update fence so we don't cg_free the data before this command is processed
    tex->texture->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

/* ------------------------------------------------------------------------- */

void cg_gl_framebuffer_free(cg_gl_state_t * const state, cg_gl_framebuffer_t * const fb) {
    CG_GL_TRACE_PUSH_FN();
    if (fb->render_target) {
        render_release(&fb->render_target->resource, MALLOC_TAG);
        fb->render_target = NULL;
    }
    CG_GL_TRACE_POP();
}

void cg_gl_discard_framebuffer(cg_gl_state_t * const state, const cg_gl_framebuffer_t * const fb) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_discard_render_target_data_indirect,
        &fb->render_target->render_target,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_render_to_framebuffer(cg_gl_state_t * const state, const cg_gl_framebuffer_t * const fb) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_target_indirect,
        &fb->render_target->render_target,
        MALLOC_TAG);

    // TODO: fix viewport

    {
        const float viewport[] = {(float)fb->width, (float)fb->height};
        cg_gl_set_uniform_vec2(state, rhi_program_uniform_viewport, viewport, MALLOC_TAG);
    }
    CG_GL_TRACE_POP();
}

void cg_gl_render_to_screen(cg_gl_state_t * const state, const int32_t width, const int32_t height) {
    // TODO: fix viewport
    CG_GL_TRACE_PUSH_FN();
    {
        const float viewport[] = {(float)width, (float)height};
        cg_gl_set_uniform_vec2(state, rhi_program_uniform_viewport, viewport, MALLOC_TAG);
    }
    CG_GL_TRACE_POP();
}

void cg_gl_framebuffer_init(cg_gl_state_t * const state, cg_gl_framebuffer_t * const fb, const int32_t width, const int32_t height) {
    CG_GL_TRACE_PUSH_FN();

    ZEROMEM(fb);
    fb->width = width;
    fb->height = height;

    image_mips_t mipmaps;
    ZEROMEM(&mipmaps);
    mipmaps.num_levels = 1;
    mipmaps.levels[0].width = width;
    mipmaps.levels[0].height = height;
    mipmaps.levels[0].depth = 1;
    mipmaps.levels[0].encoding = image_encoding_uncompressed;
    mipmaps.levels[0].bpp = 4;

    // TODO: this introduces a stall
    // because when we release the color_buffer
    // they block until those buffers are attached...

    r_texture_t * const color_buffer = render_create_texture_2d(
        state->render_device,
        mipmaps,
        rhi_framebuffer_format,
        rhi_usage_rendertarget,
        (rhi_sampler_state_desc_t){
            .min_filter = rhi_min_filter_nearest,
            .max_filter = rhi_max_filter_nearest,
            .u_wrap_mode = rhi_wrap_mode_clamp_to_edge,
            .v_wrap_mode = rhi_wrap_mode_clamp_to_edge,
            .w_wrap_mode = rhi_wrap_mode_clamp_to_edge,
            .border_color = {1, 1, 1, 1},
            .max_anisotropy = 1},
        MALLOC_TAG);

    fb->render_target = render_create_render_target(state->render_device, MALLOC_TAG);

    // attach buffers to render target
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_target_color_buffer_indirect,
        &fb->render_target->render_target,
        (rhi_render_target_color_buffer_indirect_t){
            .buffer = &color_buffer->texture,
            .index = 0},
        MALLOC_TAG);

    fb->render_target->resource.fence = color_buffer->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    // release buffers, they are now attached to the render target
    render_release(&color_buffer->resource, MALLOC_TAG);

    CG_GL_TRACE_POP();
}

r_program_t * load_shader_program(
    cg_gl_state_t * const state,
    const const_mem_region_t vert_program,
    const const_mem_region_t frag_program) {
    CG_GL_TRACE_PUSH_FN();

    r_program_t * const program = render_create_program_from_binary(
        state->render_device,
        vert_program,
        frag_program,
        MALLOC_TAG);

    // TODO: this is unnecessary once the shader_compiler does validation
    // flush and wait for error
    render_flush_cmd_stream(&state->render_device->default_cmd_stream, render_wait);
    if (program->error) {
        LOG_ERROR(TAG_CG_GL, "shader program error: %s", rhi_get_error_message(program->error));
        rhi_error_release(program->error, MALLOC_TAG);
        TRAP("shader program error.");
    }

    CG_GL_TRACE_POP();
    return program;
}

void cg_gl_state_init(cg_gl_state_t * const state, cg_heap_t * const cg_heap, render_device_t * const render_device) {
    CG_GL_TRACE_PUSH_FN();

    ZEROMEM(state);
    state->render_device = render_device;
    state->cg_heap = cg_heap;
    state->shaders.color = load_shader_program(
        state,
        canvas_vert_program,
        canvas_frag_program);
    state->shaders.color_rgb_fill_alpha_red = load_shader_program(
        state,
        canvas_vert_program,
        canvas_rgb_fill_alpha_red_frag_program);
    state->shaders.color_alpha_mask = load_shader_program(
        state,
        canvas_vert_program,
        canvas_alpha_mask_frag_program);
    state->shaders.color_alpha_test = load_shader_program(
        state,
        canvas_vert_program,
        canvas_alpha_test_frag_program);
    state->shaders.color_alpha_test_rgb_fill_alpha_red = load_shader_program(
        state,
        canvas_vert_program,
        canvas_alpha_test_rgb_fill_alpha_red_frag_program);
    state->shaders.video = load_shader_program(
        state,
        canvas_vert_program,
        canvas_video_frag_program);

    cg_gl_texture_init_with_color(state, &state->white, cg_color_packed(1, 1, 1, 1));

    // make render states

    // rasterizer
    {
        rhi_rasterizer_state_desc_t scissor_off_desc = {.cull_mode = rhi_cull_face_none, .scissor_test = false};
        rhi_rasterizer_state_desc_t scissor_on_desc = {.cull_mode = rhi_cull_face_none, .scissor_test = true};

        state->rs_scissor_off = render_create_rasterizer_state(render_device, scissor_off_desc, MALLOC_TAG);
        state->rs_scissor_on = render_create_rasterizer_state(render_device, scissor_on_desc, MALLOC_TAG);

        state->scissor.rhi.enabled = false;
        state->scissor.canvas.enabled = false;
    }

    // blends
    {
        rhi_blend_state_desc_t desc;

        desc.blend_enable = false;
        desc.blend_color[0] = desc.blend_color[1] = desc.blend_color[2] = desc.blend_color[3] = 1.f;
        desc.blend_op = rhi_blend_op_add;
        desc.blend_op_alpha = rhi_blend_op_add;
        desc.color_write_mask = rhi_color_write_mask_none;
        desc.src_blend = rhi_blend_one;
        desc.dst_blend = rhi_blend_zero;
        desc.src_blend_alpha = rhi_blend_zero;
        desc.dst_blend_alpha = rhi_blend_zero;

        state->bs_blend_off_color_write_mask_none = render_create_blend_state(render_device, desc, MALLOC_TAG);

        desc.color_write_mask = rhi_color_write_mask_rgb;

        state->bs_blend_off_color_write_mask_rgb = render_create_blend_state(render_device, desc, MALLOC_TAG);

        desc.blend_enable = true;
        desc.src_blend = rhi_blend_src_alpha;
        desc.dst_blend = rhi_blend_inv_src_alpha;
        desc.src_blend_alpha = rhi_blend_src_alpha;
        desc.dst_blend_alpha = rhi_blend_inv_src_alpha;

        state->bs_blend_alpha_color_write_mask_rgb = render_create_blend_state(render_device, desc, MALLOC_TAG);

        desc.color_write_mask = rhi_color_write_mask_all;

        state->bs_blend_alpha_color_write_mask_all = render_create_blend_state(render_device, desc, MALLOC_TAG);

        // Overwrite the contents of dst with src
        desc.src_blend = rhi_blend_one;
        desc.dst_blend = rhi_blend_zero;
        desc.src_blend_alpha = rhi_blend_one;
        desc.dst_blend_alpha = rhi_blend_zero;

        state->bs_blend_blit = render_create_blend_state(render_device, desc, MALLOC_TAG);
    }

    // depth stencil

    {
        rhi_depth_stencil_state_desc_t desc;

        desc.depth_test = false;
        desc.depth_test_func = rhi_compare_always;
        desc.depth_write_mask = false;
        desc.stencil_read_mask = 0xff;
        desc.stencil_write_mask = 0xff;
        desc.stencil_test = false;
        desc.stencil_front.stencil_test_func = rhi_compare_always;
        desc.stencil_front.stencil_fail_op = rhi_stencil_op_keep;
        desc.stencil_front.depth_fail_op = rhi_stencil_op_keep;

        // stencil sense reversed from original GL implementation
        // because RHI back-face is CW
        desc.stencil_front.stencil_depth_pass_op = rhi_stencil_op_decr_wrap;

        desc.stencil_back = desc.stencil_front;
        desc.stencil_back.stencil_depth_pass_op = rhi_stencil_op_incr_wrap;

        // no stencil test
        state->dss_stencil_off = render_create_depth_stencil_state(render_device, desc, MALLOC_TAG);

        // stencil accumulate
        desc.stencil_test = true;
        state->dss_stencil_accum = render_create_depth_stencil_state(render_device, desc, MALLOC_TAG);

        // stencil test
        desc.stencil_front.stencil_test_func = rhi_compare_equal;
        desc.stencil_front.stencil_depth_pass_op = rhi_stencil_op_keep;
        desc.stencil_back = desc.stencil_front;

        state->dss_stencil_eq = render_create_depth_stencil_state(render_device, desc, MALLOC_TAG);

        desc.stencil_front.stencil_fail_op = rhi_stencil_op_zero;
        desc.stencil_front.depth_fail_op = rhi_stencil_op_zero;
        desc.stencil_front.stencil_depth_pass_op = rhi_stencil_op_zero;
        desc.stencil_front.stencil_test_func = rhi_compare_notequal;
        desc.stencil_back = desc.stencil_front;

        state->dss_stencil_neq = render_create_depth_stencil_state(render_device, desc, MALLOC_TAG);
    }

    // create vertex layout and mesh
    {
        rhi_vertex_element_desc_t elems[2];
        elems[0].count = 2;
        elems[0].format = rhi_vertex_element_format_float;
        elems[0].offset = MEMBER_OFS(cg_gl_vertex_t, x);
        elems[0].semantic = rhi_program_input_semantic_position0;
        elems[1].count = 4;
        elems[1].format = rhi_vertex_element_format_float;
        elems[1].offset = MEMBER_OFS(cg_gl_vertex_t, r);
        elems[1].semantic = rhi_program_input_semantic_texcoord0;

        rhi_vertex_layout_desc_t verts;
        verts.elements = elems;
        verts.num_elements = 2;
        verts.stride = sizeof(cg_gl_vertex_t);
        verts.usage = rhi_usage_dynamic;

        rhi_mesh_data_layout_desc_t desc;

        desc.channels = &verts;
        desc.indices = NULL;
        desc.num_channels = 1;

        r_mesh_data_layout_t * const layout = render_create_mesh_data_layout(render_device, desc, MALLOC_TAG);

        const_mem_region_t null_buffer;
        ZEROMEM(&null_buffer);
        null_buffer.size = sizeof(cg_gl_vertex_t) * cg_gl_max_vertex_count;
        rhi_mesh_data_init_indirect_t mi;
        ZEROMEM(&mi);
        mi.num_channels = 1;
        mi.channels = &null_buffer;

        for (int i = 0; i < ARRAY_SIZE(state->meshes); ++i) {
            state->meshes[i] = render_create_mesh(render_device, mi, layout, MALLOC_TAG);
        }

        render_release(&layout->resource, MALLOC_TAG);
    }

#ifdef _CANVAS_EXPERIMENTAL
    cg_experimental_init(state, cg_heap, render_device);
#endif

    CG_GL_TRACE_POP();
}

void cg_gl_state_free(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

#ifdef _CANVAS_EXPERIMENTAL
    cg_experimental_shutdown();
#endif

    cg_gl_texture_free(state, &state->white);
    render_release(&state->shaders.color->resource, MALLOC_TAG);
    render_release(&state->shaders.color_rgb_fill_alpha_red->resource, MALLOC_TAG);
    render_release(&state->shaders.color_alpha_mask->resource, MALLOC_TAG);
    render_release(&state->shaders.color_alpha_test->resource, MALLOC_TAG);
    render_release(&state->shaders.color_alpha_test_rgb_fill_alpha_red->resource, MALLOC_TAG);
    render_release(&state->shaders.video->resource, MALLOC_TAG);

    for (int i = 0; i < ARRAY_SIZE(state->meshes); ++i) {
        render_release(&state->meshes[i]->resource, MALLOC_TAG);
    }
    render_release(&state->rs_scissor_off->resource, MALLOC_TAG);
    render_release(&state->rs_scissor_on->resource, MALLOC_TAG);

    render_release(&state->bs_blend_alpha_color_write_mask_rgb->resource, MALLOC_TAG);
    render_release(&state->bs_blend_alpha_color_write_mask_all->resource, MALLOC_TAG);
    render_release(&state->bs_blend_off_color_write_mask_rgb->resource, MALLOC_TAG);
    render_release(&state->bs_blend_off_color_write_mask_none->resource, MALLOC_TAG);
    render_release(&state->bs_blend_blit->resource, MALLOC_TAG);

    render_release(&state->dss_stencil_off->resource, MALLOC_TAG);
    render_release(&state->dss_stencil_accum->resource, MALLOC_TAG);
    render_release(&state->dss_stencil_eq->resource, MALLOC_TAG);
    render_release(&state->dss_stencil_neq->resource, MALLOC_TAG);

    for (int i = 0; i < ARRAY_SIZE(state->uniforms); ++i) {
        if (state->uniforms[i]) {
            render_release(&state->uniforms[i]->resource, MALLOC_TAG);
        }
    }

    CG_GL_TRACE_POP();
}

void cg_gl_state_begin(cg_gl_state_t * const state, const int width, const int height) {
    CG_GL_TRACE_PUSH_FN();

    // render to state->screen
    cg_gl_render_to_screen(state, width, height);

    // clear screen
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_clear_screen_cds,
        (render_clear_color_t){
            .r = 0,
            .g = 0,
            .b = 0,
            .a = 1 // full alpha for punch-through
        },
        (render_clear_depth_t){
            .depth = FLT_MAX},
        (render_clear_stencil_t){
            .stencil = 0},
        MALLOC_TAG);

    // set default render states
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        &state->rs_scissor_off->rasterizer_state,
        &state->dss_stencil_off->depth_stencil_state,
        &state->bs_blend_off_color_write_mask_rgb->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_end(cg_gl_state_t * const state) {
}

void cg_gl_state_bind_color_shader(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.color->program,
        MALLOC_TAG);

    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);
    cg_gl_texture_bind(state, (tex == NULL) ? &state->white : tex);

    CG_GL_TRACE_POP();
}

void cg_gl_state_bind_color_rgb_fill_alpha_red_shader(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.color_rgb_fill_alpha_red->program,
        MALLOC_TAG);

    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);
    cg_gl_texture_bind(state, (tex == NULL) ? &state->white : tex);

    CG_GL_TRACE_POP();
}

void cg_gl_state_bind_color_shader_alpha_mask(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const cg_gl_texture_t * const mask) {
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.color_alpha_mask->program,
        MALLOC_TAG);

    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);
    cg_gl_texture_bind_two(state, (tex == NULL) ? &state->white : tex, (mask == NULL) ? &state->white : mask);
}

void cg_gl_state_bind_color_shader_alpha_test(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const float threshold) {
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.color_alpha_test->program,
        MALLOC_TAG);
    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);
    cg_gl_set_uniform_float(state, rhi_program_uniform_threshold, threshold, MALLOC_TAG);
    cg_gl_texture_bind(state, (tex == NULL) ? &state->white : tex);
}

void cg_gl_state_bind_color_shader_alpha_rgb_fill_alpha_red_test(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const float threshold) {
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.color_alpha_test_rgb_fill_alpha_red->program,
        MALLOC_TAG);
    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);
    cg_gl_set_uniform_float(state, rhi_program_uniform_threshold, threshold, MALLOC_TAG);
    cg_gl_texture_bind(state, (tex == NULL) ? &state->white : tex);
}

void cg_gl_state_bind_color_shader_raw(cg_gl_state_t * const state, const cg_color_t * const fill, rhi_texture_t * const * const tex) {
    CG_GL_TRACE_PUSH_FN();

    ASSERT(tex);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.color->program,
        MALLOC_TAG);

    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);
    cg_gl_texture_bind_raw(state, tex);

    CG_GL_TRACE_POP();
}

void cg_gl_state_bind_video_shader(cg_gl_state_t * const state, const cg_color_t * const fill, rhi_texture_t * const * const chroma, rhi_texture_t * const * const luma) {
    CG_GL_TRACE_PUSH_FN();

    ASSERT(chroma && luma);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_program_indirect,
        &state->shaders.video->program,
        MALLOC_TAG);

    cg_gl_set_uniform_color(state, rhi_program_uniform_fill, fill, MALLOC_TAG);

    cg_gl_texture_bind_raw_two(state, chroma, luma);

    CG_GL_TRACE_POP();
}

void cg_gl_state_draw(cg_gl_state_t * const state, const rhi_draw_mode_e prim, const int count, const int offset) {
    CG_GL_TRACE_PUSH_FN();

    cg_gl_apply_scissor_state(state);

    ASSERT(!state->map_count);
    rhi_draw_params_indirect_t draw_params;

    struct {
        int idx_ofs;
        int elm_count;
        rhi_mesh_t * const * mesh;
    } * draw_single_mesh;

    render_cmd_stream_blocking_latch_cmd_buf(&state->render_device->default_cmd_stream);
    draw_single_mesh = render_cmd_buf_unchecked_alloc(state->render_device->default_cmd_stream.buf, 8, sizeof(*draw_single_mesh));

    draw_params.idx_ofs = &draw_single_mesh->idx_ofs;
    draw_params.elm_counts = &draw_single_mesh->elm_count;
    draw_params.mesh_list = &draw_single_mesh->mesh;
    draw_params.mode = prim;
    draw_params.num_meshes = 1;

    if (!draw_single_mesh || !render_cmd_buf_write_draw_indirect(state->render_device->default_cmd_stream.buf, draw_params, MALLOC_TAG)) {
        // could not allocate parameters or could not write the command to the stream
        // flush the command stream and rewrite

        render_flush_cmd_stream(&state->render_device->default_cmd_stream, render_no_wait);
        render_cmd_stream_blocking_latch_cmd_buf(&state->render_device->default_cmd_stream);

        // must succeed
        draw_single_mesh = render_cmd_buf_alloc(state->render_device->default_cmd_stream.buf, 8, sizeof(*draw_single_mesh));

        draw_params.idx_ofs = &draw_single_mesh->idx_ofs;
        draw_params.elm_counts = &draw_single_mesh->elm_count;
        draw_params.mesh_list = &draw_single_mesh->mesh;

        VERIFY(render_cmd_buf_write_draw_indirect(state->render_device->default_cmd_stream.buf, draw_params, MALLOC_TAG));
    }

    draw_single_mesh->idx_ofs = offset;
    draw_single_mesh->elm_count = count;
    draw_single_mesh->mesh = &state->meshes[state->cur_mesh]->mesh;

    state->meshes[state->cur_mesh]->resource.fence = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

cg_gl_vertex_t * cg_gl_state_map_vertex_range(cg_gl_state_t * const state, const int count) {
    CG_GL_TRACE_PUSH_FN();

    ++state->cur_mesh;
    if (state->cur_mesh >= ARRAY_SIZE(state->meshes)) {
        state->cur_mesh = 0;
    }

    state->map_count = count;

    const int vertex_ofs = state->vertex_ofs = state->map_ofs;

    if ((vertex_ofs + count) <= cg_gl_max_vertex_count) {
        const int bank = state->active_bank;
        ASSERT((bank >= 0) && (bank < cg_gl_num_vertex_banks));
        CG_GL_TRACE_POP();
        return &state->vertices[bank][vertex_ofs];
    }

    // overflow, flush this vertex bank.
    const int active_bank = state->active_bank;
    const int next_bank = (active_bank + 1) % cg_gl_num_vertex_banks;

    // make sure we have flushed the previous buffer before we start writing here...

    render_conditional_flush_cmd_stream_and_wait_fence(
        state->render_device,
        &state->render_device->default_cmd_stream,
        state->gl_fences[next_bank]);

    // no need to copy partial verts from current buffer...

    state->vertex_ofs = 0;
    state->map_ofs = 0;
    state->active_bank = next_bank;

    ASSERT(count <= cg_gl_max_vertex_count);

    CG_GL_TRACE_POP();
    return &state->vertices[next_bank][0];
}

void cg_gl_state_finish_vertex_range(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    ASSERT(state->map_count);
    const int count = state->map_count;
    state->map_count = 0;

    // move the bank watermark forward
    ASSERT(state->map_ofs < state->vertex_ofs + count);

    state->map_ofs = state->vertex_ofs + count;
    const int bank = state->active_bank;
    const float * const data = (float *)&state->vertices[bank][state->vertex_ofs];

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_upload_mesh_channel_data_indirect,
        &state->meshes[state->cur_mesh]->mesh,
        0,
        0,
        state->map_ofs - state->vertex_ofs,
        data,
        MALLOC_TAG);

    state->gl_fences[bank] = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

void cg_gl_state_finish_vertex_range_with_count(cg_gl_state_t * const state, const int count) {
    CG_GL_TRACE_PUSH_FN();

    ASSERT(state->map_count && (count <= state->map_count));
    state->map_count = 0;

    // move the bank watermark forward
    ASSERT(state->map_ofs < state->vertex_ofs + count);

    state->map_ofs = state->vertex_ofs + count;
    const int bank = state->active_bank;
    const float * const data = (float *)&state->vertices[bank][state->vertex_ofs];

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_upload_mesh_channel_data_indirect,
        &state->meshes[state->cur_mesh]->mesh,
        0,
        0,
        state->map_ofs - state->vertex_ofs,
        data,
        MALLOC_TAG);

    state->gl_fences[bank] = render_get_cmd_stream_fence(&state->render_device->default_cmd_stream);

    CG_GL_TRACE_POP();
}

#ifdef CG_TODO
void cg_gl_texture_read_pixels(const cg_gl_texture_t * const tex, const int32_t sx, const int32_t sy, const int32_t sw, const int32_t sh) {
    ASSERT(tex->data != NULL);

    glReadPixels(sx, sy, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
}
#endif

void cg_gl_state_set_mode_blend_off(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        NULL,
        &state->bs_blend_off_color_write_mask_rgb->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_blend_alpha_rgb(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        NULL,
        &state->bs_blend_alpha_color_write_mask_rgb->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_blend_alpha_all(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        NULL,
        &state->bs_blend_alpha_color_write_mask_all->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_blit(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        NULL,
        &state->bs_blend_blit->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_stencil_off(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        &state->dss_stencil_off->depth_stencil_state,
        NULL,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_stencil_accum(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        &state->dss_stencil_accum->depth_stencil_state,
        &state->bs_blend_off_color_write_mask_none->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_stencil_eq(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        &state->dss_stencil_eq->depth_stencil_state,
        &state->bs_blend_alpha_color_write_mask_rgb->blend_state,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_stencil_neq(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &state->render_device->default_cmd_stream,
        render_cmd_buf_write_set_render_state_indirect,
        NULL,
        &state->dss_stencil_neq->depth_stencil_state,
        NULL,
        0,
        MALLOC_TAG);

    CG_GL_TRACE_POP();
}

void cg_gl_state_set_mode_debug(cg_gl_state_t * const state) {
#ifdef CG_TODO
    // NOTE: linewidth/pointsize are not portable concepts
    // linewidth is not support on desktop GL
    // linewidth and pointsize are not supported in DX11

    glLineWidth(1);
    glPointSize(4);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void cg_gl_set_scissor_rect(cg_gl_state_t * const state, const int x0, const int y0, const int x1, const int y1) {
    state->scissor.canvas.x0 = x0;
    state->scissor.canvas.y0 = y0;
    state->scissor.canvas.x1 = x1;
    state->scissor.canvas.y1 = y1;
}

void cg_gl_enable_scissor(cg_gl_state_t * const state, const bool enabled) {
    state->scissor.canvas.enabled = enabled;
}

void cg_gl_apply_scissor_state(cg_gl_state_t * const state) {
    CG_GL_TRACE_PUSH_FN();

    if (state->scissor.canvas.enabled && (state->scissor.rhi.x0 != state->scissor.canvas.x0 || state->scissor.rhi.y0 != state->scissor.canvas.y0 || state->scissor.rhi.x1 != state->scissor.canvas.x1 || state->scissor.rhi.y1 != state->scissor.canvas.y1)) {
        RENDER_ENSURE_WRITE_CMD_STREAM(
            &state->render_device->default_cmd_stream,
            render_cmd_buf_write_set_scissor_rect,
            state->scissor.canvas.x0,
            state->scissor.canvas.y0,
            state->scissor.canvas.x1,
            state->scissor.canvas.y1,
            MALLOC_TAG);

        state->scissor.rhi.x0 = state->scissor.canvas.x0;
        state->scissor.rhi.y0 = state->scissor.canvas.y0;
        state->scissor.rhi.x1 = state->scissor.canvas.x1;
        state->scissor.rhi.y1 = state->scissor.canvas.y1;
    }

    if (state->scissor.rhi.enabled != state->scissor.canvas.enabled) {
        RENDER_ENSURE_WRITE_CMD_STREAM(
            &state->render_device->default_cmd_stream,
            render_cmd_buf_write_set_render_state_indirect,
            state->scissor.canvas.enabled ? &state->rs_scissor_on->rasterizer_state : &state->rs_scissor_off->rasterizer_state,
            NULL,
            NULL,
            0,
            MALLOC_TAG);

        state->scissor.rhi.enabled = state->scissor.canvas.enabled;
    }

    CG_GL_TRACE_POP();
}
