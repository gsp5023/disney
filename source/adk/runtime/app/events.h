/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
events.h

ADK application events and HID input
*/

#include "source/adk/runtime/app/keycodes.h"
#include "source/adk/runtime/time.h"
#include "source/adk/steamboat/sb_platform.h"

struct sb_window_t;

/// a constantly-sized ptr (always 8 bytes regardless of architecture)
typedef union ptr64_t {
    /// The pointer
    void * ptr;
    /// The address
    size_t adr;
    /// Used for internal house keeping
    struct {
        uint64_t force_8_bytes;
    } internal;
} ptr64_t;

STATIC_ASSERT(sizeof(ptr64_t) == 8);

/// The types of events that can be fired within the system
typedef enum adk_event_type_e {
    /// Event fired by the Application
    adk_application_event,
    /// Event fired by a Window
    adk_window_event,
    /// Keyboard events
    adk_key_event,
    /// Character event
    adk_char_event,
    /// Event fired by a mouse
    adk_mouse_event,
    /// Event fired by game controller
    adk_gamepad_event,
    /// Steamboat input event
    adk_stb_input_event,
    /// Steamboat rich input event
    adk_stb_rich_input_event,
    /// A time-based event was fired
    adk_time_event,
    /// Deeplink event was fired
    adk_deeplink_event,
    /// Event signally a change to to the power mode
    adk_power_mode_event,
    /// Event notifying client app when the system imposes a visual overlay on top of the app
    adk_system_overlay_event,
    FORCE_ENUM_INT32(adk_event_type_e)
} adk_event_type_e;

/*
===============================================================================
adk_application_event
===============================================================================
*/

/// Types of events that can be sent to the application
FFI_EXPORT
typedef enum adk_application_event_e {
    /// Directs the application to exit
    adk_application_event_quit,
    /// Informs the application that it was placed in the background
    adk_application_event_background,
    /// Informs the application that it was placed in the foreground
    adk_application_event_foreground,
    /// Informs the application that it was placed in a suspended state
    adk_application_event_suspend,
    /// Informs the application that it was removed from a suspended state
    adk_application_event_resume,
    /// Informs the application that it was has become the control focus
    adk_application_event_focus_gained,
    /// Informs the application that it has lost the control focus
    adk_application_event_focus_lost,
    /// Informs the application the network has become available
    adk_application_event_link_up,
    /// Informs the application the network has become unavailable
    adk_application_event_link_down,
    FORCE_ENUM_INT32(adk_application_event_e)
} adk_application_event_e;

/// Information about an application event
FFI_EXPORT
typedef struct adk_application_event_t {
    adk_application_event_e event;
} adk_application_event_t;

/*
===============================================================================
adk_window_event
===============================================================================
*/

/// Type of window event
typedef enum adk_window_event_e {
    /// The window closed
    adk_window_event_close,
    /// The window changed its focus
    adk_window_event_focus_changed,
    FORCE_ENUM_INT32(adk_window_event_e)
} adk_window_event_e;

/// Information about the a change in focus
FFI_EXPORT
typedef struct adk_window_event_focus_changed_t {
    /// ID of new object with focus
    int32_t new_focus;
} adk_window_event_focus_changed_t;

FFI_EXPORT
FFI_DISCRIMINATED_UNION
typedef struct adk_window_event_data_t {
    /// The type of window event
    adk_window_event_e event;
    union {
        int8_t close;
        /// Information about the a change in focus
        adk_window_event_focus_changed_t focus_changed;
    };
} adk_window_event_data_t;

/// Information about a window event
FFI_EXPORT
typedef struct adk_window_event_t {
    /// Pointer to the window
    ptr64_t window;
    adk_window_event_data_t event_data;
} adk_window_event_t;

/*
===============================================================================
adk_key_event
===============================================================================
*/

/// The type of keyboard event
FFI_EXPORT
typedef enum adk_key_event_e {
    /// A key was pressed
    adk_key_event_key_down,
    /// A key was released
    adk_key_event_key_up,
    FORCE_ENUM_INT32(adk_key_event_e)
} adk_key_event_e;

/// Information about a keyboard event
FFI_EXPORT
typedef struct adk_key_event_t {
    /// The type of keyboard event
    adk_key_event_e event;
    /// The key that fired the event
    adk_keycode_e key;
    /// Bitmask of any additional, modifying key that were pressed
    adk_mod_keys_e mod_keys;
    /// True if this was a repeat of the previous key event
    int8_t repeat;
} adk_key_event_t;

/*
===============================================================================
adk_char_event
===============================================================================
*/

/// Information about a character event
FFI_EXPORT
typedef struct adk_char_event_t {
    /// UTF-32 encoded character
    uint32_t codepoint_utf32;
} adk_char_event_t;

/*
===============================================================================
adk_mouse_event
===============================================================================
*/

/// Types of mouse events
typedef enum adk_mouse_event_e {
    /// Mouse button pressed
    adk_mouse_event_button,
    /// Mouse moved
    adk_mouse_event_motion,
    FORCE_ENUM_INT32(adk_mouse_event_e)
} adk_mouse_event_e;

