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
#ifdef NXCLIENT_SUPPORT
#include "nxclient.h"
#endif

#ifdef USE_MALLOC
// malloc based allocation instead of NEXUS_Memory_Allocate
// to test decryption from non-Nexus memory
#include <stdlib.h>
#endif

BDBG_MODULE(buffer);
#undef LOGE
#undef LOGW
#undef LOGD
#undef LOGV
#define LOGE BDBG_ERR
#define LOGW BDBG_WRN
#define LOGD BDBG_MSG
#define LOGV BDBG_MSG

using namespace dif_streamer;

Buffer::~Buffer()
{
    if (!m_givenBuffer) {
#ifdef USE_MALLOC
        free(m_data);
#else
        NEXUS_Memory_Free(m_data);
#endif
        m_data = NULL;
    }
}

bool Buffer::Initialize()
{
    if (m_data == NULL) {
        uint8_t *pBuf = NULL;

#ifdef USE_MALLOC
        pBuf = (uint8_t *)malloc(m_size);
#else
        NEXUS_MemoryAllocationSettings memSettings;
        NEXUS_Memory_GetDefaultAllocationSettings(&memSettings);
        memSettings.heap = NEXUS_Heap_Lookup(NEXUS_HeapLookupType_eMain);
        NEXUS_Memory_Allocate(m_size, &memSettings, (void **)&pBuf);
#endif

        if (pBuf == NULL) {
            LOGE(("%s: Memory not allocated from Nexus heap", BSTD_FUNCTION));
            return false;
        }
        m_data = pBuf;
    }

    return true;
}

void Buffer::Copy(uint32_t offset, uint8_t* dataToCopy, uint32_t size)
{
    BKNI_Memcpy(m_data + offset, dataToCopy, size);
}

void Buffer::Copy(uint32_t offset, IBuffer* bufToCopy, uint32_t size)
{
    if(bufToCopy->IsSecure()) {
        LOGE(("%s: not allowed to copy from secure to unsecure", BSTD_FUNCTION));
    } else {
        BKNI_Memcpy(m_data + offset, bufToCopy->GetPtr(), size);
    }
}
