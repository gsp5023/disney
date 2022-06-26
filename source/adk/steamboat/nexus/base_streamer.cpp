/******************************************************************************
 *  Copyright (C) 2019 Broadcom.
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
#include "streamer.h"
#include "nexus_base_mmap.h"

BDBG_MODULE(base_streamer);
#undef LOGE
#undef LOGW
#undef LOGD
#undef LOGV
#define LOGE BDBG_ERR
#define LOGW BDBG_WRN
#define LOGD BDBG_MSG
#define LOGV BDBG_MSG

using namespace dif_streamer;

static void StreamerDataCallback(void *context, int param)
{
    StreamerEvent* stEvent = (StreamerEvent*)context;
    BKNI_SetEvent(stEvent->event);
    stEvent->userCallback.callback(stEvent->userCallback.context, param);
}

BaseStreamer::BaseStreamer()
{
    m_internallyPushed = 0;
    m_offset = 0;
    m_playpump = NULL;
    m_pidChannel = NULL;
    m_numDesc = 0;
    BKNI_CreateEvent(&m_event.event);
}

BaseStreamer::~BaseStreamer()
{
    BKNI_DestroyEvent(m_event.event);
}

void BaseStreamer::GetDefaultPlaypumpOpenSettings(
    NEXUS_PlaypumpOpenSettings *playpumpOpenSettings)
{
    if (playpumpOpenSettings == NULL) {
        LOGW(("%s: NULL pointer was given", BSTD_FUNCTION));
        return;
    }

    NEXUS_Playpump_GetDefaultOpenSettings(playpumpOpenSettings);
}

NEXUS_PlaypumpHandle BaseStreamer::OpenPlaypump(
    NEXUS_PlaypumpOpenSettings *playpumpOpenSettings)
{
    NEXUS_PlaypumpOpenSettings defaultSettings;

    if (m_playpump != NULL) {
        LOGW(("Playpump already opened"));
        return m_playpump;
    }

    if (playpumpOpenSettings == NULL) {
        LOGW(("%s: default OpenSettings is used", BSTD_FUNCTION));
        NEXUS_Playpump_GetDefaultOpenSettings(&defaultSettings);
        playpumpOpenSettings = &defaultSettings;
    }

    if (SetupPlaypump(playpumpOpenSettings) == false) {
        LOGE(("Failed to set up playpump"));
        return NULL;
    }

    m_playpump = NEXUS_Playpump_Open(NEXUS_ANY_ID, playpumpOpenSettings);
    if (!m_playpump) {
        LOGE(("@@@ Playpump Open FAILED----"));
        return NULL;
    }

    for (int i = 0; i < MAX_DESCRIPTORS; i++) {
        m_desc[i].addr = NULL;
        m_desc[i].length = 0;
    }

    return m_playpump;
}

void BaseStreamer::GetSettings(
    NEXUS_PlaypumpSettings *playpumpSettings)
{
    if (m_playpump == NULL) {
        LOGE(("%s: Playpump hasn't been opened", BSTD_FUNCTION));
        return;
    }

    NEXUS_Playpump_GetSettings(m_playpump, playpumpSettings);
}

NEXUS_Error BaseStreamer::SetSettings(
    NEXUS_PlaypumpSettings *playpumpSettings)
{
    NEXUS_PlaypumpSettings streamerSettings;

    if (m_playpump == NULL) {
        LOGE(("%s: Playpump hasn't been opened", BSTD_FUNCTION));
        return NEXUS_NOT_AVAILABLE;
    }

    // override users callback settings
    streamerSettings = *playpumpSettings;
    m_event.userCallback = playpumpSettings->dataCallback;
    streamerSettings.dataCallback.callback = StreamerDataCallback;
    streamerSettings.dataCallback.context = (void*)&m_event;
    return NEXUS_Playpump_SetSettings(m_playpump, &streamerSettings);
}

NEXUS_PidChannelHandle BaseStreamer::OpenPidChannel(unsigned pid,
    const NEXUS_PlaypumpOpenPidChannelSettings *pSettings)
{
    if (m_playpump == NULL) {
        LOGE(("Playpump hasn't been opened"));
        return NULL;
    }

    if (m_pidChannel != NULL) {
        LOGW(("PidChannel already opened"));
        return m_pidChannel;
    }

    m_pidChannel = NEXUS_Playpump_OpenPidChannel(m_playpump, pid, pSettings);
    if (SetupPidChannel() == false) {
        LOGE(("Failed to set up pid channel"));
        return NULL;
    }

    return m_pidChannel;
}

uint8_t* BaseStreamer::WaitForBuffer(uint32_t size)
{
    uint8_t *playpumpBuffer = NULL;
    size_t bufferSize = 0;

    for (;;) {
        uint32_t fragment_size = m_offset + size - m_internallyPushed;
        NEXUS_Playpump_GetBuffer(m_playpump, (void**)&playpumpBuffer, &bufferSize);
        if (bufferSize == 0) {
            BKNI_WaitForEvent(m_event.event, BKNI_INFINITE);
            continue;
        }

        if (bufferSize >= fragment_size) {
            break;
        }

        if (m_offset > 0) {
            LOGD(("%s: internally push %u", BSTD_FUNCTION, m_offset));
            NEXUS_Playpump_WriteComplete(m_playpump, 0, m_offset);
            m_internallyPushed += m_offset;
            m_offset = 0;
            continue;
        }

        if (bufferSize > 0) {
            LOGD(("%s: skip bufferSize=%u fragment_size=%u", BSTD_FUNCTION, (uint32_t)bufferSize, fragment_size));
            NEXUS_Playpump_WriteComplete(m_playpump, bufferSize, 0);
        }
    }

    return playpumpBuffer;
}

IBuffer* BaseStreamer::GetBuffer(uint32_t size)
{
    if (m_playpump == NULL) {
        LOGE(("Playpump hasn't been opened"));
        return NULL;
    }

    uint8_t* playpumpBuffer = WaitForBuffer(size);

    IBuffer* buf = CreateBuffer(size, playpumpBuffer + m_offset);
    m_offset += size;

    return buf;
}

bool BaseStreamer::SubmitScatterGather(IBuffer* buffer, bool last)
{
    if (buffer == NULL) {
        LOGE(("BaseStreamer::%s: Buffer pointer is NULL", BSTD_FUNCTION));
        return false;
    }

    if (buffer->IsSecure())
        return SubmitScatterGather((void*)buffer->GetPtr(), buffer->GetSize(), false, last);
    else
        return SubmitScatterGather((void*)buffer->GetPtr(), buffer->GetSize(), true, last);
}

bool BaseStreamer::SubmitScatterGather(void* addr, uint32_t length, bool flush, bool last)
{
    NEXUS_Error rc;
    NEXUS_PlaypumpStatus status;
    uint32_t numConsumed = 0;
    uint32_t lengthToSubmit = 0;

    if (m_playpump == NULL) {
        LOGE(("BaseStreamer::%s: Playpump hasn't been opened", BSTD_FUNCTION));
        return false;
    }
    LOGD(("BaseStreamer::%s ptr=%p len=%u flush=%d last=%d", BSTD_FUNCTION, addr, length, flush, last));

    m_desc[m_numDesc].addr = addr;
    m_desc[m_numDesc].length = (unsigned)length;
    m_flush[m_numDesc] = flush;
    m_numDesc++;

    /* if it's not last block just return */
    if (!last) return true;

