/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
rhi_gl_device.h

OpenGL rendering device
*/

#include "glapi/glapi.h"
#include "rhi_gl_shared.h"
#include "source/adk/steamboat/private/private_apis.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
===============================================================================
OpenGL function table loaded via platform specific functions.

_ALL_ opengl api calls beyond the legacy v1.1 APIs must go through this table. 
The table is defined and created in the glapi.h and is specific to the platform 
feature set being targeted at compile time: (desktop, ES, etc)
===============================================================================
*/

extern glf_t glf;

#ifdef GL_CORE
#define GL_CALL(_x) glf._x
#else // GL_ES
#define GL_CALL(_x) _x
#endif

/*
===============================================================================
Routines to convert RHI to GL enums
===============================================================================
*/

static inline GLenum get_gl_compare_func(const rhi_compare_func_e f) {
    switch (f) {
        case rhi_compare_never:
            return GL_NEVER;
        case rhi_compare_notequal:
            return GL_NOTEQUAL;
        case rhi_compare_equal:
            return GL_EQUAL;
        case rhi_compare_less:
            return GL_LESS;
        case rhi_compare_lequal:
            return GL_LEQUAL;
        case rhi_compare_gequal:
            return GL_GEQUAL;
        case rhi_compare_greater:
            return GL_GREATER;
        default:
            return GL_ALWAYS;
    }
}

static inline GLenum get_gl_blend_factor(const rhi_blend_factor_e f) {
    switch (f) {
        case rhi_blend_zero:
            return GL_ZERO;
        case rhi_blend_one:
            return GL_ONE;
        case rhi_blend_src_color:
            return GL_SRC_COLOR;
        case rhi_blend_inv_src_color:
            return GL_ONE_MINUS_SRC_COLOR;
        case rhi_blend_dst_color:
            return GL_DST_COLOR;
        case rhi_blend_inv_dst_color:
            return GL_ONE_MINUS_DST_COLOR;
        case rhi_blend_src_alpha:
            return GL_SRC_ALPHA;
        case rhi_blend_inv_src_alpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case rhi_blend_dst_alpha:
            return GL_DST_ALPHA;
        case rhi_blend_constant_color:
            return GL_CONSTANT_COLOR;
        case rhi_blend_constant_alpha:
            return GL_CONSTANT_ALPHA;
        default:
            return GL_ONE_MINUS_DST_ALPHA;
    }
}

static inline GLenum get_gl_stencil_op(const rhi_stencil_op_e op) {
    switch (op) {
        case rhi_stencil_op_keep:
            return GL_KEEP;
        case rhi_stencil_op_zero:
            return GL_ZERO;
        case rhi_stencil_op_replace:
            return GL_REPLACE;
        case rhi_stencil_op_incr:
            return GL_INCR;
        case rhi_stencil_op_incr_wrap:
            return GL_INCR_WRAP;
        case rhi_stencil_op_decr:
            return GL_DECR;
        case rhi_stencil_op_decr_wrap:
            return GL_DECR_WRAP;
        default:
            return GL_INVERT;
    }
}

static inline GLenum get_gl_blend_op(const rhi_blend_op_e op) {
    switch (op) {
        case rhi_blend_op_add:
            return GL_FUNC_ADD;
        case rhi_blend_op_sub:
            return GL_FUNC_SUBTRACT;
        default:
            return GL_FUNC_REVERSE_SUBTRACT;
    }
}

static inline GLenum get_gl_min_filter(const rhi_min_filter_mode_e filter) {
    switch (filter) {
        case rhi_min_filter_linear:
            return GL_LINEAR;
        case rhi_min_filter_nearest_mipmap_nearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case rhi_min_filter_linear_mipmap_nearest:
            return GL_LINEAR_MIPMAP_NEAREST;
        case rhi_min_filter_nearest_mipmap_linear:
            return GL_NEAREST_MIPMAP_LINEAR;
        case rhi_min_filter_linear_mipmap_linear:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            TRAP("invalid min filter");
            FALLTHROUGH;
        case rhi_max_filter_nearest:
            return GL_NEAREST;
    }
}

