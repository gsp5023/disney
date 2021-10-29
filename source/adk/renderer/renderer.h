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
renderer.h

Multithreaded rendering, command queue, and RHI

The renderer is broken into two fundamental pieces, frontend and backend.

The render frontend is comprised of command queues that are submitted for
processing by the backend.
*/

// NOTE: removed render tags to simplify command hashing
#define ENABLE_RENDER_TAGS 0

#include "private/rbcmd.h"
#include "private/rhi.h"
#include "source/adk/manifest/manifest.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_thread.h"

// the number of frames that can be queued to render before we block on the gpu
enum { render_max_pending_frames = 1 };

/*
===============================================================================
Render resources

render resource lifecycle can only be managed from the main thread
===============================================================================
*/
struct render_cmd_stream_t;

typedef struct rb_fence_t {
#ifndef NDEBUG
    struct render_cmd_stream_t * cmd_stream; // only used when fencing a cmd stream
#endif
    rb_cmd_buf_t * cmd_buf;
    int counter;
} rb_fence_t;

static const rb_fence_t null_rb_fence = {0};

struct render_resource_t;
typedef struct render_resource_vtable_t {
    void (*destroy)(struct render_resource_t * resource, const char * const tag);
} render_resource_vtable_t;

typedef enum render_resource_type_e {
    render_resource_type_invalid,
    render_resource_type_render_device_t,
    render_resource_type_r_texture_t,
    render_resource_type_r_program_t,
    render_resource_type_r_mesh_data_layout_t,
    render_resource_type_r_mesh_t,
    render_resource_type_r_blend_state_t,
    render_resource_type_r_depth_stencil_state_t,
    render_resource_type_r_rasterizer_state_t,
    render_resource_type_r_uniform_buffer_t,
    render_resource_type_r_render_target_t,
} render_resource_type_e;

typedef struct render_resource_t {
    const render_resource_vtable_t * vtable;
    struct render_device_t * device;
    rb_fence_t fence;
    int ref_count;
    render_resource_type_e resource_type;
    const char * tag;
} render_resource_t;

static inline int render_add_ref(render_resource_t * const resource) {
    ASSERT(resource->ref_count > 0);
    return ++resource->ref_count;
}

int render_release(render_resource_t * const resource, const char * const tag);
void render_dump_heap_usage(const struct render_device_t * const device);
heap_metrics_t render_get_heap_metrics(const struct render_device_t * const device);

bool render_is_resource_ready(render_resource_t * const resource);
void render_wait_for_resource(render_resource_t * const resource);

/*
===============================================================================
Render frontend
===============================================================================
*/

typedef enum render_wait_mode_e {
    render_no_wait,
    render_wait
} render_wait_mode_e;

typedef enum rb_cmd_buf_order_e {
    cmd_buf_unordered,
    cmd_buf_ordered
} rb_cmd_buf_order_e;

typedef struct render_cmd_stream_t {
    struct {
        struct render_device_t * device;
        rb_fence_t last_fence;
    } internal;
    rb_cmd_buf_t * buf;
    bool flush;
} render_cmd_stream_t;

typedef struct rb_cmd_buf_queue_t {
    rb_cmd_buf_t * head;
    rb_cmd_buf_t * tail;
} rb_cmd_buf_que_t;

typedef struct render_device_internal_t {
    rb_cmd_buf_que_t ordered_cmd_buf_que;
    rb_cmd_buf_que_t unordered_cmd_buf_que;
    rhi_device_t * device;
    sb_mutex_t * mutex;
    rb_cmd_buf_t * free_cmd_buf_chain;
    sb_condition_variable_t * queued_signal;
    sb_condition_variable_t * retired_signal;
    heap_t object_heap;
    struct render_thread_t * threads;
    sb_atomic_int32_t submit_count;
    sb_atomic_int32_t done_count;
    sb_thread_id_t device_thread_id;
    int num_device_threads;
    sb_atomic_int32_t num_frames;
    rb_fence_t frame_fences[render_max_pending_frames + 1];
    system_guard_page_mode_e guard_page_mode;
    bool quit;
#ifdef GUARD_PAGE_SUPPORT
    mem_region_t guard_mem;
#endif
} render_device_internal_t;

typedef struct render_memory_usage_t {
    uint64_t peak_memory;
    uint64_t total_memory;
    uint64_t mesh_memory;
    uint64_t texture_memory;
    uint64_t uniform_buffer_memory;
} render_memory_usage_t;

