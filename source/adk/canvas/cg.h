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

#include "source/adk/manifest/manifest.h"
#include "source/adk/runtime/memory.h"
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
    cg_default_max_text_mesh_cache_size = 64,
    cg_default_max_tesselation_steps = 10,
    cg_gl_default_max_verts_per_vertex_bank = 16 * 1024,
    cg_gl_default_vertex_banks = 2,
    cg_gl_default_num_meshes = 64,
    cg_gzip_default_working_space = 8 * 1024, // sizeof(struct inflate_state) -- this is the only allocation that will be performed, and the struct is in an internal header.
};

/* ===========================================================================
 * STYLE
 * ==========================================================================*/

/// Various states of image loading, see individual fields for details.
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
    rb_fence_t initial_upload_fence;
    rb_fence_t recurrent_upload_fence;

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

typedef struct cg_box_t {
    cg_vec2_t centerpoint;
    cg_vec2_t half_dim;
} cg_box_t;

typedef struct cg_sdf_rect_uniforms_t {
    cg_box_t box;
    float roundness;
    float fade;
} cg_sdf_rect_uniforms_t;

typedef struct cg_sdf_rect_border_uniforms_t {
    cg_sdf_rect_uniforms_t sdf_rect_uniforms;
    cg_color_t stroke_color;
    float stroke_size;
} cg_sdf_rect_border_uniforms_t;

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

FFI_EXPORT
FFI_NAME(CGPathOptions)
FFI_TYPE_MODULE(canvas)
FFI_FIELD_NAME(cg_path_options_none, None)
FFI_FIELD_NAME(cg_path_options_concave, Concave)
FFI_FIELD_NAME(cg_path_options_no_fethering, NoFethering)
typedef enum cg_path_options_e {
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

    int32_t width;
    int32_t height;
    float view_scale_x;
    float view_scale_y;

    cg_image_t * gif_head;
    cg_image_t * gif_tail;

    cg_memory_mode_e memory_mode;
    system_guard_page_mode_e guard_page_mode;
    mem_region_t high_mem_region;

    runtime_configuration_canvas_t config;

    bool log_image_timing;
    bool using_video_texture;
    uint32_t clear_color;
};

/* ------------------------------------------------------------------------- */

typedef struct cg_context_memory_initializers_t {
    size_t high_mem_size;
    mem_region_t low_mem_region;
    mem_region_t font_scratchpad_mem;
    cg_memory_mode_e initial_memory_mode;
    system_guard_page_mode_e guard_page_mode;
} cg_context_memory_initializers_t;

typedef enum cg_context_display_size_override_e {
    cg_context_display_size_default,
    cg_context_display_size_720p,
} cg_context_display_size_override_e;

typedef enum cg_context_image_time_logging_e {
    cg_context_image_time_logging_disabled,
    cg_context_image_time_logging_enabled,
} cg_context_image_time_logging_e;

void cg_context_init(
    cg_context_t * const ctx,
    cg_gl_state_t * const gl,
    thread_pool_t * const thread_pool,
    render_device_t * const render_device,
    const cg_context_memory_initializers_t memory_initializers,
    const sb_display_mode_t display_mode,
    const runtime_configuration_canvas_t canvas_config,
    const cg_context_display_size_override_e display_size_override,
    const cg_context_image_time_logging_e image_logging,
    const char * const tag);

void cg_context_free(cg_context_t * const ctx, const char * const tag);

void cg_context_set_memory_mode(cg_context_t * const ctx, const cg_memory_mode_e memory_mode);

void cg_context_dump_heap_usage(cg_context_t * const ctx);

heap_metrics_t cg_context_get_heap_metrics_high(cg_context_t * const ctx);

heap_metrics_t cg_context_get_heap_metrics_low(cg_context_t * const ctx);

FFI_EXPORT FFI_PTR_NATIVE cg_context_t * cg_get_context();
FFI_EXPORT void cg_set_context(FFI_PTR_NATIVE cg_context_t * const ctx);

void cg_context_begin(const milliseconds_t delta_time);

void cg_context_end(const char * const tag);

/// Pushes the current state onto a stack.
/// The state consists of:
///    current transform (rotation, translation, scale)
///    current fill style
///    current alpha
///    current blend mode
///    current clip state (clip rect and enabled/disabled)
///    current feather
///    current fill image (cg_context_fill_style_image_*)
///    current global alpha
FFI_EXPORT void cg_context_save();

