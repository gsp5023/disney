/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rhi_gl_device_gles.c

rhi_gl_device implementation with GLES context
*/

#include _PCH
#include "rhi_gl_device.h"

#ifndef GL_ES
#error "do not include this on non GLES platforms!"
#endif

#ifdef _RPI
#include "source/adk/steamboat/ref_ports/linux/dispmanx.h"
#endif

#define GL_FRAMEBUFFER_BLIT_PROGRAM_IMPL
#include "framebuffer_blit_program.h"

static struct {
    int ref_count;
    EGLConfig config;
} statics = {0};

// for starboard hooks
EGLDisplay egl_display;

enum {
    red_bits = 8,
    blue_bits = 8,
    green_bits = 8,
    alpha_bits = 8,
    depth_bits = 24,
    stencil_bits = 8
};

/*
=======================================
EGL library init
=======================================
*/

static bool egl_init(rhi_error_t ** const out_error) {
    ASSERT(statics.ref_count >= 0);

    if (++statics.ref_count > 1) {
        return true;
    }

    ASSERT(!egl_display);
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY) {
        if (out_error) {
            *out_error = GL_CREATE_ERROR("eglGetDisplay() failed, did you register any exclusive displays?");
        }
        return false;
    }

    if (!eglInitialize(egl_display, NULL, NULL)) {
        if (out_error) {
            *out_error = GL_CREATE_ERROR("eglInitialize() failed.");
        }
        return false;
    }

    VERIFY(eglBindAPI(EGL_OPENGL_ES_API));

    {
        const EGLint attrs[] = {
            EGL_RED_SIZE, red_bits, EGL_GREEN_SIZE, green_bits, EGL_BLUE_SIZE, blue_bits, EGL_ALPHA_SIZE, alpha_bits, EGL_DEPTH_SIZE, depth_bits, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};

        int num_configs;
        if (!eglChooseConfig(egl_display, attrs, &statics.config, 1, &num_configs) || (num_configs != 1)) {
            if (*out_error) {
                *out_error = GL_CREATE_ERROR("eglChooseConfig() failed.");
            }
            return false;
        }
    }

    return true;
}

static void egl_shutdown() {
    ASSERT(statics.ref_count > 0);
    if (--statics.ref_count > 0) {
        return;
    }

    eglTerminate(egl_display);
    egl_display = EGL_NO_DISPLAY;
}

/*
=======================================
set_swap_interval
=======================================
*/

static void set_swap_interval(const rhi_swap_interval_t swap_interval) {
    eglSwapInterval(egl_display, (swap_interval.interval < 0) ? 0 : swap_interval.interval);
}

/*
=======================================
glc_create
=======================================
*/
bool glc_create(gl_context_t * const context, rhi_error_t ** const out_error) {
    if (!egl_init(out_error)) {
        return false;
    }

    {
        const EGLint attrs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

        context->egl_context = eglCreateContext(egl_display, statics.config, EGL_NO_CONTEXT, attrs);
        if (context->egl_context == EGL_NO_CONTEXT) {
            if (out_error) {
                *out_error = GL_CREATE_ERROR("eglCreateContext() failed.");
            }
            return false;
        }
    }

    // create window surface
#ifdef _RPI
    {
        dispmanx_window_t * dispmanx_window = (dispmanx_window_t *)context->window;
        context->egl_surface = eglCreateWindowSurface(egl_display, statics.config, &dispmanx_window->window, NULL);
    }
#elif (defined(NEXUS_PLATFORM) || defined(POSIX_STUB_GLES) || defined(_MTV))
    context->egl_surface = eglCreateWindowSurface(egl_display, statics.config, (EGLNativeWindowType)context->window, NULL);
#else
#error "port me"
#endif

    if (context->egl_surface == EGL_NO_SURFACE) {
        if (out_error) {
            *out_error = GL_CREATE_ERROR("eglCreateWindowSurface() failed.");
        }
    }

    glc_make_current(context);

    // load table if not already
    if (!glf.version_major) {
        VERIFY(load_glf_t(&glf, (void * (*)(const char *))eglGetProcAddress, eglQueryString(egl_display, EGL_EXTENSIONS)));
    }

#ifdef GL_DEBUG_CONTEXT
    if (glf.GL_KHR_debug_output) {
        glf.glDebugMessageCallbackKHR(gl_logger_callback, NULL);
        glf.glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, 0, NULL, GL_FALSE);

        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
    }
