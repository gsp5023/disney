/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#ifdef GLSL_VERSION_450
#define LAYOUT(binding_declaration) layout(binding_declaration)
#define UNIFORM_STRUCT_DEF(struct_name, def) struct_name{def};
#ifdef GLSL_FRAG
out vec4 frag_color;
#define gl_FragColor frag_color
#endif
#else // glsl #version 100 support
#define LAYOUT(binding_declaration)
#define UNIFORM_STRUCT_DEF(struct_name, def) def
#ifndef GLSL_FRAG
#define in attribute
#define out varying
#else
#define in varying
#endif
#endif