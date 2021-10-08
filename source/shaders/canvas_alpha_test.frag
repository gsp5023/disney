/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_alpha_test.frag

cg(canvas) fragment shader for drawing to canvas with a set alpha test threshold
*/

#ifdef GL_ES
precision lowp float;
#endif

LAYOUT(binding = U_TEX0_BINDING) uniform sampler2D u_tex0;
LAYOUT(binding = U_FILL_BINDING) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)
LAYOUT(binding = U_THRESHOLD_BINDING) uniform UNIFORM_STRUCT_DEF(threshold, float u_threshold;)

in vec4 v_var;

void main() {
    vec4 col = texture2D(u_tex0, v_var.xy);

    float alpha = col.a * v_var.a;
    if (alpha < u_threshold) {
        discard;
    }
    gl_FragColor = vec4((col * u_fill).rgb, alpha);
}