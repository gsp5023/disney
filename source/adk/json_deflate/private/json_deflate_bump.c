/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
json_deflate_bump.c

Bump allocator. Does not provide individual object deallocation granularity.
*/

#include "source/adk/json_deflate/json_deflate.h"
#include "source/adk/json_deflate/private/json_deflate.h"

static void * ensure_new(void * const ptr) {
    ASSERT_MSG(!*((unsigned char *)ptr), "Allocator did not return untouched memory");
    return ptr;
}

static uint8_t * get_single_buffer_borrowed(const json_deflate_bump_area_borrowed_t * const area) {
    ASSERT(area);
    ASSERT(area->borrowed_buffer);

    return area->borrowed_buffer;
}

static uint8_t * get_single_buffer_owned(const json_deflate_bump_area_owned_t * const area) {
    ASSERT(area);
    ASSERT(area->number_of_owned_buffers);
    ASSERT(area->owned_buffers);
    ASSERT(!area->growable);

    const size_t last_buffer_index = area->number_of_owned_buffers - 1;
    uint8_t * const last_buffer = area->owned_buffers[last_buffer_index];

    return last_buffer;
}

static uint8_t * get_single_buffer(const json_deflate_bump_area_t * const area) {
    ASSERT(area);

    if (area->owned) {
        return get_single_buffer_owned(&area->payload.owned);
    } else {
        return get_single_buffer_borrowed(&area->payload.borrowed);
    }
}

static uint8_t * offset_to_ptr_borrowed(const json_deflate_bump_area_borrowed_t * const area, const size_t offset) {
    ASSERT(area);
    ASSERT(area->borrowed_buffer);
    ASSERT(area->capacity);
    ASSERT(offset < area->capacity);

    return area->borrowed_buffer + offset;
}

static uint8_t * offset_to_ptr_owned(const json_deflate_bump_area_owned_t * const area, const size_t offset) {
    ASSERT(area);
    ASSERT(area->owned_buffers);
    ASSERT(area->number_of_owned_buffers);
    ASSERT(area->capacity_per_buffer);

    const size_t buffer_index = offset / area->capacity_per_buffer;
    const size_t offset_in_buffer = offset % area->capacity_per_buffer;

    uint8_t * const buffer = area->owned_buffers[buffer_index];
    uint8_t * const address = buffer + offset_in_buffer;

    return address;
}

static uint8_t * offset_to_ptr(const json_deflate_bump_area_t * const area, const size_t offset) {
    ASSERT(area);

    if (area->owned) {
        return offset_to_ptr_owned(&area->payload.owned, offset);
    } else {
        return offset_to_ptr_borrowed(&area->payload.borrowed, offset);
    }
}

static size_t ptr_to_offset_borrowed(const json_deflate_bump_area_borrowed_t * const area, const uint8_t * const ptr) {
    ASSERT(area);
    ASSERT(area->borrowed_buffer);
    ASSERT(area->capacity);
    ASSERT(ptr);
    ASSERT(area->borrowed_buffer <= ptr);
    ASSERT(ptr < area->borrowed_buffer + area->capacity);

    return (size_t)(((uintptr_t)ptr) - ((uintptr_t)area->borrowed_buffer));
}

static size_t ptr_to_offset_owned(const json_deflate_bump_area_owned_t * const area, const uint8_t * const ptr) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(area->owned_buffers);
    ASSERT(area->number_of_owned_buffers);
    ASSERT(area->capacity_per_buffer);

    for (size_t i = 0; i < area->number_of_owned_buffers; i++) {
        uint8_t * const buffer = area->owned_buffers[i];
        if (buffer <= ptr || ptr < buffer + area->capacity_per_buffer) {
            JSON_DEFLATE_TRACE_POP();
            return (size_t)(((uintptr_t)ptr) - ((uintptr_t)buffer));
        }
    }

    TRAP("Foreign pointer %p", ptr);
    return 0;
}

static size_t ptr_to_offset(const json_deflate_bump_area_t * const area, const uint8_t * const ptr) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    const size_t offset = area->owned ? ptr_to_offset_owned(&area->payload.owned, ptr) : ptr_to_offset_borrowed(&area->payload.borrowed, ptr);
    JSON_DEFLATE_TRACE_POP();
    return offset;
}

static size_t get_total_capacity_borrowed(json_deflate_bump_area_borrowed_t * const area) {
    ASSERT(area);
    return area->capacity;
}

static size_t get_total_capacity_owned(json_deflate_bump_area_owned_t * const area) {
    ASSERT(area);
    return area->number_of_owned_buffers * area->capacity_per_buffer;
}

static size_t get_total_capacity(json_deflate_bump_area_t * const area) {
    ASSERT(area);

    if (area->owned) {
        return get_total_capacity_owned(&area->payload.owned);
    } else {
        return get_total_capacity_borrowed(&area->payload.borrowed);
    }
}

static uint8_t * get_end_of_free_space_borrowed(json_deflate_bump_area_borrowed_t * const area) {
    ASSERT(area);
    ASSERT(area->borrowed_buffer);

    return area->borrowed_buffer + area->capacity - 1;
}

