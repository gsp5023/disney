/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
cg.c

2d canvas rendering
*/

#include "cg.h"

#include "private/cg_font.h"
#include "source/adk/log/log.h"
#include "source/adk/nve/video_texture_api.h"
#include "source/adk/runtime/thread_pool.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/telemetry/telemetry.h"

#include <math.h>

cg_statics_t cg_statics;

void * cg_alloc(cg_heap_t * const cg_heap, const size_t alloc_size, const char * const tag) {
    CG_TRACE_PUSH_FN();

    sb_lock_mutex(cg_heap->mutex);
    void * const ptr = heap_alloc(&cg_heap->heap, alloc_size, tag);
    sb_unlock_mutex(cg_heap->mutex);
    CG_TRACE_HEAP(&cg_heap->heap);

    CG_TRACE_POP();
    return ptr;
}

void * cg_calloc(cg_heap_t * const cg_heap, const size_t alloc_size, const char * const tag) {
    CG_TRACE_PUSH_FN();

    sb_lock_mutex(cg_heap->mutex);
    void * const ptr = heap_calloc(&cg_heap->heap, alloc_size, tag);
    sb_unlock_mutex(cg_heap->mutex);
    CG_TRACE_HEAP(&cg_heap->heap);

    CG_TRACE_POP();
    return ptr;
}

void * cg_realloc(cg_heap_t * const cg_heap, void * const ptr, const size_t new_size, const char * const tag) {
    CG_TRACE_PUSH_FN();

    sb_lock_mutex(cg_heap->mutex);
    void * const new_ptr = heap_realloc(&cg_heap->heap, ptr, new_size, tag);
    sb_unlock_mutex(cg_heap->mutex);
    CG_TRACE_HEAP(&cg_heap->heap);

    CG_TRACE_POP();
    return new_ptr;
}

void cg_free(cg_heap_t * const cg_heap, void * ptr, const char * const tag) {
    CG_TRACE_PUSH_FN();

    sb_lock_mutex(cg_heap->mutex);
    heap_free(&cg_heap->heap, ptr, tag);
    sb_unlock_mutex(cg_heap->mutex);
    CG_TRACE_HEAP(&cg_heap->heap);

    CG_TRACE_POP();
}

cg_allocation_t cg_unchecked_alloc(struct cg_heap_t * const cg_heap, const size_t alloc_size, const char * const tag) {
    CG_TRACE_PUSH_FN();
    if (!cg_heap->heap.internal.init) {
        return (cg_allocation_t){0};
    }
    sb_lock_mutex(cg_heap->mutex);
    const cg_allocation_t alloc = {{{.cg_heap = cg_heap, .region = MEM_REGION(.ptr = heap_unchecked_alloc(&cg_heap->heap, alloc_size, tag), .size = alloc_size)}}};
    sb_unlock_mutex(cg_heap->mutex);
    CG_TRACE_HEAP(&cg_heap->heap);

    CG_TRACE_POP();
    return alloc;
}

cg_allocation_t cg_unchecked_realloc(struct cg_heap_t * const cg_heap, const cg_allocation_t old_alloc, const size_t new_size, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_allocation_t alloc = {0};
    if (!cg_heap->heap.internal.init) {
        return alloc;
    }
    if ((cg_heap != old_alloc.cg_heap) && old_alloc.cg_heap) {
        alloc = cg_unchecked_alloc(cg_heap, new_size, tag);
        if (alloc.region.ptr) {
            memcpy(alloc.region.ptr, old_alloc.region.ptr, min_size_t(old_alloc.region.size, alloc.region.size));
            cg_free_alloc(old_alloc, tag);
            CG_TRACE_HEAP(&old_alloc.cg_heap->heap);
        }
    } else {
        sb_lock_mutex(cg_heap->mutex);
        alloc = (cg_allocation_t){{{.cg_heap = cg_heap, MEM_REGION(.ptr = heap_unchecked_realloc(&cg_heap->heap, old_alloc.region.ptr, new_size, tag), .size = new_size)}}};
        sb_unlock_mutex(cg_heap->mutex);
    }
    CG_TRACE_HEAP(&cg_heap->heap);

    CG_TRACE_POP();
    return alloc;
}

void cg_free_alloc(const cg_allocation_t alloc, const char * const tag) {
    cg_free(alloc.cg_heap, alloc.region.ptr, tag);
}

void cg_free_const_alloc(const cg_const_allocation_t alloc, const char * const tag) {
    cg_free(alloc.cg_heap, (void *)alloc.region.ptr, tag);
}

/* ===========================================================================
 * SUBPATH
 * ==========================================================================*/

typedef enum cg_subpath_wrap_e {
    cg_subpath_wrap_clamp,
    cg_subpath_wrap_normal,
    cg_subpath_wrap_tangent // ~ 1st order derivative
} cg_subpath_wrap_e;

typedef enum cg_subpath_winding_e {
    cg_subpath_winding_cw,
    cg_subpath_winding_ccw,
    cg_subpath_winding_none
} cg_subpath_winding_e;

static const cg_subpath_t * cg_subpath(const char * const tag) {
    CG_TRACE_PUSH_FN();

    static cg_subpath_t subpath;
    CG_ARRAY(cg_vec2_t, cg_statics.ctx, subpath.array, tag);
    subpath.closed = false;
    subpath.reverse = false;

    CG_TRACE_POP();
    return &subpath;
}

static void cg_subpath_free(cg_subpath_t * const subpath, const char * const tag) {
    CG_TRACE_PUSH_FN();
    CG_ARRAY_FREE(cg_statics.ctx, subpath->array, tag);
    CG_TRACE_POP();
}

static void cg_subpath_push(cg_subpath_t * const subpath, const cg_vec2_t * const pos, const char * const tag) {
    CG_TRACE_PUSH_FN();
    CG_ARRAY_PUSH(cg_vec2_t, cg_statics.ctx, subpath->array, *pos, tag);
    CG_TRACE_POP();
}

static int cg_subpath_size(const cg_subpath_t * const subpath) {
    CG_TRACE_PUSH_FN();
    const int size = CG_ARRAY_SIZE(subpath->array);
    CG_TRACE_POP();
    return size;
}

static const cg_vec2_t * cg_subpath_at(const cg_subpath_t * const subpath, int32_t idx, const cg_subpath_wrap_e mode) {
    CG_TRACE_PUSH_FN();
    const int32_t size = (const int32_t)cg_subpath_size(subpath);
    if (size == 0) {
        CG_TRACE_POP();
        return NULL;
    }

    if (idx < 0 || idx >= size) {
        if (mode == cg_subpath_wrap_clamp) {
            CG_TRACE_POP();
            return &subpath->array[clamp_int(idx, 0, size - 1)];
        } else if (mode == cg_subpath_wrap_normal) {
            if (idx < 0) {
                idx += size;
            } else if (idx >= size) {
                idx -= size;
            }
            CG_TRACE_POP();
            return &subpath->array[idx];
        } else if (mode == cg_subpath_wrap_tangent) {
            if (idx < 0) {
                const cg_vec2_t * p0 = &subpath->array[clamp_int(0, 0, size - 1)];
                const cg_vec2_t * p1 = &subpath->array[clamp_int(1, 0, size - 1)];
                CG_TRACE_POP();
                return cg_vec2_sub(cg_vec2_scale(p0, 2.0f), p1);
            } else if (idx >= size) {
                const cg_vec2_t * p0 = &subpath->array[clamp_int(size - 1, 0, size - 1)];
                const cg_vec2_t * p1 = &subpath->array[clamp_int(size - 2, 0, size - 1)];
                CG_TRACE_POP();
                return cg_vec2_sub(cg_vec2_scale(p0, 2.0f), p1);
            } else {
                CG_TRACE_POP();
                return cg_subpath_at(subpath, idx, cg_subpath_wrap_clamp);
            }
        }
    }
    CG_TRACE_POP();
    return &subpath->array[idx];
}

static cg_subpath_winding_e cg_subpath_winding(const cg_subpath_t * const subpath) {
    CG_TRACE_PUSH_FN();
    const size_t count = cg_subpath_size(subpath);
    if (count < 3) {
        CG_TRACE_POP();
        return cg_subpath_winding_none;
    }

    float area = 0.0f;
    for (size_t i = 2; i < count; ++i) {
        cg_vec2_t * a = &subpath->array[0];
        cg_vec2_t * b = &subpath->array[i - 1];
        cg_vec2_t * c = &subpath->array[i];
        area += cg_vec2_det(cg_vec2_sub(b, a), cg_vec2_sub(c, a));
    }

    if (area > 0.0f) {
        CG_TRACE_POP();
        return cg_subpath_winding_cw;
    }

    CG_TRACE_POP();
    return cg_subpath_winding_ccw;
}

static void cg_subpath_reverse(const cg_subpath_t * const subpath) {
    CG_TRACE_PUSH_FN();
    cg_vec2_t tmp;
    size_t i = 0, j = cg_subpath_size(subpath) - 1;
    while (i < j) {
        tmp = subpath->array[i];
        subpath->array[i] = subpath->array[j];
        subpath->array[j] = tmp;
        i++;
        j--;
    }
    CG_TRACE_POP();
}

/* ===========================================================================
 * STATE
 * ==========================================================================*/

static void cg_state_init(cg_state_t * const state, const cg_context_t * const ctx) {
    CG_TRACE_PUSH_FN();

    cg_affine_identity(&state->transform);
    state->stroke_style = *cg_color(1.0f, 1.0f, 1.0f, 1.0f);
    state->fill_style = *cg_color(1.0f, 1.0f, 1.0f, 1.0f);
    state->global_alpha = 1.0f;
    state->line_width = 1.0f;
    state->feather = 1.0f;
    state->image = NULL;
    state->alpha_test_threshold = 0.5f;
    state->blend_mode = cg_blend_mode_src_alpha_rgb;
    state->clip_state.x0 = 0;
    state->clip_state.y0 = 0;
    state->clip_state.x1 = (float)ctx->width;
    state->clip_state.y1 = (float)ctx->height;
    state->clip_state.clip_state = cg_clip_state_disabled;

    CG_TRACE_POP();
}

static void cg_bind_color_shader(cg_gl_state_t * const gl, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const cg_rgb_fill_alpha_red_mode_e rgb_fill_alpha_mode) {
    if (rgb_fill_alpha_mode == cg_rgb_fill_alpha_red_enabled) {
        cg_gl_state_bind_color_rgb_fill_alpha_red_shader(gl, fill, tex);
    } else {
        cg_gl_state_bind_color_shader(gl, fill, tex);
    }
}

