/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*
rbcmd_func.h

render command dispatch functions
these *must* be in enum order! if you get static assertion errors
in rbcmd.c then there is an out of order entry here.
*/

RENDER_COMMAND_FUNC(render_cmd_rhi_resource_add_ref_indirect,
                    rhi_add_ref(*cmd_args->resource);)

RENDER_COMMAND_FUNC(
    render_cmd_release_rhi_resource_indirect,
    render_cmd_release_rhi_resource(cmd_args);)

RENDER_COMMAND_FUNC(render_cmd_upload_mesh_channel_data_indirect,
                    rhi_upload_mesh_channel_data(device, *cmd_args->mesh, cmd_args->channel_index, cmd_args->first_elem, cmd_args->num_elems, cmd_args->data);)

RENDER_COMMAND_FUNC(render_cmd_upload_mesh_indices_indirect,
                    rhi_upload_mesh_indices(device, *cmd_args->mesh, cmd_args->first_index, cmd_args->num_indices, cmd_args->indices);)

RENDER_COMMAND_FUNC(render_cmd_upload_uniform_data_indirect,
                    rhi_upload_uniform_data(device, *cmd_args->uniform_buffer, cmd_args->data, cmd_args->ofs);)

RENDER_COMMAND_FUNC(render_cmd_upload_texture_indirect,
                    rhi_upload_texture(device, *cmd_args->texture, &cmd_args->mipmaps);)

#if !(defined(_VADER) || defined(_LEIA))
RENDER_COMMAND_FUNC(render_cmd_upload_sub_texture_indirect,
                    rhi_upload_sub_texture(device, *cmd_args->texture, &cmd_args->mipmaps);)
#endif
#if defined(_LEIA) || defined(_VADER)
RENDER_COMMAND_FUNC(render_cmd_bind_texture_address,
                    rhi_bind_texture_address(device, *cmd_args->texture, &cmd_args->mipmaps);)
#endif

RENDER_COMMAND_FUNC(render_cmd_set_display_size,
                    rhi_set_display_size(device, cmd_args->w, cmd_args->h);)

RENDER_COMMAND_FUNC(render_cmd_set_viewport,
                    rhi_set_viewport(device, cmd_args->x0, cmd_args->y0, cmd_args->x1, cmd_args->y1);)

RENDER_COMMAND_FUNC(render_cmd_set_scissor_rect,
                    rhi_set_scissor_rect(device, cmd_args->x0, cmd_args->y0, cmd_args->x1, cmd_args->y1);)

RENDER_COMMAND_FUNC(render_cmd_set_program_indirect,
                    rhi_set_program(device, *cmd_args->program);)

RENDER_COMMAND_FUNC(render_cmd_set_render_state_indirect,
                    rhi_set_render_state(device, cmd_args->rs ? *cmd_args->rs : NULL, cmd_args->dss ? *cmd_args->dss : NULL, cmd_args->bs ? *cmd_args->bs : NULL, cmd_args->stencil_ref);)

RENDER_COMMAND_FUNC(render_cmd_set_render_target_color_buffer_indirect,
                    rhi_set_render_target_color_buffer(device, *cmd_args->render_target, *cmd_args->color_buffer.buffer, cmd_args->color_buffer.index);)

RENDER_COMMAND_FUNC(render_cmd_discard_render_target_data_indirect,
                    rhi_discard_render_target_data(device, *cmd_args->render_target);)

RENDER_COMMAND_FUNC(render_cmd_set_render_target_indirect,
                    rhi_set_render_target(device, *cmd_args->render_target);)

RENDER_COMMAND_FUNC(render_cmd_clear_render_target_color_buffers_indirect,
                    rhi_clear_render_target_color_buffers(device, *cmd_args->render_target, cmd_args->clear_color);)

RENDER_COMMAND_FUNC(render_cmd_copy_render_target_buffers_indirect,
                    rhi_copy_render_target_buffers(device, *cmd_args->src_render_target, *cmd_args->dst_render_target);)

