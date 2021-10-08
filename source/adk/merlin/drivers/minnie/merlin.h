/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include "source/adk/runtime/runtime.h"

// By default demos are enabled only in non-shipping builds, can be overridden
#ifndef _MERLIN_DEMOS
#ifdef _SHIP
#define _MERLIN_DEMOS 0
#else
#define _MERLIN_DEMOS 1
#endif
#endif

#if _MERLIN_DEMOS
// Returns fully-qualified name of app asset in 'asset_path_buff', for builtin demos
const char * merlin_asset(const char * const relpath, char * const asset_path_buff, const size_t asset_path_buff_len);
#endif

// Selects display mode and inits main display for builtin demos and splash screen
void adk_app_init_main_display(const char * const app_name);