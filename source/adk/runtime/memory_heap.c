/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
memory_heap.c

memory heap
*/

#include _PCH

#include "memory.h"
#include "source/adk/log/log.h"
#define TAG_MEMORY_HEAP FOURCC('H', 'E', 'A', 'P')

#ifdef _MAT_ENABLED
#include "source/adk/steamboat/private/mat.h"
#endif

/*
===============================================================================
heap_t
block based memory allocator
===============================================================================
*/

static void * heap_get_block_base_page(heap_block_header_t * const block) {
    const size_t page_size = get_sys_page_size();
    return (void *)(REV_ALIGN_PTR(block, page_size) - page_size);
}

static void copy_extra_header_bytes(const size_t extra_bytes, const heap_block_header_t * const src, heap_block_header_t * const dst) {
    if (extra_bytes) {
        memcpy(
            (void *)((uintptr_t)src + sizeof(heap_block_header_t)),
            (void *)((uintptr_t)dst + sizeof(heap_block_header_t)),
            extra_bytes);
    }
}

static void zero_extra_header_bytes(const size_t extra_bytes, heap_block_header_t * const block) {
    if (extra_bytes) {
        memset((void *)((uintptr_t)block + sizeof(heap_block_header_t)), 0, extra_bytes);
    }
}

static heap_block_header_t * heap_emplace_free_block(void * const ptr, const size_t size) {
    heap_block_header_t * const block = (heap_block_header_t *)(ptr);
    ZEROMEM(block);
    block->id = heap_block_id_free;
    block->size = size;
    return block;
}

static bool heap_merge_free_blocks(heap_t * const heap, heap_block_header_t * const block) {
    bool merged = false;

    while (block->next != NULL) {
        heap_block_header_t * const next = block->next;
        ASSERT(heap_is_valid_block(next));

        if (next != block->next_on_free) {
            ASSERT(heap_is_used_block(next));
            break;
        }

        ASSERT(next->id == heap_block_id_free);
        block->size += next->size;
        block->next = next->next;
        block->next_on_free = next->next_on_free;
        merged = true;

        ASSERT(heap->internal.num_free_blocks > 0);
        --heap->internal.num_free_blocks;
        ++heap->internal.num_merged_blocks;
    }

    return merged;
}

void heap_insert_free_block(heap_t * const heap, heap_block_header_t * const free_block) {
    heap_block_header_t * block = NULL;
    heap_block_header_t * prev = NULL;

    free_block->id = heap_block_id_free;

    // walk the free block list and find the first free block _after_ "free_block"
    // in memory-address-order.

    for (heap_block_header_t * it = heap->internal.free; it; it = it->next_on_free) {
        if (free_block < it) {
            block = it;
            break;
        }
        prev = it;
    }

    // link it in.

    free_block->next_on_free = block;
    if (prev) {
        // there is a block preceding the block we are linking in.
        // link the new block in and try to merge free blocks.
        prev->next_on_free = free_block;
        if (prev->next == free_block) {
            //  next block is the free block, merge has to succeed
            VERIFY(heap_merge_free_blocks(heap, prev));
        } else {
            // try and merge free blocks forward
            heap_merge_free_blocks(heap, free_block);
        }
    } else {
        heap->internal.free = free_block;
        heap_merge_free_blocks(heap, free_block);
    }
}

#ifdef DEBUG_PAGE_MEMORY_SERVICES

