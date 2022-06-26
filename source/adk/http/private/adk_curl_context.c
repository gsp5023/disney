/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/http/private/adk_curl_context.h"

#include "source/adk/runtime/memory.h"
#include "source/adk/steamboat/sb_platform.h"
#include "source/adk/steamboat/sb_thread.h"

#define CURL_NO_OLDIES
#include "curl/curl.h"

// `adk_curl_context` provides a means for partitioning the global allocation strategy of curl into multiple contexts, controlled by the caller.
//
// Notes:
// - On some platforms, curl creates its own thread in order to perform certain operations (e.g. getting hostname) and these operations can allocate memory.
//		In order to handle this case, a global heap is also managed and will be used in the case where the thread-local context is not defined.
//		A reference to the context is stored in the 'fat pointer' that is allocated via the functions below. The context reference is NULL when the global heap is used.
// - In order to allow for multiple 'internal curl' threads to access to global heap, an atomic CAS loop is used to 'lock' the heap and serialize access

enum {
    // `ctx_heap_alignment` is used as the offset (i.e. size) of the curl context pointer that is 'prepended' to each memory allocation in the curl hooks
    // Because we are substituting a malloc implementation and malloc needs to guarantee that memory allocated is suitable for all types,
    // we use an offset of the largest alignment for the platform (i.e. compiler)
    ctx_heap_alignment =
#ifdef __BIGGEST_ALIGNMENT__
        __BIGGEST_ALIGNMENT__,
#else
        sizeof(void *), // used in lieu of C11's `max_align_t`
#endif
};

static THREAD_LOCAL adk_curl_context_t * adk_curl_context = NULL;

static struct {
    uint32_t init_count;
    sb_atomic_int32_t init_hazard;

    heap_t heap;
    mem_region_t region;
    sb_atomic_int32_t heap_hazard;
} statics;

static void init_lock() {
    while (sb_atomic_cas(&statics.init_hazard, 1, 0, memory_order_acquire) != 0) {
    }
}

static void init_unlock() {
    sb_atomic_store(&statics.init_hazard, 0, memory_order_release);
}

static void heap_lock() {
    while (sb_atomic_cas(&statics.heap_hazard, 1, 0, memory_order_acquire) != 0) {
    }
}

static void heap_unlock() {
    sb_atomic_store(&statics.heap_hazard, 0, memory_order_release);
}

struct adk_curl_context_t {
    void * ctx;
    adk_curl_context_callbacks_t callbacks;
};

static void * curl_context_malloc(const size_t size) {
    uint8_t * ptr;
    if (adk_curl_context == NULL) {
        heap_lock();
        ptr = heap_alloc(&statics.heap, size + ctx_heap_alignment, MALLOC_TAG);
        heap_unlock();
    } else {
        ptr = adk_curl_context->callbacks.malloc(adk_curl_context->ctx, size + ctx_heap_alignment, MALLOC_TAG);
    }

    memcpy(ptr, &adk_curl_context, sizeof(adk_curl_context_t *));

    return ptr + ctx_heap_alignment;
}

static void curl_context_free(void * const p) {
    if (p == NULL) {
        return;
    }

    uint8_t * ptr = p;
    ptr -= ctx_heap_alignment;

    adk_curl_context_t * ctx;
    memcpy(&ctx, ptr, sizeof(adk_curl_context_t *));

    if (ctx == NULL) {
        heap_lock();
        heap_free(&statics.heap, ptr, MALLOC_TAG);
        heap_unlock();
    } else {
        ctx->callbacks.free(ctx->ctx, ptr, MALLOC_TAG);
    }
}

