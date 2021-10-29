/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
telemetry.h

function telemetry support
*/

#pragma once

#include "source/adk/runtime/memory.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/telemetry/trace_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _TELEMETRY
void telemetry_print_help();
void telemetry_init(const char * const address, const int port, const char * const telemetry_groups);
void telemetry_shutdown();
#endif

#ifdef _TELEMETRY
#if defined(_VADER)
#include "extern/private/rad-tools/telemetry/tm3/ps5/rad_tm_ps5_3.5.0.19/include/rad_tm.h"
#else
#include "extern/private/rad-tools/telemetry/tm3/linux/rad_tm_linux_3.5.0.19/include/rad_tm.h"
#endif

#define TRACE_PUSH_FN(_mask) tmEnter(_mask, 0, __func__)
#define TRACE_PUSH(_mask, _name) tmEnter(_mask, 0, (_name))
#define TRACE_POP(_mask) tmLeave(_mask)
#define TRACE_TICK() tmTick(0)
#define TRACE_NAME_THREAD(_name) tmThreadName(0, 0, (_name))
#define TRACE_HEAP(_heap) tmPlot(0, TM_PLOT_UNITS_REAL, TM_PLOT_DRAW_LINE, (_heap)->internal.used_block_size, (_heap)->name)
#define TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) tmBeginTimeSpan(0, (tm_uint64)(uintptr_t)(_id), TMZF_NONE, span_name_fmt_str, ##__VA_ARGS__)
#define TRACE_TIME_SPAN_END(_id) tmEndTimeSpan(0, (tm_uint64)(uintptr_t)(_id))
#define TRACE_ALLOC(_location, _ptr, _size, _ctx, ...) tmAllocEx(0, (_location), 0, (_ptr), (_size), (_ctx), ##__VA_ARGS__)
#define TRACE_FREE(_ptr) tmFree(0, (_ptr))
#else
#define TRACE_PUSH_FN(_mask)
#define TRACE_PUSH(_mask, _name)
#define TRACE_POP(_mask)
#define TRACE_TICK()
#define TRACE_NAME_THREAD(_name)
#define TRACE_HEAP(_heap)
#define TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define TRACE_TIME_SPAN_END(_id)
#define TRACE_ALLOC(_location, _ptr, _size, _ctx, ...)
#define TRACE_FREE(_ptr)
#endif

#ifdef _TELEMETRY
#define TTFI_TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ##__VA_ARGS__)
#define TTFI_TRACE_TIME_SPAN_END(_id) TRACE_TIME_SPAN_END(_id)
#else
#define TTFI_TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define TTFI_TRACE_TIME_SPAN_END(_id)
#endif

#ifdef _TELEMETRY
#define WASM_FFI_TRACE_MASK (1 << 1)
#define WASM_FFI_TRACE_PUSH(_name) TRACE_PUSH(WASM_FFI_TRACE_MASK, _name)
#define WASM_FFI_TRACE_POP() TRACE_POP(WASM_FFI_TRACE_MASK)
#else
#define WASM_FFI_TRACE_PUSH(_name)
#define WASM_FFI_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define WASM_FN_TRACE_MASK (1 << 2)
#define WASM_FN_TRACE_PUSH(_name) TRACE_PUSH(WASM_FN_TRACE_MASK, _name)
#define WASM_FN_TRACE_POP() TRACE_POP(WASM_FN_TRACE_MASK)
#else
#define WASM_FN_TRACE_PUSH(_name)
#define WASM_FN_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define RHI_TRACE_MASK (1 << 3)
#define RHI_TRACE_PUSH_FN() TRACE_PUSH_FN(RHI_TRACE_MASK)
#define RHI_TRACE_PUSH(_name) TRACE_PUSH(RHI_TRACE_MASK, _name)
#define RHI_TRACE_POP() TRACE_POP(RHI_TRACE_MASK)
#else
#define RHI_TRACE_PUSH_FN()
#define RHI_TRACE_PUSH(_name)
#define RHI_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define APP_THUNK_TRACE_MASK (1 << 4)
#define APP_THUNK_TRACE_PUSH_FN() TRACE_PUSH_FN(APP_THUNK_TRACE_MASK)
#define APP_THUNK_TRACE_PUSH(_name) TRACE_PUSH(APP_THUNK_TRACE_MASK, _name)
#define APP_THUNK_TRACE_POP() TRACE_POP(APP_THUNK_TRACE_MASK)
#else
#define APP_THUNK_TRACE_PUSH_FN()
#define APP_THUNK_TRACE_PUSH(_name)
#define APP_THUNK_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define MERLIN_TRACE_MASK (1 << 5)
#define MERLIN_TRACE_PUSH_FN() TRACE_PUSH_FN(MERLIN_TRACE_MASK)
#define MERLIN_TRACE_PUSH(_name) TRACE_PUSH(MERLIN_TRACE_MASK, _name)
#define MERLIN_TRACE_POP() TRACE_POP(MERLIN_TRACE_MASK)
#else
#define MERLIN_TRACE_PUSH_FN()
#define MERLIN_TRACE_PUSH(_name)
#define MERLIN_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define CG_FONT_TRACE_MASK (1 << 6)
#define CG_FONT_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ##__VA_ARGS__)
#define CG_FONT_TIME_SPAN_END(_id) TRACE_TIME_SPAN_END(_id)
#define CG_FONT_TRACE_PUSH_FN() TRACE_PUSH_FN(CG_FONT_TRACE_MASK)
#define CG_FONT_TRACE_PUSH(_name) TRACE_PUSH(CG_FONT_TRACE_MASK, _name)
#define CG_FONT_TRACE_POP() TRACE_POP(CG_FONT_TRACE_MASK)
#define CG_FONT_TRACE_HEAP(_heap) TRACE_HEAP(_heap)
#else
#define CG_FONT_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define CG_FONT_TIME_SPAN_END(_id)
#define CG_FONT_TRACE_PUSH_FN()
#define CG_FONT_TRACE_PUSH(_name)
#define CG_FONT_TRACE_POP()
#define CG_FONT_TRACE_HEAP(_heap)
#endif

