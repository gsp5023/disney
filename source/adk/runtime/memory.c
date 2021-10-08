/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
memory.c

memory allocators
*/

#include _PCH
#include "memory.h"

#include "source/adk/steamboat/sb_platform.h"

int get_sys_page_size() {
    extern int sys_page_size;
    return sys_page_size;
}

#ifdef DEBUG_PAGE_MEMORY_SERVICES
/*
===============================================================================
Debug system page memory allocation
===============================================================================
*/

debug_sys_page_block_t debug_sys_map_page_block(const size_t size, const system_page_protect_e protect, const system_guard_page_mode_e guard_page_mode, const char * const tag) {
    const size_t page_size = get_sys_page_size();
    const size_t aligned_size = ALIGN_INT(size, page_size);

    if (guard_page_mode != system_guard_page_mode_disabled) {
        // bookend with no_access pages
        const size_t total_size = aligned_size + page_size * 2;
        const mem_region_t pages = debug_sys_map_pages(total_size, system_page_protect_no_access, tag);
        const uintptr_t base = (uintptr_t)pages.ptr;
        if (!base) {
            return (debug_sys_page_block_t){0};
        }
        const uintptr_t first_user_page = base + page_size;
        if (protect != system_page_protect_no_access) {
            debug_sys_protect_pages(MEM_REGION(.ptr = (void *)first_user_page, .size = aligned_size), protect);
        }

        return (debug_sys_page_block_t){
            .internal = MEM_REGION(.ptr = (void *)base, .size = total_size),
            .region = MEM_REGION(.ptr = (void *)first_user_page, .size = aligned_size)};
    }

    const mem_region_t pages = debug_sys_map_pages(aligned_size, protect, tag);
    if (!pages.ptr) {
        return (debug_sys_page_block_t){0};
    }

    return (debug_sys_page_block_t){
        .internal = pages,
        .region = pages};
}

void debug_sys_unmap_page_block(const debug_sys_page_block_t block, const char * const tag) {
    ASSERT(block.internal.ptr);
    debug_sys_unmap_pages(block.internal, tag);
}

mem_region_t debug_sys_map_pages(const size_t size, const system_page_protect_e protect, const char * const tag) {
    return sb_map_pages(size, protect);
}

void debug_sys_protect_pages(const mem_region_t pages, const system_page_protect_e protect) {
    sb_protect_pages(pages, protect);
}

void debug_sys_unmap_pages(const mem_region_t pages, const char * const tag) {
    sb_unmap_pages(pages);
}

#endif
