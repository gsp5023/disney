/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
bifurcated_heap.h

Bifurcated heap support
*/

#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"

typedef struct bifurcated_heap_t {
    heap_t low;
    heap_t high;
    size_t threshold;
} bifurcated_heap_t;

static inline void bifurcated_heap_init(
    bifurcated_heap_t * const heap,
    const mem_region_t low_mem_region,
    const mem_region_t high_mem_region,
    const size_t threshold,
    const system_guard_page_mode_e guard_page_mode,
    const char * const tag) {
#ifdef GUARD_PAGE_SUPPORT
    if (guard_page_mode == system_guard_page_mode_enabled) {
        debug_heap_init(&heap->low, low_mem_region.size, 8, 0, "bifurcated_low_heap", system_guard_page_mode_minimal, tag);
        debug_heap_init(&heap->high, high_mem_region.size, 8, 0, "bifurcated_high_heap", system_guard_page_mode_minimal, tag);
    } else
#endif
    {
        heap_init_with_region(&heap->low, low_mem_region, 8, 0, "bifurcated_low_heap");
        heap_init_with_region(&heap->high, high_mem_region, 8, 0, "bifurcated_high_heap");
    }

    heap->threshold = threshold;
}

static inline void bifurcated_heap_destroy(bifurcated_heap_t * const heap, const char * const tag) {
    heap_destroy(&heap->low, tag);
    heap_destroy(&heap->high, tag);
}

static inline heap_t * bifurcated_heap_select_by_size(bifurcated_heap_t * const heap, const size_t size) {
    return size < heap->threshold ? &heap->low : &heap->high;
}

static inline heap_t * bifurcated_heap_select_by_allocation(bifurcated_heap_t * const heap, void * const ptr) {
    const uintptr_t address = (uintptr_t)ptr;

    const uintptr_t low_heap_base = (uintptr_t)heap->low.internal.head;
    const uintptr_t low_heap_max = low_heap_base + heap->low.internal.heap_size;

    const bool is_low_heap_allocation = (address >= low_heap_base) && (address < low_heap_max);

    heap_t * const result = is_low_heap_allocation ? &heap->low : &heap->high;

#ifndef _SHIP
    if (!is_low_heap_allocation) {
        uintptr_t high_heap_base = (uintptr_t)heap->high.internal.head;
        uintptr_t high_heap_max = high_heap_base + heap->high.internal.heap_size;
        bool is_high_heap_allocation = (address >= high_heap_base) && (address < high_heap_max);
        VERIFY_MSG(is_high_heap_allocation, "ptr doesn't belong to the bifurcated heap");
    }
#endif

    return result;
}

static inline void * bifurcated_heap_alloc(bifurcated_heap_t * const heap, const size_t size, const char * const tag) {
    heap_t * const selected_heap = bifurcated_heap_select_by_size(heap, size);
    void * const ptr = heap_alloc(selected_heap, size, tag);
    return ptr;
}

static inline void * bifurcated_heap_unchecked_alloc(bifurcated_heap_t * const heap, const size_t size, const char * const tag) {
    heap_t * const selected_heap = bifurcated_heap_select_by_size(heap, size);
    void * const ptr = heap_unchecked_alloc(selected_heap, size, tag);
    return ptr;
}

static inline void * bifurcated_heap_calloc(bifurcated_heap_t * const heap, const size_t size, const char * const tag) {
    heap_t * const selected_heap = bifurcated_heap_select_by_size(heap, size);
    void * const ptr = heap_calloc(selected_heap, size, tag);
    return ptr;
}

static inline void * bifurcated_heap_unchecked_calloc(bifurcated_heap_t * const heap, const size_t size, const char * const tag) {
    heap_t * const selected_heap = bifurcated_heap_select_by_size(heap, size);
    void * const ptr = heap_unchecked_calloc(selected_heap, size, tag);
    return ptr;
}

static inline void * bifurcated_heap_unchecked_realloc(bifurcated_heap_t * const heap, void * const ptr, const size_t old_size, const size_t new_size, const char * const tag) {
    heap_t * const old_heap = bifurcated_heap_select_by_size(heap, old_size);
    heap_t * const new_heap = bifurcated_heap_select_by_size(heap, new_size);

    void * result = NULL;
    if (old_heap == new_heap) {
        result = heap_unchecked_realloc(old_heap, ptr, new_size, tag);
    } else {
        result = heap_unchecked_alloc(new_heap, new_size, tag);
        if (ptr) {
            memcpy(result, ptr, old_size);
            heap_free(old_heap, ptr, tag);
        }
    }

    return result;
}

static inline void * bifurcated_heap_realloc(bifurcated_heap_t * const heap, void * const ptr, const size_t new_size, const char * const tag) {
    heap_t * const storage = bifurcated_heap_select_by_allocation(heap, ptr);
    const size_t old_size = heap_get_block_size(storage, ptr);
    ASSERT(old_size < new_size);

    heap_t * const old_heap = bifurcated_heap_select_by_size(heap, old_size);
    heap_t * const new_heap = bifurcated_heap_select_by_size(heap, new_size);

    void * result = NULL;
    if (old_heap == new_heap) {
        result = heap_realloc(old_heap, ptr, new_size, tag);
    } else {
        result = heap_alloc(new_heap, new_size, tag);
        if (ptr) {
            memcpy(result, ptr, old_size);
            heap_free(old_heap, ptr, tag);
        }
    }

    return result;
}

static inline void bifurcated_heap_free(bifurcated_heap_t * const heap, void * const ptr, const char * const tag) {
    ASSERT_MSG(ptr, "attempt to free a NULL ptr (%s)", tag);

    heap_t * const storage = bifurcated_heap_select_by_allocation(heap, ptr);
    heap_free(storage, ptr, tag);
}
