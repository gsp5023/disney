/****************************************************************************
 * Copyright (c) 2021 Disney
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

#ifndef __NVE_DEFINES_H__
#define __NVE_DEFINES_H__

#if defined(__GNUC__) && __GNUC__ < 5
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>

/*
 * Define FFI stubs if FFI generation macros are not defined.
 */
#ifndef FFI_EXPORT
#define FFI_EXPORT
#define FFI_NAME(_name)
#define FFI_PTR_WASM
#define FFI_PTR_NATIVE
#define FFI_SINGLE
#define FFI_SLICE
#define FFI_CAN_BE_NULL
#define FFI_IMPLEMENT_IF_DEF(_macro)
#define FFI_ENUM_BITFLAGS
#endif

#define FFI_EXPORT_NVE FFI_EXPORT FFI_IMPLEMENT_IF_DEF(_NVE)

#ifndef TOKENPASTE2
#define TOKENPASTE2(_x, _y) _x##_y
#endif

#ifndef TOKENPASTE
#define TOKENPASTE(_x, _y) TOKENPASTE2(_x, _y)
#endif

#ifndef FORCE_ENUM_INT32
#define FORCE_ENUM_INT32(__name) TOKENPASTE(TOKENPASTE(__, __name), __force_int32) = INT32_MIN
#endif

#ifndef NVE_VISIBLE
#ifdef _MSC_VER
#define NVE_VISIBLE
#else
#define NVE_VISIBLE __attribute__((visibility("default")))
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void * nve_handle_t;

FFI_EXPORT typedef enum nve_media_resource_type_e {
    HDS = 0,
    HLS = 1,
    DASH = 2,
    Custom = 3,
    Unknown = 4,
    ISOBMFF = 5,
} nve_media_resource_type_e;

FFI_EXPORT typedef enum nve_result_e {
    nve_result_success = 0,
    nve_result_invalid_argument = 1,
    nve_result_null_pointer = 2,
    nve_result_illegal_state = 3,
    nve_result_interface_not_found = 4,
    nve_result_creation_failed = 5,
    nve_result_unsupported_operation = 6,
    nve_result_data_not_available = 7,
    nve_result_seek_error = 8,
    nve_result_unsupported_feature = 9,
    nve_result_range_error = 10,
    nve_result_codec_not_supported = 11,
    nve_result_media_error = 12,
    nve_result_network_error = 13,
    nve_result_generic_error = 14,
    nve_result_invalid_seek_time = 15,
    nve_result_audio_track_error = 16,
    nve_result_access_from_different_thread_error = 17,
    nve_result_element_not_found = 18,
    nve_result_not_implemented = 19,
    nve_result_preroll_disabled = 20,
    nve_result_playback_not_authorized = 57,
    nve_result_network_timeout = 58,
    nve_result_playback_operation_failed = 200,
    nve_result_native_warning = 201,
    nve_result_ad_resolver_failed = 202,
    nve_result_ad_manifest_load_failed = 203,
    nve_result_lazy_ad_resolution_in_progress = 204,
    nve_result_revenue_optimization = 300
} nve_result_e;

FFI_EXPORT typedef enum nve_event_type_e {
    nve_event_type_none,
    nve_event_type_timeline,
    nve_event_type_timed_metadata,
    nve_event_type_time_change,
    nve_event_type_size_available,
    nve_event_type_seek,
    nve_event_type_profile,
    nve_event_type_playback_rate,
    nve_event_type_media_player_status_changed,
    nve_event_type_media_player_item,
    nve_event_type_load_information,
    nve_event_type_buffer,
    nve_event_type_cenc_init_data,
    nve_event_type_drm_challenge_generated,
    nve_event_type_text_updated,
    nve_event_type_audio_updated,
    nve_event_type_drm_error,
    nve_event_type_text_manifest_updated,
    nve_event_type_audio_manifest_updated,
    nve_event_type_media_player_general_info,
    nve_event_type_main_manifest_updated,
    nve_event_type_play_start
} nve_event_type_e;

FFI_EXPORT typedef enum nve_timed_metadata_type_e {
    nve_timed_metadata_type_tag,
    nve_timed_metadata_type_id3
} nve_timed_metadata_type_e;

FFI_EXPORT typedef enum nve_media_player_status_e {
    nve_media_player_status_idle,
    nve_media_player_status_initializing,
    nve_media_player_status_initialized,
    nve_media_player_status_preparing,
    nve_media_player_status_prepared,
    nve_media_player_status_playing,
    nve_media_player_status_paused,
    nve_media_player_status_seeking,
    nve_media_player_status_complete,
    nve_media_player_status_error,
    nve_media_player_status_released,
    nve_media_player_status_suspended
} nve_media_player_status_e;

FFI_EXPORT typedef enum nve_load_information_type_e {
    nve_load_information_type_fragment_video = 0,
    nve_load_information_type_fragment_audio = 1,
    nve_load_information_type_fragment_subtitle = 2,
    nve_load_information_type_fragment_track = 3,
    nve_load_information_type_manifest_master = 4,
    nve_load_information_type_manifest_video = 5,
    nve_load_information_type_manifest_audio = 6,
    nve_load_information_type_manifest_subtitle = 7,
    nve_load_information_type_file = 8
} nve_load_information_type_e;

#define NVE_FFI_TRACK_STRING_CAPACITY 64
FFI_EXPORT typedef struct {
    int32_t id;
    char track_name[NVE_FFI_TRACK_STRING_CAPACITY];
    char track_language[NVE_FFI_TRACK_STRING_CAPACITY];
    int32_t is_forced;
    int32_t is_default;
    int32_t is_auto_select;
} nve_text_track_t;

