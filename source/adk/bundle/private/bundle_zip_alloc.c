/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  bundle_zip_alloc.c -- redirect standard heap allocation functions for libzip
*/

#include "bundle_zip_alloc.h"

#include "source/adk/runtime/memory.h"

static const char tag_lzip[] = "LZIP";

static struct {
    heap_t * heap;
} statics;

heap_t * bundle_zip_set_heap(heap_t * const heap) {
    heap_t * const old_heap = statics.heap;
    statics.heap = heap;
    return old_heap;
}

void * bundle_zip_malloc(const size_t size) {
    return heap_unchecked_alloc(statics.heap, size, tag_lzip);
}

void * bundle_zip_calloc(const size_t nmemb, const size_t size) {
    return heap_unchecked_calloc(statics.heap, nmemb * size, tag_lzip);
}

void * bundle_zip_realloc(void * const ptr, const size_t size) {
    return heap_unchecked_realloc(statics.heap, ptr, size, tag_lzip);
}

void bundle_zip_free(void * const ptr) {
    if (ptr) {
        heap_free(statics.heap, ptr, tag_lzip);
    }
}

char *
bundle_zip_strdup(const char * const str) {
    const size_t len = strlen(str) + 1;
    char * const new_str = heap_unchecked_alloc(statics.heap, len, tag_lzip);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    return new_str;
}
