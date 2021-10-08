/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas.frag

cg(canvas) fragment shader
*/

#ifdef GL_ES
precision lowp float;
#endif

LAYOUT(binding = U_TEX0_BINDING) uniform sampler2D u_tex0;
LAYOUT(binding = U_FILL_BINDING) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)

in vec4 v_var;

void main() {
    float alpha = v_var.a;
    vec4 col = texture2D(u_tex0, v_var.xy);
    gl_FragColor = vec4(u_fill.rgb, col.r * alpha);
}