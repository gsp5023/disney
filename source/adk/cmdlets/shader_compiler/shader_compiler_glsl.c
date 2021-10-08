/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
shader_compiler_glsl.c

support for generating an includable glsl shader
*/

#include "source/adk/cmdlets/shader_compiler/shader_compiler.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_file.h"

extern const char tool_generated_warning[];
extern const char copyright_string[];

extern const char * const shader_compiler_stage_suffix[];

extern const char shader_compiler_source_path[];
extern const char shader_compiler_raw_path[];
extern const char shader_compiler_compiled_path[];
extern const char shader_compiler_precompile_path[];
extern const char shader_compiler_suffix[];

void glsl_shader_compiler(
    const char * const shader_filename,
    const char * const directory_name,
    const int _alignment,
    char * const out_compiled_glsl_filename,
    const size_t compiled_glsl_filename_len,
    const shader_compiler_stage_e shader_stage) {
    char shader_filepath[shader_compiler_filename_buff_size];
    sprintf_s(shader_filepath, ARRAY_SIZE(shader_filepath), "%s%s", shader_compiler_raw_path, shader_filename);
    sprintf_s(out_compiled_glsl_filename, compiled_glsl_filename_len, "%s%s/%s%s", shader_compiler_compiled_path, directory_name, shader_filename, shader_compiler_suffix);

    FILE * const shader_file = fopen(shader_filepath, "r");
    FILE * const shader_common_macros_file = fopen("source/shaders/shader_common_macros.h", "r");
    FILE * const compiled_shader_file = fopen(out_compiled_glsl_filename, "w");
    VERIFY(shader_common_macros_file && shader_file && compiled_shader_file);

    fseek(shader_file, 0, SEEK_END);
    const long shader_file_len = ftell(shader_file);
    fseek(shader_file, 0, SEEK_SET);

    fseek(shader_common_macros_file, 0, SEEK_END);
    const long shader_common_macros_file_len = ftell(shader_common_macros_file);
    fseek(shader_common_macros_file, 0, SEEK_SET);

    const char * const shader_file_bytes = calloc(1, shader_file_len + 1);
    {
        const size_t ignored = fread((void *)shader_file_bytes, 1, shader_file_len, shader_file);
        (void)ignored;
    }
    fclose(shader_file);

    const char * const shader_common_macros_file_bytes = calloc(1, shader_common_macros_file_len + 1);
    {
        const size_t ignored = fread((void *)shader_common_macros_file_bytes, 1, shader_common_macros_file_len, shader_common_macros_file);
        (void)ignored;
    }
    fclose(shader_common_macros_file);

    char shader_identifier[shader_compiler_filename_buff_size];
    sprintf_s(shader_identifier, ARRAY_SIZE(shader_identifier), "%s", shader_filename);

    for (size_t ind = 0; shader_identifier[ind] != '\0'; ++ind) {
        if ((shader_identifier[ind] < '0') || (shader_identifier[ind] > 'z')) {
            shader_identifier[ind] = '_';
        }
    }

    char array_declaration[2 * shader_compiler_filename_buff_size];
    sprintf_s(array_declaration, ARRAY_SIZE(array_declaration), "%s\n#define SHADER_DECLARATION(_x) #_x \n\nstatic const char %s_bytes[] = SHADER_DECLARATION(#version 100\n\\n%s\\n\n", tool_generated_warning, shader_identifier, shader_stage_macro[shader_stage]);

    fwrite(array_declaration, 1, strlen(array_declaration), compiled_shader_file);

    const char newline_string[] = "\\n";
    const char newline_end_of_line_string[] = "\\n\n";

    const char * const files_to_write[] = {shader_common_macros_file_bytes, shader_file_bytes};
    for (int i = 0; i < ARRAY_SIZE(files_to_write); ++i) {
        const char * shader_line = files_to_write[i];
        while (*shader_line) {
            char * const newline_pos = strstr(shader_line, "\n");
            const bool is_macro_line = *shader_line == '#';

            if (is_macro_line) {
                fwrite(newline_string, 1, ARRAY_SIZE(newline_string) - 1, compiled_shader_file);
            }

            const size_t line_length = newline_pos ? (size_t)(newline_pos - shader_line + (is_macro_line ? 0 : 1)) : strlen(shader_line);
            fwrite(shader_line, 1, line_length, compiled_shader_file);

            if (*shader_line == '#') {
                fwrite(newline_end_of_line_string, 1, ARRAY_SIZE(newline_end_of_line_string) - 1, compiled_shader_file);
            }
            if (newline_pos) {
                shader_line = newline_pos + 1;
            } else {
                break;
            }
        }
    }

    const char end_of_array_chars[] = ");\n\n";
    fwrite(end_of_array_chars, 1, ARRAY_SIZE(end_of_array_chars) - 1, compiled_shader_file);

    char region_declaration[4 * shader_compiler_filename_buff_size];
    sprintf_s(region_declaration, ARRAY_SIZE(region_declaration), "static const const_mem_region_t %s_program = {{.ptr = %s_bytes}, .size=ARRAY_SIZE(%s_bytes)};\n\n#undef SHADER_DECLARATION", shader_identifier, shader_identifier, shader_identifier);
    fwrite(region_declaration, 1, strlen(region_declaration), compiled_shader_file);

    fclose(compiled_shader_file);
    free((void *)shader_file_bytes);
    free((void *)shader_common_macros_file_bytes);
}