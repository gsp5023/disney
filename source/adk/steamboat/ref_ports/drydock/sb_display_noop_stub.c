/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include _PCH
#include "impl_tracking.h"
#include "source/adk/steamboat/sb_display.h"

bool sb_enumerate_display_modes(
    const int32_t display_index,
    const int32_t display_mode_index,
    sb_enumerate_display_modes_result_t * const out_results) {
    UNUSED(display_index);
    UNUSED(display_mode_index);
    UNUSED(out_results);

    NOT_IMPLEMENTED_EX;

    return false;
}

sb_window_t * sb_init_main_display(
    const int display_index,
    const int display_mode_index,
    const char * const title) {
    UNUSED(display_index);
    UNUSED(display_mode_index);
    UNUSED(title);

    NOT_IMPLEMENTED_EX;

    return NULL;
}

bool sb_set_main_display_refresh_rate(const int32_t hz) {
    UNUSED(hz);

    NOT_IMPLEMENTED_EX;

    return false;
}

void sb_get_window_client_area(
    sb_window_t * const window,
    int * const out_width,
    int * const out_height) {
    UNUSED(window);
    UNUSED(out_width);
    UNUSED(out_height);

    NOT_IMPLEMENTED_EX;
}

void sb_destroy_main_window(void) {
    NOT_IMPLEMENTED_EX;
}
