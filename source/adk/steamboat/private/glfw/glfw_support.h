/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
glfw_support.h

Support for GLFW
*/

#ifndef _GLFW
#error "Do not include this file for a non-glfw target"
#endif

#include <GLFW/glfw3.h>
#undef APIENTRY

#ifdef __cplusplus
extern "C" {
#endif

bool init_glfw();
void reset_glfw_opengl_window_hints();
GLFWwindow * create_glfw_window(int width, int height, const char * title, GLFWmonitor * monitor);
bool glfw_restart_requested(void);
void glfw_restart_acknowledge(void);

#ifdef __cplusplus
}
#endif
