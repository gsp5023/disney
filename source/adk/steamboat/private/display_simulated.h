/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
display_simulated.h

support for loading simulated_display_modes.txt a text file of common desired display modes
    * also needed for tests since rhi_tests are run in 720p - later we'll be forcing tests against other resolutions (1080p)
*/

#pragma once

#include "source/adk/steamboat/sb_display.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    max_simulated_display_modes = 64
};

typedef struct simulated_display_modes_t {
    sb_display_mode_t display_modes[max_simulated_display_modes];
    int display_mode_count;
} simulated_display_modes_t;

void load_simulated_display_modes(simulated_display_modes_t * const out_display_modes, sb_display_mode_t max_screen_size);

#ifdef __cplusplus
}
#endif