heap_t * debug_heap_emplace_init(const size_t heap_size, const int alignment, const int block_header_size, const char * const name, const system_guard_page_mode_e guard_pages, const char * const tag) {
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT(tag);
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(heap_block_header_t)));

    const size_t aligned_heap_size = PAGE_ALIGN_INT(heap_size);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_pages == system_guard_page_mode_enabled) {
        const int page_size = get_sys_page_size();
        ASSERT(page_size > (int)sizeof(heap_t));
        const debug_sys_page_block_t pages = debug_sys_map_page_block(page_size, system_page_protect_read_write, system_guard_page_mode_minimal, tag);
        TRAP_OUT_OF_MEMORY(pages.region.ptr);

        heap_t * const heap = (heap_t *)((uintptr_t)pages.region.ptr + page_size - sizeof(heap_t));

        ASSERT(aligned_heap_size > sizeof(heap_t));
        const size_t new_heap_size = aligned_heap_size - sizeof(heap_t);

        ZEROMEM(heap);
        ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(heap_block_header_t)));

        const int clamped_alignment = max_int(alignment, sizeof(void *));
        const int clamped_block_header_size = max_int(block_header_size, sizeof(heap_block_header_t));

        strcpy_s(heap->name, ARRAY_SIZE(heap->name), name);

        heap->internal.init = true;
        heap->internal.guard_pages = true;
        heap->internal.guard_pages_max_heap_size = new_heap_size;
        heap->internal.alignment = clamped_alignment;
        heap->internal.free_block_size = new_heap_size;
        heap->internal.header_size = clamped_block_header_size;
        heap->internal.extra_header_bytes = clamped_block_header_size - sizeof(heap_block_header_t);
        heap->internal.ptr_ofs = ALIGN_INT(clamped_block_header_size, clamped_alignment);

        ASSERT(!heap->internal.pages_to_free_on_destroy.region.ptr);
        heap->internal.pages_to_free_on_destroy = pages;

        return heap;
    } else
#endif
    {
        const debug_sys_page_block_t pages = debug_sys_map_page_block(aligned_heap_size, system_page_protect_read_write, guard_pages, tag);
        TRAP_OUT_OF_MEMORY(pages.region.ptr);
        return debug_heap_emplace_init_with_pages_to_free_on_destroy(pages, alignment, block_header_size, name);
    }
}

void debug_heap_init(heap_t * const heap, const size_t heap_size, const int alignment, const int block_header_size, const char * const name, const system_guard_page_mode_e guard_pages, const char * const tag) {
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT(tag);
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(heap_block_header_t)));

    const size_t aligned_heap_size = PAGE_ALIGN_INT(heap_size);

#ifdef GUARD_PAGE_SUPPORT
    if (guard_pages == system_guard_page_mode_enabled) {
        ZEROMEM(heap);
        ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(heap_block_header_t)));

        const int clamped_alignment = max_int(alignment, sizeof(void *));
        const int clamped_block_header_size = max_int(block_header_size, sizeof(heap_block_header_t));

        strcpy_s(heap->name, ARRAY_SIZE(heap->name), name);

        heap->internal.init = true;
        heap->internal.guard_pages = true;
        heap->internal.guard_pages_max_heap_size = aligned_heap_size;
        heap->internal.alignment = clamped_alignment;
        heap->internal.free_block_size = aligned_heap_size;
        heap->internal.header_size = clamped_block_header_size;
        heap->internal.extra_header_bytes = clamped_block_header_size - sizeof(heap_block_header_t);
        heap->internal.ptr_ofs = ALIGN_INT(clamped_block_header_size, clamped_alignment);
    } else
#endif
    {
        const debug_sys_page_block_t pages = debug_sys_map_page_block(aligned_heap_size, system_page_protect_read_write, guard_pages, tag);
        TRAP_OUT_OF_MEMORY(pages.region.ptr);
        debug_heap_init_with_pages_to_free_on_destroy(heap, pages, alignment, block_header_size, name);
    }
}

void debug_heap_init_with_pages_to_free_on_destroy(heap_t * const heap, const debug_sys_page_block_t pages, const int alignment, const int block_header_size, const char * const name) {
    heap_init_with_region(heap, pages.region, alignment, block_header_size, name);
    heap->internal.pages_to_free_on_destroy = pages;
}

heap_t * debug_heap_emplace_init_with_pages_to_free_on_destroy(const debug_sys_page_block_t pages, const int alignment, const int block_header_size, const char * const name) {
    heap_t * const heap = heap_emplace_init_with_region(pages.region, alignment, block_header_size, name);
    if (heap) {
        heap->internal.pages_to_free_on_destroy = pages;
    }
    return heap;
}