typedef struct render_resource_tracking_t {
    bool enabled;
    render_memory_usage_t memory_usage;
} render_resource_tracking_t;

typedef struct render_device_t {
    render_resource_t resource;
    render_device_internal_t internal;
    rhi_device_caps_t caps;
    render_cmd_stream_t default_cmd_stream;
    rhi_api_t * api;
    render_resource_tracking_t resource_tracking;
} render_device_t;

typedef struct render_thread_t {
    struct render_thread_t * next;
    render_device_t * device;
    sb_thread_id_t thread;
} render_thread_t;

/*
=======================================
render_create_device

Create a rendering device from a specific RHI.
This must be called from the applications main() thread.

Memory is not allocated by this function, instead
the specified mem_region is used. Use GET_RENDER_DEVICE_SIZE()
to compute the required memory buffer needed.

Inside your main() application loop you must call
render_device_frame(). This allows devices that do
not support multithreaded rendering to render
any pending command buffers.
=======================================
*/

enum {
    aligned_render_device_size = ALIGN_INT(sizeof(render_device_t), 8),
    aligned_render_cmdbuf_size = ALIGN_INT(sizeof(rb_cmd_buf_t), 8),
    aligned_render_thread_size = ALIGN_INT(sizeof(render_thread_t), 8)
};

#define GET_RENDER_DEVICE_SIZE(_num_cmd_buffers, _cmd_buffer_size, _num_threads) \
    (aligned_render_device_size + (aligned_render_thread_size * (_num_threads)) + ((aligned_render_cmdbuf_size + ALIGN_INT(_cmd_buffer_size, 8)) * _num_cmd_buffers))

render_device_t * create_render_device(rhi_api_t * const api, struct sb_window_t * const window, rhi_error_t ** out_error, const mem_region_t block, const int num_cmd_buffers, const int cmd_buffer_size, const int max_threads, const system_guard_page_mode_e guard_page_mode, const char * const tag);

/*
=======================================
render_device_log_resource_tracking

Log tracked resource metrics to TTY
=======================================
*/

void render_device_log_resource_tracking(const render_device_t * const render_device, const logging_mode_e logging_mode);

/*
=======================================
render_device_frame

Should be called inside main application loop.
=======================================
*/

void render_device_frame(render_device_t * const device);

/*
=======================================
render_wait_frames

Will wait for the specified number of frames to pass before returning.
NOTE: this should never be called from the main thread.
=======================================
*/

void render_wait_frames(render_device_t * const device, const int32_t num_frames);

/*
=======================================
flush_render_device

Runs all submitted render command queues,
blocks until they are all completed
=======================================
*/

void flush_render_device(render_device_t * const device);

/*
=======================================
render_check_fence

Returns true if the renderer has processed
the fence
=======================================
*/

bool render_check_fence(const rb_fence_t fence);

/*
=======================================
render_wait_fence

Waits until the fence has been processed
by the renderer
=======================================
*/

void render_wait_fence(render_device_t * const device, const rb_fence_t fence);

/*
=======================================
render_get_cmd_buf

THREAD SAFE -- can be called from multiple threads.

Gets an available command buffer. If wait_mode
is render_wait then this function will block until a free
command buffer becomes available.
=======================================
*/

EXT_EXPORT rb_cmd_buf_t * render_get_cmd_buf(render_device_t * const device, const render_wait_mode_e wait_mode);

/*
=======================================
render_submit_cmd_buf

THREAD SAFE -- can be called from multiple threads.

Submits a command buffer to the renderer for later processing.
If cmd_buf_order is cmd_buf_ordered then the command buffer is
guaranteed to execute in submit order, otherwise it may be
executed out of order
=======================================
*/

rb_fence_t render_submit_cmd_buf(render_device_t * const device, rb_cmd_buf_t * const cmd_buf, const rb_cmd_buf_order_e cmd_buf_order);

/*
===============================================================================
render_cmd_stream_t

A command stream is an object that supports sending commands via a "stream" paradigm.
Internally the stream holds a cmd_buf and flushes the data as necessary to fit commands.
===============================================================================
*/

/*
=======================================
render_init_cmd_stream
=======================================
*/

static void render_init_cmd_stream(render_device_t * const device, render_cmd_stream_t * cmd_stream) {
    ZEROMEM(cmd_stream);
    cmd_stream->internal.device = device;
    cmd_stream->flush = true;
#ifndef NDEBUG
    cmd_stream->internal.last_fence.cmd_stream = cmd_stream;
#endif
}

