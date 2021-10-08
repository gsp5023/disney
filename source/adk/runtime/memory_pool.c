/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
memory_pool.c

memory pool
*/

#include _PCH

#include "memory.h"
#include "source/adk/log/log.h"

#define TAG_MEMORY_POOL FOURCC('M', 'P', 'O', 'L')

#ifdef GUARD_PAGE_SUPPORT
static void memory_pool_init_with_guard_pages(memory_pool_t * const pool, const size_t pool_size, const int block_size, const int alignment, const int block_header_size, const char * const tag) {
    // This requires a relatively _massive_ amount of memory
    // compared to non-guard page use, but the idea here is to
    // reserve the number of blocks that would normally be available
    // for use, but expanded to include no_access pages between each
    // block.

    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const int clamped_block_header_size = max_int(block_header_size, sizeof(memory_pool_block_header_t));
    const int ptr_ofs = ALIGN_INT(clamped_block_header_size, clamped_alignment);
    const int aligned_block_size = ALIGN_INT(block_size + ptr_ofs, clamped_alignment);
    const size_t extra_header_bytes = clamped_block_header_size - sizeof(memory_pool_block_header_t);

    const size_t page_size = get_sys_page_size();

    // NOTE: we don't round up pool_size here so we can simulate the non-guard page
    // path block count.
    const size_t num_blocks = pool_size / aligned_block_size;
    const size_t block_pages = (aligned_block_size + page_size - 1) / page_size;
    const size_t total_pages_per_block = 1 + block_pages;
    const size_t total_pages = 1 + total_pages_per_block * num_blocks;
    const size_t total_page_size = total_pages * page_size;

    const mem_region_t pages = debug_sys_map_pages(total_page_size, system_page_protect_no_access, tag);
    TRAP_OUT_OF_MEMORY(pages.ptr);

    const size_t start = pages.adr + page_size;
    const size_t end = start + total_page_size - page_size;

    memory_pool_block_header_t * prev = NULL;

    for (size_t i = 0; i < num_blocks; ++i) {
        const uintptr_t block_base = start + i * total_pages_per_block * page_size;
        const uintptr_t block_guard_page = block_base + block_pages * page_size;

        debug_sys_protect_pages(MEM_REGION(.adr = block_base, .size = block_pages * page_size), system_page_protect_read_write);

        const uintptr_t ublock = block_guard_page - aligned_block_size;
        memory_pool_block_header_t * const block = (memory_pool_block_header_t *)ublock;

        block->id = heap_block_id_free;
        block->tag = NULL;
        block->prev_on_free = prev;
        block->next_on_free = NULL;
        block->prev_on_head = prev;
        block->next_on_head = NULL;

        if (extra_header_bytes) {
            memset((void *)(ublock + sizeof(memory_pool_block_header_t)), 0, extra_header_bytes);
        }

        if (prev) {
            prev->next_on_free = block;
            prev->next_on_head = block;
        } else {
            pool->internal.free = block;
            pool->internal.head = block;
        }

        prev = block;
    }

    pool->internal.init = true;
    pool->internal.alignment = clamped_alignment;
    pool->internal.header_size = clamped_block_header_size;
    pool->internal.extra_header_bytes = extra_header_bytes;
    pool->internal.ptr_ofs = ptr_ofs;
    pool->internal.block_size = aligned_block_size;
    pool->internal.user_size = aligned_block_size - ptr_ofs;
    pool->internal.pages = pages;
    pool->internal.start = start;
    pool->internal.end = end;
    pool->internal.num_blocks = num_blocks;
    pool->internal.num_free_blocks = num_blocks;
}
#endif

#ifdef DEBUG_PAGE_MEMORY_SERVICES
void debug_memory_pool_init(memory_pool_t * const pool, const size_t pool_size, const int block_size, const int alignment, const int block_header_size, const system_guard_page_mode_e guard_pages, const char * const tag) {
    ZEROMEM(pool);
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(memory_pool_block_header_t)));

    const size_t aligned_pool_size = PAGE_ALIGN_INT(pool_size);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_pages == system_guard_page_mode_enabled) {
        memory_pool_init_with_guard_pages(pool, aligned_pool_size, block_size, alignment, block_header_size, tag);
    } else
