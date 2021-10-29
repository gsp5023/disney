/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rhi_gl_device_glfw.c

rhi_gl_device implementation with GLFW context
*/

#include _PCH
#ifndef _GLFW
#error "do not include this on non GLFW platforms!"
#endif

#include "rhi_gl_device.h"

#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
#define GL_FRAMEBUFFER_BLIT_PROGRAM_IMPL
#include "framebuffer_blit_program.h"
GLuint glcore_default_vertex_array_id;
#endif

#include "source/adk/steamboat/private/glfw/glfw_support.h"

/*
=======================================
set_swap_interval
=======================================
*/

static void set_swap_interval(const rhi_swap_interval_t swap_interval) {
#if defined(_WIN32) || defined(_GLX)
    if (glf.EXT_swap_interval && (swap_interval.interval < 0)) {
        glfwSwapInterval(-1);
    } else
#endif
    {
        glfwSwapInterval(swap_interval.interval < 0 ? 0 : swap_interval.interval);
    }
}

/*
=======================================
glc_create
=======================================
*/

bool glc_create(gl_context_t * const context, rhi_error_t ** const out_error) {
    glc_make_current(context);

    // load table if not already
    if (!glf.version_major) {
        VERIFY(load_glf_t(&glf, (void * (*)(const char *))glfwGetProcAddress, NULL));
    }

#ifdef GL_DEBUG_CONTEXT
#ifdef GL_CORE
    if (glf.ARB_debug_output) {
        glf.glDebugMessageCallbackARB(gl_logger_callback, NULL);
        glf.glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);

        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }
#else // GL_ES
    if (glf.GL_KHR_debug_output) {
        glf.glDebugMessageCallbackKHR(gl_logger_callback, NULL);
        glf.glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_KHR, 0, NULL, GL_FALSE);

        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
    }
#endif
#endif

    // check device caps

    context->caps.max_device_threads = 1;
    context->caps.tc_top_left = 1;

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
        glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &i);
        CHECK_GL_ERRORS();
        context->caps.max_fragment_inputs = i / 4; // number of float4's
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

    {
        GLint i;
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &i);
        CHECK_GL_ERRORS();

        if (i < rhi_max_render_target_color_buffers) {
            *out_error = GL_CREATE_ERROR_PRINTF("GL_MAX_COLOR_ATTACHMENTS(%d) < rhi_max_render_target_color_buffers!", i);
            glc_make_current(NULL);
            return false;
        }
    }

    {
        GLint i;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &i);
        CHECK_GL_ERRORS();

        if (i < rhi_max_render_target_color_buffers) {
            *out_error = GL_CREATE_ERROR_PRINTF("GL_MAX_DRAW_BUFFERS(%d) < rhi_max_render_target_color_buffers!", i);
            glc_make_current(NULL);
            return false;
        }
    }

#ifdef GL_CORE
#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
    GL_CALL(glGenVertexArrays(1, &glcore_default_vertex_array_id));
    glf.glBindVertexArray(glcore_default_vertex_array_id);
#elif !defined(GL_VAOS)
    { // 3.2 requires a vao be bound
        GLuint vao;
        GL_CALL(glGenVertexArrays(1, &vao));
        glf.glBindVertexArray(vao);
    }
#endif
#endif

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
#ifdef GL_CORE
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    CHECK_GL_ERRORS();
#endif
    GL_CALL(glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD));
    CHECK_GL_ERRORS();
    GL_CALL(glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xff));
    CHECK_GL_ERRORS();
    glStencilMask(0xff);
    CHECK_GL_ERRORS();
    GL_CALL(glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP));
    CHECK_GL_ERRORS();

#ifndef GL_DSA
    context->active_texture = GL_TEXTURE0;
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    CHECK_GL_ERRORS();
#endif

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

#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
    create_framebuffer_blit_program(context);
#endif

    return true;
}

/*
=======================================
glc_destroy
=======================================
*/

void glc_destroy(gl_context_t * const context, const char * const tag) {
#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
    destroy_framebuffer_blit_program();
#endif
    // glfw context is tied to window, nothing for us to do
    glc_make_current(NULL);
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
            glfwMakeContextCurrent((GLFWwindow *)context->window);
        } else {
            glfwMakeContextCurrent(NULL);
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

    glfwSwapBuffers((GLFWwindow *)context->window);
}
