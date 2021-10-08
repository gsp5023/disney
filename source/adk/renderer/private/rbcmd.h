/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
rbcmd.h

render command buffers, and render command support
*/

#include "rhi.h"
#include "source/adk/imagelib/imagelib.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_RB_CMD_ID_BITS 6
#define RB_CMD_ID_MASK ((1 << NUM_RB_CMD_ID_BITS) - 1)
#define MAX_RB_CMD_JUMP_BITS (32 - NUM_RB_CMD_ID_BITS)
#define MAX_RB_CMD_JUMP_MASK ((1 << MAX_RB_CMD_JUMP_BITS) - 1)
#define MAX_RB_CMD_BUF_SIZE MAX_RB_CMD_JUMP_MASK

/*
=======================================
rb_cmd_buf_t

A small buffer containing commands to be
executed by a thread on a specific RHI
device.
=======================================
*/

typedef struct rb_cmd_buf_t {
    high_low_block_allocator_t hlba;
    uint32_t * next_cmd_ptr;
    uint32_t * last_cmd_ptr;
    /* for command buffer mark/unwind */
    struct {
        uint32_t * next_cmd_ptr;
        uint32_t * last_cmd_ptr;
        int hlba_low;
        int hlba_high;
        int num_cmds;
    } mark;
    int num_cmds;
    struct rb_cmd_buf_t * next;
    int submit_counter;
    sb_atomic_int32_t retire_counter;
#ifdef GUARD_PAGE_SUPPORT
    debug_sys_page_block_t guard_pages;
#endif
} rb_cmd_buf_t;

/*
=======================================
render_mark_cmd_buf()

Saves the current write position so it can
be unwound back to the last mark
=======================================
*/

void render_mark_cmd_buf(rb_cmd_buf_t * const cmd_buf);

/*
=======================================
render_unwind_cmd_buf()

Resets cmd buffer to last mark
=======================================
*/

void render_unwind_cmd_buf(rb_cmd_buf_t * const cmd_buf);

/*
=======================================
render_cmd_buf_next_cmd_ptr

setup the next_cmd_ptr if not already setup
=======================================
*/

static bool render_cmd_buf_next_cmd_ptr(rb_cmd_buf_t * cmd_buf) {
    if (cmd_buf->next_cmd_ptr) {
        return true;
    }

    cmd_buf->next_cmd_ptr = (uint32_t *)hlba_allocate_low(&cmd_buf->hlba, 4, sizeof(uint32_t) * 1);
    return cmd_buf->next_cmd_ptr != NULL;
}

/*
=======================================
render_cmd_buf_unchecked_alloc

allocate memory inside a command buffer for use by caller
=======================================
*/

static inline void * render_cmd_buf_unchecked_alloc(rb_cmd_buf_t * cmd_buf, int alignment, int size) {
    return hlba_allocate_high(&cmd_buf->hlba, alignment, size);
}

/*
=======================================
render_cmd_buf_alloc

allocate memory inside a command buffer for use by caller

terminates program with error if out of memory
=======================================
*/

