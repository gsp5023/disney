/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * ARRAY minimal growable array
 * ==========================================================================*/
struct cg_heap_t;

void * cg_alloc(struct cg_heap_t * const cg_heap, const size_t alloc_size, const char * const tag);
void * cg_calloc(struct cg_heap_t * const cg_heap, const size_t alloc_size, const char * const tag);
void * cg_realloc(struct cg_heap_t * const cg_heap, void * const ptr, const size_t new_size, const char * const tag);
void cg_free(struct cg_heap_t * const cg_heap, void * ptr, const char * const tag);

typedef struct cg_const_allocation_t {
    struct cg_heap_t * cg_heap;
    const_mem_region_t region;
} cg_const_allocation_t;

typedef struct cg_allocation_t {
    union {
        struct {
            struct cg_heap_t * cg_heap;
            mem_region_t region;
        };
        cg_const_allocation_t consted;
    };
} cg_allocation_t;

cg_allocation_t cg_unchecked_alloc(struct cg_heap_t * const cg_heap, const size_t alloc_size, const char * const tag);
cg_allocation_t cg_unchecked_realloc(struct cg_heap_t * const cg_heap, const cg_allocation_t old_alloc, const size_t new_size, const char * const tag);
void cg_free_alloc(const cg_allocation_t alloc, const char * const tag);
void cg_free_const_alloc(const cg_const_allocation_t alloc, const char * const tag);

// this block stores an array of arbitrary elements (ints, structs, etc)

typedef struct cg_mem_block_t {
    struct cg_mem_block_t * next;
    int num_elements; // number of elements in the list
    int capacity; // the number of elements we can store without growing
    int size_in_bytes; // how many bytes the backing allocation for this block is, not counting this structure
#if _MACHINE_SIZE == 64
    int padd;
#endif
} cg_mem_block_t;

// make sure cg_mem_block_t is a multiple of 8 bytes for natural alignment
// with array elements.
#if _MACHINE_SIZE == 32
STATIC_ASSERT(sizeof(cg_mem_block_t) == 16);
#else
STATIC_ASSERT(sizeof(cg_mem_block_t) == 24);
#endif

/* -------------------------------------------------------------------------
 * dynamic array starts at 1 element and grows by 2x when full
 * -------------------------------------------------------------------------*/

#define CG_ARRAY(T, context_ptr, arr, tag)                                                             \
    do {                                                                                               \
        if (context_ptr->free_block_chain) {                                                           \
            cg_mem_block_t * block = context_ptr->free_block_chain;                                    \
            context_ptr->free_block_chain = block->next;                                               \
            block->num_elements = 0;                                                                   \
            block->capacity = block->size_in_bytes / sizeof(T);                                        \
            arr = (T *)(((uint8_t *)block) + sizeof(cg_mem_block_t));                                  \
        } else {                                                                                       \
            cg_mem_block_t * block = cg_alloc(&context_ptr->cg_heap_low, sizeof(cg_mem_block_t), tag); \
            ZEROMEM(block);                                                                            \
            arr = (T *)(((uint8_t *)block) + sizeof(cg_mem_block_t));                                  \
        }                                                                                              \
    } while (0)

#define CG_ARRAY_FREE(context_ptr, arr, tag)                                                    \
    do {                                                                                        \
        cg_mem_block_t * block = (cg_mem_block_t *)(((uint8_t *)arr) - sizeof(cg_mem_block_t)); \
        block->next = context_ptr->free_block_chain;                                            \
        context_ptr->free_block_chain = block;                                                  \
        arr = NULL;                                                                             \
    } while (0)

#define CG_ARRAY_SIZE(arr) ((cg_mem_block_t *)(((uint8_t *)arr) - sizeof(cg_mem_block_t)))->num_elements
#define CG_ARRAY_CAPACITY(arr) ((cg_mem_block_t *)(((uint8_t *)arr) - sizeof(cg_mem_block_t)))->capacity

#define CG_ARRAY_PUSH(T, context_ptr, arr, value, tag)                                                                \
    do {                                                                                                              \
        cg_mem_block_t * block = (cg_mem_block_t *)(((uint8_t *)arr) - sizeof(cg_mem_block_t));                       \
        const int store_index = block->num_elements;                                                                  \
        ++block->num_elements;                                                                                        \
        if (block->capacity == 0) {                                                                                   \
            block->capacity = 1;                                                                                      \
            block->size_in_bytes = FWD_ALIGN_INT(block->capacity * sizeof(T), 8);                                     \
            block = cg_realloc(&context_ptr->cg_heap_low, block, sizeof(cg_mem_block_t) + block->size_in_bytes, tag); \
            arr = (T *)(((uint8_t *)block) + sizeof(cg_mem_block_t));                                                 \
        } else if (block->num_elements > block->capacity) {                                                           \
            block->capacity *= 2;                                                                                     \
            block->size_in_bytes = FWD_ALIGN_INT(block->capacity * sizeof(T), 8);                                     \
            block = cg_realloc(&context_ptr->cg_heap_low, block, sizeof(cg_mem_block_t) + block->size_in_bytes, tag); \
            arr = (T *)(((uint8_t *)block) + sizeof(cg_mem_block_t));                                                 \
        }                                                                                                             \
        arr[store_index] = value;                                                                                     \
    } while (0)

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
