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
 
#ifndef __NVE_LOGGING_H__
#define __NVE_LOGGING_H__

#ifdef __cplusplus
extern "C" {
#endif

// Implemented in nve.cc to allow access to static logging state
bool nve_get_debug_logging_enabled();

#ifdef _DEBUG
#include <stdio.h>
#define __NVE_API_DEBUG_LOG(x, ...)                                            \
    do {                                                                       \
        if (nve_get_debug_logging_enabled()) {                                 \
            printf("[NVE-API] [%s] " x "\n", __FUNCTION__, ##__VA_ARGS__);     \
            fflush(stdout);                                                    \
        }                                                                      \
    } while (0)
#else
#define __NVE_API_DEBUG_LOG(x, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif
