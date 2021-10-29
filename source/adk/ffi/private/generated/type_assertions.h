/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

STATIC_ASSERT(sizeof(sb_locale_t) == 128);
STATIC_ASSERT(offsetof(sb_locale_t, language) == 0);
STATIC_ASSERT(offsetof(sb_locale_t, region) == 64);

STATIC_ASSERT(sizeof(adk_application_event_e) == 4);

STATIC_ASSERT(sizeof(adk_application_event_t) == 4);
STATIC_ASSERT(offsetof(adk_application_event_t, event) == 0);

STATIC_ASSERT(sizeof(adk_char_event_t) == 4);
STATIC_ASSERT(offsetof(adk_char_event_t, codepoint_utf32) == 0);

STATIC_ASSERT(sizeof(sb_cpu_mem_status_t) == 24);
STATIC_ASSERT(offsetof(sb_cpu_mem_status_t, cpu_utilization) == 0);
STATIC_ASSERT(offsetof(sb_cpu_mem_status_t, memory_available) == 8);
STATIC_ASSERT(offsetof(sb_cpu_mem_status_t, memory_used) == 16);

STATIC_ASSERT(sizeof(adk_curl_info_e) == 4);

STATIC_ASSERT(sizeof(adk_deeplink_event_t) == 8);
STATIC_ASSERT(offsetof(adk_deeplink_event_t, handle) == 0);

STATIC_ASSERT(sizeof(adk_device_class_e) == 4);

STATIC_ASSERT(sizeof(sb_display_mode_t) == 12);
STATIC_ASSERT(offsetof(sb_display_mode_t, width) == 0);
STATIC_ASSERT(offsetof(sb_display_mode_t, height) == 4);
STATIC_ASSERT(offsetof(sb_display_mode_t, hz) == 8);

STATIC_ASSERT(sizeof(adk_display_modes_flags_e) == 4);

STATIC_ASSERT(sizeof(sb_enumerate_display_modes_result_t) == 16);
STATIC_ASSERT(offsetof(sb_enumerate_display_modes_result_t, display_mode) == 0);
STATIC_ASSERT(offsetof(sb_enumerate_display_modes_result_t, status) == 12);

STATIC_ASSERT(sizeof(adk_event_data_t) == 64);
STATIC_ASSERT(offsetof(adk_event_data_t, type) == 0);

STATIC_ASSERT(sizeof(adk_event_t) == 72);
STATIC_ASSERT(offsetof(adk_event_t, time) == 0);
STATIC_ASSERT(offsetof(adk_event_t, event_data) == 8);

STATIC_ASSERT(sizeof(sb_file_mode_bits_e) == 4);

STATIC_ASSERT(sizeof(adk_fwrite_error_e) == 4);

STATIC_ASSERT(sizeof(adk_fwrite_result_t) == 8);
STATIC_ASSERT(offsetof(adk_fwrite_result_t, elements_written) == 0);
STATIC_ASSERT(offsetof(adk_fwrite_result_t, error) == 4);

STATIC_ASSERT(sizeof(adk_gamepad_axis_e) == 4);

STATIC_ASSERT(sizeof(adk_gamepad_axis_event_t) == 12);
STATIC_ASSERT(offsetof(adk_gamepad_axis_event_t, axis) == 0);
STATIC_ASSERT(offsetof(adk_gamepad_axis_event_t, pressure) == 4);
STATIC_ASSERT(offsetof(adk_gamepad_axis_event_t, dead_zone) == 8);

STATIC_ASSERT(sizeof(adk_gamepad_button_e) == 4);

STATIC_ASSERT(sizeof(adk_gamepad_button_event_e) == 4);

STATIC_ASSERT(sizeof(adk_gamepad_button_event_t) == 8);
STATIC_ASSERT(offsetof(adk_gamepad_button_event_t, event) == 0);
STATIC_ASSERT(offsetof(adk_gamepad_button_event_t, button) == 4);

STATIC_ASSERT(sizeof(adk_gamepad_event_data_t) == 16);
STATIC_ASSERT(offsetof(adk_gamepad_event_data_t, event) == 0);

STATIC_ASSERT(sizeof(adk_gamepad_event_t) == 20);
STATIC_ASSERT(offsetof(adk_gamepad_event_t, gamepad_index) == 0);
STATIC_ASSERT(offsetof(adk_gamepad_event_t, event_data) == 4);

STATIC_ASSERT(sizeof(adk_gamepad_state_t) == 44);
STATIC_ASSERT(offsetof(adk_gamepad_state_t, buttons) == 0);
STATIC_ASSERT(offsetof(adk_gamepad_state_t, axis) == 20);