#endif

    // check device caps

    context->caps.max_device_threads = 1;
    context->caps.tc_top_left = 1;

    CHECK_GL_ERRORS();
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &context->caps.max_texture_size);
    CHECK_GL_ERRORS();
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &context->caps.max_vertex_inputs);
    CHECK_GL_ERRORS();
    ASSERT(context->caps.max_vertex_inputs >= rhi_program_input_num_semantics);
    if (context->caps.max_vertex_inputs < rhi_program_input_num_semantics) {
        *out_error = GL_CREATE_ERROR_PRINTF("GL_MAX_VERTEX_ATTRIBS(%d) < rhi_shader_input_num_semantics!", context->caps.max_vertex_inputs);
        glc_make_current(NULL);
        return false;
    }
    {
        GLint i;
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &i);
        CHECK_GL_ERRORS();
        context->caps.max_fragment_inputs = i; // number of float4's
    }

    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &context->caps.max_vertex_textures);
    CHECK_GL_ERRORS();
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &context->caps.max_fragment_textures);
    CHECK_GL_ERRORS();
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &context->caps.max_vertex_uniforms);
    CHECK_GL_ERRORS();
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &context->caps.max_vertex_uniforms);
    CHECK_GL_ERRORS();
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &context->caps.max_combined_textures);
    CHECK_GL_ERRORS();

    context->swap_interval = (rhi_swap_interval_t){0};
    set_swap_interval(context->swap_interval);

    CHECK_GL_ERRORS();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    CHECK_GL_ERRORS();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    CHECK_GL_ERRORS();
    glDisable(GL_DEPTH_TEST);
    CHECK_GL_ERRORS();
    glDisable(GL_SCISSOR_TEST);
    CHECK_GL_ERRORS();
    glDisable(GL_STENCIL_TEST);
    CHECK_GL_ERRORS();
    glClearStencil(0);
    CHECK_GL_ERRORS();
    glDepthFunc(GL_LESS);
    CHECK_GL_ERRORS();
    glDisable(GL_CULL_FACE);
    CHECK_GL_ERRORS();
    glDisable(GL_BLEND);
    CHECK_GL_ERRORS();
    GL_CALL(glBlendColor(1.f, 1.f, 1.f, 1.f));
    CHECK_GL_ERRORS();
    glDepthMask(GL_TRUE);
    CHECK_GL_ERRORS();
    glFrontFace(GL_CCW);
    CHECK_GL_ERRORS();
    glCullFace(GL_BACK);
    CHECK_GL_ERRORS();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    CHECK_GL_ERRORS();
    glBlendFunc(GL_ONE, GL_ZERO);
    CHECK_GL_ERRORS();
    GL_CALL(glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD));
    CHECK_GL_ERRORS();
    GL_CALL(glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xff));
    CHECK_GL_ERRORS();
    glStencilMask(0xff);
    CHECK_GL_ERRORS();
    GL_CALL(glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP));
    CHECK_GL_ERRORS();

    context->rs_desc.cull_mode = rhi_cull_face_none;
    context->cull_mode = rhi_cull_face_back;

    context->stencil_ref = 0;
    context->dss_desc.depth_write_mask = true;
    context->dss_desc.depth_test_func = rhi_compare_less;
    context->dss_desc.stencil_read_mask = 0xff;
    context->dss_desc.stencil_write_mask = 0xff;

    context->dss_desc.stencil_front.stencil_fail_op = rhi_stencil_op_keep;
    context->dss_desc.stencil_front.depth_fail_op = rhi_stencil_op_keep;
    context->dss_desc.stencil_front.stencil_depth_pass_op = rhi_stencil_op_keep;
    context->dss_desc.stencil_front.stencil_test_func = rhi_compare_always;

    context->dss_desc.stencil_back.stencil_fail_op = rhi_stencil_op_keep;
    context->dss_desc.stencil_back.depth_fail_op = rhi_stencil_op_keep;
    context->dss_desc.stencil_back.stencil_depth_pass_op = rhi_stencil_op_keep;
    context->dss_desc.stencil_back.stencil_test_func = rhi_compare_always;

    context->bs_desc.src_blend = rhi_blend_one;
    context->bs_desc.dst_blend = rhi_blend_zero;
    context->bs_desc.blend_op = rhi_blend_op_add;
    context->bs_desc.src_blend_alpha = rhi_blend_one;
    context->bs_desc.dst_blend_alpha = rhi_blend_zero;
    context->bs_desc.blend_op_alpha = rhi_blend_op_add;
    context->bs_desc.color_write_mask = rhi_color_write_mask_all;

    for (int i = 0; i < ARRAY_SIZE(context->bs_desc.blend_color); ++i) {
        context->bs_desc.blend_color[i] = 1.f;
    }

    create_framebuffer_blit_program(context);

    return true;
}

/*
=======================================
glc_destroy
=======================================
*/

void glc_destroy(gl_context_t * const context, const char * const tag) {
    destroy_framebuffer_blit_program();

    glc_make_current(NULL);
    egl_shutdown();
}

/*
=======================================
glc_make_current
=======================================
*/

void glc_make_current(gl_context_t * const context) {
    if (context != gl_thread_context) {
        if (context) {
            ASSERT(context->window);
            eglMakeCurrent(egl_display, context->egl_surface, context->egl_surface, context->egl_context);
        } else {
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        gl_thread_context = context;
    }
}

/*
=======================================
glc_swap_buffers
=======================================
*/

void glc_swap_buffers(gl_context_t * const context, const rhi_swap_interval_t swap_interval) {
    if (context->swap_interval.interval != swap_interval.interval) {
        context->swap_interval = swap_interval;
        set_swap_interval(swap_interval);
    }

    eglSwapBuffers(egl_display, context->egl_surface);
}
