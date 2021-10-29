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

#ifndef __NVE_NETWORK_H__
#define __NVE_NETWORK_H__

#include "nve_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

FFI_EXPORT typedef enum nve_network_method_e {
    nve_network_method_get,
    nve_network_method_post
} nve_network_method_e;

FFI_EXPORT typedef enum nve_network_request_state_e {
    nve_network_request_state_init,
    nve_network_request_state_executing,
    nve_network_request_state_done,
    nve_network_request_state_error
} nve_network_request_state_e;

#define NVE_FFI_NETWORK_REQUEST_ERROR_TEXT_CAPACITY 256
FFI_EXPORT typedef struct {
    char error_text[NVE_FFI_NETWORK_REQUEST_ERROR_TEXT_CAPACITY];
} nve_network_request_error_data_t;

void nve_network_init();
FFI_EXPORT_NVE FFI_PTR_NATIVE void * nve_network_request_create(FFI_PTR_WASM const char * url, nve_network_method_e method);
FFI_EXPORT_NVE nve_result_e nve_network_request_release(FFI_PTR_NATIVE void * request_handle);
FFI_EXPORT_NVE nve_result_e nve_network_request_header_set(FFI_PTR_NATIVE void * request_handle, FFI_PTR_WASM const char * header_name, FFI_PTR_WASM const char * header_data);
FFI_EXPORT_NVE nve_result_e nve_network_request_payload_set(FFI_PTR_NATIVE void * request_handle, FFI_PTR_WASM FFI_SLICE const uint8_t * const data_ptr, uint32_t data_size);
FFI_EXPORT_NVE nve_result_e nve_network_request_execute(FFI_PTR_NATIVE void * request_handle);
FFI_EXPORT_NVE nve_network_request_state_e nve_network_request_get_state(FFI_PTR_NATIVE void * request_handle);
FFI_EXPORT_NVE uint32_t nve_network_request_result_code_get(FFI_PTR_NATIVE void * request_handle);
FFI_EXPORT_NVE nve_result_e nve_network_request_body_data_get(FFI_PTR_NATIVE void * request_handle, FFI_PTR_WASM FFI_SINGLE void * out_ptr, FFI_PTR_WASM int32_t * out_size);
FFI_EXPORT_NVE nve_result_e nve_network_request_header_data_get(FFI_PTR_NATIVE void * request_handle, FFI_PTR_WASM FFI_SINGLE void * out_ptr, FFI_PTR_WASM int32_t * out_size);
FFI_EXPORT_NVE nve_result_e nve_network_request_error_data_get(FFI_PTR_NATIVE void * request_handle, FFI_PTR_WASM nve_network_request_error_data_t * out_error_data);

#ifdef __cplusplus
}
#endif

#endif