#ifdef _TELEMETRY
#define CACHE_TRACE_MASK (1 << 7)
#define CACHE_TRACE_PUSH_FN() TRACE_PUSH_FN(CACHE_TRACE_MASK)
#define CACHE_TRACE_PUSH(_name) TRACE_PUSH(CACHE_TRACE_MASK, _name)
#define CACHE_TRACE_POP() TRACE_POP(CACHE_TRACE_MASK)
#else
#define CACHE_TRACE_PUSH_FN()
#define CACHE_TRACE_PUSH(_name)
#define CACHE_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define CG_TRACE_MASK (1 << 8)
#define CG_TRACE_PUSH_FN() TRACE_PUSH_FN(CG_TRACE_MASK)
#define CG_TRACE_PUSH(_name) TRACE_PUSH(CG_TRACE_MASK, _name)
#define CG_TRACE_POP() TRACE_POP(CG_TRACE_MASK)
#define CG_TRACE_HEAP(_heap) TRACE_HEAP(_heap)
#else
#define CG_TRACE_PUSH_FN()
#define CG_TRACE_PUSH(_name)
#define CG_TRACE_POP()
#define CG_TRACE_HEAP(_heap)
#endif

#ifdef _TELEMETRY
#define HTTPX_TRACE_MASK (1 << 9)
#define HTTPX_TRACE_PUSH_FN() TRACE_PUSH_FN(HTTPX_TRACE_MASK)
#define HTTPX_TRACE_PUSH(_name) TRACE_PUSH(HTTPX_TRACE_MASK, _name)
#define HTTPX_TRACE_POP() TRACE_POP(HTTPX_TRACE_MASK)
#else
#define HTTPX_TRACE_PUSH_FN()
#define HTTPX_TRACE_PUSH(_name)
#define HTTPX_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define WEBSOCKET_MINIMAL_TRACE_MASK (1 << 10)
#define WEBSOCKET_MINIMAL_TRACE_PUSH_FN() TRACE_PUSH_FN(WEBSOCKET_MINIMAL_TRACE_MASK)
#define WEBSOCKET_MINIMAL_TRACE_PUSH(_name) TRACE_PUSH(WEBSOCKET_MINIMAL_TRACE_MASK, _name)
#define WEBSOCKET_MINIMAL_TRACE_POP() TRACE_POP(WEBSOCKET_MINIMAL_TRACE_MASK)
#else
#define WEBSOCKET_MINIMAL_TRACE_PUSH_FN()
#define WEBSOCKET_MINIMAL_TRACE_PUSH(_name)
#define WEBSOCKET_MINIMAL_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define CURL_COMMON_TRACE_MASK (1 << 11)
#define CURL_COMMON_TRACE_PUSH_FN() TRACE_PUSH_FN(CURL_COMMON_TRACE_MASK)
#define CURL_COMMON_TRACE_PUSH(_name) TRACE_PUSH(CURL_COMMON_TRACE_MASK, _name)
#define CURL_COMMON_TRACE_POP() TRACE_POP(CURL_COMMON_TRACE_MASK)
#else
#define CURL_COMMON_TRACE_PUSH_FN()
#define CURL_COMMON_TRACE_PUSH(_name)
#define CURL_COMMON_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define HTTP_CURL_TRACE_MASK (1 << 12)
#define HTTP_CURL_TRACE_PUSH_FN() TRACE_PUSH_FN(HTTP_CURL_TRACE_MASK)
#define HTTP_CURL_TRACE_PUSH(_name) TRACE_PUSH(HTTP_CURL_TRACE_MASK, _name)
#define HTTP_CURL_TRACE_POP() TRACE_POP(HTTP_CURL_TRACE_MASK)
#else
#define HTTP_CURL_TRACE_PUSH_FN()
#define HTTP_CURL_TRACE_PUSH(_name)
#define HTTP_CURL_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define RUNTIME_TRACE_MASK (1 << 13)
#define RUNTIME_TRACE_PUSH_FN() TRACE_PUSH_FN(RUNTIME_TRACE_MASK)
#define RUNTIME_TRACE_PUSH(_name) TRACE_PUSH(RUNTIME_TRACE_MASK, _name)
#define RUNTIME_TRACE_POP() TRACE_POP(RUNTIME_TRACE_MASK)
#else
#define RUNTIME_TRACE_PUSH_FN()
#define RUNTIME_TRACE_PUSH(_name)
#define RUNTIME_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define JSON_DEFLATE_TRACE_MASK (1 << 14)
#define JSON_DEFLATE_TRACE_PUSH_FN() TRACE_PUSH_FN(JSON_DEFLATE_TRACE_MASK)
#define JSON_DEFLATE_TRACE_PUSH(_name) TRACE_PUSH(JSON_DEFLATE_TRACE_MASK, _name)
#define JSON_DEFLATE_TRACE_POP() TRACE_POP(JSON_DEFLATE_TRACE_MASK)
#else
#define JSON_DEFLATE_TRACE_PUSH_FN()
#define JSON_DEFLATE_TRACE_PUSH(_name)
#define JSON_DEFLATE_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define CG_GL_TRACE_MASK (1 << 15)
#define CG_GL_TRACE_PUSH_FN() TRACE_PUSH_FN(CG_GL_TRACE_MASK)
#define CG_GL_TRACE_PUSH(_name) TRACE_PUSH(CG_GL_TRACE_MASK, _name)
#define CG_GL_TRACE_POP() TRACE_POP(CG_GL_TRACE_MASK)
#else
#define CG_GL_TRACE_PUSH_FN()
#define CG_GL_TRACE_PUSH(_name)
#define CG_GL_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define GLFW_TRACE_MASK (1 << 16)
#define GLFW_TRACE_PUSH_FN() TRACE_PUSH_FN(GLFW_TRACE_MASK)
#define GLFW_TRACE_PUSH(_name) TRACE_PUSH(GLFW_TRACE_MASK, _name)
#define GLFW_TRACE_POP() TRACE_POP(GLFW_TRACE_MASK)
#else
#define GLFW_TRACE_PUSH_FN()
#define GLFW_TRACE_PUSH(_name)
#define GLFW_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define APP_TRACE_MASK (1 << 17)
#define APP_TRACE_PUSH_FN() TRACE_PUSH_FN(APP_TRACE_MASK)
#define APP_TRACE_PUSH(_name) TRACE_PUSH(APP_TRACE_MASK, _name)
#define APP_TRACE_POP() TRACE_POP(APP_TRACE_MASK)
#else
#define APP_TRACE_PUSH_FN()
#define APP_TRACE_PUSH(_name)
#define APP_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define WEBSOCKET_FULL_TRACE_MASK (1 << 18)
#define WEBSOCKET_FULL_TRACE_PUSH_FN() TRACE_PUSH_FN(WEBSOCKET_FULL_TRACE_MASK)
#define WEBSOCKET_FULL_TRACE_POP() TRACE_POP(WEBSOCKET_FULL_TRACE_MASK)
#else
#define WEBSOCKET_FULL_TRACE_PUSH_FN()
#define WEBSOCKET_FULL_TRACE_POP()
#endif

