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
rhi.h

RENDER HARDWARE INTERFACE

API Abstraction for programmable GPUs, designed to wrap OpenGL, DirectX, Vulkan,
Metal, etc.
*/

#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_thread.h"
#include "source/adk/telemetry/telemetry.h"

typedef struct rhi_device_caps_t {
    int max_samples; /* max MSAA  */
    int max_anisotropy;
    int max_vertex_inputs;
    int max_fragment_inputs;
    int max_vertex_uniforms;
    int max_fragment_uniforms;
    int max_vertex_textures;
    int max_fragment_textures;
    int max_combined_textures;
    int max_texture_size;
    int max_device_threads;
    float tc_top_left;
} rhi_device_caps_t;

enum {
    rhi_program_input_max_textures = 4,
    rhi_program_input_max_uniforms = 10,
#if defined(_RPI) || defined(_STB_NATIVE)
    rhi_max_devices = 1,
    rhi_max_render_target_color_buffers = 1
#else
    rhi_max_devices = 8,
    rhi_max_render_target_color_buffers = 4
#endif
};

typedef enum rhi_color_write_mask_e {
    rhi_color_write_mask_r = 0x1,
    rhi_color_write_mask_g = 0x2,
    rhi_color_write_mask_b = 0x4,
    rhi_color_write_mask_a = 0x8,
    rhi_color_write_mask_rgb = 0x7,
    rhi_color_write_mask_all = 0xf,
    rhi_color_write_mask_none = 0
} rhi_color_write_mask_e;

typedef enum rhi_program_input_semantic_e {
    rhi_program_input_semantic_position0,
    rhi_program_input_semantic_color0,
    rhi_program_input_semantic_normal0,
    rhi_program_input_semantic_binormal0,
    rhi_program_input_semantic_tangent0,
    rhi_program_input_semantic_texcoord0,
    rhi_program_input_semantic_texcoord1,
    rhi_program_input_num_semantics
} rhi_program_input_semantic_e;

#include "source/shaders/shader_binding_locations.h"

typedef enum rhi_program_uniform_e {
    rhi_program_uniform_mvp = U_MVP_BINDING,
    rhi_program_uniform_viewport = U_VIEWPORT_BINDING,
    rhi_program_uniform_tex0 = U_TEX0_BINDING,
    rhi_program_uniform_tex1 = U_TEX1_BINDING,
    rhi_program_uniform_fill = U_FILL_BINDING,
    rhi_program_uniform_threshold = U_THRESHOLD_BINDING,
    rhi_program_uniform_rect_roundness = U_RECT_ROUNDNESS_BINDING,
    rhi_program_uniform_rect = U_RECT_BINDING,
    rhi_program_uniform_fade = U_FADE_BINDING,
    rhi_program_uniform_stroke_color = U_STROKE_COLOR_BINDING,
    rhi_program_uniform_stroke_size = U_STROKE_SIZE_BINDING,
    rhi_program_uniform_ltexsize = U_LTEXSIZE_BINDING,
    rhi_program_uniform_ctexsize = U_CTEXSIZE_BINDING,
    rhi_program_uniform_framesize = U_FRAMESIZE_BINDING,
    rhi_program_num_uniforms,
    rhi_program_num_textures = 2,
    FORCE_ENUM_SIGNED(rhi_program_uniform_e)
} rhi_program_uniform_e;

#include "source/shaders/shader_binding_locations_undefines.h"

typedef enum rhi_program_uniform_shader_stage_e {
    rhi_program_uniform_shader_stage_vertex,
    rhi_program_uniform_shader_stage_fragment,
} rhi_program_uniform_shader_stage_e;

static const char * const rhi_program_semantic_names[rhi_program_input_num_semantics] = {
    "in_pos0",
    "in_col0",
    "in_nml0",
    "in_bin0",
    "in_tan0",
    "in_tc0",
    "in_tc1"};

