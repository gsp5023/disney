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
 * @file sb_media_output.h
 * @author Brian Hadwen (brian.hadwen@disneystreaming.com)
 * @date 26 August 2020
 * @brief Header file for steamboat media log outputs
 *
 * This is the header for the logging output for steamboat media.
 */
#ifndef SB_MEDIA_OUTPUT_H
#define SB_MEDIA_OUTPUT_H

extern "C" void sb_media_extern_output(const char * format, ...);

#ifdef FORCE_SB_MEDIA_OUTPUT_PRINTF
#    include <stdio.h>
#    define SB_MEDIA_OUTPUT(frmt, ...) fprintf(stdout,frmt, ##__VA_ARGS__); fflush(stdout);
#else
#    define SB_MEDIA_OUTPUT sb_media_extern_output
#endif

#endif /* SB_MEDIA_OUTPUT_H */

