/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
glbind.h

Generate GL function bindings inside a struct
*/
#pragma once

#ifndef GLBIND_TYPENAME
#error "#define GLBIND_TYPENAME to whatever struct name you want generated"
#endif

#include "glapi.h"
#include "source/adk/log/log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_GLBIND FOURCC('G', 'L', 'B', 'D')

typedef struct GLBIND_TYPENAME {
    int version_minor, version_major;

#define LOAD(_fn, _ext) \
    PFN##_ext##PROC _fn;
#define END
#define OPTIONAL(_ext) \
    bool _ext;
#define OPTIONAL_WGL(_ext) \
    bool _ext;
#define OPTIONAL_EGL(_ext) \
    bool _ext;
#define CHECK(_ext) \
    bool _ext;
#define CHECK_WGL(_ext) \
    bool _ext;
#define CHECK_EGL(_ext) \
    bool _ext;
#define REQUIRE(_ext) \
    bool _ext;
#define REQUIRE_WGL(_ext) \
    bool _ext;
#define REQUIRE_EGL(_ext) \
    bool _ext;
#define IF(_ext)
#define ENDIF

#include "glft.h"

#undef LOAD
#undef END
#undef OPTIONAL
#undef OPTIONAL_WGL
#undef OPTIONAL_EGL
#undef CHECK
#undef CHECK_WGL
#undef CHECK_EGL
#undef REQUIRE
#undef REQUIRE_WGL
#undef REQUIRE_EGL
#undef IF
#undef ENDIF

} GLBIND_TYPENAME;

#ifdef GL_CHECK_ERRORS
#define CHECK_GL_ERRORS() check_gl_errors(MALLOC_TAG)
#else
#define CHECK_GL_ERRORS() ((void)0)
#endif

static bool TOKENPASTE(load_, GLBIND_TYPENAME)(GLBIND_TYPENAME * table, void * (*load_proc_address)(const char * name), const char * platform_extensions) {
#if defined(GL_CHECK_ERRORS) || defined(GL_DEBUG_CONTEXT)
    LOG_DEBUG(TAG_GLBIND, "\n\n===PERF===\n GL_CHECK_ERRORS or GL_DEBUG_CONTEXT enabled -- severe performance degradation possible while this is on!\n==========\n\n");
#endif
    static char * prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL};

    ZEROMEM(table);

    CHECK_GL_ERRORS();

    // this can fail because the display was not properly initialized and thus the display creation functions would not init a proper GL context.
    VERIFY_MSG(glGetString(GL_VENDOR) != NULL, "OpenGL or OpenGL_ES context not properly init\n");

    LOG_DEBUG(TAG_GLBIND, "GL_VENDOR: %s", glGetString(GL_VENDOR));
    CHECK_GL_ERRORS();
    LOG_DEBUG(TAG_GLBIND, "GL_VERSION: %s", glGetString(GL_VERSION));
    CHECK_GL_ERRORS();

#ifdef GL_CORE
#define GL_CHECK_EXT(_ext) gl_is_ext_supported(_ext, extensions, num_extensions, platform_extensions)
    char ** extensions;
    int num_extensions = -1;
    const PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)load_proc_address("glGetStringi");
    VERIFY(glGetStringi);
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    CHECK_GL_ERRORS();
#ifdef GL_DEBUG_EXTENSIONS
    LOG_DEBUG(TAG_GLBIND, "GL_NUM_EXTENSIONS: %i", num_extensions);
#endif
    extensions = (char **)ALLOCA(sizeof(char *) * num_extensions);
    {
        int i;
        char * str;
        for (i = 0; i < num_extensions; ++i) {
            str = (char *)glGetStringi(GL_EXTENSIONS, i);
            extensions[i] = str;
#ifdef GL_DEBUG_EXTENSIONS
            LOG_DEBUG(TAG_GLBIND, str);
#endif
        }
    }
#else // GL_ES
    const char * const extensions = (const char *)glGetString(GL_EXTENSIONS);
    CHECK_GL_ERRORS();
