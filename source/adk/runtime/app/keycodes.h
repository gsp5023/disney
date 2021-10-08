/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
keycodes.h

defines ADK keycodes
these are identical to GLFW keycodes
*/
#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Printable keys */

FFI_EXPORT
FFI_ENUM_TRIM_START_NAMES(adk_)
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_keycode_e {
    /// reserved
    adk_key_none = 0,
    /// Space Key
    adk_key_space = 32,
    /// Keyboard Key: '
    adk_key_apostrophe = 39,
    /// Keyboard Key: ,
    adk_key_comma = 44,
    /// Keyboard Key: -
    adk_key_minus = 45,
    /// Keyboard Key: .
    adk_key_period = 46, /* . */
    /// Keyboard Key: /
    adk_key_slash = 47, /* / */
    /// Keyboard Key: 0
    adk_key_0 = 48,
    ///Keyboard Key: 1
    adk_key_1 = 49,
    /// Keyboard Key: 2
    adk_key_2 = 50,
    /// Keyboard Key: 3
    adk_key_3 = 51,
    /// Keyboard Key: 4
    adk_key_4 = 52,
    /// Keyboard Key: 5
    adk_key_5 = 53,
    /// Keyboard Key: 6
    adk_key_6 = 54,
    /// Keyboard Key: 7
    adk_key_7 = 55,
    /// Keyboard Key: 8
    adk_key_8 = 56,
    /// Keyboard Key: 9
    adk_key_9 = 57,
    /// Keyboard Key: ;
    adk_key_semicolon = 59, /* ; */
    /// Keyboard Key: =
    adk_key_equal = 61, /* = */
    /// Keyboard Key: a
    adk_key_a = 65,
    /// Keyboard Key: b
    adk_key_b = 66,
    /// Keyboard Key: c
    adk_key_c = 67,
    /// Keyboard Key: d
    adk_key_d = 68,
    /// Keyboard Key: e
    adk_key_e = 69,
    /// Keyboard Key: f
    adk_key_f = 70,
    /// Keyboard Key: g
    adk_key_g = 71,
    /// Keyboard Key: h
    adk_key_h = 72,
    /// Keyboard Key: i
    adk_key_i = 73,
    /// Keyboard Key: j
    adk_key_j = 74,
    /// Keyboard Key: k
    adk_key_k = 75,
    /// Keyboard Key: l
    adk_key_l = 76,
    /// Keyboard Key: m
    adk_key_m = 77,
    /// Keyboard Key: n
    adk_key_n = 78,
    /// Keyboard Key: o
    adk_key_o = 79,
    /// Keyboard Key: p
    adk_key_p = 80,
    /// Keyboard Key: q
    adk_key_q = 81,
    /// Keyboard Key: r
    adk_key_r = 82,
    /// Keyboard Key: s
    adk_key_s = 83,
    /// Keyboard Key: t
    adk_key_t = 84,
    /// Keyboard Key: u
    adk_key_u = 85,
    /// Keyboard Key: v
    adk_key_v = 86,
    /// Keyboard Key: w
    adk_key_w = 87,
    /// Keyboard Key: x
    adk_key_x = 88,
    /// Keyboard Key: y
    adk_key_y = 89,
    /// Keyboard Key: z
    adk_key_z = 90,
    /// Keyboard Key: [
    adk_key_left_bracket = 91,
    /// Keyboard Key: /* \ */
    adk_key_backslash = 92,
    /// Keyboard Key: ]
    adk_key_right_bracket = 93,
    /// Keyboard Key: /* ` */
    adk_key_grave_accent = 96,
    /// Keyboard Key: non-us #1
    adk_key_world_1 = 161,
    /// Keyboard Key: non-us #2
    adk_key_world_2 = 162,

    /* = function = keys = */
    /// Keyboard Key: Esc
    adk_key_escape = 256,
    /// Keyboard Key: Enter
    adk_key_enter = 257,
    /// Keyboard Key: tab
    adk_key_tab = 258,
    /// Keyboard Key: Backspace
    adk_key_backspace = 259,
    /// Keyboard Key: Insert
    adk_key_insert = 260,
    /// Keyboard Key: Delete
    adk_key_delete = 261,
    /// Keyboard Key: Right Arrow
    adk_key_right = 262,
    /// Keyboard Key: Left Arrow
    adk_key_left = 263,
    /// Keyboard Key: Down Arrow
    adk_key_down = 264,
    /// Keyboard Key: Up Arrow
    adk_key_up = 265,
    /// Keyboard Key: Page Up
    adk_key_page_up = 266,
    /// Keyboard Key: PageDown
    adk_key_page_down = 267,
    /// Keyboard Key: Home
    adk_key_home = 268,
    /// Keyboard Key: End
    adk_key_end = 269,
    /// Keyboard Key: Cap Lock
    adk_key_caps_lock = 280,
    /// Keyboard Key: Scroll Lock
    adk_key_scroll_lock = 281,
    /// Keyboard Key: Number Lock
    adk_key_num_lock = 282,
    /// Keyboard Key: Print Screen
    adk_key_print_screen = 283,
    /// Keyboard Key: Pause
    adk_key_pause = 284,
    /// Keyboard Key: F1
    adk_key_f1 = 290,
    /// Keyboard Key: F2
    adk_key_f2 = 291,
    /// Keyboard Key: F3
    adk_key_f3 = 292,
    /// Keyboard Key: F4
    adk_key_f4 = 293,
    /// Keyboard Key: F5
    adk_key_f5 = 294,
    /// Keyboard Key: F6
    adk_key_f6 = 295,
    /// Keyboard Key: F7
    adk_key_f7 = 296,
    /// Keyboard Key: F8
    adk_key_f8 = 297,
    /// Keyboard Key: F9
    adk_key_f9 = 298,
    /// Keyboard Key: F10
    adk_key_f10 = 299,
    /// Keyboard Key: F11
    adk_key_f11 = 300,
    /// Keyboard Key: F12
    adk_key_f12 = 301,
    /// Keyboard Key: F13
    adk_key_f13 = 302,
    /// Keyboard Key: F14
    adk_key_f14 = 303,
    /// Keyboard Key: F15
    adk_key_f15 = 304,
    /// Keyboard Key: F16
    adk_key_f16 = 305,
    /// Keyboard Key: F17
    adk_key_f17 = 306,
    /// Keyboard Key: F18
    adk_key_f18 = 307,
    /// Keyboard Key: F19
    adk_key_f19 = 308,
    /// Keyboard Key: F20
    adk_key_f20 = 309,
    /// Keyboard Key: F21
    adk_key_f21 = 310,
    /// Keyboard Key: F22
    adk_key_f22 = 311,
    /// Keyboard Key: F23
    adk_key_f23 = 312,
    /// Keyboard Key: F24
    adk_key_f24 = 313,
    /// Keyboard Key: F25
    adk_key_f25 = 314,
    /// Keypad Key: 0
    adk_key_kp_0 = 320,
    /// Keypad Key: 1
    adk_key_kp_1 = 321,
    /// Keypad Key: 2
    adk_key_kp_2 = 322,
    /// Keypad Key: 3
    adk_key_kp_3 = 323,
    /// Keypad Key: 4
    adk_key_kp_4 = 324,
    /// Keypad Key: 5
    adk_key_kp_5 = 325,
    /// Keypad Key: 6
    adk_key_kp_6 = 326,
    /// Keypad Key: 7
    adk_key_kp_7 = 327,
    /// Keypad Key: 8
    adk_key_kp_8 = 328,
    /// Keypad Key: 9
    adk_key_kp_9 = 329,
    /// Keypad Key:
    adk_key_kp_decimal = 330,
    /// Keypad Key: /
    adk_key_kp_divide = 331,
    /// Keypad Key: *
    adk_key_kp_multiply = 332,
    /// Keypad Key: -
    adk_key_kp_subtract = 333,
    /// Keypad Key: +
    adk_key_kp_add = 334,
    /// Keypad Key: Enter
    adk_key_kp_enter = 335,
    /// Keypad Key: =
    adk_key_kp_equal = 336,
    /// Keyboard Key: Left Shift
    adk_key_left_shift = 340,
    /// Keyboard Key: Left Control
    adk_key_left_control = 341,
    /// Keyboard Key: Left Alt
    adk_key_left_alt = 342,
    /// Keyboard Key: Left System Specific Key
    adk_key_left_super = 343,
    /// Keyboard Key: Right Shift
    adk_key_right_shift = 344,
    /// Keyboard Key: Right Control
    adk_key_right_control = 345,
    /// Keyboard Key: Right Alt
    adk_key_right_alt = 346,
    /// Keyboard Key: Right System Specific Key
    adk_key_right_super = 347,
    /// Keyboard Key: Menu
    adk_key_menu = 348,
    /// The last key in the enum
    adk_key_last = adk_key_menu,
    FORCE_ENUM_INT32(adk_keycode_e)
} adk_keycode_e;