static const char * const rhi_program_uniform_names[rhi_program_num_uniforms] = {
    /* [rhi_program_uniform_mvp] = */ "u_mvp",
    /* [rhi_program_uniform_viewport] = */ "u_viewport",
    /* [rhi_program_uniform_tex0] = */ "u_tex0",
    /* [rhi_program_uniform_tex1] = */ "u_tex1",
    /* [rhi_program_uniform_fill] = */ "u_fill",
    /* [rhi_program_uniform_threshold] = */ "u_threshold",
    /* [rhi_program_uniform_rect_roundness] = */ "u_rect_roundness",
    /* [rhi_program_uniform_rect] = */ "u_rect",
    /* [rhi_program_uniform_fade] = */ "u_fade",
    /* [rhi_program_uniform_stroke_color] = */ "u_stroke_color",
    /* [rhi_program_uniform_stroke_size] = */ "u_stroke_size",
    /* [rhi_program_uniform_ltexsize] = */ "u_ltex_size",
    /* [rhi_program_uniform_ctexsize] = */ "u_ctex_size",
    /* [rhi_program_uniform_framesize] = */ "u_framesize",
};

static const enum rhi_program_uniform_shader_stage_e rhi_program_uniform_shader_stages[rhi_program_num_uniforms] = {
    /* [rhi_program_uniform_mvp] = */ rhi_program_uniform_shader_stage_vertex,
    /* [rhi_program_uniform_viewport] = */ rhi_program_uniform_shader_stage_vertex,
    /* [rhi_program_uniform_tex0] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_tex1] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_fill] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_threshold] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_rect_roundness] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_rect] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_fade] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_stroke_color] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_stroke_size] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_ltexsize] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_ctexsize] = */ rhi_program_uniform_shader_stage_fragment,
    /* [rhi_program_uniform_framesize] = */ rhi_program_uniform_shader_stage_fragment,
};

static const enum rhi_program_uniform_e rhi_program_texture_samplers[rhi_program_num_textures] = {
    rhi_program_uniform_tex0,
    rhi_program_uniform_tex1,
};

typedef enum rhi_vertex_element_format_e {
    rhi_vertex_element_format_float,
    rhi_vertex_element_format_unorm_byte,
} rhi_vertex_element_format_e;

typedef enum rhi_vertex_index_format_e {
    rhi_vertex_index_format_uint16,
    rhi_vertex_index_format_uint32
} rhi_vertex_index_format_e;

typedef enum rhi_usage_e {
    rhi_usage_default,
    rhi_usage_dynamic,
    rhi_usage_rendertarget
} rhi_usage_e;

typedef enum rhi_pixel_format_e {
    rhi_pixel_format_r8_unorm,
    rhi_pixel_format_ra8_unorm,
    rhi_pixel_format_rgb8_unorm,
    rhi_pixel_format_rgba8_unorm,
#if defined(_RPI) || defined(_STB_NATIVE)
    rhi_pixel_format_etc1,
#else
    rhi_pixel_format_bc1_unorm,
    rhi_pixel_format_bc3_unorm,
#endif
#if defined(_VADER) || defined(_LEIA)
    rhi_pixel_format_gnf,
    rhi_pixel_format_luma, // r
    rhi_pixel_format_chroma, // ra8
    rhi_pixel_format_luma_hdr10, // 32 bit packed texture
    rhi_pixel_format_chroma_hdr10, // 32 bit packed texture
#endif
    rhi_num_pixels_formats,
    rhi_framebuffer_format = rhi_pixel_format_rgba8_unorm,
    FORCE_ENUM_SIGNED(rhi_pixel_format_e)
} rhi_pixel_format_e;

typedef enum rhi_cull_face_e {
    rhi_cull_face_none,
    rhi_cull_face_front, // CCW when viewed from front
    rhi_cull_face_back // CW when viewed from front
} rhi_cull_face_e;

typedef enum rhi_blend_factor_e {
    rhi_blend_zero,
    rhi_blend_one,
    rhi_blend_src_color,
    rhi_blend_inv_src_color,
    rhi_blend_dst_color,
    rhi_blend_inv_dst_color,
    rhi_blend_src_alpha,
    rhi_blend_inv_src_alpha,
    rhi_blend_dst_alpha,
    rhi_blend_inv_dst_alpha,
    rhi_blend_constant_color,
    rhi_blend_constant_alpha
} rhi_blend_factor_e;

