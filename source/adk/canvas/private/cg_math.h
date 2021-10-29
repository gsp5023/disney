/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CG_SMALL_NUM 1e-12f
#define CG_BIG_NUM 1e+12f
#define CG_TAU 6.28318530717958647692528676655900576f
#define CG_PI 3.14159265358979323846264338327950288f

#define CG_LERP(a, b, k) ((1.0f - (k)) * (a) + (k) * (b))
#define CG_ABS(x) ((x) > 0 ? (x) : -(x))
#define CG_SIGN(x) ((x) >= 0 ? 1 : -1)
#define CG_POW2(x) ((x) * (x))
#define CG_POW3(x) ((x) * (x) * (x))

/* ===========================================================================
 * COLORS
 * ==========================================================================*/

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
typedef struct cg_color_t {
    float r, g, b, a;
} cg_color_t;

typedef union cg_color_packed_t {
    struct {
        uint8_t r, g, b, a;
    };
    uint32_t rgba;
    uint8_t data[4];
} cg_color_packed_t;

/* ===========================================================================
 * VEC
 * ==========================================================================*/

FFI_EXPORT
FFI_TYPE_MODULE(canvas)
typedef struct cg_vec2_t {
    float x, y;
} cg_vec2_t;

typedef struct cg_ivec2_t {
    int x, y;
} cg_ivec2_t;

/* ===========================================================================
 * AFFINE
 * ==========================================================================*/

typedef struct cg_affine_t {
    float a, b, c, d;
    float tx, ty;
} cg_affine_t;

/* ===========================================================================
 * MATH std math approximations
 * ==========================================================================*/

float cg_to_radians(const float degrees);
float cg_to_degrees(const float radians);
float cg_trunc(const float x);
float cg_floor(const float x);
float cg_ceil(const float x);
float cg_round(const float x);

int32_t cg_as_int(const float x);
float cg_as_float(const int32_t x);
float cg_copysign(const float x, const float y);

float cg_rsqrt(const float x);
float cg_sqrt(const float x);
float cg_inv(const float x);

float cg_sin_unwrapped(const float x);
float cg_sin(const float x);
float cg_cos(const float x);
float cg_atan2(const float y, const float x);

float cg_exp2(float x);
float cg_exp(float x);
float cg_log2(const float x);
float cg_log(const float x);
float cg_pow(const float x, const float y);

/* ===========================================================================
 * COLORS
 * ==========================================================================*/

const cg_color_t * cg_color(const float r, const float g, const float b, const float a);
const cg_color_packed_t * cg_color_packed(const float r, const float g, const float b, const float a);

/* ===========================================================================
 * VEC
 * ==========================================================================*/

const cg_vec2_t * cg_vec2(const float x, const float y);
const cg_vec2_t * cg_vec2_neg(const cg_vec2_t * const s);
const cg_vec2_t * cg_vec2_add(const cg_vec2_t * const s, const cg_vec2_t * const t);
const cg_vec2_t * cg_vec2_sub(const cg_vec2_t * const s, const cg_vec2_t * const t);
const cg_vec2_t * cg_vec2_mul(const cg_vec2_t * const s, const cg_vec2_t * const t);
const cg_vec2_t * cg_vec2_scale(const cg_vec2_t * const s, const float k);
float cg_vec2_dot(const cg_vec2_t * const s, const cg_vec2_t * const t);
float cg_vec2_len(const cg_vec2_t * const s);
const cg_vec2_t * cg_vec2_lerp(const cg_vec2_t * const s, const cg_vec2_t * const t, const float k);
const cg_vec2_t * cg_vec2_normalize(const cg_vec2_t * const s);
float cg_vec2_det(const cg_vec2_t * const s, const cg_vec2_t * const t);
const cg_vec2_t * cg_vec2_rot90(const cg_vec2_t * const s);
const cg_vec2_t * cg_vec2_avg(const cg_vec2_t * const s, const cg_vec2_t * const t);
bool cg_vec2_equal(const cg_vec2_t * const s, const cg_vec2_t * const t);

/* ===========================================================================
 * AFFINE
 * ==========================================================================*/

void cg_affine_identity(cg_affine_t * const s);
void cg_affine_translate(cg_affine_t * const s, const float x, const float y);
void cg_affine_rotate(cg_affine_t * const s, const float theta);
void cg_affine_scale(cg_affine_t * const s, const float x, const float y);
void cg_affine_concat(cg_affine_t * const s, const cg_affine_t * const t);
void cg_affine_invert(cg_affine_t * const s);
float cg_affine_get_scale(const cg_affine_t * const s);
const cg_vec2_t * cg_affine_apply(const cg_affine_t * const s, const cg_vec2_t * const p);
const cg_vec2_t * cg_affine_inv_apply(const cg_affine_t * const s, const cg_vec2_t * const p);

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
