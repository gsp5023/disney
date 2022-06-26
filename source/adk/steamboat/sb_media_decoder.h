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
 * @file sb_media_decoder.h
 * @author Brian Hadwen (brian.hadwen@disneystreaming.com)
 * @date 27 May 2020
 * @brief Header file for steamboat player
 *
 * This is the header for the Steamboat Player.
 * The Steamboat player is a higher level API that allows a player to play from
 * a URL
 */

#ifndef SB_MEDIA_DECODER_H
#define SB_MEDIA_DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include "sb_media_result_codes.h"
#include "sb_media_hdmi.h"

/** Timestamp is in nanosecond units */
typedef uint64_t sb_media_timestamp_t;
#define SB_MEDIA_INVALID_PTS UINT64_MAX

typedef int32_t sb_media_decoder_handle_t;
typedef sb_media_decoder_handle_t *sb_media_decoder_handle;

typedef void (*sb_media_callback)(sb_media_decoder_handle);
#define SB_MEDIA_INVALID_HANDLE             (sb_media_decoder_handle_t) (-1)    /**< No Decoder */

/** Audio Codec, only required audio codecs are defined, bit mapped */
typedef uint8_t sb_media_audio_codec_t;
#define SB_MEDIA_AUDIO_CODEC_NONE                   (uint8_t) 0x00      /**< No Audio */
#define SB_MEDIA_AUDIO_CODEC_EAC3                   (uint8_t) 0x01      /**< EAC-3 Audio */
#define SB_MEDIA_AUDIO_CODEC_AAC                    (uint8_t) 0x02      /**< AAC Audio */
#define SB_MEDIA_AUDIO_CODEC_AC3                    (uint8_t) 0x04      /**< AC3 Audio */
#define SB_MEDIA_AUDIO_CODEC_ATMOS                  (uint8_t) 0x08      /**< ATMOS Audio */

/** Audio Output Sample Rate, only required sample rates are defined, bit mapped */
typedef uint8_t sb_media_audio_sample_rate_t;
#define SB_MEDIA_AUDIO_SAMPLE_32                     (uint8_t) 0x00     /**< 32kSamples/sec */
#define SB_MEDIA_AUDIO_SAMPLE_44_1                   (uint8_t) 0x01     /**< 44.1kSamples/sec */
#define SB_MEDIA_AUDIO_SAMPLE_48                     (uint8_t) 0x02     /**< 48kSamples/sec */
#define SB_MEDIA_AUDIO_SAMPLE_96                     (uint8_t) 0x04     /**< 96kSamples/sec */

/** Video Codec, only required video codecs are defined, bit mapped */
typedef uint8_t sb_media_video_codec_t;
#define SB_MEDIA_VIDEO_CODEC_NONE                    (uint8_t) 0x00    /**< No Video */
#define SB_MEDIA_VIDEO_CODEC_H264                    (uint8_t) 0x01  /**< H264 Video Codec */
#define SB_MEDIA_VIDEO_CODEC_HEVC                    (uint8_t) 0x02  /**< H265/HEVC Video Codec */

/** Video Dynamic range, only required video dynamic ranges are defined, bit mapped */
typedef uint8_t sb_media_video_dynamic_range_t;
#define SB_MEDIA_VIDEO_DYNAMIC_RANGE_STANDARD        (uint8_t) 0x00  /**< Standard Dynamic Range */
#define SB_MEDIA_VIDEO_DYNAMIC_RANGE_HDR10           (uint8_t) 0x01  /**< HDR10 */
#define SB_MEDIA_VIDEO_DYNAMIC_RANGE_DOLBY_VERSION_5 (uint8_t) 0x02  /**< Dolby Vision Version 5 and higher support */

/** Video Profile, only required video profiles are defined, bit mapped */
typedef uint8_t sb_media_video_profile_t;
#define SB_MEDIA_PROFILE_H264_MAIN                   (uint8_t)0x01    /**< Main */
#define SB_MEDIA_PROFILE_H264_HIGH                   (uint8_t)0x02    /**< High */
#define SB_MEDIA_PROFILE_HEVC_MAIN                   (uint8_t)0x04    /**< Main */
#define SB_MEDIA_PROFILE_HEVC_MAIN_10                (uint8_t)0x08    /**< Main 10 */