#endif

void heap_init_with_region(heap_t * const heap, const mem_region_t region, const int alignment, const int block_header_size, const char * const name) {
    ASSERT(region.size > 0);
    ZEROMEM(heap);
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(heap_block_header_t)));

    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const int clamped_block_header_size = max_int(block_header_size, sizeof(heap_block_header_t));

    ASSERT_ALIGNED(region.ptr, clamped_alignment);

    strcpy_s(heap->name, ARRAY_SIZE(heap->name), name);

    heap->internal.init = true;
    heap->internal.alignment = clamped_alignment;
    heap->internal.header_size = clamped_block_header_size;
    heap->internal.extra_header_bytes = clamped_block_header_size - sizeof(heap_block_header_t);
    heap->internal.ptr_ofs = ALIGN_INT(clamped_block_header_size, clamped_alignment);
    heap->internal.min_block_size = heap->internal.ptr_ofs + clamped_alignment;

    ASSERT(heap->internal.min_block_size <= region.size);

    heap->internal.head = heap->internal.free = heap_emplace_free_block(region.ptr, region.size);
    zero_extra_header_bytes(heap->internal.extra_header_bytes, heap->internal.free);
    heap->internal.num_free_blocks = 1;
    heap->internal.free_block_size = region.size;
    heap->internal.heap_size = region.size;

#ifdef _MAT_ENABLED
    mat_group_t group = 0;
    MAT_REGISTER_MAT_GROUP(group, heap->name, mat_get_default_group());
    MAT_ALLOC_POOL(heap->name, region.ptr, heap->internal.heap_size, group, &heap->internal.mat_pool_id);
#endif
}

heap_t * heap_emplace_init_with_region(const mem_region_t region, const int alignment, const int block_header_size, const char * const name) {
    ASSERT((alignment == 0) || IS_POW2(alignment));
    ASSERT((block_header_size == 0) || (block_header_size >= (int)sizeof(heap_block_header_t)));

    const int clamped_alignment = max_int(alignment, sizeof(void *));
    const size_t aligned_heap_struct_size = ALIGN_INT(sizeof(heap_t), clamped_alignment);

    ASSERT_ALIGNED(region.ptr, clamped_alignment);
    ASSERT(region.size > aligned_heap_struct_size);

    const mem_region_t subregion = (mem_region_t){
        {{{.adr = region.adr + aligned_heap_struct_size},
          .size = region.size - aligned_heap_struct_size}}};

    heap_t * const heap = (heap_t *)region.ptr;

    heap_init_with_region(heap, subregion, alignment, block_header_size, name);

    return heap;
}

void heap_destroy(heap_t * const heap, const char * const tag) {
    ASSERT(heap->internal.init);
    heap->internal.init = false;

#ifdef _MAT_ENABLED
    MAT_FREE_POOL(heap->internal.mat_pool_id);
#endif
#ifdef GUARD_PAGE_SUPPORT
    if (heap->internal.guard_pages) {
        heap_block_header_t * next;
        for (heap_block_header_t * block = heap->internal.head; block; block = next) {
            ASSERT(heap_is_valid_block(block));
            next = block->next;
            void * const base_page = heap_get_block_base_page(block);
            debug_sys_unmap_pages(MEM_REGION(.ptr = base_page, .size = block->guard_page_total_size), tag);
        }
    }
#endif
#ifdef DEBUG_PAGE_MEMORY_SERVICES
    if (heap->internal.pages_to_free_on_destroy.region.ptr) {
        debug_sys_unmap_page_block(heap->internal.pages_to_free_on_destroy, tag);
    }
#endif
}

