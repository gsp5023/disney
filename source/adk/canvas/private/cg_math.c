/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "cg_math.h"

/* ===========================================================================
 * MATH std math approximations
 * ==========================================================================*/

typedef union cg_real_t {
    float f;
    int32_t i;
} cg_real_t;

float cg_to_radians(const float degrees) {
    return degrees * CG_TAU / 360.0f;
}

float cg_to_degrees(const float radians) {
    return radians * 360.0f / CG_TAU;
}

float cg_trunc(const float x) {
    return (float)(int32_t)x;
}

float cg_floor(const float x) {
    return cg_trunc(x >= 0.0f ? x : x - 1.0f);
}

float cg_ceil(const float x) {
    return cg_trunc(x) + (cg_trunc(x) < x);
}

float cg_round(const float x) {
    return x >= 0.0f ? cg_floor(x + 0.5f) : cg_ceil(x - 0.5f);
}

typedef union cg_int_float_t {
    float f;
    int32_t i32;
} cg_int_float_t;

int32_t cg_as_int(const float x) {
    cg_int_float_t ret = {.f = x};
    return ret.i32;
}

float cg_as_float(const int32_t x) {
    cg_int_float_t ret = {.i32 = x};
    return ret.f;
}

float cg_copysign(const float x, const float y) {
    const int32_t ix = cg_as_int(x) & 0x7FFFFFFF;
    const int32_t iy = cg_as_int(y) & 0x80000000;
    return cg_as_float(ix | iy);
}

float cg_rsqrt(const float x) {
    cg_real_t y = {x};
    y.i = 0x5F375A86 - (y.i >> 1);

    // newton-raphson
    const float k = 3.0f / 2.0f, h = x * 0.5f;
    y.f *= k - (h * y.f * y.f); // step 1
    y.f *= k - (h * y.f * y.f); // step 2
    //  y.f *= k - (h * y.f * y.f); // step 3

    return y.f;
}

float cg_sqrt(const float x) {
    return x * cg_rsqrt(x);
}

float cg_inv(const float x) {
    if (x == 0.0f)
        return CG_BIG_NUM;

    const float sx = (float)CG_SIGN(x), ax = sx * x;
    cg_real_t v = {ax};
    v.i = 0x7EF127EA - v.i;
    const float y = ax * v.f;

    //  // 1st order newton-raphson horner form
    //  return sx * v.f * (2.0f - y);

    // 2nd order
    return sx * v.f * (4.0f + y * (y * (4.0f - y) - 6.0f));

    //  // 3rd order
    //  return sx * v.f * (8.0f +
    //                     y*(-28.0f +
    //                        y*(56.0f +
    //                           y*(-70.0f +
    //                              y*(56.0f +
    //                                 y*(-28.0f +
    //                                    y*(8.0f - y)))))));
}

float cg_sin_unwrapped(const float x) {
    cg_real_t p = {0.20363937680730309f},
              r = {0.015124940802184233f},
              s = {-0.0032225901625579573f},
              q = {x};
    const int32_t sign = q.i & 0x80000000;
    q.i &= 0x7FFFFFFF;

    const float y = (4.0f / CG_PI) * x - (4.0f / (CG_PI * CG_PI)) * x * q.f;
    const float y2 = y * y;

    p.i |= sign;
    r.i |= sign;
    s.i ^= sign;

    const float y0 = 0.78444488374548933f * y;
    return y0 + y2 * (p.f + y2 * (r.f + y2 * s.f));
}

float cg_sin(const float x) {
    //  return sinf(x);
    const int32_t k = (int32_t)(x * 1.0f / CG_TAU);
    const float half = (x < 0.0f) ? -0.5f : 0.5f;
    return cg_sin_unwrapped((half + k) * CG_TAU - x);
}

float cg_cos(const float x) {
    //  return cosf(x);
    return cg_sin(x + CG_PI / 2.0f);
}

