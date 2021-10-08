/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
    ADK reporting API.
*/

#pragma once

#include "adk_reporting_utils.h"
#include "source/adk/http/adk_httpx.h"
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Options to configure adk_reporting_sentry
*/
struct adk_reporting_init_options_t {
    // Sentry client dsn string. [Required]
    const char * sentry_dsn;

    // Heap. [Required]
    heap_t * const heap;

    // Name to identify reporter. Will be uploaded to sentry in "logger" field. [Required]
    const char * reporter_name;

    // Client for uploading. [Required]
    adk_httpx_client_t * httpx_client;

    // Set the level to report to sentry  [Required]
    adk_reporting_event_level_e min_report_level;

    // The size of the send queue's buffer  [Required]
    uint32_t send_queue_size;
};

#ifdef __cplusplus
}
#endif