#ifdef _TELEMETRY
#define CG_IMAGE_TRACE_MASK (1 << 19)
#define CG_IMAGE_TRACE_PUSH_FN() TRACE_PUSH_FN(CG_IMAGE_TRACE_MASK)
#define CG_IMAGE_TRACE_POP() TRACE_POP(CG_IMAGE_TRACE_MASK)
#define CG_IMAGE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...) TRACE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ##__VA_ARGS__)
#define CG_IMAGE_TIME_SPAN_END(_id) TRACE_TIME_SPAN_END(_id)
#else
#define CG_IMAGE_TRACE_PUSH_FN()
#define CG_IMAGE_TRACE_POP()
#define CG_IMAGE_TIME_SPAN_BEGIN(_id, span_name_fmt_str, ...)
#define CG_IMAGE_TIME_SPAN_END(_id)
#endif

#define CURL_TRACE_FN(_visibility, _return_type, _function_name, _signature) TRACE_FN(_visibility, _return_type, _function_name, _signature, HTTP_CURL_TRACE_MASK)
#define CURL_TRACE_FN_VOID(_visibility, _function_name, _signature) TRACE_FN_VOID(_visibility, _function_name, _signature, HTTP_CURL_TRACE_MASK)

#define FFI_EXPORT_TELEMETRY FFI_EXPORT FFI_IMPLEMENT_IF_DEF(_TELEMETRY)

FFI_EXPORT_TELEMETRY FFI_NAME(telemetry_push) void app_telemetry_push(FFI_PTR_WASM const char * const func_name);
FFI_EXPORT_TELEMETRY FFI_NAME(telemetry_pop) void app_telemetry_pop(void);
FFI_EXPORT_TELEMETRY FFI_NAME(telemetry_span_begin) void app_telemetry_span_begin(const uint64_t id, FFI_PTR_WASM const char * const str);
FFI_EXPORT_TELEMETRY FFI_NAME(telemetry_span_end) void app_telemetry_span_end(const uint64_t id);

#undef FFI_EXPORT_TELEMETRY

#ifdef __cplusplus
}
#endif