static inline GLenum get_gl_max_filter(const rhi_max_filter_mode_e filter) {
    switch (filter) {
        case rhi_max_filter_linear:
            return GL_LINEAR;
        default:
            TRAP("invalid max filter");
            FALLTHROUGH;
        case rhi_max_filter_nearest:
            return GL_NEAREST;
    }
}

static inline GLenum get_gl_wrap_mode(const rhi_wrap_mode_e wrap) {
    switch (wrap) {
        case rhi_wrap_mode_clamp_to_edge:
            return GL_CLAMP_TO_EDGE;
        case rhi_wrap_mode_mirror:
            return GL_MIRRORED_REPEAT;
        default:
            TRAP("invalid wrap mode");
            FALLTHROUGH;
        case rhi_wrap_mode_wrap:
            return GL_REPEAT;
    }
}

static inline GLenum gl_get_draw_mode(rhi_draw_mode_e mode) {
    switch (mode) {
        case rhi_triangle_strip:
            return GL_TRIANGLE_STRIP;
        case rhi_triangle_fan:
            return GL_TRIANGLE_FAN;
        case rhi_lines:
            return GL_LINES;
        case rhi_line_strip:
            return GL_LINE_STRIP;
        default:
            TRAP("invalid draw mode");
            FALLTHROUGH;
        case rhi_triangles:
            return GL_TRIANGLES;
    }
}

typedef struct gl_pixel_format_t {
    GLenum internal;
    GLenum format;
    GLenum type;
} gl_pixel_format_t;

#ifdef GL_CORE
static const gl_pixel_format_t pixel_formats[] = {
    [rhi_pixel_format_r8_unorm] = {.internal = GL_R8, .format = GL_RED, .type = GL_UNSIGNED_BYTE}, // red -> grayscale (with internal swizzling) GL_RED, GL_RED, GL_RED, GL_ONE
    [rhi_pixel_format_ra8_unorm] = {.internal = GL_RG8, .format = GL_RG, .type = GL_UNSIGNED_BYTE}, // rg -> grayscale + alpha (with internal swizzling) GL_RED, GL_RED, GL_RED, GL_GREEN
    [rhi_pixel_format_rgb8_unorm] = {.internal = GL_RGB8, .format = GL_RGB, .type = GL_UNSIGNED_BYTE},
    [rhi_pixel_format_rgba8_unorm] = {.internal = GL_RGBA8, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE},
    [rhi_pixel_format_bc1_unorm] = {.internal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, .format = GL_RGBA, .type = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT},
    [rhi_pixel_format_bc3_unorm] = {.internal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, .format = GL_RGBA, .type = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT}};
#else // GL_ES
static const gl_pixel_format_t pixel_formats[rhi_num_pixels_formats] = {
    [rhi_pixel_format_r8_unorm] = {.internal = GL_LUMINANCE, .format = GL_LUMINANCE, .type = GL_UNSIGNED_BYTE},
    [rhi_pixel_format_ra8_unorm] = {.internal = GL_LUMINANCE_ALPHA, .format = GL_LUMINANCE_ALPHA, .type = GL_UNSIGNED_BYTE},
    [rhi_pixel_format_rgb8_unorm] = {.internal = GL_RGB, .format = GL_RGB, .type = GL_UNSIGNED_BYTE},
    [rhi_pixel_format_rgba8_unorm] = {.internal = GL_RGBA, .format = GL_RGBA, .type = GL_UNSIGNED_BYTE},
    [rhi_pixel_format_etc1] = {.internal = GL_ETC1_RGB8_OES, .format = GL_RGB, .type = GL_UNSIGNED_BYTE}};
#endif

/*
=======================================
gl_rect_t
=======================================
*/

typedef struct gl_rect_t {
    int x1, y1, x2, y2;
} gl_rect_t;

/*
=======================================
gl_context_t

Encapsulates a rendering context's current opengl pipeline state
=======================================
*/

struct gl_blend_state_t;
struct gl_depth_stencil_state_t;
struct gl_rasterizer_state_t;
struct gl_uniform_buffer_t;
struct gl_program_t;

