/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
cg_gl.h

canvas rendering backend
*/

#include "cg_math.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * Canvas rendering interface
 * ==========================================================================*/

struct cg_vec2_t;
struct cg_sdf_rect_uniforms_t;
struct cg_sdf_rect_border_uniforms_t;
struct cg_ivec2_t;

typedef struct cg_gl_texture_t {
    r_texture_t * texture;
    rhi_sampler_state_desc_t sampler_state;
} cg_gl_texture_t;

/* ------------------------------------------------------------------------- */

typedef struct cg_gl_vertex_t {
    float x, y;
    float r, g, b, a;
} cg_gl_vertex_t;

/* ------------------------------------------------------------------------- */

typedef struct cg_gl_shader_t {
    r_program_t * program;
} cg_gl_shader_t;

/* ------------------------------------------------------------------------- */

typedef struct cg_gl_scissor_state_t {
    int x0, y0, x1, y1;
    bool enabled;
} cg_gl_scissor_state_t;

/* ------------------------------------------------------------------------- */

struct cg_heap_t;

typedef struct cg_gl_state_t {
    runtime_configuration_canvas_gl_t config;

    cg_gl_vertex_t ** vertices;
    rb_fence_t * gl_fences;

    int active_bank;
    int map_ofs;
    int vertex_ofs;
    int map_count;
    int cur_mesh;

    render_device_t * render_device;

    struct {
        r_program_t * color;
        r_program_t * color_rgb_fill_alpha_red;
        r_program_t * color_alpha_mask;
        r_program_t * color_alpha_test;
        r_program_t * color_alpha_test_rgb_fill_alpha_red;
        r_program_t * sdf_rect;
        r_program_t * sdf_rect_border;
        r_program_t * video;
        r_program_t * video_hdr;
    } shaders;

    const_mem_region_t mesh_null_buffer;

    r_mesh_t ** meshes;
    r_mesh_data_layout_t * mesh_layout;

    cg_gl_texture_t white;

    r_uniform_buffer_t * uniforms[rhi_program_num_uniforms];

    r_blend_state_t * bs_blend_off_color_write_mask_rgb;
    r_blend_state_t * bs_blend_off_color_write_mask_none;
    r_blend_state_t * bs_blend_alpha_color_write_mask_rgb;
    r_blend_state_t * bs_blend_alpha_color_write_mask_all;
    r_blend_state_t * bs_blend_blit;
    r_blend_state_t * bs_blend_font;

    r_rasterizer_state_t * rs_scissor_off;
    r_rasterizer_state_t * rs_scissor_on;

    r_depth_stencil_state_t * dss_stencil_off;
    r_depth_stencil_state_t * dss_stencil_accum;
    r_depth_stencil_state_t * dss_stencil_eq;
    r_depth_stencil_state_t * dss_stencil_neq;

    struct cg_heap_t * cg_heap; // needed for heap_t* and mutex_t*

    struct {
        cg_gl_scissor_state_t rhi; // The scissor state that has been sent to the RHI
        cg_gl_scissor_state_t canvas; // The scissor state requested by the canvas
    } scissor;
} cg_gl_state_t;

/* ------------------------------------------------------------------------- */

typedef enum cg_gl_texture_usage {
    cg_gl_static_texture_usage,
    cg_gl_dynamic_texture_usage
} cg_gl_texture_usage;

bool cg_gl_texture_from_memory(render_device_t * const render_device, cg_gl_texture_t * const tex, const rhi_sampler_state_desc_t sampler_state, const image_t image, const cg_gl_texture_usage usage, void (*free_mem_callback_called_from_render_thread)(rhi_device_t * device, void * arg), void * arg);

void cg_gl_texture_init_with_color(cg_gl_state_t * const state, cg_gl_texture_t * const tex, const cg_color_packed_t * const color);

void cg_gl_texture_bind(cg_gl_state_t * const state, const cg_gl_texture_t * const tex);

void cg_gl_texture_bind_raw(cg_gl_state_t * const state, rhi_texture_t * const * const tex);

void cg_gl_texture_free(cg_gl_state_t * const state, cg_gl_texture_t * const tex);

void cg_gl_texture_update(cg_gl_state_t * const state, cg_gl_texture_t * const tex, void * const pixels);

#if !(defined(_VADER) || defined(_LEIA))
void cg_gl_sub_texture_update(cg_gl_state_t * const state, cg_gl_texture_t * const tex, const image_mips_t image_mips);
#endif

void cg_gl_texture_update_sampler_state(cg_gl_state_t * const state, cg_gl_texture_t * const tex);

/* ------------------------------------------------------------------------- */

typedef struct cg_gl_framebuffer_t {
    r_render_target_t * render_target;
    int width, height;
} cg_gl_framebuffer_t;

void cg_gl_framebuffer_free(cg_gl_state_t * const state, cg_gl_framebuffer_t * const fb);

void cg_gl_discard_framebuffer(cg_gl_state_t * const state, const cg_gl_framebuffer_t * const fb);
void cg_gl_render_to_framebuffer(cg_gl_state_t * const state, const cg_gl_framebuffer_t * const fb);

void cg_gl_render_to_screen(cg_gl_state_t * const state, const int32_t width, const int32_t height);

void cg_gl_framebuffer_init(cg_gl_state_t * const state, cg_gl_framebuffer_t * const fb, const int32_t width, const int32_t height);

