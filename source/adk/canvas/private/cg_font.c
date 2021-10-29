/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 cg_font.c

 canvas font backend
 */

#include "cg_font.h"

#include "source/adk/canvas/cg.h"
#include "source/adk/file/file.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/crc.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/telemetry/telemetry.h"

enum {
    verts_per_quad = 6,
    non_breaking_space = 0xa0,
    default_atlas_upload_queue_limit = 64,
};

extern struct cg_statics_t cg_statics;

static bool compare_less_int32_t(const int32_t a, const int32_t b) {
    return a < b;
}

#define ALGO_STATIC int32_t
#include "source/adk/runtime/algorithm.h"

#include <math.h>

#if defined(__GNUC__) && (__GNUC__ > 4)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#define STBTT_malloc(_alloc_size, _user) lba_allocate(&((mosaic_context_t *)_user)->atlas_lba, 4, (int)(_alloc_size))
// normally this would be left blank, but this silences a warning from gcc about a blank body of an if statement.
#define STBTT_free(_ptr, _user) (void)_ptr;

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#if defined(__GNUC__) && (__GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

#define TAG_CG_FONT FOURCC('F', 'O', 'N', 'T')

enum {
    default_font_glyph_index_size = 512,
    max_unique_glyphs_per_stb_raster_pass = 16
};

struct cg_font_file_t {
    cg_context_t * cg_ctx;

    struct font_async_load_user_t * load_user;
    cg_const_allocation_t font_bytes;
    stbtt_fontinfo font_info;
    int32_t font_ascent;

    cg_font_async_load_status_e async_load_status;
    int32_t reference_count;
    int32_t id;
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static void cg_font_load_user_http_failure_cleanup(void * const void_user);

void cg_context_font_file_free(cg_font_file_t * const font, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();

    // the only time this reference count can go negative is if someone cancels loading the font,
    // then we'd set the font status to aborted.
    // then font loading part would then decrement from 0-> -1 and destruct the font file (in a main thread)
    if (--font->reference_count <= 0) {
        if (font->async_load_status != cg_font_async_load_pending || font->load_user) {
            if (font->load_user) {
                cg_font_load_user_http_failure_cleanup(font->load_user);
            }
            if (font->font_bytes.region.ptr) {
                cg_free_const_alloc(font->font_bytes, tag);
            }
            cg_free(&font->cg_ctx->cg_heap_low, font, tag);
        } else {
            font->async_load_status = cg_font_async_load_aborted;
        }
    }
    CG_FONT_TRACE_POP();
}

cg_font_context_t * cg_context_create_font_context(cg_font_file_t * const cg_font, const float size, const int32_t tab_space_multiplier, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    ASSERT((cg_font->async_load_status == cg_font_async_load_complete) && (tab_space_multiplier > 0));

    cg_font_context_t * font_ctx = cg_alloc(&cg_font->cg_ctx->cg_heap_low, sizeof(cg_font_context_t), tag);
    ZEROMEM(font_ctx);

    font_ctx->mosaic_ctx = cg_font->cg_ctx->mosaic_ctx;
    font_ctx->baseline = size;
    font_ctx->size = size;
    font_ctx->virtual_size = size;
    font_ctx->font_index = mosaic_context_font_load(cg_font->cg_ctx->mosaic_ctx, cg_font, size, tab_space_multiplier);

    CG_FONT_TRACE_POP();
    return font_ctx;
}

cg_font_metrics_t cg_context_text_measure(cg_font_context_t * const font_ctx, const char * const text) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);

    const cg_rect_t bounds = cg_context_get_text_block_extents(font_ctx, INFINITY, 0, text, cg_text_block_no_options);

    const mosaic_font_data_t * const font = &font_ctx->mosaic_ctx->fonts[font_ctx->mosaic_ctx->font_index];

    CG_FONT_TRACE_POP();
    return (cg_font_metrics_t){
        .bounds = bounds,
        .baseline = font->ascent * font->scale,
    };
}

cg_font_metrics_t cg_context_fill_text(cg_font_context_t * const font_ctx, const cg_vec2_t pos, const char * const text) {
    return cg_context_fill_text_block_with_options(font_ctx, (cg_rect_t){.x = pos.x, .y = pos.y, .width = INFINITY, .height = INFINITY}, 0, 0, text, NULL, cg_text_block_no_options);
}

cg_font_metrics_t cg_context_fill_text_with_options(cg_font_context_t * const font_ctx, const cg_vec2_t pos, const char * const text, const cg_font_fill_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    float x_off = pos.x, y_off = pos.y;

    cg_rect_t bounds = {0};
    if (!((options & cg_font_fill_options_align_left) && (options & cg_font_fill_options_align_top))) {
        float widest_line_ignored = 0;
        mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);
        bounds = mosaic_context_get_text_block_extents(font_ctx->mosaic_ctx, INFINITY, 0, text, &widest_line_ignored, cg_text_block_no_options);
    }

    if (options & cg_font_fill_options_align_left) {
        x_off += 0.0;
    } else if (options & cg_font_fill_options_align_right) {
        x_off -= bounds.width;
    } else {
        x_off -= bounds.width * 0.5f;
    }

    if (options & cg_font_fill_options_align_top) {
        y_off += 0.0f;
    } else if (options & cg_font_fill_options_align_bottom) {
        y_off -= bounds.height;
    } else {
        y_off -= bounds.height * 0.5f;
    }

    const cg_font_metrics_t metrics = cg_context_fill_text_block_with_options(font_ctx, (cg_rect_t){.x = x_off, .y = y_off, .width = INFINITY, .height = INFINITY}, 0, 0, text, NULL, cg_text_block_no_options);
    CG_FONT_TRACE_POP();
    return metrics;
}

void cg_context_fill_text_line(cg_font_context_t * const font_ctx, const cg_vec2_t pos, const char * const text) {
    CG_FONT_TRACE_PUSH_FN();

    ASSERT_MSG(strchr(text, '\n') == 0, "text shall not contain new-lines");

    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);

    mosaic_context_draw_text_line(font_ctx->mosaic_ctx, (cg_rect_t){.x = pos.x, .y = pos.y, .width = INFINITY, .height = INFINITY}, text);

    CG_FONT_TRACE_POP();
}

cg_rect_t cg_context_get_text_block_extents(cg_font_context_t * const font_ctx, const float max_line_width, const float extra_line_spacing, const char * const text, const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);

    float widest_line = 0;
    cg_rect_t rect = mosaic_context_get_text_block_extents(
        font_ctx->mosaic_ctx,
        max_line_width,
        extra_line_spacing,
        text,
        &widest_line,
        options);
    rect.width = widest_line;
    CG_FONT_TRACE_POP();
    return rect;
}

float cg_context_get_text_block_height(cg_font_context_t * const font_ctx, const float max_line_width, const float extra_line_spacing, const char * const text, const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);
    float widest_line_ignored = 0;
    CG_FONT_TRACE_POP();
    return mosaic_context_get_text_block_extents(
               font_ctx->mosaic_ctx,
               max_line_width,
               extra_line_spacing,
               text,
               &widest_line_ignored,
               options)
        .height;
}

static cg_rect_t mosaic_context_draw_text_block_memoized(
    cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const char * const optional_ellipses,
    const cg_text_block_options_e options);

cg_font_metrics_t cg_context_fill_text_block_with_options(
    cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float text_scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const char * const optional_ellipses,
    const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();

    cg_rect_t text_input_rect = text_rect;
    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);

    // if neither bit is set, skip calculating the extents and the rest of this logic.
    if (options & (cg_text_block_align_text_bottom | cg_text_block_align_text_center)) {
        float widest_line_ignored = 0;
        const cg_rect_t text_extents = mosaic_context_get_text_block_extents(font_ctx->mosaic_ctx, text_rect.width, extra_line_spacing, text, &widest_line_ignored, options);
        if (options & cg_text_block_align_text_bottom) {
            if (text_extents.height < text_rect.height) {
                text_input_rect.y += text_rect.height - text_extents.height;
                text_input_rect.height = text_extents.height;
            }
        } else {
            if (text_extents.height < text_rect.height) {
                text_input_rect.y += (text_rect.height - text_extents.height) / 2.f;
                text_input_rect.height = text_extents.height;
            }
        }
    }

    cg_rect_t bounds = {0};
    if (cg_statics.ctx->config.text_mesh_cache.enabled) {
        bounds = mosaic_context_draw_text_block_memoized(font_ctx, text_input_rect, text_scroll_offset, extra_line_spacing, text, optional_ellipses, options);
    } else {
        bounds = mosaic_context_draw_text_block(font_ctx->mosaic_ctx, text_input_rect, text_scroll_offset, extra_line_spacing, text, optional_ellipses, options);
    }
    const mosaic_font_data_t * const font = &font_ctx->mosaic_ctx->fonts[font_ctx->mosaic_ctx->font_index];

    CG_FONT_TRACE_POP();
    return (cg_font_metrics_t){
        .bounds = bounds,
        .baseline = font->ascent * font->scale,
    };
}

cg_text_block_page_offsets_t cg_context_get_text_block_page_offsets(
    cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const cg_text_block_options_e options) {
    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);
    return mosaic_context_get_text_block_page_offsets(font_ctx->mosaic_ctx, text_rect, scroll_offset, extra_line_spacing, text, options);
}

void cg_context_font_set_virtual_size(cg_font_context_t * font_ctx, float size) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_context_font_bind(font_ctx->mosaic_ctx, font_ctx->font_index);
    font_ctx->virtual_size = CLAMP(size, 4.0f, 150.0f);
    CG_FONT_TRACE_POP();
}

/* ===========================================================================
 * FONT
 * ==========================================================================*/

