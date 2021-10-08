/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 private/log_p.h

 Core Logging

 Structured log messages via cnc_bus connection, defined levels, category tagging

 Private API
 */

#pragma once

#include "source/adk/cncbus/cncbus.h"
#include "source/adk/log/log.h"
#include "source/adk/steamboat/sb_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct log_header_t {
    sb_time_since_epoch_t time_since_epoch;
    uint32_t fourcc_tag;
    const char * func;
    const char * file;
    uint16_t line;
    log_level_t level;
} log_header_t;

EXT_EXPORT void log_init(cncbus_t * const bus, const cncbus_address_t address, const cncbus_address_t subnet_mask, adk_reporting_instance_t * const reporting_instance);

// Shuts down the publishing of logs to the bus, but still allows 'basic' logging
EXT_EXPORT void log_shutdown();

void log_msg_print_basic(const log_header_t * const log_header, const char * log_msg);

#ifdef __cplusplus
}
#endif
