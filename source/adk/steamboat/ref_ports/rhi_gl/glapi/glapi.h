/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
glapi.h

Import system opengl headers which we provide. We also provide platform
hackery to avoid including any system headers. Also creates the gl function
import tables which we use to call gl functions.
*/

#include "source/adk/log/log.h"
#include "source/adk/runtime/runtime.h"
#include "source/adk/steamboat/ref_ports/rhi_gl/rhi_gl_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_GLAPI FOURCC('G', 'L', 'A', 'P')

/* Shutup MSVC */
#ifdef _MSC_VER
/* MSVC headers don't compile without warnings with strict ansi */
#pragma warning(push)
#pragma warning(disable : 4001) /* nonstandard extension 'single line comment' was used */
#endif

#ifdef GL_CORE
#include "gl/gl.h"
#include "gl/glext.h"
#else // GL_ES
#define GL_GLES_PROTOTYPES 1
#ifdef _MTV
typedef signed short int khronos_int16_t;
typedef int64_t khronos_int64_t;
typedef unsigned short int khronos_uint16_t;
typedef uint64_t khronos_uint64_t;
#endif //_MTV
#include "gles/gl2.h"
#include "gles/gl2ext.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_DEBUG_SOURCE_API GL_DEBUG_SOURCE_API_KHR
#define GL_DEBUG_TYPE_ERROR GL_DEBUG_TYPE_ERROR_KHR
#define GL_DEBUG_SEVERITY_NOTIFICATION GL_DEBUG_SEVERITY_NOTIFICATION_KHR
#define GL_DEBUG_SEVERITY_HIGH GL_DEBUG_SEVERITY_HIGH_KHR
#define GL_DEBUG_TYPE_OTHER GL_DEBUG_TYPE_OTHER_KHR
#define GL_DEBUG_TYPE_PERFORMANCE GL_DEBUG_TYPE_PERFORMANCE_KHR
#define GL_DEBUG_OUTPUT_SYNCHRONOUS GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR
#endif

#ifdef _WIN32
#define WINAPI APIENTRY
#define DECLARE_HANDLE(_x) typedef void * _x
DECLARE_HANDLE(HANDLE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;
typedef unsigned int DWORD;
typedef char CHAR;
typedef long LONG;
typedef void VOID;
typedef void * LPVOID;
typedef int BOOL;
typedef __int64 INT64;
typedef int INT32;
typedef unsigned short USHORT;
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;
#include "gl/wglext.h"
#undef WINAPI
#undef DECLARE_HANDLE
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef GL_DEBUG_CONTEXT
// GLES expects/demands that the function have const, but CORE expects no consts.
#ifdef GL_CORE
static void APIENTRY gl_logger_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, char * message, void * user)
#else // GL_ES
static void gl_logger_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char * message, const void * user)
#endif
{
    static char * str_source[6] = {
        "API",
        "WINDOW_SYSTEM",
        "SHADER_COMPILER",
        "THIRD_PARTY",
        "APPLICATION",
        "OTHER"};
    static char * str_type[6] = {
        "ERROR",
        "DEPRECATED",
        "UNDEFINED",
        "PORTABILITY",
        "PERFORMANCE",
        "OTHER"};
    static char * str_severity[3] = {
        "HIGH",
        "MEDIUM",
        "LOW"};

    ASSERT((source - GL_DEBUG_SOURCE_API) < 6);
    ASSERT((type - GL_DEBUG_TYPE_ERROR) < 6);
    ASSERT((severity - GL_DEBUG_SEVERITY_HIGH) < 6);

    debug_write_line("gldbg [%s][%s][%s] %s", str_source[source - GL_DEBUG_SOURCE_API], str_type[type - GL_DEBUG_TYPE_ERROR], (severity == GL_DEBUG_SEVERITY_NOTIFICATION) ? "INFO" : str_severity[severity - GL_DEBUG_SEVERITY_HIGH], message);
    if ((type != GL_DEBUG_TYPE_OTHER) && (type != GL_DEBUG_TYPE_PERFORMANCE)) {
        DBG_BREAK();
    }
}
#endif

#ifdef GL_CORE
static bool gl_is_ext_supported(const char * const ext, char ** const extension_table, const int num_extensions, const char * const platform_extensions) {
    for (GLint i = 0; i < num_extensions; ++i) {
        if (!strcmp(extension_table[i], ext)) {
            return true;
        }
    }
    return platform_extensions && strstr(platform_extensions, ext);
}
#endif

void check_gl_errors(const char * tag);

#define GLVERSION 320
#define GLBIND_TYPENAME glf_t
#include "glbind.h"
#undef GLBIND_TYPENAME

#ifdef __cplusplus
}
#endif