/** Video Level, only required video levels are defined, bit mapped */
typedef uint8_t sb_media_video_level_t; 
#define SB_MEDIA_LEVEL_3_1                           (uint8_t)0x01    /**< Level 3.1 */
#define SB_MEDIA_LEVEL_4                             (uint8_t)0x02    /**< Level 4 */

/** Supported DRM, only required DRM is defined, bit mapped */
typedef uint8_t sb_media_drm_flag_t;
#define SB_MEDIA_DRM_NONE                            (uint8_t) 0x00  /**< No DRM Available */
#define SB_MEDIA_DRM_PLAYREADY                       (uint8_t) 0x01  /**< PlayReady Available */
#define SB_MEDIA_DRM_WIDEVINE                        (uint8_t) 0x02  /**< Widevine Availbable */
#define SB_MEDIA_DRM_NAGRA                           (uint8_t) 0x04  /**< Nagra Availbable */

/** Encryption for AES-128, only these are required, bit mapped */
typedef uint8_t sb_media_encryption_t;
#define SB_MEDIA_ENCRYPTION_CTR                      (uint8_t) 0x01  /**< Counter mode */
#define SB_MEDIA_ENCRYPTION_CBCS                     (uint8_t) 0x02  /**< CBCS */

/** Steamboat Media Events */
typedef uint8_t sb_media_dss_event_t;
#define SB_MEDIA_PLAY_EVENT_NONE                    (uint8_t)0         /**< No Event */
#define SB_MEDIA_PLAY_EVENT_STATE_CHANGE            (uint8_t)1         /**< Change in Player state */
#define SB_MEDIA_PLAY_EVENT_EOF                     (uint8_t)2         /**< Player has reached end of file */
#define SB_MEDIA_PLAY_EVENT_REQUEST_KEY             (uint8_t)3         /**< When DRM challenge is disabled, this requests a key */
#define SB_MEDIA_PLAY_EVENT_UNDERFLOW               (uint8_t)4         /** Buffer Underflow */
#define SB_MEDIA_PLAY_EVENT_OVERFLOW                (uint8_t)5         /** Buffer Overflow */
#define SB_MEDIA_PLAY_EVENT_BUFFER_LOW              (uint8_t)6         /** Buffer Low */
#define SB_MEDIA_PLAY_EVENT_BUFFER_HIGH             (uint8_t)7         /** Buffer High */
#define SB_MEDIA_PLAY_EVENT_DIMENSION_CHANGE        (uint8_t)8         /** Buffer High */
#define SB_MEDIA_PLAY_EVENT_LAST                    (uint8_t)9         /** Last Player Event Enumeration */

typedef void (*sb_media_event_callback)(sb_media_decoder_handle handle, sb_media_dss_event_t event);

/** Structure for for audio configuration
 *  Audio decryption is not required
 *  SHOULD NOT BE CHANGED
 *  TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_audio_config_t {
    sb_media_callback                               config_done;            /**< callback when config is done */
    sb_media_callback                               decode_done;            /**< callback when audio frame decode is done */
    sb_media_audio_codec_t                          codec;                  /**< Audio Codec */
    sb_media_decoder_handle_t                       video_handle;           /**< Video Codec to lipsync */
    uint8_t                                         number_of_channels;     /**< Number of Audio Channels */
    sb_media_audio_sample_rate_t                    sample_rate;            /**< Audio Sample Rate */
    uint8_t                                         unused;                 /**< Unused for alignment */
    int16_t                                         delay;                  /**< Audio delay in msec */
    int8_t                                          volume;                 /**< Audio volume */
}sb_media_audio_config_t;

