/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

STATIC_ASSERT(sizeof(nve_abr_params_t) == 56);
STATIC_ASSERT(offsetof(nve_abr_params_t, initial_bitrate_bps) == 0);
STATIC_ASSERT(offsetof(nve_abr_params_t, min_bitrate_bps) == 4);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_bitrate_bps) == 8);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_avg_bitrate_bps) == 12);
STATIC_ASSERT(offsetof(nve_abr_params_t, min_trickplay_bitrate_bps) == 16);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_trickplay_bitrate_bps) == 20);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_trickplay_avg_bitrate_bps) == 24);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_trickplay_bandwidth_usage_bps) == 28);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_playout_rate) == 32);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_width) == 40);
STATIC_ASSERT(offsetof(nve_abr_params_t, max_height) == 44);
STATIC_ASSERT(offsetof(nve_abr_params_t, abr_policy) == 48);

STATIC_ASSERT(sizeof(nve_abr_policy_e) == 4);

STATIC_ASSERT(sizeof(nve_audio_capabilities_e) == 4);

STATIC_ASSERT(sizeof(nve_audio_codecs_e) == 4);

STATIC_ASSERT(sizeof(nve_audio_track_t) == 208);
STATIC_ASSERT(offsetof(nve_audio_track_t, id) == 0);
STATIC_ASSERT(offsetof(nve_audio_track_t, track_name) == 4);
STATIC_ASSERT(offsetof(nve_audio_track_t, track_language) == 68);
STATIC_ASSERT(offsetof(nve_audio_track_t, track_channels) == 132);
STATIC_ASSERT(offsetof(nve_audio_track_t, pid) == 196);
STATIC_ASSERT(offsetof(nve_audio_track_t, is_default) == 200);
STATIC_ASSERT(offsetof(nve_audio_track_t, is_auto_select) == 204);

STATIC_ASSERT(sizeof(nve_buffer_params_t) == 16);
STATIC_ASSERT(offsetof(nve_buffer_params_t, initial_buffer_time) == 0);
STATIC_ASSERT(offsetof(nve_buffer_params_t, play_buffer_time) == 8);

STATIC_ASSERT(sizeof(nve_capabilities_t) == 88);
STATIC_ASSERT(offsetof(nve_capabilities_t, dolby_atmos) == 0);
STATIC_ASSERT(offsetof(nve_capabilities_t, dolby_vision) == 4);
STATIC_ASSERT(offsetof(nve_capabilities_t, dolby_digital) == 8);
STATIC_ASSERT(offsetof(nve_capabilities_t, hdr10) == 12);
STATIC_ASSERT(offsetof(nve_capabilities_t, h265) == 16);
STATIC_ASSERT(offsetof(nve_capabilities_t, hdcp) == 20);
STATIC_ASSERT(offsetof(nve_capabilities_t, hdmi_capabilities) == 24);
STATIC_ASSERT(offsetof(nve_capabilities_t, decoder_capabilities) == 52);

STATIC_ASSERT(sizeof(nve_capability_flag_e) == 4);

STATIC_ASSERT(sizeof(nve_color_e) == 4);

STATIC_ASSERT(sizeof(nve_drm_e) == 4);

STATIC_ASSERT(sizeof(nve_drm_error_payload_t) == 2048);
STATIC_ASSERT(offsetof(nve_drm_error_payload_t, error_string) == 0);
STATIC_ASSERT(offsetof(nve_drm_error_payload_t, error_server_url) == 1024);

STATIC_ASSERT(sizeof(nve_drm_init_payload_t) == 280);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, init_type) == 0);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, core_player_context) == 8);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, app_storage_path) == 16);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, publisher_id) == 80);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, app_id) == 144);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, app_version) == 208);
STATIC_ASSERT(offsetof(nve_drm_init_payload_t, privacy_mode) == 272);

STATIC_ASSERT(sizeof(nve_drm_key_tag_e) == 4);

STATIC_ASSERT(sizeof(nve_drm_method_e) == 4);

STATIC_ASSERT(sizeof(nve_drm_workflow_init_type_e) == 4);

STATIC_ASSERT(sizeof(nve_event_payload_t) == 24);
STATIC_ASSERT(offsetof(nve_event_payload_t, event_type) == 0);
STATIC_ASSERT(offsetof(nve_event_payload_t, f_arg0) == 4);
STATIC_ASSERT(offsetof(nve_event_payload_t, f_arg1) == 8);
STATIC_ASSERT(offsetof(nve_event_payload_t, f_arg2) == 12);
STATIC_ASSERT(offsetof(nve_event_payload_t, event_payload_ptr) == 16);

STATIC_ASSERT(sizeof(nve_font_e) == 4);

STATIC_ASSERT(sizeof(nve_font_edge_e) == 4);