#endif
    {
        const debug_sys_page_block_t pages = debug_sys_map_page_block(aligned_pool_size, system_page_protect_read_write, guard_pages, tag);
        TRAP_OUT_OF_MEMORY(pages.region.ptr);
        debug_memory_pool_init_with_pages_to_free_on_destroy(pool, pages, block_size, alignment, block_header_size);
    }
}

void debug_memory_pool_init_with_pages_to_free_on_destroy(memory_pool_t * const pool, const debug_sys_page_block_t pages, const int block_size, const int alignment, const int block_header_size) {
    memory_pool_init_with_region(pool, pages.region, block_size, alignment, block_header_size);
    pool->internal.pages_to_free_on_destroy = pages;
}

memory_pool_t * debug_memory_pool_emplace_init(const size_t pool_size, const int block_size, const int alignment, const int block_header_size, const system_guard_page_mode_e guard_pages, const char * const tag) {
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(memory_pool_block_header_t)));

    // Q: should we require the caller to have aligned the pool_size to a multiple of the page size?
    // A: maybe... how does that work if i want to hard-code the minimum pool size? Since the page
    //    size is determined at runtime and may not be necessarily be known at compile time...
    const size_t aligned_pool_size = PAGE_ALIGN_INT(pool_size);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_pages == system_guard_page_mode_enabled) {
        const int page_size = get_sys_page_size();
        ASSERT(page_size > (int)sizeof(memory_pool_t));

        const debug_sys_page_block_t pages = debug_sys_map_page_block(page_size, system_page_protect_read_write, system_guard_page_mode_minimal, tag);
        TRAP_OUT_OF_MEMORY(pages.region.ptr);

        // this is trying to simulate the non-guard-page path in terms of ending
        // up with the same number of blocks and such.
        const int clamped_alignment = max_int(alignment, sizeof(void *));
        const size_t aligned_pool_struct_size = ALIGN_INT(sizeof(memory_pool_t), clamped_alignment);
        ASSERT(aligned_pool_size > aligned_pool_struct_size);

        // don't used the aligned_pool_struct_size to place the pool structure:
        // we want the compiler-sized structure to raise an exception if we over-run
        // and that may not happen if we use extra padding
        memory_pool_t * const pool = (memory_pool_t *)((uintptr_t)pages.region.ptr + page_size - sizeof(memory_pool_t));
        ZEROMEM(pool);
        // simulate non-guard-page path, which shrinks the pool size by the aligned structure size.
        const size_t new_pool_size = aligned_pool_size - aligned_pool_struct_size;

        memory_pool_init_with_guard_pages(pool, new_pool_size, block_size, alignment, block_header_size, tag);
        pool->internal.pages_to_free_on_destroy = pages;

        return pool;
    } else
#endif
    {
        const debug_sys_page_block_t pages = debug_sys_map_page_block(aligned_pool_size, system_page_protect_read_write, guard_pages, tag);
        TRAP_OUT_OF_MEMORY(pages.region.ptr);
        return debug_memory_pool_emplace_init_with_pages_to_free_on_destroy(pages, block_size, alignment, block_header_size);
    }
}

memory_pool_t * debug_memory_pool_emplace_init_with_pages_to_free_on_destroy(const debug_sys_page_block_t pages, const int block_size, const int alignment, const int block_header_size) {
    memory_pool_t * pool = memory_pool_emplace_init_with_region(pages.region, block_size, alignment, block_header_size);
    pool->internal.pages_to_free_on_destroy = pages;
    return pool;
}

#endif