typedef struct sb_media_rect_t {
    int16_t                                         x_offset;       /**< X location in pixels (upper left corner of rectangle */
    int16_t                                         y_offset;       /**< Y location in pixels (upper left corner of rectangle */
    int16_t                                         height;         /**< Height in pixels */
    int16_t                                         width;          /**< Width in pixels */
}sb_media_rect_t;

/** Structure for for video configuration
 *  SHOULD NOT BE CHANGED
 *  TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_video_config_t {
    sb_media_callback                               config_done;        /**< callback when config is done */
    sb_media_callback                               decode_done;        /**< callback when video frame decode is done */
    sb_media_video_codec_t                          codec;              /**< Video Codec */
    sb_media_video_dynamic_range_t                  dynamic_range;      /**< Video Dynamic Range */
    sb_media_color_format_flag_t                    color_format;       /**< Output Video Color Format */
    sb_media_color_depth_flag_t                     color_depth;        /**< Output Video Depth */
    sb_media_encryption_t                           encryption;         /**< Encryption */
    sb_media_rect_t                                 output_rectangle;   /**< Output Rectangle Size */
    int8_t                                          playback_rate;      /**< Positive numbers increase playback, negative numbers decrease playback, normal = default = 1 */
    uint8_t                                         unused[2];          /**< Unused for alignment */
}sb_media_video_config_t;

/** Output structure
 *  SHOULD NOT BE CHANGED
 *  TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_output_buffer_t {
    uint8_t                                         *buffer;        /**< pointer to output buffer */
    uint16_t                                        size;           /**< size of buffer in bytes */
}sb_media_output_buffer_t;

typedef struct sb_media_sub_sample_t
{
    uint32_t                                        clear_data;        /**< number of bytes of clear data */
    uint32_t                                        encrypted_data;    /**< number of bytes of encrypted data */
}sb_media_sub_sample_t;

/** Decryption structure
 *  SHOULD NOT BE CHANGED
 *  TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_decrypt_t {
    uint8_t                                         *iv;                    /**< Initialization vector */
    uint8_t                                         *keyID;                 /**< pointer to decryption key ID*/
    uint16_t                                        key_size;               /**< size of decryption key */
    uint16_t                                        number_of_subsamples;   /**< number of subsamples in  sub_sample */
    sb_media_sub_sample_t                           *sub_samples;           /**< array of sub_samples extracted from subSampleEncryptionTable */
    uint8_t                                         iv_size;                /**< size of initialization vector */
    uint8_t                                         unused[3];              /**< alignment */
}sb_media_decrypt_t;

typedef struct sb_media_buffer_t {
    uint8_t                                         *buffer;        /**< pointer to buffer */
    uint16_t                                        size;           /**< size of buffer in bytes */
    uint8_t                                         unused[2];      /**< alignment */
}sb_media_buffer_t;

/** Structure for for capabilities SHOULD NOT BE CHANGED
 *  TO PRESERVE BINARY COMPATIBILITY !!!!
 *  Seperate HDMI into another file */
typedef struct sb_media_decoder_capabilities_t {
    sb_media_audio_codec_t                          audio_codecs;            /**< Audio Codecs */
    sb_media_video_profile_t                        profile;                 /**< Maximum video profile */
    sb_media_video_level_t                          level;                   /**< Maximum video level */
    sb_media_video_dynamic_range_t                  dynamic_range;           /**< Video Dynamic Ranges */
    sb_media_video_output_resolution_t              video_output_resolution; /**< Output Video Resolutions */
    sb_media_video_output_rate_t                    video_output_rate;       /**< Output Video Rates */
    sb_media_drm_flag_t                             drm;                     /**< DRM capabilities */
    sb_media_encryption_t                           encryption;              /**< Encryption Capabilities */
    sb_media_hdcp_level_t                           hdcp_level;              /**< HDCP Level */
    sb_media_hdcp_level_t                           hdcp_level;              /**< HDCP Level */
    uint8_t                                         unused;                  /**< alignment */
}sb_media_decoder_capabilities_t;

