/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rmain.c

Render main
*/

#include _PCH
#include "rhi.h"
#include "rhi_device_api.h"
#include "rhi_private.h"
#include "source/adk/log/log.h"
#include "source/adk/metrics/metrics.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/steamboat/sb_thread.h"

#define RENDER_TAG FOURCC('R', 'N', 'D', 'R')

/*
=======================================
render resources
=======================================
*/

static void render_resource_init(render_resource_t * const resource, render_device_t * const device, const render_resource_vtable_t * const vtable, render_resource_type_e resource_type, const char * const tag) {
    ASSERT(resource_type != render_resource_type_invalid);

    resource->device = device;
    resource->ref_count = 1;
    resource->vtable = vtable;
    resource->tag = tag;
    resource->resource_type = resource_type;

    if (device) {
        render_add_ref(&device->resource);
    }
}

bool render_is_resource_ready(render_resource_t * const resource) {
    ASSERT(resource->device);
    return render_conditional_flush_cmd_stream_and_check_fence(resource->device, &resource->device->default_cmd_stream, resource->fence);
}

void render_wait_for_resource(render_resource_t * const resource) {
    ASSERT(resource->device);
    render_conditional_flush_cmd_stream_and_wait_fence(resource->device, &resource->device->default_cmd_stream, resource->fence);
}

static void render_track_resource_memory_allocation(const render_resource_t * const resource) {
    render_device_t * const device = resource->device;
    if (!device) {
        return;
    }
    switch (resource->resource_type) {
        case render_resource_type_r_texture_t: {
            const r_texture_t * const text = (const r_texture_t *)resource;
            device->resource_tracking.memory_usage.texture_memory += (uint64_t)text->data_len;
            device->resource_tracking.memory_usage.total_memory += (uint64_t)text->data_len;
            break;
        }
        case render_resource_type_r_mesh_t: {
            const r_mesh_t * const mesh = (const r_mesh_t *)resource;
            device->resource_tracking.memory_usage.mesh_memory += mesh->mesh_byte_size;
            device->resource_tracking.memory_usage.total_memory += mesh->mesh_byte_size;
            break;
        }
        case render_resource_type_r_uniform_buffer_t: {
            const r_uniform_buffer_t * const uniform_buffer = (const r_uniform_buffer_t *)resource;
            device->resource_tracking.memory_usage.uniform_buffer_memory += uniform_buffer->buffer_size;
            device->resource_tracking.memory_usage.total_memory += uniform_buffer->buffer_size;
            break;
        }
        default:
            break;
    }
    device->resource_tracking.memory_usage.peak_memory = max_uint64_t(device->resource_tracking.memory_usage.total_memory, device->resource_tracking.memory_usage.peak_memory);
}

static void render_track_resource_memory_release(const render_resource_t * const resource) {
    render_device_t * const device = resource->device;
    if (!device) {
        return;
    }
    switch (resource->resource_type) {
        case render_resource_type_r_texture_t: {
            const r_texture_t * const text = (const r_texture_t *)resource;
            device->resource_tracking.memory_usage.texture_memory -= (uint64_t)text->data_len;
            device->resource_tracking.memory_usage.total_memory -= (uint64_t)text->data_len;
            break;
        }
        case render_resource_type_r_mesh_t: {
            const r_mesh_t * const mesh = (const r_mesh_t *)resource;
            device->resource_tracking.memory_usage.mesh_memory -= mesh->mesh_byte_size;
            device->resource_tracking.memory_usage.total_memory -= mesh->mesh_byte_size;
            break;
        }
        case render_resource_type_r_uniform_buffer_t: {
            const r_uniform_buffer_t * const uniform_buffer = (const r_uniform_buffer_t *)resource;
            device->resource_tracking.memory_usage.uniform_buffer_memory -= uniform_buffer->buffer_size;
            device->resource_tracking.memory_usage.total_memory -= uniform_buffer->buffer_size;
            break;
        }
        default:
            break;
    }
}

int render_release(render_resource_t * const resource, const char * const tag) {
    ASSERT(resource->ref_count > 0);
    const int r = --resource->ref_count;
    if (r < 1) {
        render_device_t * const device = resource->device;
        if (device) {
            render_wait_for_resource(resource);
        }
        if (resource->vtable) {
            render_track_resource_memory_release(resource);
            resource->vtable->destroy(resource, tag);
        }
        if (device) {
            render_release(&device->resource, tag);
        }
    }
    return r;
}

void render_dump_heap_usage(const render_device_t * const device) {
    heap_dump_usage(&device->internal.object_heap);

    device->internal.device->vtable->dump_heap_usage();
}

heap_metrics_t render_get_heap_metrics(const render_device_t * const device) {
    return heap_get_metrics(&device->internal.object_heap);
}

/*
=======================================
render threads and command buffer queue helpers
=======================================
*/

static void enque_cmd_buf(rb_cmd_buf_que_t * const que, rb_cmd_buf_t * const buf) {
    ASSERT(buf->next == NULL);
    if (que->tail) {
        que->tail->next = buf;
    } else {
        ASSERT(que->head == NULL);
        que->head = que->tail = buf;
    }
}

static rb_cmd_buf_t * deque_next_cmd_buf(rb_cmd_buf_que_t * const que) {
    if (que->head) {
        rb_cmd_buf_t * const buf = que->head;
        que->head = que->head->next;
        buf->next = NULL;
        if (!que->head) {
            que->tail = NULL;
        }
        return buf;
    }

    return NULL;
}

// see rbcmd.c for this function
void rb_cmd_buf_execute(rhi_device_t * const device, rb_cmd_buf_t * const cmd_buf);

/*
=======================================
render_get_cmd_buf

THREAD SAFE -- can be called from multiple threads.

Gets an available command buffer. If wait_mode
is render_wait then this function will block until a free
command buffer becomes available.
=======================================
*/