static void cg_bind_alpha_test_shader(cg_gl_state_t * const gl, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const float threshold, const cg_rgb_fill_alpha_red_mode_e rgb_fill_alpha_mode) {
    if (rgb_fill_alpha_mode == cg_rgb_fill_alpha_red_enabled) {
        cg_gl_state_bind_color_shader_alpha_rgb_fill_alpha_red_test(gl, fill, tex, threshold);
    } else {
        cg_gl_state_bind_color_shader_alpha_test(gl, fill, tex, threshold);
    }
}

void cg_select_blend_and_shader(cg_gl_state_t * const gl, const cg_state_t * const cg_state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const cg_rgb_fill_alpha_red_mode_e rgb_fill_alpha_mode) {
    switch (cg_state->blend_mode) {
        case cg_blend_mode_blit:
            cg_gl_state_set_mode_blit(gl);
            cg_bind_color_shader(gl, fill, tex, rgb_fill_alpha_mode);
            break;
        case cg_blend_mode_alpha_test:
            cg_gl_state_set_mode_blit(gl);
            cg_bind_alpha_test_shader(gl, fill, tex, cg_state->alpha_test_threshold, rgb_fill_alpha_mode);
            break;
        case cg_blend_mode_src_alpha_all:
            cg_gl_state_set_mode_blend_alpha_all(gl);
            cg_bind_color_shader(gl, fill, tex, rgb_fill_alpha_mode);
            break;
        case cg_blend_mode_src_alpha_rgb:
        default:
            cg_gl_state_set_mode_blend_alpha_rgb(gl);
            cg_bind_color_shader(gl, fill, tex, rgb_fill_alpha_mode);
            break;
    }
}

void cg_select_blend_and_shader_alpha_mask(cg_gl_state_t * const gl, const cg_state_t * const cg_state, const cg_color_t * const fill, const cg_gl_texture_t * const tex, const cg_gl_texture_t * const mask) {
    switch (cg_state->blend_mode) {
        case cg_blend_mode_alpha_test:
            // Not supported unless we want to add an additional shader for handling this exact case.
            // Currently falls back to the blit mode
        case cg_blend_mode_blit:
            cg_gl_state_set_mode_blit(gl);
            cg_gl_state_bind_color_shader_alpha_mask(gl, fill, tex, mask);
            break;
        case cg_blend_mode_src_alpha_all:
            cg_gl_state_set_mode_blend_alpha_all(gl);
            cg_gl_state_bind_color_shader_alpha_mask(gl, fill, tex, mask);
            break;
        case cg_blend_mode_src_alpha_rgb:
        default:
            cg_gl_state_set_mode_blend_alpha_rgb(gl);
            cg_gl_state_bind_color_shader_alpha_mask(gl, fill, tex, mask);
            break;
    }
}

/* ===========================================================================
 * PATH
 * ==========================================================================*/

static const cg_path_t * cg_path(cg_state_t * const state, const char * const tag) {
    CG_TRACE_PUSH_FN();

    static cg_path_t path;
    path.state = state;
    cg_context_t * const ctx = cg_statics.ctx;
    CG_ARRAY(cg_subpath_t, ctx, path.paths, tag);
    path.last_point.x = path.last_point.y = CG_BIG_NUM;
    path.cur_path = *cg_subpath(tag);
    path.min_point = *cg_vec2(CG_BIG_NUM, CG_BIG_NUM);
    path.max_point = *cg_vec2(-CG_BIG_NUM, -CG_BIG_NUM);

    CG_TRACE_POP();

    return &path;
}

static size_t cg_path_size(const cg_path_t * const path) {
    return CG_ARRAY_SIZE(path->paths);
}

static void cg_path_free(cg_path_t * const path, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;
    const size_t count = cg_path_size(path);
    for (size_t i = 0; i < count; ++i) {
        cg_subpath_free(&path->paths[i], tag);
    }
    CG_ARRAY_FREE(ctx, path->paths, tag);
    cg_subpath_free(&path->cur_path, tag);

    CG_TRACE_POP();
}

static void cg_path_reset(cg_path_t * const path, const char * const tag) {
    CG_TRACE_PUSH_FN();

    // already at init state...
    if (cg_path_size(path) == 0 && cg_subpath_size(&path->cur_path) == 0) {
        CG_TRACE_POP();
        return;
    }

    cg_path_free(path, tag);
    *path = *cg_path(path->state, tag);

    CG_TRACE_POP();
}

static void cg_path_end(cg_path_t * const path, const char * const tag) {
    CG_TRACE_PUSH_FN();

    // only push subpath if there is at least a line
    if (cg_subpath_size(&path->cur_path) > 1) {
        CG_ARRAY_PUSH(cg_subpath_t, cg_statics.ctx, path->paths, path->cur_path, tag);
    } else {
        cg_subpath_free(&path->cur_path, tag);
    }
    path->cur_path = *cg_subpath(tag);

    CG_TRACE_POP();
}

static void cg_path_close(cg_path_t * const path, const char * const tag) {
    CG_TRACE_PUSH_FN();

    path->cur_path.closed = true;
    cg_path_end(path, tag);

    CG_TRACE_POP();
}

static void cg_path_push(cg_path_t * const path, const cg_vec2_t * const pos, const char * const tag) {
    CG_TRACE_PUSH_FN();

    if (cg_vec2_equal(&path->last_point, pos)) {
        path->last_point = *pos;
        CG_TRACE_POP();
        return;
    }
    path->last_point = *pos;
    cg_subpath_push(&path->cur_path, &path->last_point, tag);

    CG_TRACE_POP();
}

static void cg_path_xformed_push(cg_path_t * const path, const cg_vec2_t * const pos, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_push(path, cg_affine_apply(&path->state->transform, pos), tag);
    CG_TRACE_POP();
}

static void cg_path_move_to(cg_path_t * const path, const cg_vec2_t pos, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_end(path, tag);
    cg_path_xformed_push(path, &pos, tag);
    CG_TRACE_POP();
}

static void cg_path_line_to(cg_path_t * const path, const cg_vec2_t pos, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_xformed_push(path, &pos, tag);
    CG_TRACE_POP();
}

static void cg_path_arc(cg_path_t * const path, const cg_vec2_t pos, const float radius, const cg_rads_t start, const cg_rads_t end, const cg_rotation_e rotation, const char * const tag) {
    CG_TRACE_PUSH_FN();

    // theta wrapping
    float span = end.rads - start.rads;
    if (rotation == cg_rotation_clock_wise) {
        if (CG_ABS(span) >= CG_TAU) {
            span = CG_TAU;
        } else {
            while (span < 0.0f) {
                span += CG_TAU;
            }
        }
    } else {
        ASSERT(rotation == cg_rotation_counter_clock_wise);
        if (CG_ABS(span) >= CG_TAU) {
            span = -CG_TAU;
        } else {
            while (span > 0.0f) {
                span -= CG_TAU;
            }
        }
    }

    // tesselation (dynamic lod via geometric mean)
    const float scale = cg_affine_get_scale(&path->state->transform);
    const float size = CG_ABS(span) * radius * CG_TAU;
    const float cg_arc_min_steps = 1.0f;
    const float cg_arc_max_steps = (float)cg_statics.ctx->config.internal_limits.max_tessellation_steps;
    const size_t steps = (size_t)clamp_float(cg_sqrt(size * scale), cg_arc_min_steps, cg_arc_max_steps);

    float theta = start.rads;
    const float dtheta = span / (float)steps;
    for (size_t i = 0; i <= steps; ++i) {
        cg_path_xformed_push(path, cg_vec2(pos.x + cg_cos(theta) * radius, pos.y + cg_sin(theta) * radius), tag);
        theta += dtheta;
    }

    CG_TRACE_POP();
}

static void cg_path_arc_to(cg_path_t * const path, const cg_vec2_t pos1, const cg_vec2_t pos2, const float radius, const char * const tag) {
    CG_TRACE_PUSH_FN();

    // need to see if this is optimal?
    cg_vec2_t cp = *cg_affine_inv_apply(&path->state->transform, &path->last_point);
    const float a1 = cp.y - pos1.y;
    const float b1 = cp.x - pos1.x;
    const float a2 = pos2.y - pos1.y;
    const float b2 = pos2.x - pos1.x;
    const float mm = CG_ABS(a1 * b2 - b1 * a2);
    if (mm < CG_SMALL_NUM || radius < 1.0f) {
        cg_path_line_to(path, pos1, tag);
    } else {
        // https://mindcat.hatenadiary.org/entry/20100131/1264958828
        const float dd = a1 * a1 + b1 * b1;
        const float cc = a2 * a2 + b2 * b2;
        const float tt = a1 * a2 + b1 * b2;
        const float k1 = radius * cg_sqrt(dd) / mm;
        const float k2 = radius * cg_sqrt(cc) / mm;
        const float j1 = k1 * tt / dd;
        const float j2 = k2 * tt / cc;
        const float cx = k1 * b2 + k2 * b1;
        const float cy = k1 * a2 + k2 * a1;
        const float px = b1 * (k2 + j1);
        const float py = a1 * (k2 + j1);
        const float qx = b2 * (k1 + j2);
        const float qy = a2 * (k1 + j2);
        const float start = cg_atan2(py - cy, px - cx);
        const float end = cg_atan2(qy - cy, qx - cx);

        cg_path_arc(path, (cg_vec2_t){.x = cx + pos1.x, .y = cy + pos1.y}, radius, (cg_rads_t){.rads = start}, (cg_rads_t){.rads = end}, (b1 * a2 > b2 * a1) ? cg_rotation_counter_clock_wise : cg_rotation_clock_wise, tag);
    }

    CG_TRACE_POP();
}

static void cg_path_quad_bezier_step(cg_path_t * const path, const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const uint32_t _step, const float _tol, const char * const tag) {
    CG_TRACE_PUSH_FN();

    const float x12 = (x1 + x2) * 0.5f;
    const float y12 = (y1 + y2) * 0.5f;
    const float x23 = (x2 + x3) * 0.5f;
    const float y23 = (y2 + y3) * 0.5f;
    const float x123 = (x12 + x23) * 0.5f;
    const float y123 = (y12 + y23) * 0.5f;

    float dx = x3 - x1;
    float dy = y3 - y1;
    const float d = CG_ABS(((x2 - x3) * dy - (y2 - y3) * dx));

    if (d > CG_SMALL_NUM) {
        if (d * d <= _tol * (dx * dx + dy * dy)) {
            const cg_vec2_t * const pos = cg_vec2(x123, y123);
            cg_path_push(path, pos, tag);

            CG_TRACE_POP();
            return;
        }
    } else {
        dx = x123 - (x1 + x3) * 0.5f;
        dy = y123 - (y1 + y3) * 0.5f;
        if ((dx * dx + dy * dy) <= _tol) {
            const cg_vec2_t * const pos = cg_vec2(x123, y123);
            cg_path_push(path, pos, tag);

            CG_TRACE_POP();
            return;
        }
    }

    if (_step < cg_statics.ctx->config.internal_limits.max_tessellation_steps) {
        cg_path_quad_bezier_step(path, x1, y1, x12, y12, x123, y123, _step + 1, _tol, tag);
        cg_path_quad_bezier_step(path, x123, y123, x23, y23, x3, y3, _step + 1, _tol, tag);
    }
    CG_TRACE_POP();
}