/// Pops the state stack, and applies current top most state.
FFI_EXPORT void cg_context_restore();

/// Resets the states transform to identity (0 translation, 0 rotation, 1 scale)
FFI_EXPORT void cg_context_identity();

/// Returns the currently set global alpha of this state
FFI_EXPORT float cg_context_global_alpha();
/// Sets the global alpha of this canvas state, aall subsequent draws will have at most this opacity.
FFI_EXPORT void cg_context_set_global_alpha(const float alpha);

FFI_EXPORT float cg_context_feather();
FFI_EXPORT void cg_context_set_feather(const float feather);

/// Sets the clear color for the _next_ drawn frame. expected format is: 0xRR GG BB AA
/// Unless otherwise intended, AA should always be FF.
FFI_EXPORT void cg_context_set_clear_color(const uint32_t color);

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

/// Applies a rotation to the current canvas state
/// Positive rotations are counter clock wise.
/// Subsequent draws will be rotated by the current total amount of rotation.
/// reset by cg_context_identity().
FFI_EXPORT void cg_context_rotate(const cg_rads_t angle);

/// Applies a scaling factor to the current canvas state, both dimensions should be scaled by the same value.
/// Subsequent draws will be scaled by the current total scaling factor.
/// reset by cg_context_identity().
FFI_EXPORT void cg_context_scale(const cg_vec2_t scale);

/// Applies a translation to the current canvas state
/// All subsequent draws will be offset by the current total translation. effectively translating/transposing them by this combined value.
/// reset by cg_context_identity()
///
/// Example:
/// cg_context_identity();
/// cg_context_draw_image(img, cg_vec2_t{x:0,y:0}); // image top left will be drawn at x = 0, y = 0
/// cg_context_translate(cg_vec2_t{x:100, y:200});
/// cg_context_translate(cg_vec2_t{x:-10, y:50});
/// cg_context_draw_image(img, cg_vec2_t{x:10,y:10}); // image top left will be drawn at x = 100, y = 260
FFI_EXPORT void cg_context_translate(const cg_vec2_t translation);

/* ------------------------------------------------------------------------- */

/// When using sdf fill functions, will determine the tiling mode of the provided image.
FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_image_tiling_e {
    /// image repeats will behave as if they have a view onto a background that starts at y=0,x=0 and repeats.
    /// this is the legacy fill_style_image behavior
    cg_image_tiling_absolute,
    /// image repeats will have their tiling start at the dst's .x and .y
    cg_image_tiling_relative,
    /// image will be stretched to fit the bounding rect provided by dst
    /// if src.x = src.y = 0 and src.width/src.height are the same as the images dimensions then
    /// equivalent to cg_context_sdf_draw_image_rect_rounded
    cg_image_tiling_stretch,
} cg_image_tiling_e;

/// Configures how to draw using cg_context_sdf_*_rect calls.
FFI_EXPORT
FFI_TYPE_MODULE(canvas)
typedef struct cg_sdf_rect_params_t {
    /// specifies the rounding of the rect, range is clamped on [0, min(dst.width,dst.height)/2]
    /// maximally rounded squares will be a circle, maximally rounded rects will be a capsule
    float roundness;
    /// applies a fade at the edges of the border.. support is _slightly_ off and may not make perfect sense
    /// but for small values maintains a reasonble fade
    /// fading is internally smoothstepped instead of linear for better looks.
    /// note: fade + border is not currently supported and in non debug builds looks poor. only use one or the other if either are used.
    /// valid values are [0,INF) but only values roughly near the max(dst.width,dst.height) make sense visually.
    float fade;
    /// if border_width is non zero, the color of the border to be applied (independent of fill_style's color)
    cg_color_t border_color;
    /// if non zero, species with the width of the border in pixels
    /// this value does not currently get scaled by the canvas's current scale.
    float border_width;
} cg_sdf_rect_params_t;

/// Draw an image `image` with a provided image subrect `src` at screen coodinates (and scale) at `dst`
/// Allows drawing a sub region of the provided texsture (in pixel coordinates) most useful for atlases/sprite sheets/other composite images.
FFI_EXPORT void cg_context_draw_image_rect(FFI_PTR_NATIVE const cg_image_t * const image, const cg_rect_t src, const cg_rect_t dst);