int32_t utf8_to_codepoint(const char * const utf8, int32_t * const codepoint) {
    CG_FONT_TRACE_PUSH_FN();
    // utf8 binary representation: https://tools.ietf.org/html/rfc3629#section-3
    /*
	Char. number range  |        UTF-8 octet sequence
       (hexadecimal)    |              (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	*/

    const uint8_t * const s = (const uint8_t *)utf8;
    if ((*utf8 & 0x80) == 0) {
        *codepoint = *s;
        CG_FONT_TRACE_POP();
        return 1;
    } else if ((*utf8 & 0xE0) == 0xC0) {
        *codepoint = ((int32_t)(s[0] & ~0xc0) << 6) | (int32_t)(s[1] & ~0x80);
        CG_FONT_TRACE_POP();
        return 2;
    } else if ((*utf8 & 0xF0) == 0xE0) {
        *codepoint = ((int32_t)(s[0] & ~0xe0) << 12) | ((int32_t)(s[1] & ~0x80) << 6) | (int32_t)(s[2] & ~0x80);
        CG_FONT_TRACE_POP();
        return 3;
    } else if ((*utf8 & 0xF8) == 0xF0) {
        *codepoint = ((int32_t)(s[0] & ~0xf0) << 18) | ((int32_t)(s[1] & ~0x80) << 12) | ((int32_t)(s[2] & ~0x80) << 6) | (int32_t)(s[3] & ~0x80);
        CG_FONT_TRACE_POP();
        return 4;
    }
#ifndef _SHIP
    else {
        TRAP("Invalid UTF8 codepoint!");
    }
#endif

    CG_FONT_TRACE_POP();
    return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

enum {
    glyph_raster_growth_factor = 2,
    glyph_raster_default_size = 64,
};

typedef enum mosaic_raster_insert_status_e {
    mosaic_raster_insert_no_atlas_space = -1,
    mosaic_raster_insert_exists = 0,
    mosaic_raster_insert_new = 1,
} mosaic_raster_insert_status_e;

// this will be our offset into the texture atlas, there is zero chance we're going to have a texture atlas larger than 32k
typedef struct cg_int16_rect_t {
    int16_t x, y;
    int16_t width, height;
} cg_int16_rect_t;

typedef struct cg_uvec2_t {
    uint32_t x, y;
} cg_uvec2_t;

typedef enum codepoint_raster_state_e {
    codepoint_no_backing_glyph = -2,
    codepoint_packing_failed = -1,
    codepoint_uninit = 0,
    codepoint_rasterized = 1,
} codepoint_raster_state_e;

typedef enum codepoint_rasterizing_e {
    codepoint_rasterizing_partial_failure,
    codepoint_rasterizing_renderable_rasterized,
} codepoint_rasterizing_e;

typedef struct codepoint_info_t {
    cg_int16_rect_t tex_coords;
    float x_off;
    float y_off;
    float x_advance;

    codepoint_raster_state_e state;
} codepoint_info_t;

typedef struct font_glyph_cache_t {
    struct font_glyph_cache_t * prev;
    struct font_glyph_cache_t * next;

    int32_t * codepoints;
    codepoint_info_t * codepoint_infos;

    int32_t font_id;
    int32_t codepoints_size;
    int32_t codepoints_capacity;
    uint32_t num_unrasterized;
} font_glyph_cache_t;

typedef struct mosaic_glyph_raster_t {
    stbtt_pack_context pack_context;

    mosaic_context_t * mosaic_ctx;

    font_glyph_cache_t * font_glyph_cache_head;
    font_glyph_cache_t * font_glyph_cache_tail;

    bool atlas_dirty;
    struct {
        cg_uvec2_t p00;
        cg_uvec2_t p11;
    } dirty_region;
} mosaic_glyph_raster_t;

static bool is_whitespace(const int32_t codepoint) {
    return (codepoint == '\t') || (codepoint == ' ');
}

static bool is_newline(const int32_t codepoint) {
    return (codepoint == '\n') || (codepoint == '\r');
}

static bool is_control_character(const int32_t codepoint) {
    return is_whitespace(codepoint) || is_newline(codepoint) || codepoint == non_breaking_space;
}

static int32_t get_missing_glyph_codepoint(const mosaic_context_t * const mosaic_ctx, const mosaic_font_data_t * const font_data) {
    CG_FONT_TRACE_PUSH_FN();
    int32_t codepoint;
    const char * const missing_glyph_indicator = font_data->missing_glyph_codepoint[0] ? font_data->missing_glyph_codepoint : (mosaic_ctx->missing_glyph_codepoint[0] ? mosaic_ctx->missing_glyph_codepoint : " ");
    utf8_to_codepoint(missing_glyph_indicator, &codepoint);
    CG_FONT_TRACE_POP();
    return codepoint;
}

static void mosaic_glyph_raster_emplace_init(mosaic_glyph_raster_t * const glyph_raster, mosaic_context_t * const mosaic_ctx) {
    CG_FONT_TRACE_PUSH_FN();
    ZEROMEM(glyph_raster);

    glyph_raster->mosaic_ctx = mosaic_ctx;
    glyph_raster->dirty_region.p00 = (cg_uvec2_t){(uint32_t)-1, (uint32_t)-1};
    glyph_raster->dirty_region.p11 = (cg_uvec2_t){0};

#if defined(_VADER) || defined(_LEIA)
    // make sure any pending operations are complete on the atlas before we call stbtt_PackBegin (it has an implicit memset to zero on the supplied pointer)
    render_conditional_flush_cmd_stream_and_wait_fence(
        mosaic_ctx->cg_ctx->gl->render_device,
        &mosaic_ctx->cg_ctx->gl->render_device->default_cmd_stream,
        mosaic_ctx->atlas.image_fence);
#endif

    stbtt_PackBegin(
        &glyph_raster->pack_context,
        mosaic_ctx->atlas.image.pixel_buffer.region.ptr,
        mosaic_ctx->atlas.image.image.width,
        mosaic_ctx->atlas.image.image.height,
        0,
        1,
        glyph_raster->mosaic_ctx);
    CG_FONT_TRACE_POP();
}

static void font_glyph_cache_free(mosaic_glyph_raster_t * const glyph_raster, font_glyph_cache_t * const glyph_cache) {
    CG_FONT_TRACE_PUSH_FN();
    cg_heap_t * const cg_heap = &glyph_raster->mosaic_ctx->cg_ctx->cg_heap_low;
    LL_REMOVE(glyph_cache, prev, next, glyph_raster->font_glyph_cache_head, glyph_raster->font_glyph_cache_tail);
    if (glyph_cache->codepoints) {
        cg_free(cg_heap, glyph_cache->codepoints, MALLOC_TAG);
    }
    if (glyph_cache->codepoint_infos) {
        cg_free(cg_heap, glyph_cache->codepoint_infos, MALLOC_TAG);
    }
    cg_free(cg_heap, glyph_cache, MALLOC_TAG);
    CG_FONT_TRACE_POP();
}

static void mosaic_glyph_raster_shutdown(mosaic_glyph_raster_t * const glyph_raster) {
    CG_FONT_TRACE_PUSH_FN();
    font_glyph_cache_t * curr_font_glyph_cache = glyph_raster->font_glyph_cache_head;
    while (curr_font_glyph_cache != NULL) {
        font_glyph_cache_t * const next = curr_font_glyph_cache->next;
        font_glyph_cache_free(glyph_raster, curr_font_glyph_cache);
        curr_font_glyph_cache = next;
    }
    stbtt_PackEnd(&glyph_raster->pack_context);
    CG_FONT_TRACE_POP();
}

static int32_t font_glyph_cache_find_codepoint_index(const int32_t codepoint, const font_glyph_cache_t * const glyph_cache) {
    CG_FONT_TRACE_PUSH_FN();
    if (glyph_cache->codepoints_size == 0) {
        CG_FONT_TRACE_POP();
        return -1;
    }
    const int32_t ind = (int32_t)(lower_bound_const_int32_t(glyph_cache->codepoints, glyph_cache->codepoints + glyph_cache->codepoints_size, codepoint) - glyph_cache->codepoints);
    CG_FONT_TRACE_POP();
    return (ind != glyph_cache->codepoints_size) && (glyph_cache->codepoints[ind] == codepoint) ? ind : -1;
}

static const codepoint_info_t * font_glyph_cache_find_codepoint_info(const int32_t codepoint, const font_glyph_cache_t * const glyph_cache) {
    CG_FONT_TRACE_PUSH_FN();
    const int32_t codepoint_index = font_glyph_cache_find_codepoint_index(codepoint, glyph_cache);
    if (codepoint_index >= 0) {
        CG_FONT_TRACE_POP();
        return &glyph_cache->codepoint_infos[codepoint_index];
    }
    CG_FONT_TRACE_POP();
    return NULL;
}

static void font_glyph_cache_insert_new_codepoint(const int32_t codepoint, font_glyph_cache_t * const glyph_cache) {
    CG_FONT_TRACE_PUSH_FN();
    ASSERT(glyph_cache->codepoints_size + 1 <= glyph_cache->codepoints_capacity);

    const ptrdiff_t insert_pos = lower_bound_const_int32_t(glyph_cache->codepoints, glyph_cache->codepoints + glyph_cache->codepoints_size, codepoint) - glyph_cache->codepoints;
    const int32_t num_right_codepoints = (int32_t)(glyph_cache->codepoints_size - insert_pos);

    if (num_right_codepoints != 0) {
        memmove(&glyph_cache->codepoints[insert_pos + 1], &glyph_cache->codepoints[insert_pos], num_right_codepoints * sizeof(*glyph_cache->codepoints));
        memmove(&glyph_cache->codepoint_infos[insert_pos + 1], &glyph_cache->codepoint_infos[insert_pos], num_right_codepoints * sizeof(*glyph_cache->codepoint_infos));
    }

    glyph_cache->codepoints[insert_pos] = codepoint;
    glyph_cache->codepoint_infos[insert_pos] = (codepoint_info_t){0};
    ++glyph_cache->codepoints_size;
    CG_FONT_TRACE_POP();
}

static void font_glyph_cache_grow(font_glyph_cache_t * const glyph_cache, cg_heap_t * const cg_heap) {
    CG_FONT_TRACE_PUSH_FN();
    glyph_cache->codepoints_capacity = (glyph_cache->codepoints_capacity == 0) ? glyph_raster_default_size : (glyph_cache->codepoints_capacity * glyph_raster_growth_factor);
    glyph_cache->codepoints = cg_realloc(cg_heap, glyph_cache->codepoints, sizeof(*glyph_cache->codepoints) * glyph_cache->codepoints_capacity, MALLOC_TAG);
    glyph_cache->codepoint_infos = cg_realloc(cg_heap, glyph_cache->codepoint_infos, sizeof(*glyph_cache->codepoint_infos) * glyph_cache->codepoints_capacity, MALLOC_TAG);
    CG_FONT_TRACE_POP();
}

static font_glyph_cache_t * mosaic_glyph_raster_find_glyph_cache(mosaic_glyph_raster_t * const glyph_raster, const int32_t font_id) {
    CG_FONT_TRACE_PUSH_FN();
    font_glyph_cache_t * curr_font_glyph_cache = glyph_raster->font_glyph_cache_head;
    while (curr_font_glyph_cache != NULL) {
        if (curr_font_glyph_cache->font_id == font_id) {
            CG_FONT_TRACE_POP();
            return curr_font_glyph_cache;
        }
        curr_font_glyph_cache = curr_font_glyph_cache->next;
    }
    CG_FONT_TRACE_POP();
    return NULL;
}

static void mosaic_glyph_raster_destroy_glyph_cache(mosaic_glyph_raster_t * const glyph_raster, const int32_t font_id) {
    CG_FONT_TRACE_PUSH_FN();
    font_glyph_cache_t * glyph_cache = mosaic_glyph_raster_find_glyph_cache(glyph_raster, font_id);

    if (glyph_cache) {
        font_glyph_cache_free(glyph_raster, glyph_cache);
    }
    CG_FONT_TRACE_POP();
}

void cg_context_font_context_free(cg_font_context_t * font_ctx, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    if (!font_ctx) {
        CG_FONT_TRACE_POP();
        return;
    }
    mosaic_glyph_raster_destroy_glyph_cache(font_ctx->mosaic_ctx->glyph_raster, font_ctx->font_index);
    mosaic_context_font_free(font_ctx->mosaic_ctx, font_ctx->font_index);
    cg_free(&font_ctx->mosaic_ctx->cg_ctx->cg_heap_low, font_ctx, tag);
    CG_FONT_TRACE_POP();
}

static font_glyph_cache_t * mosaic_glyph_raster_create_glyph_cache(mosaic_glyph_raster_t * const glyph_raster, const int32_t font_id) {
    CG_FONT_TRACE_PUSH_FN();
    font_glyph_cache_t * glyph_cache = cg_alloc(&glyph_raster->mosaic_ctx->cg_ctx->cg_heap_low, sizeof(font_glyph_cache_t), MALLOC_TAG);
    ZEROMEM(glyph_cache);
    LL_ADD(glyph_cache, prev, next, glyph_raster->font_glyph_cache_head, glyph_raster->font_glyph_cache_tail);
    glyph_cache->font_id = font_id;
    CG_FONT_TRACE_POP();
    return glyph_cache;
}

static mosaic_raster_insert_status_e mosaic_glyph_raster_try_push_codepoint(mosaic_glyph_raster_t * const glyph_raster, const int32_t codepoint, font_glyph_cache_t * const glyph_cache) {
    CG_FONT_TRACE_PUSH_FN();
    if (font_glyph_cache_find_codepoint_index(codepoint, glyph_cache) >= 0) {
        CG_FONT_TRACE_POP();
        return mosaic_raster_insert_exists;
    }

    if (glyph_cache->codepoints_size + 1 > glyph_cache->codepoints_capacity) {
        font_glyph_cache_grow(glyph_cache, &glyph_raster->mosaic_ctx->cg_ctx->cg_heap_low);
    }

    font_glyph_cache_insert_new_codepoint(codepoint, glyph_cache);
    ++glyph_cache->num_unrasterized;
    CG_FONT_TRACE_POP();
    return mosaic_raster_insert_new;
}

static codepoint_rasterizing_e mosaic_glyph_raster_rasterize_glyph_range(
    mosaic_glyph_raster_t * const glyph_raster,
    font_glyph_cache_t * const glyph_cache,
    const int32_t * const codepoints,
    int32_t * const glyph_offsets,
    const int32_t * const codepoint_indices,
    const int32_t num_new_codepoints) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_font_data_t * const selected_font = &glyph_raster->mosaic_ctx->fonts[glyph_cache->font_id];
    codepoint_rasterizing_e rasterizing_state = codepoint_rasterizing_renderable_rasterized;
    lba_reset(&glyph_raster->mosaic_ctx->atlas_lba);
    stbtt_packedchar glyph_packed_char_buff[max_unique_glyphs_per_stb_raster_pass];

    stbtt_pack_range pack_range = {0};
    pack_range.font_size = (float)selected_font->height;
    pack_range.array_of_unicode_codepoints = codepoints;
    pack_range.num_chars = num_new_codepoints;
    pack_range.chardata_for_range = glyph_packed_char_buff;
    pack_range.out_array_of_glyph_offsets = glyph_offsets;
    stbtt_PackFontRanges(&glyph_raster->pack_context, &selected_font->cg_font->font_info, 0, &pack_range, 1);

    for (int ind = 0; ind < num_new_codepoints; ++ind) {
        const stbtt_packedchar * const packed_char = &pack_range.chardata_for_range[ind];
        codepoint_info_t * const curr_codepoint_info = &glyph_cache->codepoint_infos[codepoint_indices[ind]];

        curr_codepoint_info->tex_coords = (cg_int16_rect_t){
            .x = (int16_t)packed_char->x0,
            .y = (int16_t)packed_char->y0,
            .width = (int16_t)(packed_char->x1 - packed_char->x0),
            .height = (int16_t)(packed_char->y1 - packed_char->y0)};

        curr_codepoint_info->x_advance = packed_char->xadvance;
        curr_codepoint_info->x_off = packed_char->xoff;
        curr_codepoint_info->y_off = packed_char->yoff;

        if ((glyph_offsets[ind] == 0) && !is_control_character(codepoints[ind])) {
            curr_codepoint_info->state = codepoint_no_backing_glyph;
        } else {
            // stb indicates that a glyph is missing from the atlas (or otherwise unrenderable) by having its rect's dimensions = 0.
            // note: this is different than a width or height of zero, which apparently spaces can have? they only take up a 1x1 region (their rect dims are 1,1,1,1) so we get a width & height of zero.
            // but the xadvance is a valid value for advancing.
            const bool is_in_atlas = (packed_char->x0 != 0) || (packed_char->x1 != 0) || (packed_char->y0 != 0) || (packed_char->y1 != 0);
            curr_codepoint_info->state = is_in_atlas ? codepoint_rasterized : codepoint_packing_failed;
            if (is_in_atlas) {
                int padding = glyph_raster->pack_context.padding;
                glyph_raster->dirty_region.p00.x = min_uint32_t(glyph_raster->dirty_region.p00.x, (uint32_t)max_int32_t(packed_char->x0 - padding, 0));
                glyph_raster->dirty_region.p00.y = min_uint32_t(glyph_raster->dirty_region.p00.y, (uint32_t)max_int32_t(packed_char->y0 - padding, 0));

                glyph_raster->dirty_region.p11.x = glyph_raster->dirty_region.p11.x == (uint32_t)-1 ? (uint32_t)packed_char->x1 : max_uint32_t(glyph_raster->dirty_region.p11.x, (uint32_t)packed_char->x1 + padding);
                glyph_raster->dirty_region.p11.y = glyph_raster->dirty_region.p11.y == (uint32_t)-1 ? (uint32_t)packed_char->y1 : max_uint32_t(glyph_raster->dirty_region.p11.y, (uint32_t)packed_char->y1 + padding);

                glyph_raster->atlas_dirty = true;
            } else {
                rasterizing_state = codepoint_rasterizing_partial_failure;
            }
        }
    }
    CG_FONT_TRACE_POP();
    return rasterizing_state;
}

static codepoint_rasterizing_e mosaic_glyph_raster_rasterize_glyphs(mosaic_glyph_raster_t * const glyph_raster, font_glyph_cache_t * const glyph_cache, const int32_t first_codepoint_of_string) {
    CG_FONT_TRACE_PUSH_FN();
    if (glyph_cache->num_unrasterized == 0) {
        CG_FONT_TRACE_POP();
        return codepoint_rasterizing_renderable_rasterized;
    }

    // always pack the first glyph, since stb can fail anywhere in our range of provided glyphs (this will ensure we can make progress with this current variant)
    // if we support effectively 'masking' what glyphs can be drawn from the string, and only drawing those, we don't necessarily need to rasterize the first glyph, we can just draw all of them.
    const int32_t first_codepoint_of_string_ind = font_glyph_cache_find_codepoint_index(first_codepoint_of_string, glyph_cache);
    const int32_t first_codepoint = glyph_cache->codepoints[first_codepoint_of_string_ind];
    int32_t first_codepoint_glyph_offset;
    codepoint_rasterizing_e rasterizing_state = mosaic_glyph_raster_rasterize_glyph_range(glyph_raster, glyph_cache, &first_codepoint, &first_codepoint_glyph_offset, &first_codepoint_of_string_ind, 1);

    int32_t uninit_codepoint_indices_buffer[max_unique_glyphs_per_stb_raster_pass];
    int32_t codepoint_buffer[max_unique_glyphs_per_stb_raster_pass];
    int32_t glyph_offset_buffer[max_unique_glyphs_per_stb_raster_pass];
    int32_t codepoint_ind = 0;
    while (codepoint_ind < glyph_cache->codepoints_size) {
        int32_t codepoint_buffer_ind = 0;
        for (; (codepoint_ind < glyph_cache->codepoints_size) && (codepoint_buffer_ind < ARRAY_SIZE(uninit_codepoint_indices_buffer)); ++codepoint_ind) {
            if (glyph_cache->codepoint_infos[codepoint_ind].state == codepoint_uninit) {
                codepoint_buffer[codepoint_buffer_ind] = glyph_cache->codepoints[codepoint_ind];
                uninit_codepoint_indices_buffer[codepoint_buffer_ind++] = codepoint_ind;
            }
        }

        if (codepoint_buffer_ind > 0) {
            rasterizing_state = mosaic_glyph_raster_rasterize_glyph_range(glyph_raster, glyph_cache, codepoint_buffer, glyph_offset_buffer, uninit_codepoint_indices_buffer, codepoint_buffer_ind) == codepoint_rasterizing_partial_failure ? codepoint_rasterizing_partial_failure : rasterizing_state;
        }
    }

    glyph_cache->num_unrasterized = 0;
    CG_FONT_TRACE_POP();
    return rasterizing_state;
}

