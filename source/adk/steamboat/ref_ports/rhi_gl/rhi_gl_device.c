/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rhi_gl_device.c

rhi_gl_device implementation
*/

#include _PCH

#include "rhi_gl_device.h"

#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
#include "framebuffer_blit_program.h"
#endif

#include "source/adk/imagelib/imagelib.h"
#include "source/adk/runtime/crc.h"

/*
===============================================================================
OpenGL RHI Device Objects
===============================================================================
*/

glf_t glf;

THREAD_LOCAL gl_context_t * gl_thread_context;

#define BIND_THREAD_CONTEXT(_var)                  \
    gl_context_t * const _var = gl_thread_context; \
    ASSERT(_var)

#ifdef GL_VAOS
typedef struct gl_vao_t gl_vao_t;
typedef struct gl_device_and_context_t gl_device_and_context_t;
typedef struct gl_vao_t {
    gl_vao_t *prev, *next;
    gl_device_and_context_t * device;
    int ref_count;
    int device_id;
    int resource_id;
    GLuint vao_id;
} gl_vao_t;
#endif

/*
=======================================
gl_device_and_context_t

contains the rhi device common fields and the opengl context
associated with the device
=======================================
*/

static gl_device_and_context_t * devices[rhi_max_devices] = {0};

#ifdef GL_VAOS

/*
=======================================
gl_vao_t

Vertex Array Objects are device-specific objects (i.e. they
are not resources that are shared across devices like textures,
shaders etc).

RHI objects are not bound to a specific device and need to be usable
across different devices (example: a multi-window editor has a device
per window).

Opengl causes us pain when we want to use VAO's efficiently. To solve
this we index devices and store per-device VAOs in gl_mesh_t objects.

The opengl RHI is only driven by a single command dispatch thread
so we don't have to do synchronization here since vao's only get
manipulated on a single thread.
=======================================
*/

static int gl_vao_add_ref(gl_vao_t * const vao) {
    const int r = ++vao->ref_count;
    ASSERT(r > 1);
    return r;
}

static int gl_vao_release(gl_vao_t * const vao, const char * const tag) {
    const int r = --vao->ref_count;
    ASSERT(r >= 0);
    if (r == 0) {
        gl_free(vao, tag);
    }

    return r;
}

static void gl_vao_unlink_from_device_and_link_for_destroy(gl_vao_t * const vao, const char * const tag) {
    ASSERT(vao->device);

    // remove from device's used chain

    if (vao->prev) {
        vao->prev->next = vao->next;
    }
    if (vao->next) {
        vao->next->prev = vao->prev;
    }

    if (vao->device->vao_used_chain == vao) {
        vao->device->vao_used_chain = vao->next;
    }

    // link the vao into the pending destroy chain
    // this chain will be flushed during the device's
    // present call

    vao->prev = NULL;
    vao->next = vao->device->vao_pending_destroy_chain;
    vao->device->vao_pending_destroy_chain = vao;

    // vao is unlinked from device now
    vao->device = NULL;
}

static void gl_free_vao_chain(gl_context_t * const context, gl_vao_t ** const chain, const char * const tag) {
    gl_vao_t * next;
    for (gl_vao_t * vao = *chain; vao; vao = next) {
        next = vao->next;
        vao->device = NULL;
        glc_delete_vertex_arrays(context, 1, &vao->vao_id);
        gl_vao_release(vao, tag);
    }

    *chain = NULL;
}

#endif

/*
=======================================
gl_blend_state_t
=======================================
*/

typedef struct gl_blend_state_t {
    gl_resource_t resource;
    rhi_blend_state_desc_t desc;
} gl_blend_state_t;

static gl_blend_state_t * gl_create_blend_state(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    return (gl_blend_state_t *)gl_create_resource(sizeof(gl_blend_state_t), outer, NULL, tag);
}

/*
=======================================
gl_depth_stencil_state_t
=======================================
*/

typedef struct gl_depth_stencil_state_t {
    gl_resource_t resource;
    rhi_depth_stencil_state_desc_t desc;
} gl_depth_stencil_state_t;

static gl_depth_stencil_state_t * gl_create_depth_stencil_state(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    return (gl_depth_stencil_state_t *)gl_create_resource(sizeof(gl_depth_stencil_state_t), outer, NULL, tag);
}

/*
=======================================
gl_rasterizer_state_t
=======================================
*/

typedef struct gl_rasterizer_state_t {
    gl_resource_t resource;
    rhi_rasterizer_state_desc_t desc;
} gl_rasterizer_state_t;

static gl_rasterizer_state_t * gl_create_rasterizer_state(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    return (gl_rasterizer_state_t *)gl_create_resource(sizeof(gl_rasterizer_state_t), outer, NULL, tag);
}

/*
=======================================
gl_texture_t
=======================================
*/

static void gl_destroy_texture(gl_resource_t * const resource, const char * const tag) {
    BIND_THREAD_CONTEXT(context);
    gl_texture_t * const texture = (gl_texture_t *)resource;
    glc_delete_textures(context, 1, &texture->texture);
#ifdef GL_CORE
    glc_delete_samplers(context, 1, &texture->sampler);
#endif
}

static const gl_resource_vtable_t gl_texture_vtable_t = {
    .destroy = gl_destroy_texture};

gl_texture_t * glc_create_texture(gl_context_t * const context, rhi_resource_t * const outer, GLenum target, const char * const tag) {
    gl_texture_t * const texture = (gl_texture_t *)gl_create_resource(sizeof(gl_texture_t), outer, &gl_texture_vtable_t, tag);
    texture->target = target;
    glc_gen_textures(context, target, 1, &texture->texture);
#ifndef GL_DSA
    // non-dsa path creates dimensionless textures, bind to create
    glc_bind_texture(context, GL_TEXTURE0, target, texture->texture, texture->resource.resource.instance_id);
#endif
#ifdef GL_CORE
    glc_gen_samplers(context, 1, &texture->sampler);
#else // GL_ES
    texture->mipmaps = false;
#endif
    return texture;
}

/*
=======================================
gl_uniform_buffer_t
=======================================
*/

typedef struct gl_uniform_buffer_t {
    gl_resource_t resource;
    mem_region_t data;
    uint32_t crc;
    bool dynamic;
} gl_uniform_buffer_t;

static void gl_destroy_uniform_buffer(gl_resource_t * const resource, const char * const tag) {
    gl_uniform_buffer_t * const sub = (gl_uniform_buffer_t *)resource;
    if (sub->data.ptr) {
        gl_free(sub->data.ptr, tag);
    }
}

static const gl_resource_vtable_t gl_uniform_buffer_vtable_t = {
    .destroy = gl_destroy_uniform_buffer};

static gl_uniform_buffer_t * gl_create_uniform_buffer(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    gl_uniform_buffer_t * const sub = (gl_uniform_buffer_t *)gl_create_resource(sizeof(gl_uniform_buffer_t), outer, &gl_uniform_buffer_vtable_t, tag);
    return sub;
}

/*
=======================================
gl_mesh_data_layout_t
=======================================
*/

typedef struct gl_mesh_data_layout_t {
    gl_resource_t resource;
    rhi_mesh_data_layout_desc_t desc;
    int index_size;
    GLenum index_type;
} gl_mesh_data_layout_t;