typedef enum rhi_compare_func_e {
    rhi_compare_always,
    rhi_compare_never,
    rhi_compare_equal,
    rhi_compare_less,
    rhi_compare_lequal,
    rhi_compare_gequal,
    rhi_compare_greater,
    rhi_compare_notequal
} rhi_compare_func_e;

typedef enum rhi_stencil_op_e {
    rhi_stencil_op_keep,
    rhi_stencil_op_zero,
    rhi_stencil_op_replace,
    rhi_stencil_op_incr,
    rhi_stencil_op_incr_wrap,
    rhi_stencil_op_decr,
    rhi_stencil_op_decr_wrap,
    rhi_stencil_op_invert
} rhi_stencil_op_e;

typedef enum rhi_blend_op_e {
    rhi_blend_op_add,
    rhi_blend_op_sub,
    rhi_blend_op_rev_sub
} rhi_blend_op_e;

typedef enum rhi_min_filter_mode_e {
    rhi_min_filter_nearest,
    rhi_min_filter_linear,
    rhi_min_filter_nearest_mipmap_nearest,
    rhi_min_filter_linear_mipmap_nearest,
    rhi_min_filter_nearest_mipmap_linear,
    rhi_min_filter_linear_mipmap_linear
} rhi_min_filter_mode_e;

typedef enum rhi_max_filter_mode_e {
    rhi_max_filter_nearest,
    rhi_max_filter_linear
} rhi_max_filter_mode_e;

typedef enum rhi_wrap_mode_e {
    rhi_wrap_mode_wrap,
    rhi_wrap_mode_mirror,
    rhi_wrap_mode_clamp_to_edge
} rhi_wrap_mode_e;

// A swap interval will force a wait until `interval` number of screen updates have occurred.
// An interval of 0 indicates no waiting
// An interval of 1 is the device's native refresh rate (if the device is 60hz, this means we render at 60hz)
// An interval of 2 indicates 1/2 the device's refresh rate (30hz on a 60hz, 72 on a 144hz)
// For backends that support it -1 will allow the driver to swap immediately, even if a frame is already being presented to screen (this can cause tearing)
typedef struct rhi_swap_interval_t {
    int interval;
} rhi_swap_interval_t;

typedef enum rhi_draw_mode_e {
    rhi_triangles,
    rhi_triangle_strip,
    rhi_triangle_fan,
    rhi_lines,
    rhi_line_strip
} rhi_draw_mode_e;

typedef struct rhi_vertex_element_desc_t {
    int offset;
    int count;
    rhi_vertex_element_format_e format;
    rhi_program_input_semantic_e semantic;
} rhi_vertex_element_desc_t;

typedef struct rhi_vertex_layout_desc_t {
    const rhi_vertex_element_desc_t * elements;
    int num_elements;
    int stride;
    rhi_usage_e usage;
} rhi_vertex_layout_desc_t;

typedef struct rhi_index_layout_desc_t {
    rhi_vertex_index_format_e format;
    rhi_usage_e usage;
} rhi_index_layout_desc_t;

typedef struct rhi_mesh_data_layout_desc_t {
    const rhi_index_layout_desc_t * indices;
    const rhi_vertex_layout_desc_t * channels;
    int num_channels;
} rhi_mesh_data_layout_desc_t;

struct rhi_mesh_data_layout_t;
typedef struct rhi_mesh_data_init_indirect_t {
    struct rhi_mesh_data_layout_t * const * layout;
    const_mem_region_t indices;
    const const_mem_region_t * channels;
    int num_channels;
    int num_indices;
} rhi_mesh_data_init_indirect_t;

typedef struct rhi_blend_state_desc_t {
    bool blend_enable;
    rhi_blend_factor_e src_blend;
    rhi_blend_factor_e dst_blend;
    rhi_blend_op_e blend_op;
    rhi_blend_factor_e src_blend_alpha;
    rhi_blend_factor_e dst_blend_alpha;
    rhi_blend_op_e blend_op_alpha;
    float blend_color[4];
    rhi_color_write_mask_e color_write_mask;
} rhi_blend_state_desc_t;