void memory_pool_init_with_region(memory_pool_t * const pool, const mem_region_t region, const int block_size, const int alignment, const int block_header_size) {
    ZEROMEM(pool);
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(memory_pool_block_header_t)));

    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const int clamped_block_header_size = max_int(block_header_size, sizeof(memory_pool_block_header_t));
    const int extra_header_bytes = clamped_block_header_size - sizeof(memory_pool_block_header_t);
    const size_t ptr_ofs = ALIGN_INT(clamped_block_header_size, clamped_alignment);
    const size_t aligned_block_size = ALIGN_INT(block_size + ptr_ofs, clamped_alignment);
    const size_t num_blocks = region.size / aligned_block_size;

    ASSERT_ALIGNED(region.ptr, clamped_alignment);

    const uintptr_t base = (uintptr_t)region.ptr;
    memory_pool_block_header_t * prev = NULL;

    for (size_t i = 0; i < num_blocks; ++i) {
        const uintptr_t ublock = base + i * aligned_block_size;
        memory_pool_block_header_t * const free_block = (memory_pool_block_header_t *)ublock;

        free_block->id = heap_block_id_free;
        free_block->tag = NULL;

#ifdef GUARD_PAGE_SUPPORT
        free_block->prev_on_free = NULL;
        free_block->next_on_free = NULL;
        free_block->prev_on_head = NULL;
        free_block->next_on_head = NULL;

        if (prev) {
            prev->next_on_free = free_block;
        } else {
            pool->internal.free = free_block;
            pool->internal.head = free_block;
        }
#else
        free_block->next = NULL;
        if (prev) {
            prev->next = free_block;
        } else {
            pool->internal.free = free_block;
            pool->internal.head = free_block;
        }
#endif

        if (extra_header_bytes) {
            memset((void *)(ublock + sizeof(memory_pool_block_header_t)), 0, extra_header_bytes);
        }

        prev = free_block;
    }

    pool->internal.init = true;
    pool->internal.header_size = clamped_block_header_size;
    pool->internal.extra_header_bytes = extra_header_bytes;
    pool->internal.ptr_ofs = ptr_ofs;
    pool->internal.block_size = aligned_block_size;
    pool->internal.user_size = aligned_block_size - ptr_ofs;
    pool->internal.start = base;
    pool->internal.end = base + num_blocks * aligned_block_size;
    pool->internal.num_blocks = num_blocks;
    pool->internal.num_free_blocks = num_blocks;
}

memory_pool_t * memory_pool_emplace_init_with_region(const mem_region_t region, const int block_size, const int alignment, const int block_header_size) {
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(memory_pool_block_header_t)));

    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const size_t aligned_pool_struct_size = ALIGN_INT(sizeof(memory_pool_t), clamped_alignment);
    ASSERT(region.size > aligned_pool_struct_size);
    ASSERT_ALIGNED(region.ptr, clamped_alignment);

    memory_pool_t * const pool = (memory_pool_t *)region.ptr;

    const size_t new_pool_size = region.size - aligned_pool_struct_size;
    memory_pool_init_with_region(pool, MEM_REGION(.adr = region.adr + aligned_pool_struct_size, .size = new_pool_size), block_size, alignment, block_header_size);
    return pool;
}

void memory_pool_destroy(memory_pool_t * const pool, const char * const tag) {
    ASSERT(pool->internal.init);

    if (pool->internal.debug) {
        memory_pool_verify(pool);
    }

    pool->internal.init = false;

#ifdef GUARD_PAGE_SUPPORT
    if (pool->internal.pages.ptr) {
        debug_sys_unmap_pages(pool->internal.pages, MALLOC_TAG);
    }
#endif
}

