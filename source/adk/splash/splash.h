/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
splash.h

Simple rendering of a single cached splash-screen image
*/

#include "source/adk/bundle/bundle.h"
#include "source/adk/canvas/cg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct splash_screen_contents_t {
    const char * const app_name;
    const char * const image_path;
    cg_image_t * image;
    const char * const fallback_error_message;
} splash_screen_contents_t;

void splash_main(splash_screen_contents_t splash_screen);

#ifdef __cplusplus
}
#endif