typedef struct rhi_stencil_op_desc_t {
    rhi_stencil_op_e stencil_fail_op;
    rhi_stencil_op_e depth_fail_op;
    rhi_stencil_op_e stencil_depth_pass_op;
    rhi_compare_func_e stencil_test_func;
} rhi_stencil_op_desc_t;

static inline bool rhi_stencil_op_desc_equals(const rhi_stencil_op_desc_t * a, const rhi_stencil_op_desc_t * b) {
    return (a->stencil_fail_op == b->stencil_fail_op) && (a->depth_fail_op == b->depth_fail_op) && (a->stencil_depth_pass_op == b->stencil_depth_pass_op) && (a->stencil_test_func == b->stencil_test_func);
}

typedef struct rhi_depth_stencil_state_desc_t {
    bool depth_test;
    bool depth_write_mask;
    rhi_compare_func_e depth_test_func;
    bool stencil_test;
    uint8_t stencil_read_mask;
    uint8_t stencil_write_mask;
    rhi_stencil_op_desc_t stencil_front;
    rhi_stencil_op_desc_t stencil_back;
} rhi_depth_stencil_state_desc_t;

typedef struct rhi_rasterizer_state_desc_t {
    rhi_cull_face_e cull_mode;
    bool scissor_test;
} rhi_rasterizer_state_desc_t;

typedef struct rhi_uniform_buffer_desc_t {
    int size;
    rhi_usage_e usage;
} rhi_uniform_buffer_desc_t;

typedef struct rhi_sampler_state_desc_t {
    rhi_min_filter_mode_e min_filter;
    rhi_max_filter_mode_e max_filter;
    rhi_wrap_mode_e u_wrap_mode;
    rhi_wrap_mode_e v_wrap_mode;
    rhi_wrap_mode_e w_wrap_mode;
    float border_color[4];
    int max_anisotropy;
} rhi_sampler_state_desc_t;

typedef struct rhi_texture_bindings_indirect_t {
    struct rhi_texture_t * const * textures[rhi_program_num_textures];
    int num_textures;
} rhi_texture_bindings_indirect_t;

typedef struct rhi_uniform_binding_indirect_t {
    struct rhi_uniform_buffer_t * const * buffer;
    rhi_program_uniform_e index;
} rhi_uniform_binding_indirect_t;

typedef struct rhi_render_target_color_buffer_indirect_t {
    struct rhi_texture_t * const * buffer;
    int index;
} rhi_render_target_color_buffer_indirect_t;

typedef struct rhi_draw_params_indirect_t {
    struct rhi_mesh_t * const * const * mesh_list;
    const int * idx_ofs;
    const int * elm_counts;
    const uint32_t * hashes;
    int num_meshes;
    rhi_draw_mode_e mode;
} rhi_draw_params_indirect_t;

typedef struct rhi_counters_t {
    uint32_t num_tris;
    uint32_t num_draw_calls;
    uint32_t num_api_calls;
    uint32_t num_upload_verts;
    uint32_t upload_vert_size;
    uint32_t num_upload_indices;
    uint32_t upload_index_size;
    uint32_t num_upload_uniforms;
    uint32_t upload_uniform_size;
    uint32_t num_upload_textures;
    uint32_t upload_texture_size;
} rhi_counters_t;

static inline rhi_counters_t rhi_add_counters(const rhi_counters_t * a, const rhi_counters_t * b) {
    struct rhi_counters_t counter = {
        /*.num_tris =*/a->num_tris + b->num_tris,
        /*.num_draw_calls =*/a->num_draw_calls + b->num_draw_calls,
        /*.num_api_calls =*/a->num_api_calls + b->num_api_calls,
        /*.num_upload_verts =*/a->num_upload_verts + b->num_upload_verts,
        /*.upload_vert_size =*/a->upload_vert_size + b->upload_vert_size,
        /*.num_upload_indices =*/a->num_upload_indices + b->num_upload_indices,
        /*.upload_index_size =*/a->upload_index_size + b->upload_index_size,
        /*.num_upload_uniforms =*/a->num_upload_uniforms + b->num_upload_uniforms,
        /*.upload_uniform_size =*/a->upload_uniform_size + b->upload_uniform_size,
        /*.num_upload_textures =*/a->num_upload_textures + b->num_upload_textures,
        /*.upload_texture_size =*/a->upload_texture_size + b->upload_texture_size};
    return counter;
}

