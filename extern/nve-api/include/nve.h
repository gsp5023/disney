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

#ifndef __NVE_H__
#define __NVE_H__

#include "nve_defines.h"
#include "nve_network.h"
#include "nve_network_configuration.h"

struct cg_context_t;

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT_NVE void nve_init();
FFI_EXPORT_NVE void nve_enable_debug_logging();
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_mpi_config_create(FFI_PTR_WASM nve_network_configuration_t * network_configuration);
FFI_EXPORT_NVE void nve_mpi_config_subscribe_tag(FFI_PTR_NATIVE void * config_ptr, FFI_PTR_WASM const char * tag_name);
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_media_resource_create(FFI_PTR_WASM const char * url, nve_media_resource_type_e media_resource_type, FFI_PTR_NATIVE FFI_CAN_BE_NULL void * metadata_ptr);
FFI_EXPORT_NVE void nve_media_resource_release(FFI_PTR_NATIVE void * media_resource_ptr);
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_dispatcher_create(FFI_PTR_NATIVE void * callback_manager_ptr);
FFI_EXPORT_NVE void nve_dispatcher_bind_media_player_events(FFI_PTR_NATIVE void * dispatcher_ptr, FFI_PTR_NATIVE void * media_player_ptr);
FFI_EXPORT_NVE void nve_dispatcher_unbind_media_player_events(FFI_PTR_NATIVE void * dispatcher_ptr, FFI_PTR_NATIVE void * media_player_ptr);
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_media_player_create(FFI_PTR_NATIVE void * dispatcher_ptr);
FFI_EXPORT_NVE void nve_media_player_set_view(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_NATIVE void * view_ptr);
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_media_player_get_view(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_media_player_replace_resource(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_NATIVE void * resource_ptr, FFI_PTR_NATIVE void * config_ptr);
FFI_EXPORT_NVE void nve_media_player_play(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_media_player_prepare_to_play(FFI_PTR_NATIVE void * player_ptr, float position);
FFI_EXPORT_NVE void nve_media_player_pause(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_media_player_stop(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_media_player_seek(FFI_PTR_NATIVE void * player_ptr, double position_ms);
FFI_EXPORT_NVE double nve_media_player_position(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE nve_result_e nve_media_player_release(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE nve_result_e nve_media_player_reset(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE nve_result_e nve_media_player_suspend(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE nve_result_e nve_media_player_restore(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_media_player_buffered_range_get(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_range_t * out_range);
FFI_EXPORT_NVE void nve_media_player_playback_range_get(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_range_t * out_range);
FFI_EXPORT_NVE void nve_release_psdk_ref(FFI_PTR_NATIVE FFI_CAN_BE_NULL void * psdk_shared_ptr);

FFI_EXPORT_NVE FFI_NAME(nve_view_create) FFI_PTR_NATIVE void * nve_m5_view_create(FFI_PTR_NATIVE FFI_CAN_BE_NULL cg_context_t * const ctx, int32_t x, int32_t y, int32_t w, int32_t h);
FFI_EXPORT_NVE FFI_NAME(nve_m5_view_release) void nve_m5_view_release(FFI_PTR_NATIVE void * view_ptr);
FFI_EXPORT_NVE FFI_NAME(nve_view_get_x) int32_t nve_m5_view_get_x(FFI_PTR_NATIVE void * view);
FFI_EXPORT_NVE FFI_NAME(nve_view_get_y) int32_t nve_m5_view_get_y(FFI_PTR_NATIVE void * view);
FFI_EXPORT_NVE FFI_NAME(nve_view_get_width) int32_t nve_m5_view_get_width(FFI_PTR_NATIVE void * view);
FFI_EXPORT_NVE FFI_NAME(nve_view_get_height) int32_t nve_m5_view_get_height(FFI_PTR_NATIVE void * view);
FFI_EXPORT_NVE FFI_NAME(nve_view_set_pos) void nve_m5_view_set_position(FFI_PTR_NATIVE void * view, int32_t x, int32_t y);
FFI_EXPORT_NVE FFI_NAME(nve_view_set_size) void nve_m5_view_set_size(FFI_PTR_NATIVE void * view, int32_t width, int32_t height);

// Impl in ncp-m5/source/adk/nve/psdk/M5CallbackManager.cpp
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_m5_callback_manager_create(void);
FFI_EXPORT_NVE void nve_m5_callback_manager_release(FFI_PTR_NATIVE void * callback_manager_ptr);

// Impl in ncp-m5/source/adk/nve/psdk/M5DRMOperationCompleteListener.cpp
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_m5_operation_complete_listener_create(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_m5_operation_complete_listener_release(FFI_PTR_NATIVE void * operation_complete_listener_ptr);

// Impl in ncp-m5/source/adk/nve/psdk/M5ChallengeGeneratedListener.cpp
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_m5_challenge_generated_listener_create(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_NATIVE FFI_CAN_BE_NULL void * drm_manager_ptr, FFI_PTR_NATIVE void * operation_complete_listener_ptr);
FFI_EXPORT_NVE void nve_m5_challenge_generated_listener_release(FFI_PTR_NATIVE void * challenge_generated_listener_ptr);

FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_media_player_get_drm_manager(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE nve_result_e nve_drm_manager_generate_challenge(FFI_PTR_NATIVE void * drm_manager, FFI_PTR_WASM FFI_SLICE const uint8_t * const bytes, uint32_t len, FFI_PTR_NATIVE void * listener);
FFI_EXPORT_NVE nve_result_e nve_drm_manager_store_certificate_bytes(FFI_PTR_NATIVE void * drm_manager, FFI_PTR_WASM FFI_SLICE const uint8_t * const bytes, uint32_t len, FFI_PTR_NATIVE void * listener);
FFI_EXPORT_NVE nve_result_e nve_drm_manager_store_license_bytes(FFI_PTR_NATIVE void * drm_manager, FFI_PTR_WASM FFI_SLICE const uint8_t * const bytes, uint32_t len, FFI_PTR_NATIVE void * listener);


FFI_EXPORT_NVE void nve_event_filtering_enabled(bool is_enabled);
FFI_EXPORT_NVE void nve_media_player_event_buffer_bind(FFI_PTR_NATIVE nve_handle_t handle, uint32_t event_buffer_capacity);

FFI_EXPORT_NVE void nve_media_player_event_buffer_unbind(FFI_PTR_NATIVE nve_handle_t handle);
FFI_EXPORT_NVE void nve_media_player_event_get(FFI_PTR_NATIVE nve_handle_t handle, FFI_PTR_WASM nve_event_payload_t * payload);
FFI_EXPORT_NVE void nve_m5_cenc_init_data_get(FFI_PTR_NATIVE nve_handle_t handle, FFI_PTR_WASM FFI_SINGLE void * out_ptr, FFI_PTR_WASM int32_t * out_size, FFI_PTR_WASM nve_drm_method_e * method, FFI_PTR_WASM nve_drm_key_tag_e * tag);

FFI_EXPORT_NVE nve_result_e nve_initialize_drm(FFI_PTR_WASM nve_drm_init_payload_t * payload);

FFI_EXPORT_NVE void nve_set_local_storage_path(FFI_PTR_WASM const char * storage_path);

FFI_EXPORT_NVE void nve_m5_challenge_get(FFI_PTR_NATIVE void * challenge_generated_event_payload_ptr, FFI_PTR_WASM FFI_SINGLE void * out_buffer_ptr, FFI_PTR_WASM int32_t * out_size_ptr);

FFI_EXPORT_NVE bool nve_text_has_tracks(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE void nve_text_visibility_set(FFI_PTR_NATIVE void * player_ptr, bool cc_visibility);
FFI_EXPORT_NVE bool nve_text_visibility_get(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE int32_t nve_text_num_tracks_get(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE bool nve_text_track_get(FFI_PTR_NATIVE void * player_ptr, int32_t track_id, FFI_PTR_WASM nve_text_track_t * out_track);
FFI_EXPORT_NVE bool nve_text_current_track_get(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_text_track_t * out_track);
FFI_EXPORT_NVE bool nve_text_track_set(FFI_PTR_NATIVE void * player_ptr, int32_t track_id);
FFI_EXPORT_NVE bool nve_text_set_custom_font(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_NATIVE FFI_CAN_BE_NULL void * const font_bytes, uint32_t len, uint32_t uncompressed_len);

FFI_EXPORT_NVE int32_t nve_audio_num_tracks_get(FFI_PTR_NATIVE void * player_ptr);
FFI_EXPORT_NVE bool nve_audio_track_get(FFI_PTR_NATIVE void * player_ptr, int32_t track_id, FFI_PTR_WASM nve_audio_track_t * out_track);
FFI_EXPORT_NVE bool nve_audio_current_track_get(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_audio_track_t * out_track);
FFI_EXPORT_NVE bool nve_audio_track_set(FFI_PTR_NATIVE void * player_ptr, int32_t track_id);

FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_metadata_create();
FFI_EXPORT_NVE bool nve_metadata_event_capture(FFI_PTR_NATIVE void * metadata_ptr, FFI_PTR_NATIVE void * event_ptr);
FFI_EXPORT_NVE int32_t nve_metadata_num_keys_get(FFI_PTR_NATIVE void * metadata_ptr);
FFI_EXPORT_NVE bool nve_metadata_get(FFI_PTR_NATIVE void * metadata_ptr, int32_t metadata_index, FFI_PTR_WASM nve_metadata_key_value_t * out_metadata);

FFI_EXPORT_NVE void nve_drm_error_text_get(FFI_PTR_NATIVE void * operation_complete_ptr, FFI_PTR_WASM nve_drm_error_payload_t * out_drm_error);

FFI_EXPORT_NVE bool nve_load_information_get(FFI_PTR_NATIVE void * event_ptr, FFI_PTR_WASM nve_load_information_t * out_load_information);

FFI_EXPORT_NVE void nve_payload_ref_add(FFI_PTR_NATIVE FFI_CAN_BE_NULL void * payload_ptr);
FFI_EXPORT_NVE void nve_payload_ref_release(FFI_PTR_NATIVE FFI_CAN_BE_NULL void * payload_ptr);


FFI_EXPORT_NVE bool nve_profile_get(FFI_PTR_NATIVE void * player_ptr, int32_t profile_index, FFI_PTR_WASM nve_profile_t * out_profile);
FFI_EXPORT_NVE bool nve_current_profile_get(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_profile_t * out_profile);
FFI_EXPORT_NVE int32_t nve_profile_num_profiles_get(FFI_PTR_NATIVE void * player_ptr);

FFI_EXPORT_NVE void nve_version_get(FFI_PTR_WASM nve_version_t * out_version);

#ifdef __cplusplus
}
#endif

#endif
