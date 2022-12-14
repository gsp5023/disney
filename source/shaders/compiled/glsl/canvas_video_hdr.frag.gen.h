// warning this file is generated by a tool do not modify

/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

static const char canvas_video_hdr_frag_bytes[] = 
"#version 100\n"
"#define GLSL_VERSION_100\n"
"#define GLSL_FRAG\n"
"\n"
"/*\n"
"shader_binding_locations.h\n"
"\n"
"macro definitions for sharing between glsl/c so enum values can be shared between core and shaders for uniform/texture binding locations.\n"
"*/\n"
"\n"
"#ifndef _SHADER_BINDING_LOCATIONS_HEADER_1234124412\n"
"#define _SHADER_BINDING_LOCATIONS_HEADER_1234124412\n"
"\n"
"#define U_MVP_BINDING 0\n"
"#define U_VIEWPORT_BINDING 1\n"
"#define U_TEX0_BINDING 2\n"
"#define U_TEX1_BINDING 3\n"
"#define U_FILL_BINDING 4\n"
"#define U_THRESHOLD_BINDING 5\n"
"#define U_RECT_ROUNDNESS_BINDING 6\n"
"#define U_RECT_BINDING 7\n"
"#define U_FADE_BINDING 8\n"
"#define U_STROKE_COLOR_BINDING 9\n"
"#define U_STROKE_SIZE_BINDING 10\n"
"#define U_LTEXSIZE_BINDING 11\n"
"#define U_CTEXSIZE_BINDING 12\n"
"#define U_FRAMESIZE_BINDING 13\n"
"#endif\n"
"\n"
"#ifdef GLSL_VERSION_450\n"
"#define LAYOUT(binding_declaration) layout(binding_declaration)\n"
"#define UNIFORM_STRUCT_DEF(struct_name, def) struct_name{def};\n"
"#ifdef GLSL_FRAG\n"
"out vec4 frag_color;\n"
"#define gl_FragColor frag_color\n"
"#endif\n"
"#else // glsl #version 100 support\n"
"#define LAYOUT(binding_declaration)\n"
"#define UNIFORM_STRUCT_DEF(struct_name, def) def\n"
"#ifndef GLSL_FRAG\n"
"#define in attribute\n"
"#define out varying\n"
"#else\n"
"#define in varying\n"
"#endif\n"
"#endif\n"
"\n"
"/*\n"
"canvas_video_hdr.frag\n"
"\n"
"cg(canvas) hdr video fragment shader\n"
"*/\n"
"\n"
"#ifdef GL_ES\n"
"precision lowp float;\n"
"#endif\n"
"\n"
"LAYOUT(binding = U_TEX0_BINDING) uniform sampler2D u_tex0;\n"
"LAYOUT(binding = U_TEX1_BINDING) uniform sampler2D u_tex1;\n"
"\n"
"LAYOUT(binding = U_FILL_BINDING) uniform UNIFORM_STRUCT_DEF(fill, vec4 u_fill;)\n"
"LAYOUT(binding = U_LTEXSIZE_BINDING) uniform UNIFORM_STRUCT_DEF(ltex_size, ivec2 u_ltex_size;)\n"
"LAYOUT(binding = U_CTEXSIZE_BINDING) uniform UNIFORM_STRUCT_DEF(ctex_size, ivec2 u_ctex_size;)\n"
"LAYOUT(binding = U_FRAMESIZE_BINDING) uniform UNIFORM_STRUCT_DEF(framesize, ivec2 u_framesize;)\n"
"\n"
"in vec4 v_var;\n"
"\n"
"#define luma_sampler u_tex1\n"
"#define chroma_sampler u_tex0\n"
"\n"
"void main() {\n"
"    \n"
"    ivec2 DTid; \n"
"    ivec2 ptex_dim;\n"
"    ivec2 ptex_ind;\n"
"    vec2 ptex_pos;\n"
"    vec3 ycbcr;\n"
"\n"
"    ptex_dim = u_ltex_size; // tex size luma\n"
"    DTid.x = int(floor(v_var.x * float(u_framesize.x)));  //full res int pixel index\n"
"    DTid.y = int(floor(v_var.y * float(u_framesize.y)));\n"
"    \n"
"    ptex_ind.x = DTid.x / 3;  //tex index: convert x frame coord to triplet index\n"
"    ptex_pos.x = (float(ptex_ind.x) + 0.5) / float(ptex_dim.x);  //convert tex triplet index into fraction coord\n"
"    ptex_pos.y = v_var.y;  // just use orig y\n"
"\n"
"    ycbcr.x = texture2D(luma_sampler, ptex_pos)[int(mod(floor(float(DTid.x)),3.0))]; // get luma\n"
"\n"
"    // NOTE: bitshift right not implemented. Using N=N/(i^2) for N >> 1. bitwise AND not implemented. Using mod(N, 2) for N & 1.\n"
"\n"
"    //chroma is treated as 2 half-width rows combined and quarter height. to satisfy alignment, esp for 720 and 1080p\n"
"    DTid.x /= 2;\n"
"    DTid.y /= 2;\n"
"    ptex_dim = u_ctex_size;  //tex size chroma\n"
"\n"
"    ptex_ind.x = DTid.x / 3;   //x triplet index\n"
"    ptex_ind.y = DTid.y / 2;  //tex index: half height index\n"
"    ptex_pos =  (vec2(ptex_ind) + 0.5) / vec2(ptex_dim);   //convert to fraction coord\n"
"    ptex_pos.x += 0.5 * (mod(float(DTid.y), 2.0));  //shift over to odd row\n"
"    \n"
"    int c_ind = int(mod(floor(float(DTid.x)), 3.0));\n"
"    ycbcr.y = texture2D(chroma_sampler, ptex_pos)[c_ind]; //get Cb\n"
"\n"
"    ptex_pos.y += 0.5;  //shift down to Cr half\n"
"    ycbcr.z = texture2D(chroma_sampler, ptex_pos)[c_ind]; //get Cr\n"
"\n"
"    ycbcr -= vec3(0.0625, 0.5, 0.5); // yOffset = 0.0625\n"
"\n"
"    // matrix based on video_full_range = false\n"
"    vec3 rgb = vec3(dot(vec3(1.1644, 0.0, 1.6787), ycbcr), // R\n"
"                dot(vec3(1.1644, -0.1873, -0.6504), ycbcr), // G\n"
"                dot(vec3(1.1644, 2.1418, 0.0), ycbcr)); // B\n"
"    gl_FragColor = vec4(rgb, 1.0); // scaling_factor == 1.0\n"
"}\n"
;
static const const_mem_region_t canvas_video_hdr_frag_program = {{.ptr = canvas_video_hdr_frag_bytes}, .size = ARRAY_SIZE(canvas_video_hdr_frag_bytes)};