retry:
    rc = NEXUS_Playpump_GetStatus(m_playpump, &status);
    if (rc != BERR_SUCCESS) {
        LOGE(("BaseStreamer::%s: Playpump_GetStatus failed rc=0x%x",
            BSTD_FUNCTION, rc));
        return false;
    }

    if (status.descFifoDepth >= status.descFifoSize / 2) {
        LOGD(("BaseStreamer::%s: retrying descFifoDepth=%u", BSTD_FUNCTION, (unsigned)status.descFifoDepth));
        BKNI_WaitForEvent(m_event.event, BKNI_INFINITE);
        goto retry;
    }

    /* Flush if source is GLR and destination is secure */
    for (unsigned i = 0; i < m_numDesc; i++) {
        if (m_flush[i] && IsSecure()) {
            NEXUS_FlushCache(m_desc[i].addr, m_desc[i].length);
        }
        lengthToSubmit += m_desc[i].length;
        LOGD(("%s: m_desc[%d] length=%d addr=%p (offset=%p) flush=%d", __FUNCTION__,
            i, m_desc[i].length, (void*)m_desc[i].addr, (void*)NEXUS_AddrToOffset(m_desc[i].addr), m_flush[i]));
    }

    rc = NEXUS_Playpump_SubmitScatterGatherDescriptor(m_playpump, m_desc, m_numDesc, (size_t*)&numConsumed);
    if (rc != BERR_SUCCESS) {
        LOGE(("BaseStreamer::%s: SubmitScatterGather failed rc=0x%x",
            BSTD_FUNCTION, rc));
        m_numDesc = 0;
        return false;
    }

    if (numConsumed < m_numDesc) {
        LOGE(("BaseStreamer::%s: numConsumed didn't match: %u/%u",
            BSTD_FUNCTION, numConsumed, m_numDesc));
        // Not clear what to do for this case
        m_numDesc = 0;
        return false;
    }

    m_numDesc = 0;

    // Need to call Playpump_GetBuffer
    WaitForBuffer(lengthToSubmit);

    return true;
}