static void cg_path_quad_bezier_to(cg_path_t * const path, const float cpx, const float cpy, const float x, const float y, const char * const tag) {
    CG_TRACE_PUSH_FN();

    const float tol = CG_POW2(1.0f / cg_affine_get_scale(&path->state->transform)) * 0.25f;
    cg_vec2_t cp = *cg_affine_apply(&path->state->transform, cg_vec2(cpx, cpy));
    cg_vec2_t p = *cg_affine_apply(&path->state->transform, cg_vec2(x, y));

    cg_path_quad_bezier_step(path, path->last_point.x, path->last_point.y, cp.x, cp.y, p.x, p.y, 0, tol, tag);
    cg_path_push(path, &p, tag);

    CG_TRACE_POP();
}

static void cg_path_circle(cg_path_t * const path, const cg_vec2_t pos, const float radius, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_arc(path, pos, radius, (cg_rads_t){.rads = 0.0f}, (cg_rads_t){.rads = CG_TAU}, cg_rotation_clock_wise, tag);
    cg_path_close(path, tag);
    CG_TRACE_POP();
}

static void cg_path_rect(cg_path_t * const path, const cg_rect_t rect, const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_path_move_to(path, (cg_vec2_t){.x = rect.x, .y = rect.y}, tag);
    cg_path_line_to(path, (cg_vec2_t){.x = rect.x + rect.width, .y = rect.y}, tag);
    cg_path_line_to(path, (cg_vec2_t){.x = rect.x + rect.width, .y = rect.y + rect.height}, tag);
    cg_path_line_to(path, (cg_vec2_t){.x = rect.x, .y = rect.y + rect.height}, tag);
    cg_path_close(path, tag);

    CG_TRACE_POP();
}

static void cg_path_rounded_rect(cg_path_t * const paths, const cg_rect_t rect, const float radius, const char * const tag) {
    CG_TRACE_PUSH_FN();

    float r = radius;
    if (rect.width < 2.0f * r) {
        r = rect.width * 0.5f;
    }
    if (rect.height < 2.0f * r) {
        r = rect.height * 0.5f;
    }

    const float x = rect.x, y = rect.y, w = rect.width, h = rect.height;
    cg_path_move_to(paths, (cg_vec2_t){.x = x + r, .y = y}, tag);
    cg_path_arc_to(paths, (cg_vec2_t){.x = x + w, .y = y}, (cg_vec2_t){.x = x + w, .y = y + h}, r, tag);
    cg_path_arc_to(paths, (cg_vec2_t){.x = x + w, .y = y + h}, (cg_vec2_t){.x = x, .y = y + h}, r, tag);
    cg_path_arc_to(paths, (cg_vec2_t){.x = x, .y = y + h}, (cg_vec2_t){.x = x, .y = y}, r, tag);
    cg_path_arc_to(paths, (cg_vec2_t){.x = x, .y = y}, (cg_vec2_t){.x = x + w, .y = y}, r, tag);
    cg_path_close(paths, tag);

    CG_TRACE_POP();
}

static void cg_subpath_stroke(cg_gl_state_t * const gl, const cg_subpath_t * const subpath, const cg_state_t * const state, const cg_path_options_e options) {
    CG_TRACE_PUSH_FN();
    if (cg_subpath_size(subpath) < 2) {
        CG_TRACE_POP();
        return;
    }

    const float feather = state->feather;
    const cg_color_t color = {
        state->stroke_style.r,
        state->stroke_style.g,
        state->stroke_style.b,
        state->stroke_style.a * state->global_alpha};
    const int count = cg_subpath_size(subpath);
    const bool closed = subpath->closed;
    const int tri_strip_size = (count - 1) * 10 + (closed ? 10 : 0);
    const float radius = state->line_width * 0.5f;
    int idx = 0;
    cg_vec2_t q0, q1 = {0}, q2, q3 = {0}, q4, q5 = {0}, q6, q7 = {0};
    bool cache_valid = false;

    const int output_count = tri_strip_size;
    cg_gl_vertex_t * verts = cg_gl_state_map_vertex_range(gl, output_count);

    for (int32_t i = 0; i < count - 1; ++i) {
        cg_subpath_wrap_e wrap_mode = closed ? cg_subpath_wrap_normal : cg_subpath_wrap_tangent;
        const cg_vec2_t * const p0 = cg_subpath_at(subpath, i - 1, wrap_mode);
        const cg_vec2_t * const p1 = cg_subpath_at(subpath, i, cg_subpath_wrap_normal);
        const cg_vec2_t * const p2 = cg_subpath_at(subpath, i + 1, cg_subpath_wrap_normal);
        const cg_vec2_t * const p3 = cg_subpath_at(subpath, i + 2, wrap_mode);

        //    printf("B (%f %f) (%f %f) (%f %f) (%f %f)\n",
        //           p0->x, p0->y, p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);

        if (cg_vec2_equal(p1, p2)) {
            continue;
        }

        const cg_vec2_t * const bi_normal = cg_vec2_normalize(cg_vec2_sub(p2, p1));
        const cg_vec2_t * const normal = cg_vec2_rot90(bi_normal);
        const float k = 32.0f;

        // miter length is proportional to the angle between the binormal and normal
        // (this needs a proper limit/bevel!)
        const cg_vec2_t * const tan1 = cg_vec2_rot90(
            cg_vec2_normalize(
                cg_vec2_add(cg_vec2_normalize(cg_vec2_sub(p3, p2)), bi_normal)));
        const float miter1k = radius * cg_inv(cg_vec2_dot(normal, tan1));
        const cg_vec2_t * const miter1 = cg_vec2_scale(tan1, clamp_float(miter1k, 0.0f, k));

        // optimal, ccw wound, single tristrip order
        //
        //    i      i+1
        //    6----7 6----7 feather top
        //    | \  | | \  |
        //    |  \ | |  \ |
        //    4----5 4----5 line top
        //    | \  | | \  |
        // p1 ------------- p2
        //    |  \ | |  \ |
        //    2----3 2----3 line bot
        //    | \  | | \  |
        //    |  \ | |  \ |
        //    0----1 0----1 feather bot

        if (cache_valid == false) {
            cache_valid = true;
            q5 = *cg_vec2_add(p2, miter1);
            q7 = *cg_vec2_add(&q5, cg_vec2_scale(tan1, feather));
            q3 = *cg_vec2_sub(p2, miter1);
            q1 = *cg_vec2_sub(&q3, cg_vec2_scale(tan1, feather));

            const cg_vec2_t * const tan0 = cg_vec2_rot90(
                cg_vec2_normalize(
                    cg_vec2_add(cg_vec2_normalize(cg_vec2_sub(p1, p0)), bi_normal)));
            const float miter0k = radius * cg_inv(cg_vec2_dot(normal, tan0));
            const cg_vec2_t * const miter0 = cg_vec2_scale(tan0, clamp_float(miter0k, 0.0f, k));

            q4 = *cg_vec2_add(p1, miter0);
            q6 = *cg_vec2_add(&q4, cg_vec2_scale(tan0, feather));
            q2 = *cg_vec2_sub(p1, miter0);
            q0 = *cg_vec2_sub(&q2, cg_vec2_scale(tan0, feather));
        } else {
            q0 = q1;
            q2 = q3; // cached previous col
            q4 = q5;
            q6 = q7;

            q5 = *cg_vec2_add(p2, miter1);
            q7 = *cg_vec2_add(&q5, cg_vec2_scale(tan1, feather));
            q3 = *cg_vec2_sub(p2, miter1);
            q1 = *cg_vec2_sub(&q3, cg_vec2_scale(tan1, feather));
        }

        // output continous tristrip (10 verts per segment total)

        cg_color_t z = *cg_color(0, 0, 0, 0), a = *cg_color(0, 0, 0, color.a);
        cg_set_vert(verts, idx++, &q0, &z); // degenerate restart (might need to verify this?)
        cg_set_vert(verts, idx++, &q0, &z);
        cg_set_vert(verts, idx++, &q0, &z);
        cg_set_vert(verts, idx++, &q1, &z); // row 0
        cg_set_vert(verts, idx++, &q2, &a);
        cg_set_vert(verts, idx++, &q3, &a); // row 1
        cg_set_vert(verts, idx++, &q4, &a);
        cg_set_vert(verts, idx++, &q5, &a); // row 2
        cg_set_vert(verts, idx++, &q6, &z);
        cg_set_vert(verts, idx++, &q7, &z); // row 3

        // close path if needed

        if (closed && i == count - 2) {
            cg_set_vert(verts, idx++, &q1, &z); // degenerate restart
            cg_set_vert(verts, idx++, &q1, &z);
            cg_set_vert(verts, idx++, &q1, &z);
            cg_copy_vert(verts, 0 + 2, idx++); // row 0
            cg_set_vert(verts, idx++, &q3, &a);
            cg_copy_vert(verts, 2 + 2, idx++); // row 1
            cg_set_vert(verts, idx++, &q5, &a);
            cg_copy_vert(verts, 4 + 2, idx++); // row 2
            cg_set_vert(verts, idx++, &q7, &z);
            cg_copy_vert(verts, 6 + 2, idx++); // row 3
        }
    }

    cg_gl_state_finish_vertex_range(gl);

    cg_select_blend_and_shader(gl, state, &color, NULL, cg_rgb_fill_alpha_red_disabled);

    cg_gl_state_draw_tri_strip(gl, idx, 0);

    CG_TRACE_POP();
}