static void * curl_context_realloc(void * const p, const size_t size) {
    uint8_t * ptr = p;
    adk_curl_context_t * ctx = adk_curl_context;

    if (ptr != NULL) {
        ptr -= ctx_heap_alignment;
        memcpy(&ctx, ptr, sizeof(adk_curl_context_t *));
    }

    if (ctx == NULL) {
        heap_lock();
        ptr = heap_realloc(&statics.heap, ptr, size + ctx_heap_alignment, MALLOC_TAG);
        heap_unlock();
    } else {
        ptr = ctx->callbacks.realloc(ctx->ctx, ptr, size + ctx_heap_alignment, MALLOC_TAG);
    }

    memcpy(ptr, &ctx, sizeof(adk_curl_context_t *));

    return ptr + ctx_heap_alignment;
}

static char * curl_context_strdup(const char * const str) {
    const size_t len = strlen(str);
    char * const p = curl_context_malloc(len + 1);
    strcpy_s(p, len + 1, str);
    return p;
}

static void * curl_context_calloc(const size_t nmemb, const size_t size) {
    uint8_t * ptr;
    if (adk_curl_context == NULL) {
        heap_lock();
        ptr = heap_calloc(&statics.heap, nmemb * size + ctx_heap_alignment, MALLOC_TAG);
        heap_unlock();
    } else {
        ptr = adk_curl_context->callbacks.calloc(adk_curl_context->ctx, nmemb * size + ctx_heap_alignment, MALLOC_TAG);
    }

    memcpy(ptr, &adk_curl_context, sizeof(adk_curl_context_t *));

    return ptr + ctx_heap_alignment;
}

adk_curl_context_t * adk_curl_context_create(void * const ctx, const adk_curl_context_callbacks_t callbacks) {
    adk_curl_context_t * const curl_context = callbacks.malloc(ctx, sizeof(adk_curl_context_t), MALLOC_TAG);
    ZEROMEM(curl_context);

    curl_context->ctx = ctx;
    curl_context->callbacks = callbacks;

    init_lock();
    if (statics.init_count++ == 0) {
        // TODO: reassess/remove this size once NVE (nve-internal) is no longer using the M5 instance of curl
#ifdef _ADK_NVE_CURL_SHARING
        const size_t buffer_size = 10 * 1024 * 1024;
#else
        const size_t buffer_size = 1024 * 1024;
#endif

        statics.region = sb_map_pages(PAGE_ALIGN_INT(buffer_size), system_page_protect_read_write);
        TRAP_OUT_OF_MEMORY(statics.region.ptr);
        ASSERT_PAGE_ALIGNED(statics.region.ptr);

#ifdef GUARD_PAGE_SUPPORT
        debug_heap_init(&statics.heap, statics.region.size, ctx_heap_alignment, 0, "global-curl", system_guard_page_mode_enabled, MALLOC_TAG);
#else
        heap_init_with_region(&statics.heap, statics.region, ctx_heap_alignment, 0, "global-curl");
#endif
        CURL_PUSH_CTX(NULL);
        VERIFY(curl_global_init_mem(CURL_GLOBAL_ALL, curl_context_malloc, curl_context_free, curl_context_realloc, curl_context_strdup, curl_context_calloc) == CURLE_OK);
        CURL_POP_CTX();
    }
    init_unlock();

    return curl_context;
}

void adk_curl_context_destroy(adk_curl_context_t * const ctx) {
    init_lock();
    if (--statics.init_count == 0) {
        curl_global_cleanup();

        heap_lock();
#ifndef NDEBUG
        heap_debug_print_leaks(&statics.heap);
#endif
        heap_destroy(&statics.heap, MALLOC_TAG);
        heap_unlock();

        sb_unmap_pages(statics.region);
    }
    init_unlock();

    void * const c = ctx->ctx;
    const adk_curl_context_callbacks_t callbacks = ctx->callbacks;

    callbacks.free(c, ctx, MALLOC_TAG);
}

adk_curl_context_t * adk_curl_get_context() {
    return adk_curl_context;
}

adk_curl_context_t * adk_curl_set_context(adk_curl_context_t * const ctx) {
    adk_curl_context_t * const old_ctx = adk_curl_get_context();
    adk_curl_context = ctx;
    return old_ctx;
}