rb_cmd_buf_t * render_get_cmd_buf(render_device_t * const device, const render_wait_mode_e wait_mode) {
    rb_cmd_buf_t * buf = NULL;

    sb_lock_mutex(device->internal.mutex);

    for (;;) {
        buf = device->internal.free_cmd_buf_chain;
        if (buf) {
            device->internal.free_cmd_buf_chain = buf->next;
            buf->next = NULL;
            sb_unlock_mutex(device->internal.mutex);
            return buf;
        } else if (wait_mode == render_wait) {
            if (device->internal.num_device_threads > 0) {
                // there is one or more rendering threads
                // wait until a cmd_buf has been retired.
                sb_wait_condition(device->internal.retired_signal, device->internal.mutex, sb_timeout_infinite);
            } else {
                const sb_thread_id_t this_thread = sb_get_current_thread_id();
                if (this_thread == device->internal.device_thread_id) {
                    // this is happening in the main() application thread
                    // so we can safely run command buffers here.
                    buf = deque_next_cmd_buf(&device->internal.ordered_cmd_buf_que);
                    if (!buf) {
                        buf = deque_next_cmd_buf(&device->internal.unordered_cmd_buf_que);
                    }
                    ASSERT(buf);
                    sb_unlock_mutex(device->internal.mutex);

                    // if we have multiple devices make sure this one is current
                    rhi_thread_make_device_current(device->internal.device);
                    rb_cmd_buf_execute(device->internal.device, buf);
                    sb_atomic_fetch_add(&device->internal.done_count, 1, memory_order_relaxed);
                    // DO NOT SIGNAL retired_signal, we aren't queueing a free buffer here
                    return buf;
                }

                // this is a non-multithreaded device and we are not on the main thread
                // so wait for the main thread to do some work.
                sb_wait_condition(device->internal.retired_signal, device->internal.mutex, sb_timeout_infinite);
            }
        }
    }
}

/*
=======================================
render_submit_cmd_buf

THREAD SAFE -- can be called from multiple threads.

Submits a command buffer to the renderer for later processing
=======================================
*/

static void link_cmd_buf(rb_cmd_buf_que_t * const q, rb_cmd_buf_t * const cmd_buf) {
    ASSERT(cmd_buf->next == NULL);
    if (!q->head) {
        q->head = cmd_buf;
    }
    if (q->tail) {
        q->tail->next = cmd_buf;
    }
    q->tail = cmd_buf;
}

rb_fence_t render_submit_cmd_buf(render_device_t * const device, rb_cmd_buf_t * const cmd_buf, const rb_cmd_buf_order_e cmd_buf_order) {
    rb_fence_t fence = null_rb_fence;
    sb_lock_mutex(device->internal.mutex);

    if (cmd_buf->num_cmds > 0) {
        fence.cmd_buf = cmd_buf;
        fence.counter = ++cmd_buf->submit_counter;

        switch (cmd_buf_order) {
            case cmd_buf_ordered:
                link_cmd_buf(&device->internal.ordered_cmd_buf_que, cmd_buf);
                break;
            case cmd_buf_unordered:
                link_cmd_buf(&device->internal.unordered_cmd_buf_que, cmd_buf);
                break;
            default:
                TRAP("invalid cmd_buf_order");
        }

        sb_unlock_mutex(device->internal.mutex);
        sb_atomic_fetch_add(&device->internal.submit_count, 1, memory_order_relaxed);
        sb_condition_wake_one(device->internal.queued_signal);
    } else {
        cmd_buf->next = device->internal.free_cmd_buf_chain;
        device->internal.free_cmd_buf_chain = cmd_buf;
        sb_unlock_mutex(device->internal.mutex);
        sb_condition_wake_all(device->internal.retired_signal);
    }

    return fence;
}

/*
=======================================
render_proc_exec_cmd_buf

Executes a specific command queue, called from
a rendering thread
=======================================
*/

static int render_proc_exec_cmd_buf(render_device_t * const device, rhi_device_t * const rhi_device, rb_cmd_buf_t * const buf, const int order) {
    if (buf) {
        sb_unlock_mutex(device->internal.mutex);
        rb_cmd_buf_execute(rhi_device, buf);
        sb_lock_mutex(device->internal.mutex);
        sb_atomic_fetch_add(&device->internal.done_count, 1, memory_order_relaxed);
        buf->next = device->internal.free_cmd_buf_chain;
        device->internal.free_cmd_buf_chain = buf;
        sb_condition_wake_all(device->internal.retired_signal);

        return order + 1;
    }

    // wait for queued signal
    sb_wait_condition(device->internal.queued_signal, device->internal.mutex, sb_timeout_infinite);
    return order;
}

/*
=======================================
render_proc_any_order

Entry point for rendering threads that will execute
both in-order and out-of-order command queues
=======================================
*/

static int render_proc_any_order(void * const arg) {
    const render_thread_t * const thread = (const render_thread_t *)arg;
    render_device_t * const device = thread->device;
    rhi_device_t * const rhi_device = device->internal.device;

    unsigned int order = 0;

    rhi_thread_make_device_current(rhi_device);

    sb_lock_mutex(device->internal.mutex);
    while (!device->internal.quit) {
        rb_cmd_buf_t * buf;

        // deque next command buffer
        // swap buffer order so we service both queues

        if (order & 1) {
            buf = deque_next_cmd_buf(&device->internal.unordered_cmd_buf_que);
            if (!buf) {
                buf = deque_next_cmd_buf(&device->internal.ordered_cmd_buf_que);
            }
        } else {
            buf = deque_next_cmd_buf(&device->internal.ordered_cmd_buf_que);
            if (!buf) {
                buf = deque_next_cmd_buf(&device->internal.unordered_cmd_buf_que);
            }
        }

        order = render_proc_exec_cmd_buf(device, rhi_device, buf, order);
    }
    sb_unlock_mutex(device->internal.mutex);
    rhi_thread_device_done_current(rhi_device);
    return 0;
}

/*
=======================================
render_proc_only_out_of_order

Entry point for rendering threads that will execute
ONLY out-of-order command queues
=======================================
*/

static int render_proc_only_out_of_order(void * const arg) {
    const render_thread_t * const thread = (const render_thread_t *)arg;
    render_device_t * const device = thread->device;
    rhi_device_t * const rhi_device = device->internal.device;

    rhi_thread_make_device_current(rhi_device);

    sb_lock_mutex(device->internal.mutex);
    while (!device->internal.quit) {
        rb_cmd_buf_t * const buf = deque_next_cmd_buf(&device->internal.unordered_cmd_buf_que);
        render_proc_exec_cmd_buf(device, rhi_device, buf, 0);
    }
    sb_unlock_mutex(device->internal.mutex);

    rhi_thread_device_done_current(rhi_device);
    return 0;
}

/*
=======================================
destroy_render_device

Destroys the specified rendering device
=======================================
*/