STATIC_ASSERT(sizeof(adk_gpu_ready_texture_formats_e) == 4);

STATIC_ASSERT(sizeof(adk_key_event_e) == 4);

STATIC_ASSERT(sizeof(adk_key_event_t) == 16);
STATIC_ASSERT(offsetof(adk_key_event_t, event) == 0);
STATIC_ASSERT(offsetof(adk_key_event_t, key) == 4);
STATIC_ASSERT(offsetof(adk_key_event_t, mod_keys) == 8);
STATIC_ASSERT(offsetof(adk_key_event_t, repeat) == 12);

STATIC_ASSERT(sizeof(adk_keycode_e) == 4);

STATIC_ASSERT(sizeof(adk_mod_keys_e) == 4);

STATIC_ASSERT(sizeof(adk_mouse_button_event_e) == 4);

STATIC_ASSERT(sizeof(adk_mouse_button_event_t) == 8);
STATIC_ASSERT(offsetof(adk_mouse_button_event_t, event) == 0);
STATIC_ASSERT(offsetof(adk_mouse_button_event_t, button) == 4);

STATIC_ASSERT(sizeof(adk_mouse_button_mask_e) == 4);

STATIC_ASSERT(sizeof(adk_mouse_event_data_t) == 12);
STATIC_ASSERT(offsetof(adk_mouse_event_data_t, event) == 0);

STATIC_ASSERT(sizeof(adk_mouse_event_t) == 40);
STATIC_ASSERT(offsetof(adk_mouse_event_t, window) == 0);
STATIC_ASSERT(offsetof(adk_mouse_event_t, mouse_state) == 8);
STATIC_ASSERT(offsetof(adk_mouse_event_t, event_data) == 24);

STATIC_ASSERT(sizeof(adk_mouse_motion_event_e) == 4);

STATIC_ASSERT(sizeof(adk_mouse_state_t) == 16);
STATIC_ASSERT(offsetof(adk_mouse_state_t, x) == 0);
STATIC_ASSERT(offsetof(adk_mouse_state_t, y) == 4);
STATIC_ASSERT(offsetof(adk_mouse_state_t, button_mask) == 8);
STATIC_ASSERT(offsetof(adk_mouse_state_t, mod_keys) == 12);

STATIC_ASSERT(sizeof(adk_system_metrics_t) == 3940);
STATIC_ASSERT(offsetof(adk_system_metrics_t, core_version) == 0);
STATIC_ASSERT(offsetof(adk_system_metrics_t, config) == 256);
STATIC_ASSERT(offsetof(adk_system_metrics_t, vendor) == 512);
STATIC_ASSERT(offsetof(adk_system_metrics_t, partner) == 768);
STATIC_ASSERT(offsetof(adk_system_metrics_t, device) == 1024);
STATIC_ASSERT(offsetof(adk_system_metrics_t, firmware) == 1280);
STATIC_ASSERT(offsetof(adk_system_metrics_t, software) == 1536);
STATIC_ASSERT(offsetof(adk_system_metrics_t, revision) == 1792);
STATIC_ASSERT(offsetof(adk_system_metrics_t, gpu) == 2048);
STATIC_ASSERT(offsetof(adk_system_metrics_t, cpu) == 2304);
STATIC_ASSERT(offsetof(adk_system_metrics_t, device_id) == 2560);
STATIC_ASSERT(offsetof(adk_system_metrics_t, device_region) == 2625);
STATIC_ASSERT(offsetof(adk_system_metrics_t, tenancy) == 2881);
STATIC_ASSERT(offsetof(adk_system_metrics_t, partner_guid) == 3137);
STATIC_ASSERT(offsetof(adk_system_metrics_t, advertising_id) == 3393);
STATIC_ASSERT(offsetof(adk_system_metrics_t, main_memory_mbytes) == 3652);
STATIC_ASSERT(offsetof(adk_system_metrics_t, video_memory_mbytes) == 3656);
STATIC_ASSERT(offsetof(adk_system_metrics_t, num_hardware_threads) == 3660);
STATIC_ASSERT(offsetof(adk_system_metrics_t, num_cores) == 3664);
STATIC_ASSERT(offsetof(adk_system_metrics_t, device_class) == 3668);
STATIC_ASSERT(offsetof(adk_system_metrics_t, gpu_ready_texture_formats) == 3672);
STATIC_ASSERT(offsetof(adk_system_metrics_t, persistent_storage_available_bytes) == 3676);
STATIC_ASSERT(offsetof(adk_system_metrics_t, persistent_storage_max_write_bytes_per_second) == 3680);
STATIC_ASSERT(offsetof(adk_system_metrics_t, persona_id) == 3684);

