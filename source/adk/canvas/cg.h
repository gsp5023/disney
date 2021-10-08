/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
cg.h

2d canvas rendering
*/

#include "source/adk/runtime/runtime.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_thread.h"

/* ===========================================================================
 * Internal
 * ==========================================================================*/

#include "private/cg_gl.h"
#include "private/cg_math.h"
#include "private/cg_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    cg_default_max_states = 128,
    cg_default_max_tesselation_steps = 10,
};

/* ===========================================================================
 * STYLE
 * ==========================================================================*/

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_image_async_load_status_e {
    /// during image loading resource allocation failed due to insufficient memory.
    cg_image_async_load_out_of_memory = -7,
    /// The Ripcut server returned a proprietary error code which will be
    /// To to query this value call cg_context_get_image_ripcut_error_code().
    cg_image_async_load_ripcut_error = -6,
    /// could not load the image's file. (bad name, permission issue, bad path, other general read errors)
    cg_image_async_load_file_error = -5,
    /// unknown image format that M5 does not support.
    cg_image_async_load_unrecognized_image_format = -4,
    /// general http fetch error, most typically url does not exist or a 400/500 error.
    cg_image_async_load_http_fetch_error = -3,
    /// internal use signaling that the load was canceled by the user.
    cg_image_async_load_aborted = -2,
    /// image loading is complete and ready for use. -- at this point checks for the images dimension will return the images true size, and the image will display with draw operations.
    cg_image_async_load_complete = 0,
    /// image loading is in progress
    cg_image_async_load_pending = 1,
} cg_image_async_load_status_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
typedef enum cg_image_load_opts_e {
    /// default/no additional operations beyond default behavior for loading an image.
    cg_image_load_opts_none = 0 << 0,
    /// http logging will be performed when downloading this image.
    cg_image_load_opts_http_verbose = 1 << 0,
} cg_image_load_opts_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_image_animation_state_e {
    /// pauses animation, if resumed with `running` will continue from stopped frame.
    cg_image_animation_stopped = 0,
    /// resumes image animation, or indicates that the image is currently animating.
    cg_image_animation_running = 1,
    /// forcefully restarts the animation from the beginning.
    cg_image_animation_restart = 2,
} cg_image_animation_state_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_TRIM_START_NAMES(cg_rotation_)
typedef enum cg_rotation_e {
    cg_rotation_counter_clock_wise = 2,
    cg_rotation_clock_wise = 3,
} cg_rotation_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_clip_state_e {
    cg_clip_state_enabled = 2,
    cg_clip_state_disabled = 3,
} cg_clip_state_e;

/// Specifies where resources allocated when downloading an image should persist
FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_memory_region_e {
    /// This resource will live inside the `high` memory region, before entering low memory this resource _must_ be freed.
    cg_memory_region_high,
    /// This resource will live inside the `low` memory region.
    cg_memory_region_low,
    /// Resource will load using high memory, and transition to low memory when complete.
    /// If the resource is not complete prior to entering low memory it _must_ be freed.
    cg_memory_region_high_to_low,
} cg_memory_region_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
typedef struct cg_rads_t {
    float rads;
} cg_rads_t;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
typedef struct cg_rect_t {
    float x, y;
    float width, height;
} cg_rect_t;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
typedef struct cg_margins_t {
    float left, right;
    float top, bottom;
} cg_margins_t;

typedef struct cg_context_t cg_context_t;

typedef struct cg_statics_t {
    cg_context_t * ctx;
    struct {
        struct {
            int x0, y0, x1, y1;
        } rect;
        rhi_texture_t * surface;
    } captions_state;
} cg_statics_t;

typedef struct cg_async_image_t {
    cg_const_allocation_t resident_bytes;
    cg_allocation_t working_buffer;

    cg_context_t * cg;

    render_cmd_stream_t decode_cmd_stream;
    rb_fence_t upload_fence;

    bool decode_job_running;
    bool pending_destroy;
} cg_async_image_t;

typedef struct cg_image_gif_t {
    struct cg_image_t * prev_gif;
    struct cg_image_t * next_gif;

    cg_async_image_t async_image_data;

    uint32_t req_frame_count;
    uint32_t done_frame_count;
    uint32_t decoded_frame_count;
    microseconds_t decoded_frame_time;

    int32_t frame_remaining_duration_in_ms;
    milliseconds_t next_frame_duration;

} cg_image_gif_t;

