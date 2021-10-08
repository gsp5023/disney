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
 * @file sb_media_audio.h
 * @author Brian Hadwen (brian.hadwen@disneystreaming.com)
 * @date 24 August 2020
 * @brief Header file for steamboat aduio
 *
 * This is the header for the Steamboat Audio.
 */

#ifndef SB_MEDIA_AUDIO_H
#define SB_MEDIA_AUDIO_H

#include "sb_media_result_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets Volume Multiplier (linear) from 0 to 100
 *  where 0 mutes the output volume
 *
 * @param[in]  volume - from 0 (mute) to 100 full volume, linear
 * @return     standard sb_sb_result_t
 */
sb_media_result_t sb_media_audio_set_volume(const float volume);

#ifdef __cplusplus
}
#endif
#endif // SB_MEDIA_AUDIO_H
