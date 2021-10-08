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

#ifndef __NVE_BUFFER_PARAMS_H__
#define __NVE_BUFFER_PARAMS_H__

#include "nve_defines.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT typedef struct {
    double initial_buffer_time;
    double play_buffer_time;
} nve_buffer_params_t;

// Initialize default parameters
FFI_EXPORT_NVE void nve_buffer_params_init(FFI_PTR_WASM nve_buffer_params_t * params);

FFI_EXPORT_NVE void nve_media_player_get_buffer_params(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM nve_buffer_params_t * params);
FFI_EXPORT_NVE void nve_media_player_set_buffer_params(FFI_PTR_NATIVE void * player_ptr, FFI_PTR_WASM const nve_buffer_params_t * params);

#ifdef __cplusplus
}
#endif

#endif
