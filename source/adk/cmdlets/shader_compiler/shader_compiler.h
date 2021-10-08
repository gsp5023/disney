/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
shader_compiler.h

cmdlet for compiling glsl shaders to target shader type and #include-able memory regions.
*/

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    shader_compiler_success = 0,
    shader_compiler_no_inputs = 1,
    shader_compiler_filename_buff_size = 1024,
};

typedef enum shader_compiler_stage_e {
    shader_compiler_vert,
    shader_compiler_frag,
} shader_compiler_stage_e;

static const char * shader_stage_macro[] = {
    [shader_compiler_vert] = "#define GLSL_VERT",
    [shader_compiler_frag] = "#define GLSL_FRAG",
};

typedef struct shader_target_t {
    const char * macro_guard;
    int alignment;
    const char * name;

    void (*shader_compiler_fn)(const char * const shader_filename, const char * const directory_name, const int alignment, char * const out_compiled_filename, const size_t compiled_filename_len, const shader_compiler_stage_e shader_stage);

} shader_target_t;

int cmdlet_shader_compiler_tool(const int argc, const char * const * const argv);

void shader_compiler_raw_file_to_byte_region(
    const char * const raw_bytes_filepath,
    const char * const glsl_filename,
    const char * const directory_name,
    const int region_alignment,
    char * const out_compiled_filename,
    const size_t compiled_filename_len);

void shader_compiler_compile_glsl_with_glslcc(
    const char * const glslcc_exe,
    const char * const glsl_filename,
    const char * const shader_stage_filename,
    char * const out_target_output_name,
    const size_t target_output_name_len,
    const char * const output_sub_directory,
    const char * const target_shader_language,
    const shader_compiler_stage_e glslcc_shader_stage);

void glsl_to_vader2(
    const char * const glsl_filename,
    const char * const directory_name,
    const int alignment,
    char * const out_vader_compiled_filename,
    const size_t vader_compiled_filename_len,
    const shader_compiler_stage_e shader_stage);

void glsl_to_leia(
    const char * const glsl_filename,
    const char * const directory_name,
    const int alignment,
    char * const out_leia_compiled_filename,
    const size_t leia_compiled_filename_len,
    const shader_compiler_stage_e shader_stage);

void glsl_shader_compiler(
    const char * const shader_filename,
    const char * const directory_name,
    const int alignment,
    char * const out_compiled_glsl_filename,
    const size_t compiled_glsl_filename_len,
    const shader_compiler_stage_e shader_stage);

#ifdef __cplusplus
}
#endif