void * heap_unchecked_alloc(heap_t * const heap, const size_t size, const char * const tag) {
    ASSERT(heap->internal.init);
    ASSERT(tag);

    const bool debug = heap->internal.debug;

    if (debug) {
        heap_verify(heap);
    }

    const size_t aligned_size = size ? ALIGN_INT(size, heap->internal.alignment) : heap->internal.alignment;
    // catch overflow
    if (aligned_size < size) {
        return NULL;
    }
    const size_t ptr_ofs = heap->internal.ptr_ofs;
    const size_t needed = aligned_size + ptr_ofs;

#ifdef GUARD_PAGE_SUPPORT
    if (heap->internal.guard_pages) {
        // guard pages requested for this heap.
        // that means the user-space part of the allocation
        // needs to cross a no-access page after the last byte

        // NOTE: that this, unfortunately won't catch overruns of non-aligned size requests
        // for example a 3 byte allocation still has to be aligned to 4 bytes, meaning an
        // overwrite of 1 byte won't get caught

        const size_t page_size = get_sys_page_size();

        if ((heap->internal.used_block_size + needed) > heap->internal.guard_pages_max_heap_size) {
            return NULL;
        }

        const size_t block_size = ALIGN_INT(needed, page_size);
        const size_t total_size = block_size + page_size * 2;

        const mem_region_t pages = debug_sys_map_pages(total_size, system_page_protect_no_access, tag);
        if (pages.ptr) {
            const uintptr_t uptr = pages.adr;
            debug_sys_protect_pages(MEM_REGION(.adr = uptr + page_size, .size = block_size), system_page_protect_read_write);

            const uintptr_t ublock = uptr + page_size + block_size - needed;
            heap_block_header_t * const block = heap_emplace_free_block((void *)ublock, needed);
            block->tag = tag;
            block->id = heap_block_id_used;

            if (heap->internal.head) {
                heap->internal.head->prev = block;
            }

            block->prev = NULL;
            block->next = heap->internal.head;
            block->guard_page_total_size = total_size;
            block->original_total_allocated_page_size = total_size;
            heap->internal.head = block;
            heap->internal.used_block_size += needed;
            ++heap->internal.num_used_blocks;
            void * const ptr = (void *)(ublock + ptr_ofs);
            if (debug) {
                heap_verify_ptr(heap, ptr);
            }
#ifdef _MAT_ENABLED
            MAT_ALLOC(ptr, size, 0, 0);
#endif
            return ptr;
        }
        return NULL;
    } else
#endif
    {
        heap_block_header_t * block = NULL;
        heap_block_header_t * block_prev = NULL;

        {
            heap_block_header_t * prev = NULL;
            for (heap_block_header_t * it = heap->internal.free; it; it = it->next_on_free) {
                ASSERT(it->id == heap_block_id_free);

                if (it->size >= needed) {
                    block = it;
                    block_prev = prev;
                    break;
                }

                prev = it;
            }
        }

        if (block) {
            const uintptr_t ptr = (uintptr_t)block;
            const uintptr_t end = ptr + needed;
            const size_t free_space = block->size - needed;
            const size_t min_block_size = heap->internal.min_block_size;
            const size_t extra_header_bytes = heap->internal.extra_header_bytes;

            // if the remaining space in this free block after allocation
            // is at least as big as our minimum block size then convert
            // the remaining free space into another free block.

            if (free_space >= min_block_size) {
                block->size = needed;
                heap_block_header_t * const free_block = heap_emplace_free_block((void *)end, free_space);
                free_block->next = block->next;
                free_block->next_on_free = block->next_on_free;
                block->next = free_block;

                if (block_prev) {
                    block_prev->next_on_free = free_block;
                } else {
                    heap->internal.free = free_block;
                }

                copy_extra_header_bytes(extra_header_bytes, block, free_block);
                ASSERT(heap->internal.free_block_size >= needed);
                heap->internal.free_block_size -= needed;
            } else {
                // free block is totally consumed by this allocation
                // remove it.

                ASSERT(heap->internal.num_free_blocks > 0);
                --heap->internal.num_free_blocks;
                ASSERT(heap->internal.free_block_size >= block->size);
                heap->internal.free_block_size -= block->size;

                if (block_prev) {
                    block_prev->next_on_free = block->next_on_free;
                } else {
                    heap->internal.free = block->next_on_free;
                }
            }

            block->id = heap_block_id_used;
            block->next_on_free = NULL;
            block->tag = tag;
#ifdef GUARD_PAGE_SUPPORT
            block->guard_page_total_size = 0;
#endif
            ++heap->internal.num_used_blocks;
            heap->internal.used_block_size += block->size;
            zero_extra_header_bytes(extra_header_bytes, block);

            void * const uptr = (void *)(ptr + ptr_ofs);
            if (debug) {
                heap_verify_ptr(heap, uptr);
                heap_verify(heap);
            }
#ifdef _MAT_ENABLED
            MAT_ALLOC(uptr, size, 0, 0);
#endif
            return uptr;
        }
    }

    return NULL;
}

