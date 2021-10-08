/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
glapi.c

OpenGL API support
*/

#include _PCH

#include "glapi.h"

/*
=======================================
gl_check_errors
=======================================
*/

void check_gl_errors(const char * tag) {
    GLenum e;
    bool glHadErrors = false;
    while ((e = glGetError()) != GL_NO_ERROR) {
        static const char * errStrings[7] = {
            "GL_INVALID_ENUM",
            "GL_INVALID_VALUE",
            "GL_INVALID_OPERATION",
            "GL_STACK_OVERFLOAT",
            "GL_STACK_UNDERFLOW",
            "GL_OUT_OF_MEMORY",
            "GL_INVALID_FRAMEBUFFER_OPERATION"};
        const int errOfs = e - GL_INVALID_ENUM;
        if (errOfs < ARRAY_SIZE(errStrings)) {
            LOG_ERROR(TAG_GLAPI, "check_gl_errors: %s @ [%s]", errStrings[errOfs], tag);
        } else {
            LOG_ERROR(TAG_GLAPI, "check_gl_errors: unrecognized error code @ [%s]", tag);
        }
        glHadErrors = true;
    }
    VERIFY(!glHadErrors);
}