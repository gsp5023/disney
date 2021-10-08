/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
dispmanx.h

dispmanx windowing support for rpi
*/

#include "source/adk/runtime/runtime.h"

#include <EGL/egl.h>
#include <bcm_host.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dispmanx_window_t {
    DISPMANX_DISPLAY_HANDLE_T display;
    EGL_DISPMANX_WINDOW_T window;
    int w, h;
} dispmanx_window_t;

bool dispmanx_init();
void dispmanx_shutdown();
void dispmanx_dispatch_events();

dispmanx_window_t * dispmanx_create_window(const int w, const int h, const char * const tag);
void dispmanx_get_window_size(dispmanx_window_t * const window, int * const w, int * const h);
void dispmanx_close_window(dispmanx_window_t * const window, const char * const tag);

#ifdef __cplusplus
}
#endif