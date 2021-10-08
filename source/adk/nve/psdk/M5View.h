/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
M5View.h
*/

#ifndef M5VIEW_H
#define M5VIEW_H

// #include "adk/canvas/cg.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cg_context_t;

void * nve_m5_view_create(cg_context_t * const ctx, int x, int y, int w, int h);

int nve_m5_view_get_width(void * view);

int nve_m5_view_get_height(void * view);

int nve_m5_view_get_x(void * view);

int nve_m5_view_get_y(void * view);

void nve_m5_view_set_size(void * view, int w, int h);

void nve_m5_view_set_position(void * view, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
