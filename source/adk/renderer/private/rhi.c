/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rhi.c

RHI support
*/

#include _PCH
#include "rhi.h"

const char * const rhi_program_semantic_names[rhi_program_input_num_semantics] = {
    "in_pos0",
    "in_col0",
    "in_nml0",
    "in_bin0",
    "in_tan0",
    "in_tc0",
    "in_tc1"};

const char * const rhi_program_uniform_names[rhi_program_num_uniforms] = {
    [rhi_program_uniform_mvp] = "u_mvp",
    [rhi_program_uniform_viewport] = "u_viewport",
    [rhi_program_uniform_tex0] = "u_tex0",
    [rhi_program_uniform_tex1] = "u_tex1",
    [rhi_program_uniform_fill] = "u_fill",
    [rhi_program_uniform_threshold] = "u_threshold",
};

const enum rhi_program_uniform_shader_stage_e rhi_program_uniform_shader_stages[rhi_program_num_uniforms] = {
    [rhi_program_uniform_mvp] = rhi_program_uniform_shader_stage_vertex,
    [rhi_program_uniform_viewport] = rhi_program_uniform_shader_stage_vertex,
    [rhi_program_uniform_tex0] = rhi_program_uniform_shader_stage_fragment,
    [rhi_program_uniform_tex1] = rhi_program_uniform_shader_stage_fragment,
    [rhi_program_uniform_fill] = rhi_program_uniform_shader_stage_fragment,
    [rhi_program_uniform_threshold] = rhi_program_uniform_shader_stage_fragment,
};

const enum rhi_program_uniform_e rhi_program_texture_samplers[rhi_program_num_textures] = {
    rhi_program_uniform_tex0,
    rhi_program_uniform_tex1,
};