float cg_atan2(const float y, const float x) {
    float r, theta;
    const float abs_y = CG_ABS(y) + 1e-10f;
    if (x < 0.0f) {
        r = (x + abs_y) / (abs_y - x);
        theta = 3.0f * CG_PI / 4.0f;
    } else {
        r = (x - abs_y) / (x + abs_y);
        theta = CG_PI / 4.0f;
    }
    theta += (0.1963f * r * r - 0.9817f) * r;
    if (y < 0.0f)
        return -theta;
    return theta;
}

float cg_exp2(float x) {
    float y = x - cg_floor(x);
    y = (y - y * y) * 0.33971f;
    const float z = max_float((x + 127.0f - y) * 8388608.0f, 0.0f);
    cg_real_t a;
    a.i = (int32_t)z;
    return a.f;
}

float cg_exp(float x) {
    return cg_exp2(x * 1.44269504088896340735992f /* 1/log(2) */);
}

float cg_log2(const float x) {
    cg_real_t a;
    a.f = x;
    const float b = a.i * 0.00000011920928955078125f - 127.0f;
    const float c = b - cg_floor(b);
    return (b + (c - c * c) * 0.346607f);
}

float cg_log(const float x) {
    return cg_log2(x) * 0.69314718055994530941723f; /* 1/log2(e) */
}

float cg_pow(const float x, const float y) {
    if (x == 0.0f)
        return 1.0f;
    float r = cg_exp(cg_log(CG_ABS(x)) * y);
    float mody = CG_ABS(y) - 2.0f * cg_floor(0.5f * CG_ABS(y));
    float sign = cg_copysign(mody, x) + 0.5f;

    return cg_copysign(r, sign);
}

/* ===========================================================================
  * COLORS
  * ==========================================================================*/

const cg_color_t * cg_color(const float r, const float g, const float b, const float a) {
    static cg_color_t c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return &c;
}

const cg_color_packed_t * cg_color_packed(const float r, const float g, const float b, const float a) {
    static cg_color_packed_t c;
    c.r = (uint8_t)(clamp_float(r, 0.0f, 1.0f) * 255.0f);
    c.g = (uint8_t)(clamp_float(g, 0.0f, 1.0f) * 255.0f);
    c.b = (uint8_t)(clamp_float(b, 0.0f, 1.0f) * 255.0f);
    c.a = (uint8_t)(clamp_float(a, 0.0f, 1.0f) * 255.0f);
    return &c;
}

/* ===========================================================================
 * VEC
 * ==========================================================================*/

const cg_vec2_t * cg_vec2(const float x, const float y) {
    // simple round-robin allocator, be mindful of recursion when using!
    static size_t ri = 0;
    enum { cg_vec2_fifo_size = 128 }; // must be power of 2!
    static cg_vec2_t r[cg_vec2_fifo_size];
    ri = (ri + 1) & (cg_vec2_fifo_size - 1);
    r[ri].x = x;
    r[ri].y = y;
    return &r[ri];
}

const cg_vec2_t * cg_vec2_neg(const cg_vec2_t * const s) {
    return cg_vec2(-s->x, -s->y);
}

const cg_vec2_t * cg_vec2_add(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    return cg_vec2(s->x + t->x, s->y + t->y);
}

const cg_vec2_t * cg_vec2_sub(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    return cg_vec2(s->x - t->x, s->y - t->y);
}

const cg_vec2_t * cg_vec2_mul(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    return cg_vec2(s->x * t->x, s->y * t->y);
}

const cg_vec2_t * cg_vec2_scale(const cg_vec2_t * const s, const float k) {
    return cg_vec2(s->x * k, s->y * k);
}

float cg_vec2_dot(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    return s->x * t->x + s->y * t->y;
}

float cg_vec2_len(const cg_vec2_t * const s) {
    return cg_sqrt(s->x * s->x + s->y * s->y);
}

const cg_vec2_t * cg_vec2_lerp(const cg_vec2_t * const s, const cg_vec2_t * const t, const float k) {
    return cg_vec2(CG_LERP(s->x, t->x, k), CG_LERP(s->y, t->y, k));
}

