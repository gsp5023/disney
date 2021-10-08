/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_video.frag

cg(canvas) video fragment shader
*/

#ifdef GL_ES
precision lowp float;
#endif

LAYOUT(binding = U_TEX0_BINDING) uniform sampler2D u_tex0;
LAYOUT(binding = U_TEX1_BINDING) uniform sampler2D u_tex1;

LAYOUT(binding = U_FILL_BINDING) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)

in vec4 v_var;

void main() {
    float alpha = v_var.a;
    vec2 chroma = texture2D(u_tex0, v_var.xy).rg;
    float luminance = texture2D(u_tex1, v_var.xy).r;
    vec3 YCbCr = vec3(luminance, chroma);

    YCbCr *= 1.0;
    YCbCr -= vec3(0.0625, 0.5, 0.5);
    
    vec3 rgb = vec3(dot(vec3(1.1644, 0.0, 1.7927), YCbCr), // R
                dot(vec3(1.1644, -0.2133, -0.5329), YCbCr), // G
                dot(vec3(1.1644, 2.1124, 0.0), YCbCr)); // B
                        
    gl_FragColor = vec4(rgb * u_fill.rgb, 1.0);
}