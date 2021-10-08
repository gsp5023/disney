/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifndef _WASM3
#error "_WASM3 must be defined to include this file"
#endif // _WASM3

#include "source/adk/interpreter/interp_common.h"
#include "source/adk/wasm3/private/wasm3.h"

#undef _WASM3
EXT_EXPORT EXT_LINK_IF_DEF(_WASM3) void wasm3_export_native_function(IM3Module module, const char * const name, const wasm_sig_mask signature, const M3RawCall func_ptr);
#define _WASM3