void * heap_unchecked_calloc(heap_t * const heap, size_t size, const char * const tag) {
    void * const p = heap_unchecked_alloc(heap, size, tag);
    if (p) {
        memset(p, 0, size);
    }
    return p;
}

void * heap_unchecked_realloc(heap_t * const heap, void * const ptr, const size_t size, const char * const tag) {
    if (!ptr) {
        return heap_unchecked_alloc(heap, size, tag);
    }

    ASSERT(heap->internal.init);
    ASSERT(tag);

    const bool debug = heap->internal.debug;

    if (debug) {
        heap_verify_ptr(heap, ptr);
        heap_verify(heap);
    }

    const size_t aligned_size = size ? ALIGN_INT(size, heap->internal.alignment) : heap->internal.alignment;
    // catch overflow
    if (aligned_size < size) {
        return NULL;
    }
    const size_t ptr_ofs = heap->internal.ptr_ofs;
    const size_t needed = aligned_size + ptr_ofs;
    const size_t extra_header_bytes = heap->internal.extra_header_bytes;

    heap_block_header_t * const block = heap_get_block_header(heap, ptr);
    ASSERT(heap_is_used_block(block));

    // nothing to do?
    if (block->size == needed) {
        return ptr;
    }

#ifdef GUARD_PAGE_SUPPORT
    if (heap->internal.guard_pages) {
        // this heap is requesting guard pages,
        // which means we can resize in place as
        // long as we don't overflow our current block size.

        const size_t page_size = get_sys_page_size();
        const size_t cur_block_size = block->guard_page_total_size - page_size * 2;
        const size_t new_block_size = ALIGN_INT(needed, page_size);

        if (new_block_size <= cur_block_size) {
            // if block is growing check free space
            if (needed > block->size) {
                // exceeds free space
                if ((heap->internal.used_block_size - block->size + needed) > heap->internal.guard_pages_max_heap_size) {
                    return NULL;
                }
            }
            const size_t move_size = min_size_t(needed, block->size);

            ASSERT(heap->internal.used_block_size >= block->size);
            heap->internal.used_block_size -= block->size;
            heap->internal.used_block_size += needed;

            const uintptr_t new_end_page = (uintptr_t)heap_get_block_base_page(block) + page_size + new_block_size;
            heap_block_header_t * const new_block = (heap_block_header_t *)(new_end_page - needed);
            if (new_block != block) {
                memmove(new_block, block, move_size);

                // relink since our block moved

                if (new_block->prev) {
                    ASSERT(new_block->prev->next == block);
                    new_block->prev->next = new_block;
                } else {
                    ASSERT(heap->internal.head == block);
                    heap->internal.head = new_block;
                }

                if (new_block->next) {
                    ASSERT(new_block->next->prev == block);
                    new_block->next->prev = new_block;
                }
            }

            new_block->size = needed;
            const size_t new_guard_page_total_size = new_block_size + page_size * 2;
            ASSERT(new_guard_page_total_size <= new_block->guard_page_total_size);

            if (new_guard_page_total_size < new_block->guard_page_total_size) {
                // windows cannot unmap pages in the middle of a reservation area
                // so settle for just protecting.
                debug_sys_protect_pages(MEM_REGION(.adr = new_end_page, .size = page_size + new_block->guard_page_total_size - new_guard_page_total_size), system_page_protect_no_access);

                new_block->guard_page_total_size = new_guard_page_total_size;
            }

            return (void *)((uintptr_t)new_block + ptr_ofs);
        }

        // size is too large to satifsy in-place
        void * new_ptr = heap_unchecked_alloc(heap, size, tag);
        if (!new_ptr) {
            return NULL;
        }

        memcpy(new_ptr, ptr, block->size - ptr_ofs);
        heap_block_header_t * new_block = heap_get_block_header(heap, new_ptr);
        copy_extra_header_bytes(extra_header_bytes, block, new_block);

        heap_free(heap, ptr, tag);
        return new_ptr;
    } else
#endif
    {
        heap_block_header_t * next_free = (block->next && heap_is_free_block(block->next)) ? block->next : NULL;

        if (needed < block->size) {
            // shrink block, in this path the original pointer is returned back
            // to the user, but it may grow surrounding free blocks or create
            // new free blocks.

            block->tag = tag;

            if (next_free) {
                // this block is shrinking and the next
                // block is free, shift free block backwards
                // and grow it to consume the newly unused space

                block->size = needed;

                heap_block_header_t * prev_free = NULL;
                for (heap_block_header_t * it = heap->internal.free; it != next_free; it = it->next_on_free) {
                    prev_free = it;
                }

                const uintptr_t start = (uintptr_t)block;
                const uintptr_t end = start + needed;
                const uintptr_t delta = (uintptr_t)next_free - end;

                memmove((void *)end, next_free, heap->internal.header_size);

                next_free = (heap_block_header_t *)end;
                next_free->size += delta;

                block->next = next_free;

                if (prev_free) {
                    prev_free->next_on_free = next_free;
                } else {
                    heap->internal.free = next_free;
                }

                ASSERT(heap->internal.used_block_size >= delta);
                heap->internal.used_block_size -= delta;
                heap->internal.free_block_size += delta;

                if (debug) {
                    heap_verify_ptr(heap, ptr);
                    heap_verify(heap);
                }
            } else {
                // next block is used or end of heap address space
                // shrink in-place, and if enough space is available
                // emplace a new free block.

                const uintptr_t start = (uintptr_t)block;
                const uintptr_t end = start + needed;
                const uintptr_t end_of_block = start + block->size;

                const size_t free_space = end_of_block - end;

                if (free_space >= heap->internal.min_block_size) {
                    // enough space is free to create a new free block here.

                    block->size = needed;
                    heap_block_header_t * const free_block = heap_emplace_free_block((void *)end, free_space);
                    free_block->next = block->next;
                    free_block->next_on_free = NULL;
                    block->next = free_block;
                    heap_insert_free_block(heap, free_block);
                    zero_extra_header_bytes(extra_header_bytes, free_block);

                    ++heap->internal.num_free_blocks;
                    ASSERT(heap->internal.used_block_size >= free_space);
                    heap->internal.used_block_size -= free_space;
                    heap->internal.free_block_size += free_space;

                    if (debug) {
                        heap_verify_ptr(heap, ptr);
                        heap_verify(heap);
                    }
                }
            }

            return ptr;
        }

        // grow allocation

        const size_t min_block_size = heap->internal.min_block_size;
        const size_t delta = needed - block->size;

        if (next_free && (next_free->size >= delta)) {
            // the block after the current allocation is free
            // and it is large enough to consume the necessary
            // size delta

            heap_block_header_t * prev_free = NULL;
            for (heap_block_header_t * it = heap->internal.free; it != next_free; it = it->next_on_free) {
                prev_free = it;
            }

            const size_t free_space = next_free->size - delta;

            if (free_space >= min_block_size) {
                // the free block is large enough to split into another block
                // so move the free block header forward and relink it

                const uintptr_t src = (uintptr_t)next_free;
                next_free = (heap_block_header_t *)(src + delta);
                memmove(next_free, (void *)src, heap->internal.header_size);

                block->next = next_free;
                block->size += delta;
                next_free->size = free_space;

                if (prev_free) {
                    prev_free->next_on_free = next_free;
                } else {
                    heap->internal.free = next_free;
                }

                ASSERT(heap->internal.free_block_size >= delta);
                heap->internal.free_block_size -= delta;
                heap->internal.used_block_size += delta;

                if (debug) {
                    heap_verify_ptr(heap, ptr);
                    heap_verify(heap);
                }

            } else {
                // this free block is totally consumed by the resize operation
                // unlink it

                if (prev_free) {
                    prev_free->next_on_free = next_free->next_on_free;
                } else {
                    heap->internal.free = next_free->next_on_free;
                }

                block->next = next_free->next;
                block->size += next_free->size;

                ASSERT(heap->internal.num_free_blocks > 0);
                --heap->internal.num_free_blocks;
                ASSERT(heap->internal.free_block_size >= next_free->size);
                heap->internal.free_block_size -= next_free->size;
                heap->internal.used_block_size += next_free->size;

                if (debug) {
                    heap_verify_ptr(heap, ptr);
                    heap_verify(heap);
                }
            }

            return ptr;
        }

        // allocation cannot be resized in place
        // allocate a new block, copy, and free

        void * const new_ptr = heap_unchecked_alloc(heap, size, tag);
        if (new_ptr) {
            copy_extra_header_bytes(extra_header_bytes, block, heap_get_block_header(heap, new_ptr));
            memcpy(new_ptr, ptr, (block->size - ptr_ofs));
            heap_free(heap, ptr, tag);
        }

        return new_ptr;
    }
}