/// Draw an image with optional rounding/border/fade as indicated by cg_sdf_rect_params_t
/// This eliminates the need for image masking if you only need rounding at the corners of an image.
/// Supports drawing sub regions of an image (by changing src)
FFI_EXPORT void cg_context_sdf_draw_image_rect_rounded(FFI_PTR_NATIVE const cg_image_t * const image, const cg_rect_t src, const cg_rect_t dst, const cg_sdf_rect_params_t draw_params);

/// Tile an image with optional rounding/border/fade as indicated by cg_sdf_rect_params_t
/// Replacement function for drawing a rect via paths.
FFI_EXPORT void cg_context_sdf_fill_image_rect_rounded(FFI_PTR_NATIVE const cg_image_t * const image, const cg_rect_t dst, const cg_sdf_rect_params_t draw_params, const cg_image_tiling_e tiling);

/// Draw an non textured rectangle with optional rounding/border/fade as indicated by cg_sdf_rect_params_t
/// Replacement function for drawing a non textured rect via paths.
FFI_EXPORT void cg_context_sdf_fill_rect_rounded(const cg_rect_t rect, const cg_sdf_rect_params_t draw_params);

/// Similar to `cg_context_draw_image_rect` but takes an additional image `mask` whose alpha channel is used as a mask for `image`
/// There is a marginal performance impact on certain devices due to sampling more textures.
FFI_EXPORT void cg_context_draw_image_rect_alpha_mask(FFI_PTR_NATIVE const cg_image_t * const image, FFI_PTR_NATIVE const cg_image_t * const mask, const cg_rect_t src, const cg_rect_t dst);

/// Draw an image 9 sliced as the ascii art demos:
///
///         dx,dy                 dx+dw,dy
///         +---+-------------------+---+
///         |   |     margin top    |   |
///         +---+-------------------+---+
///         | m |                   | m |
///         | a |                   | a |
///         | r |                   | r |
///         | g |                   | g |
///         | i |                   | i |
///         | n |                   | n |
///         |   |                   |   |
///         |   |                   | r |
///         | l |                   | i |
///         | e |                   | g |
///         | f |                   | h |
///         | t |                   | t |
///         +---+-------------------+---+
///         |   |   margin bottom   |   |
///         +---+-------------------+---+
///         dx,dy+dh              dx+dw,dy+dh
///
///  NOTE: 9slice image corners are drawn 1:1 (no texture scaling)
///        vertical / horizontal sides are stretched
FFI_EXPORT void cg_context_draw_image_9slice(FFI_PTR_NATIVE const cg_image_t * const image, const cg_margins_t margin, const cg_rect_t dst);

/// Draw an image at position `pos`
/// Drawn image dimensions will be equal to the dimensions of the image. To scale the image use `cg_context_draw_image_scale`
FFI_EXPORT void cg_context_draw_image(FFI_PTR_NATIVE const cg_image_t * const image, const cg_vec2_t pos);

/// Draw an image at position `rect.x, rect.y` scaled by the ratio of the images width/height compared to rect.width, rect.height
/// If `rect` .width is 2x the with of the `image` then it will be scaled by 2x in the x dimension.
FFI_EXPORT void cg_context_draw_image_scale(FFI_PTR_NATIVE const cg_image_t * const image, const cg_rect_t rect);

/// When playing video, call to blit the video frame from NVE
FFI_EXPORT void cg_context_blit_video_frame(const cg_rect_t rect);

/* ===========================================================================
 * IMAGE
 * ==========================================================================*/

/// Free the specified iamge `image` deallocating all associated resources.
/// Note: if the image is pending an operation this may be slightly delayed.
FFI_EXPORT void cg_context_image_free(FFI_PTR_NATIVE cg_image_t * const image, FFI_MALLOC_TAG const char * const tag);

/// Load an image asynchronously (non blocking).
/// Image provided may be local to file, or remote (via http/https)
/// Image will live in the relative heap provided by `memory_mode`
/// Images may be used for drawing immediately after an async load is enqueued, but will render as a single white pixel until completely loaded.
///
/// Note: if an image is loaded into `high_to_low` and is not complete or loaded into `high` the image _must_ be freed before entering low memory mode.
FFI_EXPORT FFI_PTR_NATIVE cg_image_t * cg_context_load_image_async(FFI_PTR_WASM const char * const file_location, const cg_memory_region_e memory_region, const cg_image_load_opts_e image_load_opts, FFI_MALLOC_TAG const char * const tag);

/// Get the current load status of the image
/// See cg_image_async_load_status_e for all states
FFI_EXPORT cg_image_async_load_status_e cg_get_image_load_status(FFI_PTR_NATIVE const cg_image_t * const image);

