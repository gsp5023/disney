/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/cjson/adk_cjson_context.h"

#include "extern/cjson/cJSON.h"
#include "source/adk/steamboat/sb_thread.h"

static void * cjson_context_malloc(cJSON_Env * const env, const size_t size) {
    return env->callbacks.malloc(env->ctx, size);
}

static void cjson_context_free(cJSON_Env * const env, void * const ptr) {
    env->callbacks.free(env->ctx, ptr);
}

void adk_cjson_context_initialize() {
    cJSON_Hooks hooks = {
        .malloc_fn = cjson_context_malloc,
        .free_fn = cjson_context_free,
    };

    cJSON_InitHooks(&hooks);
}