static void destroy_render_device(render_resource_t * resource, const char * const tag) {
    render_device_t * const device = (render_device_t *)resource;
    flush_render_device(device);

    if (device->internal.num_device_threads > 0) {
        sb_lock_mutex(device->internal.mutex);
        device->internal.quit = true;
        sb_unlock_mutex(device->internal.mutex);
        sb_condition_wake_all(device->internal.queued_signal);
        for (render_thread_t * thread = device->internal.threads; thread; thread = thread->next) {
            sb_join_thread(thread->thread);
        }
    } else {
        device->internal.quit = true;
    }

    rhi_release(&device->internal.device->resource, MALLOC_TAG);

    sb_destroy_mutex(device->internal.mutex, MALLOC_TAG);
    sb_destroy_condition_variable(device->internal.queued_signal, MALLOC_TAG);
    sb_destroy_condition_variable(device->internal.retired_signal, MALLOC_TAG);

#ifdef GUARD_PAGE_SUPPORT
    if (device->internal.guard_page_mode == system_guard_page_mode_enabled) {
        // free all cmd pages
        rb_cmd_buf_t * next;
        for (rb_cmd_buf_t * buf = device->internal.free_cmd_buf_chain; buf; buf = next) {
            next = buf->next;
            debug_sys_unmap_page_block(buf->guard_pages, MALLOC_TAG);
        }
        for (rb_cmd_buf_t * buf = device->internal.ordered_cmd_buf_que.head; buf; buf = buf->next) {
            next = buf->next;
            debug_sys_unmap_page_block(buf->guard_pages, MALLOC_TAG);
        }
        for (rb_cmd_buf_t * buf = device->internal.unordered_cmd_buf_que.head; buf; buf = buf->next) {
            next = buf->next;
            debug_sys_unmap_page_block(buf->guard_pages, MALLOC_TAG);
        }
    }
#endif

#ifndef _SHIP
    heap_debug_print_leaks(&device->internal.object_heap);
#endif

    heap_destroy(&device->internal.object_heap, MALLOC_TAG);

#ifdef GUARD_PAGE_SUPPORT
    if (device->internal.guard_page_mode == system_guard_page_mode_enabled) {
        debug_sys_unmap_pages(device->internal.guard_mem, MALLOC_TAG);
    }
#endif
}

/*
=======================================
create_render_device

Create a rendering device from a specific RHI
=======================================
*/

static const render_resource_vtable_t render_device_vtable = {
    .destroy = destroy_render_device};

render_device_t * create_render_device(rhi_api_t * const api, struct sb_window_t * const window, rhi_error_t ** out_error, const mem_region_t _block, const int num_cmd_buffers, const int cmd_buf_size, const int max_threads, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
    rhi_device_t * rhi_device = rhi_create_device(api, window, out_error, tag);
    if (!rhi_device) {
        return NULL;
    }

#ifdef GUARD_PAGE_SUPPORT
    mem_region_t block;
    if (guard_page_mode == system_guard_page_mode_enabled) {
        block = debug_sys_map_pages(PAGE_ALIGN_INT(_block.size), system_page_protect_read_write, MALLOC_TAG);
    } else {
        block = _block;
    }
#else
    const mem_region_t block = _block;
#endif

    const rhi_device_caps_t caps = rhi_get_caps(rhi_device);

    const int num_device_threads = (max_threads < caps.max_device_threads) ? max_threads : caps.max_device_threads;

    // we created the device, now allocate render_device memory
    // how much memory do we need to allocate everything inline in a big ass block?

    const int render_device_size = GET_RENDER_DEVICE_SIZE(num_cmd_buffers, cmd_buf_size, num_device_threads);
    VERIFY(block.size >= (size_t)render_device_size + 1024);

    const int aligned_cmd_buf_data_size = (cmd_buf_size + 7) & ~7;
    const int aligned_cmd_buf_mem_size = aligned_render_cmdbuf_size + aligned_cmd_buf_data_size;

    render_device_t * const device = (render_device_t *)block.ptr;
    ZEROMEM(device);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        device->internal.guard_mem = block;
    }
#endif

    render_resource_init(&device->resource, NULL, &render_device_vtable, render_resource_type_render_device_t, tag);

    device->api = api;
    device->internal.device = rhi_device;
    device->internal.mutex = sb_create_mutex(MALLOC_TAG);
    device->internal.queued_signal = sb_create_condition_variable(MALLOC_TAG);
    device->internal.retired_signal = sb_create_condition_variable(MALLOC_TAG);
    device->internal.device_thread_id = sb_get_current_thread_id();
    device->internal.num_device_threads = num_device_threads;
    device->internal.guard_page_mode = guard_page_mode;

    uint8_t * const cmd_buf_block_start = block.byte_ptr + aligned_render_device_size + (aligned_render_thread_size * num_device_threads);

    // setup command buffers, go backwards to link the free chain forward
#ifdef GUARD_PAGE_SUPPORT
    const int cmd_buf_guard_page_block_size = PAGE_ALIGN_INT(aligned_cmd_buf_mem_size);
#endif

    for (int i = num_cmd_buffers - 1; i >= 0; --i) {
        uint8_t * cmd_buf_base;
#ifdef GUARD_PAGE_SUPPORT
        debug_sys_page_block_t page_block = {0};

        if (guard_page_mode == system_guard_page_mode_enabled) {
            page_block = debug_sys_map_page_block(cmd_buf_guard_page_block_size, system_page_protect_read_write, guard_page_mode, MALLOC_TAG);
            cmd_buf_base = page_block.region.byte_ptr + (cmd_buf_guard_page_block_size - aligned_cmd_buf_mem_size);
        } else
#endif
        {
            cmd_buf_base = cmd_buf_block_start + i * aligned_cmd_buf_mem_size;
        }

        ASSERT_ALIGNED(cmd_buf_base, ALIGN_OF(rb_cmd_buf_t));
        rb_cmd_buf_t * const cmd_buf = (rb_cmd_buf_t *)cmd_buf_base;
        ZEROMEM(cmd_buf);

#ifdef GUARD_PAGE_SUPPORT
        cmd_buf->guard_pages = page_block;
#endif

        hlba_init(&cmd_buf->hlba, cmd_buf_base + aligned_render_cmdbuf_size, aligned_cmd_buf_data_size);

        cmd_buf->next = device->internal.free_cmd_buf_chain;
        device->internal.free_cmd_buf_chain = cmd_buf;
    }

    // spin up rendering threads.
    render_thread_t * const threads = (render_thread_t *)(block.adr + aligned_render_device_size);

    for (int i = num_device_threads - 1; i >= 0; --i) {
        render_thread_t * const thread = &threads[i];

        thread->device = device;
        thread->next = device->internal.threads;
        device->internal.threads = thread;

        if (i != 0) {
            thread->thread = sb_create_thread("m5_render_sec", sb_thread_default_options, render_proc_only_out_of_order, thread, MALLOC_TAG);
        } else {
            // primary thread runs both
            thread->thread = sb_create_thread("m5_render_prim", sb_thread_default_options, render_proc_any_order, thread, MALLOC_TAG);
        }
    }

