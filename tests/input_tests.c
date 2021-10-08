/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
input_tests.c

input test fixture
*/

#include "source/adk/runtime/app/events.h"
#include "source/adk/steamboat/sb_platform.h"
#include "testapi.h"

// Convert joypad bit to index for mapping joypad controls
const char * joypad_button_names[] = {
    "Cross Button\n",
    "Circle Button\n",
    "Square Button\n",
    "Triangle Button\n",
    "Left Bumper\n",
    "Right Bumper\n",
    "Back Button\n",
    "Start Button\n",
    "Touchpad Button\n",
    "Left Thumb Button\n",
    "Right Thumb Button\n",
    "Dpad Up\n",
    "Dpad Right\n",
    "Dpad Down\n",
    "Dpad Left\n",
    "Touchpad\n",
    "Left Trigger\n",
    "Right Trigger\n"};

const char * joypad_axis_names[] = {
    "Left Stick X",
    "Left Stick Y",
    "Right Stick X",
    "Right Stick Y",
    "Left Trigger",
    "Right Trigger"};

const char * keyboard_key_names[] = {
    [adk_key_none] = "None\n",
    [adk_key_space] = "Space\n",
    [adk_key_apostrophe] = "Apostrophe\n",
    [adk_key_comma] = "Comma\n",
    [adk_key_minus] = "Minus\n",
    [adk_key_period] = "Period\n",
    [adk_key_slash] = "Slash\n",
    [adk_key_0] = "0\n",
    [adk_key_1] = "1\n",
    [adk_key_2] = "2\n",
    [adk_key_3] = "3\n",
    [adk_key_4] = "4\n",
    [adk_key_5] = "5\n",
    [adk_key_6] = "6\n",
    [adk_key_7] = "7\n",
    [adk_key_8] = "8\n",
    [adk_key_9] = "9\n",
    [adk_key_semicolon] = "Semicolon\n",
    [adk_key_equal] = "Equal\n",
    [adk_key_a] = "A\n",
    [adk_key_b] = "B\n",
    [adk_key_c] = "C\n",
    [adk_key_d] = "D\n",
    [adk_key_e] = "E\n",
    [adk_key_f] = "F\n",
    [adk_key_g] = "G\n",
    [adk_key_h] = "H\n",
    [adk_key_i] = "I\n",
    [adk_key_j] = "J\n",
    [adk_key_k] = "K\n",
    [adk_key_l] = "L\n",
    [adk_key_m] = "M\n",
    [adk_key_n] = "N\n",
    [adk_key_o] = "O\n",
    [adk_key_p] = "P\n",
    [adk_key_q] = "Q\n",
    [adk_key_r] = "R\n",
    [adk_key_s] = "S\n",
    [adk_key_t] = "T\n",
    [adk_key_u] = "U\n",
    [adk_key_v] = "V\n",
    [adk_key_w] = "W\n",
    [adk_key_x] = "X\n",
    [adk_key_y] = "Y\n",
    [adk_key_z] = "Z\n",
    [adk_key_left_bracket] = "Left Bracket\n",
    [adk_key_backslash] = "Back Slash\n",
    [adk_key_right_bracket] = "Right Bracket\n",
    [adk_key_grave_accent] = "Grave Accent\n",
    [adk_key_world_1] = "Non US #1\n",
    [adk_key_world_2] = "Non US #2\n",
    [adk_key_escape] = "Esc\n",
    [adk_key_enter] = "Enter\n",
    [adk_key_tab] = "Tab\n",
    [adk_key_backspace] = "Backspace\n",
    [adk_key_insert] = "Insert\n",
    [adk_key_delete] = "Delete\n",
    [adk_key_right] = "Right\n",
    [adk_key_left] = "Left\n",
    [adk_key_down] = "Down\n",
    [adk_key_up] = "Up\n",
    [adk_key_page_up] = "Page Up\n",
    [adk_key_page_down] = "Page Down\n",
    [adk_key_home] = "Home\n",
    [adk_key_end] = "End\n",
    [adk_key_caps_lock] = "Caps Lock\n",
    [adk_key_scroll_lock] = "Scroll Lock\n",
    [adk_key_num_lock] = "Num Lock\n",
    [adk_key_print_screen] = "Print Screen\n",
    [adk_key_pause] = "Pause\n",
    [adk_key_f1] = "F1\n",
    [adk_key_f2] = "F2\n",
    [adk_key_f3] = "F3\n",
    [adk_key_f4] = "F4\n",
    [adk_key_f5] = "F5\n",
    [adk_key_f6] = "F6\n",
    [adk_key_f7] = "F7\n",
    [adk_key_f8] = "F8\n",
    [adk_key_f9] = "F9\n",
    [adk_key_f10] = "F10\n",
    [adk_key_f11] = "F11\n",
    [adk_key_f12] = "F12\n",
    [adk_key_f13] = "F13\n",
    [adk_key_f14] = "F14\n",
    [adk_key_f15] = "F15\n",
    [adk_key_f16] = "F16\n",
    [adk_key_f17] = "F17\n",
    [adk_key_f18] = "F18\n",
    [adk_key_f19] = "F19\n",
    [adk_key_f20] = "F20\n",
    [adk_key_f21] = "F21\n",
    [adk_key_f22] = "F22\n",
    [adk_key_f23] = "F23\n",
    [adk_key_f24] = "F24\n",
    [adk_key_f25] = "F25\n",
    [adk_key_kp_0] = "KeyPad 0\n",
    [adk_key_kp_1] = "KeyPad 1\n",
    [adk_key_kp_2] = "KeyPad 2\n",
    [adk_key_kp_3] = "KeyPad 3\n",
    [adk_key_kp_4] = "KeyPad 4\n",
    [adk_key_kp_5] = "KeyPad 5\n",
    [adk_key_kp_6] = "KeyPad 6\n",
    [adk_key_kp_7] = "KeyPad 7\n",
    [adk_key_kp_8] = "KeyPad 8\n",
    [adk_key_kp_9] = "KeyPad 9\n",
    [adk_key_kp_decimal] = "KeyPad Decimal\n",
    [adk_key_kp_divide] = "KeyPad Divide\n",
    [adk_key_kp_multiply] = "KeyPad Multiply\n",
    [adk_key_kp_subtract] = "KeyPad Subtract\n",
    [adk_key_kp_add] = "KeyPad Add\n",
    [adk_key_kp_enter] = "KeyPad Enter\n",
    [adk_key_kp_equal] = "KeyPad Equal\n",
    [adk_key_left_shift] = "Left Shift\n",
    [adk_key_left_control] = "Left Control\n",
    [adk_key_left_alt] = "Left Alt\n",
    [adk_key_left_super] = "Left Super\n",
    [adk_key_right_shift] = "Right Shift\n",
    [adk_key_right_control] = "Right Control\n",
    [adk_key_right_alt] = "Right Alt\n",
    [adk_key_right_super] = "Right Super\n",
    [adk_key_menu] = "Menu\n",
};

