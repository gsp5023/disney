/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_media_stub.c

Stubs for minimum required methods to implement NVE playback
*/

#include "sb_media_decoder.h"

sb_media_result_t sb_media_global_init()
{
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_global_shutdown()
{
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_init_audio_config(sb_media_audio_config_t * config)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_init_audio_decoder(sb_media_audio_config_t * config, sb_media_decoder_handle handle)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_init_video_config(sb_media_video_config_t * config)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_init_video_decoder(sb_media_video_config_t * config, sb_media_decoder_handle handle)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_decode(sb_media_decoder_handle handle, uint8_t * pes, uint32_t size, sb_media_timestamp_t pts, sb_media_output_buffer_t * out, sb_media_decrypt_t * key)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_set_video_output_rectangle(sb_media_decoder_handle handle, const sb_media_rect_t * rect)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_reset_decoder(sb_media_decoder_handle handle)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_get_time(sb_media_decoder_handle handle, sb_media_timestamp_t * presentation_time)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_set_drm(sb_media_drm_flag_t drm)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_generate_challenge(const uint8_t * object, const uint16_t object_size, sb_media_challenge_callback callback)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_process_key_message_response(const uint8_t * response, const uint16_t response_size)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_get_decoder_capabilities(sb_media_decoder_capabilities_t * capabilities)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_set_event_callback(sb_media_event_callback callback)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_get_stats(sb_media_statistics_t * stats)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_get_decoder_stats(sb_media_decoder_handle handle, sb_media_decoder_stats_t * stats)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_set_playback_rate(sb_media_decoder_handle handle, int8_t playback_rate)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_set_drm_certificate(const uint8_t *certificate, uint32_t size)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_audio_set_volume(const float volume)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_get_hdmi_capabilities(sb_media_hdmi_capabilities_t *capabilities)
{
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}
