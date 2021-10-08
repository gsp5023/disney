/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/steamboat/sb_thread.h"

enum {
    adk_app_metrics_string_max = 256
};

typedef struct adk_app_metrics_t {
    char app_id[adk_app_metrics_string_max];
    char app_name[adk_app_metrics_string_max];
    char app_version[adk_app_metrics_string_max];
} adk_app_metrics_t;

typedef enum adk_app_metrics_result_e {
    adk_app_metrics_success,
    adk_app_metrics_no_app
} adk_app_metrics_result_e;

EXT_EXPORT void adk_app_metrics_init();
EXT_EXPORT void adk_app_metrics_shutdown();

EXT_EXPORT FFI_EXPORT FFI_NAME(adk_report_app_metrics) void adk_app_metrics_report(
    FFI_PTR_WASM const char * const app_id,
    FFI_PTR_WASM const char * const app_name,
    FFI_PTR_WASM const char * const app_version);

EXT_EXPORT adk_app_metrics_result_e adk_app_metrics_get(adk_app_metrics_t * app_info);