STATIC_ASSERT(sizeof(sb_network_type_e) == 4);

STATIC_ASSERT(sizeof(adk_power_mode_e) == 4);

STATIC_ASSERT(sizeof(adk_power_mode_t) == 4);
STATIC_ASSERT(offsetof(adk_power_mode_t, mode) == 0);

STATIC_ASSERT(sizeof(adk_rand_generator_t) == 32);
STATIC_ASSERT(offsetof(adk_rand_generator_t, state) == 0);

STATIC_ASSERT(sizeof(sb_stat_error_e) == 4);

STATIC_ASSERT(sizeof(sb_stat_result_t) == 64);
STATIC_ASSERT(offsetof(sb_stat_result_t, stat) == 0);
STATIC_ASSERT(offsetof(sb_stat_result_t, error) == 56);

STATIC_ASSERT(sizeof(sb_stat_t) == 56);
STATIC_ASSERT(offsetof(sb_stat_t, mode) == 0);
STATIC_ASSERT(offsetof(sb_stat_t, access_time_s) == 8);
STATIC_ASSERT(offsetof(sb_stat_t, modification_time_s) == 16);
STATIC_ASSERT(offsetof(sb_stat_t, create_time_s) == 24);
STATIC_ASSERT(offsetof(sb_stat_t, hard_links) == 32);
STATIC_ASSERT(offsetof(sb_stat_t, size) == 40);
STATIC_ASSERT(offsetof(sb_stat_t, user_id) == 48);
STATIC_ASSERT(offsetof(sb_stat_t, group_id) == 52);

STATIC_ASSERT(sizeof(adk_stb_input_event_t) == 8);
STATIC_ASSERT(offsetof(adk_stb_input_event_t, stb_key) == 0);
STATIC_ASSERT(offsetof(adk_stb_input_event_t, repeat) == 4);

STATIC_ASSERT(sizeof(adk_stb_key_e) == 4);

STATIC_ASSERT(sizeof(adk_stb_rich_input_event_t) == 12);
STATIC_ASSERT(offsetof(adk_stb_rich_input_event_t, stb_key) == 0);
STATIC_ASSERT(offsetof(adk_stb_rich_input_event_t, event) == 4);
STATIC_ASSERT(offsetof(adk_stb_rich_input_event_t, repeat) == 8);

STATIC_ASSERT(sizeof(adk_system_overlay_event_t) == 56);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, overlay_type) == 0);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, state) == 4);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, n_x) == 8);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, n_y) == 12);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, n_width) == 16);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, n_height) == 20);
STATIC_ASSERT(offsetof(adk_system_overlay_event_t, reserved) == 24);

STATIC_ASSERT(sizeof(adk_system_overlay_state_e) == 4);

STATIC_ASSERT(sizeof(adk_system_overlay_type_e) == 4);

STATIC_ASSERT(sizeof(adk_window_event_data_t) == 8);
STATIC_ASSERT(offsetof(adk_window_event_data_t, event) == 0);

STATIC_ASSERT(sizeof(adk_window_event_focus_changed_t) == 4);
STATIC_ASSERT(offsetof(adk_window_event_focus_changed_t, new_focus) == 0);

STATIC_ASSERT(sizeof(adk_window_event_t) == 16);
STATIC_ASSERT(offsetof(adk_window_event_t, window) == 0);
STATIC_ASSERT(offsetof(adk_window_event_t, event_data) == 8);

STATIC_ASSERT(sizeof(cg_color_t) == 16);
STATIC_ASSERT(offsetof(cg_color_t, r) == 0);
STATIC_ASSERT(offsetof(cg_color_t, g) == 4);
STATIC_ASSERT(offsetof(cg_color_t, b) == 8);
STATIC_ASSERT(offsetof(cg_color_t, a) == 12);

STATIC_ASSERT(sizeof(cg_font_metrics_t) == 20);
STATIC_ASSERT(offsetof(cg_font_metrics_t, bounds) == 0);
STATIC_ASSERT(offsetof(cg_font_metrics_t, baseline) == 16);

STATIC_ASSERT(sizeof(cg_margins_t) == 16);
STATIC_ASSERT(offsetof(cg_margins_t, left) == 0);
STATIC_ASSERT(offsetof(cg_margins_t, right) == 4);
STATIC_ASSERT(offsetof(cg_margins_t, top) == 8);
STATIC_ASSERT(offsetof(cg_margins_t, bottom) == 12);

STATIC_ASSERT(sizeof(cg_rads_t) == 4);
STATIC_ASSERT(offsetof(cg_rads_t, rads) == 0);

