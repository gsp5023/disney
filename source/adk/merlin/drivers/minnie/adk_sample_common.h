/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 adk_sample_common.h

 ADK sample application helpers
 */

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

void adk_app_init_main_display(const char * const app_name);
bool sample_get_runtime_duration(const uint32_t argc, const char * const * const argv, milliseconds_t * const out_runtime);

#ifdef __cplusplus
}
#endif