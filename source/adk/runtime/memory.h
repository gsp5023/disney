/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
memory.h

Memory routines
*/

#include "runtime.h"
#ifdef _MAT_ENABLED
#include "source/adk/steamboat/private/mat.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
    memory_name_tag_max_size = 32
};

/*
===============================================================================
System page memory allocation
===============================================================================
*/

EXT_EXPORT PURE int get_sys_page_size();

#define FWD_PAGE_ALIGN_INT(_x) FWD_ALIGN_INT(_x, get_sys_page_size())
#define FWD_PAGE_ALIGN_PTR(_x) FWD_ALIGN_PTR(_x, get_sys_page_size())
#define REV_PAGE_ALIGN_INT(_x) REV_ALIGN_INT(_x, get_sys_page_size())
#define REV_PAGE_ALIGN_PTR(_x) REV_ALIGN_PTR(_x, get_sys_page_size())
#define PAGE_ALIGN_INT(_x) FWD_PAGE_ALIGN_INT(_x)
#define PAGE_ALIGN_PTR(_x) FWD_PAGE_ALIGN_PTR(_x)
#define IS_PAGE_ALIGNED(_x) (!!(_x) && (IS_ALIGNED(_x, get_sys_page_size())))

#define ASSERT_PAGE_ALIGNED(_x) ASSERT(IS_PAGE_ALIGNED(_x))
#define VERIFY_PAGE_ALIGNED(_x) VERIFY(IS_PAGE_ALIGNED(_x))

#define TRAP_HEAP_OUT_OF_MEMORY(_ptr, _heap, _alloc_size, _tag) VERIFY_MSG(_ptr, "Heap [%s] out of memory!\nheap_size: %zu\nheap_used: %zu\nheap_free: %zu\nalloc size: %zu\n%s: originating tag", (_heap)->name, (_heap)->internal.heap_size, (_heap)->internal.used_block_size, (_heap)->internal.free_block_size, _alloc_size, _tag);

#ifdef DEBUG_PAGE_MEMORY_SERVICES
/*
=======================================
debug_sys_map_pages

Maps a block of pages into process memory with
the specified page access mode.
=======================================
*/

mem_region_t debug_sys_map_pages(const size_t size, const system_page_protect_e protect, const char * const tag);

/*
=======================================
debug_sys_protect_pages

Changes the page protect mode of the specified
pages.
=======================================
*/

void debug_sys_protect_pages(const mem_region_t pages, const system_page_protect_e protect);

/*
=======================================
debug_sys_unmap_pages

Unmaps the specified pages from process memory.
=======================================
*/

void debug_sys_unmap_pages(const mem_region_t pages, const char * const tag);

typedef struct debug_sys_page_block_t {
    mem_region_t internal;
    mem_region_t region;
} debug_sys_page_block_t;

debug_sys_page_block_t debug_sys_map_page_block(const size_t size, const system_page_protect_e protect, const system_guard_page_mode_e guard_page_mode, const char * const tag);
void debug_sys_unmap_page_block(const debug_sys_page_block_t block, const char * const tag);

#endif

/*
===============================================================================
linear_block_allocator_t
===============================================================================
*/

typedef struct linear_block_allocator_t {
    char name[memory_name_tag_max_size];
    void * block;
    int ofs;
    int size;
} linear_block_allocator_t;

static inline void lba_init(linear_block_allocator_t * const lba, void * const block, const int size, const char * const name) {
    lba->block = block;
    lba->ofs = 0;
    lba->size = size;

    sprintf_s(lba->name, ARRAY_SIZE(lba->name), "%s", name);
}

static void * lba_allocate_unchecked(linear_block_allocator_t * const lba, const int alignment, const int size) {
    // align offset
    const int aligned_ofs = ALIGN_INT(lba->ofs, alignment);
    if ((aligned_ofs + size) > lba->size) {
        return NULL;
    }

    lba->ofs = aligned_ofs + size;
    return ((uint8_t *)lba->block) + aligned_ofs;
}

static void * lba_allocate(linear_block_allocator_t * const lba, const int alignment, const int size) {
    void * const ptr = lba_allocate_unchecked(lba, alignment, size);
    VERIFY_MSG(ptr, "LBA [%s] out of memory\nlba_size: %i\nlba_used: %i\nalloc_size: %i", lba->name, lba->size, lba->ofs, size);
    return ptr;
}

