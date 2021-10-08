/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
canvas_clear.vert

canvas_clear vertex shader
*/

in vec4 in_pos;

void main() {
    gl_Position = in_pos;
}