/*
=======================================
render_flush_cmd_stream

Submits any unflushed commands in the stream to the renderer
and returns a fence that will not complete until all commands
up to this point are executed.
=======================================
*/

rb_fence_t render_flush_cmd_stream(render_cmd_stream_t * const cmd_stream, const render_wait_mode_e wait_mode);

/*
=======================================
render_get_cmd_stream_fence

Returns a fence that will not complete before all the commands written to the
stream at this point are executed. A fence generated from this function on a command
stream that is not explicitly flushed before the fence is checked or waited should use either

render_conditional_flush_cmd_stream_and_check_fence() OR render_conditional_flush_cmd_stream_and_wait_fence()

to make sure that the stream is flushed if the fence was generated after unflushed commands.
=======================================
*/

rb_fence_t render_get_cmd_stream_fence(render_cmd_stream_t * const cmd_stream);

/*
=======================================
render_conditional_flush_cmd_stream_and_check_fence

Check the fence for completion. This may flush the
command stream if the specified fence was generated
after unsubmitted command data.
=======================================
*/

bool render_conditional_flush_cmd_stream_and_check_fence(render_device_t * const device, render_cmd_stream_t * const cmd_stream, const rb_fence_t fence);

/*
=======================================
render_conditional_flush_cmd_stream_and_wait_fence

Waits on the fence for completion. This may flush the
command stream if the specified fence was generated
after unsubmitted command data.
=======================================
*/

void render_conditional_flush_cmd_stream_and_wait_fence(render_device_t * const device, render_cmd_stream_t * const cmd_stream, const rb_fence_t fence);

/*
=======================================
render_cmd_stream_blocking_latch_cmd_buf

Latches a render command buffer into the specified render_cmd_stream
if one is not already.
=======================================
*/

static rb_cmd_buf_t * render_cmd_stream_blocking_latch_cmd_buf(render_cmd_stream_t * const cmd_stream) {
    if (!cmd_stream->buf) {
        cmd_stream->buf = render_get_cmd_buf(cmd_stream->internal.device, render_wait);
    }

    return cmd_stream->buf;
}

/*
=======================================
RENDER_ENSURE_WRITE_CMD_STREAM

Write a command to a command stream. Command
stream is flushed if necessary, unless flush
is disabled on the stream in which case a program
error occurs.
=======================================
*/

#define RENDER_ENSURE_WRITE_CMD_STREAM(_cmd_stream, _cmd_func, ...)                           \
    if (!(_cmd_func(render_cmd_stream_blocking_latch_cmd_buf(_cmd_stream), __VA_ARGS__))) {   \
        ASSERT((_cmd_stream)->flush);                                                         \
        render_flush_cmd_stream(_cmd_stream, render_no_wait);                                 \
        ASSERT(!(_cmd_stream)->buf);                                                          \
        (_cmd_stream)->buf = render_get_cmd_buf((_cmd_stream)->internal.device, render_wait); \
        VERIFY(_cmd_func((_cmd_stream)->buf, __VA_ARGS__));                                   \
    }                                                                                         \
    ((void)0)

/*
=======================================
RENDER_TRY_WRITE_CMD_STREAM

Write a command to a command stream. This
expression evaluates to the result of the
specified command.
=======================================
*/

#define RENDER_TRY_WRITE_CMD_STREAM(_cmd_stream, _cmd_func, ...) \
    _cmd_func(render_cmd_stream_blocking_latch_cmd_buf(_cmd_stream), __VA_ARGS__)

/*
===============================================================================
Render frontend: streams, textures, programs, etc

The render front end wraps rhi objects into easier to use objects and handles
command buffer details, however it only supports being called from the main
application thread. If you want to render things off-thread you can use
command streams or command buffers directly.

You can query the create/ready status of any render front-end object via:

render_is_resource_ready() and render_wait_for_resource()
===============================================================================
*/

/*
=======================================
render_create_texture_2d

Creates a texture from the specified raw texture data.
The texture data must remain valid until the texture
has been processed by the render-backend. You can
query the status via render_is_resource_ready() or
render_wait_for_resource()
=======================================
*/

typedef struct r_texture_t {
    render_resource_t resource;
    rhi_texture_t * texture;
    int width, height, channels, data_len;
    rhi_pixel_format_e format;
} r_texture_t;