#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&device->internal.object_heap, block.size - render_device_size, 8, 0, "renderer_heap", guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(
            &device->internal.object_heap,
            MEM_REGION(.adr = block.adr + render_device_size, .size = block.size - render_device_size),
            8,
            0,
            "renderer_heap");
    }

    render_init_cmd_stream(device, &device->default_cmd_stream);

    return device;
}

void render_device_log_resource_tracking(const render_device_t * const render_device, const logging_mode_e logging_mode) {
    if (logging_mode == logging_tty || logging_mode == logging_tty_and_metrics) {
        LOG_ALWAYS(RENDER_TAG,
                   "Render resources:\n"
                   "\tpeak memory:       [%" PRIu64
                   "]\n"
                   "\ttotal memory:      [%" PRIu64
                   "]\n"
                   "\tmesh memory:       [%" PRIu64
                   "]\n"
                   "\ttexture memory:    [%" PRIu64
                   "]\n"
                   "\tuniform buffers:   [%" PRIu64 "]",
                   render_device->resource_tracking.memory_usage.peak_memory,
                   render_device->resource_tracking.memory_usage.total_memory,
                   render_device->resource_tracking.memory_usage.mesh_memory,
                   render_device->resource_tracking.memory_usage.texture_memory,
                   render_device->resource_tracking.memory_usage.uniform_buffer_memory);
    }
    if (logging_mode == logging_metrics || logging_mode == logging_tty_and_metrics) {
        STATIC_ASSERT(sizeof(metrics_render_memory_usage_t) == sizeof(render_memory_usage_t));

        metrics_render_memory_usage_t memory_usage_metric;
        memcpy(&memory_usage_metric, &render_device->resource_tracking.memory_usage, sizeof(render_device->resource_tracking.memory_usage));
        publish_metric(metric_type_metrics_render_memory_usage_t, &memory_usage_metric, sizeof(memory_usage_metric));
    }
}

/*
=======================================
flush_render_device

Runs all submitted render command queues,
blocks until they are all completed
=======================================
*/

static bool wrap_is_less(const int x, const int y) {
    const unsigned int ux = (unsigned int)x;
    const unsigned int uy = (unsigned int)y;
    return (UINT32_MAX / 2) < (ux - uy);
}

static bool wrap_is_lequal(const int x, const int y) {
    return (x == y) || wrap_is_less(x, y);
}

static void flush_cmd_buf_que(render_device_t * const device, rhi_device_t * const rhi_device, rb_cmd_buf_que_t que) {
    RHI_TRACE_PUSH_FN();
    rb_cmd_buf_t * buf = deque_next_cmd_buf(&que);

    while (buf) {
        rb_cmd_buf_execute(rhi_device, buf);

        sb_lock_mutex(device->internal.mutex);
        buf->next = device->internal.free_cmd_buf_chain;
        device->internal.free_cmd_buf_chain = buf;
        sb_unlock_mutex(device->internal.mutex);

        sb_atomic_fetch_add(&device->internal.done_count, 1, memory_order_relaxed);
        sb_condition_wake_one(device->internal.retired_signal);

        buf = deque_next_cmd_buf(&que);
    }
    RHI_TRACE_POP();
}

static void async_wait_render_device(render_device_t * const device) {
    RHI_TRACE_PUSH_FN();
    const int submit_count = sb_atomic_load(&device->internal.submit_count, memory_order_relaxed);
    // wait until complete count passes submit count
    if (wrap_is_less(sb_atomic_load(&device->internal.done_count, memory_order_relaxed), submit_count)) {
        sb_lock_mutex(device->internal.mutex);
        do {
            // wait until a command queue has been retired
            sb_wait_condition(device->internal.retired_signal, device->internal.mutex, sb_timeout_infinite);
        } while (wrap_is_less(sb_atomic_load(&device->internal.done_count, memory_order_relaxed), submit_count));
        sb_unlock_mutex(device->internal.mutex);
    }
    RHI_TRACE_POP();
}

static void flush_device_command_buffers(render_device_t * const device) {
    RHI_TRACE_PUSH_FN();
    sb_lock_mutex(device->internal.mutex);
    const rb_cmd_buf_que_t ordered = device->internal.ordered_cmd_buf_que;
    const rb_cmd_buf_que_t unordered = device->internal.unordered_cmd_buf_que;

    ZEROMEM(&device->internal.ordered_cmd_buf_que);
    ZEROMEM(&device->internal.unordered_cmd_buf_que);
    sb_unlock_mutex(device->internal.mutex);

    // make sure this device is current
    rhi_device_t * const rhi_device = device->internal.device;
    rhi_thread_make_device_current(rhi_device);

    flush_cmd_buf_que(device, rhi_device, ordered);
    flush_cmd_buf_que(device, rhi_device, unordered);
    RHI_TRACE_POP();
}

/*
=======================================
render_device_frame
=======================================
*/

void render_device_frame(render_device_t * const device) {
    RHI_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();
    STATIC_ASSERT(ARRAY_SIZE(device->internal.frame_fences) == render_max_pending_frames + 1);

    const uint32_t next_frame = sb_atomic_load(&device->internal.num_frames, memory_order_relaxed) + 1;
    const uint32_t next_frame_slot = next_frame % (render_max_pending_frames + 1);
    ASSERT(next_frame_slot <= render_max_pending_frames);

    const uint32_t tail_frame_slot = (next_frame_slot + render_max_pending_frames) % (render_max_pending_frames + 1);
    ASSERT(tail_frame_slot <= render_max_pending_frames);

    const rb_fence_t fence = render_flush_cmd_stream(&device->default_cmd_stream, render_no_wait);

    if (device->internal.num_device_threads < 1) {
        // non-threaded device, flush submitted frame
        ASSERT(device->internal.device_thread_id == sb_get_main_thread_id());
        flush_device_command_buffers(device);
    }

    // wait on tail frame
    render_wait_fence(device, device->internal.frame_fences[tail_frame_slot]);
    device->internal.frame_fences[next_frame_slot] = fence;

    sb_atomic_store(&device->internal.num_frames, (int)next_frame, memory_order_relaxed);
    RHI_TRACE_POP();
}