static void cg_subpath_fill(cg_gl_state_t * const gl, const cg_subpath_t * const subpath, cg_state_t * const state, const cg_path_options_e options) {
    CG_TRACE_PUSH_FN();

    if ((subpath->closed == false) || (cg_subpath_size(subpath) < 3)) {
        CG_TRACE_POP();
        return;
    }

    const float feather = state->feather;

    const cg_color_t color = {
        state->fill_style.r,
        state->fill_style.g,
        state->fill_style.b,
        state->fill_style.a * state->global_alpha};

    const int count = cg_subpath_size(subpath);
    const int tri_strip_size = (count - 1) * 4 + 2;
    const int fan_size = count * 2 - 2;
    int tidx = 0, fidx0 = tri_strip_size, fidx = fidx0;
    cg_vec2_t q0, q1, q2 = {0}, q3 = {0};
    bool cache_valid = false;

    const float w = ((state->image == NULL) || (state->image->status != cg_image_async_load_complete)) ? 1.0f : 1.0f / state->image->cg_texture.texture->width;
    const float h = ((state->image == NULL) || (state->image->status != cg_image_async_load_complete)) ? 1.0f : 1.0f / state->image->cg_texture.texture->height;
    const float a = color.a;

    const int output_count = tri_strip_size + fan_size;
    cg_gl_vertex_t * verts = cg_gl_state_map_vertex_range(gl, output_count);

    for (int32_t i = 0; i < count - 1; ++i) {
        const cg_vec2_t * const p0 = cg_subpath_at(subpath, i - 1, cg_subpath_wrap_normal);
        const cg_vec2_t * const p1 = cg_subpath_at(subpath, i, cg_subpath_wrap_normal);
        const cg_vec2_t * const p2 = cg_subpath_at(subpath, i + 1, cg_subpath_wrap_normal);
        const cg_vec2_t * const p3 = cg_subpath_at(subpath, i + 2, cg_subpath_wrap_normal);

        if (cg_vec2_equal(p1, p2)) {
            continue;
        }

        const cg_vec2_t * const bi_normal = cg_vec2_normalize(cg_vec2_sub(p2, p1));

        // miter length is proportional to the angle between the binormal and normal
        // (this needs a proper limit!)
        const cg_vec2_t * const tan1 = cg_vec2_rot90(
            cg_vec2_normalize(
                cg_vec2_add(cg_vec2_normalize(cg_vec2_sub(p3, p2)), bi_normal)));

        // optimal, ccw wound, single tristrip order
        //
        //    i      i+1
        //    0----2 0----2    line bot
        //    |  / | |  / |
        //    | /  | | /  |
        // p1 1----3 1----3 p2 feather bot

        if (cache_valid == false) {
            cache_valid = true;
            const cg_vec2_t * const tan0 = cg_vec2_rot90(
                cg_vec2_normalize(
                    cg_vec2_add(cg_vec2_normalize(cg_vec2_sub(p1, p0)), bi_normal)));

            q0 = *cg_vec2_sub(p1, cg_vec2_scale(tan0, feather));
            q1 = *p1;
            q2 = *cg_vec2_sub(p2, cg_vec2_scale(tan1, feather));
            q3 = *p2;
        } else {
            q0 = q2; // cached previous col
            q1 = q3;

            q2 = *cg_vec2_sub(p2, cg_vec2_scale(tan1, feather));
            q3 = *p2;
        }

        // output continous tristrip (feather)
        cg_set_vert(verts, tidx++, &q0, cg_color(q0.x * w, q0.y * h, 0, 0));
        cg_set_vert(verts, tidx++, &q1, cg_color(q1.x * w, q1.y * h, 0, a));
        cg_set_vert(verts, tidx++, &q2, cg_color(q2.x * w, q2.y * h, 0, 0));
        cg_set_vert(verts, tidx++, &q3, cg_color(q3.x * w, q3.y * h, 0, a));

        // output trifan (fill)

        cg_set_vert(verts, fidx++, &q1, cg_color(q1.x * w, q1.y * h, 0, a));
        cg_set_vert(verts, fidx++, &q3, cg_color(q3.x * w, q3.y * h, 0, a));

        // close path if needed

        if (i == count - 2) {
            cg_copy_vert(verts, 0, tidx++);
            cg_copy_vert(verts, 1, tidx++);
        }
    }

    cg_gl_state_finish_vertex_range(gl);

    cg_select_blend_and_shader(gl, state, &color, (state->image && (state->image->status == cg_image_async_load_complete)) ? &state->image->cg_texture : NULL, cg_rgb_fill_alpha_red_disabled);

    if (options & cg_path_options_concave) {
        // concave

        // fill
        cg_gl_state_set_mode_stencil_accum(gl);
        cg_gl_state_draw_tri_fan(gl, fan_size, fidx0);

        // feather
        cg_gl_state_set_mode_stencil_eq(gl);
        cg_gl_state_draw_tri_strip(gl, tidx, 0);

        // fill
        cg_gl_state_set_mode_stencil_neq(gl);
        cg_gl_state_draw_tri_fan(gl, fan_size, fidx0);

        cg_gl_state_set_mode_stencil_off(gl);
    } else {
        // convex

        // feather
        if ((options & cg_path_options_no_fethering) == 0) {
            cg_gl_state_draw_tri_strip(gl, tidx, 0);
        }

        // fill
        cg_gl_state_draw_tri_fan(gl, fan_size, fidx0);
    }

    CG_TRACE_POP();
}

/* ===========================================================================
 * CANVAS
 * ==========================================================================*/

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
    const char * const tag) {
    CG_TRACE_PUSH_FN();
    ZEROMEM(ctx);
    ASSERT(canvas_config.internal_limits.max_states);
#ifdef GUARD_PAGE_SUPPORT
    if (memory_initializers.guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&ctx->cg_heap_low.heap, memory_initializers.low_mem_region.size, 8, 0, "canvas_heap_low", memory_initializers.guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(&ctx->cg_heap_low.heap, memory_initializers.low_mem_region, 8, 0, "canvas_heap_low");
    }

    ctx->high_mem_region = MEM_REGION(.ptr = NULL, .size = memory_initializers.high_mem_size);
    ctx->guard_page_mode = memory_initializers.guard_page_mode;
    cg_context_set_memory_mode(ctx, memory_initializers.initial_memory_mode);

    ctx->log_image_timing = image_logging == cg_context_image_time_logging_enabled;

    ctx->thread_pool = thread_pool;
    ctx->cg_heap_low.mutex = sb_create_mutex(tag);
    ctx->cg_heap_high.mutex = sb_create_mutex(tag);
    ctx->gl = gl;
    ctx->config = canvas_config;

    ctx->state_idx = 0;

    ctx->states = cg_alloc(&ctx->cg_heap_low, sizeof(*ctx->states) * ctx->config.internal_limits.max_states, MALLOC_TAG);

    ctx->cur_state = &ctx->states[ctx->state_idx];
    cg_state_init(ctx->cur_state, ctx);

    ctx->path = *cg_path(ctx->cur_state, tag);

    sb_display_mode_t virtual_display = display_size_override == cg_context_display_size_720p ? (sb_display_mode_t){.width = 1280, .height = 720, .hz = 60} : display_mode;

    ctx->width = virtual_display.width;
    ctx->height = virtual_display.height;
    ctx->view_scale_x = ((float)display_mode.width / (float)virtual_display.width);
    ctx->view_scale_y = ((float)display_mode.height / (float)virtual_display.height);
    ctx->clear_color = 0x000000FF;

    ctx->using_video_texture = false;
    cg_gl_state_init(ctx->gl, &ctx->cg_heap_low, render_device, canvas_config.gl);

    ctx->mosaic_ctx = mosaic_context_new(
        ctx,
        canvas_config.font_atlas.width ? canvas_config.font_atlas.width : virtual_display.width,
        canvas_config.font_atlas.height ? canvas_config.font_atlas.height : virtual_display.height,
        memory_initializers.font_scratchpad_mem,
        memory_initializers.guard_page_mode,
        tag);

    CG_TRACE_POP();
}

void cg_context_free(cg_context_t * const ctx, const char * const tag) {
    CG_TRACE_PUSH_FN();

    mosaic_context_free(ctx->mosaic_ctx);
    cg_path_free(&ctx->path, tag);

    cg_free(&ctx->cg_heap_low, ctx->states, MALLOC_TAG);

    while (ctx->free_block_chain) {
        cg_mem_block_t * block = ctx->free_block_chain->next;
        cg_free(&ctx->cg_heap_low, ctx->free_block_chain, MALLOC_TAG);
        ctx->free_block_chain = block;
    }

#ifndef _SHIP
    heap_debug_print_leaks(&ctx->cg_heap_low.heap);
    if (ctx->cg_heap_high.heap.internal.init) {
        heap_debug_print_leaks(&ctx->cg_heap_high.heap);
    }
#endif
    if (ctx->cg_heap_high.heap.internal.init && (ctx->guard_page_mode == system_guard_page_mode_disabled)) {
        sb_unmap_pages(ctx->high_mem_region);
        ZEROMEM(&ctx->cg_heap_high.heap);
    }
    sb_destroy_mutex(ctx->cg_heap_low.mutex, tag);
    sb_destroy_mutex(ctx->cg_heap_high.mutex, tag);

    CG_TRACE_POP();
}

void cg_context_set_memory_mode(cg_context_t * const ctx, const cg_memory_mode_e memory_mode) {
    if ((memory_mode == cg_memory_mode_low) && ctx->cg_heap_high.heap.internal.init) {
#ifndef _SHIP
        heap_debug_print_leaks(&ctx->cg_heap_high.heap);
#endif
        if (ctx->guard_page_mode == system_guard_page_mode_disabled) {
            sb_unmap_pages(ctx->high_mem_region);
        }
        ZEROMEM(&ctx->cg_heap_high.heap);
    } else if ((memory_mode == cg_memory_mode_high) && !ctx->cg_heap_high.heap.internal.init) {
#ifndef _SHIP
        if (ctx->guard_page_mode == system_guard_page_mode_enabled) {
            debug_heap_init(&ctx->cg_heap_high.heap, PAGE_ALIGN_INT(ctx->high_mem_region.size), 8, 0, "canvas_heap_high", ctx->guard_page_mode, MALLOC_TAG);
        } else
#endif
        {
            ctx->high_mem_region = sb_map_pages(PAGE_ALIGN_INT(ctx->high_mem_region.size), system_page_protect_read_write);
            heap_init_with_region(&ctx->cg_heap_high.heap, ctx->high_mem_region, 8, 0, "canvas_heap_high");
        }
    }
}

void cg_context_dump_heap_usage(cg_context_t * const ctx) {
    CG_TRACE_PUSH_FN();

    sb_lock_mutex(ctx->cg_heap_low.mutex);
    heap_dump_usage(&ctx->cg_heap_low.heap);
    sb_unlock_mutex(ctx->cg_heap_low.mutex);

    if (ctx->cg_heap_high.heap.internal.init) {
        sb_lock_mutex(ctx->cg_heap_high.mutex);
        heap_dump_usage(&ctx->cg_heap_high.heap);
        sb_unlock_mutex(ctx->cg_heap_high.mutex);
    }

    CG_TRACE_POP();
}

static heap_metrics_t cg_context_get_heap_metrics(cg_heap_t * const cg_heap) {
    CG_TRACE_PUSH_FN();
    sb_lock_mutex(cg_heap->mutex);
    const heap_metrics_t metrics = heap_get_metrics(&cg_heap->heap);
    sb_unlock_mutex(cg_heap->mutex);
    CG_TRACE_POP();
    return metrics;
}