typedef struct gl_context_t {
    rhi_device_caps_t caps;
    struct sb_window_t * window;
#ifdef GL_ES
    EGLSurface egl_surface;
    EGLContext egl_context;
#endif
    rhi_counters_t counters;
#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
    int textures[rhi_program_input_max_textures];
#endif
    int texture_ids[rhi_program_input_max_textures];
    struct gl_uniform_buffer_t * ubuffers[rhi_program_input_max_uniforms];
    rhi_rasterizer_state_desc_t rs_desc;
    rhi_depth_stencil_state_desc_t dss_desc;
    rhi_blend_state_desc_t bs_desc;
    const struct gl_rasterizer_state_t * active_rs;
    const struct gl_depth_stencil_state_t * active_dss;
    const struct gl_blend_state_t * active_bs;
    struct gl_program_t * program;
    int program_id;
    int uniform_updates;
    int active_rs_id;
    int active_dss_id;
    int active_bs_id;
    GLuint stencil_ref;
    GLuint renderbuffer;
    int renderbuffer_id;
#ifdef GL_CORE
    GLuint readbuffer;
    int readbuffer_id;
    GLuint drawbuffer;
    int drawbuffer_id;
#else // GL_ES
    GLuint framebuffer;
    int framebuffer_id;
#endif
    gl_rect_t viewport;
    gl_rect_t scissor;
    rhi_cull_face_e cull_mode;
    rhi_swap_interval_t swap_interval;
#ifdef GL_VAOS
    int active_vao;
#endif
#if !(defined(GL_DSA) && defined(GL_VAOS))
    int array_buffer;
    int element_buffer;
    int uniform_buffer;
#endif
#ifndef GL_DSA
    int bind_texture;
    GLenum active_texture;
#endif
} gl_context_t;

struct gl_render_target_t;

#ifdef GL_VAOS
struct gl_vao_t;
#endif

typedef struct gl_device_and_context_t {
    rhi_device_t device;
    gl_context_t context;
    struct gl_render_target_t * active_render_target;
#ifdef GL_VAOS
    struct gl_vao_t * vao_used_chain;
    struct gl_vao_t * vao_pending_destroy_chain;
#endif
    int display_width;
    int display_height;
    int index;
} gl_device_and_context_t;

extern THREAD_LOCAL gl_context_t * gl_thread_context;

bool glc_create(gl_context_t * const context, rhi_error_t ** const out_error);
void glc_destroy(gl_context_t * const context, const char * const tag);
void glc_make_current(gl_context_t * const context);
void glc_swap_buffers(gl_context_t * const context, const rhi_swap_interval_t swap_interval);

/*
=======================================
gl_texture_t
=======================================
*/

typedef struct gl_texture_t {
    gl_resource_t resource;
    GLuint texture;
#ifdef GL_CORE
    GLuint sampler;
#endif
    GLenum target;
    int width;
    int height;
    int depth;
    rhi_pixel_format_e pixel_format;
#ifndef NDEBUG
    rhi_usage_e usage;
#endif
#ifdef GL_ES
    bool mipmaps;
#endif
    void * user; // for starboard usage
} gl_texture_t;

gl_texture_t * glc_create_texture(gl_context_t * const context, rhi_resource_t * const outer, GLenum target, const char * const tag);

/*
===============================================================================
Helper functions to wrap primitive GL calls with error checks and to transparently
support different API targets
===============================================================================
*/

