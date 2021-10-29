/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Loaded library handle
typedef struct pb_module_handle_s * pb_module_handle_t;

/// Load a dynamic library
///
/// * `filename`: path to a dynamic library
///
/// Returns a `handle` to the loaded library on success, or NULL on failure
pb_module_handle_t pb_load_module(const char * const filename);

/// Unload a dynamic library
///
/// * `handle`: handle to a loaded library
///
/// Returns `true` on success and `false` on failure
bool pb_unload_module(const pb_module_handle_t handle);

/// Obtain the address of a symbol from a loaded dynamic library
///
/// * `handle`: handle to a loaded library
/// * `sym_name`: The name of the symbol to bind
///
/// Returns the address of the requested symbol
const void * pb_bind_symbol(const pb_module_handle_t handle, const char * const sym_name);

#ifdef __cplusplus
}
#endif
