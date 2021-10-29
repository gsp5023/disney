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

#ifndef __NVE_UTILS_H__
#define __NVE_UTILS_H__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static inline void __nve_copy_str(char * dst, const char * src, uint32_t capacity) {
    strncpy(dst, src, capacity);
    // HACK: Cap off the end of the string if needed
    dst[capacity - 1] = '\0';
}

#endif