r_texture_t * render_create_texture_2d_no_flush(render_device_t * const device, const image_mips_t mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_desc, const char * const tag);
r_texture_t * render_create_texture_2d(render_device_t * const device, const image_mips_t mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_desc, const char * const tag);

/*
=======================================
render_create_program_from_binary

Create's a program from a precompiled binary blob
that is compatible with the currently bound render
device's RHI.

The program bytecode is copied inline into the
command stream, the region passed in can
be safely freed after this call
=======================================
*/

typedef struct r_program_t {
    render_resource_t resource;
    rhi_program_t * program;
    rhi_error_t * error;
} r_program_t;

r_program_t * render_create_program_from_binary(render_device_t * const device, const const_mem_region_t vert_program, const const_mem_region_t frag_program, const char * const tag);

/*
=======================================
render_create_mesh_data_layout

Copies the mesh layout inline into the command stream.
The mesh_data_layout passed into this function can be
safely freed after this call.
=======================================
*/

typedef struct r_mesh_data_layout_t {
    render_resource_t resource;
    rhi_mesh_data_layout_t * mesh_data_layout;
} r_mesh_data_layout_t;

r_mesh_data_layout_t * render_create_mesh_data_layout(render_device_t * const device, const rhi_mesh_data_layout_desc_t mesh_data_layout_to_copy, const char * const tag);

/*
=======================================
render_create_mesh

mesh init data must remain valid until the mesh resource is ready
=======================================
*/

typedef struct r_mesh_t {
    render_resource_t resource;
    rhi_mesh_t * mesh;
    uint32_t mesh_byte_size;
    uint32_t hash;
} r_mesh_t;

r_mesh_t * render_create_mesh(render_device_t * const device, const rhi_mesh_data_init_indirect_t mesh_data, r_mesh_data_layout_t * mesh_data_layout, const char * const tag);

/*
=======================================
render_create_blend_state
=======================================
*/

typedef struct r_blend_state_t {
    render_resource_t resource;
    rhi_blend_state_t * blend_state;
} r_blend_state_t;

r_blend_state_t * render_create_blend_state(render_device_t * const device, const rhi_blend_state_desc_t desc, const char * const tag);

/*
=======================================
render_create_depth_stencil_state
=======================================
*/

typedef struct r_depth_stencil_state_t {
    render_resource_t resource;
    rhi_depth_stencil_state_t * depth_stencil_state;
} r_depth_stencil_state_t;

r_depth_stencil_state_t * render_create_depth_stencil_state(render_device_t * const device, const rhi_depth_stencil_state_desc_t desc, const char * const tag);

/*
=======================================
render_create_rasterizer_state
=======================================
*/

typedef struct r_rasterizer_state_t {
    render_resource_t resource;
    rhi_rasterizer_state_t * rasterizer_state;
} r_rasterizer_state_t;

r_rasterizer_state_t * render_create_rasterizer_state(render_device_t * const device, const rhi_rasterizer_state_desc_t desc, const char * const tag);

/*
=======================================
render_create_uniform_buffer
=======================================
*/

typedef struct r_uniform_buffer_t {
    render_resource_t resource;
    uint32_t buffer_size;
    rhi_uniform_buffer_t * uniform_buffer;
} r_uniform_buffer_t;

r_uniform_buffer_t * render_create_uniform_buffer(render_device_t * const device, const rhi_uniform_buffer_desc_t desc, const void * const initial_data, const char * const tag);

/*
=======================================
render_cmd_stream_upload_uniform_buffer_data

Uploads data into a uniform buffer. The data
is copied inline into the command buffer.
=======================================
*/

bool render_cmd_stream_upload_uniform_buffer_data_no_flush(render_cmd_stream_t * const cmd_stream, r_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs, const char * const tag);
void render_cmd_stream_upload_uniform_buffer_data(render_cmd_stream_t * const cmd_stream, r_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs, const char * const tag);

/*
=======================================
render_create_render_target
=======================================
*/

typedef struct r_render_target_t {
    render_resource_t resource;
    rhi_render_target_t * render_target;
} r_render_target_t;

r_render_target_t * render_create_render_target(render_device_t * const device, const char * const tag);

/* mesh channel data */

uint32_t render_cmd_stream_upload_mesh_channel_data(
    render_cmd_stream_t * const cmd_stream,
    rhi_mesh_t * const * mesh,
    const int channel_index,
    const int first_elem,
    const int num_elems,
    const size_t stride,
    const void * const data,
    const char * const tag);

#ifdef __cplusplus
}
#endif