typedef struct cg_image_bif_t {
    cg_async_image_t async_image_data;

    uint32_t req_frame_index;
    uint32_t decoded_frame_index;

} cg_image_bif_t;

typedef struct cg_image_t {
    cg_allocation_t pixel_buffer;

    cg_gl_texture_t cg_texture;
    cg_gl_texture_t cg_texture_mask;

    image_t image;
    image_t image_mask;

    uint32_t num_frames;
    cg_image_gif_t * gif;
    cg_image_bif_t * bif;

    cg_context_t * cg_ctx;

    struct image_load_data_t * load_user;
    cg_image_async_load_status_e status;
    cg_image_animation_state_e image_animation_state;
    int32_t ripcut_error_code;
} cg_image_t;

typedef cg_image_t cg_pattern_t;
typedef cg_image_t cg_gradient_t;

/* ------------------------------------------------------------------------- */

typedef enum cg_style_type_e { // drawable
    cg_style_type_color,
    cg_style_type_image,
    cg_style_type_pattern,
    cg_style_type_gradient,
} cg_style_type_e;

typedef struct cg_style_t {
    cg_style_type_e type;
    cg_color_t color;
    cg_image_t * source;
} cg_style_t;

/* ===========================================================================
 * STATE
 * ==========================================================================*/

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_blend_mode_e {
    cg_blend_mode_src_alpha_rgb,
    cg_blend_mode_src_alpha_all,
    cg_blend_mode_blit,
    cg_blend_mode_alpha_test,
} cg_blend_mode_e;

typedef struct cg_clip_state_t {
    float x0, y0;
    float x1, y1;
    cg_clip_state_e clip_state;
} cg_clip_state_t;

typedef struct cg_state_t {
    cg_affine_t transform;
    cg_color_t stroke_style;
    cg_color_t fill_style;
    cg_clip_state_t clip_state;

    float global_alpha;
    float line_width;
    float feather;
    const cg_image_t * image;
    float alpha_test_threshold;
    cg_blend_mode_e blend_mode;
} cg_state_t;

/* ------------------------------------------------------------------------- */

void cg_state_init(cg_state_t * const state);

typedef enum cg_rgb_fill_alpha_red_mode_e {
    cg_rgb_fill_alpha_red_enabled,
    cg_rgb_fill_alpha_red_disabled,
} cg_rgb_fill_alpha_red_mode_e;

void cg_select_blend_and_shader(cg_gl_state_t * const gl, const cg_state_t * const cg_state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const cg_rgb_fill_alpha_red_mode_e rgb_fill_alpha_mode);

/* ===========================================================================
 * SUBPATH
 * ==========================================================================*/

typedef struct cg_subpath_t {
    cg_vec2_t * array;
    bool closed;
    bool reverse;
} cg_subpath_t;

/* ===========================================================================
 * PATH
 * ==========================================================================*/

typedef struct cg_path_t {
    cg_state_t * state;
    cg_subpath_t * paths;
    cg_vec2_t last_point;
    cg_subpath_t cur_path;
    cg_vec2_t min_point;
    cg_vec2_t max_point;
} cg_path_t;

FFI_EXPORT FFI_NAME(CGPathOptions)
    FFI_TYPE_MODULE(canvas)
        FFI_FIELD_NAME(cg_path_options_none, None)
            FFI_FIELD_NAME(cg_path_options_concave, Concave)
                FFI_FIELD_NAME(cg_path_options_no_fethering, NoFethering) typedef enum cg_path_options_e {
                    cg_path_options_none = 0,
                    cg_path_options_concave = 1,
                    cg_path_options_no_fethering = 2
                } cg_path_options_e;

/* ===========================================================================
 * CANVAS
 * ==========================================================================*/

typedef struct mosaic_context_t mosaic_context_t;
struct cg_mem_block_t;

typedef struct cg_heap_t {
    heap_t heap;
    sb_mutex_t * mutex;
} cg_heap_t;

typedef enum cg_memory_mode_e {
    cg_memory_mode_high,
    cg_memory_mode_low,
} cg_memory_mode_e;