/*
===============================================================================
rhi_resource_t

Contains refcount, all rhi "objects" contain this.

RHI resource references can be added from any thread, but release
can only occur using render_cmd_buf_rhi_resource_release
===============================================================================
*/

struct rhi_resource_t;

/// Function pointers used to retrieve reference counts
typedef struct rhi_resource_vtable_t {
    /// Adds a reference
    ///
    ///  * `resource`: resource to be added
    ///
    /// returns previous reference count
    int (*add_ref)(struct rhi_resource_t * const resource);

    /// Release a reference
    ///
    /// * `resource`: resource to be added
    /// * `tag`: object tag
    ///
    /// returns previous reference count
    int (*release)(struct rhi_resource_t * const resource, const char * const tag);
} rhi_resource_vtable_t;

/// Rendering Hardware Interface resource
typedef struct rhi_resource_t {
    /// Function pointers used to retrieve reference counts
    const rhi_resource_vtable_t * vtable;
    /// Reference count
    sb_atomic_int32_t ref_count;
    /// Instance ID
    int instance_id;
    /// Renderer tag
    const char * tag;
} rhi_resource_t;

static inline int rhi_add_ref(rhi_resource_t * const resource) {
    return resource->vtable->add_ref(resource);
}

/*
=======================================
rhi_resources_not_equal

Input:
	a: rhi_resource_t *
	b: rhi_resource_t *
	b_instance_id: last instance id of b when it was valid

Constraints:

	a OR b OR both may be NULL.
	a AND b are non-null in which case:
		a != b AND a OR b may point to a released object that may no longer be accessed (this is safe)
		a == b AND do not point to an invalid object

return true if two resources are not equal
=======================================
*/

static inline bool rhi_resources_not_equal(const rhi_resource_t * const a, const rhi_resource_t * const b, const int b_instance_id) {
    return a && ((a != b) || (a->instance_id != b_instance_id));
}

/*
=======================================
rhi_resources_equal

Input:
	a: rhi_resource_t *
	b: rhi_resource_t *
	b_instance_id: last instance id of b when it was valid

Constraints:

	a OR b OR both may be NULL.
	a AND b are non-null in which case:
		a != b AND a OR b may point to a released object that may no longer be accessed (this is safe)
		a == b AND do not point to an invalid object

return true if two resources are equal
=======================================
*/

static inline bool rhi_resources_equal(const rhi_resource_t * const a, const rhi_resource_t * const b, const int b_instance_id) {
    return (!a && !b && !b_instance_id) || !rhi_resources_not_equal(a, b, b_instance_id);
}

/*
===============================================================================
rhi_program_t

Represents a compiled shader program.
===============================================================================
*/

typedef struct rhi_program_t {
    rhi_resource_t resource;
} rhi_program_t;

/*
===============================================================================
rhi_mesh_t

A mesh contains vertex and index data.
===============================================================================
*/

typedef struct rhi_mesh_t {
    rhi_resource_t resource;
} rhi_mesh_t;

/*
===============================================================================
rhi_mesh_data_layout_t

Describes the layout of vertex data inside a mesh
===============================================================================
*/

typedef struct rhi_mesh_data_layout_t {
    rhi_resource_t resource;
} rhi_mesh_data_layout_t;

/*
===============================================================================
rhi_texture_t

Texture data that can be bound to a shader sampler
===============================================================================
*/

typedef struct rhi_texture_t {
    rhi_resource_t resource;
} rhi_texture_t;

/*
===============================================================================
rhi_depth_stencil_state_t

Contains depth test and stencil test states. Can be applied to a device.
===============================================================================
*/

typedef struct rhi_depth_stencil_state_t {
    rhi_resource_t resource;
} rhi_depth_stencil_state_t;

/*
===============================================================================
rhi_blend_state_t

Contains framebuffer blending states. Can be applied to a device.
===============================================================================
*/

typedef struct rhi_blend_state_t {
    rhi_resource_t resource;
} rhi_blend_state_t;