void render_wait_frames(render_device_t * const device, const int32_t num_frames) {
    VERIFY(!sb_is_main_thread());

    const uint32_t u_num_frames = (uint32_t)num_frames;
    const uint32_t start_frame = (uint32_t)sb_atomic_load(&device->internal.num_frames, memory_order_relaxed);

    uint32_t current_frame;

    do {
        for (int i = 0; i < ARRAY_SIZE(device->internal.frame_fences); ++i) {
            render_wait_fence(device, device->internal.frame_fences[i]);
        }
        current_frame = (uint32_t)sb_atomic_load(&device->internal.num_frames, memory_order_relaxed);
    } while ((current_frame - start_frame) < u_num_frames);
}

void flush_render_device(render_device_t * const device) {
    RHI_TRACE_PUSH_FN();
    render_flush_cmd_stream(&device->default_cmd_stream, render_no_wait);

    if (device->internal.num_device_threads > 0) {
        async_wait_render_device(device);
        return;
    }

    // this is not a device that supports threading
    {
        const sb_thread_id_t this_thread = sb_get_current_thread_id();
        if (this_thread != device->internal.device_thread_id) {
            // this is not the main device thread so it cannot run command buffers.
            async_wait_render_device(device);
            return;
        }
    }

    // this is the main device thread
    flush_device_command_buffers(device);
    RHI_TRACE_POP();
}

/*
=======================================
render_check_fence
=======================================
*/

bool render_check_fence(const rb_fence_t fence) {
    if (fence.cmd_buf) {
        return !wrap_is_less(sb_atomic_load(&fence.cmd_buf->retire_counter, memory_order_relaxed), fence.counter);
    }

    return true;
}

/*
=======================================
render_wait_fence
=======================================
*/

void render_wait_fence(render_device_t * const device, const rb_fence_t fence) {
    RHI_TRACE_PUSH_FN();
    if (fence.cmd_buf) {
        if (wrap_is_less(sb_atomic_load(&fence.cmd_buf->retire_counter, memory_order_relaxed), fence.counter)) {
            if (device->internal.num_device_threads > 0) {
                sb_lock_mutex(device->internal.mutex);
                while (wrap_is_less(sb_atomic_load(&fence.cmd_buf->retire_counter, memory_order_relaxed), fence.counter)) {
                    sb_wait_condition(device->internal.retired_signal, device->internal.mutex, sb_timeout_infinite);
                }
                sb_unlock_mutex(device->internal.mutex);
            } else {
                flush_device_command_buffers(device);
            }
        }
    }
    RHI_TRACE_POP();
}

/*
===============================================================================
render_cmd_stream_t

A command stream is an object that supports sending commands via a "stream" paradigm.
Internally the stream holds a cmd_buf and flushes the data as necessary to fit commands.
===============================================================================
*/

rb_fence_t render_flush_cmd_stream(render_cmd_stream_t * const cmd_stream, const render_wait_mode_e wait_mode) {
    RHI_TRACE_PUSH_FN();
    if (cmd_stream->buf) {
        if (cmd_stream->buf->num_cmds > 0) {
            const rb_fence_t fence = render_submit_cmd_buf(cmd_stream->internal.device, cmd_stream->buf, cmd_buf_ordered);
            cmd_stream->buf = NULL;
            cmd_stream->internal.last_fence = fence;
#ifndef NDEBUG
            cmd_stream->internal.last_fence.cmd_stream = cmd_stream;
#endif
            if (wait_mode == render_wait) {
                render_wait_fence(cmd_stream->internal.device, fence);
            }
        } else {
            // will just return buf to free-list
            render_submit_cmd_buf(cmd_stream->internal.device, cmd_stream->buf, cmd_buf_unordered);
            cmd_stream->buf = NULL;
        }
    }
    RHI_TRACE_POP();
    return cmd_stream->internal.last_fence;
}

rb_fence_t render_get_cmd_stream_fence(render_cmd_stream_t * const cmd_stream) {
    rb_fence_t fence;

    if (cmd_stream->buf && (cmd_stream->buf->num_cmds > 0)) {
        // we are currently building a cmd_buf, so emit a fence that will
        // sync after the cmd_buf is flushed
        fence.cmd_buf = cmd_stream->buf;
        fence.counter = cmd_stream->buf->submit_counter + 1;
#ifndef NDEBUG
        fence.cmd_stream = cmd_stream;
#endif
    } else {
        // otherwise sync to last point in stream
        fence = cmd_stream->internal.last_fence;
    }

    return fence;
}

/*
=======================================
render_check_cmd_stream_fence
=======================================
*/

bool render_conditional_flush_cmd_stream_and_check_fence(render_device_t * const device, render_cmd_stream_t * const cmd_stream, const rb_fence_t fence) {
    // check if we need to flush the command stream or not
    // if the fence we are checking is bound to the same
    // command buffer then either the fence is done or
    // the stream needs to be flushed.
    ASSERT((fence.cmd_stream == NULL) || (fence.cmd_stream == cmd_stream));
    RHI_TRACE_PUSH_FN();

    if (fence.cmd_buf && (fence.cmd_buf == cmd_stream->buf)) {
        if (wrap_is_less(fence.cmd_buf->submit_counter, fence.counter)) {
            // flush
            const rb_fence_t new_fence = render_flush_cmd_stream(cmd_stream, render_wait);
            (void)new_fence;
            ASSERT(new_fence.cmd_buf == fence.cmd_buf);
            ASSERT(new_fence.counter == fence.counter);
            RHI_TRACE_POP();
            return false;
        }
        // command buffers are equal but the submit_counter on the command buffer
        // has since been incremented which means it made a full trip through the
        // rendering command queue, ended up back in the free-list and bound to
        // this command stream.
        RHI_TRACE_POP();
        return true;
    }

    RHI_TRACE_POP();
    return render_check_fence(fence);
}