static uint8_t * get_end_of_free_space_owned(json_deflate_bump_area_owned_t * const area) {
    ASSERT(area);
    ASSERT(area->owned_buffers);

    uint8_t * const * const last_buffer = area->owned_buffers + area->number_of_owned_buffers - 1;

    return *last_buffer + area->capacity_per_buffer - 1;
}

static uint8_t * get_end_of_free_space(json_deflate_bump_area_t * const area) {
    ASSERT(area);

    if (area->owned) {
        return get_end_of_free_space_owned(&area->payload.owned);
    } else {
        return get_end_of_free_space_borrowed(&area->payload.borrowed);
    }
}

static uint8_t * try_grow_owned(json_deflate_bump_area_owned_t * const area) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);

    if (area->number_of_owned_buffers && !area->growable) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    area->number_of_owned_buffers++;

    if (!area->owned_buffers) {
        area->owned_buffers = json_deflate_unchecked_calloc(area->number_of_owned_buffers, sizeof(*area->owned_buffers));
    } else {
        uint8_t ** const new_owned_buffers_list = json_deflate_realloc(area->owned_buffers, area->number_of_owned_buffers * sizeof(*area->owned_buffers));
        if (!new_owned_buffers_list) {
            area->number_of_owned_buffers--;
            JSON_DEFLATE_TRACE_POP();
            return NULL;
        } else {
            area->owned_buffers = new_owned_buffers_list;
        }
    }

    uint8_t ** const last_buffer = area->owned_buffers + area->number_of_owned_buffers - 1;

    *last_buffer = json_deflate_unchecked_calloc(area->capacity_per_buffer, 1);

    if (*last_buffer) {
        JSON_DEFLATE_TRACE_POP();
        return *last_buffer;
    } else {
        area->number_of_owned_buffers--;
        area->owned_buffers = json_deflate_realloc(area->owned_buffers, area->number_of_owned_buffers * sizeof(*area->owned_buffers));

        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }
}

static uint8_t * try_grow(json_deflate_bump_area_t * const area) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(area->owned);

    uint8_t * const payload = try_grow_owned(&area->payload.owned);
    JSON_DEFLATE_TRACE_POP();
    return payload;
}

static size_t * get_next_borrowed(json_deflate_bump_area_borrowed_t * const area) {
    ASSERT(area);

    return &area->next;
}

static size_t * get_next_owned(json_deflate_bump_area_owned_t * const area) {
    ASSERT(area);

    return &area->next;
}

static size_t * get_next(json_deflate_bump_area_t * const area) {
    ASSERT(area);

    if (area->owned) {
        return get_next_owned(&area->payload.owned);
    } else {
        return get_next_borrowed(&area->payload.borrowed);
    }
}

static uint8_t * get_next_free_space_borrowed(json_deflate_bump_area_borrowed_t * const area, const size_t size, const size_t alignment) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(size);
    ASSERT(alignment);

    uint8_t * const start_of_empty_space = offset_to_ptr_borrowed(area, area->next);
    uint8_t * const start_of_empty_space_aligned = (void *)FWD_ALIGN_PTR(start_of_empty_space, alignment);
    uint8_t * const end_of_required_space = start_of_empty_space_aligned + size;
    uint8_t * const end_of_empty_space = &area->borrowed_buffer[area->capacity - 1];

    if (end_of_required_space >= end_of_empty_space) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    } else {
        const size_t used_space = (size_t)((uintptr_t)end_of_required_space - (uintptr_t)start_of_empty_space);
        area->next += used_space;
        JSON_DEFLATE_TRACE_POP();
        return ensure_new(start_of_empty_space_aligned);
    }
}

static uint8_t * get_next_free_space_owned(json_deflate_bump_area_owned_t * const area, const size_t size, const size_t alignment) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(size);
    ASSERT(alignment);

    uint8_t * const start_of_empty_space = offset_to_ptr_owned(area, area->next);
    uint8_t * const start_of_empty_space_aligned = (void *)FWD_ALIGN_PTR(start_of_empty_space, alignment);
    uint8_t * const end_of_required_space = start_of_empty_space_aligned + size;
    uint8_t * const last_buffer = area->owned_buffers[area->number_of_owned_buffers - 1];
    uint8_t * const end_of_empty_space = &last_buffer[area->capacity_per_buffer - 1];

    if (end_of_required_space >= end_of_empty_space) {
        if (area->growable) {
            uint8_t * const new_last_buffer = try_grow_owned(area);
            if (!new_last_buffer) {
                JSON_DEFLATE_TRACE_POP();
                return NULL;
            }

            area->next = (area->number_of_owned_buffers - 1) * area->capacity_per_buffer;

            JSON_DEFLATE_TRACE_POP();
            return get_next_free_space_owned(area, size, alignment);
        } else {
            JSON_DEFLATE_TRACE_POP();
            return NULL;
        }
    } else {
        const size_t used_space = (size_t)((uintptr_t)end_of_required_space - (uintptr_t)start_of_empty_space);
        area->next += used_space;
        JSON_DEFLATE_TRACE_POP();
        return ensure_new(start_of_empty_space_aligned);
    }
}