STATIC_ASSERT(sizeof(nve_load_information_t) == 760);
STATIC_ASSERT(offsetof(nve_load_information_t, period_index) == 0);
STATIC_ASSERT(offsetof(nve_load_information_t, size) == 4);
STATIC_ASSERT(offsetof(nve_load_information_t, track_index) == 8);
STATIC_ASSERT(offsetof(nve_load_information_t, http_status) == 12);
STATIC_ASSERT(offsetof(nve_load_information_t, latency) == 16);
STATIC_ASSERT(offsetof(nve_load_information_t, error_code) == 20);
STATIC_ASSERT(offsetof(nve_load_information_t, playlist_complete) == 24);
STATIC_ASSERT(offsetof(nve_load_information_t, playlist_type) == 28);
STATIC_ASSERT(offsetof(nve_load_information_t, track_name) == 32);
STATIC_ASSERT(offsetof(nve_load_information_t, track_type) == 96);
STATIC_ASSERT(offsetof(nve_load_information_t, url) == 160);
STATIC_ASSERT(offsetof(nve_load_information_t, profile_id) == 672);
STATIC_ASSERT(offsetof(nve_load_information_t, profile_width) == 676);
STATIC_ASSERT(offsetof(nve_load_information_t, profile_height) == 680);
STATIC_ASSERT(offsetof(nve_load_information_t, profile_peak_bit_rate) == 684);
STATIC_ASSERT(offsetof(nve_load_information_t, profile_avg_bit_rate) == 688);
STATIC_ASSERT(offsetof(nve_load_information_t, profile_framerate) == 692);
STATIC_ASSERT(offsetof(nve_load_information_t, x_request_id) == 696);

STATIC_ASSERT(sizeof(nve_media_audio_capabilities_flag_e) == 4);

STATIC_ASSERT(sizeof(nve_media_color_depth_flag_e) == 4);

STATIC_ASSERT(sizeof(nve_media_color_format_flag_e) == 4);

STATIC_ASSERT(sizeof(nve_media_color_space_flag_e) == 4);

STATIC_ASSERT(sizeof(nve_media_decoder_capabilities_t) == 36);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, audio_codecs) == 0);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, profile) == 4);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, level) == 8);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, dynamic_range) == 12);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, video_output_resolution) == 16);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, video_output_rate) == 20);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, drm) == 24);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, encryption) == 28);
STATIC_ASSERT(offsetof(nve_media_decoder_capabilities_t, hdcp_level) == 32);

STATIC_ASSERT(sizeof(nve_media_encryption_e) == 4);

STATIC_ASSERT(sizeof(nve_media_hdcp_level_e) == 4);

STATIC_ASSERT(sizeof(nve_media_hdmi_capabilities_t) == 28);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, audio_capabilities) == 0);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, video_output_resolution) == 4);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, video_output_rate) == 8);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, color_format) == 12);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, color_depth) == 16);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, color_space) == 20);
STATIC_ASSERT(offsetof(nve_media_hdmi_capabilities_t, hdcp_level) == 24);

STATIC_ASSERT(sizeof(nve_media_video_output_rate_e) == 4);

STATIC_ASSERT(sizeof(nve_media_video_output_resolution_e) == 4);

STATIC_ASSERT(sizeof(nve_metadata_key_value_t) == 2048);
STATIC_ASSERT(offsetof(nve_metadata_key_value_t, key) == 0);
STATIC_ASSERT(offsetof(nve_metadata_key_value_t, value) == 1024);

STATIC_ASSERT(sizeof(nve_network_configuration_t) == 576);
STATIC_ASSERT(offsetof(nve_network_configuration_t, use_cookie_header_for_all_requests) == 0);
STATIC_ASSERT(offsetof(nve_network_configuration_t, force_native_networking) == 1);
STATIC_ASSERT(offsetof(nve_network_configuration_t, force_https) == 2);
STATIC_ASSERT(offsetof(nve_network_configuration_t, read_set_cookie_header) == 3);
STATIC_ASSERT(offsetof(nve_network_configuration_t, use_redirect_domain) == 4);
STATIC_ASSERT(offsetof(nve_network_configuration_t, keep_alive) == 5);
STATIC_ASSERT(offsetof(nve_network_configuration_t, use_parent_manifest_query_params) == 6);
STATIC_ASSERT(offsetof(nve_network_configuration_t, account_for_buffer_length_to_calc_live_point) == 7);
STATIC_ASSERT(offsetof(nve_network_configuration_t, disable_ssl_certificate) == 8);
STATIC_ASSERT(offsetof(nve_network_configuration_t, offline_playback) == 9);
STATIC_ASSERT(offsetof(nve_network_configuration_t, custom_user_agent) == 10);
STATIC_ASSERT(offsetof(nve_network_configuration_t, network_down_url) == 266);
STATIC_ASSERT(offsetof(nve_network_configuration_t, ad_manifest_timeout) == 524);
STATIC_ASSERT(offsetof(nve_network_configuration_t, number_of_manifest_retry_before_error) == 528);
STATIC_ASSERT(offsetof(nve_network_configuration_t, number_of_map_retry_before_error) == 532);
STATIC_ASSERT(offsetof(nve_network_configuration_t, number_of_vtt_retry_before_error) == 536);
STATIC_ASSERT(offsetof(nve_network_configuration_t, number_of_audio_segments_retry_before_error) == 540);
STATIC_ASSERT(offsetof(nve_network_configuration_t, number_of_video_segments_retry_before_error) == 544);
STATIC_ASSERT(offsetof(nve_network_configuration_t, number_of_segments_from_live_point) == 548);
STATIC_ASSERT(offsetof(nve_network_configuration_t, first_byte_timeout_ms) == 552);
STATIC_ASSERT(offsetof(nve_network_configuration_t, file_timeout_ms) == 556);
STATIC_ASSERT(offsetof(nve_network_configuration_t, cookie_headers_metadata_ptr) == 560);
STATIC_ASSERT(offsetof(nve_network_configuration_t, custom_headers_metadata_ptr) == 568);