static inline void lba_reset(linear_block_allocator_t * const lba) {
    lba->ofs = 0;
}

/*
===============================================================================
high_low_block_allocator_t
===============================================================================
*/

typedef struct high_low_block_allocator_t {
    void * block;
    int ofs_low;
    int ofs_high;
    int size;
} high_low_block_allocator_t;

static inline void hlba_init(high_low_block_allocator_t * const hlba, void * const block, const int size) {
    hlba->block = block;
    hlba->ofs_low = 0;
    hlba->ofs_high = size;
    hlba->size = size;
}

static void * hlba_allocate_low(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    // align offset
    const int aligned_ofs = ALIGN_INT(hlba->ofs_low, alignment);
    if ((aligned_ofs + size) > hlba->ofs_high) {
        return NULL;
    }

    hlba->ofs_low = aligned_ofs + size;
    return ((uint8_t *)hlba->block) + aligned_ofs;
}

static void * hlba_allocate_high(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    // align offset
    const int bottom = hlba->ofs_high - size;
    const int aligned_ofs = REV_ALIGN_INT(bottom, alignment);
    ASSERT((bottom < 0) == (aligned_ofs < 0));
    if (aligned_ofs < hlba->ofs_low) {
        return NULL;
    }

    hlba->ofs_high = aligned_ofs;
    return ((uint8_t *)hlba->block) + aligned_ofs;
}

static inline void * hlba_allocate_low_high(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    void * p = hlba_allocate_low(hlba, alignment, size);
    if (p) {
        return p;
    }

    return hlba_allocate_high(hlba, alignment, size);
}

static inline void * hlba_allocate_high_low(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    void * p = hlba_allocate_high(hlba, alignment, size);
    if (p) {
        return p;
    }

    return hlba_allocate_low(hlba, alignment, size);
}

