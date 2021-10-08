/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  bundle_zip_alloc.h -- standard heap allocation functions for libzip
*/

#include <stddef.h>

// Sets libzip's heap, returning old heap (for error/state checking).  Must be called before any libzip function is
// called.
struct heap_t * bundle_zip_set_heap(struct heap_t * const heap);

void * bundle_zip_malloc(const size_t size);
void * bundle_zip_calloc(const size_t nmemb, const size_t size);
void * bundle_zip_realloc(void * const ptr, const size_t size);
char * bundle_zip_strdup(const char * const str);
void bundle_zip_free(void * const ptr);