void render_conditional_flush_cmd_stream_and_wait_fence(render_device_t * const device, render_cmd_stream_t * const cmd_stream, const rb_fence_t fence) {
    // check if we need to flush the command stream or not
    // if the fence we are checking is bound to the same
    // command buffer then either the fence is done or
    // the stream needs to be flushed.
    ASSERT((fence.cmd_stream == NULL) || (fence.cmd_stream == cmd_stream));
    RHI_TRACE_PUSH_FN();

    if (fence.cmd_buf && (fence.cmd_buf == cmd_stream->buf)) {
        if (wrap_is_less(fence.cmd_buf->submit_counter, fence.counter)) {
            // flush
            const rb_fence_t new_fence = render_flush_cmd_stream(cmd_stream, render_wait);
            (void)new_fence;
            ASSERT(new_fence.cmd_buf == fence.cmd_buf);
            ASSERT(new_fence.counter == fence.counter);
            RHI_TRACE_POP();
            return;
        }
        // command buffers are equal but the submit_counter on the command buffer
        // has since been incremented which means it made a full trip through the
        // rendering command queue, ended up back in the free-list and bound to
        // this command stream.
        RHI_TRACE_POP();
        return;
    }

    render_wait_fence(device, fence);
    RHI_TRACE_POP();
}

/*
=======================================
render_cmd_stream_write_release_rhi_resource

Writes an rhi_release command into the specified command stream.
=======================================
*/

static bool render_cmd_stream_write_release_rhi_resource_no_flush(render_cmd_stream_t * const cmd_stream, rhi_resource_t * const rhi_resource, const char * const tag) {
#ifdef RHI_NULL_DEVICE
    return true;
#else
    ASSERT(rhi_resource);

    render_cmd_stream_blocking_latch_cmd_buf(cmd_stream);
    render_mark_cmd_buf(cmd_stream->buf);

    rhi_resource_t ** rhi_resource_ptr = (rhi_resource_t **)render_cmd_buf_unchecked_alloc(cmd_stream->buf, 8, sizeof(rhi_resource_t *));
    if (!rhi_resource_ptr || !render_cmd_buf_write_release_rhi_resource_indirect(cmd_stream->buf, rhi_resource_ptr, tag)) {
        render_unwind_cmd_buf(cmd_stream->buf);
        return false;
    }

    *rhi_resource_ptr = rhi_resource;
    return true;
#endif
}

static void render_cmd_stream_write_release_rhi_resource(render_cmd_stream_t * const cmd_stream, rhi_resource_t * const rhi_resource, const char * const tag) {
    if (!render_cmd_stream_write_release_rhi_resource_no_flush(cmd_stream, rhi_resource, tag)) {
        render_flush_cmd_stream(cmd_stream, render_no_wait);
        VERIFY(render_cmd_stream_write_release_rhi_resource_no_flush(cmd_stream, rhi_resource, tag));
    }
}

/*
===============================================================================
Render frontend: streams, textures, programs, etc
===============================================================================
*/
#define RESOURCE_VTABLE(_type) TOKENPASTE(_type, _vtable)

#define MAKE_RENDER_RESOURCE(_type, _rhi_resource_field)                                                   \
    static void TOKENPASTE(render_destroy_, _type)(render_resource_t * resource, const char * const tag) { \
        render_cmd_stream_write_release_rhi_resource(                                                      \
            &resource->device->default_cmd_stream,                                                         \
            (rhi_resource_t *)((_type *)resource)->_rhi_resource_field,                                    \
            tag);                                                                                          \
        heap_free(&resource->device->internal.object_heap, resource, tag);                                 \
    }                                                                                                      \
    static const render_resource_vtable_t RESOURCE_VTABLE(_type) = {                                       \
        .destroy = TOKENPASTE(render_destroy_, _type)};

/*
=======================================
render_create_texture_2d
=======================================
*/

MAKE_RENDER_RESOURCE(r_texture_t, texture)

static uint32_t render_estimate_texture_size_on_gpu(const image_mips_t * const mipmaps) {
    return (uint32_t)(mipmaps->levels[0].data_len + (mipmaps->num_levels > 1 ? mipmaps->levels[0].data_len / 2 : 0));
}

r_texture_t * render_create_texture_2d_no_flush(render_device_t * const device, const image_mips_t mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_state, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();
    ASSERT(mipmaps.levels[0].data_len > 0);
    r_texture_t * const texture = (r_texture_t *)heap_calloc(&device->internal.object_heap, sizeof(r_texture_t), tag);
    render_resource_init(&texture->resource, device, &RESOURCE_VTABLE(r_texture_t), render_resource_type_r_texture_t, tag);

    texture->format = format;
    texture->width = mipmaps.levels[0].width;
    texture->height = mipmaps.levels[0].height;
    texture->channels = mipmaps.levels[0].bpp;
    texture->data_len = render_estimate_texture_size_on_gpu(&mipmaps);

    if (!render_cmd_buf_write_create_texture_2d(device->default_cmd_stream.buf, &texture->texture, mipmaps, format, usage, sampler_state, tag)) {
        heap_free(&device->internal.object_heap, texture, MALLOC_TAG);
        return NULL;
    }

    texture->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    render_track_resource_memory_allocation(&texture->resource);

    return texture;
}

r_texture_t * render_create_texture_2d(render_device_t * const device, const image_mips_t mipmaps, const rhi_pixel_format_e format, const rhi_usage_e usage, const rhi_sampler_state_desc_t sampler_state, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();
    ASSERT(mipmaps.levels[0].data_len > 0);

    r_texture_t * const texture = (r_texture_t *)heap_calloc(&device->internal.object_heap, sizeof(r_texture_t), tag);
    render_resource_init(&texture->resource, device, &RESOURCE_VTABLE(r_texture_t), render_resource_type_r_texture_t, tag);

    texture->format = format;
    texture->width = mipmaps.levels[0].width;
    texture->height = mipmaps.levels[0].height;
    texture->channels = mipmaps.levels[0].bpp;
    texture->data_len = render_estimate_texture_size_on_gpu(&mipmaps);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_texture_2d,
        &texture->texture,
        mipmaps,
        format,
        usage,
        sampler_state,
        tag);

    texture->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    render_track_resource_memory_allocation(&texture->resource);

    return texture;
}

/*
=======================================
render_create_program_from_binary
=======================================
*/

static void render_destroy_program(render_resource_t * resource, const char * const tag) {
    r_program_t * const program = (r_program_t *)resource;
    if (program->error) {
        render_cmd_stream_write_release_rhi_resource(
            &resource->device->default_cmd_stream,
            &program->error->resource,
            tag);
    }

    render_cmd_stream_write_release_rhi_resource(
        &resource->device->default_cmd_stream,
        (rhi_resource_t *)((r_program_t *)resource)->program,
        tag);

    heap_free(&resource->device->internal.object_heap, resource, tag);
}

static void render_program_dump_usage(const render_resource_t * const resource) {
}

static const render_resource_vtable_t r_program_vtable = {
    .destroy = render_destroy_program};