static inline void * hlba_allocate_low_or_die(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    void * const p = hlba_allocate_low(hlba, alignment, size);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

static inline void * hlba_allocate_high_or_die(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    void * const p = hlba_allocate_high(hlba, alignment, size);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

static inline void * hlba_allocate_low_high_or_die(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    void * const p = hlba_allocate_low_high(hlba, alignment, size);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

static inline void * hlba_allocate_high_low_or_die(high_low_block_allocator_t * const hlba, const int alignment, const int size) {
    void * const p = hlba_allocate_high_low(hlba, alignment, size);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

static inline void hlba_reset(high_low_block_allocator_t * const hlba) {
    hlba->ofs_low = 0;
    hlba->ofs_high = hlba->size;
}

/*
===============================================================================
heap_t

block based memory allocator.
===============================================================================
*/

#if _MACHINE_SIZE == 32
static const uint32_t heap_block_id_used = 0xdeadb1cf;
static const uint32_t heap_block_id_free = 0xfc1bdae0;
#else
static const uint64_t heap_block_id_used = 0xdeadb1cfdeadb1cfull;
static const uint64_t heap_block_id_free = 0xfc1bdae0fc1bdae0ull;
#endif

typedef struct heap_block_header_t {
    size_t id;
    const char * tag;
    struct heap_block_header_t * next;
#ifdef GUARD_PAGE_SUPPORT
    union {
        struct heap_block_header_t * next_on_free;
        struct heap_block_header_t * prev;
    };
#else
    struct heap_block_header_t * next_on_free;
#endif
    size_t size;
#ifdef GUARD_PAGE_SUPPORT
    size_t guard_page_total_size;
    size_t original_total_allocated_page_size;
#endif
} heap_block_header_t;

FFI_EXPORT
typedef struct heap_metrics_t {
    uint64_t alignment;
    uint64_t ptr_ofs;
    uint64_t header_size;
    uint64_t extra_header_bytes;
    uint64_t min_block_size;
    uint64_t heap_size;
    uint64_t num_used_blocks;
    uint64_t num_free_blocks;
    uint64_t used_block_size;
    uint64_t free_block_size;
    uint64_t num_merged_blocks;
    uint64_t max_used_size;
} heap_metrics_t;

typedef struct heap_t {
    char name[memory_name_tag_max_size];
    struct {
        heap_block_header_t * free;
        heap_block_header_t * head;
        size_t alignment;
        size_t ptr_ofs;
        size_t header_size;
        size_t extra_header_bytes;
        size_t min_block_size;
        size_t heap_size;
        size_t num_used_blocks;
        size_t num_free_blocks;
        size_t used_block_size;
        size_t free_block_size;
        size_t num_merged_blocks;
        size_t max_used_size;
#ifdef DEBUG_PAGE_MEMORY_SERVICES
        debug_sys_page_block_t pages_to_free_on_destroy;
#endif
#ifdef GUARD_PAGE_SUPPORT
        size_t guard_pages_max_heap_size;
        bool guard_pages;
#endif
        bool debug;
        bool init;
#ifdef _MAT_ENABLED
        mat_pool_t mat_pool_id;
#endif
    } internal;
} heap_t;

EXT_EXPORT void heap_init_with_region(heap_t * const heap, const mem_region_t region, const int alignment, const int block_header_size, const char * const name);
heap_t * heap_emplace_init_with_region(const mem_region_t region, const int alignment, const int block_header_size, const char * const name);

#ifdef DEBUG_PAGE_MEMORY_SERVICES
void debug_heap_init(heap_t * const heap, const size_t heap_size, const int alignment, const int block_header_size, const char * const name, const system_guard_page_mode_e guard_pages, const char * const tag);
heap_t * debug_heap_emplace_init(const size_t heap_size, const int alignment, const int block_header_size, const char * const name, const system_guard_page_mode_e guard_pages, const char * const tag);
void debug_heap_init_with_pages_to_free_on_destroy(heap_t * const heap, const debug_sys_page_block_t pages, const int alignment, const int block_header_size, const char * const name);
heap_t * debug_heap_emplace_init_with_pages_to_free_on_destroy(const debug_sys_page_block_t pages, const int alignment, const int block_header_size, const char * const name);
#endif

EXT_EXPORT void heap_destroy(heap_t * const heap, const char * const tag);

EXT_EXPORT void * heap_unchecked_alloc(heap_t * const heap, const size_t size, const char * const tag);
EXT_EXPORT void * heap_unchecked_calloc(heap_t * const heap, size_t size, const char * const tag);
EXT_EXPORT void * heap_unchecked_realloc(heap_t * const heap, void * const ptr, const size_t size, const char * const tag);
EXT_EXPORT void heap_free(heap_t * const heap, void * const ptr, const char * const tag);

void heap_walk(const heap_t * const heap, void (*const callback)(const heap_t * const heap, const heap_block_header_t * const block, void * const user), void * const user);
void heap_verify(const heap_t * const heap);
void heap_verify_ptr(const heap_t * const heap, const void * const ptr);
void heap_verify_block(const heap_t * const heap, const heap_block_header_t * const block);
EXT_EXPORT void heap_debug_print_leaks(const heap_t * const heap);
void heap_dump_usage(const heap_t * const heap);

heap_metrics_t heap_get_metrics(const heap_t * const heap);

static inline void heap_enable_debug_checks(heap_t * const heap, const bool enable) {
    ASSERT(heap->internal.init);
    heap->internal.debug = enable;
}

static inline void * heap_alloc(heap_t * const heap, const size_t size, const char * const tag) {
    void * const ptr = heap_unchecked_alloc(heap, size, tag);
    TRAP_HEAP_OUT_OF_MEMORY(ptr, heap, size, tag);
    heap->internal.max_used_size = (heap->internal.max_used_size < heap->internal.used_block_size) ? heap->internal.used_block_size : heap->internal.max_used_size;
    return ptr;
}

static inline void * heap_calloc(heap_t * const heap, size_t size, const char * const tag) {
    void * const ptr = heap_unchecked_calloc(heap, size, tag);
    TRAP_HEAP_OUT_OF_MEMORY(ptr, heap, size, tag);
    heap->internal.max_used_size = (heap->internal.max_used_size < heap->internal.used_block_size) ? heap->internal.used_block_size : heap->internal.max_used_size;
    return ptr;
}

static inline void * heap_realloc(heap_t * const heap, void * const ptr, const size_t size, const char * const tag) {
    void * const ptr2 = heap_unchecked_realloc(heap, ptr, size, tag);
    TRAP_HEAP_OUT_OF_MEMORY(ptr2, heap, size, tag);
    heap->internal.max_used_size = (heap->internal.max_used_size < heap->internal.used_block_size) ? heap->internal.used_block_size : heap->internal.max_used_size;
    return ptr2;
}

static inline heap_block_header_t * heap_get_block_header(const heap_t * const heap, const void * const ptr) {
    ASSERT(heap->internal.init);
    const uintptr_t uptr = (uintptr_t)(ptr);
    return (heap_block_header_t *)(uptr - heap->internal.ptr_ofs);
}

static inline bool heap_is_free_block(const heap_block_header_t * const block_header) {
    return block_header->id == heap_block_id_free;
}

static inline bool heap_is_used_block(const heap_block_header_t * const block_header) {
    return block_header->id == heap_block_id_used;
}

static inline bool heap_is_valid_block(const heap_block_header_t * block_header) {
    return heap_is_free_block(block_header) || heap_is_used_block(block_header);
}

static inline size_t heap_get_block_size(const heap_t * const heap, void * const ptr) {
    const heap_block_header_t * const block_header = heap_get_block_header(heap, ptr);
    ASSERT(heap_is_valid_block(block_header));
    return block_header->size - heap->internal.ptr_ofs;
}

static const char * heap_get_block_tag(const heap_t * const heap, void * const ptr) {
    const heap_block_header_t * const block_header = heap_get_block_header(heap, ptr);
    ASSERT(heap_is_valid_block(block_header));
    return block_header->tag;
}

/*
===============================================================================
memory_pool_t

fixed size memory pool
===============================================================================
*/

typedef struct memory_pool_block_header_t {
    size_t id;
    const char * tag;
#ifdef GUARD_PAGE_SUPPORT
    struct memory_pool_block_header_t * prev_on_free;
    struct memory_pool_block_header_t * next_on_free;
    struct memory_pool_block_header_t * prev_on_head;
    struct memory_pool_block_header_t * next_on_head;
#else
    struct memory_pool_block_header_t * next;
#endif
} memory_pool_block_header_t;

typedef struct memory_pool_t {
    struct {
        memory_pool_block_header_t * free;
        memory_pool_block_header_t * head;
        size_t alignment;
        size_t ptr_ofs;
        size_t header_size;
        size_t extra_header_bytes;
        size_t block_size;
        size_t user_size;
        size_t num_used_blocks;
        size_t num_free_blocks;
        size_t num_blocks;
        uintptr_t start;
        uintptr_t end;
#ifdef DEBUG_PAGE_MEMORY_SERVICES
        debug_sys_page_block_t pages_to_free_on_destroy;
#endif
#ifdef GUARD_PAGE_SUPPORT
        mem_region_t pages;
#endif
        bool debug;
        bool init;
    } internal;
} memory_pool_t;

void memory_pool_init_with_region(memory_pool_t * const pool, const mem_region_t block, const int block_size, const int alignment, const int block_header_size);
memory_pool_t * memory_pool_emplace_init_with_region(const mem_region_t region, const int block_size, const int alignment, const int block_header_size);

#ifdef DEBUG_PAGE_MEMORY_SERVICES
void debug_memory_pool_init(memory_pool_t * const pool, const size_t pool_size, const int block_size, const int alignment, const int block_header_size, const system_guard_page_mode_e guard_pages, const char * const tag);
void debug_memory_pool_init_with_pages_to_free_on_destroy(memory_pool_t * const pool, const debug_sys_page_block_t pages, const int block_size, const int alignment, const int block_header_size);
memory_pool_t * debug_memory_pool_emplace_init(const size_t pool_size, const int block_size, const int alignment, const int block_header_size, const system_guard_page_mode_e guard_pages, const char * const tag);
memory_pool_t * debug_memory_pool_emplace_init_with_pages_to_free_on_destroy(const debug_sys_page_block_t pages, const int block_size, const int alignment, const int block_header_size);
#endif

void memory_pool_destroy(memory_pool_t * const pool, const char * const tag);

EXT_EXPORT void * memory_pool_unchecked_alloc(memory_pool_t * const pool, const char * const tag);
void memory_pool_free(memory_pool_t * const pool, void * const ptr, const char * const tag);

void memory_pool_walk(const memory_pool_t * const pool, void (*const callback)(const memory_pool_t * const pool, const memory_pool_block_header_t * const block, void * const user), void * const user);
void memory_pool_verify(const memory_pool_t * const pool);
void memory_pool_verify_block(const memory_pool_t * const pool, const memory_pool_block_header_t * const block);
void memory_pool_debug_print_leaks(const memory_pool_t * const pool, const char * const pool_name);

static void * memory_pool_unchecked_calloc(memory_pool_t * const pool, const char * const tag) {
    void * const p = memory_pool_unchecked_alloc(pool, tag);
    if (p) {
        memset(p, 0, pool->internal.user_size);
    }
    return p;
}

static inline void * memory_pool_alloc(memory_pool_t * const pool, const char * const tag) {
    void * const p = memory_pool_unchecked_alloc(pool, tag);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

static inline void * memory_pool_calloc(memory_pool_t * const pool, const char * const tag) {
    void * const p = memory_pool_unchecked_calloc(pool, tag);
    TRAP_OUT_OF_MEMORY(p);
    return p;
}

static inline void memory_pool_enable_debug_checks(memory_pool_t * const pool, const bool enable) {
    ASSERT(pool->internal.init);
    pool->internal.debug = enable;
}

static inline bool memory_pool_is_free_block(const memory_pool_block_header_t * const block) {
    return block->id == heap_block_id_free;
}

static inline bool memory_pool_is_used_block(const memory_pool_block_header_t * const block) {
    return block->id == heap_block_id_used;
}

static inline memory_pool_block_header_t * memory_pool_get_block_header(const memory_pool_t * const pool, void * const ptr) {
    uintptr_t uptr = (uintptr_t)ptr;
    return (memory_pool_block_header_t *)(uptr - pool->internal.ptr_ofs);
}

static inline void * memory_pool_get_ptr(const memory_pool_t * const pool, memory_pool_block_header_t * const block) {
    return (void *)((uintptr_t)block + pool->internal.ptr_ofs);
}

static inline bool memory_pool_is_valid_block(const memory_pool_t * const pool, const memory_pool_block_header_t * const block) {
    const uintptr_t uptr = (uintptr_t)block;
    return (uptr >= pool->internal.start) && (uptr < pool->internal.end) && (memory_pool_is_free_block(block) || memory_pool_is_used_block(block));
}

static inline bool memory_pool_is_valid_ptr(const memory_pool_t * const pool, void * const ptr) {
    const uintptr_t uptr = (uintptr_t)ptr;
    return (uptr >= pool->internal.start) && (uptr < pool->internal.end) && memory_pool_is_valid_block(pool, memory_pool_get_block_header(pool, ptr));
}

static inline void memory_pool_verify_ptr(const memory_pool_t * const pool, void * const ptr) {
    ASSERT(pool->internal.init);
    VERIFY(memory_pool_is_valid_ptr(pool, ptr));
}

PURE static size_t memory_pool_get_required_memory_size(const size_t num_blocks, const size_t block_size, const size_t alignment, const size_t block_header_size) {
    ASSERT_POW2(alignment);
    const size_t ptr_ofs = ALIGN_INT(max_size_t(block_header_size, sizeof(memory_pool_block_header_t)), alignment);
    const size_t aligned_block_size = ALIGN_INT(block_size + ptr_ofs, alignment);
    return aligned_block_size * num_blocks;
}

PURE static size_t memory_pool_get_block_count(size_t memory_size, size_t block_size, size_t alignment, size_t block_header_size) {
    ASSERT_POW2(alignment);
    const size_t ptr_ofs = ALIGN_INT(max_size_t(block_header_size, sizeof(memory_pool_block_header_t)), alignment);
    const size_t aligned_block_size = ALIGN_INT(block_size + ptr_ofs, alignment);
    return memory_size / aligned_block_size;
}

#ifdef __cplusplus
}
#endif
