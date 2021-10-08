/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
adk_sample_common.c

ADK sample application helpers
*/

#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/log/log.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/runtime/app/app.h"
#include "source/adk/steamboat/sb_display.h"

void adk_app_init_main_display(const char * const app_name) {
    int display_index = 0;
    sb_enumerate_display_modes_result_t display_result;
    for (; sb_enumerate_display_modes(display_index, 0, &display_result); ++display_index) {
        if (display_result.status & sb_display_modes_primary_display) {
            break;
        }
    }

    int display_mode_index = 0;
    int preferred_mode_index = -1;
    int fallback_mode_index = -1;

    for (; sb_enumerate_display_modes(display_index, display_mode_index, &display_result); ++display_mode_index) {
        // some of our laptops are still on 1080p screens, so having this set to 1080 makes it difficult to debug the apps as they may be full screen
        if (display_result.display_mode.height == 720) {
            preferred_mode_index = display_mode_index;
            break;
        } else {
            fallback_mode_index = display_mode_index;
        }
    }

    ASSERT((fallback_mode_index != -1) || (preferred_mode_index != -1));

    app_init_main_display(display_index, preferred_mode_index == -1 ? fallback_mode_index : preferred_mode_index, app_name);
}

bool sample_get_runtime_duration(const uint32_t argc, const char * const * const argv, milliseconds_t * const out_runtime) {
    const char * const runtime_arg = getargarg("--sample-runtime", argc, argv);
    *out_runtime = (milliseconds_t){0};
    if (!runtime_arg) {
        return false;
    }

    out_runtime->ms = (uint32_t)atoi(runtime_arg) * 1000;
    return true;
}