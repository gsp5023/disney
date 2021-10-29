/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
log.h

Core Logging

Structured log messages via cnc_bus connection, defined levels, category tagging
*/

#pragma once

#include "source/adk/reporting/adk_reporting.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/sb_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT
typedef enum log_level_e {
    log_level_debug = 0,
    log_level_info,
    log_level_warn,
    log_level_error,
    log_level_always,
    log_level_app,
    log_level_none,
    num_log_levels
} log_level_e;

enum {
    max_log_msg_length = UINT16_MAX
};

typedef uint8_t log_level_t;

const char * log_get_level_name(const log_level_e level);

EXT_EXPORT const char * log_get_level_short_name(const log_level_e level);

FFI_EXPORT EXT_EXPORT log_level_e log_get_min_level();

void log_set_min_level(const log_level_e level);

#define LOG(log_level, fourcc_tag, ...)                                                         \
    do {                                                                                        \
        if (log_level >= log_get_min_level()) {                                                 \
            log_message_va(__FILE__, __LINE__, __func__, log_level, fourcc_tag, ##__VA_ARGS__); \
        }                                                                                       \
    } while (0)

#ifdef NDEBUG
#define LOG_DEBUG(fourcc_tag, ...)                        \
    do {                                                  \
        if (0) {                                          \
            log_message_va(0, 0, 0, 0, 0, ##__VA_ARGS__); \
        }                                                 \
    } while (0)
#else
#define LOG_DEBUG(fourcc_tag, ...) \
    LOG(log_level_debug, fourcc_tag, ##__VA_ARGS__)
#endif

#define LOG_INFO(fourcc_tag, ...) \
    LOG(log_level_info, fourcc_tag, ##__VA_ARGS__)

#define LOG_WARN(fourcc_tag, ...) \
    LOG(log_level_warn, fourcc_tag, ##__VA_ARGS__)

#define LOG_ERROR(fourcc_tag, ...) \
    LOG(log_level_error, fourcc_tag, ##__VA_ARGS__)

#define LOG_ALWAYS(fourcc_tag, ...) \
    LOG(log_level_always, fourcc_tag, ##__VA_ARGS__)

EXT_EXPORT void log_message(
    const char * const file,
    const int line,
    const char * const func,
    const log_level_t level,
    const uint32_t fourcc_tag,
    const char * const msg,
    va_list args);

static inline void log_message_va(
    const char * const file,
    const int line,
    const char * const func,
    const log_level_t level,
    const uint32_t fourcc_tag,
    const char * const msg,
    ...) {
    va_list args;
    va_start(args, msg);
    log_message(file, line, func, level, fourcc_tag, msg, args);
    va_end(args);
}

FFI_EXPORT void adk_log_app_msg(
    FFI_PTR_WASM const char * const file,
    const uint32_t line,
    FFI_PTR_WASM const char * const func,
    const log_level_e level,
    const uint32_t tag,
    FFI_PTR_WASM const char * const msg);

#ifdef __cplusplus
}
#endif
