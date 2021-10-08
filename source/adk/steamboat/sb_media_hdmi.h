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
/**
 * @file sb_media_hdmi.h
 * @author Brian Hadwen (brian.hadwen@disneystreaming.com)
 * @date 27 May 2020
 * @brief Header file for steamboat HDMI
 *
 * This is the header for the Steamboat HDMI.
 */

#ifndef SB_MEDIA_HDMI_H
#define SB_MEDIA_HDMI_H

#include <stdint.h>
#include "sb_media_result_codes.h"

/** Audio Capabilities Flag */
typedef uint8_t sb_media_audio_capabilities_flag_t;
#define SB_MEDIA_AUDIO_LPCM                      (uint8_t) 0x01
#define SB_MEDIA_AUDIO_DOLBY_DIGITAL             (uint8_t) 0x02
#define SB_MEDIA_AUDIO_DOLBY_ATMOS               (uint8_t) 0x04
#define SB_MEDIA_AUDIO_LAST                      (uint8_t) 0x08

/* Video output resolution */
typedef uint16_t sb_media_video_output_resolution_t;
#define SB_MEDIA_PLAYER_VIDEO_OUT_NONE     (uint16_t) 0x0000 /**< No Video Output */
#define SB_MEDIA_PLAYER_VIDEO_OUT_SD       (uint16_t) 0x0001 /**< Standard Definition */
#define SB_MEDIA_PLAYER_VIDEO_OUT_720P     (uint16_t) 0x0002 /**< 1280 x 720 */
#define SB_MEDIA_PLAYER_VIDEO_OUT_1080P    (uint16_t) 0x0004 /**< 1920 x 1080 */
#define SB_MEDIA_PLAYER_VIDEO_OUT_1440P    (uint16_t) 0x0008 /**< 2560 x 1440 */
#define SB_MEDIA_PLAYER_VIDEO_OUT_4K       (uint16_t) 0x0010 /**< 3840 x 2160 */
#define SB_MEDIA_PLAYER_VIDEO_OUT_5K       (uint16_t) 0x0020 /**< 5120 x 2880 */
#define SB_MEDIA_PLAYER_VIDEO_OUT_8K       (uint16_t) 0x0040 /**< 7680 x 4320 */
#define SB_MEDIA_PLAYER_VIDEO_OUT_OTHER    (uint16_t) 0x0080 /**< Other Resolution */
#define SB_MEDIA_PLAYER_VIDEO_OUT_LAST     (uint16_t) 0x0100 /**< Last Video Output Resolution Enumeration */

/* Video output rates */
typedef uint16_t sb_media_video_output_rate_t;
#define SB_MEDIA_PLAYER_VIDEO_RATE_NONE          (uint16_t) 0x0000 /**< No Video Output */
#define SB_MEDIA_PLAYER_VIDEO_RATE_23_98_OR_24   (uint16_t) 0x0001 /**< 23.98Hz/24Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_25            (uint16_t) 0x0002 /**< 25Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_29_97_OR_30   (uint16_t) 0x0004 /**< 29.97Hz/30Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_50            (uint16_t) 0x0008 /**< 50Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_59_94_OR_60   (uint16_t) 0x0010 /**< 59.94Hz/60Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_100           (uint16_t) 0x0020 /**< 100Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_119_98_OR_120 (uint16_t) 0x0040 /**< 119.88/120Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_200           (uint16_t) 0x0080 /**< 200Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_239_76_OR_240 (uint16_t) 0x0100 /**< 239.76/240Hz */
#define SB_MEDIA_PLAYER_VIDEO_RATE_OTHER         (uint16_t) 0x0200 /**< Other Rate */
#define SB_MEDIA_PLAYER_VIDEO_RATE_LAST          (uint16_t) 0x0400 /**< Last Video Output Resolution Enumeration */

/** Color Format Support Flag */
typedef uint8_t sb_media_color_format_flag_t;
#define SB_MEDIA_COLOR_FORMAT_4_2_0        (uint8_t) 0x01 /**< YCrCb 4:2:0, only required support */
#define SB_MEDIA_COLOR_FORMAT_LAST         (uint8_t) 0x02 /**< Last Color Format Flag */

