/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

typedef struct adk_curl_context_t adk_curl_context_t;

typedef void * (*adk_curl_context_malloc_t)(void * const ctx, const size_t size, const char * const tag);
typedef void * (*adk_curl_context_calloc_t)(void * const ctx, const size_t size, const char * const tag);
typedef void (*adk_curl_context_free_t)(void * const ctx, void * const ptr, const char * const tag);
typedef void * (*adk_curl_context_realloc_t)(void * const ctx, void * const ptr, const size_t size, const char * const tag);

typedef struct adk_curl_context_callbacks_t {
    adk_curl_context_malloc_t malloc;
    adk_curl_context_calloc_t calloc;
    adk_curl_context_free_t free;
    adk_curl_context_realloc_t realloc;
} adk_curl_context_callbacks_t;

adk_curl_context_t * adk_curl_context_create(void * const ctx, const adk_curl_context_callbacks_t callbacks);

void adk_curl_context_destroy(adk_curl_context_t * const ctx);

adk_curl_context_t * adk_curl_get_context();

// Sets the curl context to a new value - returns the old context
adk_curl_context_t * adk_curl_set_context(adk_curl_context_t * const ctx);

// caches the current curl context and sets the given context for any global operations (such as malloc/free)
// Must be followed by CURL_POP_CTX within the same scope
#define CURL_PUSH_CTX(new_ctx) \
    {                          \
        adk_curl_context_t * const old_ctx = adk_curl_set_context(new_ctx)

// restores the context cached with CURL_PUSH_CTX
// Must be preceded by CURL_PUSH_CTX(new_ctx) in the same scope
#define CURL_POP_CTX()             \
    adk_curl_set_context(old_ctx); \
    }