heap_metrics_t cg_context_get_heap_metrics_high(cg_context_t * const ctx) {
    return cg_context_get_heap_metrics(&ctx->cg_heap_high);
}

heap_metrics_t cg_context_get_heap_metrics_low(cg_context_t * const ctx) {
    return cg_context_get_heap_metrics(&ctx->cg_heap_low);
}

cg_context_t * cg_get_context() {
    return cg_statics.ctx;
}

void cg_set_context(cg_context_t * const ctx) {
    cg_statics.ctx = ctx;
}

void cg_context_save() {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    VERIFY_MSG(ctx->state_idx < ctx->config.internal_limits.max_states - 1, "Cannot save more context states, max states allowed: %i", ctx->config.internal_limits.max_states);
    ctx->cur_state = &ctx->states[++ctx->state_idx];
    *ctx->cur_state = ctx->states[ctx->state_idx - 1];
    ctx->path.state = ctx->cur_state;

    CG_TRACE_POP();
}

void cg_context_restore() {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    VERIFY_MSG(ctx->state_idx != 0, "Mismatched cg_context_save/cg_context_restore");
    ctx->cur_state = &ctx->states[--ctx->state_idx];
    ctx->path.state = ctx->cur_state;
    cg_clip_state_t * const clip_state = &ctx->cur_state->clip_state;

    cg_gl_set_scissor_rect(ctx->gl, (int)(clip_state->x0 * ctx->view_scale_x), (int)(clip_state->y0 * ctx->view_scale_y), (int)(clip_state->x1 * ctx->view_scale_x), (int)(clip_state->y1 * ctx->view_scale_y));
    cg_gl_enable_scissor(ctx->gl, clip_state->clip_state == cg_clip_state_enabled);

    CG_TRACE_POP();
}

void cg_context_identity() {
    CG_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;
    cg_affine_identity(&ctx->cur_state->transform);
    CG_TRACE_POP();
}

void cg_context_tick_gifs(cg_context_t * const ctx, const milliseconds_t delta_time);

void cg_context_begin(const milliseconds_t delta_time) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    ctx->using_video_texture = false;
    cg_context_tick_gifs(ctx, delta_time);
    cg_affine_identity(&ctx->cur_state->transform);
    cg_gl_state_begin(ctx->gl, ctx->width, ctx->height, ctx->clear_color);

    CG_TRACE_POP();
}

void cg_context_end(const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    cg_gl_state_end(ctx->gl);
    cg_path_reset(&ctx->path, tag);

    ctx->state_idx = 0;
    ctx->cur_state = &ctx->states[ctx->state_idx];
    cg_state_init(ctx->cur_state, ctx);

    CG_TRACE_POP();
}

float cg_context_global_alpha() {
    return cg_statics.ctx->cur_state->global_alpha;
}

void cg_context_set_global_alpha(const float alpha) {
    CG_TRACE_PUSH_FN();
    cg_statics.ctx->cur_state->global_alpha = alpha;
    CG_TRACE_POP();
}

float cg_context_feather() {
    return cg_statics.ctx->cur_state->feather;
}

void cg_context_set_feather(const float feather) {
    CG_TRACE_PUSH_FN();
    cg_statics.ctx->cur_state->feather = feather;
    CG_TRACE_POP();
}

void cg_context_set_clear_color(const uint32_t color) {
    CG_TRACE_PUSH_FN();
    cg_statics.ctx->clear_color = color;
    CG_TRACE_POP();
}

float cg_context_get_alpha_test_threshold() {
    return cg_statics.ctx->cur_state->alpha_test_threshold;
}

void cg_context_set_alpha_test_threshold(const float threshold) {
    CG_TRACE_PUSH_FN();
    cg_statics.ctx->cur_state->alpha_test_threshold = threshold;
    CG_TRACE_POP();
}

cg_blend_mode_e cg_context_get_punchthrough_blend_mode() {
    return cg_statics.ctx->cur_state->blend_mode;
}

void cg_context_set_punchthrough_blend_mode(const cg_blend_mode_e blend_mode) {
    CG_TRACE_PUSH_FN();
    // If we're using texture based playback and we've set the flag to use the correct behavior, use default blend mode
    if (cg_statics.ctx->using_video_texture && cg_statics.ctx->config.enable_punchthrough_blend_mode_fix) {
        cg_statics.ctx->cur_state->blend_mode = cg_blend_mode_src_alpha_rgb;
    } else {
        cg_statics.ctx->cur_state->blend_mode = blend_mode;
    }
    CG_TRACE_POP();
}

void cg_context_begin_path(const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_reset(&cg_statics.ctx->path, tag);
    CG_TRACE_POP();
}

void cg_context_end_path(const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_end(&cg_statics.ctx->path, tag);
    CG_TRACE_POP();
}

void cg_context_close_path(const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_close(&cg_statics.ctx->path, tag);
    CG_TRACE_POP();
}

void cg_context_move_to(const cg_vec2_t pos, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_move_to(&cg_statics.ctx->path, pos, tag);
    CG_TRACE_POP();
}

void cg_context_line_to(const cg_vec2_t pos, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_line_to(&cg_statics.ctx->path, pos, tag);
    CG_TRACE_POP();
}

void cg_context_rect(const cg_rect_t rect, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_rect(&cg_statics.ctx->path, rect, tag);
    CG_TRACE_POP();
}

void cg_context_rounded_rect(const cg_rect_t rect, const float radius, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_rounded_rect(&cg_statics.ctx->path, rect, radius, tag);
    CG_TRACE_POP();
}

void cg_context_fill_rect(const cg_rect_t rect, const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    cg_path_t path = *cg_path(ctx->cur_state, tag);
    cg_path_rect(&path, rect, tag);
    cg_subpath_fill(ctx->gl, &path.paths[0], ctx->cur_state, cg_path_options_none);
    cg_path_free(&path, tag);

    CG_TRACE_POP();
}

void cg_context_stroke_rect(const cg_rect_t rect, const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    cg_path_t path = *cg_path(ctx->cur_state, tag);
    cg_path_rect(&path, rect, tag);
    cg_subpath_stroke(ctx->gl, &path.paths[0], ctx->cur_state, cg_path_options_none);
    cg_path_free(&path, tag);

    CG_TRACE_POP();
}

void cg_context_clear_rect(const cg_rect_t rect, const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    cg_path_t path = *cg_path(ctx->cur_state, tag);
    cg_path_rect(&path, rect, tag);

    cg_color_t old_color = ctx->cur_state->fill_style;
    ctx->cur_state->fill_style = *cg_color(0, 0, 0, 1);
    cg_subpath_fill(ctx->gl, &path.paths[0], ctx->cur_state, cg_path_options_none);
    cg_path_free(&path, tag);
    ctx->cur_state->fill_style = old_color;

    CG_TRACE_POP();
}

void cg_context_fill_with_options(const cg_path_options_e options, const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    if (cg_subpath_size(&ctx->path.cur_path) > 0) {
        cg_path_close(&ctx->path, tag);
    }

    const size_t count = cg_path_size(&ctx->path);
    for (size_t i = 0; i < count; ++i) {
        if (cg_subpath_winding(&ctx->path.paths[i]) == cg_subpath_winding_ccw) {
            cg_subpath_reverse(&ctx->path.paths[i]);
        }

        cg_subpath_fill(ctx->gl, &ctx->path.paths[i], ctx->cur_state, options);
    }

    CG_TRACE_POP();
}

void cg_context_fill(const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_context_fill_with_options(cg_path_options_none, tag);
    CG_TRACE_POP();
}

void cg_context_stroke_with_options(const cg_path_options_e options, const char * const tag) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;
    if (cg_subpath_size(&ctx->path.cur_path) > 0) {
        cg_path_end(&ctx->path, tag);
    }

    const size_t count = cg_path_size(&ctx->path);
    for (size_t i = 0; i < count; ++i) {
        if (cg_subpath_winding(&ctx->path.paths[i]) == cg_subpath_winding_ccw) {
            cg_subpath_reverse(&ctx->path.paths[i]);
        }

        cg_subpath_stroke(ctx->gl, &ctx->path.paths[i], ctx->cur_state, options);
    }
    CG_TRACE_POP();
}

void cg_context_stroke(const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_context_stroke_with_options(cg_path_options_none, tag);
    CG_TRACE_POP();
}

void cg_context_quad_bezier_to(const float cpx, const float cpy, const float x, const float y, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_quad_bezier_to(&cg_statics.ctx->path, cpx, cpy, x, y, tag);
    CG_TRACE_POP();
}

void cg_context_arc(const cg_vec2_t pos, const float radius, const cg_rads_t start, const cg_rads_t end, const cg_rotation_e rotation, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_arc(&cg_statics.ctx->path, pos, radius, start, end, rotation, tag);
    CG_TRACE_POP();
}

void cg_context_arc_to(const cg_vec2_t pos1, const cg_vec2_t pos2, const float radius, const char * const tag) {
    CG_TRACE_PUSH_FN();
    cg_path_arc_to(&cg_statics.ctx->path, pos1, pos2, radius, tag);
    CG_TRACE_POP();
}

void cg_context_rotate(const cg_rads_t angle) {
    CG_TRACE_PUSH_FN();
    cg_affine_rotate(&cg_statics.ctx->cur_state->transform, angle.rads);
    CG_TRACE_POP();
}

void cg_context_scale(const cg_vec2_t scale) {
    CG_TRACE_PUSH_FN();
    cg_affine_scale(&cg_statics.ctx->cur_state->transform, scale.x, scale.y);
    CG_TRACE_POP();
}

void cg_context_translate(const cg_vec2_t translation) {
    CG_TRACE_PUSH_FN();
    cg_affine_translate(&cg_statics.ctx->cur_state->transform, translation.x, translation.y);
    CG_TRACE_POP();
}

void cg_context_set_line_width(const float width) {
    cg_statics.ctx->cur_state->line_width = width;
}

void cg_context_stroke_style(const cg_color_t color) {
    cg_statics.ctx->cur_state->stroke_style = (cg_color_t){.r = color.r / 255.f, .g = color.g / 255.f, .b = color.b / 255.f, .a = color.a / 255.f};
}

void cg_context_fill_style(const cg_color_t color) {
    cg_statics.ctx->cur_state->fill_style = (cg_color_t){.r = color.r / 255.f, .g = color.g / 255.f, .b = color.b / 255.f, .a = color.a / 255.f};
    cg_statics.ctx->cur_state->image = NULL;
}

void cg_context_fill_style_hex(const int32_t color) {
    const float r = (color & 0xf00) * (1.0f / 0xf00);
    const float g = (color & 0x0f0) * (1.0f / 0x0f0);
    const float b = (color & 0x00f) * (1.0f / 0x00f);

    cg_statics.ctx->cur_state->fill_style = (cg_color_t){.r = r, .g = g, .b = b, .a = 1};
    cg_statics.ctx->cur_state->image = NULL;
}

