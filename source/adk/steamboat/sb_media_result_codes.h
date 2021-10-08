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
 * @file sb_media_result_codes.h
 * @author Brian Hadwen (brian.hadwen@disneystreaming.com)
 * @date 27 May 2020
 * @brief Header file for steamboat player
 *
 * This is the header for the Steamboat Player.
 * The Steamboat player is a higher level API that allows a player to play from
 * a URL
 */

#ifndef SB_MEDIA_RESULT_CODES_H
#define SB_MEDIA_RESULT_CODES_H

#include <stdint.h>
typedef int32_t sb_media_result_t;
#define SB_MEDIA_RESULT_SUCCESS 			(sb_media_result_t)0x00000000 	/**< Successful */
#define SB_MEDIA_RESULT_NOT_CAPABLE 		(sb_media_result_t)0x00000001 	/**< Not Capable */
#define SB_MEDIA_RESULT_FAIL   			    (sb_media_result_t)0x00000002 	/**< Fail */
#define SB_MEDIA_RESULT_FORMAT   			(sb_media_result_t)0x00000003 	/**< Format Error */
#define SB_MEDIA_RESULT_OVERFLOW 			(sb_media_result_t)0x00000004 	/**< Buffer Overflow */
#define SB_MEDIA_RESULT_NOT_SUPPORTED 	    (sb_media_result_t)0x00000005 	/**< Call not supported */
#define SB_MEDIA_RESULT_NOT_RUNNING   	    (sb_media_result_t)0x00000006 	/**< Decoder not running */
#define SB_MEDIA_RESULT_EINVAL        	    (sb_media_result_t)0x00000006 	/**< Bad input value */

#endif /* SB_MEDIA_RESULT_CODES_H */
