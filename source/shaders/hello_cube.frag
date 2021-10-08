/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
hello_cube.frag

hello_cube fragment shader
*/

#ifdef GL_ES
precision lowp float;
#endif

in vec4 out_color;

void main() {
    gl_FragColor = out_color; 
}