const char * remote_button_names[] = {
    [adk_stb_key_play] = "Play",
    [adk_stb_key_pause] = "Pause",
    [adk_stb_key_play_pause] = "Play/Pause",
    [adk_stb_key_fast_forward] = "Fast Forward",
    [adk_stb_key_rewind] = "Rewind",
    [adk_stb_key_stop] = "Stop",
    [adk_stb_key_clear] = "Clear",
    [adk_stb_key_back] = "Back",
    [adk_stb_key_up] = "Up",
    [adk_stb_key_down] = "Down",
    [adk_stb_key_right] = "Right",
    [adk_stb_key_left] = "Left",
    [adk_stb_key_select] = "Select",
    [adk_stb_key_power] = "Power",
    [adk_stb_key_chan_up] = "Channel Up",
    [adk_stb_key_chan_down] = "Channel Down",
    [adk_stb_key_one] = "One",
    [adk_stb_key_two] = "Two",
    [adk_stb_key_three] = "Three",
    [adk_stb_key_four] = "Four",
    [adk_stb_key_five] = "Five",
    [adk_stb_key_six] = "Six",
    [adk_stb_key_seven] = "Seven",
    [adk_stb_key_eight] = "Eight",
    [adk_stb_key_nine] = "Nine",
    [adk_stb_key_zero] = "Zero",
    [adk_stb_key_dot] = "Dot (.)",
    [adk_stb_key_info] = "Info",
    [adk_stb_key_guide] = "Guide",
    [adk_stb_key_menu] = "Menu",
    [adk_stb_key_next] = "Next",
    [adk_stb_key_prev] = "Prev",
    [adk_stb_key_vol_up] = "Volume Up",
    [adk_stb_key_vol_down] = "Volume Down",
    [adk_stb_key_red] = "Red",
    [adk_stb_key_blue] = "Blue",
    [adk_stb_key_yellow] = "Yellow",
    [adk_stb_key_green] = "Green",
    [adk_stb_key_return] = "Return",
    [adk_stb_key_home] = "Home",
    [adk_stb_key_exit] = "Exit"};

int test_inputs() {
    const adk_event_t *head, *tail;
    print_message("Input Test\n");
    while (true) {
        sb_tick(&head, &tail);

        // Prints message if input from gamepad is from button
        if (head->event_data.type == adk_gamepad_event && head->event_data.gamepad.event_data.event && head->event_data.app.event == adk_application_event_suspend) {
            if (head->event_data.gamepad.event_data.button_event.event == adk_gamepad_button_event_down && head->event_data.gamepad.event_data.button_event.button <= adk_gamepad_button_last) {
                print_message(joypad_button_names[head->event_data.gamepad.event_data.button_event.button], "");
            }
        }

        // Prints message if input from gamepad is from axis
        if (head->event_data.type == adk_gamepad_event && head->event_data.gamepad.event_data.event == adk_gamepad_event_axis) {
            print_message(joypad_axis_names[head->event_data.gamepad.event_data.axis_event.axis], "");
            debug_write_line(": %f\n", head->event_data.gamepad.event_data.axis_event.pressure);
        }

        // Prints message if input from remote is received
        if ((head->event_data.type == adk_stb_input_event) && (head->event_data.app.event == adk_application_event_background || head->event_data.app.event == adk_application_event_suspend || head->event_data.app.event == adk_application_event_resume || head->event_data.app.event == adk_application_event_focus_gained || head->event_data.app.event == 12 || head->event_data.app.event == 30 || head->event_data.app.event == 31 || head->event_data.app.event == 38)) {
            char key[sizeof(remote_button_names[head->event_data.stb_input.stb_key]) + 2];
            sprintf(key, "%s%s", remote_button_names[head->event_data.stb_input.stb_key], "\n");
            print_message(key, "");
        }

        // Prints message if input from keyboard is received
        if (head->event_data.type == adk_key_event) {
            if (head->event_data.key.event == adk_key_event_key_down) {
                print_message(keyboard_key_names[head->event_data.key.key], "");

                // Exit the loop when ESC is pressed
                if (head->event_data.key.key == adk_key_escape) {
                    break;
                }
            }
        }
    }

    return 0;
}