void heap_free(heap_t * const heap, void * const ptr, const char * const tag) {
    ASSERT(heap->internal.init);
    ASSERT_MSG(ptr, "tried to free NULL ptr (%s)", tag);
    ASSERT(tag);

    const bool debug = heap->internal.debug;

    if (debug) {
        heap_verify_ptr(heap, ptr);
        heap_verify(heap);
    }
#ifdef _MAT_ENABLED
    MAT_FREE(ptr);
#endif
#ifdef GUARD_PAGE_SUPPORT
    if (heap->internal.guard_pages) {
        heap_block_header_t * const block = heap_get_block_header(heap, ptr);
        ASSERT(heap_is_used_block(block));

        if (block->prev) {
            block->prev->next = block->next;
        } else {
            heap->internal.head = block->next;
        }
        if (block->next) {
            block->next->prev = block->prev;
        }

        ASSERT(heap->internal.num_used_blocks > 0);
        ASSERT(heap->internal.used_block_size >= block->size);
        --heap->internal.num_used_blocks;
        heap->internal.used_block_size -= block->size;

        debug_sys_unmap_pages(MEM_REGION(.ptr = heap_get_block_base_page(block), .size = block->original_total_allocated_page_size), tag);

        if (debug) {
            heap_verify(heap);
        }
    } else
#endif
    {
        heap_block_header_t * const block = heap_get_block_header(heap, ptr);
        ASSERT(heap_is_used_block(block));
        block->tag = tag;
        ASSERT(heap->internal.num_used_blocks > 0);
        --heap->internal.num_used_blocks;
        ASSERT(heap->internal.used_block_size >= block->size);
        heap->internal.used_block_size -= block->size;
        heap->internal.free_block_size += block->size;
        ++heap->internal.num_free_blocks;
        heap_insert_free_block(heap, block);
    }
}

