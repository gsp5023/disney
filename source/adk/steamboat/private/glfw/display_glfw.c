/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
display_glfw.c

glfw display details implementation
*/

#include _PCH

#include "GLFW/glfw3.h"
#include "source/adk/app_thunk/app_thunk.h"
#include "source/adk/steamboat/private/display_simulated.h"
#include "source/adk/steamboat/sb_display.h"

#include <limits.h>

static GLFWmonitor * get_glfw_monitor_by_index(const int display_index) {
    if (display_index < 0)
        return NULL;
    int count;
    GLFWmonitor ** monitors = glfwGetMonitors(&count);
    ASSERT_MSG(count > 0, "got 0 display devices, has glfw_init() been called?");
    if (display_index >= count) {
        return NULL;
    }
    return monitors[display_index];
}

static simulated_display_modes_t statics;

static void load_file_display_modes(GLFWmonitor * const monitor) {
    const GLFWvidmode * const current_mode = glfwGetVideoMode(monitor);
    const sb_display_mode_t max_screen_size = {.width = current_mode->width, .height = current_mode->height, .hz = current_mode->refreshRate};

    load_simulated_display_modes(&statics, max_screen_size);
}

bool sb_enumerate_display_modes(const int32_t display_index, const int32_t display_mode_index, sb_enumerate_display_modes_result_t * const out_results) {
    ZEROMEM(out_results);

    GLFWmonitor * const monitor = get_glfw_monitor_by_index(display_index);
    if (!monitor) {
        return false;
    }

    if (statics.display_mode_count == 0) {
        load_file_display_modes(monitor);
    }

    if ((display_mode_index < 0) || (display_mode_index >= statics.display_mode_count)) {
        return false;
    }

    const GLFWvidmode * const current_mode = glfwGetVideoMode(monitor);

    const sb_display_mode_t selected_mode = statics.display_modes[display_mode_index];

    *out_results = (sb_enumerate_display_modes_result_t){
        .display_mode = selected_mode,
        .status = ((display_index == 0) ? sb_display_modes_primary_display : 0) | (((current_mode->height == selected_mode.height) && (current_mode->width == selected_mode.width)) ? sb_display_modes_current_display_mode : 0)};

    return true;
}