/// If there was a ripcut error when loading an image, return that value. Please see ripcut specs to determine values.
/// If there is no ripcut error returns 0.
FFI_EXPORT int32_t cg_get_image_ripcut_error_code(FFI_PTR_NATIVE const cg_image_t * const image);

/// Gets the bounding rect of the image.
/// Useful for determining how large an image is for layouts, or for calculating appropriate bounds for texture atlas coordinates.
FFI_EXPORT cg_rect_t cg_context_image_rect(FFI_PTR_NATIVE const cg_image_t * const image);

/// If the image is a `bif` will enqueue a decode of the provided frame's index.
/// Will internally clamp to valid ranges if an invalid index is provided.
FFI_EXPORT void cg_context_set_image_frame_index(FFI_PTR_NATIVE cg_image_t * const cg_image, const uint32_t image_index);

/// Get the number of frames in this bif.
///
/// Example:
/// let frame_count = cg_context_get_image_frame_count(img); // pretend frame count is 9
/// // we're trying to sample the preview image at 30% of the way through the stream
/// let frame_ind = (frame_count as f32 / 0.3) as u32;
/// cg_context_set_image_frame_index(img, frame_ind);
FFI_EXPORT uint32_t cg_context_get_image_frame_count(FFI_PTR_NATIVE const cg_image_t * const cg_image);

/// Sets the repeat mode for the provided image (Assuming the image does not fill the provided region)
/// By default images will repeat.
FFI_EXPORT void cg_context_image_set_repeat(FFI_PTR_NATIVE cg_image_t * const image, const bool repeat_x, const bool repeat_y);

/// For images that support animations (gifs, others) determines if the animation is playing, stopped, or should restart.
///
/// Notes:
/// Images will maintain whatever their last applicable state is regardless if they're being drawn.
/// A stopped animation will remain stopped.
/// A running animation will continue to run.
FFI_EXPORT void cg_context_set_image_animation_state(FFI_PTR_NATIVE cg_image_t * const image, const cg_image_animation_state_e image_animation_state);

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

/// Determines how to layout text when drawing via cg_context_fill_text_with_options
/// Note: automatic layouts do incur some cost.
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
    /// The provided x coodinate denotes the start of drawn text. (all text will be to the right of this line)
    cg_font_fill_options_align_left = 1 << 0,
    /// The provided x coordinate denotes the mid point of drawn text. (all text will be split around this line)
    cg_font_fill_options_align_center = 1 << 1,
    /// The provided x coordinate denotes the end point of drawn text. (all text will be to the left of this line)
    cg_font_fill_options_align_right = 1 << 2,
    /// The provided y coordiante denotes the start of drawn text. (all text will be below this line)
    cg_font_fill_options_align_top = 1 << 3,
    /// The provided y coordinate denotes the end of drawn text. (all text will be above this line)
    cg_font_fill_options_align_bottom = 1 << 4,
} cg_font_fill_options_e;

/// Determine how to draw a text block
FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
typedef enum cg_text_block_options_e {
    cg_text_block_no_options = 0,
    /// Individual lines of text will be aligned to the left of the text block.
    cg_text_block_align_line_left = 1 << 0,
    /// Individual lines of text will be centered in the text block.
    cg_text_block_align_line_center = 1 << 1,
    /// Individual lines of text will be aligned to the right of the text block.
    cg_text_block_align_line_right = 1 << 2,

    /// Default value when drawing with a text block.
    /// Text will be aligned to the top of the text block (text drawing starts at y=0)
    cg_text_block_align_text_top = 1 << 3,
    /// If the text does not fill the text block, center the text vertically inside the text block.
    cg_text_block_align_text_center = 1 << 4,
    /// If the text does not fill the text block, offset it so the last line is at the height limit for the text block.
    cg_text_block_align_text_bottom = 1 << 5,

    /// Allow text to extend above/below the text block by up to 1 line of text each.
    /// This combined with clipping with the same rect as provided to the text block draw, will produce sharp edges for text.
    /// Not enabling this will cull any text that would overlap with the text rect.
    cg_text_block_allow_block_bounds_overflow = 1 << 6,
    /// if this flag is passed the `extra_line_spacing` value is treated as a relative height offset
    /// 1.0 -> normal line spacing, 1.2 -> 120% line height.. etc
    /// if the flag is not passed in, then `extra_line_spacing` is treated as an absolute offset.
    /// 0 -> normal line spacing, -20 -> 20 units of overlap, +20 -> 20 units of extra space
    cg_text_block_line_space_relative = 1 << 7,
} cg_text_block_options_e;

