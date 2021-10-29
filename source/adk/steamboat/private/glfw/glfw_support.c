/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
glfw_support.c

GLFW support
*/

#include _PCH
#include "glfw_support.h"

#include "source/adk/log/log.h"
#include "source/adk/runtime/private/events.h"
#include "source/adk/steamboat/ref_ports/rhi_gl/rhi_gl_config.h"
#include "source/adk/telemetry/telemetry.h"

#define TAG_GLFW FOURCC('G', 'L', 'F', 'W')

static GLFWwindow * share_window = NULL;
static bool restart_req = false;

static adk_mod_keys_e get_modifier_keys(GLFWwindow * window) {
    adk_mod_keys_e keys = 0;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)) {
        keys |= adk_mod_shift;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL)) {
        keys |= adk_mod_control;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT)) {
        keys |= adk_mod_alt;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER)) {
        keys |= adk_mod_super;
    }

    return keys;
}

static adk_mouse_button_mask_e get_mouse_buttons(GLFWwindow * const window) {
    adk_mouse_button_mask_e mask = 0;

    for (int i = 0; i < 3; ++i) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1 + i)) {
            mask |= (1 << i);
        }
    }

    return mask;
}

static void user_close_window_callback(GLFWwindow * window) {
    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_window_event,
            .win = (adk_window_event_t){
                .window = window,
                .event_data = (adk_window_event_data_t){
                    adk_window_event_close,
                }}}});

    glfwSetWindowShouldClose(window, 0);
}

static void char_callback(GLFWwindow * const window, const uint32_t codepoint) {
    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_char_event,
            .ch = (adk_char_event_t){
                .codepoint_utf32 = codepoint}}});
}

static void key_callback(GLFWwindow * const window, const int key, const int scancode, const int action, const int mods) {
#ifndef _SHIP
    if ((key == GLFW_KEY_F2) && (action == GLFW_RELEASE)) {
        LOG_INFO(TAG_GLFW, "Simulating video restart");
        restart_req = true;
    }
#endif
    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_key_event,
            .key = (adk_key_event_t){
                .event = (action == GLFW_PRESS || action == GLFW_REPEAT) ? adk_key_event_key_down : adk_key_event_key_up,
                .key = key,
                .mod_keys = mods,
                .repeat = action == GLFW_REPEAT}}});
}

static void cursor_enter_callback(GLFWwindow * const window, const int entered) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_mouse_event,
            .mouse = (adk_mouse_event_t){
                .event_data = (adk_mouse_event_data_t){
                    .event = adk_mouse_event_motion,
                    .motion_event = entered ? adk_mouse_motion_event_enter : adk_mouse_motion_event_leave,
                },
                .window = (struct sb_window_t *)window,
                .mouse_state = (adk_mouse_state_t){.x = (int)x, .y = (int)y, .button_mask = get_mouse_buttons(window), .mod_keys = get_modifier_keys(window)},
            }}});
}

static void cursor_position_callback(GLFWwindow * const window, const double x, const double y) {
    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_mouse_event,
            .mouse = (adk_mouse_event_t){
                .event_data = (adk_mouse_event_data_t){
                    .event = adk_mouse_event_motion,
                    .motion_event = adk_mouse_motion_event_move,
                },
                .window = (struct sb_window_t *)window,
                .mouse_state = (adk_mouse_state_t){.x = (int)x, .y = (int)y, .button_mask = get_mouse_buttons(window), .mod_keys = get_modifier_keys(window)},
            }}});
}

static void mouse_button_callback(GLFWwindow * const window, const int button, const int action, const int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    adk_post_event((adk_event_t){
        .time = adk_read_millisecond_clock(),
        .event_data = {
            .type = adk_mouse_event,
            .mouse = (adk_mouse_event_t){
                .event_data = (adk_mouse_event_data_t){
                    .event = adk_mouse_event_button,
                    .button_event = (adk_mouse_button_event_t){
                        .event = action ? adk_mouse_button_event_down : adk_mouse_button_event_up,
                        .button = button}},
                .window = (struct sb_window_t *)window,
                .mouse_state = (adk_mouse_state_t){.x = (int)x, .y = (int)y, .button_mask = get_mouse_buttons(window), .mod_keys = mods},
            }}});
}

static void hook_events(GLFWwindow * window) {
    glfwSetWindowCloseCallback(window, user_close_window_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetCursorEnterCallback(window, cursor_enter_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
}

static void error_callback(const int err, const char * const message) {
    LOG_ERROR(TAG_GLFW, "%d: %s", err, message);
}

bool init_glfw() {
    GLFW_TRACE_PUSH_FN();

    glfwSetErrorCallback(error_callback);

    GLFW_TRACE_PUSH("glfwInit");
    if (!glfwInit()) {
        GLFW_TRACE_POP();
        GLFW_TRACE_POP();
        return false;
    }
    GLFW_TRACE_POP();

    reset_glfw_opengl_window_hints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFW_TRACE_PUSH("glfwCreateWindow");
    share_window = glfwCreateWindow(32, 32, "share_context_window", NULL, NULL);
    GLFW_TRACE_POP();
    if (!share_window) {
        LOG_ERROR(TAG_GLFW, "failed to create glfw share_window");
        glfwTerminate();
        GLFW_TRACE_POP();
        return false;
    }
    reset_glfw_opengl_window_hints();
    GLFW_TRACE_POP();
    return true;
}

void reset_glfw_opengl_window_hints() {
    glfwDefaultWindowHints();
#ifdef GL_CORE
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else // GL_ES
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef GL_DEBUG_CONTEXT
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
}

GLFWwindow * create_glfw_window(int width, int height, const char * title, GLFWmonitor * monitor) {
    GLFW_TRACE_PUSH_FN();

    GLFWwindow * w = glfwCreateWindow(width, height, title, monitor, share_window);
    if (w) {
        hook_events(w);
    }
    GLFW_TRACE_POP();
    return w;
}

bool glfw_restart_requested(void) {
    return restart_req;
}

void glfw_restart_acknowledge(void) {
    restart_req = false;
}
