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

#ifndef __NVE_PLATFORM_H__
#define __NVE_PLATFORM_H__

#include "nve_text_format.h"

#ifdef __cplusplus
extern "C" {
#endif

void nve_set_system_cc_styles(void * handle);
FFI_EXPORT_NVE bool nve_get_system_cc_styles(FFI_PTR_WASM nve_text_format_t * text_format);

#ifdef __cplusplus
}
#endif

#endif