STATIC_ASSERT(sizeof(nve_network_request_error_data_t) == 256);
STATIC_ASSERT(offsetof(nve_network_request_error_data_t, error_text) == 0);

STATIC_ASSERT(sizeof(nve_profile_t) == 24);
STATIC_ASSERT(offsetof(nve_profile_t, id) == 0);
STATIC_ASSERT(offsetof(nve_profile_t, width) == 4);
STATIC_ASSERT(offsetof(nve_profile_t, height) == 8);
STATIC_ASSERT(offsetof(nve_profile_t, peak_bit_rate) == 12);
STATIC_ASSERT(offsetof(nve_profile_t, avg_bit_rate) == 16);
STATIC_ASSERT(offsetof(nve_profile_t, framerate) == 20);

STATIC_ASSERT(sizeof(nve_range_t) == 16);
STATIC_ASSERT(offsetof(nve_range_t, begin) == 0);
STATIC_ASSERT(offsetof(nve_range_t, end) == 8);

STATIC_ASSERT(sizeof(nve_size_e) == 4);

STATIC_ASSERT(sizeof(nve_text_format_t) == 76);
STATIC_ASSERT(offsetof(nve_text_format_t, font) == 0);
STATIC_ASSERT(offsetof(nve_text_format_t, size) == 4);
STATIC_ASSERT(offsetof(nve_text_format_t, font_edge) == 8);
STATIC_ASSERT(offsetof(nve_text_format_t, font_color) == 12);
STATIC_ASSERT(offsetof(nve_text_format_t, background_color) == 16);
STATIC_ASSERT(offsetof(nve_text_format_t, fill_color) == 20);
STATIC_ASSERT(offsetof(nve_text_format_t, edge_color) == 24);
STATIC_ASSERT(offsetof(nve_text_format_t, font_opacity) == 28);
STATIC_ASSERT(offsetof(nve_text_format_t, background_opacity) == 32);
STATIC_ASSERT(offsetof(nve_text_format_t, fill_opacity) == 36);
STATIC_ASSERT(offsetof(nve_text_format_t, treat_space_as_alpha_num) == 40);
STATIC_ASSERT(offsetof(nve_text_format_t, bottom_inset) == 44);
STATIC_ASSERT(offsetof(nve_text_format_t, safe_area) == 60);

STATIC_ASSERT(sizeof(nve_text_track_t) == 144);
STATIC_ASSERT(offsetof(nve_text_track_t, id) == 0);
STATIC_ASSERT(offsetof(nve_text_track_t, track_name) == 4);
STATIC_ASSERT(offsetof(nve_text_track_t, track_language) == 68);
STATIC_ASSERT(offsetof(nve_text_track_t, is_forced) == 132);
STATIC_ASSERT(offsetof(nve_text_track_t, is_default) == 136);
STATIC_ASSERT(offsetof(nve_text_track_t, is_auto_select) == 140);

STATIC_ASSERT(sizeof(nve_timed_metadata_tag_t) == 64);
STATIC_ASSERT(offsetof(nve_timed_metadata_tag_t, tag) == 0);

STATIC_ASSERT(sizeof(nve_version_t) == 84);
STATIC_ASSERT(offsetof(nve_version_t, version) == 0);
STATIC_ASSERT(offsetof(nve_version_t, major) == 64);
STATIC_ASSERT(offsetof(nve_version_t, minor) == 68);
STATIC_ASSERT(offsetof(nve_version_t, revision) == 72);
STATIC_ASSERT(offsetof(nve_version_t, revision_minor) == 76);
STATIC_ASSERT(offsetof(nve_version_t, api_version) == 80);

STATIC_ASSERT(sizeof(nve_video_dynamic_range_e) == 4);

STATIC_ASSERT(sizeof(nve_video_level_e) == 4);

STATIC_ASSERT(sizeof(nve_video_profile_e) == 4);