#define GL_CHECK_EXT(_ext) (strstr(extensions, _ext) || (platform_extensions && strstr(platform_extensions, _ext)))
#ifdef GL_DEBUG_EXTENSIONS
    LOG_DEBUG(TAG_GLBIND, "GL_EXTENSIONS: %s", extensions);
#endif
#endif

    if (platform_extensions) {
        LOG_DEBUG(TAG_GLBIND, platform_extensions);
    }

    const char * version = (const char *)glGetString(GL_VERSION);

    /* skip ES version crap. */
    {
        int i;
        for (i = 0; prefixes[i]; ++i) {
            size_t len = strlen(prefixes[i]);
            if (strncmp(version, prefixes[i], len) == 0) {
                version += len;
                break;
            }
        }
    }

    bool success = true;

#ifdef _MSC_VER
    sscanf_s(version, "%d.%d", &table->version_major, &table->version_minor);
#else
    sscanf(version, "%d.%d", &table->version_major, &table->version_minor);
#endif
#ifdef GL_CORE
    if ((table->version_major < 3) || ((table->version_major == 3) && (table->version_minor < 2))) {
        LOG_ERROR(TAG_GLBIND, "opengl version 3.2 required!");
        return false;
    }
#else // GL_ES
    if (table->version_major < 2) {
        LOG_ERROR(TAG_GLBIND, "opengl ES 2 required!");
        return false;
    }
#endif

#define LOAD(_fn, _ext)                                                   \
    table->_fn = (PFN##_ext##PROC)load_proc_address(#_fn);                \
    if (table->_fn == NULL) {                                             \
        LOG_ERROR(TAG_GLBIND, "failed to load opengl entry point " #_fn); \
        success = false;                                                  \
    }
#define END }

#define OPTIONAL(_ext)                       \
    table->_ext = GL_CHECK_EXT("GL_" #_ext); \
    if (table->_ext) {
#define OPTIONAL_WGL(_ext)                    \
    table->_ext = GL_CHECK_EXT("WGL_" #_ext); \
    if (table->_ext) {
#define OPTIONAL_EGL(_ext)                    \
    table->_ext = GL_CHECK_EXT("EGL_" #_ext); \
    if (table->_ext) {
#define CHECK(_ext) table->_ext = GL_CHECK_EXT("GL_" #_ext);
#define CHECK_WGL(_ext) table->_ext = GL_CHECK_EXT("WGL_" #_ext);
#define CHECK_EGL(_ext) table->_ext = GL_CHECK_EXT("EGL_" #_ext);
#define REQUIRE(_ext)                                                                                             \
    table->_ext = GL_CHECK_EXT("GL_" #_ext);                                                                      \
    if (!table->_ext) {                                                                                           \
        LOG_ERROR(TAG_GLBIND, "required opengl extenion is not supported by your video card drivers: GL_" #_ext); \
        success = false;                                                                                          \
    }
#define REQUIRE_WGL(_ext)                                                                                          \
    table->_ext = GL_CHECK_EXT("WGL_" #_ext);                                                                      \
    if (!table->_ext) {                                                                                            \
        LOG_ERROR(TAG_GLBIND, "required opengl extenion is not supported by your video card drivers: WGL_" #_ext); \
        success = false;                                                                                           \
    }
#define REQUIRE_EGL(_ext)                                                                                          \
    table->_ext = GL_CHECK_EXT("EGL_" #_ext);                                                                      \
    if (!table->_ext) {                                                                                            \
        LOG_ERROR(TAG_GLBIND, "required opengl extenion is not supported by your video card drivers: EGL_" #_ext); \
        success = false;                                                                                           \
    }
#define IF(_ext) if (table->_ext) {
#define ENDIF }

#include "glft.h"

#undef LOAD
#undef END
#undef OPTIONAL
#undef OPTIONAL_WGL
#undef OPTIONAL_EGL
#undef CHECK
#undef CHECK_WGL
#undef CHECK_EGL
#undef REQUIRE
#undef REQUIRE_WGL
#undef REQUIRE_EGL
#undef IF
#undef ENDIF

    return success;
}

#ifdef __cplusplus
}
#endif