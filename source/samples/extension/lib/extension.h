/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/interpreter/interp_all.h"
#include "source/adk/runtime/runtime.h"

FFI_EXPORT int32_t extension_squared(int32_t value);
FFI_EXPORT void extension_println(FFI_PTR_WASM const char * msg);