/// Information about the state of the mouse during an event
FFI_EXPORT
typedef struct adk_mouse_state_t {
    /// Position of the mouse cursor
    int32_t x, y;
    /// Bitmask used to signal which mouse buttons have been pressed
    adk_mouse_button_mask_e button_mask;
    /// Bitmask of any modifying keys
    adk_mod_keys_e mod_keys;
} adk_mouse_state_t;

/// The type of mouse button event
FFI_EXPORT
typedef enum adk_mouse_button_event_e {
    /// Mouse button pressed
    adk_mouse_button_event_down,
    /// Mouse button released
    adk_mouse_button_event_up,
    FORCE_ENUM_INT32(adk_mouse_button_event_e)
} adk_mouse_button_event_e;

/// Information about a mouse button event
FFI_EXPORT
typedef struct adk_mouse_button_event_t {
    ///  The type of mouse button event
    adk_mouse_button_event_e event;
    /// The button's ID
    int32_t button;
} adk_mouse_button_event_t;

/// Typse of motion events generated by a mouse
FFI_EXPORT
typedef enum adk_mouse_motion_event_e {
    /// The moue moved
    adk_mouse_motion_event_move,
    /// The mouse entered a region
    adk_mouse_motion_event_enter,
    /// The mouse left a region
    adk_mouse_motion_event_leave,
    FORCE_ENUM_INT32(adk_mouse_motion_event_e)
} adk_mouse_motion_event_e;

FFI_EXPORT
FFI_DISCRIMINATED_UNION
typedef struct adk_mouse_event_data_t {
    /// Type of mouse event
    adk_mouse_event_e event;
    union {
        /// Information about the event valid if `event` = `adk_mouse_event_button`
        adk_mouse_button_event_t button_event;
        /// Information about the event valid if `event` = `adk_mouse_event_motion`
        adk_mouse_motion_event_e motion_event;
    };
} adk_mouse_event_data_t;

/// Information about a mouse event
FFI_EXPORT
typedef struct adk_mouse_event_t {
    /// The window which recorded the mouse event
    ptr64_t window;
    /// The state of the mouse at the time of the event
    adk_mouse_state_t mouse_state;
    adk_mouse_event_data_t event_data;
} adk_mouse_event_t;

/*
===============================================================================
adk_gamepad_event
===============================================================================
*/

FFI_EXPORT
typedef struct adk_gamepad_state_t {
    uint8_t buttons[adk_gamepad_button_last + 1];
    float axis[adk_gamepad_axis_last + 1];
} adk_gamepad_state_t;

/// Types of game controller events
typedef enum adk_gamepad_event_e {
    /// The controller was connected
    adk_gamepad_event_connect,
    /// The controller was disconnected
    adk_gamepad_event_disconnect,
    /// The controller had a button pressed
    adk_gamepad_event_button,
    /// The controller had a analog stick move
    adk_gamepad_event_axis,
    FORCE_ENUM_INT32(adk_gamepad_event_e)
} adk_gamepad_event_e;

// The type of game-pad button event
FFI_EXPORT
typedef enum adk_gamepad_button_event_e {
    /// The button was pressed
    adk_gamepad_button_event_down,
    /// The button was released
    adk_gamepad_button_event_up,
    FORCE_ENUM_INT32(adk_gamepad_button_event_e)
} adk_gamepad_button_event_e;

/// Information about a game controller's button event
FFI_EXPORT
typedef struct adk_gamepad_button_event_t {
    /// The type of game-pad button event
    adk_gamepad_button_event_e event;
    /// The button
    adk_gamepad_button_e button;
} adk_gamepad_button_event_t;

/// Information about a game controller's control stick movement
FFI_EXPORT
typedef struct adk_gamepad_axis_event_t {
    /// The type of axis event
    adk_gamepad_axis_e axis;
    /// for joysticks the range is [-1,1] for bumpers the range is [0,1]
    float pressure;
    /// dead zone will be scaled to fit in pressure's range
    float dead_zone;
} adk_gamepad_axis_event_t;

FFI_EXPORT
FFI_DISCRIMINATED_UNION
typedef struct adk_gamepad_event_data_t {
    /// The type of game-pad event
    adk_gamepad_event_e event;
    union {
        int8_t unused1;
        int8_t unused2;
        // Information about the event, valid when `even` == `adk_gamepad_event_button`
        adk_gamepad_button_event_t button_event;
        // Information about the event, valid when `even` == `adk_gamepad_event_axis`
        adk_gamepad_axis_event_t axis_event;
    };
} adk_gamepad_event_data_t;

/// Information about a game controller event
FFI_EXPORT
typedef struct adk_gamepad_event_t {
    /// ID of game-pad
    int32_t gamepad_index;
    adk_gamepad_event_data_t event_data;
} adk_gamepad_event_t;

/*
===============================================================================
adk_stb_input_event
===============================================================================
*/

