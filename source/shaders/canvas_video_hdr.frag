/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_video_hdr.frag

cg(canvas) hdr video fragment shader
*/

#ifdef GL_ES
precision lowp float;
#endif

LAYOUT(binding = U_TEX0_BINDING) uniform sampler2D u_tex0;
LAYOUT(binding = U_TEX1_BINDING) uniform sampler2D u_tex1;

LAYOUT(binding = U_FILL_BINDING) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)
LAYOUT(binding = U_LTEXSIZE_BINDING) uniform UNIFORM_STRUCT_DEF(ltex_size, ivec2 u_ltex_size;)
LAYOUT(binding = U_CTEXSIZE_BINDING) uniform UNIFORM_STRUCT_DEF(ctex_size, ivec2 u_ctex_size;)
LAYOUT(binding = U_FRAMESIZE_BINDING) uniform UNIFORM_STRUCT_DEF(framesize, ivec2 u_framesize;)

in vec4 v_var;

#define luma_sampler u_tex1
#define chroma_sampler u_tex0

void main() {
    
    ivec2 DTid; 
    ivec2 ptex_dim;
    ivec2 ptex_ind;
    vec2 ptex_pos;
    vec3 ycbcr;

    ptex_dim = u_ltex_size; // tex size luma
    DTid.x = int(floor(v_var.x * float(u_framesize.x)));  //full res int pixel index
    DTid.y = int(floor(v_var.y * float(u_framesize.y)));
    
    ptex_ind.x = DTid.x / 3;  //tex index: convert x frame coord to triplet index
    ptex_pos.x = (float(ptex_ind.x) + 0.5) / float(ptex_dim.x);  //convert tex triplet index into fraction coord
    ptex_pos.y = v_var.y;  // just use orig y

    ycbcr.x = texture2D(luma_sampler, ptex_pos)[int(mod(floor(float(DTid.x)),3.0))]; // get luma

    // NOTE: bitshift right not implemented. Using N=N/(i^2) for N >> 1. bitwise AND not implemented. Using mod(N, 2) for N & 1.

    //chroma is treated as 2 half-width rows combined and quarter height. to satisfy alignment, esp for 720 and 1080p
    DTid.x /= 2;
    DTid.y /= 2;
    ptex_dim = u_ctex_size;  //tex size chroma

    ptex_ind.x = DTid.x / 3;   //x triplet index
    ptex_ind.y = DTid.y / 2;  //tex index: half height index
    ptex_pos =  (vec2(ptex_ind) + 0.5) / vec2(ptex_dim);   //convert to fraction coord
    ptex_pos.x += 0.5 * (mod(float(DTid.y), 2.0));  //shift over to odd row
    
    int c_ind = int(mod(floor(float(DTid.x)), 3.0));
    ycbcr.y = texture2D(chroma_sampler, ptex_pos)[c_ind]; //get Cb

    ptex_pos.y += 0.5;  //shift down to Cr half
    ycbcr.z = texture2D(chroma_sampler, ptex_pos)[c_ind]; //get Cr

    ycbcr -= vec3(0.0625, 0.5, 0.5); // yOffset = 0.0625

    // matrix based on video_full_range = false
    vec3 rgb = vec3(dot(vec3(1.1644, 0.0, 1.6787), ycbcr), // R
                dot(vec3(1.1644, -0.1873, -0.6504), ycbcr), // G
                dot(vec3(1.1644, 2.1418, 0.0), ycbcr)); // B
    gl_FragColor = vec4(rgb, 1.0); // scaling_factor == 1.0
}