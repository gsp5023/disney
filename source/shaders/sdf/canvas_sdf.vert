/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas.vert

cg(canvas) vertex shader
*/

#ifdef GL_ES
precision highp float;
#endif

LAYOUT(location = POSITION) in vec4 in_pos0;

LAYOUT(binding = U_VIEWPORT_BINDING) uniform UNIFORM_STRUCT_DEF(viewport, vec2 u_viewport;)

in vec4 in_tc0;
out vec4 v_var;
out vec4 v_pos0;

void main() {
    v_var = in_tc0;
    v_pos0 = in_pos0;
    
    gl_Position = vec4(2.0 * in_pos0.x / u_viewport.x - 1.0, 1.0 - 2.0 * in_pos0.y / u_viewport.y, 0, 1);
}