static void font_atlas_upload_region_push_free(mosaic_context_t * const mosaic_ctx, font_atlas_upload_region_t * const node) {
    LL_REMOVE(node, prev, next, mosaic_ctx->atlas.upload_region.pending_head, mosaic_ctx->atlas.upload_region.pending_tail);
    LL_ADD(node, prev, next, mosaic_ctx->atlas.upload_region.free_head, mosaic_ctx->atlas.upload_region.free_tail);
}

static font_atlas_upload_region_t * font_atlas_upload_region_try_pop_free(mosaic_context_t * const mosaic_ctx) {
    font_atlas_upload_region_t * const node = mosaic_ctx->atlas.upload_region.free_head;
    if (node) {
        LL_REMOVE(node, prev, next, mosaic_ctx->atlas.upload_region.free_head, mosaic_ctx->atlas.upload_region.free_tail);
        LL_ADD(node, prev, next, mosaic_ctx->atlas.upload_region.pending_head, mosaic_ctx->atlas.upload_region.pending_tail);
    }
    return node;
}

static font_atlas_upload_region_t * font_atlas_upload_get_last_signaled_node(mosaic_context_t * const mosaic_ctx) {
    font_atlas_upload_region_t * curr = mosaic_ctx->atlas.upload_region.pending_head;
    if (!curr || !render_check_fence(curr->fence)) {
        return NULL;
    }
    while (curr) {
        if (!curr->next || !render_check_fence(curr->next->fence)) {
            break;
        }
        curr = curr->next;
    }
    return curr;
}

static void font_atlas_upload_region_free_mem_up_to_node(mosaic_context_t * const mosaic_ctx, font_atlas_upload_region_t * const node) {
    font_atlas_upload_region_t * curr = mosaic_ctx->atlas.upload_region.pending_head;
    while (curr && curr != node) {
        font_atlas_upload_region_t * const next = curr->next;
        // with the way nodes are currently created, we can try to make a new node, run out of memory for allocating the region for it, then we'll try to flush all the nodes.
        // that newly created node will not have a memory region but be on this list.
        // this check is to allow that to silently pass.
        if (curr->region.ptr) {
            heap_free(&mosaic_ctx->atlas.sub_image_heap, curr->region.ptr, MALLOC_TAG);
        }
        curr->fence = (rb_fence_t){0};
        font_atlas_upload_region_push_free(mosaic_ctx, curr);
        curr = next;
    }
}

static font_atlas_upload_region_t * font_atlas_upload_region_conditional_flush_and_pop_free(mosaic_context_t * const mosaic_ctx, font_atlas_upload_region_t * const last_node_to_free) {
    CG_FONT_TRACE_PUSH_FN();
    font_atlas_upload_region_t * node = font_atlas_upload_region_try_pop_free(mosaic_ctx);
    if (!node) {
        render_conditional_flush_cmd_stream_and_wait_fence(
            mosaic_ctx->cg_ctx->gl->render_device,
            &mosaic_ctx->cg_ctx->gl->render_device->default_cmd_stream,
            last_node_to_free->fence);

        font_atlas_upload_region_free_mem_up_to_node(mosaic_ctx, last_node_to_free);
        node = font_atlas_upload_region_try_pop_free(mosaic_ctx);
        ASSERT(node); // logically this can't fail, but I'm paranoid right now incase we're miss handling linked lists.
    }
    CG_FONT_TRACE_POP();
    return node;
}

static void mosaic_glyph_raster_copy_sub_image(const image_t * const src, const image_t * const dst) {
    CG_FONT_TRACE_PUSH_FN();
    for (int y = 0; y < dst->height; ++y) {
        for (int x = 0; x < dst->width; ++x) {
            ((char *)dst->data)[x + y * dst->width] = ((char *)src->data)[(dst->x + x) + (dst->y + y) * src->width];
        }
    }
    CG_FONT_TRACE_POP();
}

static font_atlas_upload_region_t * mosaic_glyph_raster_alloc_sub_texture_region(mosaic_glyph_raster_t * const glyph_raster, const size_t region_size) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_context_t * const mosaic_ctx = glyph_raster->mosaic_ctx;
    font_atlas_upload_region_t * const last_signaled_node = font_atlas_upload_get_last_signaled_node(mosaic_ctx);
    font_atlas_upload_region_t * node = font_atlas_upload_region_conditional_flush_and_pop_free(mosaic_ctx, last_signaled_node ? last_signaled_node : mosaic_ctx->atlas.upload_region.pending_tail);
    node->region = MEM_REGION(.ptr = heap_unchecked_alloc(&mosaic_ctx->atlas.sub_image_heap, region_size, MALLOC_TAG), .size = region_size);

    if (!node->region.ptr) {
        // we could have enough nodes, but not enough space so if we run out of mem we'll need to wait on the pendings to complete.
        render_conditional_flush_cmd_stream_and_wait_fence(
            mosaic_ctx->cg_ctx->gl->render_device,
            &mosaic_ctx->cg_ctx->gl->render_device->default_cmd_stream,
            mosaic_ctx->atlas.upload_region.pending_tail->fence);

        font_atlas_upload_region_free_mem_up_to_node(mosaic_ctx, mosaic_ctx->atlas.upload_region.pending_tail);
        node = font_atlas_upload_region_try_pop_free(mosaic_ctx);
        ASSERT(node); // logically this can't fail, but I'm paranoid right now incase we're miss handling linked lists.

        node->region = MEM_REGION(.ptr = heap_alloc(&mosaic_ctx->atlas.sub_image_heap, region_size, MALLOC_TAG), .size = region_size);
    }
    VERIFY(node->region.ptr);
    CG_FONT_TRACE_POP();
    return node;
}

static void mosaic_glyph_raster_flush_atlas(mosaic_glyph_raster_t * const glyph_raster) {
    CG_FONT_TRACE_PUSH_FN();
    if (glyph_raster->atlas_dirty) {
        glyph_raster->atlas_dirty = false;

        cg_image_t * const atlas_image = &glyph_raster->mosaic_ctx->atlas.image;
#if !(defined(_VADER) || defined(_LEIA))
        image_t image = atlas_image->image;
        image = atlas_image->image;
        image.width = glyph_raster->dirty_region.p11.x - glyph_raster->dirty_region.p00.x;
        image.height = glyph_raster->dirty_region.p11.y - glyph_raster->dirty_region.p00.y;
        image.x = glyph_raster->dirty_region.p00.x;
        image.y = glyph_raster->dirty_region.p00.y;

        font_atlas_upload_region_t * const upload_region = mosaic_glyph_raster_alloc_sub_texture_region(glyph_raster, image.width * image.height * 1);
        image.data = upload_region->region.ptr;

        image_mips_t image_mips = {0};
        image_mips.num_levels = 1;
        image_mips.levels[0] = image;

        mosaic_glyph_raster_copy_sub_image(&atlas_image->image, &image);
        cg_gl_sub_texture_update(glyph_raster->mosaic_ctx->cg_ctx->gl, &atlas_image->cg_texture, image_mips);

        upload_region->fence = render_flush_cmd_stream(&glyph_raster->mosaic_ctx->cg_ctx->gl->render_device->default_cmd_stream, render_no_wait);
#else
        render_conditional_flush_cmd_stream_and_wait_fence(
            glyph_raster->mosaic_ctx->cg_ctx->gl->render_device,
            &glyph_raster->mosaic_ctx->cg_ctx->gl->render_device->default_cmd_stream,
            glyph_raster->mosaic_ctx->atlas.image_fence);

        cg_gl_texture_update(glyph_raster->mosaic_ctx->cg_ctx->gl, &atlas_image->cg_texture, atlas_image->image.data);

        glyph_raster->mosaic_ctx->atlas.image_fence = render_flush_cmd_stream(&glyph_raster->mosaic_ctx->cg_ctx->gl->render_device->default_cmd_stream, render_no_wait);

#endif
        glyph_raster->dirty_region.p00 = (cg_uvec2_t){(uint32_t)-1, (uint32_t)-1};
        glyph_raster->dirty_region.p11 = (cg_uvec2_t){0};
    }
    CG_FONT_TRACE_POP();
}

static codepoint_rasterizing_e mosaic_glyph_raster_try_rasterize_glyphs(mosaic_context_t * const mosaic_ctx, font_glyph_cache_t * const glyph_cache, const char * const text_begin, const char * const text_end) {
    CG_FONT_TRACE_PUSH_FN();
    // tries to rasterize provided codepoints into glyphs but this could actually fail to rasterize any of the provided glyphs,
    // so a subsequent check will be needed to actually verify they're in the atlas (which is part of the normal draw_text_partial loop)
    mosaic_glyph_raster_try_push_codepoint(mosaic_ctx->glyph_raster, (int32_t)' ', glyph_cache);

    codepoint_rasterizing_e rasterize_state = codepoint_rasterizing_renderable_rasterized;
    {
        // make sure to insert the missing glyph indicator, so if we encounter a missing glyph we will have a valid fallback character to use (assuming that this glyph is in the font itself...)
        const int32_t missing_glyph_codepoint = get_missing_glyph_codepoint(mosaic_ctx, &mosaic_ctx->fonts[glyph_cache->font_id]);
        mosaic_glyph_raster_try_push_codepoint(mosaic_ctx->glyph_raster, missing_glyph_codepoint, glyph_cache);
        rasterize_state = mosaic_glyph_raster_rasterize_glyphs(mosaic_ctx->glyph_raster, glyph_cache, missing_glyph_codepoint);
    }

    const char * curr_text = text_begin;
    while (curr_text != text_end) {
        int32_t codepoint;
        const int32_t codepoint_size = utf8_to_codepoint(curr_text, &codepoint);
        if (codepoint_size == 0) {
            break;
        }

        curr_text += codepoint_size;
        if (mosaic_glyph_raster_try_push_codepoint(mosaic_ctx->glyph_raster, codepoint, glyph_cache) == mosaic_raster_insert_no_atlas_space) {
            break;
        }
    }

    int32_t first_codepoint;
    utf8_to_codepoint(text_begin, &first_codepoint);
    rasterize_state = mosaic_glyph_raster_rasterize_glyphs(mosaic_ctx->glyph_raster, glyph_cache, first_codepoint) == codepoint_rasterizing_partial_failure ? codepoint_rasterizing_partial_failure : rasterize_state;
    CG_FONT_TRACE_POP();

    return rasterize_state;
}

typedef struct mosaic_glyph_raster_rebuild_font_atlas_rets_t {
    font_glyph_cache_t * glyph_cache;
    codepoint_rasterizing_e rasterize_state;
} mosaic_glyph_raster_rebuild_font_atlas_rets_t;

static mosaic_glyph_raster_rebuild_font_atlas_rets_t mosaic_glyph_raster_rebuild_font_atlas(mosaic_context_t * const mosaic_ctx, const int32_t font_id, const char * const text_begin, const char * const text_end) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_glyph_raster_t * const glyph_raster = mosaic_ctx->glyph_raster;

    mosaic_glyph_raster_shutdown(glyph_raster);
    mosaic_glyph_raster_emplace_init(glyph_raster, mosaic_ctx);

    mosaic_glyph_raster_rebuild_font_atlas_rets_t rebuild_rets;
    rebuild_rets.glyph_cache = mosaic_glyph_raster_create_glyph_cache(glyph_raster, font_id);

    rebuild_rets.rasterize_state = mosaic_glyph_raster_try_rasterize_glyphs(mosaic_ctx, rebuild_rets.glyph_cache, text_begin, text_end);
    CG_FONT_TRACE_POP();
    return rebuild_rets;
}

void mosaic_context_glyph_raster_debug_draw(const mosaic_context_t * const mosaic_ctx, const int32_t mouse_pos_x, const int32_t mouse_pos_y, const bool draw_rects) {
    CG_FONT_TRACE_PUSH_FN();
    font_glyph_cache_t * glyph_cache = mosaic_ctx->glyph_raster->font_glyph_cache_head;
    while (glyph_cache != NULL) {
        for (int codepoint_ind = 0; codepoint_ind < glyph_cache->codepoints_size; ++codepoint_ind) {
            const codepoint_info_t * const codepoint_info = &glyph_cache->codepoint_infos[codepoint_ind];

            srand((((uint32_t)codepoint_info->tex_coords.x) << 16) | (uint32_t)codepoint_info->tex_coords.y);
            const float r = (float)((rand() % RAND_MAX) / RAND_MAX) * 100.0f + 155.0f;
            const float g = (float)((rand() % RAND_MAX) / RAND_MAX) * 100.0f + 155.0f;
            const float b = (float)((rand() % RAND_MAX) / RAND_MAX) * 100.0f + 155.0f;
            cg_context_fill_style((cg_color_t){.r = r, .g = g, .b = b, .a = 255});

            const int32_t x = codepoint_info->tex_coords.x, y = codepoint_info->tex_coords.y;
            const int32_t w = codepoint_info->tex_coords.width;
            const int32_t h = codepoint_info->tex_coords.height;
            const int32_t mx = (int32_t)mouse_pos_x - x;
            const int32_t my = (int32_t)mouse_pos_y - y;

            if (mx >= 0.0f && mx <= w && my >= 0.0f && my <= h) {
                cg_context_fill_style((cg_color_t){.r = 255, .g = 0, .b = 0, .a = 255});
            }

            const cg_rect_t rect = {.x = (float)x, .y = (float)y, .width = (float)w, .height = (float)h};
            if (draw_rects) {
                cg_context_fill_rect(rect, MALLOC_TAG);
            } else {
                cg_context_draw_image_rect(&mosaic_ctx->atlas.image, rect, rect);
            }
        }
        glyph_cache = glyph_cache->next;
    }
    CG_FONT_TRACE_POP();
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

int32_t mosaic_context_font_load(mosaic_context_t * ctx, cg_font_file_t * const cg_font, const float height, const int32_t tab_space_multiplier) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_font_data_t font_data;
    ZEROMEM(&font_data);

    font_data.cg_font = cg_font;
    ++cg_font->reference_count;

    font_data.ascent = cg_font->font_ascent;

    font_data.scale = stbtt_ScaleForPixelHeight(&font_data.cg_font->font_info, height);

    font_data.height = (int32_t)height;
    font_data.tab_space_multiplier = tab_space_multiplier;

    // try to reuse old font slot
    ctx->font_index = -1;
    for (int32_t i = 0; i < ctx->font_count; ++i) {
        if (ctx->fonts[i].cg_font == NULL) {
            ctx->font_index = i;
            break;
        }
    }

    // old slot wasn't found, resize array
    if (ctx->font_index < 0) {
        CG_ARRAY_PUSH(mosaic_font_data_t, ctx->cg_ctx, ctx->fonts, font_data, MALLOC_TAG);

        ctx->font_index = ctx->font_count;
        ctx->font_count++;
    }

    // copy loaded font
    ctx->fonts[ctx->font_index] = font_data;
    CG_FONT_TRACE_POP();
    return ctx->font_index;
}

