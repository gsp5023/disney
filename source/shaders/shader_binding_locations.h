/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
shader_binding_locations.h

macro definitions for sharing between glsl/c so enum values can be shared between core and shaders for uniform/texture binding locations.
*/

#ifndef _SHADER_BINDING_LOCATIONS_HEADER_1234124412
#define _SHADER_BINDING_LOCATIONS_HEADER_1234124412

#define U_MVP_BINDING 0
#define U_VIEWPORT_BINDING 1
#define U_TEX0_BINDING 2
#define U_TEX1_BINDING 3
#define U_FILL_BINDING 4
#define U_THRESHOLD_BINDING 5
#define U_RECT_ROUNDNESS_BINDING 6
#define U_RECT_BINDING 7
#define U_FADE_BINDING 8
#define U_STROKE_COLOR_BINDING 9
#define U_STROKE_SIZE_BINDING 10
#define U_LTEXSIZE_BINDING 11
#define U_CTEXSIZE_BINDING 12
#define U_FRAMESIZE_BINDING 13
#endif