void cg_context_fill_style_image(const cg_color_t color, const cg_image_t * const image) {
    cg_context_fill_style(color);
    cg_statics.ctx->cur_state->image = image;
}

void cg_context_fill_style_image_hex(const int32_t color, const cg_image_t * const image) {
    cg_context_fill_style_hex(color);
    cg_statics.ctx->cur_state->image = image;
}

void cg_context_draw_image_rect(const cg_image_t * const image, const cg_rect_t src, const cg_rect_t dst) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;

    const cg_affine_t * const xform = &ctx->cur_state->transform;
    const cg_vec2_t * const p0 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y));
    const cg_vec2_t * const p1 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y));
    const cg_vec2_t * const p2 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y + dst.height));
    const cg_vec2_t * const p3 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y + dst.height));

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    // normalize tex-coords
    ASSERT(!drawable_image || drawable_image->cg_texture.texture);

    const float iw = 1.0f / (drawable_image ? drawable_image->cg_texture.texture->width : 1);
    const float ih = 1.0f / (drawable_image ? drawable_image->cg_texture.texture->height : 1);
    const float u = src.x * iw, v = src.y * ih;
    const float w = src.width * iw, h = src.height * ih;

    const float a = ctx->cur_state->fill_style.a * ctx->cur_state->global_alpha;

    cg_gl_vertex_t * verts = cg_gl_state_map_vertex_range(ctx->gl, 4);

    cg_set_vert(verts, 0, p0, cg_color(u, v, 0, a));
    cg_set_vert(verts, 1, p1, cg_color(u + w, v, 0, a));
    cg_set_vert(verts, 2, p2, cg_color(u + w, v + h, 0, a));
    cg_set_vert(verts, 3, p3, cg_color(u, v + h, 0, a));

    cg_gl_state_finish_vertex_range(ctx->gl);

    if (NULL == drawable_image) {
        cg_select_blend_and_shader(ctx->gl, ctx->cur_state, &ctx->cur_state->fill_style, NULL, cg_rgb_fill_alpha_red_disabled);
    } else if (NULL != drawable_image->cg_texture_mask.texture) {
        // Detected that we have alpha mask passed in
        cg_select_blend_and_shader_alpha_mask(ctx->gl, ctx->cur_state, &ctx->cur_state->fill_style, drawable_image ? &drawable_image->cg_texture : NULL, drawable_image ? &drawable_image->cg_texture_mask : NULL);
    } else {
        cg_select_blend_and_shader(ctx->gl, ctx->cur_state, &ctx->cur_state->fill_style, drawable_image ? &drawable_image->cg_texture : NULL, cg_rgb_fill_alpha_red_disabled);
    }

    cg_gl_state_draw_tri_fan(ctx->gl, 4, 0);

    CG_TRACE_POP();
}

typedef struct cg_rect_positions_t {
    cg_vec2_t p00, p01, p11, p10;
} cg_rect_positions_t;

static cg_rect_positions_t cg_affine_apply_rect(const cg_rect_t rect) {
    cg_context_t * const ctx = cg_statics.ctx;

    const cg_affine_t * const xform = &ctx->cur_state->transform;
    return (cg_rect_positions_t){
        .p00 = *cg_affine_apply(xform, cg_vec2(rect.x, rect.y)),
        .p01 = *cg_affine_apply(xform, cg_vec2(rect.x + rect.width, rect.y)),
        .p11 = *cg_affine_apply(xform, cg_vec2(rect.x + rect.width, rect.y + rect.height)),
        .p10 = *cg_affine_apply(xform, cg_vec2(rect.x, rect.y + rect.height)),
    };
}

static cg_box_t cg_get_box(const cg_rect_t rect) {
    const cg_rect_positions_t rect_pos = cg_affine_apply_rect(rect);
    const float width = rect_pos.p01.x - rect_pos.p00.x;
    const float height = rect_pos.p10.y - rect_pos.p00.y;
    return (cg_box_t){.centerpoint = {rect_pos.p00.x + width / 2, rect_pos.p00.y + height / 2}, .half_dim = {width / 2, height / 2}};
}

typedef struct cg_2d_mesh_t {
    cg_gl_vertex_t * verts;
    int32_t vert_index;
    const int32_t max_verts;
} cg_2d_mesh_t;

static void cg_set_quad_verts(cg_2d_mesh_t * const mesh, const cg_vec2_t inverse_img_dims, const cg_rect_t src, const cg_rect_t dst, const float alpha) {
    const float u0 = src.x * inverse_img_dims.x;
    const float v0 = src.y * inverse_img_dims.y;
    const float u1 = u0 + src.width * inverse_img_dims.x;
    const float v1 = v0 + src.height * inverse_img_dims.y;

    const cg_rect_positions_t rect_pos = cg_affine_apply_rect(dst);
    ASSERT(mesh->vert_index + 6 <= mesh->max_verts);

    cg_set_vert(mesh->verts, mesh->vert_index++, &rect_pos.p00, cg_color(u0, v0, 0.0f, alpha));
    cg_set_vert(mesh->verts, mesh->vert_index++, &rect_pos.p01, cg_color(u1, v0, 0.0f, alpha));
    cg_set_vert(mesh->verts, mesh->vert_index++, &rect_pos.p11, cg_color(u1, v1, 0.0f, alpha));

    cg_set_vert(mesh->verts, mesh->vert_index++, &rect_pos.p11, cg_color(u1, v1, 0.0f, alpha));
    cg_set_vert(mesh->verts, mesh->vert_index++, &rect_pos.p10, cg_color(u0, v1, 0.0f, alpha));
    cg_set_vert(mesh->verts, mesh->vert_index++, &rect_pos.p00, cg_color(u0, v0, 0.0f, alpha));
}

typedef enum cg_sdf_stroke_mode_e {
    cg_sdf_stroke_enabled,
    cg_sdf_stroke_disabled,
} cg_sdf_stroke_mode_e;

static void cg_finish_range_and_draw_2d_sdf_rect_mesh(cg_2d_mesh_t * const mesh, const cg_image_t * const drawable_image, const cg_box_t box, const cg_sdf_rect_params_t draw_params, const cg_sdf_stroke_mode_e stroke_mode) {
    CG_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;
    ASSERT_MSG((stroke_mode != cg_sdf_stroke_enabled) || (draw_params.fade == 0.0), "Strokes and fades together produce bad output when rendered and is not functioning correctly currently. Run m5 in release to see the poor output.");
    cg_gl_state_finish_vertex_range(ctx->gl);

    cg_gl_state_set_mode_blend_alpha_rgb(ctx->gl);

    const cg_sdf_rect_uniforms_t rect_uniforms = {
        .box = box,
        .roundness = draw_params.roundness,
        .fade = draw_params.fade,
    };

    if (stroke_mode == cg_sdf_stroke_enabled) {
        const cg_sdf_rect_border_uniforms_t border_uniforms = {
            .sdf_rect_uniforms = rect_uniforms,
            .stroke_color = draw_params.border_color,
            .stroke_size = draw_params.border_width,
        };
        cg_gl_state_bind_sdf_rect_border_shader(
            ctx->gl,
            &ctx->cur_state->fill_style,
            drawable_image ? &drawable_image->cg_texture : NULL,
            border_uniforms);
    } else {
        cg_gl_state_bind_sdf_rect_shader(
            ctx->gl,
            &ctx->cur_state->fill_style,
            drawable_image ? &drawable_image->cg_texture : NULL,
            rect_uniforms);
    }

    cg_gl_state_draw(ctx->gl, rhi_triangles, mesh->vert_index, 0);
    CG_TRACE_POP();
}

void cg_context_sdf_draw_image_rect_rounded(const cg_image_t * const image, const cg_rect_t src, const cg_rect_t dst, const cg_sdf_rect_params_t draw_params) {
    CG_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;
    const float scale = cg_affine_get_scale(&ctx->cur_state->transform);
    const float smallest_dim = min_float(dst.width, dst.height);
    const float radius = max_float(min_float(smallest_dim / 2, draw_params.roundness), 0.0) * scale;
    const cg_box_t box = cg_get_box(dst);

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    ASSERT(!drawable_image || drawable_image->cg_texture.texture);
    // normalize tex-coords
    const float iw = 1.0f / (drawable_image ? drawable_image->cg_texture.texture->width : 1);
    const float ih = 1.0f / (drawable_image ? drawable_image->cg_texture.texture->height : 1);

    cg_gl_vertex_t * verts = cg_gl_state_map_vertex_range(ctx->gl, 6);
    cg_2d_mesh_t mesh = {.verts = verts, .max_verts = 6};
    cg_set_quad_verts(&mesh, (cg_vec2_t){iw, ih}, src, dst, ctx->cur_state->fill_style.a * ctx->cur_state->global_alpha);

    cg_finish_range_and_draw_2d_sdf_rect_mesh(&mesh, drawable_image, box, (cg_sdf_rect_params_t){.roundness = radius, .fade = max_float(draw_params.fade, 0.f) * scale, .border_width = draw_params.border_width, .border_color = draw_params.border_color}, cg_sdf_stroke_disabled);

    CG_TRACE_POP();
}