/// An event coming from a set-top box controller
FFI_EXPORT
typedef struct adk_stb_input_event_t {
    /// The controller key
    adk_stb_key_e stb_key;
    /// Number of consecutive firings of this event
    int32_t repeat;
} adk_stb_input_event_t;

/*
===============================================================================
adk_stb_rich_input_event
===============================================================================
*/

/// An event coming from a media remote
FFI_EXPORT
typedef struct adk_stb_rich_input_event_t {
    /// The controller key
    adk_stb_key_e stb_key;
    /// Up/down
    adk_key_event_e event;
    /// Non-zero if this was a key repeat.
    int8_t repeat;
} adk_stb_rich_input_event_t;

/*
===============================================================================
adk_deeplink_event
===============================================================================
*/

/// Information about a deep link triggered event
FFI_EXPORT
typedef struct adk_deeplink_event_t {
    /// Handle to the deeplink information
    ptr64_t handle;
} adk_deeplink_event_t;

/*
===============================================================================
adk_power_mode_event
===============================================================================
*/

/// Types of power modes
FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
typedef enum adk_power_mode_e {
    /// Performance power mode
    adk_power_mode_performance,
    /// Balanced power mode
    adk_power_mode_balanced,
    ///  Low Power Mode
    adk_power_mode_low_power,
    FORCE_ENUM_INT32(adk_power_mode_e)
} adk_power_mode_e;

/// Information about a power event
FFI_EXPORT
typedef struct adk_power_mode_t {
    /// The type of power event
    adk_power_mode_e mode;
} adk_power_mode_t;

/*
===============================================================================
adk_system_overlay_event
===============================================================================
*/

/// Types of system overlays
FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
typedef enum adk_system_overlay_type_e {
    /// An overlay informing the user of a potential problem
    adk_system_overlay_alert,
    /// An overlay reminding the user to perform an action or informing the user of pertinent information
    adk_system_overlay_reminder,
    /// An overlay facilitating a text search
    adk_system_overlay_text_search,
    /// An overlay facilitating a voice search
    adk_system_overlay_voice_search,
    /// To be used if the overlay does not match any of the other overlay types
    adk_system_overlay_reserved,
    FORCE_ENUM_INT32(adk_system_overlay_type_e)
} adk_system_overlay_type_e;

/// Types of system overlay states
FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
typedef enum adk_system_overlay_state_e {
    /// The overlay was displayed on screen
    adk_system_overlay_begin,
    /// The displayed overlay for removed from the screen
    adk_system_overlay_end,
    FORCE_ENUM_INT32(adk_system_overlay_state_e)
} adk_system_overlay_state_e;

/// Information about a system overlay event
FFI_EXPORT
typedef struct adk_system_overlay_event_t {
    /// The type of the overlay
    adk_system_overlay_type_e overlay_type; // NOTE: cannot be named 'type' as it conflicts with the Rust keyword in the FFI conversion
    /// The state of the overlay
    adk_system_overlay_state_e state;
    /// The normalized position of the overlay on the screen.  (i.e., value between 0 and 1 inclusive)
    float n_x, n_y;
    /// The normalized dimensions of the overlay rectangle (i.e., value between 0 and 1 inclusive with 1 = the entire height/width)
    float n_width, n_height;
    /// Field to pass data when using using the `adk_system_overlay_reserved` overlay type;
    uint8_t reserved[32];
} adk_system_overlay_event_t;

/*
===============================================================================
general event container
===============================================================================
*/

FFI_EXPORT
FFI_DISCRIMINATED_UNION
typedef struct adk_event_data_t {
    /// The type of event
    adk_event_type_e type;
    union {
        /// Event information - valid when `type` = `adk_application_event`
        adk_application_event_t app;
        /// Event information - valid when `type` = `adk_window_event`
        adk_window_event_t win;
        /// Event information - valid when `type` = `adk_key_event`
        adk_key_event_t key;
        /// Event information - valid when `type` = `adk_char_event`
        adk_char_event_t ch;
        /// Event information - valid when `type` = `adk_mouse_event`
        adk_mouse_event_t mouse;
        /// Event information - valid when `type` = `adk_gamepad_event`
        adk_gamepad_event_t gamepad;
        /// Event information - valid when `type` = `adk_stb_input_event`
        adk_stb_input_event_t stb_input;
        /// Event information - valid when `type` = `adk_stb_rich_input_event`
        adk_stb_rich_input_event_t stb_rich_input;
        /// Event information - valid when `type` = `adk_time_event`
        int8_t unused;
        /// Event information - valid when `type` = `adk_deeplink_event`
        adk_deeplink_event_t deeplink;
        /// Event information - valid when `type` = `adk_power_mode_event`
        adk_power_mode_t power_mode;
        /// Event information - valid when `type` = `adk_system_overlay_event`
        adk_system_overlay_event_t overlay;
    };
} adk_event_data_t;

/// General event container
FFI_EXPORT
typedef struct adk_event_t {
    /// The time of the event in milliseconds
    milliseconds_t time;
    /// The event data
    adk_event_data_t event_data;
} adk_event_t;