void * memory_pool_unchecked_alloc(memory_pool_t * const pool, const char * const tag) {
    ASSERT(pool->internal.init);
    ASSERT(tag);

    const bool debug = pool->internal.debug;
    if (debug) {
        memory_pool_verify(pool);
    }

    const size_t ptr_ofs = pool->internal.ptr_ofs;
    memory_pool_block_header_t * const block = pool->internal.free;

#ifdef GUARD_PAGE_SUPPORT
    if (pool->internal.pages.ptr) {
        if (block) {
            ASSERT(memory_pool_is_free_block(block));

            if (block->next_on_free) {
                block->next_on_free->prev_on_free = NULL;
            }

            pool->internal.free = block->next_on_free;

            block->tag = tag;
            block->id = heap_block_id_used;

            if (debug) {
                memory_pool_verify_block(pool, block);
            }

            ASSERT(pool->internal.num_free_blocks > 0);
            --pool->internal.num_free_blocks;
            ++pool->internal.num_used_blocks;

            return (void *)((uintptr_t)block + ptr_ofs);
        }
    } else
#endif
    {
        if (block) {
            ASSERT(memory_pool_is_free_block(block));
#ifdef GUARD_PAGE_SUPPORT
            pool->internal.free = block->next_on_free;
#else
            pool->internal.free = block->next;
#endif
            block->tag = tag;
            block->id = heap_block_id_used;

            if (debug) {
                memory_pool_verify_block(pool, block);
            }

            ASSERT(pool->internal.num_free_blocks > 0);
            --pool->internal.num_free_blocks;
            ++pool->internal.num_used_blocks;

            return (void *)((uintptr_t)block + ptr_ofs);
        }
    }

    return NULL;
}

void memory_pool_free(memory_pool_t * const pool, void * const ptr, const char * const tag) {
    ASSERT(pool->internal.init);
    ASSERT(tag);

    const bool debug = pool->internal.debug;

    if (debug) {
        memory_pool_verify(pool);
        memory_pool_verify_ptr(pool, ptr);
    }

    memory_pool_block_header_t * const block = memory_pool_get_block_header(pool, ptr);
    ASSERT(memory_pool_is_used_block(block));

    block->id = heap_block_id_free;
    block->tag = tag;

#ifdef GUARD_PAGE_SUPPORT
    if (pool->internal.pages.ptr) {
        block->next_on_free = pool->internal.free;
        if (pool->internal.free) {
            pool->internal.free->prev_on_free = block;
        }
        pool->internal.free = block;
    } else {
        block->next_on_free = pool->internal.free;
        pool->internal.free = block;
    }
#else
    block->next = pool->internal.free;
    pool->internal.free = block;
#endif

    ASSERT(pool->internal.num_used_blocks > 0);
    --pool->internal.num_used_blocks;
    ++pool->internal.num_free_blocks;
}

void memory_pool_walk(const memory_pool_t * const pool, void (*const callback)(const memory_pool_t * const pool, const memory_pool_block_header_t * const block, void * const user), void * const user) {
    ASSERT(pool->internal.init);

#ifdef GUARD_PAGE_SUPPORT
    if (!pool->internal.pages.ptr) {
        for (const memory_pool_block_header_t * it = pool->internal.head; it; it = it->next_on_head) {
            VERIFY(memory_pool_is_valid_block(pool, it));
            callback(pool, it, user);
        }
    } else
#endif
    {
        const size_t num_blocks = pool->internal.num_blocks;
        const size_t block_size = pool->internal.block_size;

        const uintptr_t uptr = (uintptr_t)(pool->internal.head);
        for (size_t i = 0; i < num_blocks; ++i) {
            const memory_pool_block_header_t * const block = (const memory_pool_block_header_t *)(uptr + i * block_size);
            VERIFY(memory_pool_is_valid_block(pool, block));
            callback(pool, block, user);
        }
    }
}