const cg_vec2_t * cg_vec2_normalize(const cg_vec2_t * const s) {
    const float k = cg_rsqrt(cg_vec2_dot(s, s));
    //  const float k = 1.0f / sqrtf(cg_vec2_dot(s, s));
    return cg_vec2(s->x * k, s->y * k);
}

float cg_vec2_det(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    return s->x * t->y - t->x * s->y;
}

const cg_vec2_t * cg_vec2_rot90(const cg_vec2_t * const s) {
    return cg_vec2(-s->y, s->x);
}

const cg_vec2_t * cg_vec2_avg(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    return cg_vec2((s->x + t->x) * 0.5f, (s->y + t->y) * 0.5f);
}

bool cg_vec2_equal(const cg_vec2_t * const s, const cg_vec2_t * const t) {
    const cg_vec2_t * const d = cg_vec2_sub(s, t);
    return cg_vec2_dot(d, d) < 0.1f; // (CG_SMALL_NUM*CG_SMALL_NUM);
}

/* ===========================================================================
 * AFFINE
 * ==========================================================================*/

void cg_affine_identity(cg_affine_t * const s) {
    s->a = s->d = 1.0f;
    s->b = s->c = 0.0f;
    s->tx = s->ty = 0.0f;
}

void cg_affine_translate(cg_affine_t * const s, const float x, const float y) {
    s->tx += s->a * x + s->c * y;
    s->ty += s->b * x + s->d * y;
}

void cg_affine_rotate(cg_affine_t * const s, const float theta) {
    const float sn = cg_sin(theta);
    const float cs = cg_cos(theta);

    const float a = s->a * cs + s->c * sn;
    const float b = s->b * cs + s->d * sn;
    const float c = s->c * cs - s->a * sn;
    const float d = s->d * cs - s->b * sn;

    s->a = a;
    s->b = b;
    s->c = c;
    s->d = d;
}

void cg_affine_scale(cg_affine_t * const s, const float x, const float y) {
    s->a *= x;
    s->b *= x;
    s->c *= y;
    s->d *= y;
}

void cg_affine_concat(cg_affine_t * const s, const cg_affine_t * const t) {
    const float a = s->a * t->a + s->b * t->c;
    const float b = s->a * t->b + s->b * t->d;
    const float c = s->c * t->a + s->d * t->c;
    const float d = s->c * t->b + s->d * t->d;
    const float tx = s->tx * t->a + s->ty * t->c + t->tx;
    const float ty = s->tx * t->b + s->ty * t->d + t->ty;

    s->a = a;
    s->b = b;
    s->c = c;
    s->d = d;
    s->tx = tx;
    s->ty = ty;
}

void cg_affine_invert(cg_affine_t * const s) {
    const float det = 1.0f / (s->a * s->d - s->b * s->c);

    const float a = det * s->d;
    const float b = -det * s->b;
    const float c = -det * s->c;
    const float d = det * s->a;
    const float tx = det * (s->c * s->ty - s->d * s->tx);
    const float ty = det * (s->b * s->tx - s->a * s->ty);

    s->a = a;
    s->b = b;
    s->c = c;
    s->d = d;
    s->tx = tx;
    s->ty = ty;
}

float cg_affine_get_scale(const cg_affine_t * const s) {
    const float sx = cg_sqrt((s->a * s->a) + (s->c * s->c));
    const float sy = cg_sqrt((s->b * s->b) + (s->d * s->d));
    return (sx + sy) * 0.5f;

    //  return cg_sqrt(s->a * s->a + s->c * s->c);
}

const cg_vec2_t * cg_affine_apply(const cg_affine_t * const s, const cg_vec2_t * const p) {
    return cg_vec2(s->a * p->x + s->c * p->y + s->tx, s->b * p->x + s->d * p->y + s->ty);
}

const cg_vec2_t * cg_affine_inv_apply(const cg_affine_t * const s, const cg_vec2_t * const p) {
    cg_affine_t inv_s = *s;
    cg_affine_invert(&inv_s);

    return cg_vec2(inv_s.a * p->x + inv_s.c * p->y + inv_s.tx, inv_s.b * p->x + inv_s.d * p->y + inv_s.ty);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */