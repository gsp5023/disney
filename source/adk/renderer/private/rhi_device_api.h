/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
rhi_device_api.h

RENDER HARDWARE INTERFACE

device vtable
*/

#include "rhi.h"
#include "source/adk/imagelib/imagelib.h"

// define this to turn off calls to RHI device
//#define RHI_NULL_DEVICE

/*
===============================================================================
rhi_device_t

Devices are created from and render into a specific window.
===============================================================================
*/

typedef struct rhi_device_vtable_t {
    rhi_device_caps_t (*get_caps)(rhi_device_t * const device);
    void (*thread_make_device_current)(rhi_device_t * const device);
    void (*thread_device_done_current)(rhi_device_t * const device);
    void (*upload_mesh_channel_data)(rhi_device_t * const device, rhi_mesh_t * const mesh, const int channel_index, const int first_elem, const int num_elems, const void * const data);
    void (*upload_mesh_indices)(rhi_device_t * const device, rhi_mesh_t * const mesh, const int first_index, const int num_indices, const void * const indices);
    void (*upload_uniform_data)(rhi_device_t * const device, rhi_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs);
    void (*upload_texture)(rhi_device_t * const device, rhi_texture_t * const texture, const image_mips_t * const mipmaps);
#if defined(_LEIA) || defined(_VADER)
    void (*bind_texture_address)(rhi_device_t * const device, rhi_texture_t * const texture, const image_mips_t * const mipmaps);
#endif
    void (*set_display_size)(rhi_device_t * const device, const int w, const int h);
    void (*set_viewport)(rhi_device_t * const device, const int x0, const int y0, const int x1, const int y1);
    void (*set_scissor_rect)(rhi_device_t * const device, const int x0, const int y0, const int x1, const int y1);
    void (*set_program)(rhi_device_t * const device, rhi_program_t * const program);
    void (*set_render_state)(rhi_device_t * const device, rhi_rasterizer_state_t * const rs, rhi_depth_stencil_state_t * const dss, rhi_blend_state_t * const bs, const uint32_t stencil_ref);
    void (*set_render_target_color_buffer)(rhi_device_t * const device, rhi_render_target_t * const render_target, rhi_texture_t * const color_buffer, const int index);
    void (*discard_render_target_data)(rhi_device_t * const device, rhi_render_target_t * const render_target);
    void (*set_render_target)(rhi_device_t * const device, rhi_render_target_t * const render_target);
    void (*set_uniform_bindings)(rhi_device_t * const device, rhi_uniform_buffer_t * const uniform_buffer, const int index);
    void (*set_texture_sampler_state)(rhi_device_t * const device, rhi_texture_t * const texture, const rhi_sampler_state_desc_t sampler_state);
    void (*set_texture_bindings_indirect)(rhi_device_t * const device, const rhi_texture_bindings_indirect_t * const texture_bindings);
    void (*clear_render_target_color_buffers)(rhi_device_t * const device, rhi_render_target_t * const render_target, const uint32_t color);
    void (*copy_render_target_buffers)(rhi_device_t * const device, rhi_render_target_t * const src, rhi_render_target_t * const dst);
    void (*draw_indirect)(rhi_device_t * const device, const rhi_draw_params_indirect_t * const params);
    void (*clear_screen_cds)(rhi_device_t * const device, const float r, const float g, const float b, const float a, const float d, const uint8_t s);
    void (*clear_screen_ds)(rhi_device_t * const device, const float d, const uint8_t s);
    void (*clear_screen_c)(rhi_device_t * const device, const float r, const float g, const float b, const float a);
    void (*clear_screen_d)(rhi_device_t * const device, const float d);
    void (*clear_screen_s)(rhi_device_t * const device, const uint8_t s);
    void (*present)(rhi_device_t * const device, const rhi_swap_interval_t swap_interval);
    void (*screenshot)(rhi_device_t * const device, image_t * const image, const mem_region_t mem_region);
    void (*read_and_clear_counters)(rhi_device_t * const device, rhi_counters_t * const counters);

    rhi_mesh_data_layout_t * (*create_mesh_data_layout)(rhi_device_t * const device, const rhi_mesh_data_layout_desc_t * const mesh_data_layout, const char * const tag);
    rhi_mesh_t * (*create_mesh)(rhi_device_t * const device, const rhi_mesh_data_init_indirect_t mesh_data, const char * const tag);
    rhi_mesh_t * (*create_mesh_from_shared_vertex_data)(rhi_device_t * const device, rhi_mesh_t * const mesh, const const_mem_region_t indices, const int num_indices, const char * const tag);
    rhi_program_t * (*create_program_from_binary)(rhi_device_t * const device, const const_mem_region_t vert_program, const const_mem_region_t frag_program, rhi_error_t ** const out_err, const char * const tag);
    rhi_blend_state_t * (*create_blend_state)(rhi_device_t * const device, const rhi_blend_state_desc_t * const desc, const char * const tag);
    rhi_depth_stencil_state_t * (*create_depth_stencil_state)(rhi_device_t * const device, const rhi_depth_stencil_state_desc_t * const desc, const char * const tag);
    rhi_rasterizer_state_t * (*create_rasterizer_state)(rhi_device_t * const device, const rhi_rasterizer_state_desc_t * const desc, const char * const tag);
    rhi_uniform_buffer_t * (*create_uniform_buffer)(rhi_device_t * device, const rhi_uniform_buffer_desc_t * desc, const void * initial_data, const char * const tag);
    rhi_texture_t * (*create_texture_2d)(rhi_device_t * const device, const image_mips_t * const mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_state, const char * const tag);
    rhi_render_target_t * (*create_render_target)(rhi_device_t * const device, const char * const tag);

    void (*dump_heap_usage)();
} rhi_device_vtable_t;

static inline rhi_device_caps_t rhi_get_caps(rhi_device_t * const device) {
    return device->vtable->get_caps(device);
}

static inline void rhi_thread_make_device_current(rhi_device_t * const device) {
#ifndef RHI_NULL_DEVICE
    device->vtable->thread_make_device_current(device);
#endif
}

static inline void rhi_thread_device_done_current(rhi_device_t * const device) {
#ifndef RHI_NULL_DEVICE
    device->vtable->thread_device_done_current(device);
#endif
}

static inline void rhi_upload_mesh_channel_data(rhi_device_t * const device, rhi_mesh_t * const mesh, const int channel_index, const int first_elem, const int num_elems, const void * const data) {
#ifndef RHI_NULL_DEVICE
    device->vtable->upload_mesh_channel_data(device, mesh, channel_index, first_elem, num_elems, data);
#endif
}

static inline void rhi_upload_mesh_indices(rhi_device_t * device, rhi_mesh_t * const mesh, const int first_index, const int num_indices, const void * const indices) {
#ifndef RHI_NULL_DEVICE
    device->vtable->upload_mesh_indices(device, mesh, first_index, num_indices, indices);
#endif
}

static inline void rhi_upload_uniform_data(rhi_device_t * const device, rhi_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs) {
#ifndef RHI_NULL_DEVICE
    device->vtable->upload_uniform_data(device, uniform_buffer, data, ofs);
#endif
}

static inline void rhi_upload_texture(rhi_device_t * const device, rhi_texture_t * const texture, const image_mips_t * const mipmaps) {
#ifndef RHI_NULL_DEVICE
    device->vtable->upload_texture(device, texture, mipmaps);
#endif
}

#if defined(_LEIA) || defined(_VADER)
static inline void rhi_bind_texture_address(rhi_device_t * const device, rhi_texture_t * const texture, const image_mips_t * const mipmaps) {
#ifndef RHI_NULL_DEVICE
    device->vtable->bind_texture_address(device, texture, mipmaps);
#endif
}
#endif

static inline void rhi_set_display_size(rhi_device_t * const device, const int w, const int h) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_display_size(device, w, h);
#endif
}

static inline void rhi_set_viewport(rhi_device_t * const device, const int x0, const int y0, const int x1, const int y1) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_viewport(device, x0, y0, x1, y1);
#endif
}

static inline void rhi_set_scissor_rect(rhi_device_t * const device, const int x0, const int y0, const int x1, const int y1) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_scissor_rect(device, x0, y0, x1, y1);
#endif
}

static inline void rhi_set_program(rhi_device_t * const device, rhi_program_t * const program) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_program(device, program);
#endif
}

static inline void rhi_set_render_state(rhi_device_t * const device, rhi_rasterizer_state_t * const rs, rhi_depth_stencil_state_t * const dss, rhi_blend_state_t * const bs, const uint32_t stencil_ref) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_render_state(device, rs, dss, bs, stencil_ref);
#endif
}

static inline void rhi_set_render_target_color_buffer(rhi_device_t * const device, rhi_render_target_t * const render_target, rhi_texture_t * const color_buffer, const int index) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_render_target_color_buffer(device, render_target, color_buffer, index);
#endif
}

static inline void rhi_discard_render_target_data(rhi_device_t * const device, rhi_render_target_t * const render_target) {
#ifndef RHI_NULL_DEVICE
    device->vtable->discard_render_target_data(device, render_target);
#endif
}

static inline void rhi_set_render_target(rhi_device_t * const device, rhi_render_target_t * const render_target) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_render_target(device, render_target);
#endif
}

static inline void rhi_set_uniform_bindings(rhi_device_t * const device, rhi_uniform_buffer_t * const uniform_buffer, const int index) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_uniform_bindings(device, uniform_buffer, index);
#endif
}

static inline void rhi_set_texture_sampler_state(rhi_device_t * const device, rhi_texture_t * const texture, const rhi_sampler_state_desc_t sampler_state) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_texture_sampler_state(device, texture, sampler_state);
#endif
}

static inline void rhi_set_texture_bindings_indirect(rhi_device_t * const device, const rhi_texture_bindings_indirect_t * const texture_bindings) {
#ifndef RHI_NULL_DEVICE
    device->vtable->set_texture_bindings_indirect(device, texture_bindings);
#endif
}

static inline void rhi_clear_render_target_color_buffers(rhi_device_t * const device, rhi_render_target_t * const render_target, const uint32_t color) {
#ifndef RHI_NULL_DEVICE
    device->vtable->clear_render_target_color_buffers(device, render_target, color);
#endif
}

static inline void rhi_copy_render_target_buffers(rhi_device_t * const device, rhi_render_target_t * const src, rhi_render_target_t * const dst) {
#ifndef RHI_NULL_DEVICE
    device->vtable->copy_render_target_buffers(device, src, dst);
#endif
}

static inline void rhi_draw_indirect(rhi_device_t * const device, const rhi_draw_params_indirect_t * const params) {
#ifndef RHI_NULL_DEVICE
    device->vtable->draw_indirect(device, params);
#endif
}

static inline void rhi_clear_screen_cds(rhi_device_t * const device, const float r, const float g, const float b, const float a, const float d, const uint8_t s) {
#ifndef RHI_NULL_DEVICE
    device->vtable->clear_screen_cds(device, r, g, b, a, d, s);
#endif
}

static inline void rhi_clear_screen_ds(rhi_device_t * const device, const float d, const uint8_t s) {
#ifndef RHI_NULL_DEVICE
    device->vtable->clear_screen_ds(device, d, s);
#endif
}

static inline void rhi_clear_screen_c(rhi_device_t * const device, const float r, const float g, const float b, const float a) {
#ifndef RHI_NULL_DEVICE
    device->vtable->clear_screen_c(device, r, g, b, a);
#endif
}

static inline void rhi_clear_screen_d(rhi_device_t * const device, const float d) {
#ifndef RHI_NULL_DEVICE
    device->vtable->clear_screen_d(device, d);
#endif
}

static inline void rhi_clear_screen_s(rhi_device_t * const device, const uint8_t s) {
#ifndef RHI_NULL_DEVICE
    device->vtable->clear_screen_s(device, s);
#endif
}

static inline void rhi_present(rhi_device_t * const device, const rhi_swap_interval_t swap_interval) {
#ifndef RHI_NULL_DEVICE
    device->vtable->present(device, swap_interval);
#endif
}

static inline void rhi_screenshot(rhi_device_t * const device, image_t * const out_image, const mem_region_t mem_region) {
#ifndef RHI_NULL_DEVICE
    device->vtable->screenshot(device, out_image, mem_region);
#endif
}

static inline void rhi_read_and_clear_counters(rhi_device_t * const device, rhi_counters_t * const counter) {
#ifndef RHI_NULL_DEVICE
    device->vtable->read_and_clear_counters(device, counter);
#endif
}

static inline rhi_mesh_data_layout_t * rhi_create_mesh_data_layout(rhi_device_t * const device, const rhi_mesh_data_layout_desc_t * const mesh_data_layout, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_mesh_data_layout(device, mesh_data_layout, tag);
#endif
}

static inline rhi_mesh_t * rhi_create_mesh(rhi_device_t * const device, const rhi_mesh_data_init_indirect_t mesh_data, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_mesh(device, mesh_data, tag);
#endif
}

static inline rhi_mesh_t * rhi_create_mesh_from_shared_vertex_data(rhi_device_t * const device, rhi_mesh_t * const mesh, const const_mem_region_t indices, const int num_indices, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_mesh_from_shared_vertex_data(device, mesh, indices, num_indices, tag);
#endif
}

static inline rhi_program_t * rhi_create_program_from_binary(rhi_device_t * const device, const const_mem_region_t vert_program, const const_mem_region_t frag_program, rhi_error_t ** const out_err, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_program_from_binary(device, vert_program, frag_program, out_err, tag);
#endif
}

static inline rhi_blend_state_t * rhi_create_blend_state(rhi_device_t * const device, const rhi_blend_state_desc_t * const desc, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_blend_state(device, desc, tag);
#endif
}

static inline rhi_depth_stencil_state_t * rhi_create_depth_stencil_state(rhi_device_t * const device, const rhi_depth_stencil_state_desc_t * const desc, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_depth_stencil_state(device, desc, tag);
#endif
}

static inline rhi_rasterizer_state_t * rhi_create_rasterizer_state(rhi_device_t * const device, const rhi_rasterizer_state_desc_t * const desc, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_rasterizer_state(device, desc, tag);
#endif
}

static inline rhi_uniform_buffer_t * rhi_create_uniform_buffer(rhi_device_t * const device, const rhi_uniform_buffer_desc_t * const desc, const void * const initial_data, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_uniform_buffer(device, desc, initial_data, tag);
#endif
}

static inline rhi_texture_t * rhi_create_texture_2d(rhi_device_t * const device, const image_mips_t * const mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_state, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_texture_2d(device, mipmaps, format, usage, sampler_state, tag);
#endif
}

static inline rhi_render_target_t * rhi_create_render_target(rhi_device_t * const device, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return NULL;
#else
    return device->vtable->create_render_target(device, tag);
#endif
}

#ifdef __cplusplus
}
#endif