/// Bitmask of any modifying keys
FFI_EXPORT
FFI_ENUM_BITFLAGS
FFI_ENUM_TRIM_START_NAMES(adk_)
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_mod_keys_e {
    /// Shift key
    adk_mod_shift = 0x0001,
    /// Control key
    adk_mod_control = 0x0002,
    /// Alt key
    adk_mod_alt = 0x0004,
    /// Platform specific "super" key
    adk_mod_super = 0x0008,
    FORCE_ENUM_INT32(adk_mod_keys_e)
} adk_mod_keys_e;

FFI_EXPORT
FFI_ENUM_TRIM_START_NAMES(adk_)
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_mouse_button_e {
    adk_mouse_button_1 = 0,
    adk_mouse_button_2 = 1,
    adk_mouse_button_3 = 2,
    adk_mouse_button_4 = 3,
    adk_mouse_button_5 = 4,
    adk_mouse_button_6 = 5,
    adk_mouse_button_7 = 6,
    adk_mouse_button_8 = 7,
    adk_mouse_button_last = adk_mouse_button_8,
    adk_mouse_button_left = adk_mouse_button_1,
    adk_mouse_button_right = adk_mouse_button_2,
    adk_mouse_button_middle = adk_mouse_button_3
} adk_mouse_button_e;