void memory_pool_verify(const memory_pool_t * const pool) {
    ASSERT(pool->internal.init);

    size_t num_free = 0;
    size_t num_used = 0;

#ifdef GUARD_PAGE_SUPPORT
    if (pool->internal.pages.ptr) {
        for (const memory_pool_block_header_t * block = pool->internal.head; block; block = block->next_on_head) {
            VERIFY(memory_pool_is_valid_block(pool, block));
            if (memory_pool_is_free_block(block)) {
                ++num_free;
            } else {
                VERIFY(memory_pool_is_used_block(block));
                ++num_used;
            }
        }
    } else
#endif
    {
        const size_t num_blocks = pool->internal.num_blocks;
        const size_t block_size = pool->internal.block_size;

        const uintptr_t uptr = (uintptr_t)(pool->internal.head);
        for (size_t i = 0; i < num_blocks; ++i) {
            const memory_pool_block_header_t * const block = (const memory_pool_block_header_t *)(uptr + i * block_size);

            VERIFY(memory_pool_is_valid_block(pool, block));
            if (memory_pool_is_free_block(block)) {
                ++num_free;
            } else {
                VERIFY(memory_pool_is_used_block(block));
                ++num_used;
            }
        }
    }

    VERIFY(num_free == pool->internal.num_free_blocks);
    VERIFY_MSG(num_used == pool->internal.num_used_blocks, "num_used: [%" PRIu64 "]\nnum_used_blocks: [%" PRIu64 "]", num_used, pool->internal.num_used_blocks);
    VERIFY((num_free + num_used) == pool->internal.num_blocks);

    for (const memory_pool_block_header_t * block = pool->internal.free; block;
#ifdef GUARD_PAGE_SUPPORT
         block = block->next_on_free) {
#else
         block = block->next) {
#endif
        memory_pool_verify_block(pool, block);
    }
}

void memory_pool_verify_block(const memory_pool_t * const pool, const memory_pool_block_header_t * const block) {
    ASSERT(pool->internal.init);
    VERIFY(memory_pool_is_valid_block(pool, block));

#ifdef GUARD_PAGE_SUPPORT
    if (pool->internal.pages.ptr) {
        for (const memory_pool_block_header_t * it = pool->internal.head; it; it = it->next_on_head) {
            VERIFY(memory_pool_is_valid_block(pool, it));
            if (it == block) {
                return;
            }
        }
        TRAP("Mislinked pool block");
    } else
#endif
    {
        const size_t num_blocks = pool->internal.num_blocks;
        const size_t block_size = pool->internal.block_size;
        const uintptr_t uptr = (uintptr_t)(pool->internal.head);

        for (size_t i = 0; i < num_blocks; ++i) {
            const memory_pool_block_header_t * const it = (const memory_pool_block_header_t *)(uptr + i * block_size);
            VERIFY(memory_pool_is_valid_block(pool, it));
            if (it == block) {
                return;
            }
        }
        TRAP("Mislinked pool block");
    }
}

void memory_pool_debug_print_leaks(const memory_pool_t * const pool, const char * const pool_name) {
    ASSERT(pool->internal.init);

    const size_t block_size = pool->internal.block_size;

#ifdef GUARD_PAGE_SUPPORT
    if (pool->internal.pages.ptr) {
        for (const memory_pool_block_header_t * block = pool->internal.head; block; block = block->next_on_head) {
            memory_pool_verify_block(pool, block);

            if (memory_pool_is_used_block(block)) {
                LOG_WARN(TAG_MEMORY_POOL, "%s: Memory leak in pool \"%s\": %i byte(s) @ %x", block->tag, pool_name, block_size, block);
            } else {
                VERIFY(memory_pool_is_free_block(block));
            }
        }
    } else
#endif
    {
        const size_t num_blocks = pool->internal.num_blocks;
        const uintptr_t uptr = (uintptr_t)(pool->internal.head);

        for (size_t i = 0; i < num_blocks; ++i) {
            const memory_pool_block_header_t * const block = (const memory_pool_block_header_t *)(uptr + i * block_size);
            VERIFY(memory_pool_is_valid_block(pool, block));
            if (memory_pool_is_used_block(block)) {
                LOG_WARN(TAG_MEMORY_POOL, "%s: Memory leak in pool \"%s\": %i byte(s) @ %x", block->tag, pool_name, block_size, block);
            } else {
                VERIFY(memory_pool_is_free_block(block));
            }
        }
    }
}
