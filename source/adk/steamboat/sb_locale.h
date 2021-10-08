/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_locale.h

steamboat locale support
*/

#pragma once

#include "source/tools/adk-bindgen/lib/ffi_gen_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /// Maximum size of individual locale property
    sb_locale_str_size = 64
};

/// Locale properties (for multi-language support)
FFI_EXPORT
FFI_NAME(_adk_locale_t)
typedef struct sb_locale_t {
    // The language code supported by the sub-system (e.g., ISO 639-1, ISO 639-2, ...)
    char language[sb_locale_str_size];
    // The country code supported by the sub-system (e.g., ISO 3166-1)
    char region[sb_locale_str_size];
} sb_locale_t;

/// Queries and returns the current locale of the platform
EXT_EXPORT FFI_EXPORT FFI_NO_RUST_THUNK FFI_NAME(adk_get_locale) sb_locale_t sb_get_locale();

#ifdef __cplusplus
}
#endif
