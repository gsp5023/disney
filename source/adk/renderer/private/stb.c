/* ===========================================================================
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 * ==========================================================================*/

#include _PCH

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4224)
#pragma warning(disable : 4244)
#pragma warning(disable : 4456)
#pragma warning(disable : 4702)
#endif

#if defined(__GNUC__) && (__GNUC__ >= 7)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#define STB_DS_IMPLEMENTATION
#include "extern/stb/stb/stb_ds.h"

#if defined(__GNUC__) && (__GNUC__ >= 7)
#pragma GCC diagnostic pop
#endif

#ifdef _WIN32
#pragma warning(pop)
#endif
