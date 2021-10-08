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

typedef struct mosaic_font_data_t {
    struct cg_font_file_t * cg_font;
    char missing_glyph_codepoint[cg_utf8_max_codepoint_len];

    float scale;
    int32_t ascent;
    int32_t height;
    int32_t tab_space_multiplier;
} mosaic_font_data_t;

/* ------------------------------------------------------------------------- */

struct mosaic_glyph_raster_t;

struct mosaic_context_t {
    cg_context_t * cg_ctx;
    struct mosaic_glyph_raster_t * glyph_raster;
    cg_image_t atlas_image;
    rb_fence_t atlas_fence;
    linear_block_allocator_t atlas_lba;

    int32_t max_width;
    int32_t max_height;

    int32_t font_index;
    int32_t font_count;
    mosaic_font_data_t * fonts;

    char missing_glyph_codepoint[cg_utf8_max_codepoint_len];

#ifdef GUARD_PAGE_SUPPORT
    system_guard_page_mode_e guard_page_mode;
    mem_region_t guard_pages;
#endif
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

mosaic_context_t * mosaic_context_new(cg_context_t * const cg_ctx, const int32_t width, const int32_t height, const mem_region_t font_atlas_lba_region, const system_guard_page_mode_e guard_page_mode, const char * const tag);

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
