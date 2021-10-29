/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#ifndef GL_VERSION_450
precision highp float;
#endif

LAYOUT(binding = U_TEX0_BINDING) uniform sampler2D u_tex0;
LAYOUT(binding = U_FILL_BINDING) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)
LAYOUT(binding = U_RECT_ROUNDNESS_BINDING) uniform UNIFORM_STRUCT_DEF(rect_roundness, float u_rect_roundness;)
LAYOUT(binding = U_RECT_BINDING) uniform UNIFORM_STRUCT_DEF(rect, vec4 u_rect;)
LAYOUT(binding = U_FADE_BINDING) uniform UNIFORM_STRUCT_DEF(fade, float u_fade;)
LAYOUT(binding = U_STROKE_COLOR_BINDING) uniform UNIFORM_STRUCT_DEF(stroke_color, vec4 u_stroke_color;)
LAYOUT(binding = U_STROKE_SIZE_BINDING) uniform UNIFORM_STRUCT_DEF(stroke_size, float u_stroke_size;)

in vec4 v_var;
in vec4 v_pos0;

float sdf_rounded_box(vec2 box_origin, vec2 half_box, float radius, vec2 world_pos) {
    vec2 d = abs(world_pos - box_origin) - half_box + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

void main() {
    vec4 col = texture2D(u_tex0, v_var.xy);

    float sdf_sample = sdf_rounded_box(u_rect.xy, u_rect.zw, u_rect_roundness, v_pos0.xy);
    float stroke_sample = abs(sdf_sample) - u_stroke_size;
    float sdf_clamped = smoothstep(0.0, 1.0, -sdf_sample/u_fade);
    float stroke_clamped = clamp(-stroke_sample, 0.0, 1.0);

    float mixer = step(0.0, stroke_clamped) * stroke_clamped;
    gl_FragColor = mix(
        vec4((col * u_fill).rgb, col.a * v_var.a * sdf_clamped),
        vec4(u_stroke_color.rgb, u_stroke_color.a * v_var.a * stroke_clamped),
        mixer);
}