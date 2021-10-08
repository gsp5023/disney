// warning this file is generated by a tool do not modify


#define SHADER_DECLARATION(_x) #_x

static const char canvas_clear_vert_bytes[] = SHADER_DECLARATION(
\n#version 100\n
\n#define GLSL_VERSION_100\n
\n#define GLSL_VERT\n
/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
shader_binding_locations.h

macro definitions for sharing between glsl/c so enum values can be shared between core and shaders for uniform/texture binding locations.
*/

\n#ifndef _SHADER_BINDING_LOCATIONS_HEADER_1234124412\n
\n#define _SHADER_BINDING_LOCATIONS_HEADER_1234124412\n

\n#define U_MVP_BINDING 0\n
\n#define U_VIEWPORT_BINDING 1\n
\n#define U_TEX0_BINDING 2\n
\n#define U_TEX1_BINDING 3\n
\n#define U_FILL_BINDING 4\n
\n#define U_THRESHOLD_BINDING 5\n

\n#endif\n
/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

\n#ifdef GLSL_VERSION_450\n
\n#define LAYOUT(binding_declaration) layout(binding_declaration)\n
\n#define UNIFORM_STRUCT_DEF(struct_name, def) struct_name{def};\n
\n#ifdef GLSL_FRAG\n
out vec4 frag_color;
\n#define gl_FragColor frag_color\n
\n#endif\n
\n#else // glsl #version 100 support\n
\n#define LAYOUT(binding_declaration)\n
\n#define UNIFORM_STRUCT_DEF(struct_name, def) def\n
\n#ifndef GLSL_FRAG\n
\n#define in attribute\n
\n#define out varying\n
\n#else\n
\n#define in varying\n
\n#endif\n
\n#endif\n
/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_clear.vert

canvas_clear vertex shader
*/

in vec4 in_pos;

void main() {
    gl_Position = in_pos;
}

);
static const const_mem_region_t canvas_clear_vert_program = {{.ptr = canvas_clear_vert_bytes}, .size = ARRAY_SIZE(canvas_clear_vert_bytes)};

#undef SHADER_DECLARATION