static gl_mesh_data_layout_t * gl_create_mesh_data_layout(gl_context_t * const context, rhi_resource_t * const outer, const rhi_mesh_data_layout_desc_t * const layout_to_copy, const char * const tag) {
    // allocate all this in 1 block and copy it
    int struct_size = sizeof(gl_mesh_data_layout_t);

    if (layout_to_copy->indices) {
        struct_size += sizeof(rhi_index_layout_desc_t);
    }

    for (int i = 0; i < layout_to_copy->num_channels; ++i) {
        struct_size += sizeof(rhi_vertex_layout_desc_t) + (layout_to_copy->channels[i].num_elements * sizeof(rhi_vertex_element_desc_t));
    }

    uintptr_t base = (uintptr_t)gl_create_resource(struct_size, outer, NULL, tag);

    gl_mesh_data_layout_t * layout = (gl_mesh_data_layout_t *)base;
    layout->desc = *layout_to_copy;
    base += sizeof(gl_mesh_data_layout_t);

    if (layout_to_copy->indices) {
        layout->desc.indices = (rhi_index_layout_desc_t *)base;
        ASSERT_ALIGNED(layout->desc.indices, ALIGN_OF(rhi_index_layout_desc_t));
        *((rhi_index_layout_desc_t *)layout->desc.indices) = *layout_to_copy->indices;
        layout->index_size = (layout_to_copy->indices->format == rhi_vertex_index_format_uint16) ? 2 : 4;
        layout->index_type = (layout_to_copy->indices->format == rhi_vertex_index_format_uint16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        base += sizeof(rhi_index_layout_desc_t);
    }

    layout->desc.channels = (rhi_vertex_layout_desc_t *)base;
    ASSERT_ALIGNED(layout->desc.channels, ALIGN_OF(rhi_vertex_layout_desc_t));
    base += layout_to_copy->num_channels * sizeof(rhi_vertex_layout_desc_t);

    for (int i = 0; i < layout_to_copy->num_channels; ++i) {
        rhi_vertex_layout_desc_t channel = layout_to_copy->channels[i];
        ASSERT(channel.stride > 0);
        channel.elements = (rhi_vertex_element_desc_t *)base;
        ASSERT_ALIGNED(channel.elements, ALIGN_OF(rhi_vertex_element_desc_t));
        memcpy((void *)channel.elements, layout_to_copy->channels[i].elements, sizeof(rhi_vertex_element_desc_t) * channel.num_elements);
        base += sizeof(rhi_vertex_element_desc_t) * channel.num_elements;
        ((rhi_vertex_layout_desc_t *)layout->desc.channels)[i] = channel;
    }

    return layout;
}

/*
=======================================
gl_mesh_t
=======================================
*/

typedef struct gl_mesh_t {
    gl_resource_t resource;

    // begin contiguous block
    GLuint * vertex_buffers;
    int32_t * vertex_buffer_ids;
    int32_t * vertex_buffer_sizes;
    // end contiguous block

    int num_indices;
    gl_mesh_data_layout_t * layout;
    struct gl_mesh_t * shared_verts;
#ifdef GL_VAOS
    gl_vao_t * vaos[rhi_max_devices];
#endif
    const char * tag;
    GLuint index_buffer;
    int index_buffer_id;
} gl_mesh_t;

static void gl_destroy_mesh(gl_resource_t * const resource, const char * const tag) {
    BIND_THREAD_CONTEXT(context);
    gl_mesh_t * const mesh = (gl_mesh_t *)resource;
    ASSERT(mesh->layout);

#ifdef GL_VAOS
    for (int i = 0; i < ARRAY_SIZE(mesh->vaos); ++i) {
        gl_vao_t * vao = mesh->vaos[i];
        if (vao) {
            gl_vao_unlink_from_device_and_link_for_destroy(vao, tag);
        }
    }
#endif

    if (mesh->index_buffer) {
        glc_delete_buffers(context, 1, &mesh->index_buffer);
    }

    if (mesh->vertex_buffers && !mesh->shared_verts) {
        // we own these vertex buffers
        glc_delete_buffers(context, mesh->layout->desc.num_channels, mesh->vertex_buffers);
        gl_free(mesh->vertex_buffers, tag); // this also frees mesh->vertex_buffer_ids!
    }

    if (mesh->shared_verts) {
        rhi_release(&mesh->shared_verts->resource.resource, tag);
    } else if (mesh->layout) {
        rhi_release(&mesh->layout->resource.resource, tag);
    }
}

static const gl_resource_vtable_t gl_mesh_vtable_t = {
    .destroy = gl_destroy_mesh};

static gl_mesh_t * gl_create_mesh(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    return (gl_mesh_t *)gl_create_resource(sizeof(gl_mesh_t), outer, &gl_mesh_vtable_t, tag);
}

/*
=======================================
gl_program_t
=======================================
*/

typedef struct gl_program_t {
    gl_resource_t resource;
    GLuint program;
    GLint uniform_locations[rhi_program_num_uniforms];
    uint32_t uniform_crcs[rhi_program_num_uniforms];
    int uniform_updates;
} gl_program_t;

static void gl_destroy_program(gl_resource_t * const resource, const char * const tag) {
    BIND_THREAD_CONTEXT(context);
    gl_program_t * const program = (gl_program_t *)resource;
    glc_delete_program(context, program->program);
}

static const gl_resource_vtable_t gl_program_vtable_t = {
    .destroy = gl_destroy_program};

static gl_program_t * gl_create_program(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    gl_program_t * const program = (gl_program_t *)gl_create_resource(sizeof(gl_program_t), outer, &gl_program_vtable_t, tag);
    program->program = glc_create_program(context);
    return program;
}

static inline void glc_use_program(gl_context_t * const context, gl_program_t * const program) {
    ASSERT(program);
    if (context->program_id != program->resource.resource.instance_id) {
        context->program_id = program->resource.resource.instance_id;
        context->program = program;
        GL_CALL(glUseProgram(program->program));
        CHECK_GL_ERRORS();
        ++context->counters.num_api_calls;
    }
}

/*
=======================================
glc_sync_program_uniform_data

if uniform data has changed, reupload for this program
NOTE: opengl program uniforms are unique-per-program unless
we use uniform buffers which we don't because of glES
=======================================
*/

static void gl_upload_uniform_matrix4x4(const int index, const gl_uniform_buffer_t * const ub) {
    ASSERT(ub->data.size == sizeof(float) * 16);
    ASSERT(ub->data.ptr);
    GL_CALL(glUniformMatrix4fv(index, 1, false, ub->data.ptr));
    CHECK_GL_ERRORS();
}

static void gl_upload_uniform_1i(const int index, const gl_uniform_buffer_t * const ub) {
    ASSERT(ub->data.size == 16); // 16 byte aligned
    ASSERT(ub->data.ptr);
    GL_CALL(glUniform1i(index, *(const int *)ub->data.ptr));
    CHECK_GL_ERRORS();
}

static void gl_upload_uniform_1f(const int index, const gl_uniform_buffer_t * const ub) {
    ASSERT(ub->data.size == 16); // 16 byte aligned
    ASSERT(ub->data.ptr);
    GL_CALL(glUniform1f(index, *(const float *)ub->data.ptr));
    CHECK_GL_ERRORS();
}

static void gl_upload_uniform_ivec2(const int index, const gl_uniform_buffer_t * const ub) {
    ASSERT(ub->data.size == 16); // 16 byte aligned
    ASSERT(ub->data.ptr);
    GL_CALL(glUniform2iv(index, 1, (const int *)ub->data.ptr));
    CHECK_GL_ERRORS();
}

static void gl_upload_uniform_vec2f(const int index, const gl_uniform_buffer_t * const ub) {
    ASSERT(ub->data.size == 16); // 16 byte aligned
    ASSERT(ub->data.ptr);
    GL_CALL(glUniform2fv(index, 1, (const float *)ub->data.ptr));
    CHECK_GL_ERRORS();
}

static void gl_upload_uniform_vec4f(const int index, const gl_uniform_buffer_t * const ub) {
    ASSERT(ub->data.size == 16); // 16 byte aligned
    ASSERT(ub->data.ptr);
    GL_CALL(glUniform4fv(index, 1, (const float *)ub->data.ptr));
    CHECK_GL_ERRORS();
}

static void (*gl_upload_uniform_funcs[rhi_program_num_uniforms])(const int index, const gl_uniform_buffer_t * const ub) = {
    [rhi_program_uniform_mvp] = gl_upload_uniform_matrix4x4,
    [rhi_program_uniform_viewport] = gl_upload_uniform_vec2f,
    [rhi_program_uniform_tex0] = gl_upload_uniform_1i,
    [rhi_program_uniform_tex1] = gl_upload_uniform_1i,
    [rhi_program_uniform_fill] = gl_upload_uniform_vec4f,
    [rhi_program_uniform_threshold] = gl_upload_uniform_1f,
    [rhi_program_uniform_rect_roundness] = gl_upload_uniform_1f,
    [rhi_program_uniform_rect] = gl_upload_uniform_vec4f,
    [rhi_program_uniform_fade] = gl_upload_uniform_1f,
    [rhi_program_uniform_stroke_color] = gl_upload_uniform_vec4f,
    [rhi_program_uniform_stroke_size] = gl_upload_uniform_1f,
    [rhi_program_uniform_ltexsize] = gl_upload_uniform_ivec2,
    [rhi_program_uniform_ctexsize] = gl_upload_uniform_ivec2,
    [rhi_program_uniform_framesize] = gl_upload_uniform_ivec2,
};

static inline void glc_sync_program_uniform_data(gl_context_t * const context) {
    gl_program_t * const program = context->program;
    ASSERT(program);
    ASSERT(context->program_id == program->resource.resource.instance_id);

    if (program->uniform_updates != context->uniform_updates) {
        program->uniform_updates = context->uniform_updates;
        for (int i = 0; i < rhi_program_num_uniforms; ++i) {
            const int uniform_location = program->uniform_locations[i];
            if (uniform_location != -1) {
                gl_uniform_buffer_t * const ub = context->ubuffers[i];
                if (ub && ub->data.ptr && (ub->crc != program->uniform_crcs[i])) {
                    program->uniform_crcs[i] = ub->crc;
                    gl_upload_uniform_funcs[i](uniform_location, ub);
                }
            }
        }
    }
}

/*
=======================================
gl_render_target_t
=======================================
*/

typedef struct gl_render_target_t {
    gl_resource_t resource;
    gl_texture_t * color_buffers[rhi_max_render_target_color_buffers];
    int color_buffer_ids[rhi_max_render_target_color_buffers];
    GLuint framebuffer;
} gl_render_target_t;

static void glc_discard_render_target_data(gl_context_t * const context, const GLenum target, const gl_render_target_t * const render_target) {
    GLenum attachments[rhi_max_render_target_color_buffers + 1];
    int num_attachments = 0;

    for (int i = 0; i < ARRAY_SIZE(render_target->color_buffers); ++i) {
        if (render_target->color_buffers[i]) {
            ASSERT(num_attachments < rhi_max_render_target_color_buffers);
            attachments[num_attachments++] = GL_COLOR_ATTACHMENT0 + i;
        }
    }

    if (num_attachments > 0) {
        glc_discard_framebuffer(
            context,
            target,
            render_target->framebuffer,
            render_target->resource.resource.instance_id,
            num_attachments,
            attachments);
    }
}

static void gl_destroy_render_target(gl_resource_t * const resource, const char * const tag) {
    BIND_THREAD_CONTEXT(context);
    gl_render_target_t * const render_target = (gl_render_target_t *)resource;
    gl_device_and_context_t * const device = (gl_device_and_context_t *)render_target->resource.outer;

    // render targets can't be shared across devices
    ASSERT(!device || (&device->context == context));

    glc_delete_framebuffers(context, 1, &render_target->framebuffer);

    for (int i = 0; i < ARRAY_SIZE(render_target->color_buffers); ++i) {
        gl_texture_t * const color_buffer = render_target->color_buffers[i];
        if (color_buffer) {
            rhi_release(&color_buffer->resource.resource, tag);
        }
    }

    if (device && (device->active_render_target == render_target)) {
        device->active_render_target = NULL;
    }
}

static const gl_resource_vtable_t gl_render_target_vtable_t = {
    .destroy = gl_destroy_render_target};

static gl_render_target_t * gl_create_render_target(gl_context_t * const context, rhi_resource_t * const outer, const char * const tag) {
    gl_render_target_t * const render_target = (gl_render_target_t *)gl_create_resource(sizeof(gl_render_target_t), outer, &gl_render_target_vtable_t, tag);
    glc_gen_framebuffers(context, 1, &render_target->framebuffer);
    return render_target;
}

/*
===============================================================================
Context helpers
===============================================================================
*/

/*
=======================================
glc_set_all_state
glc_set_rasterizer_state
glc_set_depth_stencil_state
glc_set_blend_state

These big workhorse functions apply the delta's between
two state descriptors to the open gl pipeline.

At the end of these calls the open gl pipeline state will match
the specified descriptions
=======================================
*/

#define RESOURCES_NOT_EQUAL(_a, _b) rhi_resources_not_equal((_a) ? &(_a)->resource.resource : NULL, (_b) ? &(_b)->resource.resource : NULL, TOKENPASTE(_b, _id))

/*
=======================================
glc_set_rasterizer_state
=======================================
*/

static void glc_set_rasterizer_state(gl_context_t * const context, const gl_rasterizer_state_t * const rs) {
    if (RESOURCES_NOT_EQUAL(rs, context->active_rs)) {
        if (rs->desc.cull_mode != context->rs_desc.cull_mode) {
            if (context->rs_desc.cull_mode == rhi_cull_face_none) {
                glEnable(GL_CULL_FACE);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            } else if (rs->desc.cull_mode == rhi_cull_face_none) {
                glDisable(GL_CULL_FACE);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }

            if ((rs->desc.cull_mode != rhi_cull_face_none) && (rs->desc.cull_mode != context->cull_mode)) {
                glCullFace((rs->desc.cull_mode == rhi_cull_face_front) ? GL_FRONT : GL_BACK);
                CHECK_GL_ERRORS();
                context->cull_mode = rs->desc.cull_mode;
                ++context->counters.num_api_calls;
            }
        }

        if (rs->desc.scissor_test != context->rs_desc.scissor_test) {
            if (rs->desc.scissor_test) {
                glEnable(GL_SCISSOR_TEST);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            } else {
                glDisable(GL_SCISSOR_TEST);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
        }

        context->rs_desc = rs->desc;
        context->active_rs = rs;
        context->active_rs_id = rs->resource.resource.instance_id;
    }
}

/*
=======================================
glc_set_depth_stencil_state
=======================================
*/

static void glc_set_depth_stencil_state(gl_context_t * const context, const gl_depth_stencil_state_t * const dss, uint32_t stencil_ref) {
    if (RESOURCES_NOT_EQUAL(dss, context->active_dss)) {
        if (dss->desc.depth_test != context->dss_desc.depth_test) {
            if (dss->desc.depth_test) {
                glEnable(GL_DEPTH_TEST);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            } else {
                glDisable(GL_DEPTH_TEST);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
            context->dss_desc.depth_test = dss->desc.depth_test;
        }

        if (dss->desc.depth_write_mask != context->dss_desc.depth_write_mask) {
            glDepthMask(dss->desc.depth_write_mask ? GL_TRUE : GL_FALSE);
            CHECK_GL_ERRORS();
            context->dss_desc.depth_write_mask = dss->desc.depth_write_mask;
            ++context->counters.num_api_calls;
        }

        if (dss->desc.depth_test) {
            if (dss->desc.depth_test_func != context->dss_desc.depth_test_func) {
                glDepthFunc(get_gl_compare_func(dss->desc.depth_test_func));
                CHECK_GL_ERRORS();
                context->dss_desc.depth_test_func = dss->desc.depth_test_func;
                ++context->counters.num_api_calls;
            }
        }

        if (dss->desc.stencil_test != context->dss_desc.stencil_test) {
            if (dss->desc.stencil_test) {
                glEnable(GL_STENCIL_TEST);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            } else {
                glDisable(GL_STENCIL_TEST);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
            context->dss_desc.stencil_test = dss->desc.stencil_test;
        }

        if (dss->desc.stencil_test) {
            if (dss->desc.stencil_write_mask != context->dss_desc.stencil_write_mask) {
                glStencilMask(dss->desc.stencil_write_mask);
                CHECK_GL_ERRORS();
                context->dss_desc.stencil_write_mask = dss->desc.stencil_write_mask;
                ++context->counters.num_api_calls;
            }

            if ((dss->desc.stencil_read_mask != context->dss_desc.stencil_read_mask) || (stencil_ref != context->stencil_ref)) {
                if (dss->desc.stencil_front.stencil_test_func == dss->desc.stencil_back.stencil_test_func) {
                    GL_CALL(glStencilFuncSeparate(GL_FRONT_AND_BACK, get_gl_compare_func(dss->desc.stencil_front.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                    CHECK_GL_ERRORS();
                    ++context->counters.num_api_calls;
                } else {
                    GL_CALL(glStencilFuncSeparate(GL_FRONT, get_gl_compare_func(dss->desc.stencil_front.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                    CHECK_GL_ERRORS();
                    ++context->counters.num_api_calls;
                    GL_CALL(glStencilFuncSeparate(GL_BACK, get_gl_compare_func(dss->desc.stencil_back.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                    CHECK_GL_ERRORS();
                    ++context->counters.num_api_calls;
                }

                context->stencil_ref = stencil_ref;
                context->dss_desc.stencil_read_mask = dss->desc.stencil_read_mask;
                context->dss_desc.stencil_front.stencil_test_func = dss->desc.stencil_front.stencil_test_func;
                context->dss_desc.stencil_back.stencil_test_func = dss->desc.stencil_back.stencil_test_func;

            } else if (dss->desc.stencil_front.stencil_test_func != context->dss_desc.stencil_front.stencil_test_func) {
                if (dss->desc.stencil_back.stencil_test_func != context->dss_desc.stencil_back.stencil_test_func) {
                    if (dss->desc.stencil_front.stencil_test_func == dss->desc.stencil_back.stencil_test_func) {
                        GL_CALL(glStencilFuncSeparate(GL_FRONT_AND_BACK, get_gl_compare_func(dss->desc.stencil_front.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                        CHECK_GL_ERRORS();
                        ++context->counters.num_api_calls;
                    } else {
                        GL_CALL(glStencilFuncSeparate(GL_BACK, get_gl_compare_func(dss->desc.stencil_back.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                        CHECK_GL_ERRORS();
                        ++context->counters.num_api_calls;
                    }
                    context->dss_desc.stencil_back.stencil_test_func = dss->desc.stencil_back.stencil_test_func;
                } else {
                    GL_CALL(glStencilFuncSeparate(GL_FRONT, get_gl_compare_func(dss->desc.stencil_front.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                    CHECK_GL_ERRORS();
                    ++context->counters.num_api_calls;
                }

                context->dss_desc.stencil_front.stencil_test_func = dss->desc.stencil_front.stencil_test_func;

            } else if (dss->desc.stencil_back.stencil_test_func != context->dss_desc.stencil_back.stencil_test_func) {
                GL_CALL(glStencilFuncSeparate(GL_BACK, get_gl_compare_func(dss->desc.stencil_back.stencil_test_func), stencil_ref, dss->desc.stencil_read_mask));
                CHECK_GL_ERRORS();
                context->dss_desc.stencil_back.stencil_test_func = dss->desc.stencil_back.stencil_test_func;
                ++context->counters.num_api_calls;
            }

            bool test_back = true;

            if ((dss->desc.stencil_front.stencil_fail_op != context->dss_desc.stencil_front.stencil_fail_op) || (dss->desc.stencil_front.depth_fail_op != context->dss_desc.stencil_front.depth_fail_op) || (dss->desc.stencil_front.stencil_depth_pass_op != context->dss_desc.stencil_front.stencil_depth_pass_op)) {
                if (rhi_stencil_op_desc_equals(&dss->desc.stencil_front, &dss->desc.stencil_back)) {
                    GL_CALL(glStencilOpSeparate(GL_FRONT_AND_BACK, get_gl_stencil_op(dss->desc.stencil_front.stencil_fail_op), get_gl_stencil_op(dss->desc.stencil_front.depth_fail_op), get_gl_stencil_op(dss->desc.stencil_front.stencil_depth_pass_op)));
                    context->dss_desc.stencil_back = dss->desc.stencil_front;
                    test_back = false;
                } else {
                    GL_CALL(glStencilOpSeparate(GL_FRONT, get_gl_stencil_op(dss->desc.stencil_front.stencil_fail_op), get_gl_stencil_op(dss->desc.stencil_front.depth_fail_op), get_gl_stencil_op(dss->desc.stencil_front.stencil_depth_pass_op)));
                }

                CHECK_GL_ERRORS();
                context->dss_desc.stencil_front = dss->desc.stencil_front;
                ++context->counters.num_api_calls;
            }
            if (test_back) {
                if ((dss->desc.stencil_back.stencil_fail_op != context->dss_desc.stencil_back.stencil_fail_op) || (dss->desc.stencil_back.depth_fail_op != context->dss_desc.stencil_back.depth_fail_op) || (dss->desc.stencil_back.stencil_depth_pass_op != context->dss_desc.stencil_back.stencil_depth_pass_op)) {
                    GL_CALL(glStencilOpSeparate(GL_BACK, get_gl_stencil_op(dss->desc.stencil_back.stencil_fail_op), get_gl_stencil_op(dss->desc.stencil_back.depth_fail_op), get_gl_stencil_op(dss->desc.stencil_back.stencil_depth_pass_op)));
                    CHECK_GL_ERRORS();
                    context->dss_desc.stencil_back = dss->desc.stencil_back;
                    ++context->counters.num_api_calls;
                }
            }
        }

        context->active_dss = dss;
        context->active_dss_id = dss->resource.resource.instance_id;
    }
}

/*
=======================================
glc_set_blend_state
=======================================
*/

static void glc_set_blend_state(gl_context_t * const context, const gl_blend_state_t * const bs) {
    if (RESOURCES_NOT_EQUAL(bs, context->active_bs)) {
        if (bs->desc.blend_enable != context->bs_desc.blend_enable) {
            if (bs->desc.blend_enable) {
                glEnable(GL_BLEND);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            } else {
                glDisable(GL_BLEND);
                CHECK_GL_ERRORS();
                ++context->counters.num_api_calls;
            }
            context->bs_desc.blend_enable = bs->desc.blend_enable;
        }
        if (bs->desc.blend_enable) {
            if ((bs->desc.src_blend != context->bs_desc.src_blend) || (bs->desc.dst_blend != context->bs_desc.dst_blend) || (bs->desc.src_blend_alpha != context->bs_desc.src_blend_alpha) || (bs->desc.dst_blend_alpha != context->bs_desc.dst_blend_alpha)) {
                GL_CALL(glBlendFuncSeparate(
                    get_gl_blend_factor(bs->desc.src_blend),
                    get_gl_blend_factor(bs->desc.dst_blend),
                    get_gl_blend_factor(bs->desc.src_blend_alpha),
                    get_gl_blend_factor(bs->desc.dst_blend_alpha)));
                CHECK_GL_ERRORS();
                context->bs_desc.src_blend = bs->desc.src_blend;
                context->bs_desc.dst_blend = bs->desc.dst_blend;
                context->bs_desc.src_blend_alpha = bs->desc.src_blend_alpha;
                context->bs_desc.dst_blend_alpha = bs->desc.dst_blend_alpha;
                ++context->counters.num_api_calls;
            }
            if ((bs->desc.blend_op != context->bs_desc.blend_op) || (bs->desc.blend_op_alpha != context->bs_desc.blend_op_alpha)) {
                GL_CALL(glBlendEquationSeparate(get_gl_blend_op(bs->desc.blend_op), get_gl_blend_op(bs->desc.blend_op_alpha)));
                CHECK_GL_ERRORS();
                context->bs_desc.blend_op = bs->desc.blend_op;
                context->bs_desc.blend_op_alpha = bs->desc.blend_op_alpha;
                ++context->counters.num_api_calls;
            }
            if ((bs->desc.blend_color[0] != context->bs_desc.blend_color[0]) || (bs->desc.blend_color[1] != context->bs_desc.blend_color[1]) || (bs->desc.blend_color[2] != context->bs_desc.blend_color[2]) || (bs->desc.blend_color[3] != context->bs_desc.blend_color[3])) {
                if ((context->bs_desc.src_blend == rhi_blend_constant_color) || (context->bs_desc.src_blend == rhi_blend_constant_alpha) || (context->bs_desc.dst_blend == rhi_blend_constant_color) || (context->bs_desc.src_blend == rhi_blend_constant_alpha) || (context->bs_desc.src_blend_alpha == rhi_blend_constant_color) || (context->bs_desc.src_blend_alpha == rhi_blend_constant_alpha) || (context->bs_desc.dst_blend_alpha == rhi_blend_constant_color) || (context->bs_desc.dst_blend_alpha == rhi_blend_constant_alpha)) {
                    GL_CALL(glBlendColor(
                        bs->desc.blend_color[0],
                        bs->desc.blend_color[1],
                        bs->desc.blend_color[2],
                        bs->desc.blend_color[3]));
                    CHECK_GL_ERRORS();
                    context->bs_desc.blend_color[0] = bs->desc.blend_color[0];
                    context->bs_desc.blend_color[1] = bs->desc.blend_color[1];
                    context->bs_desc.blend_color[2] = bs->desc.blend_color[2];
                    context->bs_desc.blend_color[3] = bs->desc.blend_color[3];
                    ++context->counters.num_api_calls;
                }
            }
        }
        if (bs->desc.color_write_mask != context->bs_desc.color_write_mask) {
            glColorMask(
                (bs->desc.color_write_mask & 1) ? GL_TRUE : GL_FALSE,
                (bs->desc.color_write_mask & 2) ? GL_TRUE : GL_FALSE,
                (bs->desc.color_write_mask & 4) ? GL_TRUE : GL_FALSE,
                (bs->desc.color_write_mask & 8) ? GL_TRUE : GL_FALSE);
            CHECK_GL_ERRORS();
            context->bs_desc.color_write_mask = bs->desc.color_write_mask;
            ++context->counters.num_api_calls;
        }

        context->active_bs = bs;
        context->active_bs_id = bs->resource.resource.instance_id;
    }
}

/*
===============================================================================
OpenGL RHI Device API
===============================================================================
*/

#define D ((gl_device_and_context_t *)device)
#define DC (&D->context)

/*
=======================================
gld_get_caps
=======================================
*/

static rhi_device_caps_t gld_get_caps(rhi_device_t * const device) {
    return DC->caps;
}

/*
=======================================
gld_thread_make_device_current
=======================================
*/

static void gld_thread_make_device_current(rhi_device_t * const device) {
    glc_make_current(DC);
}

/*
=======================================
gld_thread_device_done_current
=======================================
*/

static void gld_thread_device_done_current(rhi_device_t * const device) {
    glc_make_current(NULL);
}

/*
=======================================
gld_upload_mesh_channel_data
=======================================
*/

static void gld_upload_mesh_channel_data(rhi_device_t * const device, rhi_mesh_t * const mesh, const int channel_index, const int first_elem, const int num_elems, const void * const data) {
    gl_mesh_t * const gl_mesh = (gl_mesh_t *)mesh;
    ASSERT(!gl_mesh->shared_verts);

    ASSERT(channel_index >= 0);
    ASSERT(channel_index < gl_mesh->layout->desc.num_channels);

    const GLuint vertex_buffer = gl_mesh->vertex_buffers[channel_index];
    const int vertex_buffer_id = gl_mesh->vertex_buffer_ids[channel_index];
    int * const vertex_buffer_size = &gl_mesh->vertex_buffer_sizes[channel_index];
    const int stride = gl_mesh->layout->desc.channels[channel_index].stride;
    const int len = num_elems * stride;
    ASSERT(len > 0);

    if (first_elem == 0) {
        *vertex_buffer_size = len;
        glc_dynamic_buffer_data(DC, GL_ARRAY_BUFFER, vertex_buffer, vertex_buffer_id, len, data);
    } else {
        ASSERT(((first_elem * stride) + len) <= *vertex_buffer_size);
        glc_buffer_sub_data(DC, GL_ARRAY_BUFFER, vertex_buffer, vertex_buffer_id, first_elem * stride, len, data);
    }

    DC->counters.num_upload_verts += num_elems;
    DC->counters.upload_vert_size += len;
}

/*
=======================================
gld_upload_mesh_indices
=======================================
*/

static void gld_upload_mesh_indices(rhi_device_t * const device, rhi_mesh_t * const mesh, const int first_index, const int num_indices, const void * const indices) {
    gl_mesh_t * const gl_mesh = (gl_mesh_t *)mesh;

    ASSERT(gl_mesh->layout->desc.indices->usage == rhi_usage_dynamic);

#if defined(GL_VAOS) && !defined(GL_DSA)
    // if we have to bind to the GL_ELEMENT_ARRAY_BUFFER bind point then
    // avoid wiping the binding on any currently bound VAO.
    glc_bind_vertex_array(DC, 0, 0);
#endif

    const int stride = gl_mesh->layout->index_size;
    const int len = stride * num_indices;

    ASSERT(len > 0);

    if ((first_index == 0) && (num_indices > gl_mesh->num_indices)) {
        gl_mesh->num_indices = num_indices;
        glc_dynamic_buffer_data(DC, GL_ELEMENT_ARRAY_BUFFER, gl_mesh->index_buffer, gl_mesh->index_buffer_id, len, indices);
    } else {
        glc_buffer_sub_data(DC, GL_ELEMENT_ARRAY_BUFFER, gl_mesh->index_buffer, gl_mesh->index_buffer_id, first_index * stride, len, indices);
    }

    DC->counters.num_upload_indices += num_indices;
    DC->counters.upload_index_size += len;
}

/*
=======================================
gld_upload_uniform_data
=======================================
*/

static void gld_upload_uniform_data(rhi_device_t * const device, rhi_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs) {
    gl_uniform_buffer_t * const gl_ub = (gl_uniform_buffer_t *)uniform_buffer;

    ++DC->uniform_updates;

    if (gl_ub->dynamic) {
        if ((ofs == 0) && (data.size > gl_ub->data.size)) {
            gl_ub->data.ptr = gl_realloc(gl_ub->data.ptr, data.size, MALLOC_TAG);
            gl_ub->data.size = data.size;
            memcpy(gl_ub->data.ptr, data.ptr, data.size);
            gl_ub->crc = crc_32(data.ptr, data.size);
        } else {
            ASSERT((ofs + data.size) <= gl_ub->data.size);
            memcpy(gl_ub->data.byte_ptr + ofs, data.ptr, data.size);
            gl_ub->crc = crc_32(gl_ub->data.ptr, gl_ub->data.size);
        }
    } else {
        ASSERT(ofs == 0);
        if (gl_ub->data.size != data.size) {
            gl_ub->data.size = data.size;
            gl_ub->data.ptr = gl_realloc(gl_ub->data.ptr, data.size, MALLOC_TAG);
        }
        memcpy(gl_ub->data.ptr, data.ptr, data.size);
        gl_ub->crc = crc_32(data.ptr, data.size);
    }

    ++DC->counters.num_upload_uniforms;
    DC->counters.upload_uniform_size += (uint32_t)data.size;
}

/*
=======================================
gld_upload_texture
=======================================
*/

static void gld_upload_texture(rhi_device_t * const device, rhi_texture_t * const texture, const image_mips_t * const mipmaps) {
    const gl_texture_t * const gl_texture = (gl_texture_t *)texture;
    const rhi_pixel_format_e pixel_format = gl_texture->pixel_format;

    ASSERT(pixel_format >= 0);
    ASSERT(pixel_format < rhi_num_pixels_formats);

    const gl_pixel_format_t gl_pixel_format = pixel_formats[pixel_format];

    for (int i = 0; i < mipmaps->num_levels; ++i) {
        const image_t image = mipmaps->levels[i];
        if (image.data) {
            ASSERT(image.depth == 1);
            ASSERT(image.spitch > 0);

            if ((pixel_format >= rhi_pixel_format_r8_unorm)
                && (pixel_format <= rhi_pixel_format_rgba8_unorm)) {
                glc_texture_subimage_2d(
                    DC,
                    (texture_subimage_2d_args_t){
                        .tunit = GL_TEXTURE0,
                        .target = gl_texture->target,
                        .texture = gl_texture->texture,
                        .id = gl_texture->resource.resource.instance_id,
                        .level = i,
                        .xoffset = 0,
                        .yoffset = 0,
                        .w = image.width,
                        .h = image.height,
                        .format = gl_pixel_format.format,
                        .type = gl_pixel_format.type,
                        .pixels = image.data,
                    });
            } else {
#ifdef GL_CORE
                ASSERT((pixel_format == rhi_pixel_format_bc1_unorm) || (pixel_format == rhi_pixel_format_bc3_unorm));
                glc_compressed_texture_subimage_2d(
                    DC,
                    GL_TEXTURE0,
                    gl_texture->target,
                    gl_texture->texture,
                    gl_texture->resource.resource.instance_id,
                    i,
                    0,
                    0,
                    image.width,
                    image.height,
                    gl_pixel_format.format,
                    image.spitch,
                    image.data);
#else // GL_ES
                ASSERT(pixel_format == rhi_pixel_format_etc1);
                glc_compressed_texture_image_2d(
                    DC,
                    GL_TEXTURE0,
                    gl_texture->target,
                    gl_texture->texture,
                    gl_texture->resource.resource.instance_id,
                    i,
                    image.width,
                    image.height,
                    gl_pixel_format.format,
                    0,
                    image.spitch,
                    image.data);
#endif
            }

            ++DC->counters.num_upload_textures;
            DC->counters.upload_texture_size += image.spitch;
        }
    }
}

static void gld_upload_sub_texture(rhi_device_t * const device, rhi_texture_t * const texture, const image_mips_t * const mipmaps) {
    const gl_texture_t * const gl_texture = (gl_texture_t *)texture;
    const rhi_pixel_format_e pixel_format = gl_texture->pixel_format;

    ASSERT(pixel_format >= 0);
    ASSERT_MSG((pixel_format >= rhi_pixel_format_r8_unorm) && (pixel_format <= rhi_pixel_format_rgba8_unorm), "Cannot upload a sub image/texture with a compressed format -- support is not in currently");

    const gl_pixel_format_t gl_pixel_format = pixel_formats[pixel_format];

    for (int i = 0; i < mipmaps->num_levels; ++i) {
        const image_t image = mipmaps->levels[i];
        if (image.data) {
            ASSERT(image.depth == 1);
            ASSERT(image.spitch > 0);

            glc_texture_subimage_2d(
                DC,
                (texture_subimage_2d_args_t){
                    .tunit = GL_TEXTURE0,
                    .target = gl_texture->target,
                    .texture = gl_texture->texture,
                    .id = gl_texture->resource.resource.instance_id,
                    .level = i,
                    .xoffset = image.x,
                    .yoffset = image.y,
                    .w = image.width,
                    .h = image.height,
                    .format = gl_pixel_format.format,
                    .type = gl_pixel_format.type,
                    .pixels = image.data,
                });
        }
    }
}

/*
=======================================
gld_set_display_size
=======================================
*/

static void gld_set_display_size(rhi_device_t * const device, const int w, const int h) {
    D->display_width = w;
    D->display_height = h;
}

/*
=======================================
gld_set_viewport
=======================================
*/

static void gld_set_viewport(rhi_device_t * const device, const int x0, const int y0, const int x1, const int y1) {
    if (D->active_render_target) {
        gl_texture_t * const color_buffer = D->active_render_target->color_buffers[0];
        ASSERT(color_buffer);
        glc_viewport(DC, x0, color_buffer->height - y1, x1 - x0, y1 - y0);
    } else {
        glc_viewport(DC, x0, D->display_height - y1, x1 - x0, y1 - y0);
    }
}

/*
=======================================
gld_set_scissor_rect
=======================================
*/

static void gld_set_scissor_rect(rhi_device_t * const device, const int x0, const int y0, const int x1, const int y1) {
    if (D->active_render_target) {
        gl_texture_t * const color_buffer = D->active_render_target->color_buffers[0];
        ASSERT(color_buffer);
        glc_scissor(DC, x0, color_buffer->height - y1, x1 - x0, y1 - y0);
    } else {
        glc_scissor(DC, x0, D->display_height - y1, x1 - x0, y1 - y0);
    }
}

/*
=======================================
gld_set_program
=======================================
*/

static void gld_set_program(rhi_device_t * const device, rhi_program_t * const program) {
    gl_program_t * const gl_program = (gl_program_t *)program;
    glc_use_program(DC, gl_program);
}

/*
=======================================
gld_set_render_state
=======================================
*/

static void gld_set_render_state(rhi_device_t * const device, rhi_rasterizer_state_t * const rs, rhi_depth_stencil_state_t * const dss, rhi_blend_state_t * const bs, const uint32_t stencil_ref) {
    glc_set_rasterizer_state(DC, (gl_rasterizer_state_t *)rs);
    glc_set_depth_stencil_state(DC, (gl_depth_stencil_state_t *)dss, stencil_ref);
    glc_set_blend_state(DC, (gl_blend_state_t *)bs);
}

/*
=======================================
gld_set_render_target_color_buffer
=======================================
*/

static void gld_set_render_target_color_buffer(rhi_device_t * const device, rhi_render_target_t * const render_target, rhi_texture_t * const color_buffer, const int index) {
    ASSERT(index >= 0);
    ASSERT(index < rhi_max_render_target_color_buffers);

    gl_render_target_t * const gl_render_target = (gl_render_target_t *)render_target;
    const GLuint framebuffer = gl_render_target->framebuffer;
    const int framebuffer_id = gl_render_target->resource.resource.instance_id;

    gl_texture_t * const dst_color_buffer = gl_render_target->color_buffers[index];
    const int dst_color_buffer_id = gl_render_target->color_buffer_ids[index];
    gl_texture_t * const src_color_buffer = (gl_texture_t *)color_buffer;
    const int src_color_buffer_id = src_color_buffer ? src_color_buffer->resource.resource.instance_id : 0;

    if (src_color_buffer_id != dst_color_buffer_id) {
        gl_render_target->color_buffers[index] = src_color_buffer;
        gl_render_target->color_buffer_ids[index] = src_color_buffer_id;

        if (dst_color_buffer) {
            rhi_release(&dst_color_buffer->resource.resource, MALLOC_TAG);
        }

        if (src_color_buffer) {
            rhi_add_ref(&src_color_buffer->resource.resource);
            glc_framebuffer_texture_2d(DC, GL_FRAMEBUFFER, framebuffer, framebuffer_id, GL_COLOR_ATTACHMENT0 + index, src_color_buffer->texture, 0);
        } else {
            glc_framebuffer_texture_2d(DC, GL_FRAMEBUFFER, framebuffer, framebuffer_id, GL_NONE, 0, 0);
        }
    }

#ifdef GL_CORE
    // set draw buffers
    GLenum drawbuffers[rhi_max_render_target_color_buffers];

    for (int i = 0; i < ARRAY_SIZE(drawbuffers); ++i) {
        drawbuffers[i] = gl_render_target->color_buffers[i] ? GL_COLOR_ATTACHMENT0 + index : GL_NONE;
    }

    glc_framebuffer_drawbuffers(DC, framebuffer, framebuffer_id, ARRAY_SIZE(drawbuffers), drawbuffers);
#endif
}

/*
=======================================
gld_discard_render_target_data
=======================================
*/

static void gld_discard_render_target_data(rhi_device_t * const device, rhi_render_target_t * const render_target) {
    gl_render_target_t * const gl_render_target = (gl_render_target_t *)render_target;
    glc_discard_render_target_data(DC, GL_FRAMEBUFFER, gl_render_target);
}

/*
=======================================
gld_set_render_target
=======================================
*/

static void gld_set_render_target(rhi_device_t * const device, rhi_render_target_t * const render_target) {
    gl_render_target_t * const gl_render_target = (gl_render_target_t *)render_target;
    D->active_render_target = gl_render_target;

    if (render_target) {
        ASSERT(glc_check_framebuffer_status(DC, GL_FRAMEBUFFER, gl_render_target->framebuffer, gl_render_target->resource.resource.instance_id));
        glc_bind_framebuffer(DC, GL_FRAMEBUFFER, gl_render_target->framebuffer, gl_render_target->resource.resource.instance_id);
    } else {
        glc_bind_framebuffer(DC, GL_FRAMEBUFFER, 0, 0);
    }
}

/*
=======================================
gld_set_uniform_bindings
=======================================
*/

static void gld_set_uniform_bindings(rhi_device_t * const device, rhi_uniform_buffer_t * const uniform_buffer, const int index) {
    ASSERT(uniform_buffer);
    ASSERT(index >= 0);
    ASSERT(index < rhi_program_num_uniforms);
    gl_uniform_buffer_t * const gl_uniform_buffer = (gl_uniform_buffer_t *)uniform_buffer;
    DC->ubuffers[index] = gl_uniform_buffer;
    ++DC->uniform_updates;
}

/*
=======================================
gld_set_texture_sampler_state_indirect
=======================================
*/

static inline void gld_set_texture_sampler_state(rhi_device_t * const device, rhi_texture_t * const texture, const rhi_sampler_state_desc_t sampler_state) {
    gl_texture_t * const gl_texture = (gl_texture_t *)texture;

#ifdef GL_CORE
    glc_sampler_parameter_i(DC, gl_texture->sampler, GL_TEXTURE_MIN_FILTER, get_gl_min_filter(sampler_state.min_filter));
    glc_sampler_parameter_i(DC, gl_texture->sampler, GL_TEXTURE_MAG_FILTER, get_gl_max_filter(sampler_state.max_filter));
    glc_sampler_parameter_i(DC, gl_texture->sampler, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(sampler_state.u_wrap_mode));
    glc_sampler_parameter_i(DC, gl_texture->sampler, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(sampler_state.v_wrap_mode));
    glc_sampler_parameter_i(DC, gl_texture->sampler, GL_TEXTURE_WRAP_R, get_gl_wrap_mode(sampler_state.w_wrap_mode));
    glc_sampler_parameter_i(DC, gl_texture->sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler_state.max_anisotropy ? sampler_state.max_anisotropy : DC->caps.max_anisotropy);
#else // GL_ES
    const GLuint t = DC->textures[0];
    const int tid = DC->texture_ids[0];

    glc_tex_parameter_i(DC, GL_TEXTURE0, gl_texture->target, gl_texture->texture, gl_texture->resource.resource.instance_id, GL_TEXTURE_MIN_FILTER, get_gl_min_filter(sampler_state.min_filter));
    glc_tex_parameter_i(DC, GL_TEXTURE0, gl_texture->target, gl_texture->texture, gl_texture->resource.resource.instance_id, GL_TEXTURE_MAG_FILTER, get_gl_max_filter(sampler_state.max_filter));
    glc_tex_parameter_i(DC, GL_TEXTURE0, gl_texture->target, gl_texture->texture, gl_texture->resource.resource.instance_id, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(sampler_state.u_wrap_mode));
    glc_tex_parameter_i(DC, GL_TEXTURE0, gl_texture->target, gl_texture->texture, gl_texture->resource.resource.instance_id, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(sampler_state.v_wrap_mode));

    if (tid) {
        // restore prior texture bindings
        glc_bind_texture(DC, GL_TEXTURE0, GL_TEXTURE_2D, t, tid);
    }
#endif
}

/*
=======================================
gld_set_texture_bindings_indirect
=======================================
*/

static void gld_set_texture_bindings_indirect(rhi_device_t * const device, const rhi_texture_bindings_indirect_t * const texture_bindings) {
    for (int i = 0; i < texture_bindings->num_textures; ++i) {
        rhi_texture_t * const * const src = texture_bindings->textures[i];
        {
            const int dst = DC->texture_ids[i];
            const gl_texture_t * const tex = (gl_texture_t *)(src ? *src : NULL);
            if (tex && (tex->resource.resource.instance_id != dst)) {
                // bound texture change
                glc_bind_texture(DC, GL_TEXTURE0 + i, tex->target, tex->texture, tex->resource.resource.instance_id);
#ifdef GL_CORE
                glf.glBindSampler(i, tex->sampler);
#endif
                CHECK_GL_ERRORS();
                ++DC->counters.num_api_calls;
            }
        }
    }
}

/*
=======================================
gld_clear_render_target_color_buffers
=======================================
*/

static void gld_clear_render_target_color_buffers(rhi_device_t * const device, rhi_render_target_t * const render_target, const uint32_t color) {
    const float floats_rgba[4] = {
        (color & 0xff) / 255.f,
        ((color >> 8) & 0xff) / 255.f,
        ((color >> 16) & 0xff) / 255.f,
        ((color >> 24) & 0xff) / 255.f};

    const uint8_t old_color_mask = DC->bs_desc.color_write_mask;
    const bool old_scissor_test = DC->rs_desc.scissor_test;

    glc_color_mask(DC, rhi_color_write_mask_all);
    glc_enable_scissor(DC, false);

    gl_render_target_t * gl_render_target = (gl_render_target_t *)render_target;
    const GLuint framebuffer = gl_render_target->framebuffer;
#ifdef GL_DSA
    for (int i = 0; i < ARRAY_SIZE(gl_render_target->color_buffer_ids); ++i) {
        if (gl_render_target->color_buffer_ids[i]) {
            glf.glClearNamedFramebufferfv(framebuffer, GL_COLOR, i, floats_rgba);
            CHECK_GL_ERRORS();
        }
    }

#else
#ifdef GL_CORE
    glc_bind_framebuffer(DC, GL_DRAW_FRAMEBUFFER, framebuffer, gl_render_target->resource.resource.instance_id);
#else // GL_ES
    glc_bind_framebuffer(DC, GL_FRAMEBUFFER, framebuffer, gl_render_target->resource.resource.instance_id);
#endif
    glClearColor(floats_rgba[0], floats_rgba[1], floats_rgba[2], floats_rgba[3]);
    CHECK_GL_ERRORS();
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERRORS();

    // restore active render target
    if (D->active_render_target) {
#ifdef GL_CORE
        glc_bind_framebuffer(DC, GL_DRAW_FRAMEBUFFER, D->active_render_target->framebuffer, D->active_render_target->resource.resource.instance_id);
#else // GL_ES
        glc_bind_framebuffer(DC, GL_FRAMEBUFFER, D->active_render_target->framebuffer, D->active_render_target->resource.resource.instance_id);
#endif
    }
#endif
    glc_color_mask(DC, old_color_mask);
    glc_enable_scissor(DC, old_scissor_test);
}

/*
=======================================
gld_copy_render_target_buffers
=======================================
*/

static void glc_framebuffer_blit(gl_context_t * const context, const gl_render_target_t * const src, const GLuint dst, const int dst_id) {
    ASSERT(context->active_rs);
    ASSERT(context->active_dss);
    ASSERT(context->active_bs);

    ASSERT(src);
    ASSERT(src->framebuffer != dst);

    const bool old_scissor_test = context->rs_desc.scissor_test;
    glc_enable_scissor(context, false);

#ifdef GL_FRAMEBUFFER_BLIT_PROGRAM
    const bool old_depth_mask = context->dss_desc.depth_write_mask;
    const bool old_stencil_mask = context->dss_desc.stencil_write_mask;
    const uint8_t old_color_mask = context->bs_desc.color_write_mask;
    const rhi_cull_face_e old_cull_mode = context->rs_desc.cull_mode;
    const bool old_blend = context->bs_desc.blend_enable;

    glc_depth_mask(context, false);
    glc_stencil_mask(context, 0);
    glc_color_mask(context, rhi_color_write_mask_all);
    glc_cull_mode(context, rhi_cull_face_none);
    glc_enable_blend(context, false);

#if defined(GL_CORE)
    extern GLuint glcore_default_vertex_array_id;
#ifdef GL_VAOS
    glc_bind_vertex_array(context, glcore_default_vertex_array_id, 0);
#endif
#elif defined(GL_VAOS) // GL_ES
    glc_bind_vertex_array(context, 0, 0);
#endif

#if defined(GL_DSA) && defined(GL_VAOS)
    glf.glVertexArrayVertexBuffer(glcore_default_vertex_array_id, 0, gl_framebuffer_blit_program.vb, 0, 8);
    CHECK_GL_ERRORS();
#elif defined(GL_VAOS)
    glc_bind_buffer(context, GL_ARRAY_BUFFER, gl_framebuffer_blit_program.vb, gl_framebuffer_blit_program.vbid, true);
#else
    glc_bind_buffer(context, GL_ARRAY_BUFFER, gl_framebuffer_blit_program.vb, gl_framebuffer_blit_program.vbid, false);
#endif

#if defined(GL_DSA) && defined(GL_VAOS)
    glf.glEnableVertexArrayAttrib(glcore_default_vertex_array_id, 0);
    CHECK_GL_ERRORS();
    glf.glVertexArrayAttribFormat(glcore_default_vertex_array_id, 0, 2, GL_FLOAT, GL_FALSE, 0);
    CHECK_GL_ERRORS();
    glf.glVertexArrayAttribBinding(glcore_default_vertex_array_id, 0, 0);
    CHECK_GL_ERRORS();
    context->counters.num_api_calls += 4;
#else
    GL_CALL(glEnableVertexAttribArray(0));
    CHECK_GL_ERRORS();
    GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL));
    CHECK_GL_ERRORS();
    context->counters.num_api_calls += 2;
#endif

    // bind target framebuffer
    glc_bind_framebuffer(context, GL_FRAMEBUFFER, dst, dst_id);

    if (context->program_id != gl_framebuffer_blit_program.id) {
        context->program_id = gl_framebuffer_blit_program.id;
        GL_CALL(glUseProgram(gl_framebuffer_blit_program.program));
        CHECK_GL_ERRORS();
    }

    const GLuint old_t = context->textures[0];
    const int old_tid = context->texture_ids[0];

    glc_bind_texture(context, GL_TEXTURE0, GL_TEXTURE_2D, src->color_buffers[0]->texture, src->color_buffer_ids[0]);
#ifdef GL_CORE
    glf.glBindSampler(0, src->color_buffers[0]->sampler);
    CHECK_GL_ERRORS();
#endif

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    CHECK_GL_ERRORS();

    glc_depth_mask(context, old_depth_mask);
    glc_stencil_mask(context, old_stencil_mask);
    glc_color_mask(context, old_color_mask);
    glc_cull_mode(context, old_cull_mode);
    glc_enable_blend(context, old_blend);

    if (old_tid) {
        glc_bind_texture(context, GL_TEXTURE0, GL_TEXTURE_2D, old_t, old_tid);
    }

    context->counters.num_api_calls += 3;
#else
    const int width = src->color_buffers[0]->width;
    const int height = src->color_buffers[0]->width;

#ifdef GL_DSA
    glf.glBlitNamedFramebuffer(src->framebuffer, dst, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    CHECK_GL_ERRORS();
#else
    glc_bind_framebuffer(context, GL_READ_FRAMEBUFFER, src->framebuffer, src->resource.resource.instance_id);
    glc_bind_framebuffer(context, GL_DRAW_FRAMEBUFFER, dst, dst_id);

    glf.glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    CHECK_GL_ERRORS();
    ++context->counters.num_api_calls;
#endif
#endif

    glc_enable_scissor(context, old_scissor_test);
}

static void gld_copy_render_target_buffers(rhi_device_t * const device, rhi_render_target_t * const src, rhi_render_target_t * const dst) {
    ASSERT(dst);

    gl_render_target_t * const src_render_target = (gl_render_target_t *)src;
    gl_render_target_t * const dst_render_target = (gl_render_target_t *)dst;

    glc_framebuffer_blit(DC, src_render_target, dst_render_target->framebuffer, dst_render_target->resource.resource.instance_id);
}

/*
=======================================
gld_draw_indirect
=======================================
*/

static void gld_draw_indirect(rhi_device_t * const device, const rhi_draw_params_indirect_t * const params) {
    const GLenum draw_mode = gl_get_draw_mode(params->mode);
#ifdef GL_VAOS
    const int device_id = D->device.resource.instance_id;
    const int device_index = D->index;
#endif

    glc_use_program(DC, DC->program);
    glc_sync_program_uniform_data(DC);

    if (D->active_render_target) {
        glc_bind_framebuffer(DC, GL_FRAMEBUFFER, D->active_render_target->framebuffer, D->active_render_target->resource.resource.instance_id);
    }

    for (int i = 0; i < params->num_meshes; ++i) {
        gl_mesh_t * const mesh = (gl_mesh_t *)(*params->mesh_list[i]);
        const int idx_ofs = params->idx_ofs ? params->idx_ofs[i] : 0;
        const rhi_mesh_data_layout_desc_t layout = mesh->layout->desc;
        const int idx_size = mesh->layout->index_size;
        const int idx_type = mesh->layout->index_type;

        ASSERT(params->elm_counts);
        const int elm_count = params->elm_counts[i];

#ifdef GL_VAOS
        gl_vao_t * vao = mesh->vaos[device_index];
        if (!vao || (vao->device_id != device_id)) {
            if (vao) {
                gl_vao_unlink_from_device_and_link_for_destroy(vao, MALLOC_TAG);
            }
            vao = (gl_vao_t *)gl_alloc(sizeof(gl_vao_t), MALLOC_TAG);
            vao->ref_count = 1;
            vao->resource_id = gl_get_next_obj_id();
            vao->device_id = device_id;
            vao->device = D;
            vao->prev = NULL;
            vao->next = D->vao_used_chain;

            if (D->vao_used_chain) {
                D->vao_used_chain->prev = vao;
            }
            D->vao_used_chain = vao;

            // setup vao
            glc_gen_vertex_arrays(DC, 1, &vao->vao_id);

#ifndef GL_DSA
            glc_bind_vertex_array(DC, vao->vao_id, vao->resource_id);
#endif
#else
        {
#endif

            if (mesh->index_buffer) {
#if defined(GL_DSA) && defined(GL_VAOS)
                glf.glVertexArrayElementBuffer(vao->vao_id, mesh->index_buffer);
                CHECK_GL_ERRORS();
#elif defined(GL_VAOS)
                glc_bind_buffer(DC, GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer, mesh->index_buffer_id, true);
#else
            glc_bind_buffer(DC, GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer, mesh->index_buffer_id, false);
#endif
            }

#if !defined(GL_DSA) || !defined(GL_VAOS)
            {
                const int max_inputs = min_int(rhi_program_input_num_semantics, DC->caps.max_vertex_inputs);
                for (int j = 0; j < max_inputs; ++j) {
                    GL_CALL(glDisableVertexAttribArray(j));
                    CHECK_GL_ERRORS();
                }
            }
#endif

            for (int j = 0; j < layout.num_channels; ++j) {
                const rhi_vertex_layout_desc_t channel = layout.channels[j];
                bool bound = false;

                for (int jj = 0; jj < channel.num_elements; ++jj) {
                    const rhi_vertex_element_desc_t element = channel.elements[jj];

                    GLenum type = 0;
                    bool normalized = false;

                    switch (element.format) {
                        case rhi_vertex_element_format_float:
                            type = GL_FLOAT;
                            break;
                        case rhi_vertex_element_format_unorm_byte:
                            type = GL_UNSIGNED_BYTE;
                            normalized = true;
                            break;
                        default:
                            TRAP("OpenGL: unhandled vertex format");
                    }

                    if (!bound) {
                        ASSERT(mesh->vertex_buffers[j]);
#if defined(GL_DSA) && defined(GL_VAOS)
                        glf.glVertexArrayVertexBuffer(vao->vao_id, j, mesh->vertex_buffers[j], 0, channel.stride);
                        CHECK_GL_ERRORS();
#else
                        glc_bind_buffer(DC, GL_ARRAY_BUFFER, mesh->vertex_buffers[j], mesh->vertex_buffer_ids[j], false);
#endif
                        bound = true;
                        ++DC->counters.num_api_calls;
                    }

#if defined(GL_DSA) && defined(GL_VAOS)
                    glf.glEnableVertexArrayAttrib(vao->vao_id, (int)element.semantic);
                    CHECK_GL_ERRORS();
                    glf.glVertexArrayAttribFormat(vao->vao_id, (int)element.semantic, element.count, type, normalized ? GL_TRUE : GL_FALSE, element.offset);
                    CHECK_GL_ERRORS();
                    glf.glVertexArrayAttribBinding(vao->vao_id, (int)element.semantic, j);
                    CHECK_GL_ERRORS();
                    DC->counters.num_api_calls += 3;
#else
                    GL_CALL(glEnableVertexAttribArray((int)element.semantic));
                    CHECK_GL_ERRORS();
                    GL_CALL(glVertexAttribPointer((int)element.semantic, element.count, type, normalized ? GL_TRUE : GL_FALSE, channel.stride, (const void *)(size_t)element.offset));
                    CHECK_GL_ERRORS();
                    DC->counters.num_api_calls += 2;
#endif
                }
            }

#ifdef GL_VAOS
            mesh->vaos[device_index] = vao;
#endif
        }

#ifdef GL_VAOS
        glc_bind_vertex_array(DC, vao->vao_id, vao->resource_id);
#endif

        if (mesh->index_buffer) {
            glDrawElements(draw_mode, elm_count, idx_type, (void *)(size_t)(idx_size * idx_ofs));
            CHECK_GL_ERRORS();
        } else {
            glDrawArrays(draw_mode, idx_ofs, elm_count);
            CHECK_GL_ERRORS();
        }

        ++DC->counters.num_api_calls;

        if (draw_mode == GL_LINES) {
            DC->counters.num_tris += elm_count / 2;
        } else {
            DC->counters.num_tris += elm_count / 3;
        }
    }
}

/*
=======================================
gld_clear_screen_cds
=======================================
*/

static void gld_clear_screen_cds(rhi_device_t * const device, const float r, const float g, const float b, const float a, const float d, const uint8_t s) {
    const bool old_scissor = DC->rs_desc.scissor_test;
    const bool old_depth_mask = DC->dss_desc.depth_write_mask;
    const bool old_stencil_mask = DC->dss_desc.stencil_write_mask;
    const uint8_t old_color_mask = DC->bs_desc.color_write_mask;

    glc_depth_mask(DC, true);
    glc_color_mask(DC, rhi_color_write_mask_all);
    glc_stencil_mask(DC, 0xff);
    glc_enable_scissor(DC, false);

    glc_bind_framebuffer(DC, GL_FRAMEBUFFER, 0, 0);

    glClearColor(r, g, b, a);
    CHECK_GL_ERRORS();
    glClearStencil(s);
    CHECK_GL_ERRORS();

#ifdef GL_CORE
    glClearDepth(d);
#else // GL_ES
    glClearDepthf(d);
#endif
    CHECK_GL_ERRORS();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK_GL_ERRORS();

    glc_color_mask(DC, old_color_mask);
    glc_depth_mask(DC, old_depth_mask);
    glc_stencil_mask(DC, old_stencil_mask);
    glc_enable_scissor(DC, old_scissor);
}

/*
=======================================
gld_clear_screen_ds
=======================================
*/

static void gld_clear_screen_ds(rhi_device_t * const device, const float d, const uint8_t s) {
    const bool old_scissor = DC->rs_desc.scissor_test;
    const bool old_depth_mask = DC->dss_desc.depth_write_mask;
    const bool old_stencil_mask = DC->dss_desc.stencil_write_mask;

    glc_depth_mask(DC, true);
    glc_stencil_mask(DC, 0xff);
    glc_enable_scissor(DC, false);

    glc_bind_framebuffer(DC, GL_FRAMEBUFFER, 0, 0);

    glClearStencil(s);
    CHECK_GL_ERRORS();

#ifdef GL_CORE
    glClearDepth(d);
#else // GL_ES
    glClearDepthf(d);
#endif
    CHECK_GL_ERRORS();

    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK_GL_ERRORS();

    glc_depth_mask(DC, old_depth_mask);
    glc_stencil_mask(DC, old_stencil_mask);
    glc_enable_scissor(DC, old_scissor);
}

/*
=======================================
gld_clear_screen_c
=======================================
*/

static void gld_clear_screen_c(rhi_device_t * const device, const float r, const float g, const float b, const float a) {
    const bool old_scissor = DC->rs_desc.scissor_test;
    const uint8_t old_color_mask = DC->bs_desc.color_write_mask;

    glc_color_mask(DC, rhi_color_write_mask_all);
    glc_enable_scissor(DC, false);

    glc_bind_framebuffer(DC, GL_FRAMEBUFFER, 0, 0);

    glClearColor(r, g, b, a);
    CHECK_GL_ERRORS();

    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERRORS();

    glc_color_mask(DC, old_color_mask);
    glc_enable_scissor(DC, old_scissor);
}

/*
=======================================
gld_clear_screen_d
=======================================
*/

static void gld_clear_screen_d(rhi_device_t * const device, const float d) {
    const bool old_scissor = DC->rs_desc.scissor_test;
    const bool old_depth_mask = DC->dss_desc.depth_write_mask;

    glc_depth_mask(DC, true);
    glc_enable_scissor(DC, false);

    glc_bind_framebuffer(DC, GL_FRAMEBUFFER, 0, 0);

#ifdef GL_CORE
    glClearDepth(d);
#else // GL_ES
    glClearDepthf(d);
#endif
    CHECK_GL_ERRORS();

    glClear(GL_DEPTH_BUFFER_BIT);
    CHECK_GL_ERRORS();

    glc_depth_mask(DC, old_depth_mask);
    glc_enable_scissor(DC, old_scissor);
}

/*
=======================================
gld_clear_screen_s
=======================================
*/

static void gld_clear_screen_s(rhi_device_t * const device, const uint8_t s) {
    const bool old_scissor = DC->rs_desc.scissor_test;
    const bool old_stencil_mask = DC->dss_desc.stencil_write_mask;

    glc_stencil_mask(DC, 0xff);
    glc_enable_scissor(DC, false);

    glc_bind_framebuffer(DC, GL_FRAMEBUFFER, 0, 0);

    glClearStencil(s);
    CHECK_GL_ERRORS();

    glClear(GL_STENCIL_BUFFER_BIT);
    CHECK_GL_ERRORS();

    glc_stencil_mask(DC, old_stencil_mask);
    glc_enable_scissor(DC, old_scissor);
}

/*
=======================================
gld_present
=======================================
*/

static void gld_present(rhi_device_t * const device, const rhi_swap_interval_t swap_interval) {
    glc_swap_buffers(DC, swap_interval);
#ifdef GL_VAOS
    gl_free_vao_chain(DC, &D->vao_pending_destroy_chain, MALLOC_TAG);
#endif
}

/*
=======================================
gld_screenshot
=======================================
*/

static void gld_screenshot(rhi_device_t * const device, image_t * const out_image, const mem_region_t mem_region) {
    ZEROMEM(out_image);

    out_image->bpp = 4;
    out_image->width = D->display_width;
    out_image->height = D->display_height;
    out_image->depth = 1;
    out_image->pitch = out_image->width * out_image->bpp;
    out_image->data_len = out_image->pitch * out_image->height;
    out_image->spitch = out_image->data_len;
    out_image->data = mem_region.ptr;

    ASSERT_MSG((size_t)out_image->data_len <= mem_region.size, "insufficient space for screenshot, size required: %i", out_image->data_len);

#ifdef GL_CORE
    glReadBuffer(GL_BACK);
    CHECK_GL_ERRORS();
#endif
    glReadPixels(0, 0, D->display_width, D->display_height, GL_RGBA, GL_UNSIGNED_BYTE, out_image->data);
    CHECK_GL_ERRORS();

    image_vertical_flip_in_place(out_image);
}

/*
=======================================
gld_read_and_clear_counters
=======================================
*/

static void gld_read_and_clear_counters(rhi_device_t * const device, rhi_counters_t * const counters) {
    *counters = DC->counters;
    ZEROMEM(&DC->counters);
}

/*
=======================================
gld_create_mesh_data_layout
=======================================
*/

static rhi_mesh_data_layout_t * gld_create_mesh_data_layout(rhi_device_t * const device, const rhi_mesh_data_layout_desc_t * const mesh_data_layout, const char * const tag) {
    return (rhi_mesh_data_layout_t *)gl_create_mesh_data_layout(DC, &device->resource, mesh_data_layout, tag);
}

/*
=======================================
gld_create_mesh
=======================================
*/

static rhi_mesh_t * gld_create_mesh(rhi_device_t * const device, const rhi_mesh_data_init_indirect_t mesh_data, const char * const tag) {
    ASSERT(*mesh_data.layout);
    ASSERT(tag);

    gl_mesh_t * const new_mesh = gl_create_mesh(DC, &device->resource, tag);

    new_mesh->layout = (gl_mesh_data_layout_t *)*mesh_data.layout;
    rhi_add_ref(&(*mesh_data.layout)->resource);

    new_mesh->num_indices = mesh_data.num_indices;
    new_mesh->tag = tag;

    const rhi_mesh_data_layout_desc_t layout = new_mesh->layout->desc;

    ASSERT(layout.num_channels);

#ifndef NDEBUG
    if (layout.num_channels != mesh_data.num_channels) {
        char buff[256];
        sprintf_s(buff, 256, "layout.num_channels != mesh_data.num_channels @ [%s]", tag);
        TRAP(buff);
    }
#endif

    if (layout.indices) {
#if defined(GL_VAOS) && !defined(GL_DSA)
        // if we have to bind to the GL_ELEMENT_ARRAY_BUFFER bind point then
        // avoid wiping the binding on any currently bound VAO.
        glc_bind_vertex_array(DC, 0, 0);
#endif

        glc_gen_buffers(DC, 1, &new_mesh->index_buffer);
        new_mesh->index_buffer_id = gl_get_next_obj_id();

        if (layout.indices->usage == rhi_usage_default) {
            ASSERT(mesh_data.indices.size);
            glc_static_buffer_storage(DC, GL_ELEMENT_ARRAY_BUFFER, new_mesh->index_buffer, new_mesh->index_buffer_id, mesh_data.indices.size, mesh_data.indices.ptr);
        } else if (mesh_data.indices.size) {
            ASSERT(layout.indices->usage == rhi_usage_dynamic);
            glc_dynamic_buffer_data(DC, GL_ELEMENT_ARRAY_BUFFER, new_mesh->index_buffer, new_mesh->index_buffer_id, mesh_data.indices.size, mesh_data.indices.ptr);
        }

        if (mesh_data.indices.ptr) {
            DC->counters.num_upload_indices += (uint32_t)mesh_data.indices.size / ((layout.indices->format == rhi_vertex_index_format_uint16) ? 2 : 4);
            DC->counters.upload_index_size += (uint32_t)mesh_data.indices.size;
        }
    }

    new_mesh->vertex_buffers = (GLuint *)gl_alloc(sizeof(GLuint) * layout.num_channels * 3, tag);
    new_mesh->vertex_buffer_ids = (int *)new_mesh->vertex_buffers + layout.num_channels;
    new_mesh->vertex_buffer_sizes = (int *)new_mesh->vertex_buffer_ids + layout.num_channels;

    glc_gen_buffers(DC, layout.num_channels, new_mesh->vertex_buffers);

    for (int i = 0; i < layout.num_channels; ++i) {
        const int vertex_buffer_id = gl_get_next_obj_id();
        new_mesh->vertex_buffer_ids[i] = vertex_buffer_id;

        const rhi_vertex_layout_desc_t channel_layout = layout.channels[i];
        const const_mem_region_t channel_data = mesh_data.channels[i];

        const int vertex_buffer = new_mesh->vertex_buffers[i];
        new_mesh->vertex_buffer_sizes[i] = (int)channel_data.size;

        if (channel_layout.usage == rhi_usage_default) {
            ASSERT(channel_data.size);
            glc_static_buffer_storage(DC, GL_ARRAY_BUFFER, vertex_buffer, vertex_buffer_id, channel_data.size, channel_data.ptr);
        } else if (channel_data.size) {
            ASSERT(channel_layout.usage == rhi_usage_dynamic);
            glc_dynamic_buffer_data(DC, GL_ARRAY_BUFFER, vertex_buffer, vertex_buffer_id, channel_data.size, channel_data.ptr);
        }

        if (channel_data.ptr) {
            DC->counters.num_upload_verts += (uint32_t)channel_data.size / channel_layout.stride;
            DC->counters.upload_vert_size += (uint32_t)channel_data.size;
        }
    }

    return (rhi_mesh_t *)new_mesh;
}

/*
=======================================
gld_create_mesh_from_shared_vertex_data
=======================================
*/

static rhi_mesh_t * gld_create_mesh_from_shared_vertex_data(rhi_device_t * const device, rhi_mesh_t * const mesh_with_shared_vertices, const const_mem_region_t indices, const int num_indices, const char * const tag) {
    gl_mesh_t * const src = (gl_mesh_t *)mesh_with_shared_vertices;
    gl_mesh_t * const gl_mesh_with_shared_vertices = src->shared_verts ? src->shared_verts : src;

    gl_mesh_t * new_mesh = gl_create_mesh(DC, &device->resource, tag);
    new_mesh->tag = tag;
    new_mesh->num_indices = num_indices;
    new_mesh->shared_verts = gl_mesh_with_shared_vertices;
    rhi_add_ref(&mesh_with_shared_vertices->resource);

    const rhi_mesh_data_layout_desc_t layout = gl_mesh_with_shared_vertices->layout->desc;

#if defined(GL_VAOS) && !defined(GL_DSA)
    // if we have to bind to the GL_ELEMENT_ARRAY_BUFFER bind point then
    // avoid wiping the binding on any currently bound VAO.
    glc_bind_vertex_array(DC, 0, 0);
#endif

    glc_gen_buffers(DC, 1, &new_mesh->index_buffer);
    new_mesh->index_buffer_id = gl_get_next_obj_id();

    if (layout.indices->usage == rhi_usage_default) {
        ASSERT(indices.ptr);
        glc_static_buffer_storage(DC, GL_ELEMENT_ARRAY_BUFFER, new_mesh->index_buffer, new_mesh->index_buffer_id, indices.size, indices.ptr);
        DC->counters.num_upload_indices += (uint32_t)indices.size / ((layout.indices->format == rhi_vertex_index_format_uint16) ? 2 : 4);
        DC->counters.upload_index_size += (uint32_t)indices.size;
    } else if (indices.size) {
        glc_dynamic_buffer_data(DC, GL_ELEMENT_ARRAY_BUFFER, new_mesh->index_buffer, new_mesh->index_buffer_id, indices.size, indices.ptr);
    }

    return (rhi_mesh_t *)new_mesh;
}

/*
=======================================
gld_create_program_from_binary
=======================================
*/

static rhi_program_t * gld_create_program_from_binary(rhi_device_t * const device, const const_mem_region_t vert_program, const const_mem_region_t frag_program, rhi_error_t ** const out_err, const char * const tag) {
    const GLuint vs = glc_create_shader(DC, GL_VERTEX_SHADER, vert_program.ptr, (GLint)vert_program.size);
    if (glc_get_compile_status(DC, vs) != GL_TRUE) {
        if (out_err) {
            const GLint len = glc_get_shader_log_size(DC, vs);
            char * const log = (char *)ALLOCA(len);
            *out_err = GL_CREATE_ERROR_PRINTF("vertex shader error: '%s'", glc_get_shader_log(DC, vs, log, len));
        }
        glc_delete_shader(DC, vs);
        return NULL;
    }

    const GLuint fs = glc_create_shader(DC, GL_FRAGMENT_SHADER, frag_program.ptr, (GLint)frag_program.size);
    if (glc_get_compile_status(DC, fs) != GL_TRUE) {
        if (out_err) {
            const GLint len = glc_get_shader_log_size(DC, fs);
            char * const log = (char *)ALLOCA(len);
            *out_err = GL_CREATE_ERROR_PRINTF("fragment shader error: '%s'", glc_get_shader_log(DC, fs, log, len));
        }
        glc_delete_shader(DC, fs);
        return NULL;
    }

    const GLuint p = glc_create_program(DC);
    glc_attach_shader(DC, p, vs);
    glc_attach_shader(DC, p, fs);

    for (int i = 0; i < ARRAY_SIZE(rhi_program_semantic_names); ++i) {
        GL_CALL(glBindAttribLocation(p, i, rhi_program_semantic_names[i]));
        CHECK_GL_ERRORS();
    }

    glc_link_program(DC, p);

    glc_delete_shader(DC, vs);
    glc_delete_shader(DC, fs);

    if (glc_get_link_status(DC, p) != GL_TRUE) {
        if (out_err) {
            const GLint len = glc_get_program_log_size(DC, p);
            char * const log = (char *)ALLOCA(len);
            *out_err = GL_CREATE_ERROR_PRINTF("program error: '%s'", glc_get_program_log(DC, p, log, len));
        }
        glc_delete_program(DC, p);
        return NULL;
    }

    gl_program_t * const program = gl_create_program(DC, &device->resource, tag);
    program->program = p;
    glc_use_program(DC, program);

    for (int i = 0; i < ARRAY_SIZE(rhi_program_uniform_names); ++i) {
        program->uniform_locations[i] = GL_CALL(glGetUniformLocation(p, rhi_program_uniform_names[i]));
    }

    // set the sampler texture indexes
    for (int i = 0; i < ARRAY_SIZE(rhi_program_texture_samplers); ++i) {
        const int loc = program->uniform_locations[rhi_program_texture_samplers[i]];
        if (loc != -1) {
            GL_CALL(glUniform1i(loc, i));
        }
    }

    if (out_err) {
        *out_err = NULL;
    }

    return (rhi_program_t *)program;
}

/*
=======================================
gld_create_blend_state
=======================================
*/

static rhi_blend_state_t * gld_create_blend_state(rhi_device_t * const device, const rhi_blend_state_desc_t * const desc, const char * const tag) {
    gl_blend_state_t * const blend_state = gl_create_blend_state(DC, &device->resource, tag);
    blend_state->desc = *desc;
    return (rhi_blend_state_t *)blend_state;
}

/*
=======================================
gld_create_depth_stencil_state
=======================================
*/

static rhi_depth_stencil_state_t * gld_create_depth_stencil_state(rhi_device_t * const device, const rhi_depth_stencil_state_desc_t * const desc, const char * const tag) {
    gl_depth_stencil_state_t * const depth_stencil_state = gl_create_depth_stencil_state(DC, &device->resource, tag);
    depth_stencil_state->desc = *desc;
    return (rhi_depth_stencil_state_t *)depth_stencil_state;
}

/*
=======================================
gld_thread_device_done_current
=======================================
*/

static rhi_rasterizer_state_t * gld_create_rasterizer_state(rhi_device_t * const device, const rhi_rasterizer_state_desc_t * const desc, const char * const tag) {
    gl_rasterizer_state_t * const rasterizer_state = gl_create_rasterizer_state(DC, &device->resource, tag);
    rasterizer_state->desc = *desc;
    return (rhi_rasterizer_state_t *)rasterizer_state;
}

/*
=======================================
gld_create_uniform_buffer
=======================================
*/

static rhi_uniform_buffer_t * gld_create_uniform_buffer(rhi_device_t * device, const rhi_uniform_buffer_desc_t * desc, const void * initial_data, const char * const tag) {
    ASSERT(desc->size);
    ASSERT((desc->usage == rhi_usage_dynamic) || initial_data);
    ASSERT_ALIGNED_16(desc->size);
    ASSERT((desc->usage == rhi_usage_default) || (desc->usage == rhi_usage_dynamic));

    gl_uniform_buffer_t * const uniform_buffer = gl_create_uniform_buffer(DC, &device->resource, tag);
    uniform_buffer->data.size = desc->size;
    uniform_buffer->dynamic = desc->usage == rhi_usage_dynamic;

    uniform_buffer->data.ptr = gl_alloc(desc->size, tag);

    if (initial_data) {
        memcpy(uniform_buffer->data.ptr, initial_data, desc->size);
        uniform_buffer->crc = crc_32(initial_data, desc->size);
    }

    return (rhi_uniform_buffer_t *)uniform_buffer;
}

/*
=======================================
gld_create_texture_2d
=======================================
*/

static rhi_texture_t * gld_create_texture_2d(rhi_device_t * const device, const image_mips_t * const mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_state, const char * const tag) {
    ASSERT(format >= 0);
    ASSERT(format < rhi_num_pixels_formats);
    ASSERT(format < ARRAY_SIZE(pixel_formats));

    gl_texture_t * const texture = glc_create_texture(DC, &device->resource, GL_TEXTURE_2D, tag);
    texture->target = GL_TEXTURE_2D;
    texture->pixel_format = format;

#ifndef NDEBUG
    texture->usage = usage;
#endif

    ASSERT(mipmaps->num_levels > 0);
    ASSERT(mipmaps->first_level == 0);
    ASSERT(mipmaps->levels[0].depth == 1);
    ASSERT(sampler_state.max_anisotropy > 0);

    texture->width = mipmaps->levels[0].width;
    texture->height = mipmaps->levels[0].height;
    texture->depth = 1;

    const gl_pixel_format_t gl_pixel_format = pixel_formats[format];

#ifndef GL_CORE
    if (format == rhi_pixel_format_etc1) {
        const image_t img = mipmaps->levels[0];
        glc_compressed_texture_image_2d(
            DC,
            GL_TEXTURE0,
            texture->target,
            texture->texture,
            texture->resource.resource.instance_id,
            0,
            img.width,
            img.height,
            gl_pixel_format.internal,
            0,
            img.data_len,
            img.data);
    } else
#endif
    {
        glc_texture_storage_2d(
            DC,
            GL_TEXTURE0,
            texture->texture,
            texture->resource.resource.instance_id,
            mipmaps->num_levels,
            gl_pixel_format.internal,
            texture->width,
            texture->height,
            gl_pixel_format.format,
            gl_pixel_format.type);

        if (mipmaps->levels[0].data) {
            for (int i = 0; i < mipmaps->num_levels; ++i) {
                const image_t img = mipmaps->levels[i];

                if ((format >= rhi_pixel_format_r8_unorm)
                    && (format <= rhi_pixel_format_rgba8_unorm)) {
                    glc_texture_subimage_2d(
                        DC,
                        (texture_subimage_2d_args_t){
                            .tunit = GL_TEXTURE0,
                            .target = texture->target,
                            .texture = texture->texture,
                            .id = texture->resource.resource.instance_id,
                            .level = i,
                            .xoffset = 0,
                            .yoffset = 0,
                            .w = img.width,
                            .h = img.height,
                            .format = gl_pixel_format.format,
                            .type = gl_pixel_format.type,
                            .pixels = img.data,
                        });
#ifdef GL_CORE
                    // swizzle grayscale images
                    if (format == rhi_pixel_format_ra8_unorm) {
                        glc_texture_set_swizzle_mask(DC, GL_TEXTURE0, texture->target, texture->texture, texture->resource.resource.instance_id, GL_RED, GL_RED, GL_RED, GL_GREEN);
                    } else if (format == rhi_pixel_format_r8_unorm) {
                        glc_texture_set_swizzle_mask(DC, GL_TEXTURE0, texture->target, texture->texture, texture->resource.resource.instance_id, GL_RED, GL_RED, GL_RED, GL_ONE);
                    }
#endif
                } else {
#ifdef GL_CORE
                    ASSERT((format == rhi_pixel_format_bc1_unorm) || (format == rhi_pixel_format_bc3_unorm));
                    glc_compressed_texture_subimage_2d(
                        DC,
                        GL_TEXTURE0,
                        texture->target,
                        texture->texture,
                        texture->resource.resource.instance_id,
                        i,
                        0,
                        0,
                        img.width,
                        img.height,
                        gl_pixel_format.internal,
                        img.data_len,
                        img.data);
#endif
                }

#ifdef GL_ES
                // GL_ES requires a complete mipmap chain,
                // if we don't send a complete one in then skip loading sublevels
                // and mipmap this
                if ((i == 0) && (sampler_state.min_filter != rhi_min_filter_nearest) && (sampler_state.min_filter != rhi_min_filter_linear)) {
                    const image_t last_mip = mipmaps->levels[mipmaps->num_levels - 1];
                    if ((last_mip.width == 1) && (last_mip.height == 1)) {
                        texture->mipmaps = true; // full mipmaps
                    } else {
                        VERIFY_MSG(format <= rhi_pixel_format_rgba8_unorm, "GLES error: compressed texture without complete mipchain!");
                    }
                }
#endif
            }
        }
    }
#ifdef GL_CORE
    glc_tex_parameter_i(
        DC,
        GL_TEXTURE0,
        texture->target,
        texture->texture,
        texture->resource.resource.instance_id,
        GL_TEXTURE_MAX_LEVEL,
        mipmaps->num_levels - 1);

    glc_sampler_parameter_i(DC, texture->sampler, GL_TEXTURE_MIN_FILTER, get_gl_min_filter(sampler_state.min_filter));
    glc_sampler_parameter_i(DC, texture->sampler, GL_TEXTURE_MAG_FILTER, get_gl_max_filter(sampler_state.max_filter));
    glc_sampler_parameter_i(DC, texture->sampler, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(sampler_state.u_wrap_mode));
    glc_sampler_parameter_i(DC, texture->sampler, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(sampler_state.v_wrap_mode));
    glc_sampler_parameter_i(DC, texture->sampler, GL_TEXTURE_WRAP_R, get_gl_wrap_mode(sampler_state.w_wrap_mode));
    glc_sampler_parameter_i(DC, texture->sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler_state.max_anisotropy ? sampler_state.max_anisotropy : DC->caps.max_anisotropy);
#else // GL_ES
    if (!texture->mipmaps && (sampler_state.min_filter != rhi_min_filter_linear) && (sampler_state.min_filter != rhi_min_filter_nearest)) {
        glGenerateMipmap(texture->target);
        CHECK_GL_ERRORS();
        texture->mipmaps = true;
    }
    glc_tex_parameter_i(DC, GL_TEXTURE0, texture->target, texture->texture, texture->resource.resource.instance_id, GL_TEXTURE_MIN_FILTER, get_gl_min_filter(sampler_state.min_filter));
    glc_tex_parameter_i(DC, GL_TEXTURE0, texture->target, texture->texture, texture->resource.resource.instance_id, GL_TEXTURE_MAG_FILTER, get_gl_max_filter(sampler_state.max_filter));
    glc_tex_parameter_i(DC, GL_TEXTURE0, texture->target, texture->texture, texture->resource.resource.instance_id, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(sampler_state.u_wrap_mode));
    glc_tex_parameter_i(DC, GL_TEXTURE0, texture->target, texture->texture, texture->resource.resource.instance_id, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(sampler_state.v_wrap_mode));
#endif

    return (rhi_texture_t *)texture;
}

/*
=======================================
gld_create_render_target
=======================================
*/

static rhi_render_target_t * gld_create_render_target(rhi_device_t * const device, const char * const tag) {
    return (rhi_render_target_t *)gl_create_render_target(DC, &device->resource, tag);
}

/*
===============================================================================
Device vtable / creation
===============================================================================
*/

static const rhi_device_vtable_t gl_rhi_device_vtable = {
    .get_caps = gld_get_caps,
    .thread_make_device_current = gld_thread_make_device_current,
    .thread_device_done_current = gld_thread_device_done_current,
    .upload_mesh_channel_data = gld_upload_mesh_channel_data,
    .upload_mesh_indices = gld_upload_mesh_indices,
    .upload_uniform_data = gld_upload_uniform_data,
    .upload_texture = gld_upload_texture,
    .upload_sub_texture = gld_upload_sub_texture,
    .set_display_size = gld_set_display_size,
    .set_viewport = gld_set_viewport,
    .set_scissor_rect = gld_set_scissor_rect,
    .set_program = gld_set_program,
    .set_render_state = gld_set_render_state,
    .set_render_target_color_buffer = gld_set_render_target_color_buffer,
    .discard_render_target_data = gld_discard_render_target_data,
    .set_render_target = gld_set_render_target,
    .set_uniform_bindings = gld_set_uniform_bindings,
    .set_texture_sampler_state = gld_set_texture_sampler_state,
    .set_texture_bindings_indirect = gld_set_texture_bindings_indirect,
    .clear_render_target_color_buffers = gld_clear_render_target_color_buffers,
    .copy_render_target_buffers = gld_copy_render_target_buffers,
    .draw_indirect = gld_draw_indirect,
    .clear_screen_cds = gld_clear_screen_cds,
    .clear_screen_ds = gld_clear_screen_ds,
    .clear_screen_c = gld_clear_screen_c,
    .clear_screen_d = gld_clear_screen_d,
    .clear_screen_s = gld_clear_screen_s,
    .present = gld_present,
    .screenshot = gld_screenshot,
    .read_and_clear_counters = gld_read_and_clear_counters,
    .create_mesh_data_layout = gld_create_mesh_data_layout,
    .create_mesh = gld_create_mesh,
    .create_mesh_from_shared_vertex_data = gld_create_mesh_from_shared_vertex_data,
    .create_program_from_binary = gld_create_program_from_binary,
    .create_blend_state = gld_create_blend_state,
    .create_depth_stencil_state = gld_create_depth_stencil_state,
    .create_rasterizer_state = gld_create_rasterizer_state,
    .create_uniform_buffer = gld_create_uniform_buffer,
    .create_texture_2d = gld_create_texture_2d,
    .create_render_target = gld_create_render_target,
    .dump_heap_usage = gl_dump_heap_usage};

/*
=======================================
gl_device_add_ref
=======================================
*/

static int gl_device_add_ref(rhi_resource_t * const resource) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, 1, memory_order_relaxed);
    ASSERT(r > 0);
    return r;
}

/*
=======================================
gl_device_release
=======================================
*/

static int gl_device_release(rhi_resource_t * const resource, const char * const tag) {
    const int r = sb_atomic_fetch_add(&resource->ref_count, -1, memory_order_relaxed);
    ASSERT(r > 0);

    if (r == 1) {
        gl_device_and_context_t * const device = (gl_device_and_context_t *)resource;
        CHECK_GL_ERRORS();
        glc_make_current(&device->context);
        CHECK_GL_ERRORS();
#ifdef GL_VAOS
        gl_free_vao_chain(&device->context, &device->vao_used_chain, tag);
        CHECK_GL_ERRORS();
        gl_free_vao_chain(&device->context, &device->vao_pending_destroy_chain, tag);
        CHECK_GL_ERRORS();
#endif
        glc_destroy(&device->context, tag);
        CHECK_GL_ERRORS();
        devices[device->index] = NULL;
        gl_free(resource, tag);
    }

    return r;
}

static const rhi_resource_vtable_t gl_rhi_device_resource_vtable_t = {
    .add_ref = gl_device_add_ref,
    .release = gl_device_release};

/*
=======================================
gl_rhi_api_create_device
=======================================
*/

rhi_device_t * gl_rhi_api_create_device(struct sb_window_t * const window, rhi_error_t ** const out_error, const char * const tag) {
    if (out_error) {
        *out_error = NULL;
    }

    // find free device index
    int device_index;
    for (device_index = 0; device_index < ARRAY_SIZE(devices); ++device_index) {
        if (!devices[device_index]) {
            break;
        }
    }

    if (device_index == ARRAY_SIZE(devices)) {
        if (out_error) {
            *out_error = GL_CREATE_ERROR("rhi_max_devices");
        }
        return NULL;
    }

    gl_device_and_context_t * const device = (gl_device_and_context_t *)gl_alloc(sizeof(gl_device_and_context_t), MALLOC_TAG);
    ZEROMEM(device);

    device->index = device_index;
    device->context.window = window;
    if (!glc_create(&device->context, out_error)) {
        gl_free(device, MALLOC_TAG);
        return NULL;
    }

    device->device.resource.vtable = &gl_rhi_device_resource_vtable_t;
    device->device.resource.instance_id = gl_get_next_obj_id();
    device->device.resource.ref_count.i32 = 1;
    device->device.vtable = &gl_rhi_device_vtable;

    glClearColor(0, 0, 0, 0);
    CHECK_GL_ERRORS();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    CHECK_GL_ERRORS();

    glc_make_current(NULL);

    return &device->device;
}