bool BaseStreamer::SubmitSample(SampleInfo *pSample, IBuffer *clear, IBuffer *decrypted)
{
    if (m_playpump == NULL) {
        LOGE(("Playpump hasn't been opened"));
        return false;
    }

    if (pSample == NULL || decrypted == NULL) {
        LOGE(("%s: parameters are not set correctly", BSTD_FUNCTION));
        return false;
    }

    LOGD(("SampleSize=%d entries=%d", pSample->size, pSample->nbOfEntries));
    if (pSample->nbOfEntries == 0) {
        // Full sample case - assuming fully encrypted
        SubmitScatterGather(decrypted->GetPtr(),
                pSample->size, !decrypted->IsSecure(), true);
        return true;
    }

    if (clear == NULL) {
        LOGE(("%s: need clear for subsamples", BSTD_FUNCTION));
        return false;
    }

    uint32_t clearSize = 0;
    uint32_t encSize = 0;
    bool last = false;
    // Submit each piece from clear and decrypted one by one
    // assuming clear data comes first in subsamplesr
    for (int i = 0; i < pSample->nbOfEntries; i++ ) {
        if (pSample->entries[i].bytesOfClearData > 0) {
            if (i >= pSample->nbOfEntries - 1 &&
                pSample->entries[i].bytesOfEncData == 0)
                last = true;
            LOGD(("Submit clear entry=%d size=%d last=%d", i,
                pSample->entries[i].bytesOfClearData, last));
            // Submit clear subsample
            SubmitScatterGather(clear->GetPtr() + clearSize + encSize,
                pSample->entries[i].bytesOfClearData, true, last);
            clearSize += pSample->entries[i].bytesOfClearData;
        }

        if (pSample->entries[i].bytesOfEncData > 0) {
            if (i >= pSample->nbOfEntries - 1)
                last = true;
            LOGD(("Submit decrypted entry=%d size=%d last=%d", i,
                pSample->entries[i].bytesOfEncData, last));
            // Submit decrypted subsample
            SubmitScatterGather(decrypted->GetPtr() + clearSize + encSize,
                pSample->entries[i].bytesOfEncData, !IsSecure(), last);
            encSize += pSample->entries[i].bytesOfEncData;
        }
    }

    return true;
}

bool BaseStreamer::Push(uint32_t size)
{
    if (m_playpump == NULL) {
        LOGE(("Playpump hasn't been opened"));
        return false;
    }

    LOGD(("BaseStreamer::%s size=%u", BSTD_FUNCTION, size));
    NEXUS_Error rc = NEXUS_SUCCESS;
    if (size > m_internallyPushed) {
        rc = NEXUS_Playpump_WriteComplete(m_playpump, 0, size - m_internallyPushed);
        m_internallyPushed = 0;
    } else
        LOGW(("BaseStreamer::%s warning size=%u < internallyPushed=%u", BSTD_FUNCTION, size, m_internallyPushed));

    m_offset = 0;
    return (rc == NEXUS_SUCCESS);
}