/* ------------------------------------------------------------------------- */

typedef enum font_load_type_e {
    font_load_type_file,
    font_load_type_url,
} font_load_type_e;

typedef struct font_async_load_user_t {
    cg_font_file_t * cg_font;
    cg_heap_t * resource_heap;

    bool stb_parsed_font_successfully;

    adk_curl_handle_t * curl_handle;
    cg_allocation_t font_bytes;

    char * url;
    sb_file_t * file;

    font_load_type_e load_type;
    bool hit_oom;
} font_async_load_user_t;

static void font_file_finalize_main_thread(void * const void_user, thread_pool_t * const thread_pool) {
    CG_FONT_TRACE_PUSH_FN();
    font_async_load_user_t * const user = void_user;
    // error reporting and conditional cg_font_file_t deletion

    cg_context_t * const cg_ctx = user->cg_font->cg_ctx;
    CG_FONT_TIME_SPAN_END(user->url);

    if (user->cg_font->async_load_status == cg_font_async_load_aborted) {
        cg_context_font_file_free(user->cg_font, MALLOC_TAG);
    } else if (user->hit_oom) {
        LOG_WARN(TAG_CG_FONT,
                 "Font loading failed due to out of memory.\n"
                 "Could not not finish loading font: [%s]\n"
                 "File size:            [%" PRIu64 "]\n",
                 user->url,
                 (uint64_t)user->font_bytes.region.size);
        user->cg_font->async_load_status = cg_font_async_load_out_of_memory;

    } else if ((user->load_type == font_load_type_file) && (user->file == NULL)) {
        LOG_WARN(TAG_CG_FONT, "Font loading failed. Could not open file: [%s]", user->url);
        user->cg_font->async_load_status = cg_font_async_load_file_error;

    } else if (!user->stb_parsed_font_successfully) {
        user->cg_font->async_load_status = cg_font_async_load_font_parse_error;
    } else if (user->cg_font->async_load_status == cg_font_async_load_pending) {
        user->cg_font->async_load_status = cg_font_async_load_complete;
    }

    cg_free(&cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
    cg_free(&cg_ctx->cg_heap_low, user, MALLOC_TAG);
    CG_FONT_TRACE_POP();
}

static void font_load_job(void * const void_user, thread_pool_t * const thread_pool) {
    CG_FONT_TRACE_PUSH_FN();
    font_async_load_user_t * const user = void_user;

    if (user->cg_font->async_load_status != cg_font_async_load_pending) {
        CG_FONT_TRACE_POP();
        return;
    }

    if (user->load_type == font_load_type_file) {
        user->file = adk_fopen(sb_app_root_directory, user->url, "rb");

        // delay deleting the filename so we can report what file failed to load.
        if (!user->file) {
            CG_FONT_TRACE_POP();
            return;
        }

        const sb_stat_result_t stat_result = adk_stat(sb_app_root_directory, user->url);
        if (stat_result.error != sb_stat_success) {
            adk_fclose(user->file);
            user->file = NULL;
            CG_FONT_TRACE_POP();
            return;
        }

        user->font_bytes = cg_unchecked_alloc(user->resource_heap, stat_result.stat.size, MALLOC_TAG);

        if (user->font_bytes.region.ptr == NULL) {
            adk_fclose(user->file);
            user->hit_oom = true;
            CG_FONT_TRACE_POP();
            return;
        }

        adk_fread(user->font_bytes.region.ptr, 1, user->font_bytes.region.size, user->file);
        adk_fclose(user->file);
    }

    user->cg_font->font_bytes = user->font_bytes.consted;

    user->stb_parsed_font_successfully = stbtt_InitFont(&user->cg_font->font_info, user->cg_font->font_bytes.region.ptr, 0);
    if (!user->stb_parsed_font_successfully) {
        CG_FONT_TRACE_POP();
        return;
    }

    stbtt_GetFontVMetrics(&user->cg_font->font_info, &user->cg_font->font_ascent, 0, 0);
    CG_FONT_TRACE_POP();
}

static void cg_font_load_user_http_failure_cleanup(void * const void_user) {
    font_async_load_user_t * const user = void_user;
    cg_context_t * const cg_ctx = user->cg_font->cg_ctx;
    adk_curl_close_handle(user->curl_handle);
    if (user->font_bytes.region.ptr) {
        cg_free_alloc(user->font_bytes, MALLOC_TAG);
    }
    cg_free(&cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
    cg_free(&cg_ctx->cg_heap_low, user, MALLOC_TAG);
}

static void url_font_http_complete(adk_curl_handle_t * const handle, const adk_curl_result_e result, const adk_curl_callbacks_t * const callbacks) {
    CG_FONT_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();
    font_async_load_user_t * const user = callbacks->user[0];
    user->cg_font->load_user = NULL;
    cg_context_t * const cg_ctx = user->cg_font->cg_ctx;

    long http_status_code = 0;
    CG_FONT_TIME_SPAN_END(user->url);
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
    if ((result == adk_curl_result_ok) && found_expected_http_status) {
        CG_FONT_TIME_SPAN_BEGIN(user->url, "[file] %s", user->url);
        thread_pool_enqueue(cg_ctx->thread_pool, font_load_job, font_file_finalize_main_thread, user); // what do we even sanely do here? font loading appears to be directly bound to the main thread.
    } else {
        if (result != adk_curl_result_ok) {
            if (user->hit_oom) {
                LOG_WARN(TAG_CG_FONT, "Ran out of memory when attempting to download font at [%s]", user->url);
                user->cg_font->async_load_status = cg_font_async_load_out_of_memory;
            } else {
                LOG_WARN(TAG_CG_FONT, "Encountered an error while trying to fetch a font at [%s] adk_curl error code: [%i]", user->url, result);
                user->cg_font->async_load_status = cg_font_async_load_http_fetch_error;
            }
        } else {
            LOG_WARN(TAG_CG_FONT, "Encountered an error while trying to fetch a font at [%s] http status: [%i]", user->url, http_status_code);
            user->cg_font->async_load_status = cg_font_async_load_http_fetch_error;
        }
        if (user->font_bytes.region.ptr) {
            cg_free_alloc(user->font_bytes, MALLOC_TAG);
        }
        cg_free(&cg_ctx->cg_heap_low, user->url, MALLOC_TAG);
        cg_free(&cg_ctx->cg_heap_low, user, MALLOC_TAG);
    }
    adk_curl_close_handle(handle);
    CG_FONT_TRACE_POP();
}

static bool url_font_http_receive(adk_curl_handle_t * const handle, const const_mem_region_t bytes, const adk_curl_callbacks_t * const callbacks) {
    CG_FONT_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();
    font_async_load_user_t * const user = callbacks->user[0];
    ASSERT(!user->hit_oom);
    const cg_allocation_t alloc = cg_unchecked_realloc(user->resource_heap, user->font_bytes, user->font_bytes.region.size + bytes.size, MALLOC_TAG);
    if (!alloc.region.ptr) {
        if (user->font_bytes.region.ptr) {
            cg_free_alloc(user->font_bytes, MALLOC_TAG);
        }
        user->font_bytes.region = (mem_region_t){0};
        user->hit_oom = true;
        CG_FONT_TRACE_POP();
        return false;
    }
    memcpy(alloc.region.byte_ptr + user->font_bytes.region.size, bytes.ptr, bytes.size);
    user->font_bytes = alloc;
    CG_FONT_TRACE_POP();
    return true;
}

cg_font_file_t * cg_context_load_font_file_async(const char * const file_location, const cg_memory_region_e memory_region, const cg_font_load_opts_e font_load_opts, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;

    font_async_load_user_t * const font_user = cg_alloc(&ctx->cg_heap_low, sizeof(font_async_load_user_t), tag);
    ZEROMEM(font_user);

    font_user->resource_heap = memory_region == cg_memory_region_high ? &ctx->cg_heap_high : &ctx->cg_heap_low;
    font_user->cg_font = cg_alloc(&ctx->cg_heap_low, sizeof(cg_font_file_t), MALLOC_TAG);
    ZEROMEM(font_user->cg_font);

    font_user->cg_font->id = ctx->mosaic_ctx->font_id_counter++;
    font_user->cg_font->cg_ctx = ctx;
    font_user->cg_font->reference_count = 1;
    font_user->cg_font->async_load_status = cg_font_async_load_pending;

    {
        const size_t name_length = strlen(file_location) + 1;
        font_user->url = cg_alloc(&ctx->cg_heap_low, name_length, MALLOC_TAG);
        memcpy(font_user->url, file_location, name_length);
    }

    if (strstr(file_location, "://") != NULL) {
        adk_curl_handle_t * const handle = adk_curl_open_handle();
        adk_curl_set_opt_ptr(handle, adk_curl_opt_url, (void *)file_location);
        adk_curl_set_opt_long(handle, adk_curl_opt_follow_location, 1);
        if (font_load_opts & cg_font_load_opts_http_verbose) {
            adk_curl_set_opt_long(handle, adk_curl_opt_verbose, 1);
        }

        const adk_curl_callbacks_t callbacks = {
            .on_http_header_recv = NULL,
            .on_http_recv = url_font_http_receive,
            .on_complete = url_font_http_complete,
            .user = {font_user}};

        font_user->curl_handle = handle;
        font_user->cg_font->load_user = font_user;
        font_user->load_type = font_load_type_url;
        CG_FONT_TIME_SPAN_BEGIN(font_user->url, "[url] %s", font_user->url);
        adk_curl_async_perform(handle, callbacks);
    } else {
        font_user->load_type = font_load_type_file;
        CG_FONT_TIME_SPAN_BEGIN(font_user->url, "[file] %s", font_user->url);
        thread_pool_enqueue(ctx->thread_pool, font_load_job, font_file_finalize_main_thread, font_user);
    }

    CG_FONT_TRACE_POP();
    return font_user->cg_font;
}

cg_font_async_load_status_e cg_get_font_load_status(const cg_font_file_t * const cg_font) {
    return cg_font->async_load_status;
}

/* ------------------------------------------------------------------------- */

static void text_mesh_cache_init_emplace(text_mesh_cache_t * const cache, uint32_t size, const char * const tag);

mosaic_context_t * mosaic_context_new(
    cg_context_t * const cg_ctx,
    const int32_t width,
    const int32_t height,
    const mem_region_t canvas_font_scratchpad,
    const system_guard_page_mode_e guard_page_mode,
    const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_context_t * const mosaic_ctx = cg_alloc(&cg_ctx->cg_heap_low, sizeof(mosaic_context_t), tag);
    ZEROMEM(mosaic_ctx);

    mosaic_ctx->cg_ctx = cg_ctx;
    mosaic_ctx->max_width = width;
    mosaic_ctx->max_height = height;

    if (cg_statics.ctx->config.text_mesh_cache.enabled) {
        text_mesh_cache_init_emplace(&mosaic_ctx->text_mesh_cache, cg_statics.ctx->config.text_mesh_cache.size, MALLOC_TAG);
    }

#ifdef GUARD_PAGE_SUPPORT
    mosaic_ctx->guard_page_mode = guard_page_mode;
    if (guard_page_mode == system_guard_page_mode_enabled) {
        const size_t needed = ALIGN_INT(canvas_font_scratchpad.size, 8);
        const size_t page_size = get_sys_page_size();
        const size_t block_size = ALIGN_INT(needed, page_size);
        const size_t total_size = block_size + page_size * 2;

        mosaic_ctx->guard_pages = debug_sys_map_pages(total_size, system_page_protect_no_access, MALLOC_TAG);
        VERIFY(mosaic_ctx->guard_pages.ptr);

        const uintptr_t uptr = mosaic_ctx->guard_pages.adr;
        debug_sys_protect_pages(MEM_REGION(.adr = uptr + page_size, .size = block_size), system_page_protect_read_write);

        const uintptr_t ublock = uptr + page_size + block_size - needed;

        lba_init(&mosaic_ctx->atlas_lba, (void *)ublock, (int)canvas_font_scratchpad.size, "font_atlas_lba");
    } else
#endif
    {
        lba_init(&mosaic_ctx->atlas_lba, canvas_font_scratchpad.ptr, (int)canvas_font_scratchpad.size, "font_atlas_lba");
    }

    const size_t atlas_size = width * height * 1;
    mosaic_ctx->atlas.image.pixel_buffer = (cg_allocation_t){{{.cg_heap = &cg_ctx->cg_heap_low, .region = MEM_REGION(.ptr = cg_alloc(&cg_ctx->cg_heap_low, atlas_size, MALLOC_TAG), .size = atlas_size)}}};

#if !(defined(_VADER) || defined(_LEIA))
    mosaic_ctx->atlas.upload_region.buffer_size = default_atlas_upload_queue_limit;
    mosaic_ctx->atlas.upload_region.buffer = cg_alloc(&cg_ctx->cg_heap_low, mosaic_ctx->atlas.upload_region.buffer_size * sizeof(font_atlas_upload_region_t), MALLOC_TAG);
    for (uint32_t i = 0; i < mosaic_ctx->atlas.upload_region.buffer_size; ++i) {
        LL_ADD(&mosaic_ctx->atlas.upload_region.buffer[i], prev, next, mosaic_ctx->atlas.upload_region.free_head, mosaic_ctx->atlas.upload_region.free_tail);
    }

    mosaic_ctx->atlas.sub_image_copy_buffer = (cg_allocation_t){{{.cg_heap = &cg_ctx->cg_heap_low, .region = MEM_REGION(.ptr = cg_alloc(&cg_ctx->cg_heap_low, atlas_size, MALLOC_TAG), .size = atlas_size)}}};

    heap_init_with_region(&mosaic_ctx->atlas.sub_image_heap, mosaic_ctx->atlas.sub_image_copy_buffer.region, 8, 0, "mosaic_ctx_atlas_sub_image_heap");
#endif
    mosaic_ctx->atlas.image.status = cg_image_async_load_complete;
    mosaic_ctx->atlas.image.image = (image_t){
        .encoding = image_encoding_uncompressed,
        .width = width,
        .height = height,
        .depth = 1,
        .bpp = 1,
        .pitch = 1 * width,
        .spitch = width * height * 1,
        .data_len = width * height * 1,
        .data = mosaic_ctx->atlas.image.pixel_buffer.region.ptr,
    };

    const rhi_sampler_state_desc_t atlas_sampler_state = (rhi_sampler_state_desc_t){
        .max_anisotropy = 1,
        .min_filter = rhi_min_filter_linear,
        .max_filter = rhi_max_filter_linear,
        .u_wrap_mode = rhi_wrap_mode_clamp_to_edge,
        .v_wrap_mode = rhi_wrap_mode_clamp_to_edge,
        .w_wrap_mode = rhi_wrap_mode_clamp_to_edge,
        .border_color = {1, 1, 1, 1}};

    image_mips_t mipmaps;
    ZEROMEM(&mipmaps);
    mipmaps.num_levels = 1;
    mipmaps.levels[0] = mosaic_ctx->atlas.image.image;
    mipmaps.levels[0].data = NULL;

    mosaic_ctx->atlas.image.cg_texture.texture = render_create_texture_2d(cg_ctx->gl->render_device, mipmaps, rhi_pixel_format_r8_unorm, rhi_usage_dynamic, atlas_sampler_state, MALLOC_TAG);

    mosaic_ctx->glyph_raster = cg_alloc(&cg_ctx->cg_heap_low, sizeof(mosaic_glyph_raster_t), MALLOC_TAG);
    mosaic_glyph_raster_emplace_init(mosaic_ctx->glyph_raster, mosaic_ctx);

    CG_ARRAY(mosaic_font_data_t, mosaic_ctx->cg_ctx, mosaic_ctx->fonts, MALLOC_TAG);

    CG_FONT_TRACE_POP();
    return mosaic_ctx;
}

void mosaic_context_font_free(mosaic_context_t * ctx, int32_t index) {
    CG_FONT_TRACE_PUSH_FN();
    if (ctx->fonts[index].cg_font) {
        cg_context_font_file_free(ctx->fonts[index].cg_font, MALLOC_TAG);
        ZEROMEM(&ctx->fonts[index]);
    }
    CG_FONT_TRACE_POP();
}

static void text_mesh_cache_free(text_mesh_cache_t * const cache, const char * const tag);

void mosaic_context_free(mosaic_context_t * ctx) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_glyph_raster_shutdown(ctx->glyph_raster);

#ifdef GUARD_PAGE_SUPPORT
    if (ctx->guard_page_mode == system_guard_page_mode_enabled) {
        debug_sys_unmap_pages(ctx->guard_pages, MALLOC_TAG);
    }
#endif

    cg_free(&ctx->cg_ctx->cg_heap_low, ctx->glyph_raster, MALLOC_TAG);
    cg_gl_texture_free(ctx->cg_ctx->gl, &ctx->atlas.image.cg_texture);
    cg_free_alloc(ctx->atlas.image.pixel_buffer, MALLOC_TAG);

#if !(defined(_VADER) || defined(_LEIA))
    // sub_image_heap has allocations lazily freed, so any reporting here would report leaks (that are not relavant)
    // the underlying memory will be freed by freeing sub_image_copy_buffer, and the heap will be gone when the mosaic_context goes away.
    cg_free_alloc(ctx->atlas.sub_image_copy_buffer, MALLOC_TAG);

    cg_free(&ctx->cg_ctx->cg_heap_low, ctx->atlas.upload_region.buffer, MALLOC_TAG);
#endif

    for (int i = 0; i < ctx->font_count; ++i) {
        mosaic_context_font_free(ctx, i);
    }
    CG_ARRAY_FREE(ctx->cg_ctx, ctx->fonts, MALLOC_TAG);

    if (cg_statics.ctx->config.text_mesh_cache.enabled) {
        text_mesh_cache_free(&ctx->text_mesh_cache, MALLOC_TAG);
    }

    cg_free(&ctx->cg_ctx->cg_heap_low, ctx, MALLOC_TAG);
    CG_FONT_TRACE_POP();
}

/* ------------------------------------------------------------------------- */

void mosaic_context_font_bind(mosaic_context_t * ctx, int32_t index) {
    VERIFY(index >= 0 && index < ctx->font_count);

    ctx->font_index = index;
}

/* ------------------------------------------------------------------------- */

static void find_linebreak_position(
    const mosaic_context_t * const mosaic_ctx,
    const float max_line_width,
    const char * const text,
    const char ** const out_text_linebreak_location,
    float * const out_last_renderable_width,
    float * const out_line_width) {
    CG_FONT_TRACE_PUSH_FN();
    const char * curr_text_location = text;
    *out_text_linebreak_location = text;
    float curr_line_width = 0;
    float line_width_no_white_space = 0;
    *out_last_renderable_width = 0;
    *out_line_width = 0;

    const mosaic_font_data_t * const mosaic_font = &mosaic_ctx->fonts[mosaic_ctx->font_index];
    const stbtt_fontinfo * const stbtt_font_info = &mosaic_font->cg_font->font_info;
    const float font_scale = mosaic_font->scale;

    // calculate the length of a line..
    //   up to the last breaching character
    //   the last breaching word
    //   save at the last position after a non white space character

    // if the start of our text is a new line, immediately bail and move to the next character.
    if (is_newline(*curr_text_location)) {
        *out_text_linebreak_location = curr_text_location + 1;
        CG_FONT_TRACE_POP();
        return;
    }

    int32_t x_advance, ignored;
    const int32_t missing_glyph_codepoint = get_missing_glyph_codepoint(mosaic_ctx, mosaic_font);
    const int32_t missing_glyph_index = stbtt_FindGlyphIndex(stbtt_font_info, missing_glyph_codepoint);
    stbtt_GetGlyphHMetrics(stbtt_font_info, missing_glyph_index, &x_advance, &ignored);
    const int32_t missing_glyph_codepoint_width = x_advance;

    const int32_t space_glyph_index = stbtt_FindGlyphIndex(stbtt_font_info, (int32_t)' ');
    stbtt_GetGlyphHMetrics(stbtt_font_info, space_glyph_index, &x_advance, &ignored);
    const int32_t tab_width = x_advance * mosaic_font->tab_space_multiplier;

    const char * last_codepoint_location = curr_text_location;

    while (*curr_text_location) {
        int32_t curr_codepoint, next_codepoint;
        last_codepoint_location = curr_text_location;
        curr_text_location += utf8_to_codepoint(curr_text_location, &curr_codepoint);
        utf8_to_codepoint(curr_text_location, &next_codepoint);
        int32_t curr_glyph_index = stbtt_FindGlyphIndex(stbtt_font_info, curr_codepoint);
        const int32_t next_glyph_index = stbtt_FindGlyphIndex(stbtt_font_info, next_codepoint);

        if ((curr_codepoint != '\t') && curr_glyph_index) {
            stbtt_GetGlyphHMetrics(stbtt_font_info, curr_glyph_index, &x_advance, &ignored);
        } else if ((curr_codepoint != '\t') && !curr_glyph_index) {
            curr_glyph_index = curr_glyph_index ? curr_glyph_index : missing_glyph_index;
            x_advance = missing_glyph_codepoint_width;
        } else {
            x_advance = tab_width;
        }

        curr_line_width += font_scale * x_advance;
        curr_line_width += font_scale * stbtt_GetGlyphKernAdvance(stbtt_font_info, curr_codepoint != '\t' ? curr_glyph_index : space_glyph_index, next_glyph_index);

        if (curr_line_width > max_line_width) {
            break;
        }

        if (!is_whitespace(curr_codepoint) && !is_newline(curr_codepoint)) {
            line_width_no_white_space = curr_line_width;
        }

        if (is_newline(next_codepoint)) {
            if (!is_whitespace(curr_codepoint)) {
                *out_last_renderable_width = line_width_no_white_space;
                *out_text_linebreak_location = curr_text_location;
            }
            break;
        }

        if ((!is_whitespace(curr_codepoint) && is_whitespace(next_codepoint)) || !is_whitespace(**out_text_linebreak_location)) {
            *out_last_renderable_width = line_width_no_white_space;
            *out_text_linebreak_location = curr_text_location;
        }
    }

    if ((curr_line_width < max_line_width) && (*curr_text_location == '\0') && !(is_whitespace(*last_codepoint_location) || is_newline(*last_codepoint_location))) {
        *out_last_renderable_width = curr_line_width;
        *out_text_linebreak_location = curr_text_location;
    }

    *out_line_width = curr_line_width;
    CG_FONT_TRACE_POP();
}

typedef enum draw_text_status_e {
    draw_text_incomplete = -4, // only used for initializing
    draw_text_codepoint_not_in_cache = -3, // codepoint is missing from glyph cache, it is plausible to still fit it in without an explicit atlas flush.
    draw_text_glyph_not_in_atlas = -2, // encountered a missing glyph, and we need to flush, render, and clear the atlas to continue drawing
    draw_text_no_more_indices = -1, // no more space in the existing vertex buffer, buffer must be flushed, rendered, and reset

    draw_text_complete = 0, // count the entire text line as consumed (either the glyphs were rendered to the screen, or culled because they would show up off screen)
} draw_text_status_e;
// returns a `draw_partial_text_return_e` indicating completion (or 'error' status)
// if not all codepoints were drawn then `out_last_codepoint` and `out_last_x_offset` will point to the next drawable character and the offset to draw it at
static draw_text_status_e draw_partial_text(
    mosaic_context_t * const mosaic_ctx,
    font_glyph_cache_t * const glyph_cache,
    text_mesh_t * const mesh,
    const cg_affine_t * const transform,
    const cg_vec2_t offset,
    const float max_width,
    const char * const text_begin,
    const char * const text_end,
    const char ** const out_last_codepoint,
    float * const out_last_x_offset) {
    CG_FONT_TRACE_PUSH_FN();
    *out_last_codepoint = text_begin;
    *out_last_x_offset = offset.x;

    if (is_newline(**out_last_codepoint)) {
        ++*out_last_codepoint;
        CG_FONT_TRACE_POP();
        return draw_text_complete;
    }

    const float uv_inverse_width = 1.0f / mosaic_ctx->max_width;
    const float uv_inverse_height = 1.0f / mosaic_ctx->max_height;

    const float draw_style_alpha = mosaic_ctx->cg_ctx->cur_state->fill_style.a;

    const mosaic_font_data_t * const mosaic_font = &mosaic_ctx->fonts[mosaic_ctx->font_index];
    const stbtt_fontinfo * const stbtt_font_info = &mosaic_font->cg_font->font_info;
    const float font_scale = mosaic_font->scale;
    const float font_baseline = mosaic_font->ascent * font_scale;

    const int32_t missing_glyph_codepoint = get_missing_glyph_codepoint(mosaic_ctx, mosaic_font);
    const int32_t missing_glyph_index = stbtt_FindGlyphIndex(stbtt_font_info, missing_glyph_codepoint);
    const codepoint_info_t * const missing_glyph_codepoint_info = font_glyph_cache_find_codepoint_info(missing_glyph_index ? missing_glyph_codepoint : ' ', glyph_cache);

    float curr_width = offset.x;

    while (*out_last_codepoint != text_end) {
        *out_last_x_offset = curr_width;

        if ((mesh->vert_index + verts_per_quad > mesh->reserved_verts)) {
            CG_FONT_TRACE_POP();
            return draw_text_no_more_indices;
        }

        int32_t curr_codepoint;
        const char * const curr_text = *out_last_codepoint + utf8_to_codepoint(*out_last_codepoint, &curr_codepoint);

        const bool is_tab = curr_codepoint == '\t';
        const codepoint_info_t * codepoint_info = font_glyph_cache_find_codepoint_info(!is_tab ? curr_codepoint : (int32_t)' ', glyph_cache);
        if (codepoint_info == NULL) {
            CG_FONT_TRACE_POP();
            return draw_text_codepoint_not_in_cache;
        } else if ((codepoint_info->state == codepoint_no_backing_glyph) && missing_glyph_codepoint_info && (missing_glyph_codepoint_info->state == codepoint_rasterized) && !is_control_character(curr_codepoint)) {
            codepoint_info = missing_glyph_codepoint_info;
        } else if (codepoint_info->state != codepoint_rasterized) {
            CG_FONT_TRACE_POP();
            return draw_text_glyph_not_in_atlas;
        }

        *out_last_codepoint = curr_text;

        // 1px padding
        const float x = codepoint_info->tex_coords.x, y = codepoint_info->tex_coords.y;
        const float w = codepoint_info->tex_coords.width, h = codepoint_info->tex_coords.height;
        const float x0 = curr_width + codepoint_info->x_off;
        const float y0 = offset.y + codepoint_info->y_off + font_baseline;

        const float x1 = x0 + w;
        const float y1 = y0 + h;

        curr_width += codepoint_info->x_advance * ((is_tab) ? mosaic_font->tab_space_multiplier : 1);

        const int32_t next_codepoint = **out_last_codepoint;
        if (next_codepoint) {
            curr_width += font_scale * stbtt_GetCodepointKernAdvance(stbtt_font_info, curr_codepoint, next_codepoint);
        }

        if (is_whitespace(curr_codepoint)) {
            continue;
        }

        const cg_vec2_t * const quad_tl = cg_affine_apply(transform, cg_vec2(x0, y0));
        const cg_vec2_t * const quad_tr = cg_affine_apply(transform, cg_vec2(x1, y0));
        const cg_vec2_t * const quad_br = cg_affine_apply(transform, cg_vec2(x1, y1));
        const cg_vec2_t * const quad_bl = cg_affine_apply(transform, cg_vec2(x0, y1));

        const float u0 = x * uv_inverse_width;
        const float v0 = y * uv_inverse_height;

        const float u1 = u0 + w * uv_inverse_width;
        const float v1 = v0 + h * uv_inverse_height;

        cg_set_vert(mesh->verts, mesh->vert_index++, quad_tl, cg_color(u0, v0, 0.0f, draw_style_alpha));
        cg_set_vert(mesh->verts, mesh->vert_index++, quad_tr, cg_color(u1, v0, 0.0f, draw_style_alpha));
        cg_set_vert(mesh->verts, mesh->vert_index++, quad_br, cg_color(u1, v1, 0.0f, draw_style_alpha));

        cg_set_vert(mesh->verts, mesh->vert_index++, quad_br, cg_color(u1, v1, 0.0f, draw_style_alpha));
        cg_set_vert(mesh->verts, mesh->vert_index++, quad_bl, cg_color(u0, v1, 0.0f, draw_style_alpha));
        cg_set_vert(mesh->verts, mesh->vert_index++, quad_tl, cg_color(u0, v0, 0.0f, draw_style_alpha));

        ++mesh->glyphs_drawn;
    }
    *out_last_x_offset = curr_width;
    CG_FONT_TRACE_POP();
    return draw_text_complete;
}

static void flush_and_draw_mesh(mosaic_context_t * const mosaic_ctx, text_mesh_t * const text_mesh) {
    CG_FONT_TRACE_PUSH_FN();
    cg_context_t * const cg_ctx = mosaic_ctx->cg_ctx;
    if (text_mesh->vert_index == 0) {
        CG_FONT_TRACE_POP();
        return;
    }
    // force commit per string
    mosaic_glyph_raster_flush_atlas(mosaic_ctx->glyph_raster);

    cg_gl_state_finish_vertex_range_with_count(cg_ctx->gl, text_mesh->reserved_verts);

    cg_select_blend_and_shader(cg_ctx->gl, cg_ctx->cur_state, &cg_ctx->cur_state->fill_style, &mosaic_ctx->atlas.image.cg_texture, cg_rgb_fill_alpha_red_enabled);
    cg_gl_state_draw(cg_ctx->gl, rhi_triangles, text_mesh->vert_index, 0);

    if (text_mesh->glyphs_drawn < text_mesh->total_glyphs) {
        text_mesh->reserved_verts = min_int((text_mesh->total_glyphs - text_mesh->glyphs_drawn) * verts_per_quad, cg_ctx->gl->config.internal_limits.max_verts_per_vertex_bank);
        text_mesh->verts = cg_gl_state_map_vertex_range(cg_ctx->gl, text_mesh->reserved_verts);
        text_mesh->vert_index = 0;
    }
    CG_FONT_TRACE_POP();
}

static float calculate_line_offset(const float box_width, const float line_width, const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    if (options & cg_text_block_align_line_center) {
        CG_FONT_TRACE_POP();
        return (box_width - line_width) / 2.0f;
    } else if (options & cg_text_block_align_line_right) {
        CG_FONT_TRACE_POP();
        return box_width - line_width;
    }
    CG_FONT_TRACE_POP();
    return 0.0f;
}

typedef struct line_height_args_t {
    float curr_height;
    float line_height;
    float max_height;
    float max_line_width;
    float space_width;
    float tab_width;
    float ellipses_width;
    const char * text;
} line_height_args_t;

static float line_height_at_next_word(const mosaic_context_t * const mosaic_ctx, const line_height_args_t args) {
    CG_FONT_TRACE_PUSH_FN();
    // this function is used to check the line height at the next word..
    // we need this in the event that we have many 'blank' lines in a row and are meant to place ellipses.
    // so if we have a 'bad' input of something like... "some word\n\r\r\n                                      \n\n\nanother word"
    // where we could have our text blocks' max height be blown somewhere before 'another'
    // the new lines will directly contribute to the over all height
    // however space/tabs will only count if they are at the start of a valid line
    // we may have sufficient spaces/tabs to force our normal line to constitute a line by themselves
    // in that event we need to drop any extra spaces/tabs after that line threshold is reached.
    if (!is_whitespace((int32_t)*args.text) && !is_newline((int32_t)*args.text)) {
        CG_FONT_TRACE_POP();
        return args.curr_height;
    }

    float curr_line_height = args.curr_height;
    float curr_line_width = 0.f;
    bool got_space_contribution_for_this_line = false;
    const char * curr_text = args.text;
    while (*curr_text && (curr_line_height < args.max_height)) {
        // check for new lines, and increment our line height (and reset space/tab contributions)
        if (is_newline((int32_t)*curr_text)) {
            curr_line_width = 0;
            got_space_contribution_for_this_line = false;

            // check CRLF and count it as only one new line.
            if ((*curr_text == '\r') && (*(curr_text + 1) == '\n')) {
                curr_text += 2;
            } else {
                ++curr_text;
            }
            curr_line_height += args.line_height;

            // check for space/tab and determine if we need to create another line due to leading whitespace on a new line -- but only once for that line.
        } else if (is_whitespace((int32_t)*curr_text)) {
            if (*curr_text == ' ') {
                curr_line_width += args.space_width;
            } else {
                curr_line_width += args.tab_width;
            }
            if ((curr_line_width > args.max_line_width) && !got_space_contribution_for_this_line) {
                got_space_contribution_for_this_line = true;
                curr_line_width = 0;
                curr_line_height += args.line_height;
            }
            ++curr_text;

            // we found a non white space character (word or other)
        } else {
            break;
        }
        CG_FONT_TRACE_POP();
    }

    // if our curr_line_width is > 0 check to see if the ellipses would be able to fit at the current white-spaced position
    if (curr_line_width + args.ellipses_width > args.max_line_width) {
        curr_line_height += args.line_height;

        // if not, verify the word won't breach the threshold for ellipses and won't get dropped..
    } else if (curr_line_width > 0) {
        const char * linebreak_position;
        float width_ignored;
        float line_width_whitespace_ignored;

        // check to see if we find a valid linebreak location for this line, given we already have a 'valid' linebreak location (sine this line begins with a tab/space)
        // this can/may over check to the next set of valid words. regadless if linebreak_position is whitespace/newline then we have found a valid breaking spot that could support our words + ellipses
        find_linebreak_position(mosaic_ctx, args.max_line_width - args.ellipses_width - curr_line_width, curr_text, &linebreak_position, &width_ignored, &line_width_whitespace_ignored);

        // if we can't consume all the remaining text and aren't on whitespace/newline then increment height as this line would get skipped
        if (*linebreak_position && !is_whitespace(*linebreak_position) && !is_newline(*linebreak_position)) {
            curr_line_height += args.line_height;
        }
    }
    CG_FONT_TRACE_POP();
    return curr_line_height;
}

static void skip_trailing_white_space_and_first_newline(const char * const start_of_line, const char ** const end_of_line) {
    CG_FONT_TRACE_PUSH_FN();
    // check to make sure we're not going to overly consume newlines
    if (is_newline(*start_of_line)) {
        CG_FONT_TRACE_POP();
        return;
    }

    // skip spacing characters at the end of the line, and the first new line we'd encounter.
    while (is_whitespace((int32_t) * *end_of_line)) {
        ++*end_of_line;
    }

    if (is_newline((int32_t) * *end_of_line)) {
        // check CRLF and count it as only one new line.
        if ((**end_of_line == '\r') && (*(*end_of_line + 1) == '\n')) {
            *end_of_line += 2;
        } else {
            ++*end_of_line;
        }
    }
    CG_FONT_TRACE_POP();
}

cg_rect_t mosaic_context_draw_text_block(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const char * const optional_ellipses,
    const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    cg_rect_t box = {.x = text_rect.x, .y = text_rect.y, .width = 0.0f, .height = 0.0f};
    if (*text == '\0') {
        ZEROMEM(&box);
        CG_FONT_TRACE_POP();
        return box;
    }

    const mosaic_font_data_t * const font = &mosaic_ctx->fonts[mosaic_ctx->font_index];

    font_glyph_cache_t * glyph_cache = mosaic_glyph_raster_find_glyph_cache(mosaic_ctx->glyph_raster, mosaic_ctx->font_index);
    if (glyph_cache == NULL) {
        glyph_cache = mosaic_glyph_raster_create_glyph_cache(mosaic_ctx->glyph_raster, mosaic_ctx->font_index);
    }

    const float font_height = (float)font->height;
    const float line_height = (options & cg_text_block_line_space_relative) ? (font_height * extra_line_spacing) : (font_height + extra_line_spacing);
    ASSERT(fabs(line_height) >= 1.0f);

    const cg_context_t * const cg_ctx = mosaic_ctx->cg_ctx;

    const int text_len = (int)strlen(text);
    const int initial_reserved_verts = min_int(text_len * verts_per_quad, cg_ctx->gl->config.internal_limits.max_verts_per_vertex_bank);
    text_mesh_t text_mesh = {
        .reserved_verts = initial_reserved_verts,
        .vert_index = 0,
        .verts = cg_gl_state_map_vertex_range(cg_ctx->gl, initial_reserved_verts),
        .total_glyphs = text_len,
        .glyphs_drawn = 0};

    float ellipses_width = 0;
    if (optional_ellipses) {
        const char * linebreak_ignored;
        float line_width_ignored = 0;
        find_linebreak_position(mosaic_ctx, INFINITY, optional_ellipses, &linebreak_ignored, &ellipses_width, &line_width_ignored);
    }

    int32_t space_width_int, tab_width_int, lsb_ignored;
    stbtt_GetCodepointHMetrics(&font->cg_font->font_info, ' ', &space_width_int, &lsb_ignored);
    stbtt_GetCodepointHMetrics(&font->cg_font->font_info, '\t', &tab_width_int, &lsb_ignored);

    const float space_width = font->scale * space_width_int;
    const float tab_width = font->scale * tab_width_int;

    float curr_y = text_rect.y + scroll_offset;
    const float max_y = text_rect.y + text_rect.height;

    const char * curr_text_position = text;
    const float displayable_glyph_width = min_float(text_rect.width, cg_ctx->width - text_rect.x);

    while (*curr_text_position) {
        const char * linebreak_position = NULL;
        float last_renderable_width = 0;
        float line_width = 0;
        find_linebreak_position(mosaic_ctx, text_rect.width, curr_text_position, &linebreak_position, &last_renderable_width, &line_width);

        // if we would render glyphs above the box cull them, if its intersecting cull only if allow_bounds_overflow isn't enabled.
        if (options & cg_text_block_allow_block_bounds_overflow) {
            if (curr_y < text_rect.y - line_height) {
                curr_y += line_height;
                continue;
            }
        } else {
            if (curr_y < text_rect.y) {
                curr_y += line_height;
                continue;
            }
        }

        const float ellipses_height_threshold = curr_y + line_height + ((options & cg_text_block_allow_block_bounds_overflow) ? 0 : line_height);
        const bool use_ellipses = (*linebreak_position != '\0')
                                  && optional_ellipses
                                  && ((ellipses_height_threshold > max_y)
                                      || (line_height_at_next_word(
                                              mosaic_ctx,
                                              (line_height_args_t){
                                                  .curr_height = curr_y + line_height,
                                                  .line_height = line_height,
                                                  .max_height = max_y,
                                                  .max_line_width = text_rect.width,
                                                  .space_width = space_width,
                                                  .tab_width = tab_width,
                                                  .ellipses_width = ellipses_width,
                                                  .text = linebreak_position})
                                          >= max_y));

        // if ellipses are enabled, and we need to draw them re-calculate the line's cut off position if we can't fit the ellipses on the existing line.
        if (use_ellipses && (last_renderable_width + ellipses_width > text_rect.width)) {
            find_linebreak_position(mosaic_ctx, text_rect.width - ellipses_width, curr_text_position, &linebreak_position, &last_renderable_width, &line_width);
        }

        const float line_offset = calculate_line_offset(text_rect.width, last_renderable_width + (use_ellipses ? ellipses_width : 0), options);
        float curr_x = text_rect.x + line_offset;

        const char * const start_of_line = curr_text_position;

        draw_text_status_e draw_text_status = draw_text_incomplete;
        while (draw_text_status != draw_text_complete) {
            draw_text_status = draw_partial_text(
                mosaic_ctx,
                glyph_cache,
                &text_mesh,
                &cg_ctx->cur_state->transform,
                (cg_vec2_t){.x = curr_x, .y = curr_y},
                displayable_glyph_width,
                curr_text_position,
                linebreak_position,
                &curr_text_position,
                &curr_x);

            if (draw_text_status == draw_text_codepoint_not_in_cache) {
                mosaic_glyph_raster_try_rasterize_glyphs(mosaic_ctx, glyph_cache, curr_text_position, linebreak_position);

            } else if (draw_text_status == draw_text_glyph_not_in_atlas) {
                flush_and_draw_mesh(mosaic_ctx, &text_mesh);
                glyph_cache = mosaic_glyph_raster_rebuild_font_atlas(mosaic_ctx, mosaic_ctx->font_index, curr_text_position, linebreak_position).glyph_cache;

            } else if (draw_text_status == draw_text_no_more_indices) {
                flush_and_draw_mesh(mosaic_ctx, &text_mesh);

            } else if (!use_ellipses) {
                ASSERT(draw_text_status == draw_text_complete);

                curr_x = text_rect.x;
                curr_y += line_height;

                if (box.width < line_width) {
                    box.width = line_width;
                    box.x = text_rect.x + line_offset;
                }
                box.height = curr_y - text_rect.y;
                skip_trailing_white_space_and_first_newline(start_of_line, &curr_text_position);
            }
        }
        if (use_ellipses) {
            draw_text_status = draw_text_incomplete;
            const char * curr_ellipses_position = optional_ellipses;
            const char * const end_of_ellipses = optional_ellipses + strlen(optional_ellipses);
            while (draw_text_status != draw_text_complete) {
                draw_text_status = draw_partial_text(
                    mosaic_ctx,
                    glyph_cache,
                    &text_mesh,
                    &cg_ctx->cur_state->transform,
                    (cg_vec2_t){.x = curr_x, .y = curr_y},
                    displayable_glyph_width,
                    curr_ellipses_position,
                    end_of_ellipses,
                    &curr_ellipses_position,
                    &curr_x);

                if (draw_text_status == draw_text_codepoint_not_in_cache) {
                    mosaic_glyph_raster_try_rasterize_glyphs(mosaic_ctx, glyph_cache, curr_ellipses_position, end_of_ellipses);

                } else if (draw_text_status == draw_text_glyph_not_in_atlas) {
                    flush_and_draw_mesh(mosaic_ctx, &text_mesh);
                    glyph_cache = mosaic_glyph_raster_rebuild_font_atlas(mosaic_ctx, mosaic_ctx->font_index, curr_ellipses_position, end_of_ellipses).glyph_cache;

                } else if (draw_text_status == draw_text_no_more_indices) {
                    flush_and_draw_mesh(mosaic_ctx, &text_mesh);

                    const int remaining_ellipses_len = (int)strlen(curr_ellipses_position);
                    const int remaining_ellipses_vert = min_int(remaining_ellipses_len * verts_per_quad, cg_ctx->gl->config.internal_limits.max_verts_per_vertex_bank);
                    text_mesh = (text_mesh_t){
                        .reserved_verts = remaining_ellipses_vert,
                        .vert_index = 0,
                        .verts = cg_gl_state_map_vertex_range(cg_ctx->gl, remaining_ellipses_vert),
                        .total_glyphs = remaining_ellipses_len,
                        .glyphs_drawn = 0};

                } else {
                    ASSERT(draw_text_status == draw_text_complete);

                    curr_y += line_height;

                    if (box.width < line_width + ellipses_width) {
                        box.width = line_width + ellipses_width;
                        box.x = text_rect.x + line_offset;
                    }
                    box.height = curr_y - text_rect.y;
                }
            }
            // there are no valid glyphs to draw after we've drawn ellipses so bail.
            break;
        }

        // if we would render glyphs beneath the text block bail
        const float y_limit = options & cg_text_block_allow_block_bounds_overflow ? curr_y : curr_y + line_height;
        if (y_limit > max_y) {
            break;
        }
    }

    if (text_mesh.vert_index > 0) {
        flush_and_draw_mesh(mosaic_ctx, &text_mesh);
    }

    CG_FONT_TRACE_POP();
    return box;
}

void mosaic_context_draw_text_line(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const char * const text) {
    CG_FONT_TRACE_PUSH_FN();

    if (*text == '\0') {
        CG_FONT_TRACE_POP();
        return;
    }

    font_glyph_cache_t * glyph_cache = mosaic_glyph_raster_find_glyph_cache(mosaic_ctx->glyph_raster, mosaic_ctx->font_index);
    if (glyph_cache == NULL) {
        glyph_cache = mosaic_glyph_raster_create_glyph_cache(mosaic_ctx->glyph_raster, mosaic_ctx->font_index);
    }

    const cg_context_t * const cg_ctx = mosaic_ctx->cg_ctx;

    const int text_len = (int)strlen(text);
    const char * const text_end = text + text_len;
    const int initial_reserved_verts = min_int(text_len * verts_per_quad, cg_ctx->gl->config.internal_limits.max_verts_per_vertex_bank);
    text_mesh_t text_mesh = {
        .reserved_verts = initial_reserved_verts,
        .vert_index = 0,
        .verts = cg_gl_state_map_vertex_range(cg_ctx->gl, initial_reserved_verts),
        .total_glyphs = text_len,
        .glyphs_drawn = 0};

    const char * curr_text_position = text;
    const float displayable_glyph_width = min_float(text_rect.width, cg_ctx->width - text_rect.x);

    while (*curr_text_position) {
        float curr_x = text_rect.x;

        draw_text_status_e draw_text_status = draw_text_incomplete;
        while (draw_text_status != draw_text_complete) {
            draw_text_status = draw_partial_text(
                mosaic_ctx,
                glyph_cache,
                &text_mesh,
                &cg_ctx->cur_state->transform,
                (cg_vec2_t){.x = curr_x, .y = 0.0f},
                displayable_glyph_width,
                curr_text_position,
                text_end,
                &curr_text_position,
                &curr_x);

            if (draw_text_status == draw_text_codepoint_not_in_cache) {
                mosaic_glyph_raster_try_rasterize_glyphs(mosaic_ctx, glyph_cache, curr_text_position, text_end);

            } else if (draw_text_status == draw_text_glyph_not_in_atlas) {
                flush_and_draw_mesh(mosaic_ctx, &text_mesh);
                glyph_cache = mosaic_glyph_raster_rebuild_font_atlas(mosaic_ctx, mosaic_ctx->font_index, curr_text_position, text_end).glyph_cache;

            } else if (draw_text_status == draw_text_no_more_indices) {
                flush_and_draw_mesh(mosaic_ctx, &text_mesh);

            } else {
                ASSERT(draw_text_status == draw_text_complete);
            }
        }
    }

    if (text_mesh.vert_index > 0) {
        flush_and_draw_mesh(mosaic_ctx, &text_mesh);
    }

    CG_FONT_TRACE_POP();
}

cg_rect_t mosaic_context_get_text_block_extents(
    mosaic_context_t * const mosaic_ctx,
    const float max_line_width,
    const float extra_line_spacing,
    const char * const text,
    float * const out_widest_line,
    const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    *out_widest_line = 0;
    cg_rect_t extents = {0};
    if (*text == '\0') {
        CG_FONT_TRACE_POP();
        return extents;
    }

    const mosaic_font_data_t * const font = &mosaic_ctx->fonts[mosaic_ctx->font_index];
    const float font_height = (float)font->height;
    const float line_height = (options & cg_text_block_line_space_relative) ? (font_height * extra_line_spacing) : (font_height + extra_line_spacing);
    ASSERT(fabs(line_height) >= 1.0f);

    const char * curr_text_position = text;

    while (*curr_text_position) {
        float line_width = 0;
        float line_width_whitespace = 0;
        const char * const start_of_line = curr_text_position;
        find_linebreak_position(mosaic_ctx, max_line_width, curr_text_position, &curr_text_position, &line_width, &line_width_whitespace);
        if (line_width_whitespace > *out_widest_line) {
            *out_widest_line = line_width_whitespace;
        }

        skip_trailing_white_space_and_first_newline(start_of_line, &curr_text_position);
        extents.height += line_height;
        if (extents.width < line_width) {
            extents.width = line_width;
        }
    }

    CG_FONT_TRACE_POP();
    return extents;
}

cg_text_block_page_offsets_t mosaic_context_get_text_block_page_offsets(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    cg_text_block_page_offsets_t offsets = {0};
    if (*text == '\0') {
        CG_FONT_TRACE_POP();
        return offsets;
    }

    const mosaic_font_data_t * const font = &mosaic_ctx->fonts[mosaic_ctx->font_index];
    const float font_height = (float)font->height;
    const float line_height = (options & cg_text_block_line_space_relative) ? (font_height * extra_line_spacing) : (font_height + extra_line_spacing);
    ASSERT(fabs(line_height) >= 1.0f);

    const float max_y = text_rect.y + text_rect.height;

    const char * curr_text_position = text;
    const char * first_displayable_text = text;

    float curr_y = text_rect.y + scroll_offset;

    while (*curr_text_position) {
        float line_width_ignored = 0;
        float line_width_whitespace_ignored = 0;
        const char * const start_of_line = curr_text_position;
        find_linebreak_position(mosaic_ctx, text_rect.width, curr_text_position, &curr_text_position, &line_width_ignored, &line_width_whitespace_ignored);

        // if we would render glyphs above the box cull them, if its intersecting cull only if allow_bounds_overflow isn't enabled.
        if (options & cg_text_block_allow_block_bounds_overflow) {
            if (curr_y < text_rect.y - line_height) {
                curr_y += line_height;
                continue;
            }
        } else {
            if (curr_y < text_rect.y) {
                curr_y += line_height;
                continue;
            }
        }

        if (first_displayable_text != text) {
            first_displayable_text = curr_text_position;
        }

        skip_trailing_white_space_and_first_newline(start_of_line, &curr_text_position);
        curr_y += line_height;

        // if we would render glyphs beneath the text block bail
        const float y_limit = options & cg_text_block_allow_block_bounds_overflow ? curr_y : curr_y + line_height;
        if (y_limit > max_y) {
            break;
        }
    }

    offsets.begin_offset = (uint32_t)(ptrdiff_t)(first_displayable_text - text);
    offsets.end_offset = (uint32_t)(ptrdiff_t)(curr_text_position - text);
    CG_FONT_TRACE_POP();
    return offsets;
}

void cg_context_set_global_missing_glyph_indicator(const char * const missing_glyph_indicator) {
    CG_FONT_TRACE_PUSH_FN();
    ZEROMEM(&cg_statics.ctx->mosaic_ctx->missing_glyph_codepoint);
    if (!missing_glyph_indicator || (*missing_glyph_indicator == '\0')) {
        CG_FONT_TRACE_POP();
        return;
    }

    int32_t ignored;
    const int32_t codepoint_len = utf8_to_codepoint(missing_glyph_indicator, &ignored);
    ASSERT(missing_glyph_indicator[codepoint_len] == '\0');

    memcpy(cg_statics.ctx->mosaic_ctx->missing_glyph_codepoint, missing_glyph_indicator, codepoint_len);
    CG_FONT_TRACE_POP();
}

void cg_context_set_font_context_missing_glyph_indicator(cg_font_context_t * const font_ctx, const char * const missing_glyph_indicator) {
    CG_FONT_TRACE_PUSH_FN();
    mosaic_font_data_t * const selected_font = &font_ctx->mosaic_ctx->fonts[font_ctx->font_index];
    ZEROMEM(&selected_font->missing_glyph_codepoint);
    if (!missing_glyph_indicator || (*missing_glyph_indicator == '\0')) {
        CG_FONT_TRACE_POP();
        return;
    }

    int32_t ignored;
    const int32_t codepoint_len = utf8_to_codepoint(missing_glyph_indicator, &ignored);
    ASSERT(missing_glyph_indicator[codepoint_len] == '\0');

    memcpy(selected_font->missing_glyph_codepoint, missing_glyph_indicator, codepoint_len);
    CG_FONT_TRACE_POP();
}

static codepoint_rasterizing_e mosaic_context_precache_glyphs(cg_font_context_t * const font_ctx, const char * const characters) {
    mosaic_context_t * const mosaic_ctx = font_ctx->mosaic_ctx;
    font_glyph_cache_t * glyph_cache = mosaic_glyph_raster_find_glyph_cache(mosaic_ctx->glyph_raster, font_ctx->font_index);
    if (glyph_cache == NULL) {
        glyph_cache = mosaic_glyph_raster_create_glyph_cache(mosaic_ctx->glyph_raster, font_ctx->font_index);
    }
    return mosaic_glyph_raster_try_rasterize_glyphs(mosaic_ctx, glyph_cache, characters, characters + strlen(characters));
}

void cg_context_font_precache_glyphs(cg_font_context_t * const font_ctx, const char * const characters) {
    mosaic_context_precache_glyphs(font_ctx, characters);
}

void cg_context_font_clear_glyph_cache() {
    mosaic_context_t * const mosaic_ctx = cg_statics.ctx->mosaic_ctx;
    mosaic_glyph_raster_t * const glyph_raster = mosaic_ctx->glyph_raster;

    mosaic_glyph_raster_shutdown(glyph_raster);
    mosaic_glyph_raster_emplace_init(glyph_raster, mosaic_ctx);
}

static uint32_t string_get_num_codepoints(const char * const str) {
    CG_FONT_TRACE_PUSH_FN();

    const char * curr_utf8 = str;
    uint32_t visible_codepoint_count = 0;
    while (*curr_utf8) {
        int32_t codepoint;
        curr_utf8 += utf8_to_codepoint(curr_utf8, &codepoint);
        visible_codepoint_count += !(is_whitespace(codepoint) || is_newline(codepoint));
    }

    CG_FONT_TRACE_POP();
    return visible_codepoint_count;
}

static void text_mesh_init_emplace(text_mesh_t * const text_mesh, const uint32_t num_codepoints, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    *text_mesh = (text_mesh_t){0};

    text_mesh->resource_heap = &cg_statics.ctx->cg_heap_low;
    text_mesh->reserved_verts = num_codepoints * 6;

    text_mesh->null_buffer = CONST_MEM_REGION(.ptr = NULL, .size = sizeof(cg_gl_vertex_t) * text_mesh->reserved_verts);

    text_mesh->verts = cg_alloc(text_mesh->resource_heap, text_mesh->null_buffer.size, MALLOC_TAG);

    rhi_mesh_data_init_indirect_t mi;
    ZEROMEM(&mi);
    mi.num_channels = 1;
    mi.channels = &text_mesh->null_buffer;

    text_mesh->r_mesh = render_create_mesh(cg_statics.ctx->gl->render_device, mi, cg_statics.ctx->gl->mesh_layout, MALLOC_TAG);
    text_mesh->fence = render_get_cmd_stream_fence(&cg_statics.ctx->gl->render_device->default_cmd_stream);

    CG_FONT_TRACE_POP();
}

static void text_mesh_draw(text_mesh_t * const mesh) {
    CG_FONT_TRACE_PUSH_FN();
    if (mesh->vert_index > 0) {
        cg_context_t * const cg_ctx = cg_statics.ctx;
        cg_select_blend_and_shader(cg_ctx->gl, cg_ctx->cur_state, &cg_ctx->cur_state->fill_style, &cg_ctx->mosaic_ctx->atlas.image.cg_texture, cg_rgb_fill_alpha_red_enabled);
        cg_gl_state_draw_mesh(cg_ctx->gl, mesh->r_mesh, rhi_triangles, mesh->vert_index, 0);
    }
    CG_FONT_TRACE_POP();
}

static void text_mesh_free_verts(text_mesh_t * const mesh, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    render_conditional_flush_cmd_stream_and_wait_fence(cg_statics.ctx->gl->render_device, &cg_statics.ctx->gl->render_device->default_cmd_stream, mesh->fence);
    cg_free(mesh->resource_heap, mesh->verts, tag);
    mesh->verts = NULL;
    CG_FONT_TRACE_POP();
}

static void text_mesh_free(text_mesh_t * const mesh, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();

    render_conditional_flush_cmd_stream_and_wait_fence(cg_statics.ctx->gl->render_device, &cg_statics.ctx->gl->render_device->default_cmd_stream, mesh->fence);
    render_release(&mesh->r_mesh->resource, tag);
    if (mesh->verts) {
        text_mesh_free_verts(mesh, tag);
    }
    ZEROMEM(mesh);

    CG_FONT_TRACE_POP();
}

static void text_mesh_upload_mesh_indirect(text_mesh_t * const mesh) {
    CG_FONT_TRACE_PUSH_FN();

    if (mesh->vert_index > 0) {
        cg_gl_state_t * const state = cg_statics.ctx->gl;

        const int channel_index = 0;
        const int first_elem = 0;
        const int num_elems = mesh->vert_index;
        const size_t stride = sizeof(cg_gl_vertex_t);
        const void * const data = mesh->verts;

        const uint32_t hash = render_cmd_stream_upload_mesh_channel_data(
            &state->render_device->default_cmd_stream,
            &mesh->r_mesh->mesh,
            channel_index,
            first_elem,
            num_elems,
            stride,
            data,
            MALLOC_TAG);

        mesh->r_mesh->hash = hash;
        mesh->fence = render_get_cmd_stream_fence(&cg_statics.ctx->gl->render_device->default_cmd_stream);
    }

    CG_FONT_TRACE_POP();
}

static void mosaic_context_create_text_block_mesh(
    mosaic_context_t * const mosaic_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const char * const optional_ellipses,
    const cg_text_block_options_e options,
    text_mesh_t * const out_text_mesh) {
    CG_FONT_TRACE_PUSH_FN();
    cg_rect_t box = {.x = text_rect.x, .y = text_rect.y, .width = 0.0f, .height = 0.0f};
    if (*text == '\0') {
        CG_FONT_TRACE_POP();
    }

    const mosaic_font_data_t * const font = &mosaic_ctx->fonts[mosaic_ctx->font_index];

    font_glyph_cache_t * glyph_cache = mosaic_glyph_raster_find_glyph_cache(mosaic_ctx->glyph_raster, mosaic_ctx->font_index);
    if (glyph_cache == NULL) {
        glyph_cache = mosaic_glyph_raster_create_glyph_cache(mosaic_ctx->glyph_raster, mosaic_ctx->font_index);
    }

    const float font_height = (float)font->height;
    const float line_height = (options & cg_text_block_line_space_relative) ? (font_height * extra_line_spacing) : (font_height + extra_line_spacing);
    ASSERT(fabs(line_height) >= 1.0f);

    const cg_context_t * const cg_ctx = mosaic_ctx->cg_ctx;

    text_mesh_init_emplace(out_text_mesh, string_get_num_codepoints(text) + (optional_ellipses ? string_get_num_codepoints(optional_ellipses) : 0), MALLOC_TAG);

    float ellipses_width = 0;
    if (optional_ellipses) {
        const char * linebreak_ignored;
        float line_width_ignored = 0;
        find_linebreak_position(mosaic_ctx, INFINITY, optional_ellipses, &linebreak_ignored, &ellipses_width, &line_width_ignored);
    }

    int32_t space_width_int, tab_width_int, lsb_ignored;
    stbtt_GetCodepointHMetrics(&font->cg_font->font_info, ' ', &space_width_int, &lsb_ignored);
    stbtt_GetCodepointHMetrics(&font->cg_font->font_info, '\t', &tab_width_int, &lsb_ignored);

    const float space_width = font->scale * space_width_int;
    const float tab_width = font->scale * tab_width_int;

    float curr_y = text_rect.y + scroll_offset;
    const float max_y = text_rect.y + text_rect.height;

    const char * curr_text_position = text;
    const float displayable_glyph_width = min_float(text_rect.width, cg_ctx->width - text_rect.x);

    while (*curr_text_position) {
        const char * linebreak_position = NULL;
        float last_renderable_width = 0;
        float line_width = 0;
        find_linebreak_position(mosaic_ctx, text_rect.width, curr_text_position, &linebreak_position, &last_renderable_width, &line_width);

        // if we would render glyphs above the box cull them, if its intersecting cull only if allow_bounds_overflow isn't enabled.
        if (options & cg_text_block_allow_block_bounds_overflow) {
            if (curr_y < text_rect.y - line_height) {
                curr_y += line_height;
                continue;
            }
        } else {
            if (curr_y < text_rect.y) {
                curr_y += line_height;
                continue;
            }
        }

        const float ellipses_height_threshold = curr_y + line_height + ((options & cg_text_block_allow_block_bounds_overflow) ? 0 : line_height);
        const bool use_ellipses = (*linebreak_position != '\0')
                                  && optional_ellipses
                                  && ((ellipses_height_threshold > max_y)
                                      || (line_height_at_next_word(
                                              mosaic_ctx,
                                              (line_height_args_t){
                                                  .curr_height = curr_y + line_height,
                                                  .line_height = line_height,
                                                  .max_height = max_y,
                                                  .max_line_width = text_rect.width,
                                                  .space_width = space_width,
                                                  .tab_width = tab_width,
                                                  .ellipses_width = ellipses_width,
                                                  .text = linebreak_position})
                                          >= max_y));

        // if ellipses are enabled, and we need to draw them re-calculate the line's cut off position if we can't fit the ellipses on the existing line.
        if (use_ellipses && (last_renderable_width + ellipses_width > text_rect.width)) {
            find_linebreak_position(mosaic_ctx, text_rect.width - ellipses_width, curr_text_position, &linebreak_position, &last_renderable_width, &line_width);
        }

        const float line_offset = calculate_line_offset(text_rect.width, last_renderable_width + (use_ellipses ? ellipses_width : 0), options);
        float curr_x = text_rect.x + line_offset;

        const char * const start_of_line = curr_text_position;

        draw_text_status_e draw_text_status = draw_partial_text(
            mosaic_ctx,
            glyph_cache,
            out_text_mesh,
            &cg_ctx->cur_state->transform,
            (cg_vec2_t){.x = curr_x, .y = curr_y},
            displayable_glyph_width,
            curr_text_position,
            linebreak_position,
            &curr_text_position,
            &curr_x);

        VERIFY_MSG(draw_text_status == draw_text_complete, "incomplete draw-text-status: %i", draw_text_status);

        if (!use_ellipses) {
            curr_x = text_rect.x;
            curr_y += line_height;

            if (box.width < line_width) {
                box.width = line_width;
                box.x = text_rect.x + line_offset;
            }
            box.height = curr_y - text_rect.y;
            skip_trailing_white_space_and_first_newline(start_of_line, &curr_text_position);
        }

        if (use_ellipses) {
            const char * curr_ellipses_position = optional_ellipses;
            const char * const end_of_ellipses = optional_ellipses + strlen(optional_ellipses);
            draw_text_status = draw_partial_text(
                mosaic_ctx,
                glyph_cache,
                out_text_mesh,
                &cg_ctx->cur_state->transform,
                (cg_vec2_t){.x = curr_x, .y = curr_y},
                displayable_glyph_width,
                curr_ellipses_position,
                end_of_ellipses,
                &curr_ellipses_position,
                &curr_x);

            VERIFY_MSG(draw_text_status == draw_text_complete, "incomplete draw-text-status: %i", draw_text_status);

            // there are no valid glyphs to draw after we've drawn ellipses so bail.
            break;
        }

        // if we would render glyphs beneath the text block bail
        const float y_limit = options & cg_text_block_allow_block_bounds_overflow ? curr_y : curr_y + line_height;
        if (y_limit > max_y) {
            break;
        }
    }

    out_text_mesh->bounding_box = box;
    CG_FONT_TRACE_POP();
}

static bool text_mesh_id_block_compare(const text_mesh_id_block_t * const left, const text_mesh_id_block_t * const right) {
    return (left->crc == right->crc) && (left->font_id == right->font_id) && (left->str_len == right->str_len) && (left->has_ellipses == right->has_ellipses) && (memcmp(left->first_n_chars, right->first_n_chars, sizeof(left->first_n_chars)) == 0) && (left->scroll_offset == right->scroll_offset) && (left->options == right->options) && (memcmp(&left->rect, &right->rect, sizeof(left->rect)) == 0);
}

static cg_rect_t affine_apply_rect(const cg_rect_t rect) {
    const cg_state_t * const cg_state = cg_statics.ctx->cur_state;
    const cg_vec2_t tl_pos = {.x = rect.x, .y = rect.y};
    const cg_vec2_t br_pos = {.x = rect.x + rect.width, .y = rect.y + rect.height};
    const cg_vec2_t * const tl = cg_affine_apply(&cg_state->transform, &tl_pos);
    const cg_vec2_t * const br = cg_affine_apply(&cg_state->transform, &br_pos);

    // this isn't actually a rect.
    return (cg_rect_t){tl->x, tl->y, br->x, br->y};
}

static text_mesh_id_block_t mosaic_context_text_block_create_mesh_id_block(mosaic_context_t * const mosaic_ctx, const cg_rect_t text_rect, const float scroll_offset, const float extra_line_spacing, const char * const text, const char * const optional_ellipses, const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    text_mesh_id_block_t id_block = {0};
    const mosaic_font_data_t * const font = &mosaic_ctx->fonts[mosaic_ctx->font_index];
    id_block.str_len = (uint32_t)strlen(text);
    id_block.scroll_offset = scroll_offset;
    id_block.options = options;
    id_block.has_ellipses = optional_ellipses;
    id_block.font_id = font->cg_font->id;
    id_block.rect = affine_apply_rect(text_rect);
    memcpy(id_block.first_n_chars, text, min_size_t(id_block.str_len, sizeof(id_block.first_n_chars)));

    // TODO: need to apply the cg affine to the text rect, else we could have a case where client tries to draw the same string in 4 locations, with the entire same set of data... but they just translate via cg_translate.
    id_block.crc = crc_32((const unsigned char *)&text_rect, sizeof(text_rect));
    id_block.crc = update_crc_32(id_block.crc, (const unsigned char *)&id_block.font_id, sizeof(id_block.font_id));
    id_block.crc = update_crc_32(id_block.crc, (const unsigned char *)&scroll_offset, sizeof(scroll_offset));
    id_block.crc = update_crc_32(id_block.crc, (const unsigned char *)&extra_line_spacing, sizeof(extra_line_spacing));

    id_block.crc = crc_32((const unsigned char *)text, id_block.str_len);
    if (optional_ellipses) {
        id_block.crc = update_crc_32(id_block.crc, (const unsigned char *)optional_ellipses, strlen(optional_ellipses));
    }

    id_block.crc = update_crc_32(id_block.crc, (const unsigned char *)&options, sizeof(options));

    CG_FONT_TRACE_POP();
    return id_block;
}

static void text_mesh_cache_init_emplace(text_mesh_cache_t * const cache, uint32_t size, const char * const tag) {
    cg_context_t * const ctx = cg_statics.ctx;
    *cache = (text_mesh_cache_t){0};

    cache->storage = cg_alloc(&ctx->cg_heap_low, size * sizeof(text_mesh_cache_node_t), tag);
    cache->storage_len = size;

    for (uint32_t i = 0; i < size; ++i) {
        LL_ADD(&cache->storage[i], prev, next, cache->free.head, cache->free.tail);
    }
}

static void text_mesh_cache_evict_active_nodes(text_mesh_cache_t * const cache, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    text_mesh_cache_node_t * curr_node = cache->active.head;
    while (curr_node) {
        text_mesh_cache_node_t * const next_node = curr_node->next;
        text_mesh_free(&curr_node->text_mesh, tag);
        LL_REMOVE(curr_node, prev, next, cache->active.head, cache->active.tail);
        *curr_node = (text_mesh_cache_node_t){0};
        LL_ADD(curr_node, prev, next, cache->free.head, cache->free.tail);
        curr_node = next_node;
    }
    CG_FONT_TRACE_POP();
}

static void text_mesh_cache_free(text_mesh_cache_t * const cache, const char * const tag) {
    CG_FONT_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;

    text_mesh_cache_evict_active_nodes(cache, tag);
    cg_free(&ctx->cg_heap_low, cache->storage, tag);
    CG_FONT_TRACE_POP();
}

static text_mesh_cache_node_t * text_mesh_cache_try_pop_free(text_mesh_cache_t * const cache, const text_mesh_id_block_t * const id_block) {
    text_mesh_cache_node_t * const node = cache->free.head;
    if (node) {
        LL_REMOVE(node, prev, next, cache->free.head, cache->free.tail);
        LL_ADD(node, prev, next, cache->active.head, cache->active.tail);
        node->id_block = *id_block;
    }
    return node;
}

static text_mesh_cache_node_t * text_mesh_cache_reuse_oldest_node(text_mesh_cache_t * const cache, const text_mesh_id_block_t * const id_block, const char * const tag) {
    ASSERT(cache->active.tail);
    text_mesh_cache_node_t * const node = cache->active.tail;
    LL_REMOVE(node, prev, next, cache->active.head, cache->active.tail);
    LL_PUSH_FRONT(node, prev, next, cache->active.head, cache->active.tail);
    text_mesh_free(&node->text_mesh, tag);
    node->id_block = *id_block;
    return node;
}

static text_mesh_cache_node_t * text_mesh_cache_find_node(text_mesh_cache_t * const cache, const text_mesh_id_block_t * const id_block) {
    CG_FONT_TRACE_PUSH_FN();
    text_mesh_cache_node_t * curr_node = cache->active.head;
    while (curr_node) {
        if (text_mesh_id_block_compare(&curr_node->id_block, id_block)) {
            // shuffle the nodes around so the most recently used node is up front, so we can have the least used towards the end of the list.
            LL_REMOVE(curr_node, prev, next, cache->active.head, cache->active.tail);
            LL_PUSH_FRONT(curr_node, prev, next, cache->active.head, cache->active.tail);
            CG_FONT_TRACE_POP();
            return curr_node;
        }
        curr_node = curr_node->next;
    }
    CG_FONT_TRACE_POP();
    return NULL;
}

static cg_rect_t mosaic_context_draw_text_block_memoized(
    cg_font_context_t * const font_ctx,
    const cg_rect_t text_rect,
    const float scroll_offset,
    const float extra_line_spacing,
    const char * const text,
    const char * const optional_ellipses,
    const cg_text_block_options_e options) {
    CG_FONT_TRACE_PUSH_FN();
    text_mesh_cache_t * const text_mesh_cache = &font_ctx->mosaic_ctx->text_mesh_cache;

    const text_mesh_id_block_t id_block = mosaic_context_text_block_create_mesh_id_block(font_ctx->mosaic_ctx, text_rect, scroll_offset, extra_line_spacing, text, optional_ellipses, options);

    // we have the node, trivial case of just reusing.
    text_mesh_cache_node_t * node = text_mesh_cache_find_node(text_mesh_cache, &id_block);
    if (node) {
        text_mesh_draw(&node->text_mesh);
        if (node->text_mesh.verts && render_check_fence(node->text_mesh.fence)) {
            text_mesh_free_verts(&node->text_mesh, MALLOC_TAG);
        }
        CG_FONT_TRACE_POP();
        return node->text_mesh.bounding_box;
    } else if ((mosaic_context_precache_glyphs(font_ctx, text) == codepoint_rasterizing_renderable_rasterized) && (!optional_ellipses || (mosaic_context_precache_glyphs(font_ctx, optional_ellipses) == codepoint_rasterizing_renderable_rasterized))) {
        // we do have glyphs in the atlas, just need to find a valid node we can use.
        node = text_mesh_cache_try_pop_free(text_mesh_cache, &id_block);
        if (!node) {
            node = text_mesh_cache_reuse_oldest_node(text_mesh_cache, &id_block, MALLOC_TAG);
        }
    } else {
        // glyphs could not fit, must flush the atlas, and all existing meshes and rebuild.
        text_mesh_cache_evict_active_nodes(text_mesh_cache, MALLOC_TAG);
        VERIFY(mosaic_glyph_raster_rebuild_font_atlas(font_ctx->mosaic_ctx, font_ctx->font_index, text, text + id_block.str_len).rasterize_state == codepoint_rasterizing_renderable_rasterized);
        node = text_mesh_cache_try_pop_free(text_mesh_cache, &id_block);
        VERIFY(node);
    }
    ASSERT(node);

    // create the text mesh, flush the atlas, and draw.
    mosaic_context_create_text_block_mesh(font_ctx->mosaic_ctx, text_rect, scroll_offset, extra_line_spacing, text, optional_ellipses, options, &node->text_mesh);
    text_mesh_upload_mesh_indirect(&node->text_mesh);
    mosaic_glyph_raster_flush_atlas(font_ctx->mosaic_ctx->glyph_raster);
    text_mesh_draw(&node->text_mesh);
    node->text_mesh.fence = render_get_cmd_stream_fence(&cg_statics.ctx->gl->render_device->default_cmd_stream);

    CG_FONT_TRACE_POP();
    return node->text_mesh.bounding_box;
}