struct cg_context_t {
    thread_pool_t * thread_pool;
    mosaic_context_t * mosaic_ctx;
    cg_heap_t cg_heap_low;
    cg_heap_t cg_heap_high;
    cg_gl_state_t * gl;
    cg_path_t path;
    struct cg_mem_block_t * free_block_chain;

    uint32_t state_idx;
    cg_state_t * cur_state;
    cg_state_t * states;
    uint32_t max_states;

    int32_t width;
    int32_t height;
    float view_scale_x;
    float view_scale_y;

    cg_image_t * gif_head;
    cg_image_t * gif_tail;

    cg_memory_mode_e memory_mode;
    system_guard_page_mode_e guard_page_mode;
    mem_region_t high_mem_region;

    struct {
        uint32_t max_tesselation_steps;
    } quality;

    bool log_image_timing;
    bool using_video_texture;
    bool enable_punchthrough_blend_mode_fix;
};

/* ------------------------------------------------------------------------- */

typedef struct cg_context_memory_initializers_t {
    size_t high_mem_size;
    mem_region_t low_mem_region;
    mem_region_t font_scratchpad_mem;
    cg_memory_mode_e initial_memory_mode;
    system_guard_page_mode_e guard_page_mode;
} cg_context_memory_initializers_t;

typedef struct cg_context_dimension_t {
    int32_t width;
    int32_t height;
} cg_context_dimension_t;

typedef struct cg_context_dimensions_t {
    cg_context_dimension_t virtual_dims;
    cg_context_dimension_t display_dims;
    cg_context_dimension_t font_atlas_dims;
} cg_context_dimensions_t;

void cg_context_init(
    cg_context_t * const ctx,
    const cg_context_memory_initializers_t memory_initializers,
    const cg_context_dimensions_t dimensions,
    const uint32_t max_states,
    const uint32_t max_tesellation_steps,
    const bool enable_punchthrough_blend_mode_fix,
    cg_gl_state_t * const gl,
    thread_pool_t * const thread_pool,
    render_device_t * const render_device,
    const bool enable_image_time_logging,
    const char * const tag);

void cg_context_free(cg_context_t * const ctx, const char * const tag);

void cg_context_set_memory_mode(cg_context_t * const ctx, const cg_memory_mode_e memory_mode);

void cg_context_dump_heap_usage(cg_context_t * const ctx);

FFI_EXPORT FFI_PTR_NATIVE cg_context_t * cg_get_context();
FFI_EXPORT void cg_set_context(FFI_PTR_NATIVE cg_context_t * const ctx);

void cg_context_begin(const milliseconds_t delta_time);

void cg_context_end(const char * const tag);

FFI_EXPORT void cg_context_save();

FFI_EXPORT void cg_context_restore();

FFI_EXPORT void cg_context_identity();

FFI_EXPORT float cg_context_global_alpha();
FFI_EXPORT void cg_context_set_global_alpha(const float alpha);

FFI_EXPORT float cg_context_feather();
FFI_EXPORT void cg_context_set_feather(const float feather);

FFI_EXPORT float cg_context_get_alpha_test_threshold();
FFI_EXPORT void cg_context_set_alpha_test_threshold(const float threshold);

FFI_EXPORT cg_blend_mode_e cg_context_get_punchthrough_blend_mode();
FFI_EXPORT void cg_context_set_punchthrough_blend_mode(const cg_blend_mode_e blend_mode);

FFI_EXPORT void cg_context_set_line_width(const float width);

/* ------------------------------------------------------------------------- */