void heap_walk(const heap_t * const heap, void (*const callback)(const heap_t * const heap, const heap_block_header_t * const block, void * const user), void * const user) {
    for (const heap_block_header_t * block = heap->internal.head; block; block = block->next) {
        callback(heap, block, user);
    }
}

void heap_verify(const heap_t * const heap) {
    VERIFY(heap->internal.init);

    size_t num_used = 0;
    size_t num_free = 0;
    size_t used_size = 0;
    size_t free_size = 0;

#ifdef GUARD_PAGE_SUPPORT
    const bool guard_pages = heap->internal.guard_pages;
    if (guard_pages) {
        free_size = heap->internal.free_block_size;
    }
#endif

    for (heap_block_header_t * block = heap->internal.head; block; block = block->next) {
        VERIFY(heap_is_valid_block(block));
        if (heap_is_free_block(block)) {
#ifdef GUARD_PAGE_SUPPORT
            VERIFY(!guard_pages);
#endif
            ++num_free;
            free_size += block->size;
            heap_block_header_t * search;
            for (search = heap->internal.free; search && (search != block); search = search->next_on_free) {
                VERIFY((search->next_on_free == NULL) || (search < search->next_on_free));
            }
            VERIFY_MSG(search, "Mislinked free block");
        } else {
            ++num_used;
            used_size += block->size;
        }
#ifdef GUARD_PAGE_SUPPORT
        if (!guard_pages) {
            VERIFY((block->next == NULL) || (block < block->next));
            VERIFY((block->next_on_free == NULL) || (block < block->next_on_free));
        }
#endif
    }

    VERIFY(num_used == heap->internal.num_used_blocks);
    VERIFY(num_free == heap->internal.num_free_blocks);
    VERIFY(used_size == heap->internal.used_block_size);
    VERIFY(free_size == heap->internal.free_block_size);
}