void cg_context_sdf_fill_image_rect_rounded(const cg_image_t * const image, const cg_rect_t dst, const cg_sdf_rect_params_t draw_params, const cg_image_tiling_e tiling) {
    CG_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;
    const float scale = cg_affine_get_scale(&ctx->cur_state->transform);

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    ASSERT(!drawable_image || drawable_image->cg_texture.texture);

    const float smallest_dim = min_float(dst.width, dst.height);
    const float radius = max_float(min_float(smallest_dim / 2, draw_params.roundness), 0.0) * scale;
    const cg_box_t box = cg_get_box(dst);
    const cg_rect_t src = cg_context_image_rect(image);

    // normalize tex-coords
    const cg_vec2_t inverse_img_dims = {
        1.0f / src.width,
        1.0f / src.height};

    cg_vec2_t uv_repeats;
    if (tiling == cg_image_tiling_stretch) {
        uv_repeats = (cg_vec2_t){
            .x = (src.width + draw_params.border_width * 2) / src.width,
            .y = (src.height + draw_params.border_width * 2) / src.height};
    } else {
        uv_repeats = (cg_vec2_t){
            .x = (dst.width + draw_params.border_width * 2) / (src.width / scale),
            .y = (dst.height + draw_params.border_width * 2) / (src.height / scale)};
    }

    const cg_rect_t dst_rect = {
        .x = dst.x - draw_params.border_width,
        .y = dst.y - draw_params.border_width,
        .width = dst.width + 2 * draw_params.border_width,
        .height = dst.height + 2 * draw_params.border_width};

    cg_vec2_t src_start_coord;
    if (tiling == cg_image_tiling_relative || tiling == cg_image_tiling_stretch) {
        src_start_coord = (cg_vec2_t){
            .x = draw_params.border_width > 0.f ? src.width - draw_params.border_width : src.x,
            .y = draw_params.border_width > 0.f ? src.height - draw_params.border_width : src.y,
        };
    } else {
        ASSERT(tiling == cg_image_tiling_absolute);
        src_start_coord = (cg_vec2_t){fmodf(dst_rect.x * scale, src.width), fmodf(dst_rect.y * scale, src.height)};
    }

    const cg_rect_t src_rect = {
        .x = src_start_coord.x,
        .y = src_start_coord.y,
        .width = src.width * uv_repeats.x,
        .height = src.height * uv_repeats.y,
    };

    cg_gl_vertex_t * const verts = cg_gl_state_map_vertex_range(ctx->gl, 6);
    cg_2d_mesh_t mesh = (cg_2d_mesh_t){.verts = verts, .max_verts = 6};

    cg_set_quad_verts(&mesh, inverse_img_dims, src_rect, dst_rect, ctx->cur_state->fill_style.a * ctx->cur_state->global_alpha);
    cg_finish_range_and_draw_2d_sdf_rect_mesh(
        &mesh,
        drawable_image,
        box,
        (cg_sdf_rect_params_t){
            .roundness = radius,
            .fade = max_float(draw_params.fade, 0.f) * scale,
            .border_width = draw_params.border_width,
            .border_color = draw_params.border_color},
        draw_params.border_width > 0.f ? cg_sdf_stroke_enabled : cg_sdf_stroke_disabled);
    CG_TRACE_POP();
}

void cg_context_sdf_fill_rect_rounded(const cg_rect_t rect, const cg_sdf_rect_params_t draw_params) {
    CG_TRACE_PUSH_FN();
    cg_context_t * const ctx = cg_statics.ctx;

    const cg_image_t white_image = {
        .cg_texture = ctx->gl->white,
        .image = (image_t){
            .width = 1,
            .height = 1,
        },
        .status = cg_image_async_load_complete,
    };
    cg_context_sdf_fill_image_rect_rounded(&white_image, rect, draw_params, cg_image_tiling_stretch);
    CG_TRACE_POP();
}

void cg_context_draw_image_rect_alpha_mask(const cg_image_t * const image, const cg_image_t * const mask, const cg_rect_t src, const cg_rect_t dst) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;

    const cg_affine_t * const xform = &ctx->cur_state->transform;
    const cg_vec2_t * const p0 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y));
    const cg_vec2_t * const p1 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y));
    const cg_vec2_t * const p2 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y + dst.height));
    const cg_vec2_t * const p3 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y + dst.height));

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    ASSERT(!drawable_image || drawable_image->cg_texture.texture);

    const cg_image_t * const mask_image = (mask->status == cg_image_async_load_complete) ? mask : NULL;
    ASSERT(!mask_image || mask_image->cg_texture.texture);

    // normalize tex-coords
    const float iw = 1.0f / (drawable_image ? drawable_image->cg_texture.texture->width : 1);
    const float ih = 1.0f / (drawable_image ? drawable_image->cg_texture.texture->height : 1);
    const float u = src.x * iw, v = src.y * ih;
    const float w = src.width * iw, h = src.height * ih;

    // apply fill style and global alpha
    const float a = ctx->cur_state->fill_style.a * ctx->cur_state->global_alpha;

    cg_gl_vertex_t * const verts = cg_gl_state_map_vertex_range(ctx->gl, 4);

    cg_set_vert(verts, 0, p0, cg_color(u, v, 0, a));
    cg_set_vert(verts, 1, p1, cg_color(u + w, v, 0, a));
    cg_set_vert(verts, 2, p2, cg_color(u + w, v + h, 0, a));
    cg_set_vert(verts, 3, p3, cg_color(u, v + h, 0, a));

    cg_gl_state_finish_vertex_range(ctx->gl);

    // Instead of calling cg_select_blend_and_shader(),
    // using a custom switch here since alpha mask uses a different shader
    switch (ctx->cur_state->blend_mode) {
        case cg_blend_mode_alpha_test:
            // Not supported unless we want to add an additional shader for handling this exact case.
            // Currently falls back to the blit mode
        case cg_blend_mode_blit:
            cg_gl_state_set_mode_blit(ctx->gl);
            break;
        case cg_blend_mode_src_alpha_all:
            cg_gl_state_set_mode_blend_alpha_all(ctx->gl);
            break;
        case cg_blend_mode_src_alpha_rgb:
        default:
            cg_gl_state_set_mode_blend_alpha_rgb(ctx->gl);
            break;
    }

    cg_gl_state_bind_color_shader_alpha_mask(ctx->gl, &ctx->cur_state->fill_style, drawable_image ? &drawable_image->cg_texture : NULL, mask_image ? &mask_image->cg_texture : NULL);

    cg_gl_state_draw_tri_fan(ctx->gl, 4, 0);

    CG_TRACE_POP();
}

//
//  NOTE: 9slice image corners are drawn 1:1 (no texture scaling)
//        vertical / horizontal sides are stretched
void cg_context_draw_image_9slice(const cg_image_t * const image, const cg_margins_t margin, const cg_rect_t dst) {
    CG_TRACE_PUSH_FN();

    if ((dst.width <= 0) || (dst.height <= 0)) {
        CG_TRACE_POP();
        return;
    }

    cg_context_t * const ctx = cg_statics.ctx;

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    // normalize tex-coords
    ASSERT(!drawable_image || drawable_image->cg_texture.texture);

    const float sw = drawable_image ? 1.0f / drawable_image->cg_texture.texture->width : 0.0f;
    const float sh = drawable_image ? 1.0f / drawable_image->cg_texture.texture->height : 0.0f;

    const cg_affine_t * const xform = &ctx->cur_state->transform;
    const cg_vec2_t * const p0 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y));
    const cg_vec2_t * const p1 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y + margin.top));
    const cg_vec2_t * const p2 = cg_affine_apply(xform, cg_vec2(dst.x + margin.left, dst.y + margin.top));
    const cg_vec2_t * const p3 = cg_affine_apply(xform, cg_vec2(dst.x + margin.left, dst.y));

    const cg_vec2_t * const p4 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width - margin.right, dst.y));
    const cg_vec2_t * const p5 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y));
    const cg_vec2_t * const p6 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width - margin.right, dst.y + margin.top));
    const cg_vec2_t * const p7 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y + margin.top));

    const cg_vec2_t * const p8 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width - margin.right, dst.y + dst.height - margin.top));
    const cg_vec2_t * const p9 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y + dst.height - margin.top));
    const cg_vec2_t * const p10 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width - margin.right, dst.y + dst.height));
    const cg_vec2_t * const p11 = cg_affine_apply(xform, cg_vec2(dst.x + dst.width, dst.y + dst.height));

    const cg_vec2_t * const p12 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y + dst.height - margin.top));
    const cg_vec2_t * const p13 = cg_affine_apply(xform, cg_vec2(dst.x + margin.left, dst.y + dst.height - margin.top));
    const cg_vec2_t * const p14 = cg_affine_apply(xform, cg_vec2(dst.x, dst.y + dst.height));
    const cg_vec2_t * const p15 = cg_affine_apply(xform, cg_vec2(dst.x + margin.left, dst.y + dst.height));

    const float a = ctx->cur_state->fill_style.a * ctx->cur_state->global_alpha;

    cg_gl_vertex_t * const verts = cg_gl_state_map_vertex_range(ctx->gl, 28);

    //         dx,dy                 dx+dw,dy
    //      p0 +---+-p3-------------p4-+---+ p5
    //         |   |     margin top    |   |
    //      p1 +---+-p2-------------p6-+---+ p7
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
    //     p12 +---+-p13------------p8-+---+ p9
    //         |   |   margin bottom   |   |
    //     p14 +---+-p15-----------p10-+---+ p11
    //        dx,dy+dh              dx+dw,dy+dh

    const float u0 = margin.left * sw;
    const float u1 = 1 - (margin.right * sw);
    const float v0 = margin.top * sh;
    const float v1 = 1 - (margin.bottom * sh);

    int i = 0;

    // top margin
    cg_set_vert(verts, i++, p1, cg_color(0, v0, 0, a));
    cg_set_vert(verts, i++, p0, cg_color(0, 0, 0, a));
    cg_set_vert(verts, i++, p2, cg_color(u0, v0, 0, a));
    cg_set_vert(verts, i++, p3, cg_color(u0, 0, 0, a));
    cg_set_vert(verts, i++, p6, cg_color(u1, v0, 0, a));
    cg_set_vert(verts, i++, p4, cg_color(u1, 0, 0, a));
    cg_set_vert(verts, i++, p5, cg_color(1, 0, 0, a));
    cg_set_vert(verts, i++, p5, cg_color(1, 0, 0, a)); // degen- restart tristrip
    cg_set_vert(verts, i++, p7, cg_color(1, v0, 0, a));
    cg_set_vert(verts, i++, p6, cg_color(u1, v0, 0, a));

    // right margin
    cg_set_vert(verts, i++, p9, cg_color(1, v1, 0, a));
    cg_set_vert(verts, i++, p8, cg_color(u1, v1, 0, a));
    cg_set_vert(verts, i++, p11, cg_color(1, 1, 0, a));
    cg_set_vert(verts, i++, p10, cg_color(u1, 1, 0, a));
    cg_set_vert(verts, i++, p10, cg_color(u1, 1, 0, a)); // degen- restart tristrip

    // bottom margin
    cg_set_vert(verts, i++, p8, cg_color(u1, v1, 0, a));
    cg_set_vert(verts, i++, p15, cg_color(u0, 1, 0, a));
    cg_set_vert(verts, i++, p13, cg_color(u0, v1, 0, a));
    cg_set_vert(verts, i++, p14, cg_color(0, 1, 0, a));
    cg_set_vert(verts, i++, p12, cg_color(0, v1, 0, a));
    cg_set_vert(verts, i++, p12, cg_color(0, v1, 0, a)); // degen- restart tristrip

    // left margin
    cg_set_vert(verts, i++, p13, cg_color(u0, v1, 0, a));
    cg_set_vert(verts, i++, p1, cg_color(0, v0, 0, a));
    cg_set_vert(verts, i++, p2, cg_color(u0, v0, 0, a));
    cg_set_vert(verts, i++, p2, cg_color(u0, v0, 0, a)); // degen- restart tristrip

    // center
    cg_set_vert(verts, i++, p6, cg_color(u1, v0, 0, a));
    cg_set_vert(verts, i++, p13, cg_color(u0, v1, 0, a));
    cg_set_vert(verts, i++, p8, cg_color(u1, v1, 0, a));

    ASSERT(i == 28);

    cg_gl_state_finish_vertex_range(ctx->gl);

    cg_select_blend_and_shader(ctx->gl, ctx->cur_state, &ctx->cur_state->fill_style, drawable_image ? &drawable_image->cg_texture : NULL, cg_rgb_fill_alpha_red_disabled);

    cg_gl_state_draw_tri_strip(ctx->gl, i, 0);

    CG_TRACE_POP();
}

