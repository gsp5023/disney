/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "m3_core.h"

d_m3BeginExternC

#define M3_CALLSTACK_MAX_DEPTH 256

typedef struct m3_exec_ctx {
    IM3Function callstack[M3_CALLSTACK_MAX_DEPTH];
    size_t callstack_len;
} m3_exec_ctx;

static inline void m3_exec_ctx_push_call(m3_exec_ctx * const ctx, IM3Function function) {
    assert(ctx->callstack_len <= M3_CALLSTACK_MAX_DEPTH);
    ctx->callstack[ctx->callstack_len++] = function;
}

static inline void m3_exec_ctx_pop_call(m3_exec_ctx * const ctx) {
    ctx->callstack_len -= 1;
}

d_m3EndExternC