/// Bitmask used to signal which mouse buttons have been pressed
FFI_EXPORT
FFI_ENUM_BITFLAGS
FFI_ENUM_TRIM_START_NAMES(adk_)
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_mouse_button_mask_e {
    /// Left mouse button
    adk_mouse_button_mask_left = (1 << adk_mouse_button_left),
    /// Right mouse button
    adk_mouse_button_mask_right = (1 << adk_mouse_button_right),
    /// Middle mouse button
    adk_mouse_button_mask_middle = (1 << adk_mouse_button_middle),
    FORCE_ENUM_INT32(adk_mouse_button_mask_e)
} adk_mouse_button_mask_e;

/// List of all the supported game-pad buttons
FFI_EXPORT
FFI_ENUM_TRIM_START_NAMES(adk_gamepad_button_)
typedef enum adk_gamepad_button_e {
    /// A Button
    adk_gamepad_button_a = 0,
    /// B Button
    adk_gamepad_button_b = 1,
    /// X Button
    adk_gamepad_button_x = 2,
    /// Y Button
    adk_gamepad_button_y = 3,

    /// cross (i.e., X) Button
    adk_gamepad_button_cross = adk_gamepad_button_a,
    /// Circle Button
    adk_gamepad_button_circle = adk_gamepad_button_b,
    /// Square Button
    adk_gamepad_button_square = adk_gamepad_button_x,
    /// Triangle Button
    adk_gamepad_button_triangle = adk_gamepad_button_y,

    /// Left Bumper Button
    adk_gamepad_button_left_bumper = 4,
    /// Right Bumper Button
    adk_gamepad_button_right_bumper = 5,

    /// Back Button
    adk_gamepad_button_back = 6,
    /// Start Button
    adk_gamepad_button_start = 7,
    /// Guide Button
    adk_gamepad_button_guide = 8,

    /// Left Thumb-stick Button
    adk_gamepad_button_left_thumb = 9,
    /// Right Thumb-stick Button
    adk_gamepad_button_right_thumb = 10,

    /// D-Pad Up Button
    adk_gamepad_button_dpad_up = 11,
    /// D-Pad Right Button
    adk_gamepad_button_dpad_right = 12,
    /// D-Pad Down Button
    adk_gamepad_button_dpad_down = 13,
    /// D-Pad Left Button
    adk_gamepad_button_dpad_left = 14,

    /// Touchpad Button
    adk_gamepad_button_touchpad = 15,

    /// Left Trigger Button
    adk_gamepad_button_left_trigger = 16,
    /// Right Trigger Button
    adk_gamepad_button_right_trigger = 17,

    /// The end of the supported game-pad button list
    adk_gamepad_button_last = adk_gamepad_button_right_trigger,

    FORCE_ENUM_INT32(adk_gamepad_button_e)
} adk_gamepad_button_e;

