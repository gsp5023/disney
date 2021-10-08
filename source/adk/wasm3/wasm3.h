/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifndef _WASM3
#error "_WASM3 must be defined to include this file"
#endif // _WASM3

#include "source/adk/interpreter/interp_api.h"

wasm_interpreter_t * get_wasm3_interpreter(void);
void wasm3_register_linker(const wasm_linker_t linker);
