/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_display.h

steamboat display hardware details
*/

#pragma once

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Represents a handle to a native window object
typedef struct sb_window_t sb_window_t;

/// Properties of a display mode
FFI_EXPORT FFI_NAME(adk_display_mode_t) typedef struct sb_display_mode_t {
    /// Display width in pixels
    int32_t width;
    /// Display height in pixels
    int32_t height;
    /// Refresh rate in cycles per second
    int32_t hz;
} sb_display_mode_t;

/// Indicates if a display/mode combo is the primary or current display
FFI_EXPORT FFI_NAME(adk_display_modes_flags_e) FFI_ENUM_BITFLAGS FFI_ENUM_CLEAN_NAMES typedef enum sb_enumerate_display_modes_status_e {
    /// Display_index is the primary display's index.
    sb_display_modes_primary_display = 0x1,
    /// Display_mode_index is the displays current mode
    sb_display_modes_current_display_mode = 0x2,
    FORCE_ENUM_INT32(adk_display_modes_flags_e)
} adk_display_modes_flags_e;

/// Return type of sb_enumerate_display_modes indicating if the display/mode is valid, is the primary display, is the current mode.
/// If the combo is valid display_mode will be non-zero and filled with a value relating to a valid display mode for the specified display (f.e. 1920x1080x60)
FFI_EXPORT FFI_NAME(adk_enumerate_display_modes_result_t) typedef struct sb_enumerate_display_modes_result_t {
    sb_display_mode_t display_mode;
    adk_display_modes_flags_e status;
} sb_enumerate_display_modes_result_t;

/// Fill a display mode descriptor for the specified display and display mode.
///
/// * `display_index`: Index number of the display to load
/// * `display_mode_index`: Index number of the static display mode to return
/// * `out_results`: The returned results
///
/// Returns false if the display or mode doesn't exist, true otherwise.
EXT_EXPORT FFI_EXPORT FFI_NO_RUST_THUNK FFI_NAME(adk_enumerate_display_modes) bool sb_enumerate_display_modes(const int32_t display_index, const int32_t display_mode_index, FFI_PTR_WASM sb_enumerate_display_modes_result_t * const out_results);

/// Initialize the main display device and open a window for rendering.
///
/// * `display_index`: Index number of the display to initialize
/// * `display_mode_index`: Index number of the display mode to which the display is to be initialized
/// * `title`: Title of the display
///
/// You must use the ADKs RHI to create a rendering device from the native
/// window returned from this function.
EXT_EXPORT sb_window_t * sb_init_main_display(const int display_index, const int display_mode_index, const char * const title);

/// Change the main display refresh rate to the requested rate, if supported.
///
/// * `hz`: The requested refresh rate, must be supported at current resolution
///
/// Returns true if successful.
EXT_EXPORT bool sb_set_main_display_refresh_rate(const int32_t hz);

/// Get window client area
///
/// * `window`: The window whose dimensions are retrieved.
/// * `out_width`: The returned width (in pixels) of the client area
/// * `out_height`: The returned width (in pixels) of the client area
///
EXT_EXPORT void sb_get_window_client_area(sb_window_t * const window, int * const out_width, int * const out_height);

/// Destroy the window created by sb_init_main_display
EXT_EXPORT void sb_destroy_main_window();

#ifdef __cplusplus
}
#endif