/// The type of game-pad axis event
FFI_EXPORT
FFI_ENUM_TRIM_START_NAMES(adk_gamepad_axis_)
typedef enum adk_gamepad_axis_e {
    /// The left sticks X coordinate
    adk_gamepad_axis_left_x = 0,
    /// The left sticks Y coordinate
    adk_gamepad_axis_left_y = 1,
    /// The Right sticks X coordinate
    adk_gamepad_axis_right_x = 2,
    /// The Right sticks Y coordinate
    adk_gamepad_axis_right_y = 3,
    /// The left trigger
    adk_gamepad_axis_left_trigger = 4,
    /// The Right trigger
    adk_gamepad_axis_right_trigger = 5,
    /// The last supported event type
    adk_gamepad_axis_last = adk_gamepad_axis_right_trigger,
    FORCE_ENUM_INT32(adk_gamepad_axis_e)
} adk_gamepad_axis_e;

/// Types of supported Set-top box buttons
FFI_EXPORT
FFI_ENUM_CLEAN_NAMES
FFI_ENUM_CAPITALIZE_NAMES
typedef enum adk_stb_key_e {
    /// Play
    adk_stb_key_play = 1,
    /// Pause
    adk_stb_key_pause = 2,
    /// Fast Forward
    adk_stb_key_fast_forward = 3,
    /// Rewind
    adk_stb_key_rewind = 4,
    /// Stop
    adk_stb_key_stop = 5,
    /// Clear
    adk_stb_key_clear = 6,
    /// Back
    adk_stb_key_back = 7,
    /// Up
    adk_stb_key_up = 8,
    /// Down
    adk_stb_key_down = 9,
    /// Right
    adk_stb_key_right = 10,
    /// Left
    adk_stb_key_left = 11,
    /// Select
    adk_stb_key_select = 12,
    /// Power
    adk_stb_key_power = 13,
    /// Channel Up
    adk_stb_key_chan_up = 14,
    /// Channel Down
    adk_stb_key_chan_down = 15,
    /// 1
    adk_stb_key_one = 16,
    /// 2
    adk_stb_key_two = 17,
    /// 3
    adk_stb_key_three = 18,
    /// 4
    adk_stb_key_four = 19,
    /// 5
    adk_stb_key_five = 20,
    /// 6
    adk_stb_key_six = 21,
    /// 7
    adk_stb_key_seven = 22,
    /// 8
    adk_stb_key_eight = 23,
    /// 9
    adk_stb_key_nine = 24,
    /// 0
    adk_stb_key_zero = 25,
    /// Dot
    adk_stb_key_dot = 26,
    /// Info
    adk_stb_key_info = 27,
    /// Guide
    adk_stb_key_guide = 28,
    /// Menu
    adk_stb_key_menu = 29,
    /// Next
    adk_stb_key_next = 30,
    /// Previous
    adk_stb_key_prev = 31,
    /// Volume Up
    adk_stb_key_vol_up = 32,
    /// Volume Down
    adk_stb_key_vol_down = 33,
    /// Red
    adk_stb_key_red = 34,
    /// Blue
    adk_stb_key_blue = 35,
    /// Yellow
    adk_stb_key_yellow = 36,
    /// Green
    adk_stb_key_green = 37,
    /// Return
    adk_stb_key_return = 38,
    /// Home
    adk_stb_key_home = 39,
    /// Exit
    adk_stb_key_exit = 40,
    /// Play/Pause Button (i.e., both actions share a single button)
    adk_stb_key_play_pause = 41,
    /// Play From Start: Start playback from the beginning of the content
    adk_stb_key_play_from_start = 42,
    FORCE_ENUM_INT32(adk_stb_key_e)
} adk_stb_key_e;

#ifdef __cplusplus
}
#endif
