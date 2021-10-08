/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_clear.frag

cg(canvas_clear) fragment shader
*/

#ifdef GL_ES
precision lowp float;
#endif

LAYOUT(binding = 0) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)

void main() {
    gl_FragColor = u_fill;
}
