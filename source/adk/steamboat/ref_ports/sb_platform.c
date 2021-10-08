/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 sb_platform.c

 steamboat platform functions
*/

#include _PCH

#include "source/adk/steamboat/sb_platform.h"

#include "source/adk/log/log.h"

void sb_report_app_metrics(
    const char * const app_id,
    const char * const app_name,
    const char * const app_version) {
    ASSERT(app_id);
    ASSERT(app_name);
    ASSERT(app_version);

    LOG_ALWAYS(0, "app metrics: %s - %s - %s", app_id, app_name, app_version);
    // App will provide nul terminated utf8 inputs for app_id/app_name/app_version.
    // It is up to the implementation to determine how they wish to report the metrics.
}
