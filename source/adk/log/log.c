/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 log.c

 Core Logging

 Structured log messages via cnc_bus connection, defined levels, category tagging
 */

#include "source/adk/cncbus/cncbus.h"
#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/log/private/log_p.h"
#include "source/adk/steamboat/sb_platform.h"

static struct {
    cncbus_t * bus;
    cncbus_address_t address;
    cncbus_address_t subnet_mask;
    // slow_buffer is only used when we overflow
    // our modestly sized stack-local buffer for
    // formatting log messages.
    // no mutex is used here so log has no dependencies
    // on runtime
    sb_atomic_int32_t slow_buffer_hazard;
    char slow_buffer[max_log_msg_length];
    adk_reporting_instance_t * reporter;
} statics;

void log_init(cncbus_t * const bus, const cncbus_address_t address, const cncbus_address_t subnet_mask, adk_reporting_instance_t * const reporting_instance) {
    ZEROMEM(&statics);
    statics.bus = bus;
    statics.address = address;
    statics.subnet_mask = subnet_mask;
    statics.reporter = reporting_instance;
}

void log_shutdown() {
    ZEROMEM(&statics);
}

void log_message(const char * const file, const int line, const char * const func, const log_level_t level, const uint32_t fourcc_tag, const char * const msg, va_list args) {
    ASSERT(level < num_log_levels);

    if (statics.reporter != NULL) {
        adk_reporting_key_val_t tags = {.key = "subsystem", .value = VAPRINTF("%*.*s", 4, 4, (const char *)&fourcc_tag)};
        const adk_reporting_event_level_e event_level = level >= log_level_always ? event_level_debug : level + 1;
        adk_reporting_report_msg(statics.reporter, file, line, func, event_level, &tags, msg, args);
    }

    const log_header_t log_header = {
        .time_since_epoch = sb_get_time_since_epoch(),
        .fourcc_tag = fourcc_tag,
        .func = func,
        .file = file,
        .line = (uint16_t)line,
        .level = level,
    };

    enum { max_log_msg_stack_len = 8192 };
    char log_msg[max_log_msg_stack_len];
    char * p_log_msg = log_msg;

    // future TODO, write directly to cncbus msg data buf of reserved size if can be guaranteed to occupy within a single cnc bus memory fragment

    int message_size = vsnprintf(log_msg, max_log_msg_stack_len, msg, args) + 1; // vsnprintf will null-terminate even on truncate, but the null terminator is not included in the returned count

    // required message size exceeds our stack buffer len (including the null terminator)
    if (message_size > max_log_msg_stack_len) {
        // acquire the hazard to write to the buffer
        while (sb_atomic_cas(&statics.slow_buffer_hazard, 1, 0, memory_order_relaxed) != 0) {
        }

        message_size = vsnprintf(statics.slow_buffer, max_log_msg_length, msg, args) + 1; // vsnprintf will null-terminate even on truncate, but the null terminator is not included in the returned count

        p_log_msg = statics.slow_buffer;
    }

    if (statics.bus) {
        // vsnprintf will return the number of character which would have been written given enough space, so clamp to the max length.
        if (message_size > max_log_msg_length) {
            message_size = max_log_msg_length;
        }

        cncbus_msg_t * const cncbus_msg = cncbus_msg_begin_unchecked(statics.bus, cncbus_msg_type_log_v1);
        // Write message metadata followed by the message contents
        cncbus_msg_write_checked(cncbus_msg, &log_header, sizeof(log_header));
        cncbus_msg_write_checked(cncbus_msg, p_log_msg, (int)message_size);

        if (p_log_msg != log_msg) {
            // release the hazard, we are done with the buffer
            sb_atomic_store(&statics.slow_buffer_hazard, 0, memory_order_relaxed);
        }

        cncbus_send_async(cncbus_msg, CNCBUS_INVALID_ADDRESS, statics.address, statics.subnet_mask, NULL);
    } else {
        log_msg_print_basic(&log_header, p_log_msg);
        if (p_log_msg != log_msg) {
            // release the hazard, we are done with the buffer
            sb_atomic_store(&statics.slow_buffer_hazard, 0, memory_order_relaxed);
        }
    }
}

static const char * const log_level_names[num_log_levels] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "ALWAYS"};

static const char * const log_level_short_names[num_log_levels] = {
    "DBG",
    "INF",
    "WRN",
    "ERR",
    "ALW"};

static log_level_e minimum_log_level =
#ifdef NDEBUG
    log_level_info;
#else
    log_level_debug;
#endif

log_level_e log_get_min_level() {
    return minimum_log_level;
}

void log_set_min_level(const log_level_e level) {
    ASSERT(level < num_log_levels);

    minimum_log_level = level;
}

const char * log_get_level_name(const log_level_e level) {
    ASSERT(level < num_log_levels);

    return log_level_names[level];
}

const char * log_get_level_short_name(const log_level_e level) {
    ASSERT(level < num_log_levels);

    return log_level_short_names[level];
}

void log_msg_print_basic(const log_header_t * const log_header, const char * log_msg) {
    ASSERT(log_header);
    ASSERT(log_msg);

    // example output (compatible w/Visual Studio IDE):
    // c:\git\ncp-m5\tests\log_tests.c(38): test_log()
    //       [01049979.771456][TEST][ALW] this is a test message
    // TODO provision output for non-visual studio IDEs based on config setting
    struct tm time_info;
    sb_seconds_since_epoch_to_localtime(log_header->time_since_epoch.seconds, &time_info);

    char time_str[27];
    snprintf(time_str, ARRAY_SIZE(time_str), "%04d-%02d-%02d|%02d:%02d:%02d.%06d", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec, log_header->time_since_epoch.microseconds);

    debug_write_line("%s(%d): %s: %s()\n\t[%s][%*.*s][%s] %s", log_header->file, log_header->line, log_get_level_name(log_header->level), log_header->func, time_str, 4, 4, (char *)&log_header->fourcc_tag, log_get_level_short_name(log_header->level), log_msg);
}