/// Various states of font loading, see individual fields for details
FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_CLEAN_NAMES
typedef enum cg_font_async_load_status_e {
    /// Found loading failed due to out of memory in internal heaps (and must still be explicitly freed).
    cg_font_async_load_out_of_memory = -6,
    /// Font loading failed due to a file IO error.
    cg_font_async_load_file_error = -5,
    /// Font loading failed due to the file format being unrecognized.
    cg_font_async_load_font_parse_error = -4,
    /// Font loading failed due to an http error.
    cg_font_async_load_http_fetch_error = -3,
    /// internal use signaling that the load was canceled by the user.
    cg_font_async_load_aborted = -2,
    /// Font loading completed successfully and is ready for use.
    cg_font_async_load_complete = 0,
    /// Font loading in progress.
    cg_font_async_load_pending = 1,
} cg_font_async_load_status_e;

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
FFI_ENUM_BITFLAGS
FFI_ENUM_CLEAN_NAMES
typedef enum cg_font_load_opts_e {
    /// No additional behavior when loading a font
    cg_font_load_opts_none = 0 << 0,
    /// Enable verbose logging for the http backend
    cg_font_load_opts_http_verbose = 1 << 0,
} cg_font_load_opts_e;

typedef struct cg_font_context_t {
    mosaic_context_t * mosaic_ctx;

    int32_t font_index;
    float size; // size when font was created
    float virtual_size; // size that will be used when rendering
    float baseline;
} cg_font_context_t;

/// Offsets into the provided string (relative to the start of the provided string) that denote a sub region of text that can fit inside of the text block provided.
FFI_EXPORT
typedef struct cg_text_block_page_offsets_t {
    /// Page beginning byte offset
    uint32_t begin_offset;
    /// Page ending byte offset.
    uint32_t end_offset;
} cg_text_block_page_offsets_t;

typedef struct cg_font_file_t cg_font_file_t;

/// Free a font context provided by `font` -- font files will be freed when reference counts hit zero.
FFI_EXPORT void cg_context_font_context_free(FFI_CAN_BE_NULL FFI_PTR_NATIVE cg_font_context_t * font, FFI_MALLOC_TAG const char * const tag);
/// Decrement or free a font file if the reference count is zero.
FFI_EXPORT void cg_context_font_file_free(FFI_PTR_NATIVE cg_font_file_t * const font, FFI_MALLOC_TAG const char * const tag);

/// Load a font file asynchronously. Font files may not be used until the font file's status is `complete`.
FFI_EXPORT FFI_PTR_NATIVE cg_font_file_t * cg_context_load_font_file_async(FFI_PTR_WASM const char * const filepath, const cg_memory_region_e memory_region, const cg_font_load_opts_e font_load_opts, FFI_MALLOC_TAG const char * const tag);
/// Will create a font context later used for drawing text, font files are reference counted and shared between multiple contexts.
/// Creating a new context is 'cheap' -- but each unique context will have uniquely rasterized glyphs.
/// `size` is the height of text that will be drawn with this context.
/// `tab_space_multiplier` is how many spaces a tab is considered. Many text editors default to 4, but a tab does not have this info in a font file, and must be manually specified as it is a control character.
FFI_EXPORT FFI_PTR_NATIVE cg_font_context_t * cg_context_create_font_context(FFI_PTR_NATIVE cg_font_file_t * const cg_font, const float size, const int32_t tab_space_multiplier, FFI_MALLOC_TAG const char * const tag);

/// Get the current load status of a font. see `cg_font_async_load_status_e`
FFI_EXPORT cg_font_async_load_status_e cg_get_font_load_status(FFI_PTR_NATIVE const cg_font_file_t * const cg_font);

/// Sets the global (for this cg_context) missing glyph indicator, expects a single utf8 character for the indicator.
///
/// To reset to using the default value `missing_glyph_indocator` should be either a nullptr, or a zero length string e.g. NULL or ""
FFI_EXPORT void cg_context_set_global_missing_glyph_indicator(FFI_PTR_WASM const char * const missing_glyph_indicator);

/// Sets the missing glyph indicator for this font context, this has higher priority than `cg_context_set_global_missing_glyph_indicator`.
///
/// To reset to using the default value `missing_glyph_indocator` should be either a nullptr, or a zero length string e.g. NULL or ""
FFI_EXPORT void cg_context_set_font_context_missing_glyph_indicator(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, FFI_PTR_WASM const char * const missing_glyph_indicator);