r_program_t * render_create_program_from_binary(render_device_t * const device, const const_mem_region_t vert_program, const const_mem_region_t frag_program, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_program_t * const program = (r_program_t *)heap_calloc(&device->internal.object_heap, sizeof(r_program_t), tag);
    render_resource_init(&program->resource, device, &r_program_vtable, render_resource_type_r_program_t, tag);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_program_from_binary,
        &program->program,
        &program->error,
        vert_program,
        frag_program,
        tag);

    return program;
}

/*
=======================================
render_create_mesh_data_layout
=======================================
*/

MAKE_RENDER_RESOURCE(r_mesh_data_layout_t, mesh_data_layout)

static bool copy_mesh_data_layout_desc_to_buffer(void * const buffer, const rhi_mesh_data_layout_desc_t * const mesh_data_layout_to_copy, rhi_mesh_data_layout_desc_t * out) {
    if (!buffer) {
        return false;
    }

    ZEROMEM(out);
    out->num_channels = mesh_data_layout_to_copy->num_channels;

    uint8_t * base = (uint8_t *)buffer;
    if (mesh_data_layout_to_copy->indices) {
        out->indices = (rhi_index_layout_desc_t *)base;
        memcpy(base, mesh_data_layout_to_copy->indices, sizeof(rhi_index_layout_desc_t));
        base += sizeof(rhi_index_layout_desc_t);
    }

    out->channels = (rhi_vertex_layout_desc_t *)base;
    ASSERT_ALIGNED(out->channels, ALIGN_OF(rhi_vertex_layout_desc_t));
    base += sizeof(rhi_vertex_layout_desc_t) * mesh_data_layout_to_copy->num_channels;

    for (int i = 0; i < mesh_data_layout_to_copy->num_channels; ++i) {
        ASSERT(mesh_data_layout_to_copy->channels[i].stride > 0);
        ((rhi_vertex_layout_desc_t *)out->channels)[i] = mesh_data_layout_to_copy->channels[i];
        ((rhi_vertex_layout_desc_t *)out->channels)[i].elements = (rhi_vertex_element_desc_t *)base;
        memcpy((void *)out->channels[i].elements, mesh_data_layout_to_copy->channels[i].elements, sizeof(rhi_vertex_element_desc_t) * mesh_data_layout_to_copy->channels[i].num_elements);
        base += sizeof(rhi_vertex_element_desc_t) * mesh_data_layout_to_copy->channels[i].num_elements;
    }

    return true;
}

r_mesh_data_layout_t * render_create_mesh_data_layout(render_device_t * const device, const rhi_mesh_data_layout_desc_t mesh_data_layout_to_copy, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_mesh_data_layout_t * const mesh_data_layout = (r_mesh_data_layout_t *)heap_calloc(&device->internal.object_heap, sizeof(r_mesh_data_layout_t), tag);
    render_resource_init(&mesh_data_layout->resource, device, &RESOURCE_VTABLE(r_mesh_data_layout_t), render_resource_type_r_mesh_data_layout_t, tag);

    int struct_size = 0;

    if (mesh_data_layout_to_copy.indices) {
        struct_size += sizeof(rhi_index_layout_desc_t);
    }

    for (int i = 0; i < mesh_data_layout_to_copy.num_channels; ++i) {
        struct_size += sizeof(rhi_vertex_layout_desc_t) + sizeof(rhi_vertex_element_desc_t) * mesh_data_layout_to_copy.channels[i].num_elements;
    }

    render_cmd_stream_blocking_latch_cmd_buf(&device->default_cmd_stream);
    render_mark_cmd_buf(device->default_cmd_stream.buf);

    void * buffer = render_cmd_buf_unchecked_alloc(device->default_cmd_stream.buf, 8, struct_size);
    rhi_mesh_data_layout_desc_t desc;

    if (!copy_mesh_data_layout_desc_to_buffer(buffer, &mesh_data_layout_to_copy, &desc) || !render_cmd_buf_write_create_mesh_data_layout(device->default_cmd_stream.buf, &mesh_data_layout->mesh_data_layout, desc, tag)) {
        render_unwind_cmd_buf(device->default_cmd_stream.buf);
        render_flush_cmd_stream(&device->default_cmd_stream, render_no_wait);
        render_cmd_stream_blocking_latch_cmd_buf(&device->default_cmd_stream);
        buffer = render_cmd_buf_alloc(device->default_cmd_stream.buf, 8, struct_size);
        copy_mesh_data_layout_desc_to_buffer(buffer, &mesh_data_layout_to_copy, &desc);
        VERIFY(render_cmd_buf_write_create_mesh_data_layout(device->default_cmd_stream.buf, &mesh_data_layout->mesh_data_layout, desc, tag));
    }

    mesh_data_layout->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    return mesh_data_layout;
}

/*
=======================================
render_create_mesh
=======================================
*/

static uint32_t render_estimate_mesh_size_on_gpu(const rhi_mesh_data_init_indirect_t mesh_data) {
    uint32_t memory_usage = mesh_data.num_indices;
    for (int i = 0; i < mesh_data.num_channels; ++i) {
        memory_usage += (uint32_t)mesh_data.channels[i].size;
    }
    return memory_usage;
}

MAKE_RENDER_RESOURCE(r_mesh_t, mesh)

r_mesh_t * render_create_mesh(render_device_t * const device, const rhi_mesh_data_init_indirect_t mesh_data, r_mesh_data_layout_t * mesh_data_layout, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_mesh_t * const mesh = (r_mesh_t *)heap_calloc(&device->internal.object_heap, sizeof(r_mesh_t), tag);
    render_resource_init(&mesh->resource, device, &RESOURCE_VTABLE(r_mesh_t), render_resource_type_r_mesh_t, tag);

    rhi_mesh_data_init_indirect_t mesh_init_data_copy = mesh_data;
    if (mesh_data_layout) {
        mesh_init_data_copy.layout = &mesh_data_layout->mesh_data_layout;
    }

    render_cmd_stream_blocking_latch_cmd_buf(&device->default_cmd_stream);
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_mesh_indirect,
        &mesh->mesh,
        mesh_init_data_copy,
        tag);

    mesh->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    mesh->mesh_byte_size = render_estimate_mesh_size_on_gpu(mesh_data);
    render_track_resource_memory_allocation(&mesh->resource);

    return mesh;
}

/*
=======================================
render_create_blend_state
=======================================
*/

MAKE_RENDER_RESOURCE(r_blend_state_t, blend_state)

