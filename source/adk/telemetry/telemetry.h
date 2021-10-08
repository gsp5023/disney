/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
telemetry.h

function telemetry support
*/

#pragma once

#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _TELEMETRY
#include _RAD_TM_H

#define TRACE_PUSH_FN() tmEnter(0, 0, __func__)
#define TRACE_PUSH(_name) tmEnter(0, 0, (_name))
#define TRACE_POP() tmLeave(0)
#define TRACE_TICK() tmTick(0)
#define TRACE_NAME_THREAD(_name) tmThreadName(0, 0, (_name))
#define TRACE_HEAP(_heap) tmPlot(0, TM_PLOT_UNITS_REAL, TM_PLOT_DRAW_LINE, (_heap)->internal.used_block_size, (_heap)->name)
#define TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) tmBeginTimeSpan(0, (tm_uint64)(uintptr_t)(_id), TMZF_NONE, span_name_fmt_str, ##__VA_ARGS__)
#define TRACE_TIME_SPAN_END(_id) tmEndTimeSpan(0, (tm_uint64)(uintptr_t)(_id))
#else
#define TRACE_PUSH_FN()
#define TRACE_PUSH(_name)
#define TRACE_POP()
#define TRACE_TICK()
#define TRACE_NAME_THREAD(_name)
#define TRACE_HEAP(_heap)
#define TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define TRACE_TIME_SPAN_END(_id)
#endif

void telemetry_init(const char * const address, const int port);
void telemetry_shutdown();

#ifdef __cplusplus
}
#endif