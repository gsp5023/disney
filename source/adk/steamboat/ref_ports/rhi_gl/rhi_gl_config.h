/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
rhi_gl_config.h

OpenGL rendering device configuration
*/

#if defined(_RPI) || defined(_STB_NATIVE)
#define GL_ES
#define GL_ES2
#define GL_DEBUG_EXTENSIONS
#define GL_FRAMEBUFFER_BLIT_PROGRAM
#if !defined(_NDEBUG) && defined(_ENABLE_GL_CHECK_ERRORS)
#define GL_CHECK_ERRORS
#endif
#elif defined(_WIN32) || defined(__linux__)
#define GL_VAOS
#define GL_DSA
#define GL_CORE
#define GL_DEBUG_CONTEXT
#if !defined(_NDEBUG) && defined(_ENABLE_GL_CHECK_ERRORS)
#define GL_CHECK_ERRORS
#endif
#elif defined(__APPLE__)
#define GL_VAOS
#define GL_CORE
#define GL_DEBUG_CONTEXT
#endif
