/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/canvas/cg.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
enum {
    cg_utf8_max_codepoint_len = 4,
};

struct cg_font_file_t;

typedef struct text_mesh_t {
    r_mesh_t * r_mesh;
    rb_fence_t fence;

    cg_gl_vertex_t * verts;
    const_mem_region_t null_buffer;

    cg_heap_t * resource_heap;

    int32_t vert_index;
    int32_t total_glyphs;
    int32_t glyphs_drawn;
    int32_t reserved_verts;

    cg_rect_t bounding_box;
} text_mesh_t;

typedef struct text_mesh_id_block_t {
    // hash
    uint32_t crc;
    // amount of stuff to render
    uint32_t str_len;

    // modifiers to offset, but not total length or anything else
    float scroll_offset;
    cg_text_block_options_e options;

    // input data to make reasonably sure its unique
    cg_rect_t rect;
    int font_id;
    char first_n_chars[7];
    bool has_ellipses;
} text_mesh_id_block_t;

typedef struct text_mesh_cache_node_t {
    text_mesh_id_block_t id_block;
    text_mesh_t text_mesh;
    struct text_mesh_cache_node_t * prev;
    struct text_mesh_cache_node_t * next;
} text_mesh_cache_node_t;

typedef struct text_mesh_cache_t {
    struct {
        text_mesh_cache_node_t * head;
        text_mesh_cache_node_t * tail;
    } active;
    struct {
        text_mesh_cache_node_t * head;
        text_mesh_cache_node_t * tail;
    } free;

    text_mesh_cache_node_t * storage;
    uint32_t storage_len;
} text_mesh_cache_t;

typedef struct mosaic_font_data_t {
    struct cg_font_file_t * cg_font;
    char missing_glyph_codepoint[cg_utf8_max_codepoint_len];

    float scale;
    int32_t ascent;
    int32_t height;
    int32_t tab_space_multiplier;
} mosaic_font_data_t;

/* ------------------------------------------------------------------------- */

typedef struct font_atlas_upload_region_t {
    struct font_atlas_upload_region_t * prev;
    struct font_atlas_upload_region_t * next;
    mem_region_t region;
    rb_fence_t fence;
} font_atlas_upload_region_t;

struct mosaic_glyph_raster_t;

struct mosaic_context_t {
    cg_context_t * cg_ctx;
    struct mosaic_glyph_raster_t * glyph_raster;

    struct {
        // the image's image.data is cpu local bytes only.
        cg_image_t image;
#if defined(_VADER) || defined(_LEIA)
        rb_fence_t image_fence;
#endif
        // TODO: promote this to a real backed region vs inside canvas's heap.
        cg_allocation_t sub_image_copy_buffer;
        heap_t sub_image_heap;
        struct {
            font_atlas_upload_region_t * free_head;
            font_atlas_upload_region_t * free_tail;
            font_atlas_upload_region_t * pending_head;
            font_atlas_upload_region_t * pending_tail;
            font_atlas_upload_region_t * buffer;
            uint32_t buffer_size;
        } upload_region;
    } atlas;

    text_mesh_cache_t text_mesh_cache;

    linear_block_allocator_t atlas_lba;

    int32_t max_width;
    int32_t max_height;

    int32_t font_index;
    int32_t font_count;
    int32_t font_id_counter;
    mosaic_font_data_t * fonts;

    char missing_glyph_codepoint[cg_utf8_max_codepoint_len];

#ifdef GUARD_PAGE_SUPPORT
    system_guard_page_mode_e guard_page_mode;
    mem_region_t guard_pages;
#endif
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

mosaic_context_t * mosaic_context_new(
    cg_context_t * const cg_ctx,
    const int32_t width,
    const int32_t height,
    const mem_region_t font_atlas_lba_region,
    const system_guard_page_mode_e guard_page_mode,
    const char * const tag);

int32_t mosaic_context_font_load(mosaic_context_t * ctx, cg_font_file_t * const cg_font, const float height, const int32_t tab_space_multiplier);

void mosaic_context_font_free(mosaic_context_t * ctx, int32_t index);

void mosaic_context_free(mosaic_context_t * ctx);

void mosaic_context_font_bind(mosaic_context_t * ctx, int32_t index);

cg_rect_t mosaic_context_draw_text_block(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const char * const optional_ellipses,
    const cg_text_block_options_e options);

void mosaic_context_draw_text_line(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const char * const text);

cg_rect_t mosaic_context_get_text_block_extents(
    mosaic_context_t * const mosaic_ctx,
    const float max_line_width,
    const float extra_line_spacing,
    const char * const text,
    float * const out_widest_line,
    const cg_text_block_options_e options);

cg_text_block_page_offsets_t mosaic_context_get_text_block_page_offsets(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const cg_text_block_options_e options);

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
