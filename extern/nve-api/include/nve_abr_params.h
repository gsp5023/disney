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

#ifndef __NVE_ABR_PARAMS_H__
#define __NVE_ABR_PARAMS_H__

#include "nve_defines.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT typedef enum nve_abr_policy_e {
    nve_abr_policy_conservative = 0,
    nve_abr_policy_moderate = 1,
    nve_abr_policy_aggressive = 2,
    FORCE_ENUM_INT32(nve_abr_policy_e)
} nve_abr_policy_e;

FFI_EXPORT typedef struct {
    uint32_t initial_bitrate_bps;
    uint32_t min_bitrate_bps;
    uint32_t max_bitrate_bps;
    uint32_t max_avg_bitrate_bps;
    uint32_t min_trickplay_bitrate_bps;
    uint32_t max_trickplay_bitrate_bps;
    uint32_t max_trickplay_avg_bitrate_bps;
    uint32_t max_trickplay_bandwidth_usage_bps;
    double   max_playout_rate;
    uint32_t max_width;
    uint32_t max_height;
    nve_abr_policy_e abr_policy;
} nve_abr_params_t;

// Initialize default parameters
FFI_EXPORT_NVE void nve_abr_params_init(FFI_PTR_WASM nve_abr_params_t * params);

FFI_EXPORT_NVE void nve_media_player_get_abr_params(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_abr_params_t * params);
FFI_EXPORT_NVE void nve_media_player_set_abr_params(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM const nve_abr_params_t * params);

#ifdef __cplusplus
}
#endif

#endif