void cg_context_draw_image(const cg_image_t * const image, const cg_vec2_t pos) {
    CG_TRACE_PUSH_FN();

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    const float iw = drawable_image ? (float)drawable_image->cg_texture.texture->width : 1.f;
    const float ih = drawable_image ? (float)drawable_image->cg_texture.texture->height : 1.f;
    cg_context_draw_image_rect(image, (cg_rect_t){.x = 0, .y = 0, .width = iw, .height = ih}, (cg_rect_t){.x = pos.x, .y = pos.y, .width = iw, .height = ih});

    CG_TRACE_POP();
}

void cg_context_draw_image_scale(const cg_image_t * const image, const cg_rect_t rect) {
    CG_TRACE_PUSH_FN();

    const cg_image_t * const drawable_image = (image->status == cg_image_async_load_complete) ? image : NULL;
    const float iw = drawable_image ? (float)drawable_image->cg_texture.texture->width : 1.f;
    const float ih = drawable_image ? (float)drawable_image->cg_texture.texture->height : 1.f;
    cg_context_draw_image_rect(image, (cg_rect_t){.x = 0, .y = 0, .width = iw, .height = ih}, rect);

    CG_TRACE_POP();
}

// handles punch-through OR video texture
void cg_context_blit_video_frame(const cg_rect_t rect) {
    CG_TRACE_PUSH_FN();

    cg_context_t * const ctx = cg_statics.ctx;

    const cg_affine_t * const xform = &ctx->cur_state->transform;

#ifdef _NVE
    const float fill_alpha = ctx->cur_state->fill_style.a * ctx->cur_state->global_alpha;

    nve_texture_api_rect_t input_rect = {
        rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};

    nve_texture_api_rect_t output_rect;

    rhi_texture_t * const * const subtitle_frame = nve_get_current_subtitle_frame(input_rect, &output_rect);
    const nve_video_frame_t video_frame = nve_get_current_video_frame();

    // the punch-through step is redundant if the subtitles are full-screen and we are doing punch-through video.
    const bool skip_punchthrough = (subtitle_frame && !(video_frame.chroma && video_frame.luma)) ? (
                                       (output_rect.x1 == input_rect.x1) && (output_rect.y1 == input_rect.y1) && (output_rect.x2 == input_rect.x2) && (output_rect.y2 == input_rect.y2))
                                                                                                 : false;

    ctx->using_video_texture = (video_frame.chroma && video_frame.luma);
    const float video_alpha = (video_frame.chroma && video_frame.luma) ? fill_alpha : 0.0f;
#else
    const float video_alpha = 0.0f;
#endif

#ifdef _NVE
    if (!skip_punchthrough)
#endif
    {
        const cg_vec2_t * const p0 = cg_affine_apply(xform, cg_vec2(rect.x, rect.y));
        const cg_vec2_t * const p1 = cg_affine_apply(xform, cg_vec2(rect.x + rect.width, rect.y));
        const cg_vec2_t * const p2 = cg_affine_apply(xform, cg_vec2(rect.x + rect.width, rect.y + rect.height));
        const cg_vec2_t * const p3 = cg_affine_apply(xform, cg_vec2(rect.x, rect.y + rect.height));

        cg_gl_vertex_t * verts = cg_gl_state_map_vertex_range(ctx->gl, 4);

        cg_set_vert(verts, 0, p0, cg_color(0, 0, 0, video_alpha));
        cg_set_vert(verts, 1, p1, cg_color(1, 0, 0, video_alpha));
        cg_set_vert(verts, 2, p2, cg_color(1, 1, 0, video_alpha));
        cg_set_vert(verts, 3, p3, cg_color(0, 1, 0, video_alpha));

        cg_gl_state_finish_vertex_range(ctx->gl);

#ifdef _NVE
        if (video_frame.chroma && video_frame.luma) {
            if (video_frame.hdr10) {
                cg_gl_state_bind_video_shader_hdr(ctx->gl, &ctx->cur_state->fill_style, video_frame.chroma, video_frame.luma, (cg_ivec2_t){video_frame.luma_tex_width, video_frame.luma_tex_height}, (cg_ivec2_t){video_frame.chroma_tex_width, video_frame.chroma_tex_height}, (cg_ivec2_t){video_frame.framesize_width, video_frame.framesize_height});
            } else {
                cg_gl_state_bind_video_shader(ctx->gl, &ctx->cur_state->fill_style, video_frame.chroma, video_frame.luma, (cg_ivec2_t){video_frame.luma_tex_width, video_frame.luma_tex_height}, (cg_ivec2_t){video_frame.chroma_tex_width, video_frame.chroma_tex_height}, (cg_ivec2_t){video_frame.framesize_width, video_frame.framesize_height});
            }

        } else
#endif
        {
            cg_color_t black = {0};
            cg_gl_state_bind_color_shader(ctx->gl, &black, NULL);
            cg_gl_state_set_mode_blit(ctx->gl);
        }

        cg_gl_state_draw_tri_fan(ctx->gl, 4, 0);

#ifdef _NVE
        if (video_frame.chroma && video_frame.luma) {
            nve_done_video_frame(video_frame);
        }
#endif
    }

// blit subtitles?
#ifdef _NVE
    if (subtitle_frame) {
        const float a = fill_alpha;

        cg_gl_vertex_t * verts = cg_gl_state_map_vertex_range(ctx->gl, 4);

        const cg_vec2_t * const p0 = cg_affine_apply(xform, cg_vec2(output_rect.x1, output_rect.y1));
        const cg_vec2_t * const p1 = cg_affine_apply(xform, cg_vec2(output_rect.x2, output_rect.y1));
        const cg_vec2_t * const p2 = cg_affine_apply(xform, cg_vec2(output_rect.x2, output_rect.y2));
        const cg_vec2_t * const p3 = cg_affine_apply(xform, cg_vec2(output_rect.x1, output_rect.y2));

        cg_set_vert(verts, 0, p0, cg_color(0, 0, 0, a));
        cg_set_vert(verts, 1, p1, cg_color(1, 0, 0, a));
        cg_set_vert(verts, 2, p2, cg_color(1, 1, 0, a));
        cg_set_vert(verts, 3, p3, cg_color(0, 1, 0, a));

        cg_gl_state_finish_vertex_range(ctx->gl);

        cg_gl_state_bind_color_shader_raw(ctx->gl, &ctx->cur_state->fill_style, subtitle_frame);
        if (video_frame.chroma && video_frame.luma) {
            // we have a background video texture, alpha blend captions.
            cg_gl_state_set_mode_blend_alpha_rgb(ctx->gl);
        } else {
            // punch-through with captions.
            cg_gl_state_set_mode_blit(ctx->gl);
        }

        cg_gl_state_draw_tri_fan(ctx->gl, 4, 0);
        nve_done_subtitle_frame(subtitle_frame);
    }
#endif

    CG_TRACE_POP();
}

/* ===========================================================================
 * PATTERN
 * ==========================================================================*/

const cg_pattern_t * cg_context_pattern(const cg_image_t * const image, const bool repeat_x, const bool repeat_y) {
    CG_TRACE_PUSH_FN();

    static cg_pattern_t pattern;

    pattern = *image;
    pattern.cg_texture.sampler_state.u_wrap_mode = repeat_x ? rhi_wrap_mode_wrap : rhi_wrap_mode_clamp_to_edge;
    pattern.cg_texture.sampler_state.v_wrap_mode = repeat_y ? rhi_wrap_mode_wrap : rhi_wrap_mode_clamp_to_edge;

    CG_TRACE_POP();
    return &pattern;
}

/* ===========================================================================
 * CANVAS
 * ==========================================================================*/

void cg_context_set_clip_rect(const cg_rect_t rect) {
    CG_TRACE_PUSH_FN();

    ASSERT(rect.width >= 0 && rect.height >= 0);

    cg_context_t * const ctx = cg_statics.ctx;

    const cg_vec2_t * const xy_pos = cg_affine_apply(&ctx->cur_state->transform, cg_vec2(rect.x, rect.y));
    const cg_vec2_t * const xh_pos = cg_affine_apply(&ctx->cur_state->transform, cg_vec2(rect.x, rect.height + rect.y));
    const cg_vec2_t * const wy_pos = cg_affine_apply(&ctx->cur_state->transform, cg_vec2(rect.width + rect.x, rect.y));
    const cg_vec2_t * const wh_pos = cg_affine_apply(&ctx->cur_state->transform, cg_vec2(rect.width + rect.x, rect.height + rect.y));

    const float x0 = min_float(min_float(xy_pos->x, xh_pos->x), min_float(wy_pos->x, wh_pos->x));
    const float y0 = min_float(min_float(xy_pos->y, xh_pos->y), min_float(wy_pos->y, wh_pos->y));
    const float x1 = max_float(max_float(xy_pos->x, xh_pos->x), max_float(wy_pos->x, wh_pos->x));
    const float y1 = max_float(max_float(xy_pos->y, xh_pos->y), max_float(wy_pos->y, wh_pos->y));

    cg_clip_state_t * const clip_state = &ctx->cur_state->clip_state;

    clip_state->x0 = max_float(x0, clip_state->x0);
    clip_state->y0 = max_float(y0, clip_state->y0);
    clip_state->x1 = max_float(min_float(x1, clip_state->x1), clip_state->x0); // make sure we don't wind up with a negative width or height.
    clip_state->y1 = max_float(min_float(y1, clip_state->y1), clip_state->y0);

    cg_gl_set_scissor_rect(ctx->gl, (int)(clip_state->x0 * ctx->view_scale_x), (int)(clip_state->y0 * ctx->view_scale_y), (int)(clip_state->x1 * ctx->view_scale_x), (int)(clip_state->y1 * ctx->view_scale_y));

    CG_TRACE_POP();
}

void cg_context_set_clip_state(const cg_clip_state_e clip_state) {
    ASSERT((clip_state == cg_clip_state_enabled) || (clip_state == cg_clip_state_disabled));
    CG_TRACE_PUSH_FN();
    cg_statics.ctx->cur_state->clip_state.clip_state = clip_state;
    cg_gl_enable_scissor(cg_statics.ctx->gl, clip_state == cg_clip_state_enabled);
    CG_TRACE_POP();
}
