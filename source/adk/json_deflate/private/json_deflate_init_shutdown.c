/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 json_deflate_init_shutdown.c

 JSON deflate init/shutdown operations.
 */

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/private/json_deflate.h"
#include "source/adk/log/log.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_file.h"
#include "source/adk/steamboat/sb_thread.h"

static struct {
    heap_t heap;
    sb_mutex_t * mutex;
    json_deflate_options_t hooks;
    thread_pool_t * pool;
    system_guard_page_mode_e guard_mode;

    sb_mutex_t * parallel_mutex;
    sb_condition_variable_t * cv;
} statics;

thread_pool_t * json_deflate_get_pool(void) {
    return statics.pool;
}

sb_mutex_t * json_deflate_get_parallel_mutex(void) {
    return statics.parallel_mutex;
}

sb_condition_variable_t * json_deflate_get_cv(void) {
    return statics.cv;
}

void * json_deflate_calloc(const size_t nmemb, const size_t size) {
    return statics.hooks.calloc_hook(nmemb, size);
}

void * json_deflate_unchecked_calloc(const size_t nmemb, const size_t size) {
    return statics.hooks.unchecked_calloc_hook(nmemb, size);
}

void * json_deflate_malloc(const size_t size) {
    return statics.hooks.malloc_hook(size);
}

void * json_deflate_realloc(void * const ptr, const size_t size) {
    return statics.hooks.realloc_hook(ptr, size);
}

void json_deflate_free(void * const ptr) {
    statics.hooks.free_hook(ptr);
}

void * json_deflate_calloc_device(const size_t nmemb, const size_t size) {
    sb_lock_mutex(statics.mutex);
    void * const r = heap_calloc(&statics.heap, nmemb * size, MALLOC_TAG);
    sb_unlock_mutex(statics.mutex);
    return r;
}

void * json_deflate_unchecked_calloc_device(const size_t nmemb, const size_t size) {
    sb_lock_mutex(statics.mutex);
    void * const r = heap_unchecked_calloc(&statics.heap, nmemb * size, MALLOC_TAG);
    sb_unlock_mutex(statics.mutex);
    return r;
}

void * json_deflate_malloc_device(const size_t size) {
    sb_lock_mutex(statics.mutex);
    void * const r = heap_alloc(&statics.heap, size, MALLOC_TAG);
    sb_unlock_mutex(statics.mutex);
    return r;
}

void * json_deflate_realloc_device(void * const ptr, const size_t size) {
    sb_lock_mutex(statics.mutex);
    void * const r = heap_realloc(&statics.heap, ptr, size, MALLOC_TAG);
    sb_unlock_mutex(statics.mutex);
    return r;
}

void json_deflate_free_device(void * const ptr) {
    if (ptr) {
        sb_lock_mutex(statics.mutex);
        heap_free(&statics.heap, ptr, MALLOC_TAG);
        sb_unlock_mutex(statics.mutex);
    }
}

system_guard_page_mode_e json_deflate_get_guard_mode(void) {
    return statics.guard_mode;
}

void json_deflate_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, thread_pool_t * const thread_pool) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();

    ASSERT_MSG(thread_pool, "[json_deflate] A thread pool must be provided");

    statics.mutex = sb_create_mutex(MALLOC_TAG);
    statics.parallel_mutex = sb_create_mutex(MALLOC_TAG);
    statics.pool = thread_pool;
    statics.cv = sb_create_condition_variable(MALLOC_TAG);

#ifdef GUARD_PAGE_SUPPORT
    statics.guard_mode = guard_page_mode;
    if (guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&statics.heap, region.size, 8, 0, "json_deflate_heap", guard_page_mode, MALLOC_TAG);
    } else
#endif
    {
        heap_init_with_region(&statics.heap, region, 8, 0, "json_deflate_heap");
    }
    json_deflate_options_t memory_hooks = {
        .calloc_hook = json_deflate_calloc_device,
        .unchecked_calloc_hook = json_deflate_unchecked_calloc_device,
        .malloc_hook = json_deflate_malloc_device,
        .realloc_hook = json_deflate_realloc_device,
        .free_hook = json_deflate_free_device,
    };
    json_deflate_init_hooks(&memory_hooks);
    JSON_DEFLATE_TRACE_POP();
}

void json_deflate_init_hooks(const json_deflate_options_t * const memory_hooks) {
    memcpy(&statics.hooks, memory_hooks, sizeof(*memory_hooks));
}

void json_deflate_shutdown(void) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT_IS_MAIN_THREAD();

    LOG_DEBUG(TAG_JSON_DEFLATE, "JSON deflate shutting down");

#ifndef NDEBUG
    heap_debug_print_leaks(&statics.heap);
#endif

    heap_destroy(&statics.heap, MALLOC_TAG);
    sb_destroy_condition_variable(statics.cv, MALLOC_TAG);
    sb_destroy_mutex(statics.mutex, MALLOC_TAG);
    sb_destroy_mutex(statics.parallel_mutex, MALLOC_TAG);
    JSON_DEFLATE_TRACE_POP();
}

void json_deflate_dump_heap_usage(void) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    sb_lock_mutex(statics.mutex);
    heap_dump_usage(&statics.heap);
    sb_unlock_mutex(statics.mutex);
    JSON_DEFLATE_TRACE_POP();
}

heap_metrics_t json_deflate_get_heap_metrics(void) {
    return heap_get_metrics(&statics.heap);
}