/** Structure for Decoder Statistics
 * SHOULD NOT BE CHANGED TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_decoder_stats_t {
    uint32_t                                        raw_width;          /**< Width of source video */
    uint32_t                                        raw_height;         /**< Height of source video */
    uint32_t                                        displayed_width;    /**< Width of displayed video */
    uint32_t                                        displayed_height;   /**< Height of displayed video */
    uint32_t                                        frames_displayed;   /**< Number of frames displayed */
    /**< Number of free bytes in buffer, please refer to audio/video_buffer_bytes for total size in sb_media_statistics_t depending on nature of decoder handle. */
    uint32_t                                        free_buffer_bytes; 
    uint8_t                                         active;             /**< is the decoder running, true = 1 */
    uint8_t                                         used;               /**< is the decoder used, true = 1 */
    uint8_t                                         unused[2];          /**< alignment */
} sb_media_decoder_stats_t;


/** Structure for Player Statistics
 * SHOULD NOT BE CHANGED TO PRESERVE BINARY COMPATIBILITY !!!! */
typedef struct sb_media_statistics_t {
    float                                           frame_rate;             /**< Number of frames per second */
    uint32_t                                        dropped_frames;         /**< Number of dropped frames since last read */
    uint32_t                                        audio_buffer_bytes;     /**< buffer in bytes */
    uint32_t                                        video_buffer_bytes;     /**< buffer in bytes */
    sb_media_timestamp_t                            pts;                    /**< Current Presentation Time Stamp */
    uint32_t                                        frames_displayed;       /**< Number of frames displayed */
    uint32_t                                        frames_decoded;         /**< Number of frames decoded */
    uint32_t                                        buffered_range;         /**< buffered range in seconds */
    uint32_t                                        playback_range;         /**< Playback range in seconds */
    uint32_t                                        seekable_range;         /**< Seekable range in seconds */
} sb_media_statistics_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Discovers and Initializes Global resources 
 *  this may be called multiple times, but initialized once
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_global_init();

/**
 * Shut down and Release Global resources 
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_global_shutdown();

/**
 * Initializes Audio Config to default values
 *
 * @param[in]  config pointer to audio configuration
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_init_audio_config(sb_media_audio_config_t *config);

/**
 * Initialize Audio Decoder
 *
 * No more than one audio decoder required
 * Decoder can be re-initialized if handle != NULL.
 *  Decoder re-initialization will also reset the decoder
 *
 * @param[in]  config pointer to audio configuration
 * @param[in]  handle returned handle, NULL if unsuccessful
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_init_audio_decoder(sb_media_audio_config_t *config, sb_media_decoder_handle handle);

/**
 * Initializes Video Config to default values
 *
 * @param[in]  config pointer to video configuration
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_init_video_config(sb_media_video_config_t *config);

/**
 * Initialize Video Decoder
 *
 * It should be possible for more than one video decoder
 * Decoder can be re-initialized if handle != NULL.
 *  Decoder re-initialization will also reset the decoder
 *
 * @param[in]  config pointer to video configuration
 * @param[in]  handle returned handle, NULL if unsuccessful
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_init_video_decoder(sb_media_video_config_t *config, sb_media_decoder_handle handle);

/**
 * Decode
 *
 * Decode Audio or Video Elementary Stream
 *
 * @param[in]  handle of decoder handle
 * @param[in]  pes pointer to start of PES header, that includes the NALU header
 *             If *pes is NULL, then the decoder is to be flushed
 * @param[in]  length of PES packet pointer to by pes pointer to start of PES header including NALU header
 * @param[in]  out pointer to start output decode structure, the pointer is provided by dss-nve
 *             *out is valid only when rendering is NOT done by hardware, else it is NULL
 *             Layout of the output buffer when doing software rendering should be consistent with the initialized
 *             sb_media_video_config_t.
 * @param[in]  key pointer to decryption key structure, if NULL then the data is not encrypted. 
 *             PES header is never encrypted 
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_decode(sb_media_decoder_handle handle, uint8_t *pes, uint32_t size, sb_media_timestamp_t pts, sb_media_output_buffer_t *out, sb_media_decrypt_t *key);

/**
 * Set Video Decode Output Position
 *
 * Sets the screen output position of the decoded. This is only valid for punch-through video
 * decoders. 
 *
 * @param[in]  handle of video decoder
 * @param[in]  output position 
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_set_video_output_rectangle(sb_media_decoder_handle handle, const sb_media_rect_t *rect);

/**
 * Set Playback Rate
 *
 * Sets the playback rate of the decoder, positive numbers increase playback, negative numbers decrease playback, normal = 1 and pause = 0
 *
 * @param[in]  handle of decoder
 * @param[in]  playback rate
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_set_playback_rate(sb_media_decoder_handle handle, int8_t playback_rate);

/**
 * Reset Decoder
 *
 * Resets Audio or Video Decoder
 *
 * @param[in]  handle of decoder handle
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_reset_decoder(sb_media_decoder_handle handle);

/**
 * Get Time
 *
 * @param[in]  handle of decoder handle
 * @param[in]  time - pointer to current presentation time
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_get_time(sb_media_decoder_handle handle, sb_media_timestamp_t *presentation_time);

/**
 * Set DRM
 *  Set to use Playready, Widevine, Nagra or No DRM
 *
 * @param[in]  drm - DRM to use (None, Playready or Widevine)
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_set_drm(sb_media_drm_flag_t drm);

/**
 * Sets DRM certificate
 *  Used for Widevine DRM
 *
 * @param[in]  certificate - pointer to DRM certificate
 * @param[in]  size - size of certificate in bytes
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_set_drm_certificate(const uint8_t *certificate, uint32_t size);


/**
 * Challenge callback
 *
 * Called back with Key challenge
 *
 * @param[in]  challenge      - pointer to challenge input buffer
 * @param[in]  challenge_size - challenge input buffer size
 *
 * @return     standard sb_media_result_t
 */
