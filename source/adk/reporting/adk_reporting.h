/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
    ADK reporting API.
*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    adk_reporting_max_string_length = 256
};

typedef struct adk_reporting_init_options_t adk_reporting_init_options_t;
typedef struct adk_reporting_instance_t adk_reporting_instance_t;

typedef enum adk_reporting_event_level_e {
    event_level_unknown = 0,
    event_level_debug,
    event_level_info,
    event_level_warning,
    event_level_error,
    event_level_fatal
} adk_reporting_event_level_e;

typedef struct adk_reporting_key_val_t {
    const char * key;
    const char * value;
    struct adk_reporting_key_val_t * next;
} adk_reporting_key_val_t;

// Create/free adk_reporting_instance_t
adk_reporting_instance_t * adk_reporting_instance_create(const adk_reporting_init_options_t * const options);
void adk_reporting_instance_free(adk_reporting_instance_t * const instance);

// Push/clear tags that will be sent on every event. [global tags]
void adk_reporting_instance_push_tag(adk_reporting_instance_t * const instance, const char * const key, const char * const value);
void adk_reporting_instance_clear_tags(adk_reporting_instance_t * const instance);

// periodic service tick, may dispatch network events.
bool adk_reporting_tick(adk_reporting_instance_t * const instance);

void adk_reporting_report_msg(adk_reporting_instance_t * const instance, const char * const file, const int line, const char * const func, const adk_reporting_event_level_e level, adk_reporting_key_val_t * const tags, const char * const msg, va_list args);
static inline void adk_reporting_report_msg_va(adk_reporting_instance_t * const instance, const char * const file, const int line, const char * const func, const adk_reporting_event_level_e level, adk_reporting_key_val_t * const tags, const char * const msg, ...) {
    va_list args;
    va_start(args, msg);
    adk_reporting_report_msg(instance, file, line, func, level, tags, msg, args);
    va_end(args);
}
void adk_reporting_report_exception(adk_reporting_instance_t * const instance, const char * const file, const int line, const char * const func, const adk_reporting_event_level_e level, adk_reporting_key_val_t * const tags, void ** stacktrace, const size_t stack_size, const char * const error_type, const char * const error_message, va_list args);
static inline void adk_reporting_report_exception_va(adk_reporting_instance_t * const instance, const char * const file, const int line, const char * const func, const adk_reporting_event_level_e level, adk_reporting_key_val_t * const tags, void ** stacktrace, const size_t stack_size, const char * const error_type, const char * const error_message, ...) {
    va_list args;
    va_start(args, error_message);
    adk_reporting_report_exception(instance, file, line, func, level, tags, stacktrace, stack_size, error_type, error_message, args);
    va_end(args);
}

// MESSAGE INTERFACE
#define ADK_REPORTING_REPORT_MSG(instance, event_level, tags, ...)                                             \
    do {                                                                                                       \
        adk_reporting_report_msg_va(instance, __FILE__, __LINE__, __func__, event_level, tags, ##__VA_ARGS__); \
    } while (0)

#define ADK_REPORTING_REPORT_MSG_DEBUG(instance, tags, ...) \
    ADK_REPORTING_REPORT_MSG(instance, event_level_debug, tags, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_MSG_INFO(instance, tags, ...) \
    ADK_REPORTING_REPORT_MSG(instance, event_level_info, tags, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_MSG_WARN(instance, tags, ...) \
    ADK_REPORTING_REPORT_MSG(instance, event_level_warning, tags, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_MSG_ERROR(instance, tags, ...) \
    ADK_REPORTING_REPORT_MSG(instance, event_level_error, tags, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_MSG_FATAL(instance, tags, ...) \
    ADK_REPORTING_REPORT_MSG(instance, event_level_fatal, tags, ##__VA_ARGS__)

// EXCEPTION/STACKTRACE INTERFACE
#define ADK_REPORTING_REPORT_EXCEPTION(instance, event_level, tags, stacktrace, stack_size, error_type, ...)                                             \
    do {                                                                                                                                                 \
        adk_reporting_report_exception_va(instance, __FILE__, __LINE__, __func__, event_level, tags, stacktrace, stack_size, error_type, ##__VA_ARGS__); \
    } while (0)

#define ADK_REPORTING_REPORT_EXCEPTION_DEBUG(instance, tags, stacktrace, stack_size, error_type, ...) \
    ADK_REPORTING_REPORT_EXCEPTION(instance, event_level_debug, tags, stacktrace, stack_size, error_type, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_EXCEPTION_INFO(instance, tags, stacktrace, stack_size, error_type, ...) \
    ADK_REPORTING_REPORT_EXCEPTION(instance, event_level_info, tags, stacktrace, stack_size, error_type, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_EXCEPTION_WARN(instance, tags, stacktrace, stack_size, error_type, ...) \
    ADK_REPORTING_REPORT_EXCEPTION(instance, event_level_warning, tags, stacktrace, stack_size, error_type, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_EXCEPTION_ERROR(instance, tags, stacktrace, stack_size, error_type, ...) \
    ADK_REPORTING_REPORT_EXCEPTION(instance, event_level_error, tags, stacktrace, stack_size, error_type, ##__VA_ARGS__)

#define ADK_REPORTING_REPORT_EXCEPTION_FATAL(instance, tags, stacktrace, stack_size, error_type, ...) \
    ADK_REPORTING_REPORT_EXCEPTION(instance, event_level_fatal, tags, stacktrace, stack_size, error_type, ##__VA_ARGS__)

static inline const char * string_from_event_level(const adk_reporting_event_level_e level) {
    static const char * strings[] = {"unknown", "debug", "info", "warning", "error", "fatal"};
    return strings[level];
}

#ifdef __cplusplus
}
#endif