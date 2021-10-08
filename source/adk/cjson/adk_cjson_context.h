/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

/// cJSON context provides memory allocations hooks via `cJSON_InitHooks` for all m5 components.
/// cJSON has been modified for m5 use to include a `cJSON_Env * env` parameter on all functions that will
/// invoke the init hooks. This `env` is then passed to the init hooks for m5 use. For this cJSON context,
/// we give `cJSON_Env` a concrete structure and will invoke the context `callbacks` below with `ctx`.

typedef void * (*adk_cjson_context_malloc_t)(void * const ctx, const size_t size);
typedef void (*adk_cjson_context_free_t)(void * const ctx, void * const ptr);

typedef struct adk_cjson_context_callbacks_t {
    adk_cjson_context_malloc_t malloc;
    adk_cjson_context_free_t free;
} adk_cjson_context_callbacks_t;

struct cJSON_Env {
    void * ctx;
    adk_cjson_context_callbacks_t callbacks;
};

void adk_cjson_context_initialize();