typedef sb_media_result_t (*sb_media_challenge_callback)(const uint8_t *challenge, const uint16_t challenge_size);

/**
 * Generate Key Challenge
 *
 * Generates Key challenge
 *
 * @param[in]  object      - pointer to obj input buffer (PlayReady PRO, Widevine PSSH Box)
 * @param[in]  object_size - obj input buffer size
 * @param[in]  callback    - response callback
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_generate_challenge(const uint8_t *object, const uint16_t object_size, sb_media_challenge_callback callback);

/**
 * Process Key Message Response
 *
 * Adds license
 *
 * @param[in]  response      - pointer to license response input buffer
 * @param[in]  response_size - response input buffer size
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_process_key_message_response(const uint8_t *response, const uint16_t response_size);

/**
 * Get Decoder Capabilities
 *
 * @param[in]  capabilities - pointer to Decoder Capabilities (capabilities_t)
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_get_decoder_capabilities(sb_media_decoder_capabilities_t *capabilities);

/**
 * Set Event Callback Function
 *  Sets the Callback Function
 *  Callbacks are used for:
 *   - Buffer Empty, i.e. SB_MEDIA_PLAY_EVENT_UNDERFLOW
 *   - Buffer Full, i.e. SB_MEDIA_PLAY_EVENT_OVERFLOW
 *   - Detected video rectangle, i.e. SB_MEDIA_PLAY_EVENT_DIMENSION_CHANGE
 *
 * @param[in]  callback - Pointer to callback function
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_set_event_callback(sb_media_event_callback callback);

/**
 * Get Player Statistics
 *
 * @param[in]  stats - pointer to statistic structure
 *
 * @return     standard sb_result_t
 */
sb_media_result_t sb_media_get_stats(sb_media_statistics_t *stats);

/**
 * Get Player Statistics
 *
 * @param[in]  stats - pointer to statistic structure
 *
 * @return     standard sb_media_result_t
 */
sb_media_result_t sb_media_get_decoder_stats(sb_media_decoder_handle handle, sb_media_decoder_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* SB_MEDIA_DECODER_H */