FFI_EXPORT typedef struct {
    int32_t id;
    char track_name[NVE_FFI_TRACK_STRING_CAPACITY];
    char track_language[NVE_FFI_TRACK_STRING_CAPACITY];
    char track_channels[NVE_FFI_TRACK_STRING_CAPACITY];
    int32_t pid;
    int32_t is_default;
    int32_t is_auto_select;
} nve_audio_track_t;

#define NVE_FFI_METADATA_STRING_CAPACITY 1024 
FFI_EXPORT typedef struct {
    char key[NVE_FFI_METADATA_STRING_CAPACITY];
    char value[NVE_FFI_METADATA_STRING_CAPACITY];
} nve_metadata_key_value_t;

#define NVE_FFI_DRM_ERROR_STRING_CAPACITY 1024
FFI_EXPORT typedef struct {
    char error_string[NVE_FFI_DRM_ERROR_STRING_CAPACITY];
    char error_server_url[NVE_FFI_DRM_ERROR_STRING_CAPACITY];
} nve_drm_error_payload_t;

FFI_EXPORT typedef enum nve_playlist_type_e {
    nve_playlist_type_not_playlist = 0,
    nve_playlist_type_no_type = 1,
    nve_playlist_type_event = 2,
    nve_playlist_type_vod = 3
} nve_playlist_type_e;

#define NVE_FFI_LOAD_INFORMATION_URL_CAPACITY 512
FFI_EXPORT typedef struct {
    int32_t period_index;
    int32_t size;
    int32_t track_index;
    int32_t http_status;
    int32_t latency;
    int32_t error_code;
    bool playlist_complete;
    nve_playlist_type_e playlist_type;
    char track_name[NVE_FFI_TRACK_STRING_CAPACITY];
    char track_type[NVE_FFI_TRACK_STRING_CAPACITY];
    char url[NVE_FFI_LOAD_INFORMATION_URL_CAPACITY];
    int32_t profile_id;
    int32_t profile_width;
    int32_t profile_height;
    int32_t profile_peak_bit_rate;
    int32_t profile_avg_bit_rate;
    float profile_framerate;
    char x_request_id[NVE_FFI_TRACK_STRING_CAPACITY];
} nve_load_information_t;

typedef enum {
    nve_event_payload_flag_none,
    nve_event_payload_flag_user_data,
    nve_event_payload_flag_psdk_event
} nve_event_payload_flag_e;

FFI_EXPORT typedef struct {
    nve_event_type_e event_type;
    float f_arg0;
    float f_arg1;
    float f_arg2;
    // HACK: Using uint64_t for the event native ptr - use void * when this gets
    // supported
    uint64_t event_payload_ptr;
} nve_event_payload_t;

FFI_EXPORT typedef struct {
    int32_t id;
    int32_t width;
    int32_t height;
    int32_t peak_bit_rate;
    int32_t avg_bit_rate;
    float framerate;
} nve_profile_t;

#define NVE_FFI_VERSION_STRING_CAPACITY 64
FFI_EXPORT typedef struct {
    char version[NVE_FFI_VERSION_STRING_CAPACITY];
    uint32_t major;
    uint32_t minor;
    uint32_t revision;
    uint32_t revision_minor;
    uint32_t api_version;
} nve_version_t;

FFI_EXPORT typedef enum nve_drm_method_e {
    nve_drm_method_none = 0,
    nve_drm_method_vanilla_aes128 = 1,
    nve_drm_method_aes128 = 2,
    nve_drm_method_sample_aes = 3,
    nve_drm_method_playready_aes128 = 4,
    nve_drm_method_widevine = 5,
    FORCE_ENUM_INT32(nve_drm_method_e)
} nve_drm_method_e;

FFI_EXPORT typedef enum nve_drm_key_tag_e {
    nve_drm_key_tag_unknown = 0,
    nve_drm_key_tag_session = 1,
    nve_drm_key_tag_variant = 2,
    FORCE_ENUM_INT32(nve_drm_key_tag_e)
} nve_drm_key_tag_e;

FFI_EXPORT typedef struct {
    double begin;
    double end;
} nve_range_t;

FFI_EXPORT typedef enum nve_drm_workflow_init_type_e {
    nve_drm_workflow_init_type_app_context = 0,
    nve_drm_workflow_init_type_url_context = 1,
    nve_drm_workflow_init_type_drm_manager_context = 2,
    FORCE_ENUM_INT32(nve_drm_workflow_init_type_e)
} nve_drm_workflow_init_type_e;

#define NVE_FFI_DRM_INIT_STRING_CAPACITY 64
FFI_EXPORT typedef struct {
    nve_drm_workflow_init_type_e init_type;
    uint64_t core_player_context;
    char app_storage_path[NVE_FFI_DRM_INIT_STRING_CAPACITY];
    char publisher_id[NVE_FFI_DRM_INIT_STRING_CAPACITY];
    char app_id[NVE_FFI_DRM_INIT_STRING_CAPACITY];
    char app_version[NVE_FFI_DRM_INIT_STRING_CAPACITY];
    bool privacy_mode;
} nve_drm_init_payload_t;

#define NVE_TIMED_METADATA_TAG_STRING_CAPACITY 64
FFI_EXPORT typedef struct {
    char tag[NVE_FFI_VERSION_STRING_CAPACITY];
} nve_timed_metadata_tag_t;

#ifdef __cplusplus
}
#endif

#endif
