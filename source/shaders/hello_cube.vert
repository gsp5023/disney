/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
hello_cube.vert

hello_cube vertex shader
*/

#ifdef GL_ES
precision highp float;
#endif

LAYOUT(location = POSITION) in vec4 in_pos0;
LAYOUT(binding = U_MVP_BINDING) uniform UNIFORM_STRUCT_DEF(mvp, mat4 u_mvp;)

in vec4 in_col0;
out vec4 out_color;

void main() {
    gl_Position = u_mvp * in_pos0;
    out_color = in_col0;
}