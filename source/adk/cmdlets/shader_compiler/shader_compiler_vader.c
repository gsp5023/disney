/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
shader_compiler_vader.c

support for generating vader shaders
*/

#include "source/adk/cmdlets/shader_compiler/shader_compiler.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"
#ifdef _WIN32
#include <direct.h>
#else
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define strdup(_x) _strdup(_x)
#define getcwd(_x, _size) _getcwd(_x, _size)
#endif

extern const char * const shader_compiler_stage_suffix[];

extern const char shader_compiler_source_path[];
extern const char shader_compiler_raw_path[];
extern const char shader_compiler_compiled_path[];
extern const char shader_compiler_precompile_path[];
extern const char shader_compiler_suffix[];

void glsl_to_vader2(
    const char * const glsl_filename,
    const char * const directory_name,
    const int alignment,
    char * const out_vader_compiled_filename,
    const size_t vader_compiled_filename_len,
    const shader_compiler_stage_e _shader_stage) {
    const size_t filename_len = strlen(glsl_filename);

    const char * const fragment_suffix = shader_compiler_stage_suffix[shader_compiler_frag];
    const char * const vertex_suffix = shader_compiler_stage_suffix[shader_compiler_vert];
    const size_t fragment_len = strlen(fragment_suffix);
    const size_t vertex_len = strlen(vertex_suffix);

#ifndef _VADER_SHADER_TARGET
#define _VADER_SHADER_TARGET ""
#endif
    const char vader_vertex_tag[] = "_vv";
    const char vader_fragment_tag[] = "_p";

    char shader_stage_filename[shader_compiler_filename_buff_size];
    shader_compiler_stage_e glslcc_shader_stage = shader_compiler_vert;

    if (strstr(glsl_filename + (filename_len - fragment_len), fragment_suffix) != NULL) {
        const int filelen_to_suffix = (int)(filename_len - fragment_len);
        sprintf_s(shader_stage_filename, ARRAY_SIZE(shader_stage_filename), "%.*s%s.ags", filelen_to_suffix, glsl_filename, vader_fragment_tag);
        glslcc_shader_stage = shader_compiler_frag;

    } else if (strstr(glsl_filename + (filename_len - vertex_len), vertex_suffix) != NULL) {
        const int filelen_to_suffix = (int)(filename_len - vertex_len);
        sprintf_s(shader_stage_filename, ARRAY_SIZE(shader_stage_filename), "%.*s%s.ags", filelen_to_suffix, glsl_filename, vader_vertex_tag);
        glslcc_shader_stage = shader_compiler_vert;
    }

    char vader_output_name[shader_compiler_filename_buff_size];
    shader_compiler_compile_glsl_with_glslcc("glslcc-prospero", glsl_filename, shader_stage_filename, vader_output_name, ARRAY_SIZE(vader_output_name), directory_name, _VADER_SHADER_TARGET, glslcc_shader_stage);

    shader_compiler_raw_file_to_byte_region(vader_output_name, glsl_filename, directory_name, alignment, out_vader_compiled_filename, vader_compiled_filename_len);
}