FFI_EXPORT void cg_context_begin_path(FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_end_path(FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_close_path(FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_move_to(const cg_vec2_t pos, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_line_to(const cg_vec2_t pos, FFI_MALLOC_TAG const char * const tag);

/* ------------------------------------------------------------------------- */

FFI_EXPORT void cg_context_rect(const cg_rect_t rect, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_rounded_rect(const cg_rect_t rect, const float radius, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_fill_rect(const cg_rect_t rect, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_stroke_rect(const cg_rect_t rect, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_clear_rect(const cg_rect_t rect, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_quad_bezier_to(const float cpx, const float cpy, const float x, const float y, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_arc(const cg_vec2_t pos, const float radius, const cg_rads_t start, const cg_rads_t end, const cg_rotation_e rotation, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_arc_to(const cg_vec2_t pos1, const cg_vec2_t pos2, const float radius, FFI_MALLOC_TAG const char * const tag);

/* ------------------------------------------------------------------------- */

FFI_EXPORT void cg_context_stroke_style(const cg_color_t color);

FFI_EXPORT void cg_context_fill_style(const cg_color_t color);

FFI_EXPORT void cg_context_fill_style_hex(const int32_t color);

FFI_EXPORT void cg_context_fill_style_image(const cg_color_t color, FFI_PTR_NATIVE const cg_image_t * const image);

FFI_EXPORT void cg_context_fill_style_image_hex(const int32_t color, FFI_PTR_NATIVE const cg_image_t * const image);

FFI_EXPORT void cg_context_fill(FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_fill_with_options(const cg_path_options_e options, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_stroke(FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT void cg_context_stroke_with_options(const cg_path_options_e options, FFI_MALLOC_TAG const char * const tag);

/* ------------------------------------------------------------------------- */

FFI_EXPORT void cg_context_rotate(const cg_rads_t angle);

FFI_EXPORT void cg_context_scale(const cg_vec2_t scale);

FFI_EXPORT void cg_context_translate(const cg_vec2_t translation);

/* ------------------------------------------------------------------------- */

FFI_EXPORT void cg_context_draw_image_rect(FFI_PTR_NATIVE const cg_image_t * const image, const cg_rect_t src, const cg_rect_t dst);

FFI_EXPORT void cg_context_draw_image_rect_alpha_mask(FFI_PTR_NATIVE const cg_image_t * const image, FFI_PTR_NATIVE const cg_image_t * const mask, const cg_rect_t src, const cg_rect_t dst);

//         dx,dy                 dx+dw,dy
//         +---+-------------------+---+
//         |   |     margin top    |   |
//         +---+-------------------+---+
//         | m |                   | m |
//         | a |                   | a |
//         | r |                   | r |
//         | g |                   | g |
//         | i |                   | i |
//         | n |                   | n |
//         |   |                   |   |
//         |   |                   | r |
//         | l |                   | i |
//         | e |                   | g |
//         | f |                   | h |
//         | t |                   | t |
//         +---+-------------------+---+
//         |   |   margin bottom   |   |
//         +---+-------------------+---+
//         dx,dy+dh              dx+dw,dy+dh
//
//  NOTE: 9slice image corners are drawn 1:1 (no texture scaling)
//        vertical / horizontal sides are stretched
FFI_EXPORT void cg_context_draw_image_9slice(FFI_PTR_NATIVE const cg_image_t * const image, const cg_margins_t margin, const cg_rect_t dst);

FFI_EXPORT void cg_context_draw_image(FFI_PTR_NATIVE const cg_image_t * const image, const cg_vec2_t pos);

FFI_EXPORT void cg_context_draw_image_scale(FFI_PTR_NATIVE const cg_image_t * const image, const cg_rect_t rect);

FFI_EXPORT void cg_context_blit_video_frame(const cg_rect_t rect);

/* ===========================================================================
 * IMAGE
 * ==========================================================================*/

FFI_EXPORT void cg_context_image_free(FFI_PTR_NATIVE cg_image_t * const image, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT FFI_PTR_NATIVE cg_image_t * cg_context_load_image_async(FFI_PTR_WASM const char * const file_location, const cg_memory_region_e memory_region, const cg_image_load_opts_e image_load_opts, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT cg_image_async_load_status_e cg_get_image_load_status(FFI_PTR_NATIVE const cg_image_t * const image);

FFI_EXPORT cg_rect_t cg_context_image_rect(FFI_PTR_NATIVE const cg_image_t * const image);

FFI_EXPORT void cg_context_set_image_frame_index(FFI_PTR_NATIVE cg_image_t * const cg_image, const uint32_t image_index);
FFI_EXPORT uint32_t cg_context_get_image_frame_count(FFI_PTR_NATIVE const cg_image_t * const cg_image);

FFI_EXPORT void cg_context_image_set_repeat(FFI_PTR_NATIVE cg_image_t * const image, const bool repeat_x, const bool repeat_y);

FFI_EXPORT void cg_context_set_image_animation_state(FFI_PTR_NATIVE cg_image_t * const image, const cg_image_animation_state_e image_animation_state);

/* ===========================================================================
 * IMAGE_DATA
 * ==========================================================================*/

#ifdef CG_TODO
void cg_context_image_data_free(cg_context_t * const ctx, cg_gl_texture_t * const image_data);

const cg_gl_texture_t * const cg_context_image_data(cg_context_t * const ctx, const int32_t w, const int32_t h);

const cg_gl_texture_t * const cg_context_get_image_data(cg_context_t * const ctx, const int32_t sx, const int32_t sy, const int32_t sw, const int32_t sh);

void cg_context_put_image_data(cg_context_t * const ctx, const cg_gl_texture_t * const image_data, const int32_t dx, const int32_t dy);
#endif

/* ===========================================================================
 * PATTERN
 * ==========================================================================*/

FFI_EXPORT FFI_PTR_NATIVE const cg_pattern_t * cg_context_pattern(FFI_PTR_NATIVE const cg_image_t * const image, const bool repeat_x, const bool repeat_y);

/* ===========================================================================
 * FONT
 * ==========================================================================*/

FFI_EXPORT
typedef struct cg_font_metrics_t {
    cg_rect_t bounds;
    float baseline;
} cg_font_metrics_t;

FFI_EXPORT
FFI_NAME(CGFontFillOptions)
FFI_TYPE_MODULE(canvas)
FFI_ENUM_BITFLAGS
FFI_FIELD_NAME(cg_font_fill_options_align_left, AlignLeft)
FFI_FIELD_NAME(cg_font_fill_options_align_center, AlignCenter)
FFI_FIELD_NAME(cg_font_fill_options_align_right, AlignRight)
FFI_FIELD_NAME(cg_font_fill_options_align_top, AlignTop)
FFI_FIELD_NAME(cg_font_fill_options_align_bottom, AlignBotton)
typedef enum cg_font_fill_options_e {
    cg_font_fill_options_align_left = 1 << 0,
    cg_font_fill_options_align_center = 1 << 1,
    cg_font_fill_options_align_right = 1 << 2,
    cg_font_fill_options_align_top = 1 << 3,
    cg_font_fill_options_align_bottom = 1 << 4,
} cg_font_fill_options_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
typedef enum cg_text_block_options_e {
    cg_text_block_no_options = 0,
    cg_text_block_align_line_left = 1 << 0,
    cg_text_block_align_line_center = 1 << 1,
    cg_text_block_align_line_right = 1 << 2,

    cg_text_block_align_text_top = 1 << 3,
    cg_text_block_align_text_center = 1 << 4,
    cg_text_block_align_text_bottom = 1 << 5,

    cg_text_block_allow_block_bounds_overflow = 1 << 6,
    // if this flag is passed the `extra_line_spacing` value is treated as a relative height offset
    // 1.0 -> normal line spacing, 1.2 -> 120% line height.. etc
    // if the flag is not passed in, then `extra_line_spacing` is treated as an absolute offset.
    // 0 -> normal line spacing, -20 -> 20 units of overlap, +20 -> 20 units of extra space
    cg_text_block_line_space_relative = 1 << 7,
} cg_text_block_options_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_font_async_load_status_e {
    cg_font_async_load_out_of_memory = -6,
    cg_font_async_load_file_error = -5,
    cg_font_async_load_font_parse_error = -4,
    cg_font_async_load_http_fetch_error = -3,
    cg_font_async_load_aborted = -2,

    cg_font_async_load_complete = 0,
    cg_font_async_load_pending = 1,
} cg_font_async_load_status_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
typedef enum cg_font_load_opts_e {
    cg_font_load_opts_none = 0 << 0,
    cg_font_load_opts_http_verbose = 1 << 0,
} cg_font_load_opts_e;

typedef struct cg_font_context_t {
    mosaic_context_t * mosaic_ctx;

    int32_t font_index;
    float size; // size when font was created
    float virtual_size; // size that will be used when rendering
    float baseline;
} cg_font_context_t;

FFI_EXPORT
typedef struct cg_text_block_page_offsets_t {
    uint32_t begin_offset;
    uint32_t end_offset;
} cg_text_block_page_offsets_t;

typedef struct cg_font_file_t cg_font_file_t;

FFI_EXPORT void cg_context_font_context_free(FFI_CAN_BE_NULL FFI_PTR_NATIVE cg_font_context_t * font, FFI_MALLOC_TAG const char * const tag);
FFI_EXPORT void cg_context_font_file_free(FFI_PTR_NATIVE cg_font_file_t * const font, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT FFI_PTR_NATIVE cg_font_file_t * cg_context_load_font_file_async(FFI_PTR_WASM const char * const filepath, const cg_memory_region_e memory_region, const cg_font_load_opts_e font_load_opts, FFI_MALLOC_TAG const char * const tag);
FFI_EXPORT FFI_PTR_NATIVE cg_font_context_t * cg_context_create_font_context(FFI_PTR_NATIVE cg_font_file_t * const cg_font, const float size, const int32_t tab_space_multiplier, FFI_MALLOC_TAG const char * const tag);

FFI_EXPORT cg_font_async_load_status_e cg_get_font_load_status(FFI_PTR_NATIVE const cg_font_file_t * const cg_font);
FFI_EXPORT int32_t cg_get_image_ripcut_error_code(FFI_PTR_NATIVE const cg_image_t * const image);

/// Sets the global (for this cg_context) missing glyph indicator, expects a single utf8 character for the indicator.
///
/// To reset to using the default value `missing_glyph_indocator` should be either a nullptr, or a zero length string e.g. NULL or ""
FFI_EXPORT void cg_context_set_global_missing_glyph_indicator(FFI_PTR_WASM const char * const missing_glyph_indicator);

/// Sets the missing glyph indicator for this font context, this has higher priority than `cg_context_set_global_missing_glyph_indicator`.
///
/// To reset to using the default value `missing_glyph_indocator` should be either a nullptr, or a zero length string e.g. NULL or ""
FFI_EXPORT void cg_context_set_font_context_missing_glyph_indicator(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, FFI_PTR_WASM const char * const missing_glyph_indicator);

FFI_EXPORT cg_font_metrics_t cg_context_text_measure(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, FFI_PTR_WASM const char * const text);
FFI_EXPORT cg_font_metrics_t cg_context_fill_text(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, const cg_vec2_t pos, FFI_PTR_WASM const char * const text);

FFI_EXPORT cg_font_metrics_t cg_context_fill_text_with_options(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, const cg_vec2_t pos, FFI_PTR_WASM const char * const text, const cg_font_fill_options_e options);

FFI_EXPORT cg_font_metrics_t cg_context_fill_text_block_with_options(
    FFI_PTR_NATIVE cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float text_scroll_offset,
    const float extra_line_spacing,
    FFI_PTR_WASM const char * const text,
    FFI_PTR_WASM FFI_CAN_BE_NULL const char * const optional_ellipses,
    const cg_text_block_options_e options);

FFI_EXPORT
/// Attempts to precache all glyphs in `characters` into the internal atlas
void cg_context_font_precache_glyphs(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, FFI_PTR_WASM const char * const characters);

FFI_EXPORT
/// Clears internal font atlas along with all associated glyph tracking meta data.
void cg_context_font_clear_glyph_cache();

cg_rect_t cg_context_get_text_block_extents(
    cg_font_context_t * const font_ctx,
    const float line_width,
    const float extra_line_spacing,
    const char * const text,
    const cg_text_block_options_e options);

FFI_EXPORT float cg_context_get_text_block_height(
    FFI_PTR_NATIVE cg_font_context_t * const font_ctx,
    const float line_width,
    const float extra_line_spacing,
    FFI_PTR_WASM const char * const text,
    const cg_text_block_options_e options);

FFI_EXPORT
cg_text_block_page_offsets_t cg_context_get_text_block_page_offsets(
    FFI_PTR_NATIVE cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    FFI_PTR_WASM const char * const text,
    const cg_text_block_options_e options);

FFI_EXPORT void cg_context_font_set_virtual_size(FFI_PTR_NATIVE cg_font_context_t * font_ctx, float size);

FFI_EXPORT void cg_context_set_clip_rect(const cg_rect_t rect);
FFI_EXPORT void cg_context_set_clip_state(const cg_clip_state_e clip_state);

#ifdef __cplusplus
}
#endif