static uint8_t * get_next_free_space(json_deflate_bump_area_t * const area, const size_t size, const size_t alignment) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    uint8_t * const owned = area->owned ? get_next_free_space_owned(&area->payload.owned, size, alignment) : get_next_free_space_borrowed(&area->payload.borrowed, size, alignment);
    JSON_DEFLATE_TRACE_POP();
    return owned;
}

static void destroy_owned(json_deflate_bump_area_owned_t * const area) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    if (area) {
        if (area->owned_buffers) {
            for (size_t i = 0; i < area->number_of_owned_buffers; i++) {
                uint8_t * const buffer = area->owned_buffers[i];
                if (buffer) {
                    json_deflate_free(buffer);
                }
            }

            json_deflate_free(area->owned_buffers);
        }
    }
    JSON_DEFLATE_TRACE_POP();
}

void json_deflate_bump_owned_init(json_deflate_bump_area_t * const area, const size_t capacity, const bool growable) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(capacity);
    ASSERT(!*get_next(area));

    area->owned = true;

    json_deflate_bump_area_owned_t * const owned = &area->payload.owned;

    owned->capacity_per_buffer = capacity;
    owned->growable = growable;
    owned->number_of_owned_buffers = 0;
    owned->owned_buffers = NULL;

    try_grow_owned(owned);

    if (owned->number_of_owned_buffers) {
        *get_next(area) = 1;
    }
    JSON_DEFLATE_TRACE_POP();
}

void json_deflate_bump_owned_destroy(json_deflate_bump_area_t * const area) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(area->owned);

    destroy_owned(&area->payload.owned);
    JSON_DEFLATE_TRACE_POP();
}

void json_deflate_bump_borrowed_init(json_deflate_bump_area_t * const area, const mem_region_t region) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(region.ptr);
    ASSERT(!*get_next(area));

    area->owned = false;
    *get_next(area) = 1;

    json_deflate_bump_area_borrowed_t * const borrowed = &area->payload.borrowed;

    borrowed->borrowed_buffer = region.ptr;
    borrowed->capacity = region.size;
    borrowed->readonly = false;
    JSON_DEFLATE_TRACE_POP();
}

void json_deflate_bump_borrowed_init_readonly(json_deflate_bump_area_t * const area, const const_mem_region_t region) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(region.ptr);
    ASSERT(!*get_next(area));

    json_deflate_bump_borrowed_init(area, MEM_REGION(.ptr = region.ptr, .size = region.size));
    area->payload.borrowed.readonly = true;
    JSON_DEFLATE_TRACE_POP();
}

void * json_deflate_bump_alloc(json_deflate_bump_area_t * const area, const size_t size, const size_t alignment) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);

    const size_t actual_size = size ? size : 1;
    const size_t actual_alignment = alignment ? alignment : 1;

    uint8_t * ret = get_next_free_space(area, actual_size, actual_alignment);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

void * json_deflate_bump_alloc_array(json_deflate_bump_area_t * const area, const size_t count, const size_t elem_size, const size_t alignment) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);

    void * ret = json_deflate_bump_alloc(area, count * elem_size, alignment);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

char * json_deflate_bump_store_str(json_deflate_bump_area_t * const area, const char * const str) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);
    ASSERT(str);

    size_t len = strlen(str) + 1;
    char * const ptr = json_deflate_bump_alloc_array(area, (int)len, sizeof(char), sizeof(char));
    strcpy_s(ptr, len, str);
    JSON_DEFLATE_TRACE_POP();
    return ptr;
}

void * json_deflate_bump_copy_object(json_deflate_bump_area_t * const base, const void * const src, const size_t size, const size_t alignment) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    void * const ptr = json_deflate_bump_alloc(base, size, alignment);
    memcpy(ptr, src, size);
    JSON_DEFLATE_TRACE_POP();
    return ptr;
}

void * json_deflate_bump_get_ptr(const json_deflate_bump_area_t * const area, const json_deflate_bump_ptr_t offset) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);

    if (!offset) {
        JSON_DEFLATE_TRACE_POP();
        return NULL;
    }

    uint8_t * ret = offset_to_ptr(area, offset);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

size_t json_deflate_bump_get_offset(const json_deflate_bump_area_t * const area, const void * const ptr) {
    JSON_DEFLATE_TRACE_PUSH_FN();
    ASSERT(area);

    if (!ptr) {
        JSON_DEFLATE_TRACE_POP();
        return 0;
    }

    size_t ret = ptr_to_offset(area, ptr);
    JSON_DEFLATE_TRACE_POP();
    return ret;
}

void * json_deflate_get_single_buffer(const json_deflate_bump_area_t * const area) {
    ASSERT(area);

    return get_single_buffer(area);
}

bool json_deflate_bump_initialized(json_deflate_bump_area_t * const area) {
    ASSERT(area);

    return *get_next(area) != 0;
}