static inline void glc_gen_buffers(gl_context_t * const context, const GLsizei n, GLuint * const b) {
#ifdef GL_DSA
    glf.glCreateBuffers(n, b);
#else
    GL_CALL(glGenBuffers(n, b));
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_delete_buffers(gl_context_t * const context, GLsizei n, const GLuint * const b) {
    GL_CALL(glDeleteBuffers(n, b));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

#if !(defined(GL_DSA) && defined(GL_VAOS))
static inline void glc_bind_buffer(gl_context_t * const context, const GLenum target, const GLuint b, const int id, const bool force) {
    switch (target) {
        case GL_ARRAY_BUFFER: {
            if (force || (context->array_buffer != id)) {
                context->array_buffer = id;
                GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, b));
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
        } break;
        case GL_ELEMENT_ARRAY_BUFFER: {
            if (force || (context->element_buffer != id)) {
                context->element_buffer = id;
                GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b));
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
        } break;
#ifdef GL_CORE
        case GL_UNIFORM_BUFFER: {
            if (force || (context->uniform_buffer != id)) {
                context->uniform_buffer = id;
                glf.glBindBuffer(GL_UNIFORM_BUFFER, b);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
        } break;
#endif
        default:
            TRAP("Invalid buffer binding target.");
            break;
    }
}
#endif

static inline void glc_static_buffer_storage(gl_context_t * const context, const GLenum target, const GLuint b, const int id, const GLsizeiptr size, const void * const data) {
#ifdef GL_DSA
    glf.glNamedBufferData(b, size, data, GL_STATIC_DRAW);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#else
    glc_bind_buffer(context, target, b, id, false);
    GL_CALL(glBufferData(target, size, data, GL_STATIC_DRAW));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#endif
}

static inline void glc_buffer_sub_data(gl_context_t * const context, const GLenum target, const GLuint b, const int id, const GLintptr offset, const GLsizeiptr size, const void * const src) {
#ifdef GL_DSA
    glf.glNamedBufferSubData(b, offset, size, src);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#else
    glc_bind_buffer(context, target, b, id, false);
    GL_CALL(glBufferSubData(target, offset, size, src));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#endif
}

static inline void glc_dynamic_buffer_data(gl_context_t * const context, const GLenum target, const GLuint b, const int id, const GLsizeiptr size, const void * const src) {
#ifdef GL_DSA
    CHECK_GL_ERRORS();
    glf.glNamedBufferData(b, size, src, GL_DYNAMIC_DRAW);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#else
    glc_bind_buffer(context, target, b, id, false);
    GL_CALL(glBufferData(target, size, src, GL_DYNAMIC_DRAW));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#endif
}

static inline GLuint glc_create_program(gl_context_t * const context) {
    const GLuint p = GL_CALL(glCreateProgram());
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return p;
}

static inline void glc_delete_program(gl_context_t * const context, const GLuint p) {
    GL_CALL(glDeleteProgram(p));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline GLuint glc_create_shader(gl_context_t * const context, const GLenum type, const char * const source, const GLint len) {
    const GLuint s = GL_CALL(glCreateShader(type));
    CHECK_GL_ERRORS();
    GL_CALL(glShaderSource(s, 1, &source, &len));
    CHECK_GL_ERRORS();
    GL_CALL(glCompileShader(s));
    CHECK_GL_ERRORS();
    context->counters.num_api_calls += 3;
    return s;
}

static inline void glc_delete_shader(gl_context_t * const context, const GLuint s) {
    GL_CALL(glDeleteShader(s));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_attach_shader(gl_context_t * const context, const GLuint p, const GLuint s) {
    GL_CALL(glAttachShader(p, s));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_detach_shader(gl_context_t * const context, const GLuint p, const GLuint s) {
    GL_CALL(glDetachShader(p, s));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline GLint glc_get_compile_status(gl_context_t * const context, const GLuint s) {
    GLint r;
    GL_CALL(glGetShaderiv(s, GL_COMPILE_STATUS, &r));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

static inline GLint glc_get_link_status(gl_context_t * const context, const GLuint p) {
    GLint r;
    GL_CALL(glGetProgramiv(p, GL_LINK_STATUS, &r));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

static inline GLint glc_get_shader_log_size(gl_context_t * const context, const GLuint s) {
    GLint r;
    GL_CALL(glGetShaderiv(s, GL_INFO_LOG_LENGTH, &r));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

static inline char * glc_get_shader_log(gl_context_t * const context, const GLuint s, char * const log, const GLsizei len) {
    GL_CALL(glGetShaderInfoLog(s, len, NULL, log));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return log;
}

static inline GLint glc_get_program_log_size(gl_context_t * const context, const GLuint p) {
    GLint r;
    GL_CALL(glGetProgramiv(p, GL_INFO_LOG_LENGTH, &r));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

static inline char * glc_get_program_log(gl_context_t * const context, const GLuint p, char * const log, const GLsizei len) {
    GL_CALL(glGetProgramInfoLog(p, len, NULL, log));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return log;
}

static inline void glc_link_program(gl_context_t * const context, const GLuint p) {
    GL_CALL(glLinkProgram(p));
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

static inline GLint glc_get_uniform_location(gl_context_t * const context, const GLuint p, const char * const name) {
    const GLint r = GL_CALL(glGetUniformLocation(p, name));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

static inline GLint glc_get_attrib_location(gl_context_t * const context, GLuint p, const char * const name) {
    const GLint r = GL_CALL(glGetAttribLocation(p, name));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

#ifdef GL_VAOS
static inline void glc_gen_vertex_arrays(gl_context_t * const context, const int count, GLuint * const out_arrays) {
#ifdef GL_DSA
    glf.glCreateVertexArrays(count, out_arrays);
#else
    glf.glGenVertexArrays(count, out_arrays);
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_delete_vertex_arrays(gl_context_t * const context, const int count, GLuint * const vaos) {
    glf.glDeleteVertexArrays(count, vaos);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}
#endif

#ifdef GL_CORE
static inline void glc_gen_samplers(gl_context_t * const context, const int count, GLuint * const samplers) {
    glf.glGenSamplers(count, samplers);

    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_delete_samplers(gl_context_t * const context, const int count, GLuint * const samplers) {
    glf.glDeleteSamplers(count, samplers);

    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_sampler_parameter_i(gl_context_t * const context, const GLuint sampler, const GLenum pname, const GLuint value) {
    glf.glSamplerParameteri(sampler, pname, value);

    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_sampler_parameter_fv(gl_context_t * const context, const GLuint sampler, const GLenum pname, const GLfloat * const params) {
    glf.glSamplerParameterfv(sampler, pname, params);

    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}
#endif

static inline void glc_gen_textures(gl_context_t * const context, const GLenum target, const int count, GLuint * const out_textures) {
#ifdef GL_DSA
    glf.glCreateTextures(target, count, out_textures);
#else
    glGenTextures(count, out_textures);
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_delete_textures(gl_context_t * const context, const int count, const GLuint * const textures) {
    glDeleteTextures(count, textures);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_bind_texture(gl_context_t * const context, const GLenum texture_unit, const GLenum target, const GLuint texture, const int id) {
    ASSERT(target == GL_TEXTURE_2D);
    const int idx = texture_unit - GL_TEXTURE0;
    ASSERT((idx >= 0) && (idx < rhi_program_input_max_textures));
    if (context->texture_ids[idx] != id) {
        context->texture_ids[idx] = id;
#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
        context->textures[idx] = texture;
#endif
#ifdef GL_DSA
        glf.glBindTextureUnit(idx, texture);
#else
        if (context->active_texture != texture_unit) {
            context->active_texture = texture_unit;
            GL_CALL(glActiveTexture(texture_unit));
            CHECK_GL_ERRORS();
            ++context->counters.num_api_calls;
        }
        glBindTexture(target, texture);
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
#endif
    }
}

static inline void glc_tex_parameter_i(gl_context_t * const context, const GLenum tunit, const GLenum target, const GLuint texture, const int id, const GLenum pname, const GLint param) {
#ifdef GL_DSA
    glf.glTextureParameteri(texture, pname, param);
#else
    glc_bind_texture(context, tunit, target, texture, id);
    glTexParameteri(target, pname, param);
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

#ifdef GL_ES
static inline void glc_tex_parameter_fv(gl_context_t * const context, const GLenum tunit, const GLenum target, const GLuint texture, const int id, const GLenum pname, const GLfloat * const params) {
    glc_bind_texture(context, tunit, target, texture, id);
    glTexParameterfv(target, pname, params);
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}
#endif

static inline void glc_texture_storage_2d(gl_context_t * const context, const GLenum tunit, const GLuint texture, const int id, const GLsizei levels, const GLenum internalformat, const GLsizei w, const GLsizei h, const GLenum format, const GLenum type) {
#ifdef GL_DSA
    glf.glTextureStorage2D(texture, levels, internalformat, w, h);
#else
    glc_bind_texture(context, tunit, GL_TEXTURE_2D, texture, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, format, type, NULL);
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

static inline void glc_texture_subimage_2d(gl_context_t * const context, const GLenum tunit, const GLenum target, const GLuint texture, int id, const GLint level, const GLint xoffset, const GLint yoffset, const GLsizei w, const GLsizei h, const GLenum format, const GLenum type, const void * const pixels) {
#ifdef GL_DSA
    glf.glTextureSubImage2D(texture, level, xoffset, yoffset, w, h, format, type, pixels);
#else
    glc_bind_texture(context, tunit, target, texture, id);
    glTexSubImage2D(target, level, xoffset, yoffset, w, h, format, type, pixels);
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

static inline void glc_compressed_texture_image_2d(gl_context_t * const context, const GLenum tunit, const GLenum target, const GLuint texture, const int id, const GLint level, const GLsizei w, const GLsizei h, const GLenum format, GLint border, const GLsizei imageSize, const void * const pixels) {
#ifdef GL_DSA
    glf.glCompressedTexImage2D(target, level, format, w, h, border, imageSize, pixels);
#else
    glc_bind_texture(context, tunit, target, texture, id);
    GL_CALL(glCompressedTexImage2D(target, level, format, w, h, border, imageSize, pixels));
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

static inline void glc_compressed_texture_subimage_2d(gl_context_t * const context, const GLenum tunit, const GLenum target, const GLuint texture, const int id, const GLint level, const GLint xoffset, const GLint yoffset, const GLsizei w, const GLsizei h, const GLenum format, const GLsizei imageSize, const void * const pixels) {
#ifdef GL_DSA
    glf.glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, w, h, format, imageSize, pixels);
#else
    glc_bind_texture(context, tunit, target, texture, id);
    GL_CALL(glCompressedTexSubImage2D(target, level, xoffset, yoffset, w, h, format, imageSize, pixels));
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

#ifdef GL_CORE
static inline void glc_texture_set_swizzle_mask(gl_context_t * const context, const GLenum texture_unit, const GLenum target, const GLuint texture, const int id, const GLenum red_channel, const GLenum green_channel, const GLenum blue_channel, const GLenum alpha_channel) {
    glc_bind_texture(context, texture_unit, target, texture, id);
    GLint swizzle_mask[] = {red_channel, green_channel, blue_channel, alpha_channel};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
    ++context->counters.num_api_calls;
}
#endif

#ifdef GL_VAOS
static inline void glc_bind_vertex_array(gl_context_t * const context, const GLuint va, const int vaid) {
    if (context->active_vao != vaid) {
        context->active_vao = vaid;
        glf.glBindVertexArray(va);
        ++context->counters.num_api_calls;
    }
    CHECK_GL_ERRORS();
}
#endif

static inline void glc_gen_framebuffers(gl_context_t * const context, const int size, GLuint * const out_fbs) {
#ifdef GL_DSA
    glf.glCreateFramebuffers(size, out_fbs);
#else
    GL_CALL(glGenFramebuffers(size, out_fbs));
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_delete_framebuffers(gl_context_t * const context, const int size, const GLuint * const fbs) {
    GL_CALL(glDeleteFramebuffers(size, fbs));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_bind_framebuffer(gl_context_t * const context, const GLenum target, const GLuint fb, const int id) {
#ifdef GL_CORE
    const bool drawbuffer = (target == GL_FRAMEBUFFER) || (target == GL_DRAW_FRAMEBUFFER);
    const bool readbuffer = (target == GL_FRAMEBUFFER) || (target == GL_READ_FRAMEBUFFER);
    ASSERT(drawbuffer || readbuffer);

    if ((drawbuffer && (id != context->drawbuffer_id)) || (readbuffer && (id != context->readbuffer_id))) {
        if (drawbuffer) {
            context->drawbuffer_id = id;
            context->drawbuffer = fb;

            if (readbuffer) {
                context->readbuffer_id = id;
                context->readbuffer = fb;

                glf.glBindFramebuffer(GL_FRAMEBUFFER, fb);
            } else {
                glf.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
            }
        } else {
            context->readbuffer_id = id;
            context->readbuffer = fb;
            glf.glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
        }
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
#else // GL_ES
    ASSERT(target == GL_FRAMEBUFFER);

    if (id != context->framebuffer_id) {
        context->framebuffer_id = id;
        context->framebuffer = fb;

        glBindFramebuffer(GL_FRAMEBUFFER, fb);

        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
#endif
}

static inline GLenum glc_check_framebuffer_status(gl_context_t * const context, const GLenum target, const GLuint fb, const int id) {
#ifdef GL_DSA
    const GLenum r = glf.glCheckNamedFramebufferStatus(fb, GL_FRAMEBUFFER);
#else
    glc_bind_framebuffer(context, target, fb, id);
    const GLenum r = GL_CALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
    return r;
}

static inline void glc_gen_renderbuffers(gl_context_t * const context, const int count, GLuint * const out_rbs) {
#ifdef GL_DSA
    glf.glCreateRenderbuffers(count, out_rbs);
#else
    GL_CALL(glGenRenderbuffers(count, out_rbs));
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_renderbuffer_storage(gl_context_t * const context, const GLuint rb, const int id, const GLenum internalFormat, const GLsizei width, const GLsizei height) {
    CHECK_GL_ERRORS();
#ifdef GL_DSA
    glf.glNamedRenderbufferStorage(rb, internalFormat, width, height);
#else
    if (context->renderbuffer_id != id) {
        context->renderbuffer = rb;
        context->renderbuffer_id = id;
        GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, rb));
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
    GL_CALL(glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height));
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
}

static inline void glc_delete_renderbuffers(gl_context_t * const context, const int count, const GLuint * const rbs) {
    GL_CALL(glDeleteRenderbuffers(count, rbs));
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}
static inline void glc_framebuffer_renderbuffer(gl_context_t * const context, const GLenum target, const GLuint fb, const int id, const GLenum attachment, const GLenum renderbuffer_target, const GLuint rb) {
#ifdef GL_DSA
    glf.glNamedFramebufferRenderbuffer(fb, attachment, renderbuffer_target, rb);
#else
    glc_bind_framebuffer(context, target, fb, id);
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

static inline void glc_framebuffer_texture_2d(gl_context_t * const context, const GLenum target, const GLuint fb, const int id, const GLenum attachment, const GLuint t, const GLint level) {
#ifdef GL_DSA
    glf.glNamedFramebufferTexture(fb, attachment, t, level);
#else
    glc_bind_framebuffer(context, target, fb, id);
    GL_CALL(glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, t, level));
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}

#ifdef GL_CORE
static inline void glc_framebuffer_drawbuffers(gl_context_t * const context, const GLuint fb, const int id, const GLsizei count, const GLenum * const bufs) {
#ifdef GL_DSA
    glf.glNamedFramebufferDrawBuffers(fb, count, bufs);
#else
    glc_bind_framebuffer(context, GL_DRAW_FRAMEBUFFER, fb, id);
    glf.glDrawBuffers(count, bufs);
#endif
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
}
#endif

static inline void glc_discard_framebuffer(gl_context_t * const context, GLenum target, const GLuint fb, const int id, const GLsizei num_attachments, const GLenum * const attachments) {
#ifdef GL_DSA
    glf.glInvalidateNamedFramebufferData(fb, num_attachments, attachments);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#else
#ifdef GL_CORE
#if (GLVERSION < 430)
    if (glf.ARB_invalidate_subdata)
#endif
    {
        glc_bind_framebuffer(context, target, fb, id);
        glf.glInvalidateFramebuffer(target, num_attachments, attachments);
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
#else // GL_ES
    glc_bind_framebuffer(context, target, fb, id);
    //glf.glDiscardFramebufferEXT(target, num_attachments, attachments);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#endif
#endif
}

static inline void glc_scissor(gl_context_t * const context, const GLint x, const GLint y, const GLsizei w, const GLsizei h) {
    if ((context->scissor.x1 != x) || (context->scissor.y1 != y) || (context->scissor.x2 != w) || (context->scissor.y2 != h)) {
        context->scissor.x1 = x;
        context->scissor.y1 = y;
        context->scissor.x2 = w;
        context->scissor.y2 = h;
        glScissor(x, y, w, h);
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
}

static inline void glc_viewport(gl_context_t * const context, const GLint x, const GLint y, const GLsizei w, const GLsizei h) {
    if ((context->viewport.x1 != x) || (context->viewport.y1 != y) || (context->viewport.x2 != w) || (context->viewport.y2 != h)) {
        context->viewport.x1 = x;
        context->viewport.y1 = y;
        context->viewport.x2 = w;
        context->viewport.y2 = h;
        glViewport(x, y, w, h);
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
}

static inline void glc_color_mask(gl_context_t * const context, const uint8_t mask) {
    if (context->bs_desc.color_write_mask != mask) {
        glColorMask(
            (mask & 1) ? GL_TRUE : GL_FALSE,
            (mask & 2) ? GL_TRUE : GL_FALSE,
            (mask & 4) ? GL_TRUE : GL_FALSE,
            (mask & 8) ? GL_TRUE : GL_FALSE);
        CHECK_GL_ERRORS();
        context->bs_desc.color_write_mask = mask;
        ++context->counters.num_api_calls;
    }
}

static inline void glc_enable_blend(gl_context_t * const context, const bool enabled) {
    if (context->bs_desc.blend_enable != enabled) {
        if (enabled) {
            glEnable(GL_BLEND);
            CHECK_GL_ERRORS();
            ++context->counters.num_api_calls;
        } else {
            glDisable(GL_BLEND);
            CHECK_GL_ERRORS();
            ++context->counters.num_api_calls;
        }
        context->bs_desc.blend_enable = enabled;
    }
}

static inline void glc_stencil_mask(gl_context_t * const context, const uint8_t mask) {
    if (context->dss_desc.stencil_write_mask != mask) {
        glStencilMask(mask);
        CHECK_GL_ERRORS();
        context->dss_desc.stencil_write_mask = mask;
        ++context->counters.num_api_calls;
    }
}

static inline void glc_depth_mask(gl_context_t * const context, const bool mask) {
    if (context->dss_desc.depth_write_mask != mask) {
        glDepthMask(mask);
        CHECK_GL_ERRORS();
        context->dss_desc.depth_write_mask = mask;
        ++context->counters.num_api_calls;
    }
}

static inline void glc_enable_scissor(gl_context_t * const context, const bool enabled) {
    if (context->rs_desc.scissor_test != enabled) {
        context->rs_desc.scissor_test = enabled;
        if (enabled) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        CHECK_GL_ERRORS();
    }
}

static inline void glc_cull_mode(gl_context_t * const context, const rhi_cull_face_e cull_mode) {
    if (context->rs_desc.cull_mode != cull_mode) {
        if (context->rs_desc.cull_mode == rhi_cull_face_none) {
            glEnable(GL_CULL_FACE);
            CHECK_GL_ERRORS();
            ++context->counters.num_api_calls;
        } else if (cull_mode == rhi_cull_face_none) {
            glDisable(GL_CULL_FACE);
            CHECK_GL_ERRORS();
            ++context->counters.num_api_calls;
        }

        if ((cull_mode != rhi_cull_face_none) && (cull_mode != context->cull_mode)) {
            glCullFace((cull_mode == rhi_cull_face_front) ? GL_FRONT : GL_BACK);
            CHECK_GL_ERRORS();
            context->cull_mode = cull_mode;
            ++context->counters.num_api_calls;
        }

        context->rs_desc.cull_mode = cull_mode;

        CHECK_GL_ERRORS();
    }
}

/*
===============================================================================
gl_rhi_api_create_device

Create an opengl device that renders into an application window.
===============================================================================
*/

rhi_device_t * gl_rhi_api_create_device(struct sb_window_t * const window, rhi_error_t ** const out_error, const char * const tag);

#ifdef __cplusplus
}
#endif
