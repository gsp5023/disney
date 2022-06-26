/******************************************************************************
 *  Copyright (C) 2018 Broadcom.
 *  The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to
 *  the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein. IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
 *  A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME
 *  THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *  OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/
#include "nexus_platform.h"

#include "playback_context.h"

BDBG_MODULE(playback_context);
#define LOG_TAG "playback_context"
#undef LOGE
#undef LOGW
#undef LOGD
#undef LOGV
#define LOGE BDBG_ERR
#define LOGW BDBG_WRN
#define LOGD BDBG_MSG
#define LOGV BDBG_MSG

using namespace dif_streamer;
// using namespace media_parser;

PlaybackContext::PlaybackContext()
{
    for (int i = 0; i < MAX_MOSAICS; i++) {
        // parser[i] = NULL;
        // decryptor[i] = NULL;
        videoStreamer[i] = NULL;
        audioStreamer[i] = NULL;
        video_decode_hdr[i] = 0;
        nalu_len[i] = 0;
        pAvccHdr[i] = NULL;
        pAudioHeaderBuf[i] = NULL;
        pVideoHeaderBuf[i] = NULL;
        last_video_fragment_time[i] = 0;
        last_audio_fragment_time[i] = 0;
#ifdef DIF_SCATTER_GATHER
        audio_idx[i] = 0;
        video_idx[i] = 0;
        for (int j = 0; j < NUM_SAMPLES_HELD; j++) {
            audioPesBuf[i][j] = NULL;
            videoPesBuf[i][j] = NULL;
            audioDecOut[i][j] = NULL;
            videoDecOut[i][j] = NULL;
        }
#endif
    }

    bypassAudio = false;
    perf = false;
	h265Video = false;
    perfFailed = false;
    sizeDecryptedVideo = 0;
    timeDecryptVideo = 0;
    timeProcessSample = 0;
    numSamplesDecryptVideo = 0;
    numSubSamplesDecryptVideo = 0;
}

PlaybackContext::~PlaybackContext()
{
    LOGW(("%s: cleaning up", BSTD_FUNCTION));

    for (int i = 0; i < MAX_MOSAICS; i++) {
        // if(parser[i]) {
        //     delete parser[i];
        //     LOGW(("Destroying parser %p", (void*)parser[i]));
        // }

        // if (decryptor[i] != NULL) {
        //     LOGW(("Destroying decryptor %p", (void*)decryptor[i]));
        //     DecryptorFactory::DestroyDecryptor(decryptor[i]);
        // }

        if (videoStreamer[i] != NULL) {
            LOGW(("Destroying videoStreamer %p", (void*)videoStreamer[i]));
            StreamerFactory::DestroyStreamer(videoStreamer[i]);
        }

        if (audioStreamer[i] != NULL) {
            LOGW(("Destroying audioStreamer %p", (void*)audioStreamer[i]));
            StreamerFactory::DestroyStreamer(audioStreamer[i]);
        }

        if (pAvccHdr[i]) NEXUS_Memory_Free(pAvccHdr[i]);
        if (pAudioHeaderBuf[i]) NEXUS_Memory_Free(pAudioHeaderBuf[i]);
        if (pVideoHeaderBuf[i]) NEXUS_Memory_Free(pVideoHeaderBuf[i]);

#ifdef DIF_SCATTER_GATHER
        for (int j = 0; j < NUM_SAMPLES_HELD; j++) {
            if (audioPesBuf[i][j] != NULL) {
                BufferFactory::DestroyBuffer(audioPesBuf[i][j]);
            }
            if (videoPesBuf[i][j] != NULL) {
                BufferFactory::DestroyBuffer(videoPesBuf[i][j]);
            }
            if (audioDecOut[i][j] != NULL) {
                BufferFactory::DestroyBuffer(audioDecOut[i][j]);
            }
            if (videoDecOut[i][j] != NULL) {
                BufferFactory::DestroyBuffer(videoDecOut[i][j]);
            }
        }
#endif
    }

    if (perf) {
        if (perfFailed) {
            LOGE(("Could not get perf data"));
        } else {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("Total size decrypted: %lu bytes\n", sizeDecryptedVideo);
            printf("Total time to process samples: %lu msec\n", timeProcessSample/1000L);
            printf("Total time to decrypt: %lu msec\n", timeDecryptVideo/1000L);
            printf("Total #samples to decrypt: %u\n", numSamplesDecryptVideo);
            printf("Total #subsamples to decrypt: %u\n", numSubSamplesDecryptVideo);
            printf("ProcessTime/samples: %lu usec\n", timeProcessSample / numSamplesDecryptVideo);
            printf("DecTime/samples: %lu usec\n", timeDecryptVideo / numSamplesDecryptVideo);
            printf("DecTime/subsamples: %lu usec\n", timeDecryptVideo / numSubSamplesDecryptVideo);
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        }
    }
}