/* ------------------------------------------------------------------------- */

void cg_gl_state_init(cg_gl_state_t * const state, struct cg_heap_t * const cg_heap, render_device_t * const render_device, const runtime_configuration_canvas_gl_t config);

void cg_gl_state_free(cg_gl_state_t * const state);

void cg_gl_state_begin(cg_gl_state_t * const state, const int width, const int height, uint32_t clear_color);

void cg_gl_state_end(cg_gl_state_t * const state);

void cg_gl_state_draw(cg_gl_state_t * const state, const rhi_draw_mode_e prim, const int count, const int offset);

void cg_gl_state_draw_mesh(cg_gl_state_t * const state, r_mesh_t * const mesh, const rhi_draw_mode_e prim, const int count, const int offset);

static inline void cg_gl_state_draw_points(cg_gl_state_t * const state, const int count, const int offset) {
    cg_gl_state_draw(state, rhi_triangles, count, offset);
}

static inline void cg_gl_state_draw_line_strip(cg_gl_state_t * const state, const int count, const int offset) {
    cg_gl_state_draw(state, rhi_line_strip, count, offset);
}

static inline void cg_gl_state_draw_tri_fan(cg_gl_state_t * const state, const int count, const int offset) {
    cg_gl_state_draw(state, rhi_triangle_fan, count, offset);
}

static inline void cg_gl_state_draw_tri_strip(cg_gl_state_t * const state, const int count, const int offset) {
    cg_gl_state_draw(state, rhi_triangle_strip, count, offset);
}

void cg_gl_state_bind_color_shader(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex);
void cg_gl_state_bind_color_rgb_fill_alpha_red_shader(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex);

void cg_gl_state_bind_color_shader_alpha_mask(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const cg_gl_texture_t * const mask);

void cg_gl_state_bind_color_shader_alpha_test(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const float threshold);
void cg_gl_state_bind_color_shader_alpha_rgb_fill_alpha_red_test(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const float threshold);

void cg_gl_state_bind_color_shader_raw(cg_gl_state_t * const state, const cg_color_t * const fill, rhi_texture_t * const * const tex);
void cg_gl_state_bind_video_shader(cg_gl_state_t * const state, const cg_color_t * const fill, rhi_texture_t * const * const chroma, rhi_texture_t * const * const luma, const cg_ivec2_t luma_tex_dim, const cg_ivec2_t chroma_tex_dim, const cg_ivec2_t framesize_dim);
void cg_gl_state_bind_video_shader_hdr(cg_gl_state_t * const state, const cg_color_t * const fill, rhi_texture_t * const * const chroma, rhi_texture_t * const * const luma, const cg_ivec2_t luma_tex_dim, const cg_ivec2_t chroma_tex_dim, const cg_ivec2_t framesize_dim);

void cg_gl_state_bind_sdf_rect_shader(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const struct cg_sdf_rect_uniforms_t uniforms);
void cg_gl_state_bind_sdf_rect_border_shader(cg_gl_state_t * const state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const struct cg_sdf_rect_border_uniforms_t uniforms);

cg_gl_vertex_t * cg_gl_state_map_vertex_range(cg_gl_state_t * const state, const int count);
void cg_gl_state_finish_vertex_range(cg_gl_state_t * const state);
void cg_gl_state_finish_vertex_range_with_count(cg_gl_state_t * const state, const int count);

static inline void cg_set_vert(cg_gl_vertex_t * const v, const int idx, const struct cg_vec2_t * const pos, const cg_color_t * const col) {
    v[idx].x = pos->x;
    v[idx].y = pos->y;
    v[idx].r = col->r; // u
    v[idx].g = col->g; // v
    v[idx].b = 0.0f;
    v[idx].a = col->a; // alpha
}

static inline void cg_copy_vert(cg_gl_vertex_t * const v, const int src, const int dst) {
    ASSERT(src < dst);
    v[dst] = v[src];
}

#ifdef CG_TO_DISCUSS
void cg_gl_texture_read_pixels(const cg_gl_texture_t * const tex, const int32_t sx, const int32_t sy, const int32_t sw, const int32_t sh);
#endif

void cg_gl_state_set_mode_blend_off(cg_gl_state_t * const state);
void cg_gl_state_set_mode_blend_alpha_rgb(cg_gl_state_t * const state);
void cg_gl_state_set_mode_blend_alpha_all(cg_gl_state_t * const state);
void cg_gl_state_set_mode_blit(cg_gl_state_t * const state);

void cg_gl_state_set_mode_stencil_off(cg_gl_state_t * const state);
void cg_gl_state_set_mode_stencil_accum(cg_gl_state_t * const state);
void cg_gl_state_set_mode_stencil_eq(cg_gl_state_t * const state);
void cg_gl_state_set_mode_stencil_neq(cg_gl_state_t * const state);

void cg_gl_state_set_mode_debug(cg_gl_state_t * const state);

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void cg_gl_set_scissor_rect(cg_gl_state_t * const state, const int x0, const int y0, const int x1, const int y1);
void cg_gl_enable_scissor(cg_gl_state_t * const state, const bool enabled);
void cg_gl_apply_scissor_state(cg_gl_state_t * const state);

#ifdef __cplusplus
}
#endif