/** Color Depth Flag */
typedef uint8_t sb_media_color_depth_flag_t;
#define SB_MEDIA_COLOR_DEPTH_8BPC      (uint8_t) 0x01 /**< 8 bits/pixel/color 24 bits/pixel) */
#define SB_MEDIA_COLOR_DEPTH_10BPC     (uint8_t) 0x02 /**< 10 bits/pixel/color 30 bits/pixel) */
#define SB_MEDIA_COLOR_DEPTH_12BPC     (uint8_t) 0x04 /**< 12 bits/pixel/color 36 bits/pixel) */
#define SB_MEDIA_COLOR_DEPTH_LAST      (uint8_t) 0x08 /**< Last Color Depth Flag */

/** Color Space Flag */
typedef uint8_t sb_media_color_space_flag_t;
#define SB_MEDIA_COLOR_SPACE_ITU_R_BT601          (uint8_t) 0x01 /**< ITU_R BT.601 */
#define SB_MEDIA_COLOR_SPACE_ITU_R_BT709          (uint8_t) 0x02 /**< ITU-R BT.709 */
#define SB_MEDIA_COLOR_SPACE_SRGB                 (uint8_t) 0x04 /**< sRGB */
#define SB_MEDIA_COLOR_SPACE_ITU_R_BT2020         (uint8_t) 0x08 /**< ITU-R BT.2020 */
#define SB_MEDIA_COLOR_SPACE_LAST                 (uint8_t) 0x10 /**< Last Color Space Flag */

/** HDCP Level */
typedef uint8_t sb_media_hdcp_level_t;
#define SB_MEDIA_HDCP_NONE      (uint8_t) 0x00 /**< HDCP Not Supported */
#define SB_MEDIA_HDCP_1_0       (uint8_t) 0x01 /**< HDCP 1.0 Supported */
#define SB_MEDIA_HDCP_1_1       (uint8_t) 0x02 /**< HDCP 1.1 Supported */
#define SB_MEDIA_HDCP_1_2       (uint8_t) 0x03 /**< HDCP 1.2 Supported */
#define SB_MEDIA_HDCP_1_3       (uint8_t) 0x04 /**< HDCP 1.4 Supported */
#define SB_MEDIA_HDCP_1_4       (uint8_t) 0x05 /**< HDCP 1.0 Supported */
#define SB_MEDIA_HDCP_2_0       (uint8_t) 0x06 /**< HDCP 2.0 Supported */
#define SB_MEDIA_HDCP_2_1       (uint8_t) 0x07 /**< HDCP 2.1 Supported */
#define SB_MEDIA_HDCP_2_2       (uint8_t) 0x08 /**< HDCP 2.2 Supported */
#define SB_MEDIA_HDCP_2_3       (uint8_t) 0x09 /**< HDCP 2.3 Supported */
#define SB_MEDIA_HDCP_LAST      (uint8_t) 0x0A /**< Last HDCP Level */

/** Structure for for capabilities SHOULD NOT BE CHANGED
 *  TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_hdmi_capabilities_t {
    sb_media_audio_capabilities_flag_t audio_capabilities;          /**< Audio Capabilities */
    sb_media_video_output_resolution_t video_output_resolution;     /**< Output Video Resolutions */
    sb_media_video_output_rate_t       video_output_rate;           /**< Output Video Rates */
    sb_media_color_format_flag_t       color_format;                /**< Supported Color Format Capabilities */
    sb_media_color_depth_flag_t        color_depth;                 /**< Color Depth Capabilities */
    sb_media_color_space_flag_t        color_space;                 /**< Color Depth Capabilities */
    sb_media_hdcp_level_t              hdcp_level;                  /**< HDCP Levels */
    uint8_t                            unused[3];                   /**< alignment */
}sb_media_hdmi_capabilities_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Get HDMI Capabilities
 *
 * @param[in]  capabilities - pointer to HDMI capabilities (hdmi_capabilities_t)
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_get_hdmi_capabilities(sb_media_hdmi_capabilities_t *capabilities);

#ifdef __cplusplus
}
#endif

#endif /* SB_MEDIA_HDMI_H */