RENDER_COMMAND_FUNC(render_cmd_set_uniform_binding_indirect,
                    rhi_set_uniform_bindings(device, *cmd_args->uniform_binding.buffer, cmd_args->uniform_binding.index);)

RENDER_COMMAND_FUNC(render_cmd_set_texture_sampler_state_indirect,
                    rhi_set_texture_sampler_state(device, *cmd_args->texture, cmd_args->sampler_state);)

RENDER_COMMAND_FUNC(render_cmd_set_texture_bindings_indirect,
                    rhi_set_texture_bindings_indirect(device, &cmd_args->texture_bindings);)

RENDER_COMMAND_FUNC(render_cmd_draw_indirect,
                    rhi_draw_indirect(device, &cmd_args->params);)

RENDER_COMMAND_FUNC(render_cmd_clear_screen_cds,
                    rhi_clear_screen_cds(device, cmd_args->r, cmd_args->g, cmd_args->b, cmd_args->a, cmd_args->d, cmd_args->s);)

RENDER_COMMAND_FUNC(render_cmd_clear_screen_ds,
                    rhi_clear_screen_ds(device, cmd_args->d, cmd_args->s);)

RENDER_COMMAND_FUNC(render_cmd_clear_screen_c,
                    rhi_clear_screen_c(device, cmd_args->r, cmd_args->g, cmd_args->b, cmd_args->a);)

RENDER_COMMAND_FUNC(render_cmd_clear_screen_d,
                    rhi_clear_screen_d(device, cmd_args->d);)

RENDER_COMMAND_FUNC(render_cmd_clear_screen_s,
                    rhi_clear_screen_s(device, cmd_args->s);)

RENDER_COMMAND_FUNC(render_cmd_present,
                    rhi_present(device, cmd_args->swap_interval);)

RENDER_COMMAND_FUNC(render_cmd_screenshot,
                    rhi_screenshot(device, cmd_args->image, cmd_args->mem_region);)

RENDER_COMMAND_FUNC(render_cmd_create_mesh_data_layout,
                    *cmd_args->out = rhi_create_mesh_data_layout(device, &cmd_args->layout_desc, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_mesh_indirect,
                    *cmd_args->out = rhi_create_mesh(device, cmd_args->init, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_mesh_shared_vertex_data_indirect,
                    *cmd_args->out = rhi_create_mesh_from_shared_vertex_data(device, *cmd_args->src, cmd_args->indices, cmd_args->num_indices, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_program_from_binary,
                    *cmd_args->out_program = rhi_create_program_from_binary(device, cmd_args->vert_program, cmd_args->frag_program, cmd_args->out_err, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_blend_state,
                    *cmd_args->out_blend_state = rhi_create_blend_state(device, &cmd_args->blend_state_desc, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_depth_stencil_state,
                    *cmd_args->out_depth_stencil_state = rhi_create_depth_stencil_state(device, &cmd_args->depth_stencil_state_desc, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_rasterizer_state,
                    *cmd_args->out_rasterizer_state = rhi_create_rasterizer_state(device, &cmd_args->rasterizer_state_desc, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_uniform_buffer,
                    *cmd_args->out_uniform_buffer = rhi_create_uniform_buffer(device, &cmd_args->uniform_buffer_desc, cmd_args->init_data, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_texture_2d,
                    *cmd_args->out_texture = rhi_create_texture_2d(device, &cmd_args->mipmaps, cmd_args->format, cmd_args->usage, cmd_args->sampler_state, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_create_render_target,
                    *cmd_args->out_render_target = rhi_create_render_target(device, cmd_args->tag);)

RENDER_COMMAND_FUNC(render_cmd_callback,
                    cmd_args->callback(device, cmd_args->arg);)

RENDER_COMMAND_FUNC(render_cmd_breakpoint,
                    DBG_BREAK();)

#ifdef __cplusplus
}
#endif