static inline void * render_cmd_buf_alloc(rb_cmd_buf_t * cmd_buf, int alignment, int size) {
    ASSERT(cmd_buf);
    void * p = render_cmd_buf_unchecked_alloc(cmd_buf, alignment, size);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

/*
===============================================================================
render commands
===============================================================================
*/

typedef enum render_cmd_e {
    render_cmd_rhi_resource_add_ref_indirect,
    render_cmd_release_rhi_resource_indirect,
    render_cmd_upload_mesh_channel_data_indirect,
    render_cmd_upload_mesh_indices_indirect,
    render_cmd_upload_uniform_data_indirect,
    render_cmd_upload_texture_indirect,
#if defined(_LEIA) || defined(_VADER)
    render_cmd_bind_texture_address,
#endif
    render_cmd_set_display_size,
    render_cmd_set_viewport,
    render_cmd_set_scissor_rect,
    render_cmd_set_program_indirect,
    render_cmd_set_render_state_indirect,
    render_cmd_set_render_target_color_buffer_indirect,
    render_cmd_discard_render_target_data_indirect,
    render_cmd_set_render_target_indirect,
    render_cmd_clear_render_target_color_buffers_indirect,
    render_cmd_copy_render_target_buffers_indirect,
    render_cmd_set_uniform_binding_indirect,
    render_cmd_set_texture_sampler_state_indirect,
    render_cmd_set_texture_bindings_indirect,
    render_cmd_draw_indirect,
    render_cmd_clear_screen_cds,
    render_cmd_clear_screen_ds,
    render_cmd_clear_screen_c,
    render_cmd_clear_screen_d,
    render_cmd_clear_screen_s,
    render_cmd_present,
    render_cmd_screenshot,

    render_cmd_create_mesh_data_layout,
    render_cmd_create_mesh_indirect,
    render_cmd_create_mesh_shared_vertex_data_indirect,
    render_cmd_create_program_from_binary,
    render_cmd_create_blend_state,
    render_cmd_create_depth_stencil_state,
    render_cmd_create_rasterizer_state,
    render_cmd_create_uniform_buffer,
    render_cmd_create_texture_2d,
    render_cmd_create_render_target,
    render_cmd_callback,
    render_cmd_breakpoint,
    num_render_commands,
    FORCE_ENUM_SIGNED(render_cmd_e)
} render_cmd_e;

STATIC_ASSERT(num_render_commands < RB_CMD_ID_MASK);

bool render_cmd_buf_write_render_command(rb_cmd_buf_t * const cmd_buf, const render_cmd_e cmd_id, const void * const cmd, const int alignment, const int size);

#ifdef ENABLE_RENDER_TAGS
#define DECL_RENDER_TAG const char * tag;
#define ASSIGN_RENDER_TAG tag
#else
#define DECL_RENDER_TAG
#define ASSIGN_RENDER_TAG
#endif

/*
=======================================
render_cmd_rhi_resource_add_ref_indirect
=======================================
*/

typedef struct render_cmd_rhi_resource_add_ref_indirect_t {
    rhi_resource_t * const * resource;
    DECL_RENDER_TAG
} render_cmd_rhi_resource_add_ref_indirect_t;

static inline bool render_cmd_buf_write_rhi_resource_add_ref_indirect(rb_cmd_buf_t * const cmd_buf, rhi_resource_t * const * const resource, const char * const tag) {
    const render_cmd_rhi_resource_add_ref_indirect_t cmd = {
        resource,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_rhi_resource_add_ref_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_release_rhi_resource_indirect
=======================================
*/

typedef struct render_cmd_release_rhi_resource_indirect_t {
    rhi_resource_t * const * resource;
    DECL_RENDER_TAG
} render_cmd_release_rhi_resource_indirect_t;

static inline bool render_cmd_buf_write_release_rhi_resource_indirect(rb_cmd_buf_t * const cmd_buf, rhi_resource_t * const * const resource, const char * const tag) {
    const render_cmd_release_rhi_resource_indirect_t cmd = {
        resource,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_release_rhi_resource_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_upload_mesh_channel_data_indirect
=======================================
*/

typedef struct render_cmd_upload_mesh_channel_data_indirect_t {
    rhi_mesh_t * const * mesh;
    int channel_index;
    int first_elem;
    int num_elems;
    const void * data;
    DECL_RENDER_TAG
} render_cmd_upload_mesh_channel_data_indirect_t;

static inline bool render_cmd_buf_write_upload_mesh_channel_data_indirect(rb_cmd_buf_t * const cmd_buf, rhi_mesh_t * const * const mesh, const int channel_index, const int first_elem, const int num_elems, const void * const data, const char * const tag) {
    const render_cmd_upload_mesh_channel_data_indirect_t cmd = {
        mesh,
        channel_index,
        first_elem,
        num_elems,
        data,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_upload_mesh_channel_data_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_upload_mesh_indices_indirect
=======================================
*/

typedef struct render_cmd_upload_mesh_indices_indirect_t {
    rhi_mesh_t * const * mesh;
    int first_index;
    int num_indices;
    const void * indices;
    DECL_RENDER_TAG
} render_cmd_upload_mesh_indices_indirect_t;

static inline bool render_cmd_buf_write_upload_mesh_indices_indirect(rb_cmd_buf_t * const cmd_buf, rhi_mesh_t * const * const mesh, const int first_index, const int num_indices, const void * const indices, const char * const tag) {
    const render_cmd_upload_mesh_indices_indirect_t cmd = {
        mesh,
        first_index,
        num_indices,
        indices,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_upload_mesh_indices_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_upload_uniform_data_indirect
=======================================
*/

typedef struct render_cmd_upload_uniform_data_indirect_t {
    rhi_uniform_buffer_t * const * uniform_buffer;
    int ofs;
    const_mem_region_t data;
    DECL_RENDER_TAG
} render_cmd_upload_uniform_data_indirect_t;

static inline bool render_cmd_buf_write_upload_uniform_data_indirect(rb_cmd_buf_t * const cmd_buf, rhi_uniform_buffer_t * const * const uniform_buffer, const const_mem_region_t data, const int ofs, const char * const tag) {
    const render_cmd_upload_uniform_data_indirect_t cmd = {
        uniform_buffer,
        ofs,
        data,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_upload_uniform_data_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_upload_texture_indirect
=======================================
*/

typedef struct render_cmd_upload_texture_indirect_t {
    rhi_texture_t * const * texture;
    image_mips_t mipmaps;
    DECL_RENDER_TAG
} render_cmd_upload_texture_indirect_t;

static inline bool render_cmd_buf_write_upload_texture_indirect(rb_cmd_buf_t * const cmd_buf, rhi_texture_t * const * const texture, const image_mips_t mipmaps, const char * const tag) {
    const render_cmd_upload_texture_indirect_t cmd = {
        texture,
        mipmaps,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_upload_texture_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_bind_texture_address
=======================================
*/

#if defined(_LEIA) || defined(_VADER)

typedef struct render_cmd_bind_texture_address_t {
    rhi_texture_t * const * texture;
    image_mips_t mipmaps;
    DECL_RENDER_TAG
} render_cmd_bind_texture_address_t;

static inline bool render_cmd_buf_write_bind_texture_address(rb_cmd_buf_t * const cmd_buf, rhi_texture_t * const * const texture, const image_mips_t mipmaps, const char * const tag) {
    const render_cmd_bind_texture_address_t cmd = {
        texture,
        mipmaps,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_bind_texture_address, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

#endif

/*
=======================================
render_cmd_buf_write_set_display_size
=======================================
*/

typedef struct render_cmd_set_display_size_t {
    int w, h;
    DECL_RENDER_TAG
} render_cmd_set_display_size_t;

static inline bool render_cmd_buf_write_set_display_size(rb_cmd_buf_t * const cmd_buf, const int w, const int h, const char * const tag) {
    ASSERT_MSG((w > 0) && (h > 0), "Error: framebuffer size must have dimensions greater than zero.");
    const render_cmd_set_display_size_t cmd = {
        w,
        h,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_display_size, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_viewport
=======================================
*/

typedef struct render_cmd_set_viewport_t {
    int x0, y0, x1, y1;
    DECL_RENDER_TAG
} render_cmd_set_viewport_t;

static inline bool render_cmd_buf_write_set_viewport(rb_cmd_buf_t * const cmd_buf, const int x0, const int y0, const int x1, const int y1, const char * const tag) {
    const render_cmd_set_viewport_t cmd = {
        x0,
        y0,
        x1,
        y1,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_viewport, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_scissor_rect
=======================================
*/

typedef struct render_cmd_set_scissor_rect_t {
    int x0, y0, x1, y1;
    DECL_RENDER_TAG
} render_cmd_set_scissor_rect_t;

static inline bool render_cmd_buf_write_set_scissor_rect(rb_cmd_buf_t * const cmd_buf, const int x0, const int y0, const int x1, const int y1, const char * const tag) {
    const render_cmd_set_scissor_rect_t cmd = {
        x0,
        y0,
        x1,
        y1,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_scissor_rect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_program_indirect_t
=======================================
*/

typedef struct render_cmd_set_program_indirect_t {
    rhi_program_t * const * program;
    DECL_RENDER_TAG
} render_cmd_set_program_indirect_t;

static inline bool render_cmd_buf_write_set_program_indirect(rb_cmd_buf_t * const cmd_buf, rhi_program_t * const * const program, const char * const tag) {
    const render_cmd_set_program_indirect_t cmd = {
        program,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_program_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_render_state_indirect
=======================================
*/

typedef struct render_cmd_set_render_state_indirect_t {
    rhi_rasterizer_state_t * const * rs;
    rhi_depth_stencil_state_t * const * dss;
    rhi_blend_state_t * const * bs;
    uint32_t stencil_ref;
    DECL_RENDER_TAG
} render_cmd_set_render_state_indirect_t;

static inline bool render_cmd_buf_write_set_render_state_indirect(rb_cmd_buf_t * const cmd_buf, rhi_rasterizer_state_t * const * const rs, rhi_depth_stencil_state_t * const * const dss, rhi_blend_state_t * const * const bs, const uint32_t stencil_ref, const char * const tag) {
    const render_cmd_set_render_state_indirect_t cmd = {
        rs,
        dss,
        bs,
        stencil_ref,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_render_state_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_render_target_color_buffer_indirect
=======================================
*/

typedef struct render_cmd_set_render_target_color_buffer_indirect_t {
    rhi_render_target_t * const * render_target;
    rhi_render_target_color_buffer_indirect_t color_buffer;
    DECL_RENDER_TAG
} render_cmd_set_render_target_color_buffer_indirect_t;

static inline bool render_cmd_buf_write_set_render_target_color_buffer_indirect(rb_cmd_buf_t * const cmd_buf, rhi_render_target_t * const * const render_target, const rhi_render_target_color_buffer_indirect_t color_buffer, const char * const tag) {
    const render_cmd_set_render_target_color_buffer_indirect_t cmd = {
        render_target,
        color_buffer,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_render_target_color_buffer_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_discard_render_target_data_indirect
=======================================
*/

typedef struct render_cmd_discard_render_target_data_indirect_t {
    rhi_render_target_t * const * render_target;
    DECL_RENDER_TAG
} render_cmd_discard_render_target_data_indirect_t;

static inline bool render_cmd_buf_write_discard_render_target_data_indirect(rb_cmd_buf_t * const cmd_buf, rhi_render_target_t * const * const render_target, const char * const tag) {
    const render_cmd_discard_render_target_data_indirect_t cmd = {
        render_target,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_discard_render_target_data_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_render_target_indirect
=======================================
*/

typedef struct render_cmd_set_render_target_indirect_t {
    rhi_render_target_t * const * render_target;
    DECL_RENDER_TAG
} render_cmd_set_render_target_indirect_t;

static inline bool render_cmd_buf_write_set_render_target_indirect(rb_cmd_buf_t * const cmd_buf, rhi_render_target_t * const * const render_target, const char * const tag) {
    const render_cmd_set_render_target_indirect_t cmd = {
        render_target,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_render_target_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_clear_render_target_color_buffers_indirect
=======================================
*/

typedef struct render_cmd_clear_render_target_color_buffers_indirect_t {
    rhi_render_target_t * const * render_target;
    uint32_t clear_color;
    DECL_RENDER_TAG
} render_cmd_clear_render_target_color_buffers_indirect_t;

static inline bool render_cmd_buf_write_clear_render_target_color_buffers_indirect(rb_cmd_buf_t * const cmd_buf, rhi_render_target_t * const * const render_target, uint32_t clear_color, const char * const tag) {
    const render_cmd_clear_render_target_color_buffers_indirect_t cmd = {
        render_target,
        clear_color,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_clear_render_target_color_buffers_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_copy_render_target_buffers_indirect
=======================================
*/

typedef struct render_cmd_copy_render_target_buffers_indirect_t {
    rhi_render_target_t * const * src_render_target;
    rhi_render_target_t * const * dst_render_target;
    DECL_RENDER_TAG
} render_cmd_copy_render_target_buffers_indirect_t;

static inline bool render_cmd_buf_write_copy_render_target_buffers_indirect(rb_cmd_buf_t * const cmd_buf, rhi_render_target_t * const * const src_render_target, rhi_render_target_t * const * const dst_render_target, const char * const tag) {
    const render_cmd_copy_render_target_buffers_indirect_t cmd = {
        src_render_target,
        dst_render_target,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_copy_render_target_buffers_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_uniform_binding_indirect
=======================================
*/

typedef struct render_cmd_set_uniform_binding_indirect_t {
    rhi_uniform_binding_indirect_t uniform_binding;
    DECL_RENDER_TAG
} render_cmd_set_uniform_binding_indirect_t;

static inline bool render_cmd_buf_write_set_uniform_binding(rb_cmd_buf_t * const cmd_buf, const rhi_uniform_binding_indirect_t uniform_binding, const char * const tag) {
    const render_cmd_set_uniform_binding_indirect_t cmd = {
        uniform_binding,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_uniform_binding_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_texture_sampler_state_indirect
=======================================
*/

typedef struct render_cmd_set_texture_sampler_state_indirect_t {
    rhi_texture_t * const * texture;
    rhi_sampler_state_desc_t sampler_state;
    DECL_RENDER_TAG
} render_cmd_set_texture_sampler_state_indirect_t;

static inline bool render_cmd_buf_write_set_texture_sampler_state_indirect(rb_cmd_buf_t * const cmd_buf, rhi_texture_t * const * const texture, const rhi_sampler_state_desc_t sampler_state, const char * const tag) {
    const render_cmd_set_texture_sampler_state_indirect_t cmd = {
        texture,
        sampler_state,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_texture_sampler_state_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_set_texture_bindings_indirect
=======================================
*/

typedef struct render_cmd_set_texture_bindings_indirect_t {
    rhi_texture_bindings_indirect_t texture_bindings;
    DECL_RENDER_TAG
} render_cmd_set_texture_bindings_indirect_t;

static inline bool render_cmd_buf_write_set_texture_bindings_indirect(rb_cmd_buf_t * const cmd_buf, const rhi_texture_bindings_indirect_t texture_bindings, const char * const tag) {
    const render_cmd_set_texture_bindings_indirect_t cmd = {
        texture_bindings,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_set_texture_bindings_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_draw_indirect
=======================================
*/

typedef struct render_cmd_draw_indirect_t {
    rhi_draw_params_indirect_t params;
    DECL_RENDER_TAG
} render_cmd_draw_indirect_t;

static inline bool render_cmd_buf_write_draw_indirect(rb_cmd_buf_t * const cmd_buf, const rhi_draw_params_indirect_t params, const char * const tag) {
    const render_cmd_draw_indirect_t cmd = {
        params,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_draw_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_clear_screen_cds
=======================================
*/

typedef struct render_clear_color_t {
    float r, g, b, a;
} render_clear_color_t;

typedef struct render_clear_depth_t {
    float depth;
} render_clear_depth_t;

typedef struct render_clear_stencil_t {
    uint8_t stencil;
} render_clear_stencil_t;

typedef struct render_cmd_clear_screen_cds_t {
    float r, g, b, a, d;
    uint8_t s;
    DECL_RENDER_TAG
} render_cmd_clear_screen_cds_t;

static inline bool render_cmd_buf_write_clear_screen_cds(rb_cmd_buf_t * const cmd_buf, const render_clear_color_t color, const render_clear_depth_t depth, const render_clear_stencil_t stencil, const char * const tag) {
    const render_cmd_clear_screen_cds_t cmd = {
        color.r,
        color.g,
        color.b,
        color.a,
        depth.depth,
        stencil.stencil,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_clear_screen_cds, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_clear_screen_ds
=======================================
*/

typedef struct render_cmd_clear_screen_ds_t {
    float d;
    uint8_t s;
    DECL_RENDER_TAG
} render_cmd_clear_screen_ds_t;

static inline bool render_cmd_buf_write_clear_screen_ds(rb_cmd_buf_t * const cmd_buf, const render_clear_depth_t depth, const render_clear_stencil_t stencil, const char * const tag) {
    const render_cmd_clear_screen_ds_t cmd = {
        depth.depth,
        stencil.stencil,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_clear_screen_ds, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_clear_screen_c
=======================================
*/

typedef struct render_cmd_clear_screen_c_t {
    float r, g, b, a;
    DECL_RENDER_TAG
} render_cmd_clear_screen_c_t;

static inline bool render_cmd_buf_write_clear_screen_c(rb_cmd_buf_t * const cmd_buf, const render_clear_color_t color, const char * const tag) {
    const render_cmd_clear_screen_c_t cmd = {
        color.r,
        color.g,
        color.b,
        color.a,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_clear_screen_c, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_clear_screen_d
=======================================
*/

typedef struct render_cmd_clear_screen_d_t {
    float d;
    DECL_RENDER_TAG
} render_cmd_clear_screen_d_t;

static inline bool render_cmd_buf_write_clear_screen_d(rb_cmd_buf_t * const cmd_buf, const float d, const char * const tag) {
    const render_cmd_clear_screen_d_t cmd = {
        d,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_clear_screen_d, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_clear_screen_s
=======================================
*/

typedef struct render_cmd_clear_screen_s_t {
    uint8_t s;
    DECL_RENDER_TAG
} render_cmd_clear_screen_s_t;

static inline bool render_cmd_buf_write_clear_screen_s(rb_cmd_buf_t * const cmd_buf, const uint8_t s, const char * const tag) {
    const render_cmd_clear_screen_s_t cmd = {
        s,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_clear_screen_s, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_present
=======================================
*/

typedef struct render_cmd_present_t {
    rhi_swap_interval_t swap_interval;
    DECL_RENDER_TAG
} render_cmd_present_t;

static inline bool render_cmd_buf_write_present(rb_cmd_buf_t * const cmd_buf, const rhi_swap_interval_t swap_interval, const char * const tag) {
    const render_cmd_present_t cmd = {
        swap_interval,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_present, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_screenshot
=======================================
*/

typedef struct render_cmd_screenshot_t {
    image_t * image;
    const mem_region_t mem_region;
    DECL_RENDER_TAG
} render_cmd_screenshot_t;

static inline bool render_cmd_buf_screenshot(rb_cmd_buf_t * const cmd_buf, image_t * const out_image, const mem_region_t mem_region, const char * const tag) {
    render_cmd_screenshot_t cmd = {
        out_image,
        mem_region,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_screenshot, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_mesh_data_layout
=======================================
*/

typedef struct render_cmd_create_mesh_data_layout_t {
    rhi_mesh_data_layout_t ** out;
    rhi_mesh_data_layout_desc_t layout_desc;
    const char * tag;
} render_cmd_create_mesh_data_layout_t;

static inline bool render_cmd_buf_write_create_mesh_data_layout(rb_cmd_buf_t * const cmd_buf, rhi_mesh_data_layout_t ** const out, const rhi_mesh_data_layout_desc_t layout_desc, const char * const tag) {
    const render_cmd_create_mesh_data_layout_t cmd = {
        out,
        layout_desc,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_mesh_data_layout, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_mesh_indirect
=======================================
*/

typedef struct render_cmd_create_mesh_indirect_t {
    rhi_mesh_t ** out;
    rhi_mesh_data_init_indirect_t init;
    const char * tag;
} render_cmd_create_mesh_indirect_t;

static inline bool render_cmd_buf_write_create_mesh_indirect(rb_cmd_buf_t * const cmd_buf, rhi_mesh_t ** out, const rhi_mesh_data_init_indirect_t init_data, const char * const tag) {
    const render_cmd_create_mesh_indirect_t cmd = {
        out,
        init_data,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_mesh_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_mesh_shared_vertex_data_indirect
=======================================
*/

typedef struct render_cmd_create_mesh_shared_vertex_data_indirect_t {
    rhi_mesh_t ** out;
    rhi_mesh_t * const * src;
    const_mem_region_t indices;
    int num_indices;
    const char * tag;
} render_cmd_create_mesh_shared_vertex_data_indirect_t;

static inline bool render_cmd_buf_write_create_mesh_shared_vertex_data_indirect(rb_cmd_buf_t * const cmd_buf, rhi_mesh_t ** out, rhi_mesh_t * const * const meshes_with_shared_vertices, const const_mem_region_t indices, int num_indices, const char * const tag) {
    const render_cmd_create_mesh_shared_vertex_data_indirect_t cmd = {
        out,
        meshes_with_shared_vertices,
        indices,
        num_indices,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_mesh_shared_vertex_data_indirect, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_program_from_binary_t
=======================================
*/

typedef struct render_cmd_create_program_from_binary_t {
    rhi_program_t ** out_program;
    rhi_error_t ** out_err;
    const_mem_region_t vert_program;
    const_mem_region_t frag_program;
    const char * tag;
} render_cmd_create_program_from_binary_t;

static inline bool render_cmd_buf_write_create_program_from_binary(rb_cmd_buf_t * const cmd_buf, rhi_program_t ** const out_program, rhi_error_t ** const out_err, const const_mem_region_t vert_program, const const_mem_region_t frag_program, const char * const tag) {
    const render_cmd_create_program_from_binary_t cmd = {
        out_program,
        out_err,
        vert_program,
        frag_program,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_program_from_binary, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_blend_state
=======================================
*/

typedef struct render_cmd_create_blend_state_t {
    rhi_blend_state_t ** out_blend_state;
    rhi_blend_state_desc_t blend_state_desc;
    const char * tag;
} render_cmd_create_blend_state_t;

static inline bool render_cmd_buf_write_create_blend_state(rb_cmd_buf_t * const cmd_buf, rhi_blend_state_t ** const out_blend_state, const rhi_blend_state_desc_t desc, const char * const tag) {
    const render_cmd_create_blend_state_t cmd = {
        out_blend_state,
        desc,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_blend_state, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_depth_stencil_state
=======================================
*/

typedef struct render_cmd_create_depth_stencil_state_t {
    rhi_depth_stencil_state_t ** out_depth_stencil_state;
    rhi_depth_stencil_state_desc_t depth_stencil_state_desc;
    const char * tag;
} render_cmd_create_depth_stencil_state_t;

static inline bool render_cmd_buf_write_create_depth_stencil_state(rb_cmd_buf_t * const cmd_buf, rhi_depth_stencil_state_t ** const out_depth_stencil_state, const rhi_depth_stencil_state_desc_t desc, const char * const tag) {
    const render_cmd_create_depth_stencil_state_t cmd = {
        out_depth_stencil_state,
        desc,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_depth_stencil_state, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_rasterizer_state
=======================================
*/

typedef struct render_cmd_create_rasterizer_state_t {
    rhi_rasterizer_state_t ** out_rasterizer_state;
    rhi_rasterizer_state_desc_t rasterizer_state_desc;
    const char * tag;
} render_cmd_create_rasterizer_state_t;

static inline bool render_cmd_buf_write_create_rasterizer_state(rb_cmd_buf_t * const cmd_buf, rhi_rasterizer_state_t ** const out_rasterizer_state, const rhi_rasterizer_state_desc_t desc, const char * const tag) {
    const render_cmd_create_rasterizer_state_t cmd = {
        out_rasterizer_state,
        desc,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_rasterizer_state, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_uniform_buffer
=======================================
*/

typedef struct render_cmd_create_uniform_buffer_t {
    rhi_uniform_buffer_t ** out_uniform_buffer;
    rhi_uniform_buffer_desc_t uniform_buffer_desc;
    const void * init_data;
    const char * tag;
} render_cmd_create_uniform_buffer_t;

static inline bool render_cmd_buf_write_create_uniform_buffer(rb_cmd_buf_t * const cmd_buf, rhi_uniform_buffer_t ** const out_uniform_buffer, const rhi_uniform_buffer_desc_t desc, const void * init_data, const char * const tag) {
    const render_cmd_create_uniform_buffer_t cmd = {
        out_uniform_buffer,
        desc,
        init_data,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_uniform_buffer, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_texture2d
=======================================
*/

typedef struct render_cmd_create_texture_2d_t {
    rhi_texture_t ** const out_texture;
    image_mips_t mipmaps;
    rhi_pixel_format_e format;
    rhi_usage_e usage;
    rhi_sampler_state_desc_t sampler_state;
    const char * tag;
} render_cmd_create_texture_2d_t;

static inline bool render_cmd_buf_write_create_texture_2d(rb_cmd_buf_t * const cmd_buf, rhi_texture_t ** const out_texture, const image_mips_t mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_state, const char * const tag) {
    const render_cmd_create_texture_2d_t cmd = {
        out_texture,
        mipmaps,
        format,
        usage,
        sampler_state,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_texture_2d, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_create_render_target
=======================================
*/

typedef struct render_cmd_create_render_target_t {
    rhi_render_target_t ** out_render_target;
    const char * tag;
} render_cmd_create_render_target_t;

static inline bool render_cmd_buf_write_create_render_target(rb_cmd_buf_t * const cmd_buf, rhi_render_target_t ** const out_render_target, const char * const tag) {
    const render_cmd_create_render_target_t cmd = {
        out_render_target,
        tag};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_create_render_target, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_callback
=======================================
*/

typedef struct render_cmd_callback_t {
    void (*callback)(rhi_device_t * device, void * arg);
    void * arg;
    DECL_RENDER_TAG
} render_cmd_callback_t;

static inline bool render_cmd_buf_write_callback(rb_cmd_buf_t * const cmd_buf, void (*const callback)(rhi_device_t * device, void * arg), void * const arg, const char * const tag) {
    const render_cmd_callback_t cmd = {
        callback,
        arg,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_callback, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

/*
=======================================
render_cmd_buf_write_breakpoint
=======================================
*/

typedef struct render_cmd_breakpoint_t {
    void * arg;
    DECL_RENDER_TAG
} render_cmd_breakpoint_t;

static inline bool render_cmd_buf_write_breakpoint(rb_cmd_buf_t * const cmd_buf, void * const arg, const char * const tag) {
    const render_cmd_breakpoint_t cmd = {
        arg,
        ASSIGN_RENDER_TAG};

    return render_cmd_buf_write_render_command(cmd_buf, render_cmd_breakpoint, &cmd, ALIGN_OF(cmd), sizeof(cmd));
}

#ifdef __cplusplus
}
#endif
