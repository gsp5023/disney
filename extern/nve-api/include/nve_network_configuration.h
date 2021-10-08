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

#ifndef __NVE_NETWORK_CONFIG_H__
#define __NVE_NETWORK_CONFIG_H__

#include "nve_defines.h"    
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVE_FFI_NETWORK_CONFIGURATION_CUSTOM_USER_AGENT_CAPACTIY 256
#define NVE_FFI_NETWORK_CONFIGURATION_DOWN_URL_CAPACTIY 256

FFI_EXPORT typedef struct {
    bool use_cookie_header_for_all_requests;
    bool force_native_networking;
    bool force_https;
    bool read_set_cookie_header;
    bool use_redirect_domain;
    bool keep_alive;
    bool use_parent_manifest_query_params;
    bool account_for_buffer_length_to_calc_live_point;
    bool disable_ssl_certificate;
    bool offline_playback;
    char custom_user_agent[NVE_FFI_NETWORK_CONFIGURATION_CUSTOM_USER_AGENT_CAPACTIY];
    char network_down_url[NVE_FFI_NETWORK_CONFIGURATION_DOWN_URL_CAPACTIY];
    uint32_t ad_manifest_timeout;
    uint32_t number_of_manifest_retry_before_error;
    uint32_t number_of_map_retry_before_error;
    uint32_t number_of_vtt_retry_before_error;
    uint32_t number_of_audio_segments_retry_before_error;
    uint32_t number_of_video_segments_retry_before_error;
    uint32_t number_of_segments_from_live_point;
    uint32_t first_byte_timeout_ms;
    uint32_t file_timeout_ms;
    uint64_t cookie_headers_metadata_ptr; // Hack since void * can't be used in structs for FFI_GEN
    uint64_t custom_headers_metadata_ptr; // Hack since void * can't be used in structs for FFI_GEN
} nve_network_configuration_t;

FFI_EXPORT_NVE nve_result_e nve_media_player_item_network_configuration_get(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_network_configuration_t * config);

#ifdef __cplusplus
}
#endif

#endif