/*
===============================================================================
rhi_rasterizer_state_t

Contains culling and scissor test states. Can be applied to a device.
===============================================================================
*/

typedef struct rhi_rasterizer_state_t {
    rhi_resource_t resource;
} rhi_rasterizer_state_t;

/*
===============================================================================
rhi_uniform_buffer_t

Contains shader uniforms
===============================================================================
*/

typedef struct rhi_uniform_buffer_t {
    rhi_resource_t resource;
} rhi_uniform_buffer_t;

/*
===============================================================================
rhi_render_target_t

Texture resource that can be rendered into.
===============================================================================
*/

typedef struct rhi_render_target_t {
    rhi_resource_t resource;
} rhi_render_target_t;

/*
===============================================================================
rhi_error_t

Error object.
===============================================================================
*/

struct rhi_error_t;
typedef struct rhi_error_vtable_t {
    const char * (*get_message)(const struct rhi_error_t * const msg);
} rhi_error_vtable_t;

typedef struct rhi_error_t {
    /// RHI resource
    rhi_resource_t resource;
    /// RHI functions
    const rhi_error_vtable_t * vtable;
} rhi_error_t;

static inline const char * rhi_get_error_message(const rhi_error_t * const err) {
    return err->vtable->get_message(err);
}

static inline void rhi_error_release(rhi_error_t * const err, const char * const tag) {
    err->resource.vtable->release(&err->resource, tag);
}

/*
===============================================================================
rhi_device_t

Devices contain a rendering context and state
===============================================================================
*/

struct rhi_device_vtable_t;

/// The Rendering Hardware Interface device
typedef struct rhi_device_t {
    /// RHI resource associated with that error
    rhi_resource_t resource;
    /// RHI functions
    const struct rhi_device_vtable_t * vtable;
} rhi_device_t;

/*
===============================================================================
rhi_api_t

A specific API backend, can created rendering devices
===============================================================================
*/

struct sb_window_t;
/// function pointers to initialize the RHI system
typedef struct rhi_api_vtable_t {
    /// Initialize the RHI system
    ///
    /// * `memory_block`: Memory to use
    /// * `guard_page_mode`: Guard page mode
    /// * `out_error`: Return variable for any errors
    ///
    /// Returns true  on success
    bool (*init)(const mem_region_t memory_block, const system_guard_page_mode_e guard_page_mode, rhi_error_t ** const out_error);
    /// Create a new RHI device
    ///
    /// * `window`: Window in which to create the device
    /// * `out_error`: Return variable for any errors
    /// * `tag`: Used to tag the device object
    ///
    /// Returns device
    rhi_device_t * (*create_device)(struct sb_window_t * const window, rhi_error_t ** const out_error, const char * const tag);
} rhi_api_vtable_t;

/// The Rendering Hardware Interface
typedef struct rhi_api_t {
    /// The RHI resource
    rhi_resource_t resource;
    /// Initialization functions
    const rhi_api_vtable_t * vtable;
} rhi_api_t;

/*
===============================================================================
rhi_init

Initialize the specified RHI api. To shutdown the API call rhi_release(). When
all RHI resources are released the API will be closed.

Inputs:
	api: pointer to the API to initialize

Returns:
	If an error occurs, returns false and out_error will be set to the specific error.
	Otherwise returns true.
===============================================================================
*/

static inline bool rhi_init(rhi_api_t * const api, const mem_region_t memory_block, const system_guard_page_mode_e guard_page_mode, rhi_error_t ** const out_error) {
    return api->vtable->init(memory_block, guard_page_mode, out_error);
}

/*
===============================================================================
rhi_create_device

Creates a device capable of rendering into the specified window

Inputs:
	window: pointer to native window the device will render into.

Returns:
	The created device, if successful. If an error occurs out_error
	will contain an error object and the function will return NULL.
===============================================================================
*/

static inline rhi_device_t * rhi_create_device(rhi_api_t * const api, struct sb_window_t * const window, rhi_error_t ** const out_error, const char * const tag) {
    return api->vtable->create_device(window, out_error, tag);
}

#ifdef __cplusplus
}
#endif
