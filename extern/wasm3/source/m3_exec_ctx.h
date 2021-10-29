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

void m3_exec_ctx_push_call(m3_exec_ctx * const ctx, IM3Function function);
void m3_exec_ctx_pop_call(m3_exec_ctx * const ctx);

d_m3EndExternC