STATIC_ASSERT(sizeof(cg_rect_t) == 16);
STATIC_ASSERT(offsetof(cg_rect_t, x) == 0);
STATIC_ASSERT(offsetof(cg_rect_t, y) == 4);
STATIC_ASSERT(offsetof(cg_rect_t, width) == 8);
STATIC_ASSERT(offsetof(cg_rect_t, height) == 12);

STATIC_ASSERT(sizeof(cg_sdf_rect_params_t) == 28);
STATIC_ASSERT(offsetof(cg_sdf_rect_params_t, roundness) == 0);
STATIC_ASSERT(offsetof(cg_sdf_rect_params_t, fade) == 4);
STATIC_ASSERT(offsetof(cg_sdf_rect_params_t, border_color) == 8);
STATIC_ASSERT(offsetof(cg_sdf_rect_params_t, border_width) == 24);

STATIC_ASSERT(sizeof(cg_text_block_page_offsets_t) == 8);
STATIC_ASSERT(offsetof(cg_text_block_page_offsets_t, begin_offset) == 0);
STATIC_ASSERT(offsetof(cg_text_block_page_offsets_t, end_offset) == 4);

STATIC_ASSERT(sizeof(cg_vec2_t) == 8);
STATIC_ASSERT(offsetof(cg_vec2_t, x) == 0);
STATIC_ASSERT(offsetof(cg_vec2_t, y) == 4);

STATIC_ASSERT(sizeof(crypto_hmac_hex_t) == 65);
STATIC_ASSERT(offsetof(crypto_hmac_hex_t, bytes) == 0);

STATIC_ASSERT(sizeof(heap_metrics_t) == 96);
STATIC_ASSERT(offsetof(heap_metrics_t, alignment) == 0);
STATIC_ASSERT(offsetof(heap_metrics_t, ptr_ofs) == 8);
STATIC_ASSERT(offsetof(heap_metrics_t, header_size) == 16);
STATIC_ASSERT(offsetof(heap_metrics_t, extra_header_bytes) == 24);
STATIC_ASSERT(offsetof(heap_metrics_t, min_block_size) == 32);
STATIC_ASSERT(offsetof(heap_metrics_t, heap_size) == 40);
STATIC_ASSERT(offsetof(heap_metrics_t, num_used_blocks) == 48);
STATIC_ASSERT(offsetof(heap_metrics_t, num_free_blocks) == 56);
STATIC_ASSERT(offsetof(heap_metrics_t, used_block_size) == 64);
STATIC_ASSERT(offsetof(heap_metrics_t, free_block_size) == 72);
STATIC_ASSERT(offsetof(heap_metrics_t, num_merged_blocks) == 80);
STATIC_ASSERT(offsetof(heap_metrics_t, max_used_size) == 88);

STATIC_ASSERT(sizeof(json_deflate_parse_result_t) == 12);
STATIC_ASSERT(offsetof(json_deflate_parse_result_t, status) == 0);
STATIC_ASSERT(offsetof(json_deflate_parse_result_t, offset) == 4);
STATIC_ASSERT(offsetof(json_deflate_parse_result_t, end) == 8);

STATIC_ASSERT(sizeof(json_deflate_parse_status_e) == 4);

STATIC_ASSERT(sizeof(microseconds_t) == 8);
STATIC_ASSERT(offsetof(microseconds_t, us) == 0);

STATIC_ASSERT(sizeof(milliseconds_t) == 4);
STATIC_ASSERT(offsetof(milliseconds_t, ms) == 0);

STATIC_ASSERT(sizeof(nanoseconds_t) == 8);
STATIC_ASSERT(offsetof(nanoseconds_t, ns) == 0);

STATIC_ASSERT(sizeof(native_slice_t) == 16);
STATIC_ASSERT(offsetof(native_slice_t, ptr) == 0);
STATIC_ASSERT(offsetof(native_slice_t, size) == 8);

STATIC_ASSERT(sizeof(sb_uuid_t) == 16);
STATIC_ASSERT(offsetof(sb_uuid_t, id) == 0);

STATIC_ASSERT(sizeof(ptr64_t) == 8);
STATIC_ASSERT(offsetof(ptr64_t, ptr) == 0);
STATIC_ASSERT(offsetof(ptr64_t, adr) == 0);
STATIC_ASSERT(offsetof(ptr64_t, internal) == 0);

STATIC_ASSERT(sizeof(xoroshiro256plusplus_t) == 32);
STATIC_ASSERT(offsetof(xoroshiro256plusplus_t, s) == 0);

