/****************************************************************************
 * Copyright (c) 2020 Disney
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __NVE_CAPABILITIES_H__
#define __NVE_CAPABILITIES_H__

#include "nve_defines.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_media_video_output_resolution_e {
    nve_media_video_output_resolution_none = 0x0000,
    nve_media_video_output_resolution_sd = 0x0001,
    nve_media_video_output_resolution_720p = 0x0002,
    nve_media_video_output_resolution_1080p = 0x0004,
    nve_media_video_output_resolution_1440p = 0x0008,
    nve_media_video_output_resolution_4k = 0x0010,
    nve_media_video_output_resolution_5k = 0x0020,
    nve_media_video_output_resolution_8k = 0x0040,
    nve_media_video_output_resolution_other = 0x0080,
    nve_media_video_output_resolution_last = 0x0100,
    FORCE_ENUM_INT32(nve_media_video_output_resolution_e)
} nve_media_video_output_resolution_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_media_video_output_rate_e {
    nve_media_video_output_rate_none = 0x0000,
    nve_media_video_output_rate_23_98_or_24 = 0x0001,
    nve_media_video_output_rate_25 = 0x0002,
    nve_media_video_output_rate_29_97_or_30 = 0x0004,
    nve_media_video_output_rate_50 = 0x0008,
    nve_media_video_output_rate_59_94_or_60 = 0x0010,
    nve_media_video_output_rate_100 = 0x0020,
    nve_media_video_output_rate_119_98_or_120 = 0x0040,
    nve_media_video_output_rate_200 = 0x0080,
    nve_media_video_output_rate_239_76_or_240 = 0x0100,
    nve_media_video_output_rate_other = 0x0200,
    nve_media_video_output_rate_last = 0x0400,
    FORCE_ENUM_INT32(nve_media_video_output_rate_e)
} nve_media_video_output_rate_e;

// Decoder Capabilities
// see dss-nve/code/third_party/dss-nve-shared/steamboat/include/sb_media_decoder.h

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_audio_codecs_e {
    nve_audio_codecs_none  = 0x00,
    nve_audio_codecs_eac3  = 0x01,
    nve_audio_codecs_aac   = 0x02,
    nve_audio_codecs_ac3   = 0x04,
    nve_audio_codecs_atmos = 0x08,
    FORCE_ENUM_INT32(nve_audio_codecs_e)
} nve_audio_codecs_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_video_dynamic_range_e {
    nve_video_dynamic_range_standard = 0x00,
    nve_video_dynamic_range_hdr10 = 0x01,
    nve_video_dynamic_range_dolby_version_5 = 0x02,
    FORCE_ENUM_INT32(nve_video_dynamic_range_e)
} nve_video_dynamic_range_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_video_profile_e {
    nve_video_profile_none = 0x00,
    nve_video_profile_h264_main = 0x01,
    nve_video_profile_h264_high = 0x02,
    nve_video_profile_hevc_main = 0x04,
    nve_video_profile_hevc_main_10 = 0x08,
    FORCE_ENUM_INT32(nve_video_profile_e)
} nve_video_profile_e;

FFI_EXPORT
typedef enum nve_video_level_e {
    nve_video_level_none = 0x00,
    nve_video_level_3_1 = 0x01,
    nve_video_level_4 = 0x02,
    FORCE_ENUM_INT32(nve_video_level_e)
} nve_video_level_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_drm_e {
    nve_drm_none      = 0x00,
    nve_drm_playready = 0x01,
    nve_drm_widevine  = 0x02,
    nve_drm_nagra     = 0x04,
    FORCE_ENUM_INT32(nve_drm_e)
} nve_drm_e;

FFI_EXPORT 
FFI_ENUM_BITFLAGS
typedef enum nve_media_encryption_e {
    nve_media_encryption_none = 0x00,
    nve_media_encryption_ctr = 0x01,
    nve_media_encryption_cbcs = 0x02,
    FORCE_ENUM_INT32(nve_media_encryption_e)
} nve_media_encryption_e;

FFI_EXPORT
typedef enum nve_media_hdcp_level_e {
    nve_media_hdcp_level_none = 0x00,
    nve_media_hdcp_level_1_0 = 0x01,
    nve_media_hdcp_level_1_1 = 0x02,
    nve_media_hdcp_level_1_2 = 0x03,
    nve_media_hdcp_level_1_3 = 0x04,
    nve_media_hdcp_level_1_4 = 0x05,
    nve_media_hdcp_level_2_0 = 0x06,
    nve_media_hdcp_level_2_1 = 0x07,
    nve_media_hdcp_level_2_2 = 0x08,
    nve_media_hdcp_level_2_3 = 0x09,
    nve_media_hdcp_level_last = 0x0A,
    FORCE_ENUM_INT32(nve_media_hdcp_level_e)
} nve_media_hdcp_level_e;

FFI_EXPORT typedef struct {
    nve_audio_codecs_e audio_codecs;
    nve_video_profile_e profile;
    nve_video_level_e level;
    nve_video_dynamic_range_e dynamic_range;
    nve_media_video_output_resolution_e video_output_resolution;
    nve_media_video_output_rate_e video_output_rate;
    nve_drm_e drm;
    nve_media_encryption_e encryption;
    nve_media_hdcp_level_e hdcp_level;
} nve_media_decoder_capabilities_t;

// HDMI Capabilities
// see dss-nve/code/third_party/dss-nve-shared/steamboat/include/sb_media_hdmi.h

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_audio_capabilities_e {
    nve_audio_capabilities_none = 0x00,
    nve_audio_capabilities_lpcm = 0x01,
    nve_audio_capabilities_dolby_digital = 0x02,
    nve_audio_capabilities_dolby_atmos = 0x04,
    nve_audio_capabilities_last = 0x08,
    FORCE_ENUM_INT32(nve_audio_capabilities_e)
} nve_audio_capabilities_e;

FFI_EXPORT 
FFI_ENUM_BITFLAGS
typedef enum {
    nve_media_color_space_flag_none = 0x00,
    nve_media_color_space_flag_ITU_R_BT601 = 0x01,
    nve_media_color_space_flag_ITU_R_BT709 = 0x02,
    nve_media_color_space_flag_SRGB = 0x04,
    nve_media_color_space_flag_ITU_R_BT2020 = 0x08,
    nve_media_color_space_flag_last = 0x10,
    FORCE_ENUM_INT32(nve_media_color_space_flag_e)
} nve_media_color_space_flag_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum {
    nve_media_color_depth_flag_none = 0x00,
    nve_media_color_depth_flag_8bpc = 0x01,
    nve_media_color_depth_flag_10bpc = 0x02,
    nve_media_color_depth_flag_12bpc = 0x04,
    nve_media_color_depth_flag_last = 0x08,
    FORCE_ENUM_INT32(nve_media_color_depth_flag_e)
} nve_media_color_depth_flag_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum {
    nve_media_color_format_flag_none = 0x00,
    nve_media_color_format_flag_4_2_0 = 0x01,
    nve_media_color_format_flag_last = 0x02,
    FORCE_ENUM_INT32(nve_media_color_format_flag_e)
} nve_media_color_format_flag_e;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum {
    nve_media_audio_capabilities_flag_none = 0x00,
    nve_media_audio_capabilities_flag_lpcm = 0x01,
    nve_media_audio_capabilities_flag_dolby_digital = 0x02,
    nve_media_audio_capabilities_flag_dolby_atmos = 0x04,
    nve_media_audio_capabilities_flag_last = 0x08,
    FORCE_ENUM_INT32(nve_media_audio_capabilities_flag_e)
} nve_media_audio_capabilities_flag_e;

FFI_EXPORT typedef struct {
    nve_media_audio_capabilities_flag_e audio_capabilities;
    nve_media_video_output_resolution_e video_output_resolution;
    nve_media_video_output_rate_e video_output_rate;
    nve_media_color_format_flag_e color_format;
    nve_media_color_depth_flag_e color_depth;
    nve_media_color_space_flag_e color_space;
    nve_media_hdcp_level_e hdcp_level;
} nve_media_hdmi_capabilities_t;

FFI_EXPORT
FFI_ENUM_BITFLAGS
typedef enum nve_capability_flag_e {
    nve_capability_no_support        = 0x00,
    nve_capability_decoder_support   = 0x01,
    nve_capability_hardware_support  = 0x02,
    FORCE_ENUM_INT32(nve_capability_flag_e)
} nve_capability_flag_e;

FFI_EXPORT typedef struct {
    nve_capability_flag_e dolby_atmos;
    nve_capability_flag_e dolby_vision;
    nve_capability_flag_e dolby_digital;
    nve_capability_flag_e hdr10;
    nve_capability_flag_e h265;
    nve_capability_flag_e hdcp;
    nve_media_hdmi_capabilities_t hdmi_capabilities;
    nve_media_decoder_capabilities_t decoder_capabilities;
} nve_capabilities_t;

FFI_EXPORT_NVE nve_result_e nve_capabilities_get(FFI_PTR_WASM nve_capabilities_t * caps);

#ifdef __cplusplus
}
#endif

#endif // __NVE_CAPABILITIES_H__
