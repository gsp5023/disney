/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
shader_compiler_leia.c

support for generating leia shaders
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

void glsl_to_leia(
    const char * const glsl_filename,
    const char * const directory_name,
    const int alignment,
    char * const out_leia_compiled_filename,
    const size_t leia_compiled_filename_len,
    const shader_compiler_stage_e _shader_stage) {
#ifndef _LEIA_SHADER_TARGET
#define _LEIA_SHADER_TARGET "pssl"
#endif
    const size_t filename_len = strlen(glsl_filename);

    const char * const fragment_suffix = shader_compiler_stage_suffix[shader_compiler_frag];
    const char * const vertex_suffix = shader_compiler_stage_suffix[shader_compiler_vert];
    const size_t fragment_len = strlen(fragment_suffix);
    const size_t vertex_len = strlen(vertex_suffix);

    const char leia_vertex_tag[] = "_vs";
    const char leia_fragment_tag[] = "_fs";

    char shader_stage_filename[shader_compiler_filename_buff_size];
    char compiled_stage_filename[shader_compiler_filename_buff_size];
    shader_compiler_stage_e glslcc_shader_stage = shader_compiler_vert;

    const char precompile_path[] = "build/glslcc/precompile/leia/";
    if (strstr(glsl_filename + (filename_len - fragment_len), fragment_suffix) != NULL) {
        const int filelen_to_suffix = (int)(filename_len - fragment_len);
        sprintf_s(shader_stage_filename, ARRAY_SIZE(shader_stage_filename), "%.*s%s.frag", filelen_to_suffix, glsl_filename, leia_fragment_tag);
        sprintf_s(compiled_stage_filename, ARRAY_SIZE(compiled_stage_filename), "%s%.*s%s.sb", precompile_path, filelen_to_suffix, glsl_filename, leia_fragment_tag);
        glslcc_shader_stage = shader_compiler_frag;
    } else if (strstr(glsl_filename + (filename_len - vertex_len), vertex_suffix) != NULL) {
        const int filelen_to_suffix = (int)(filename_len - vertex_len);
        sprintf_s(shader_stage_filename, ARRAY_SIZE(shader_stage_filename), "%.*s%s.vert", filelen_to_suffix, glsl_filename, leia_vertex_tag);
        sprintf_s(compiled_stage_filename, ARRAY_SIZE(compiled_stage_filename), "%s%.*s%s.sb", precompile_path, filelen_to_suffix, glsl_filename, leia_vertex_tag);
        glslcc_shader_stage = shader_compiler_vert;
    }
    char leia_output_name[shader_compiler_filename_buff_size];
    shader_compiler_compile_glsl_with_glslcc("glslcc", glsl_filename, shader_stage_filename, leia_output_name, ARRAY_SIZE(leia_output_name), directory_name, _LEIA_SHADER_TARGET, glslcc_shader_stage);
    shader_compiler_raw_file_to_byte_region(compiled_stage_filename, glsl_filename, directory_name, alignment, out_leia_compiled_filename, leia_compiled_filename_len);
}
