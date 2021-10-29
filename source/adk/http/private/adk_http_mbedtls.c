/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
http_mbedtls.c
mbedtls heap / custom allocation setup.
*/

#include "mbedtls/platform.h"
#include "source/adk/http/adk_http.h"
#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_thread.h"

static struct {
    heap_t heap;
    sb_mutex_t * mutex;
} statics;

static void * mbedtls_heap_calloc(size_t num_elements, size_t element_size) {
    sb_lock_mutex(statics.mutex);
    void * const new_ptr = heap_calloc(&statics.heap, num_elements * element_size, MALLOC_TAG);
    sb_unlock_mutex(statics.mutex);
    return new_ptr;
}

static void mbedtls_heap_free(void * ptr) {
    if (ptr) {
        sb_lock_mutex(statics.mutex);
        heap_free(&statics.heap, ptr, MALLOC_TAG);
        sb_unlock_mutex(statics.mutex);
    }
}

//forward declare this to avoid implicit declaration after moving define to enable this func to premake
int mbedtls_platform_set_calloc_free(void * (*calloc_func)(size_t, size_t), void (*free_func)(void *));

bool adk_mbedtls_init(const mem_region_t region, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
    statics.mutex = sb_create_mutex(tag);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode != system_guard_page_mode_disabled) {
        debug_heap_init(&statics.heap, region.size, 8, 0, "adk_http_mbedtls_heap", guard_page_mode, tag);
    } else
#endif
    {
        heap_init_with_region(&statics.heap, region, 8, 0, "adk_http_mbedtls_heap");
    }

    if (mbedtls_platform_set_calloc_free(&mbedtls_heap_calloc, &mbedtls_heap_free) != 0) {
        return false;
    }

    return true;
}

void adk_mbedtls_shutdown(const char * const tag) {
    heap_destroy(&statics.heap, tag);
    sb_destroy_mutex(statics.mutex, tag);
    // TODO this shouldn't need to be done and should be removed as part of
    // https://jira.disneystreaming.com/browse/M5-2867
    mbedtls_platform_set_calloc_free(&MBEDTLS_PLATFORM_STD_CALLOC, &MBEDTLS_PLATFORM_STD_FREE);
}