r_blend_state_t * render_create_blend_state(render_device_t * const device, const rhi_blend_state_desc_t desc, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_blend_state_t * const blend_state = (r_blend_state_t *)heap_calloc(&device->internal.object_heap, sizeof(r_blend_state_t), tag);
    render_resource_init(&blend_state->resource, device, &RESOURCE_VTABLE(r_blend_state_t), render_resource_type_r_blend_state_t, tag);

    render_cmd_stream_blocking_latch_cmd_buf(&device->default_cmd_stream);
    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_blend_state,
        &blend_state->blend_state,
        desc,
        tag);

    blend_state->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    return blend_state;
}

/*
=======================================
render_create_depth_stencil_state
=======================================
*/

MAKE_RENDER_RESOURCE(r_depth_stencil_state_t, depth_stencil_state)

r_depth_stencil_state_t * render_create_depth_stencil_state(render_device_t * const device, const rhi_depth_stencil_state_desc_t desc, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_depth_stencil_state_t * const depth_stencil_state = (r_depth_stencil_state_t *)heap_calloc(&device->internal.object_heap, sizeof(r_depth_stencil_state_t), tag);
    render_resource_init(&depth_stencil_state->resource, device, &RESOURCE_VTABLE(r_depth_stencil_state_t), render_resource_type_r_depth_stencil_state_t, tag);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_depth_stencil_state,
        &depth_stencil_state->depth_stencil_state,
        desc,
        tag);

    depth_stencil_state->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    return depth_stencil_state;
}

/*
=======================================
render_create_rasterizer_state
=======================================
*/

MAKE_RENDER_RESOURCE(r_rasterizer_state_t, rasterizer_state)

r_rasterizer_state_t * render_create_rasterizer_state(render_device_t * const device, const rhi_rasterizer_state_desc_t desc, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_rasterizer_state_t * const rasterizer_state = (r_rasterizer_state_t *)heap_calloc(&device->internal.object_heap, sizeof(r_rasterizer_state_t), tag);
    render_resource_init(&rasterizer_state->resource, device, &RESOURCE_VTABLE(r_rasterizer_state_t), render_resource_type_r_rasterizer_state_t, tag);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_rasterizer_state,
        &rasterizer_state->rasterizer_state,
        desc,
        tag);

    rasterizer_state->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    return rasterizer_state;
}

/*
=======================================
render_create_uniform_buffer
=======================================
*/

MAKE_RENDER_RESOURCE(r_uniform_buffer_t, uniform_buffer)

r_uniform_buffer_t * render_create_uniform_buffer(render_device_t * const device, const rhi_uniform_buffer_desc_t desc, const void * const initial_data, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_uniform_buffer_t * const uniform_buffer = (r_uniform_buffer_t *)heap_calloc(&device->internal.object_heap, sizeof(r_uniform_buffer_t), tag);
    render_resource_init(&uniform_buffer->resource, device, &RESOURCE_VTABLE(r_uniform_buffer_t), render_resource_type_r_uniform_buffer_t, tag);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_uniform_buffer,
        &uniform_buffer->uniform_buffer,
        desc,
        initial_data,
        tag);

    uniform_buffer->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    uniform_buffer->buffer_size = desc.size;
    render_track_resource_memory_allocation(&uniform_buffer->resource);

    return uniform_buffer;
}

bool render_cmd_stream_upload_uniform_buffer_data_no_flush(render_cmd_stream_t * const cmd_stream, r_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs, const char * const tag) {
    render_cmd_stream_blocking_latch_cmd_buf(cmd_stream);
    render_mark_cmd_buf(cmd_stream->buf);

    void * p = render_cmd_buf_unchecked_alloc(cmd_stream->buf, 8, (int)data.size);
    if (!p || !render_cmd_buf_write_upload_uniform_data_indirect(cmd_stream->buf, &uniform_buffer->uniform_buffer, CONST_MEM_REGION(.ptr = p, .size = data.size), ofs, tag)) {
        render_unwind_cmd_buf(cmd_stream->buf);
        return false;
    }

    memcpy(p, data.ptr, data.size);
    return true;
}

void render_cmd_stream_upload_uniform_buffer_data(render_cmd_stream_t * const cmd_stream, r_uniform_buffer_t * const uniform_buffer, const const_mem_region_t data, const int ofs, const char * const tag) {
    if (!render_cmd_stream_upload_uniform_buffer_data_no_flush(cmd_stream, uniform_buffer, data, ofs, tag)) {
        render_flush_cmd_stream(cmd_stream, render_no_wait);
        VERIFY(render_cmd_stream_upload_uniform_buffer_data_no_flush(cmd_stream, uniform_buffer, data, ofs, tag));
    }
}

/*
=======================================
render_create_render_target
=======================================
*/

MAKE_RENDER_RESOURCE(r_render_target_t, render_target)

r_render_target_t * render_create_render_target(render_device_t * const device, const char * const tag) {
    ASSERT_IS_MAIN_THREAD();

    r_render_target_t * const render_target = (r_render_target_t *)heap_calloc(&device->internal.object_heap, sizeof(r_render_target_t), tag);
    render_resource_init(&render_target->resource, device, &RESOURCE_VTABLE(r_render_target_t), render_resource_type_r_render_target_t, tag);

    RENDER_ENSURE_WRITE_CMD_STREAM(
        &device->default_cmd_stream,
        render_cmd_buf_write_create_render_target,
        &render_target->render_target,
        tag);

    render_target->resource.fence = render_get_cmd_stream_fence(&device->default_cmd_stream);

    return render_target;
}

/* mesh channel data */

uint32_t render_cmd_stream_upload_mesh_channel_data(
    render_cmd_stream_t * const cmd_stream,
    rhi_mesh_t * const * mesh,
    const int channel_index,
    const int first_elem,
    const int num_elems,
    const size_t stride,
    const void * const data,
    const char * const tag) {
    uint32_t hash = 0;
    if (render_cmd_get_is_rhi_command_diffing_enabled()) {
        const size_t offset = first_elem * stride;
        const size_t byte_length = num_elems * stride;
        hash = rbcmd_hash_bytes(channel_index, (const uint8_t *)data + offset, byte_length);
    }

    RENDER_ENSURE_WRITE_CMD_STREAM(
        cmd_stream,
        render_cmd_buf_write_upload_mesh_channel_data_indirect,
        mesh,
        channel_index,
        first_elem,
        num_elems,
        data,
        hash,
        tag);

    return hash;
}