void heap_verify_ptr(const heap_t * const heap, const void * const ptr) {
    heap_block_header_t * const block = heap_get_block_header(heap, ptr);
    heap_verify_block(heap, block);
}

void heap_verify_block(const heap_t * const heap, const heap_block_header_t * const block) {
    VERIFY(heap_is_used_block(block));

    heap_block_header_t * search;
    for (search = heap->internal.head; search && (search != block); search = search->next) {
        VERIFY(heap_is_valid_block(search));
    }

    VERIFY_MSG(search, "Block not found in heap");
}

void heap_debug_print_leaks(const heap_t * const heap) {
    ASSERT(heap->internal.init);
    for (heap_block_header_t * it = heap->internal.head; it; it = it->next) {
        if (heap_is_used_block(it)) {
            LOG_WARN(TAG_MEMORY_HEAP, "\n%s: Memory leak in heap \"%s\": %zu byte(s) @ %zx", it->tag, heap->name, it->size, (size_t)it);
        } else {
            VERIFY(heap_is_free_block(it));
        }
    }
}

void heap_dump_usage(const heap_t * const heap) {
    if (heap->internal.init) { // temporary fix: do not crash under wasm3
        ASSERT(heap->internal.init);
        LOG_ALWAYS(TAG_MEMORY_HEAP, "[%s]: Heap statistics:\nheap_size: %zu\nheap_used : %zu\nheap_free: %zu\nmax_used_size: %zu\n", heap->name, heap->internal.heap_size, heap->internal.used_block_size, heap->internal.free_block_size, heap->internal.max_used_size);
    }
}

heap_metrics_t heap_get_metrics(const heap_t * const heap) {
    return (heap_metrics_t){
        .alignment = heap->internal.alignment,
        .ptr_ofs = heap->internal.ptr_ofs,
        .header_size = heap->internal.header_size,
        .extra_header_bytes = heap->internal.extra_header_bytes,
        .min_block_size = heap->internal.min_block_size,
        .heap_size = heap->internal.heap_size,
        .num_used_blocks = heap->internal.num_used_blocks,
        .num_free_blocks = heap->internal.num_free_blocks,
        .used_block_size = heap->internal.used_block_size,
        .free_block_size = heap->internal.free_block_size,
        .num_merged_blocks = heap->internal.num_merged_blocks,
        .max_used_size = heap->internal.max_used_size,
    };
}
