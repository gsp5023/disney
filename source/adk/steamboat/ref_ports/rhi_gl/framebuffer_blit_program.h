/* ===========================================================================
*
* Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
*
* ==========================================================================*/

#include "source/adk/log/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
framebuffer_blit_program.h

framebuffer blit support using a shader program
*/

typedef struct gl_framebuffer_blit_program_t {
    int ref_count;
    GLuint program;
    int id;
    GLuint vb;
    GLuint vbid;
} gl_framebuffer_blit_program_t;

extern gl_framebuffer_blit_program_t gl_framebuffer_blit_program;

#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM_IMPL
gl_framebuffer_blit_program_t gl_framebuffer_blit_program = {0};

#define SHADER_STRING(_x) "#version 100\n" #_x

#define TAG_FB_BLIT FOURCC('F', 'B', 'B', 'T')

static void create_framebuffer_blit_program(gl_context_t * const context) {
    ASSERT(gl_framebuffer_blit_program.ref_count >= 0);

    if (++gl_framebuffer_blit_program.ref_count > 1) {
        return;
    }

    const char * vertex_program = SHADER_STRING(
        attribute vec2 in_pos;
        varying highp vec2 uvs;
        void main() {
            uvs = in_pos * 0.5 + 0.5;
            gl_Position = vec4(in_pos, 0.0, 1.0);
        });

    const char * fragment_program = SHADER_STRING(
        uniform sampler2D tex;
        varying highp vec2 uvs;
        void main() {
            gl_FragColor = texture2D(tex, uvs);
        });

    const GLuint vertex_shader = GL_CALL(glCreateShader(GL_VERTEX_SHADER));
    const GLuint fragment_shader = GL_CALL(glCreateShader(GL_FRAGMENT_SHADER));

    GL_CALL(glShaderSource(vertex_shader, 1, &vertex_program, NULL));
    GL_CALL(glShaderSource(fragment_shader, 1, &fragment_program, NULL));

    GL_CALL(glCompileShader(vertex_shader));

    int status = 0;
    char info_log[4096];

    GL_CALL(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status));
    if (!status) {
        GL_CALL(glGetShaderInfoLog(vertex_shader, ARRAY_SIZE(info_log), NULL, info_log));
        LOG_ERROR(TAG_FB_BLIT, "GLES frambuffer copy vertex shader error\n%s\n", info_log);
        TRAP("vertex shader compilation failure");
    }

    GL_CALL(glCompileShader(fragment_shader));
    GL_CALL(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status));
    if (!status) {
        GL_CALL(glGetShaderInfoLog(fragment_shader, ARRAY_SIZE(info_log), NULL, info_log));
        LOG_ERROR(TAG_FB_BLIT, "GLES frambuffer copy fragment shader error\n%s\n", info_log);
        TRAP("fragment shader compilation failure");
    }

    gl_framebuffer_blit_program.program = GL_CALL(glCreateProgram());
    gl_framebuffer_blit_program.id = gl_get_next_obj_id();

    GL_CALL(glAttachShader(gl_framebuffer_blit_program.program, vertex_shader));
    GL_CALL(glAttachShader(gl_framebuffer_blit_program.program, fragment_shader));
    GL_CALL(glDeleteShader(vertex_shader));
    GL_CALL(glDeleteShader(fragment_shader));
    GL_CALL(glBindAttribLocation(gl_framebuffer_blit_program.program, 0, "in_pos"));
    GL_CALL(glLinkProgram(gl_framebuffer_blit_program.program));
    GL_CALL(glGetProgramiv(gl_framebuffer_blit_program.program, GL_LINK_STATUS, &status));

    if (!status) {
        GL_CALL(glGetProgramInfoLog(gl_framebuffer_blit_program.program, ARRAY_SIZE(info_log), NULL, info_log));
        LOG_ERROR(TAG_FB_BLIT, "GLES frambuffer program link error\n%s\n", info_log);
        TRAP("Shader link failure");
    }

    // bind the sampler location
    const int sampler_location = GL_CALL(glGetUniformLocation(gl_framebuffer_blit_program.program, "tex"));
    if (sampler_location != -1) {
        VERIFY(sampler_location != -1);
        context->program = NULL;
        context->program_id = gl_framebuffer_blit_program.id;
        GL_CALL(glUseProgram(gl_framebuffer_blit_program.program));
        GL_CALL(glUniform1i(sampler_location, 0));
    }

    const float verts[4][2] = {
        {-1.0f, 1.0f},
        {-1.0f, -1.0f},
        {1.0f, -1.0f},
        {1.0f, 1.0f}};

#ifdef GL_VAOS
    // don't modify a bound VAO
#ifdef GL_CORE
    extern GLuint glcore_default_vertex_array_id;
    glc_bind_vertex_array(context, glcore_default_vertex_array_id, 0);
#else // GL_ES
    glc_bind_vertex_array(context, 0, 0);
#endif
#endif

#ifdef GL_DSA
    glf.glCreateBuffers(1, &gl_framebuffer_blit_program.vb);
#else
    GL_CALL(glGenBuffers(1, &gl_framebuffer_blit_program.vb));
#endif
    ++context->counters.num_api_calls;
    CHECK_GL_ERRORS();
    gl_framebuffer_blit_program.vbid = gl_get_next_obj_id();
    glc_static_buffer_storage(context, GL_ARRAY_BUFFER, gl_framebuffer_blit_program.vb, gl_framebuffer_blit_program.vbid, sizeof(verts), verts);

    context->counters.num_api_calls += 17;
}

static void destroy_framebuffer_blit_program() {
    ASSERT(gl_framebuffer_blit_program.ref_count > 0);
    if (--gl_framebuffer_blit_program.ref_count == 0) {
        GL_CALL(glDeleteProgram(gl_framebuffer_blit_program.program));
#ifdef GL_CORE
        GL_CALL(glDeleteBuffers(1, &gl_framebuffer_blit_program.vb));
#endif
    }
}

#undef SHADER_STRING
#endif

#ifdef __cplusplus
}
#endif