/// Determine the bounds of provided text as it would be drawn assuming no bounds applied.
FFI_EXPORT cg_font_metrics_t cg_context_text_measure(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, FFI_PTR_WASM const char * const text);

/// Draw text at the provided position. equivalent to cg_context_fill_text_with_options(font_ctx, pos, text, cg_font_fill_options_e::left);
FFI_EXPORT cg_font_metrics_t cg_context_fill_text(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, const cg_vec2_t pos, FFI_PTR_WASM const char * const text);

/// Draw text without checking for bounds, and without handling new lines.
FFI_EXPORT void cg_context_fill_text_line(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, const cg_vec2_t pos, FFI_PTR_WASM const char * const text);

/// Draw text at the provided position with options `options`. see cg_font_fill_options_e for option behavior.
FFI_EXPORT cg_font_metrics_t cg_context_fill_text_with_options(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, const cg_vec2_t pos, FFI_PTR_WASM const char * const text, const cg_font_fill_options_e options);

/// Draw a text block bound by `text_rect`
/// `text_scroll_offset` applies a vertical offset to all drawn text to enable a scrolling effect.
/// `extra_line_spacing` applies an additional offset to the vertical space between lines (if larger spacing is needed)
/// `text` the text to be drawn.
/// `optional_ellipses` if ellipses are provided, and the text would not fit inside the text block, will place ellipses at the end of the last format appropriate glyph.
/// `options` see cg_text_block_options_e for behavior.
FFI_EXPORT cg_font_metrics_t cg_context_fill_text_block_with_options(
    FFI_PTR_NATIVE cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float text_scroll_offset,
    const float extra_line_spacing,
    FFI_PTR_WASM const char * const text,
    FFI_PTR_WASM FFI_CAN_BE_NULL const char * const optional_ellipses,
    const cg_text_block_options_e options);

/// Precaches glyphs provided by `characters` into internal font atlas without rendering them to the screen.
/// Provided as a convenience/ease of use optimization for uploading multiple characters.
/// This is most useful when having to draw a ton of individual characters with separate text draw functions. (e.g. an on screen keyboard)
/// This can have _massive_ performance gains with reducing stalling.
/// However no all glyphs are guaranteed to be cached, and this could be a pessimizing thing depending on the state of the font atlas.
FFI_EXPORT
void cg_context_font_precache_glyphs(FFI_PTR_NATIVE cg_font_context_t * const font_ctx, FFI_PTR_WASM const char * const characters);

/// Clears internal font atlas along with all associated glyph tracking meta data.
/// Generally should not be called as it is _usually_ a pessimizing operation.
FFI_EXPORT
void cg_context_font_clear_glyph_cache();

cg_rect_t cg_context_get_text_block_extents(
    cg_font_context_t * const font_ctx,
    const float line_width,
    const float extra_line_spacing,
    const char * const text,
    const cg_text_block_options_e options);

/// Returns the total height of text as if it were drawn without a heigh limit, but bounded by `line_width`
FFI_EXPORT float cg_context_get_text_block_height(
    FFI_PTR_NATIVE cg_font_context_t * const font_ctx,
    const float line_width,
    const float extra_line_spacing,
    FFI_PTR_WASM const char * const text,
    const cg_text_block_options_e options);

/// Returns begin/end offsets that can fit inside the provided text block relative to the provided text `text`
/// To determine total number of pages, call this function until end equals the end of the provided text.
FFI_EXPORT
cg_text_block_page_offsets_t cg_context_get_text_block_page_offsets(
    FFI_PTR_NATIVE cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    FFI_PTR_WASM const char * const text,
    const cg_text_block_options_e options);

/// Unused/no behavior (left for FFI compatibility)
/// Old behavior produced degraded font rendering.
/// May be re-used in the future.
FFI_EXPORT void cg_context_font_set_virtual_size(FFI_PTR_NATIVE cg_font_context_t * font_ctx, float size);

/// Sets the clip/scissor that will be applied when subsequent things are drawn.
FFI_EXPORT void cg_context_set_clip_rect(const cg_rect_t rect);
/// Enable/disable clip/scissoring.
FFI_EXPORT void cg_context_set_clip_state(const cg_clip_state_e clip_state);

#ifdef __cplusplus
}
#endif
