/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
sb_media_stub.c

Stubs for minimum required methods to implement NVE playback
*/

#include "sb_media_decoder.h"
#include "sb_media_audio.h"
#include "source/adk/log/log.h"

#include <nexus_config.h>
#include <nexus_platform.h>
#include "nxclient.h"
#include "nexus_playback.h"

#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <condition_variable>

#include <open_cdm.h>
#include <open_cdm_ext.h>
#include <open_cdm_adapter.h>

#include "nexus/streamer.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

extern "C" {
#include <curl/curl.h>
#include "curl_base64.h"
}

#define TAG_RT_MEDIA FOURCC('S', 'T', 'M', 'E')
#define BRCM_PTS_TO_NANO_SECONDS(a) (a*1000000/45)
#define NANO_SECONDS_TO_BRCM_PTS(a) (a*45/1000000)
static void print_adts(int size);

#define VIDEO_PLAYPUMP_BUF_SIZE (1024 * 1024 * 2) /* 2MB */
#define AUDIO_PLAYPUMP_BUF_SIZE (2048 * 32)

using namespace dif_streamer;

enum {
    BRCM_AUDIO_STREAM_ID = 0xC0,
    BRCM_VIDEO_STREAM_ID = 0xE0,
    AUDIO_LOOP_BUFFER_SIZE = 0x100000*5,
    VIDEO_LOOP_BUFFER_SIZE = 0x500000*5,
};

template <typename Decoder> class NexusSteamboatPlayHelper {
public:
    NexusSteamboatPlayHelper() : streamer(nullptr) {
        stopped = false;
    }

    ~NexusSteamboatPlayHelper() {
        deinit();
    }

    void init(int bufferSize, bool secure = false) {
        this->secure = secure;
        this->BUFFER_SIZE = bufferSize;
        streamer = StreamerFactory::CreateStreamer(!secure);
        event = NEXUS_Event_Create();
        lastConsumedPts = 0;
        lastBufferedPts = 0;
        bufferedBytes = 0;
        if (streamer) {
            NEXUS_PlaypumpOpenSettings playpumpOpenSettings;
            streamer->GetDefaultPlaypumpOpenSettings(&playpumpOpenSettings);
            playpumpOpenSettings.fifoSize = BUFFER_SIZE;
            playpump = streamer->OpenPlaypump(&playpumpOpenSettings);

            NEXUS_PlaypumpSettings playpumpSettings;
            streamer->GetSettings(&playpumpSettings);
            playpumpSettings.dataCallback.callback = [] (void* context, int param) {
                NEXUS_Event_Signal((NEXUS_EventHandle)context);
            };
            playpumpSettings.dataCallback.context = event;
            playpumpSettings.transportType = NEXUS_TransportType_eMpeg2Pes;
            streamer->SetSettings(&playpumpSettings);

            NEXUS_PlaypumpOpenPidChannelSettings pidSettings;
            NEXUS_Playpump_GetDefaultOpenPidChannelSettings(&pidSettings);
            pidSettings.pidType = (BUFFER_SIZE == VIDEO_PLAYPUMP_BUF_SIZE) ? NEXUS_PidType_eVideo : NEXUS_PidType_eAudio;
            pidChannel = streamer->OpenPidChannel((BUFFER_SIZE == VIDEO_PLAYPUMP_BUF_SIZE) ? BRCM_VIDEO_STREAM_ID : BRCM_AUDIO_STREAM_ID, &pidSettings);
        }
    }

    void deinit() {
        if (streamer) {
            StreamerFactory::DestroyStreamer(streamer);
            streamer = nullptr;
        }
        if (event) {
            NEXUS_Event_Destroy(event);
            event = nullptr;
        }
    }

    struct Buffer {
        Buffer(uint8_t* b, int len, uint64_t pts) : buffer(nullptr), len(0), encrypted(nullptr), encryptedLength(0), pts(pts) {
            if (len > 0 && b != nullptr) {
                buffer = (uint8_t*)malloc(len);
                memcpy(buffer, b, len);
                this->len = len;
            }
        }
        Buffer(uint8_t* b, int len, uint8_t* encrypted, int encLen, uint64_t pts) : buffer(nullptr), len(0), encrypted(encrypted), encryptedLength(encLen), pts(pts) {
            if (len > 0 && b != nullptr) {
                buffer = (uint8_t*)malloc(len);
                memcpy(buffer, b, len);
                this->len = len;
            }
        }
        ~Buffer() {
            if (buffer) {
                free(buffer);
            }
        }
        uint8_t* buffer;
        int len;
        uint8_t* encrypted;
        int encryptedLength;
        uint64_t pts;
    };

    Decoder decoder;
    NEXUS_PlaypumpHandle playpump;
    NEXUS_PidChannelHandle pidChannel;
    NEXUS_EventHandle event;
    sb_media_decoder_handle steamboatDecoderHandle;
    std::thread thread;
    std::mutex lock;
    std::condition_variable waitForData;
    std::deque<std::shared_ptr<Buffer>> buffers;
    bool stopped;
    uint64_t lastConsumedPts;
    uint64_t lastBufferedPts;
    int bufferedBytes;
    bool paused = false;
    IStreamer* streamer;
    bool secure;
    uint32_t BUFFER_SIZE;
    
    Decoder getDecoder() {
        return decoder;
    }


    void start() {
        paused = false;
        NEXUS_Playpump_Start(playpump);
    }

    void pause() {
        LOG_DEBUG(TAG_RT_MEDIA, "Start pause..");
        std::unique_lock<std::mutex> lk(lock);
        buffers.clear();
        lastConsumedPts = 0;
        lastBufferedPts = 0;
        bufferedBytes = 0;
        paused = true;
        stopped = true;
        streamer->Wakeup();
        waitForData.notify_one();
        lk.unlock();
        LOG_DEBUG(TAG_RT_MEDIA, "Start pause join");
        thread.join();
        LOG_DEBUG(TAG_RT_MEDIA, "End pause join");
    }

    int push(uint8_t* buf, int len, uint64_t pts) {
        std::unique_lock<std::mutex> lk(lock);
        buffers.emplace_back(new Buffer(buf, len, pts));
        lastBufferedPts = pts;
        bufferedBytes += len;
        waitForData.notify_one();
        return buffers.size();
    }

    int push(uint8_t* buf, int len, uint8_t* encrypted, int encryptedLen, uint64_t pts) {
        std::unique_lock<std::mutex> lk(lock);
        buffers.emplace_back(new Buffer(buf, len, encrypted, encryptedLen, pts));
        lastBufferedPts = pts;
        bufferedBytes += (len + encryptedLen);
        waitForData.notify_one();
                    fprintf(stderr, "%s %s %d Decode buffer count %d\n", __FILE__, __func__, __LINE__, buffers.size());
        return buffers.size();
    }

    int64_t getBufferedRangeMs() {
        return (lastBufferedPts - lastConsumedPts)/10000000;
    }

    void run() {
        stopped = false;
        thread = std::thread([this]{
            const size_t COUNT = (BUFFER_SIZE == VIDEO_PLAYPUMP_BUF_SIZE) ? 1 : 1;
            while (!stopped) {
                std::unique_lock<std::mutex> lk(lock);
                if (buffers.size() < COUNT && !stopped) {
                    waitForData.wait(lk, [this, COUNT] {
                        return ((buffers.size() >= COUNT) || stopped);
                    });
                }
                if (stopped) {
                    break;
                }

                auto buf = buffers.front();
                buffers.pop_front();
                lastConsumedPts = buf->pts;
                bufferedBytes -= (buf->len + buf->encryptedLength);      
                lk.unlock();

                if (stopped) {
                    break;
                }

                int written = 0;
                /*
                print_adts(buf->len);

                LOG_DEBUG(TAG_RT_MEDIA, "Buffer %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                buf->buffer[0], buf->buffer[1], buf->buffer[2], buf->buffer[3], buf->buffer[4], buf->buffer[5], buf->buffer[6], buf->buffer[7],
                buf->buffer[8+0], buf->buffer[8+1], buf->buffer[8+2], buf->buffer[8+3], buf->buffer[8+4], buf->buffer[8+5], buf->buffer[8+6], buf->buffer[8+7],
                buf->buffer[16+0], buf->buffer[16+1], buf->buffer[16+2], buf->buffer[16+3], buf->buffer[16+4], buf->buffer[16+5], buf->buffer[16+6], buf->buffer[16+7]);
                */


               {
                    if (buf->len > 0 && buf->buffer != nullptr) {
                        IBuffer* data = BufferFactory::CreateBuffer(buf->len, nullptr);
                        data->Copy(0, buf->buffer, buf->len);
                        IBuffer* input = streamer->GetBuffer(buf->len);
                        input->Copy(0, data, buf->len);
                        streamer->Push(buf->len);
                        BufferFactory::DestroyBuffer(input);
                        BufferFactory::DestroyBuffer(data);
                    }

                    if (buf->encrypted != nullptr && buf->encryptedLength > 0) {
                        IBuffer* data = BufferFactory::CreateBuffer(buf->encryptedLength, buf->encrypted, true);
                        IBuffer* input = streamer->GetBuffer(buf->encryptedLength);
                        input->Copy(0, data, buf->encryptedLength);
                        streamer->Push(buf->encryptedLength);
                        BufferFactory::DestroyBuffer(input);
                        BufferFactory::DestroyBuffer(data);

                        if (buf->encrypted) {
                            NEXUS_Memory_Free(buf->encrypted);
                        }
                    }
               }
            }
        });
    }
};

class OpenCDMContext {
    public:
    private:
    
};

static std::string parseTag(const std::string content, const std::string tag){
    std::string startTag = "<" + tag + ">";
    std::size_t found = content.find(startTag);
    if (found != std::string::npos) {
        std::string endTag = "</" + tag + ">";
        found += startTag.length();
        std::size_t endTagFound = content.find(endTag, found + 1);
        if (endTagFound != std::string::npos) {
            return content.substr(found, endTagFound - found);
        }
    }
    return "";
}

struct DRMSession {
    enum class KeyState {
	// Has been initialized.
	KEY_INIT = 0,
	// Has a key message pending to be processed.
	KEY_PENDING = 1,
	// Has a usable key.
	KEY_READY = 2,
	// Has an error.
	KEY_ERROR = 3,
	// Has been closed.
	KEY_CLOSED = 4,
	// Has Empty DRM session id.
	KEY_ERROR_EMPTY_SESSION_ID = 5
    };

    OpenCDMSystem* system;
    OpenCDMSession* session;
    KeyState keyState;
    sb_media_challenge_callback challengeCallback;
    OpenCDMSessionCallbacks ocdmCallbacks;
    bool challengeProcessed;
    std::condition_variable waitForKeyUpdate;
    std::mutex lock;
    std::string licenseNonce;
    int keySize;
    std::vector<std::vector<uint8_t>> keyData;
    uint8_t KID[16];
    std::vector<uint8_t> initData;
    std::vector<uint8_t> responseData;
    std::vector<uint8_t> challengeData;

    DRMSession (OpenCDMSystem* system, sb_media_challenge_callback callback) : system(system), session(nullptr), keyState(KeyState::KEY_INIT), 
                challengeCallback(callback), challengeProcessed(false) {}

    ~DRMSession () {
        if (session) {
            opencdm_destruct_session(session);
        }
    }

    bool init(const uint8_t* initData, uint32_t initDataSize) {

        this->initData = std::vector<uint8_t>(initData, initData + initDataSize);

		memset(&ocdmCallbacks, 0, sizeof(ocdmCallbacks));
		ocdmCallbacks.process_challenge_callback = [](OpenCDMSession* session, void* userData, const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize) {
            LOG_DEBUG(TAG_RT_MEDIA, "process_challenge_callback");
			DRMSession* userSession = reinterpret_cast<DRMSession*>(userData);
            {
                FILE* fp = fopen("/userdata/challenge2.bin", "w");
                if (fp) {
                    fwrite(challenge, 1, challengeSize, fp);
                    fclose(fp);
                }
            }
			userSession->processOCDMChallenge(destUrl, challenge, challengeSize);
		};

		ocdmCallbacks.key_update_callback = [](OpenCDMSession* session, void* userData, const uint8_t key[], const uint8_t keySize) {
            LOG_DEBUG(TAG_RT_MEDIA, "key_update_callback" );
			DRMSession* userSession = reinterpret_cast<DRMSession*>(userData);
			userSession->keyUpdateOCDM(key, keySize);
		};

		ocdmCallbacks.error_message_callback = [](OpenCDMSession* session, void* userData, const char message[]) {
            DRMSession* userSession = reinterpret_cast<DRMSession*>(userData);
            userSession->keyState = KeyState::KEY_ERROR;
            LOG_DEBUG(TAG_RT_MEDIA, "error_message_callback %s, %s", message, userSession->licenseNonce.c_str());

		};

		ocdmCallbacks.keys_updated_callback = [](const OpenCDMSession* session, void* userData) {
            LOG_DEBUG(TAG_RT_MEDIA, "keys_updated_callback");
			DRMSession* userSession = reinterpret_cast<DRMSession*>(userData);
			userSession->keysUpdatedOCDM();
		};
/*
        static const uint8_t playreadySystemId[] = {
        0x9A, 0x04, 0xF0, 0x79, 0x98, 0x40, 0x42, 0x86,
        0xAB, 0x92, 0xE6, 0x5B, 0xE0, 0x88, 0x5F, 0x95,
        };        

        int ocdmInitSize = 32 + initDataSize;
        uint8_t* ocdmInitData = (uint8_t*)malloc(ocdmInitSize);
        memset(ocdmInitData, 0, ocdmInitSize);
        ocdmInitData[0] = ((ocdmInitSize >> 24)& 0xff);
        ocdmInitData[1] = ((ocdmInitSize >> 16) & 0xff);
        ocdmInitData[2] = ((ocdmInitSize >> 8) & 0xff);
        ocdmInitData[3] = ((ocdmInitSize >> 0) & 0xff);
        ocdmInitData[4] = 'p';
        ocdmInitData[5] = 's';
        ocdmInitData[6] = 's';
        ocdmInitData[7] = 'h';
        memcpy(ocdmInitData + 12, playreadySystemId, 16);
        ocdmInitData[28] = ((initDataSize >> 24) & 0xff);
        ocdmInitData[29] = ((initDataSize >> 16) & 0xff);
        ocdmInitData[30] = ((initDataSize >> 8) & 0xff);
        ocdmInitData[31] = ((initDataSize >> 0) & 0xff);
        memcpy(ocdmInitData + 32, initData, initDataSize);
*/
        int ocdmInitSize = 10 + initDataSize;
        uint8_t* ocdmInitData = (uint8_t*)malloc(ocdmInitSize);
        memset(ocdmInitData, 0, ocdmInitSize);
        ocdmInitData[0] = ((ocdmInitSize >> 0) & 0xff);
        ocdmInitData[1] = ((ocdmInitSize >> 8) & 0xff);
        ocdmInitData[2] = ((ocdmInitSize >> 16) & 0xff);
        ocdmInitData[3] = ((ocdmInitSize >> 24) & 0xff);
        ocdmInitData[4] = 0x1; // RecordCount = 1
        ocdmInitData[5] = 0x0; 
        ocdmInitData[6] = 0x1; // Type - 1
        ocdmInitData[7] = 0x0; 
        ocdmInitData[8] = initDataSize & 0xff;
        ocdmInitData[9] = (initDataSize >> 8) & 0xff;
        memcpy(ocdmInitData + 10, initData, initDataSize);

        OpenCDMError ocdmRet = opencdm_construct_session(system, LicenseType::Temporary, "video/mp4",
                    const_cast<unsigned char*>(ocdmInitData), ocdmInitSize,
                    nullptr, 0, //No Custom Data
                    &ocdmCallbacks,
                    static_cast<void*>(this),
                    &session);     

        free(ocdmInitData);

		if (ocdmRet != ERROR_NONE) {
			LOG_ERROR(TAG_RT_MEDIA, "Error constructing OCDM session. OCDM err=0x%x", ocdmRet);
			keyState = KeyState::KEY_ERROR;
            return false;
		}                      

        keyState = KeyState::KEY_INIT;
        return true;
    }

	void processOCDMChallenge(const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize);

    void update(const uint8_t* response, int response_size) {
        LOG_DEBUG(TAG_RT_MEDIA, "update");
        opencdm_session_set_drm_header(session, initData.data(), initData.size());
        opencdm_session_update(session, response, response_size);
        responseData = std::vector<uint8_t>(response, response + response_size);
        LOG_DEBUG(TAG_RT_MEDIA, "update memcmp %d", memcmp(responseData.data(), response, response_size));
        char filename[256];
        sprintf(filename, "/userdata/%s-%02x%02x.bin", licenseNonce.c_str(), KID[0], KID[1]);
        FILE* fp = fopen (filename, "w");
        if (fp){
            fwrite(response, 1, response_size, fp);
            fclose(fp);
        }
    }

    void waitForKey() {
        std::unique_lock<std::mutex> lk(lock);
        if (!challengeProcessed) {
            waitForKeyUpdate.wait_for(lk, std::chrono::milliseconds(5000), [this]() {return challengeProcessed;});
        }
    }

	void keysUpdatedOCDM() {
        LOG_DEBUG(TAG_RT_MEDIA, "Key updated!! ");
    }

	void keyUpdateOCDM(const uint8_t key[], const uint8_t keySize){
        LOG_DEBUG(TAG_RT_MEDIA, "KeyUpdated keySize %d (%02x %02x %02x %02x %02x %02x %02x %02x)licenseNonce %s", keySize, key[0], key[1], key[2], key[3],
            key[4], key[5], key[6], key[7], licenseNonce.c_str());
        this->keySize = keySize;
        
        if (!hasKey(key, keySize)) {
            std::vector<uint8_t> newKey(keySize);
            memcpy(newKey.data(), key, keySize);
            keyData.push_back(newKey);
            LOG_DEBUG(TAG_RT_MEDIA, "Check hasKey %d", hasKey(key, keySize));
        }

        keyState = KeyState::KEY_READY;
        // if (memcmp(key, KID, sizeof(copySize))) {
        //     LOG_ERROR(TAG_RT_MEDIA, "Key different.. %02x %02x %02x %02x %02x %02x %02x %02x", KID[0], KID[1], KID[2], KID[3], KID[4], KID[5], KID[6], KID[7]);
        // } else {
        //     LOG_ERROR(TAG_RT_MEDIA, "Key same");
        // }
    }

    bool hasKey(const uint8_t key[], size_t keyLength) {
        if (keyState == KeyState::KEY_ERROR) {
            LOG_ERROR(TAG_RT_MEDIA, "Key state error.. ");
            return false;
        }
        for (auto& k : keyData) {
            LOG_DEBUG(TAG_RT_MEDIA, "size %d %d", k.size(), keyLength);
            if (k.size() == keyLength) {
                LOG_DEBUG(TAG_RT_MEDIA, "memcmp %d", memcmp(k.data(), key, keyLength));
                return (memcmp(k.data(), key, keyLength) == 0);
            }
        }
        return false;
    }
};

struct DRMSystem {
    DRMSystem() : drmType(SB_MEDIA_DRM_NONE), ocdmSystem(nullptr) {}
    sb_media_drm_flag_t drmType;
    struct OpenCDMSystem* ocdmSystem;
    std::vector<std::shared_ptr<DRMSession>> ocdmSessions;
    std::shared_ptr<DRMSession> ocdmCurrentSession;
    std::thread initDrmThread;
    const int INIT_DRM_TIMEOUT_MS = 10000;

    int init(sb_media_drm_flag_t drm) {
        LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d DRM %d", __FILE__, __func__, __LINE__, drm);
        if (drmType != drm) {
            if (ocdmSystem != nullptr) {
                LOG_DEBUG(TAG_RT_MEDIA, "DRM changed %d --> %d system %p", ocdmSystem, drm, ocdmSystem);
                if (ocdmSystem) {
                    shutdown();
                    ocdmSystem = nullptr;
                }
            }

            drmType = SB_MEDIA_DRM_NONE;

            if (drm == SB_MEDIA_DRM_PLAYREADY) {
                LOG_DEBUG(TAG_RT_MEDIA, "SB_MEDIA_DRM_PLAYREADY");
                ocdmSystem = opencdm_create_system("com.microsoft.playready");
            } else if (drm == SB_MEDIA_DRM_WIDEVINE) {
                LOG_DEBUG(TAG_RT_MEDIA, "SB_MEDIA_DRM_WIDEVINE");
                ocdmSystem = opencdm_create_system("com.widevine.alpha");
            } else if (drm == SB_MEDIA_DRM_NONE){
                LOG_DEBUG(TAG_RT_MEDIA, "SB_MEDIA_DRM_NONE - DRM destroyed");
                return SB_MEDIA_RESULT_SUCCESS;
            } else {
                return SB_MEDIA_RESULT_NOT_SUPPORTED;
            }
            
            drmType = drm;
        }
        return SB_MEDIA_RESULT_SUCCESS;        
    }

    void shutdown() {
        for (auto& session : ocdmSessions) {
            session.reset();
        }
        ocdmSessions.clear();
        opencdm_destruct_system(ocdmSystem);
    }


    void initPlayReadySessions(const uint8_t* drmHeader, int initDataLength, sb_media_challenge_callback callback) {
        std::vector<uint8_t> vec(initDataLength);
        uint8_t* initData = vec.data();
        memcpy(initData, drmHeader, initDataLength);
        int remainedSize = initDataLength;
        int offset = 0;
        int data = 0;
        #define READ_DWORD() { \
            if (remainedSize <= 4) { LOG_ERROR(TAG_RT_MEDIA, "Can not read 4 bytes!!"); return; } \
            remainedSize -= 4; \
            data = (initData[offset] << 0) | (initData[offset + 1] << 8) | (initData[offset + 2] << 16) | (initData[offset + 3] << 24); \
            offset += 4; \
        };
        #define READ_WORD() { \
            if (remainedSize <= 2) { LOG_ERROR(TAG_RT_MEDIA, "Can not read 2 bytes!!"); return; } \
            remainedSize -= 2; \
            data = (initData[offset] << 0) | (initData[offset + 1] << 8); \
            offset += 2; \
        };

        READ_DWORD();
        LOG_DEBUG(TAG_RT_MEDIA, "Total length %d %d", data, initDataLength);
        if (data != initDataLength) {
            LOG_ERROR(TAG_RT_MEDIA, "Invalid length? %d %d", data, initDataLength);
        }
        READ_WORD();
        LOG_DEBUG(TAG_RT_MEDIA, "PRO has %d objects", data);
        int objCount = data;

        uint8_t assembledBuffer[64*1024];
        int assembledLength = 0;
        uint8_t* nextKIDStartPoint = nullptr;

        for (int i = 0; i < objCount; i++) {
            READ_WORD();
            int recordType = data;
            READ_WORD();
            int recordLength = data;
            LOG_DEBUG(TAG_RT_MEDIA, "RecordType %d RecordLength %d", recordType, recordLength);
            if (recordLength > remainedSize) {
                LOG_ERROR(TAG_RT_MEDIA, "Insufficient length!!! %d %d", recordLength, remainedSize);
                return;
            }

            if (recordLength <= 0) {
                LOG_ERROR(TAG_RT_MEDIA, "Error invalid recordLength");
                break;
            }

            const uint8_t kidPattern[] = {0x3c, 0x00,  0x4b, 0x00, 0x49, 0x00, 0x44, 0x00, 0x3e, 0x00};
            const uint8_t kidEndPattern[] = {0x3c, 0x00, 0x2f, 0x00, 0x4b, 0x00, 0x49, 0x00, 0x44, 0x00, 0x3e, 0x00};

            const uint8_t* kidStart = (uint8_t*)memmem(initData + offset, recordLength, kidPattern, sizeof(kidPattern));  
            const uint8_t* kidEnd = (uint8_t*)memmem(initData + offset, recordLength, kidEndPattern, sizeof(kidEndPattern));        
            uint8_t in[256];
            memset(in, 0, sizeof(256));
            int len = 0;
            for (const uint8_t* start = kidStart + 10; start < kidEnd; start += 2) {
                in[len] = *start;
                len++;
            }

            uint8_t* out = nullptr;
            size_t base64Out = 256;
            Curl_base64_decode((const char*)in, (uint8_t**)&out, &base64Out);

            LOG_DEBUG(TAG_RT_MEDIA, "Session Init i = %d", i);
            std::shared_ptr<DRMSession> session = std::make_shared<DRMSession>(ocdmSystem, callback);

            if (out != nullptr) {
                LOG_DEBUG(TAG_RT_MEDIA, "KID %s outlen : %d %02x %02x %02x %02x", in, base64Out, out[0], out[1], out[2], out[3]);
                if (base64Out == 16) {
                    for (int i = 0; i < 4; i += 4) {
                        session->KID[i] = out[i+3];
                        session->KID[i+1] = out[i+2];
                        session->KID[i+2] = out[i+3];
                        session->KID[i+3] = out[i];
                    }
                }
            }

            if (session->init(initData + offset, recordLength)) {
                ocdmSessions.push_back(session);    
                LOG_DEBUG(TAG_RT_MEDIA, "WaitForKey...");
                session->waitForKey();
            }

            remainedSize -= recordLength;
            offset += recordLength;
        }         
    }
};

static struct
{
    NxClient_AllocResults allocResults;
    NEXUS_SimpleStcChannelHandle stcChannel;
    NexusSteamboatPlayHelper<NEXUS_SimpleAudioDecoderHandle> audioPlayer;
    NexusSteamboatPlayHelper<NEXUS_SimpleVideoDecoderHandle> videoPlayer; 
    sb_media_audio_config_t audioConfig;
    sb_media_video_config_t videoConfig;
    NEXUS_SimpleAudioDecoderStartSettings audioStartSettings;
    NEXUS_SimpleVideoDecoderStartSettings videoStartSettings;

    unsigned connectId;

    bool isAudioDecoder(sb_media_decoder_handle handle) {
        return audioPlayer.steamboatDecoderHandle == handle;
    }

    bool isVideoDecoder(sb_media_decoder_handle handle) {
        return videoPlayer.steamboatDecoderHandle == handle;
    }

    NEXUS_SimpleAudioDecoderHandle getAudioDecoder() {
        return audioPlayer.getDecoder();
    }

    NEXUS_SimpleVideoDecoderHandle getVideoDecoder() {
        return videoPlayer.getDecoder();
    }

    void pauseAudio() {
        LOG_DEBUG(TAG_RT_MEDIA, "pauseAudio");
        NEXUS_SimpleAudioDecoder_Stop(audioPlayer.getDecoder());
        audioPlayer.pause();
        audioPlayer.deinit();
    }
    
    void pauseVideo() {
        LOG_DEBUG(TAG_RT_MEDIA, "pauseVideo");
        NEXUS_SimpleVideoDecoder_Stop(videoPlayer.getDecoder());
        videoPlayer.pause();
        videoPlayer.deinit();
        NEXUS_SimpleStcChannel_Invalidate(stcChannel);
    }

    void resumeAudio(uint64_t pts) {
        LOG_DEBUG(TAG_RT_MEDIA, "resumeAudio %lld", pts);
        audioPlayer.init(AUDIO_PLAYPUMP_BUF_SIZE, true);
        audioPlayer.start();
        // NEXUS_SimpleAudioDecoder_SetStcChannel(audioPlayer.getDecoder(), stcChannel);
        audioStartSettings.primary.pidChannel = audioPlayer.pidChannel;
        NEXUS_SimpleAudioDecoder_Start(audioPlayer.getDecoder(), &audioStartSettings);
        audioPlayer.lastBufferedPts = pts;
        audioPlayer.lastConsumedPts = pts;
        audioPlayer.run();
    }

    void resumeVideo(uint64_t pts) {
        LOG_DEBUG(TAG_RT_MEDIA, "resumeVideo %lld", pts);
        videoPlayer.init(VIDEO_PLAYPUMP_BUF_SIZE, true);
        videoPlayer.start();
        NEXUS_SimpleStcChannel_Invalidate(stcChannel);
         // NEXUS_SimpleVideoDecoder_SetStcChannel(videoPlayer.getDecoder(), stcChannel);
        // NEXUS_SimpleVideoDecoder_SetStartPts(videoPlayer.getDecoder(), NANO_SECONDS_TO_BRCM_PTS(pts));
        videoStartSettings.settings.pidChannel = videoPlayer.pidChannel;
        NEXUS_SimpleVideoDecoder_Start(videoPlayer.getDecoder(), &videoStartSettings);
        videoPlayer.lastBufferedPts = pts;
        videoPlayer.lastConsumedPts = pts;
        videoPlayer.run();
    }
    
    NEXUS_ClientConfiguration clientConfig;
    DRMSystem drmSystem;
    sb_media_event_callback event_callback;

    struct OpenCDMSystem* getOcdmSystem() {
        return drmSystem.ocdmSystem;
    }
} statics;

void DRMSession::processOCDMChallenge(const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize)
{
    LOG_DEBUG(TAG_RT_MEDIA, "processOCDMChallenge Size %d TID %d", challengeSize, syscall(__NR_gettid));
    licenseNonce = parseTag((const char*)challenge, "LicenseNonce");
    LOG_DEBUG(TAG_RT_MEDIA, "LicenseNonce %s", licenseNonce.c_str());
    
    LOG_DEBUG(TAG_RT_MEDIA, "Calling ChallengeCallback");
    challengeCallback(challenge, challengeSize);
    
    statics.event_callback(statics.videoPlayer.steamboatDecoderHandle, SB_MEDIA_PLAY_EVENT_REQUEST_KEY);
    challengeData = std::vector<uint8_t>(challenge, challenge + challengeSize);

    std::unique_lock<std::mutex> lk(lock);
    challengeProcessed = true;
    waitForKeyUpdate.notify_one();
}


sb_media_result_t sb_media_global_init()
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_global_init" );   
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings(&joinSettings);

    strcpy(joinSettings.name, "Disney+");
    NEXUS_Error rc = NxClient_Join(&joinSettings);
    BDBG_ASSERT(!rc);

    NxClient_AllocSettings allocSettings;
    NxClient_GetDefaultAllocSettings(&allocSettings);
    allocSettings.simpleVideoDecoder = 1;
    allocSettings.simpleAudioDecoder = 1;
    rc = NxClient_Alloc(&allocSettings, &statics.allocResults);

    statics.stcChannel = NEXUS_SimpleStcChannel_Create(NULL);
    NEXUS_SimpleStcChannelSettings stcSettings;
    NEXUS_SimpleStcChannel_GetSettings(statics.stcChannel, &stcSettings);
    stcSettings.mode = NEXUS_StcChannelMode_eAuto;
    stcSettings.modeSettings.Auto.transportType = NEXUS_TransportType_eMpeg2Pes;
    stcSettings.modeSettings.Auto.behavior = NEXUS_StcChannelAutoModeBehavior_eVideoMaster;
    NEXUS_SimpleStcChannel_SetSettings(statics.stcChannel, &stcSettings);

    statics.videoPlayer.decoder = NEXUS_SimpleVideoDecoder_Acquire(statics.allocResults.simpleVideoDecoder[0].id);
    statics.audioPlayer.decoder = NEXUS_SimpleAudioDecoder_Acquire(statics.allocResults.simpleAudioDecoder.id);

    NxClient_ConnectSettings connectSettings;
    NxClient_GetDefaultConnectSettings(&connectSettings);
    connectSettings.simpleVideoDecoder[0].id = statics.allocResults.simpleVideoDecoder[0].id;
    connectSettings.simpleVideoDecoder[0].surfaceClientId = statics.allocResults.surfaceClient[0].id;
    connectSettings.simpleAudioDecoder.id = statics.allocResults.simpleAudioDecoder.id;
    connectSettings.simpleVideoDecoder[0].decoderCapabilities.secureVideo = NxClient_SecureDecoderMode_eSecure;
    connectSettings.simpleAudioDecoder.decoderCapabilities.secure = NxClient_SecureDecoderMode_eSecure;
    rc = NxClient_Connect(&connectSettings, &statics.connectId);
    BDBG_ASSERT(!rc);

    /* Show heaps info */
    NEXUS_Platform_GetClientConfiguration(&statics.clientConfig);

    int g;
    LOG_DEBUG(TAG_RT_MEDIA, "NxClient Heaps Info -----------------");
    for (g = NXCLIENT_DEFAULT_HEAP; g <= NXCLIENT_ARR_HEAP; g++)
    {
        NEXUS_MemoryStatus status;
        rc = NEXUS_Heap_GetStatus(statics.clientConfig.heap[g], &status);
        if (!rc) continue;
        LOG_DEBUG(TAG_RT_MEDIA, "Heap[%d]: memoryType=%u, heapType=0x%x, offset=%u, addr=%p, size=%u",
            g, status.memoryType, status.heapType, (uint32_t)status.offset, status.addr, status.size);
    }
    LOG_DEBUG(TAG_RT_MEDIA, "-------------------------------------");


    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_global_shutdown()
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_global_shutdown" );   
    statics.drmSystem.shutdown();
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_init_audio_config(sb_media_audio_config_t * config)
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_init_audio_config codec %d channels %d sample_rate %d delay %d volume %d",
             config->codec, config->number_of_channels, config->sample_rate, config->volume);   

    statics.audioPlayer.init(AUDIO_PLAYPUMP_BUF_SIZE, true);

    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_init_audio_decoder(sb_media_audio_config_t * config, sb_media_decoder_handle handle)
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_init_audio_decoder handle %p callback %p, %p", handle, config->config_done, config->decode_done);   

    NEXUS_SimpleAudioDecoderSettings settings;
    NEXUS_SimpleAudioDecoder_GetSettings(statics.getAudioDecoder(), &settings);
    
#define BRCM_AUDIO_DECODER_FIFO_SIZE     (32768)
    unsigned threshold = BRCM_AUDIO_DECODER_FIFO_SIZE;

    LOG_DEBUG(TAG_RT_MEDIA, "Primary fifoThreshold %d", settings.primary.fifoThreshold);
    settings.primary.fifoThreshold = threshold;
    LOG_DEBUG(TAG_RT_MEDIA, "Primary fifoThreshold set to %d", settings.primary.fifoThreshold);
    NEXUS_SimpleAudioDecoder_SetSettings(statics.getAudioDecoder(), &settings);    

    NEXUS_SimpleAudioDecoderStartSettings startSettings;
    NEXUS_SimpleAudioDecoder_GetDefaultStartSettings(&startSettings);
    switch (config->codec){
        case SB_MEDIA_AUDIO_CODEC_EAC3:
            startSettings.primary.codec = NEXUS_AudioCodec_eAc3Plus;
            break;
        case SB_MEDIA_AUDIO_CODEC_AC3:
            startSettings.primary.codec = NEXUS_AudioCodec_eAc3;
            break;
        case SB_MEDIA_AUDIO_CODEC_AAC:
        default:
            startSettings.primary.codec = NEXUS_AudioCodec_eAac;
            break;
    }
    statics.audioConfig = *config;

    startSettings.primary.pidChannel = statics.audioPlayer.pidChannel;
    startSettings.master = true;

    NEXUS_SimpleAudioDecoder_SetStcChannel(statics.getAudioDecoder(), statics.stcChannel);

    int ret = NEXUS_SimpleAudioDecoder_Start(statics.getAudioDecoder(), &startSettings);
    LOG_DEBUG(TAG_RT_MEDIA, "SimpleAudio Started returns %d", ret);
    statics.audioStartSettings = startSettings;

    
    statics.audioPlayer.start();
    statics.audioPlayer.run();

    *handle = reinterpret_cast<int>(&statics.audioPlayer);
    statics.audioPlayer.steamboatDecoderHandle = handle;
    if (config->config_done) {
        config->config_done(handle);
    }
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_init_video_config(sb_media_video_config_t * config)
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_init_video_config");   
    statics.videoPlayer.init(VIDEO_PLAYPUMP_BUF_SIZE, true);

    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_init_video_decoder(sb_media_video_config_t * config, sb_media_decoder_handle handle)
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_init_video_decoder callback %p %p", config->config_done, config->decode_done);   
    NEXUS_VideoDecoderSettings settings;
    NEXUS_SimpleVideoDecoder_GetSettings(statics.getVideoDecoder(), &settings);
    
    NEXUS_SimpleVideoDecoder_SetSettings(statics.getVideoDecoder(), &settings);    

    NEXUS_SimpleVideoDecoderStartSettings startSettings;
    NEXUS_SimpleVideoDecoder_GetDefaultStartSettings(&startSettings);
    switch (config->codec){
        case SB_MEDIA_VIDEO_CODEC_H264:
            startSettings.settings.codec = NEXUS_VideoCodec_eH264;
            break;
        case SB_MEDIA_VIDEO_CODEC_HEVC:
            startSettings.settings.codec = NEXUS_VideoCodec_eH265;
            break;
        default:
            startSettings.settings.codec = NEXUS_VideoCodec_eH264;
            break;
    }

    startSettings.displayEnabled = true;
    startSettings.settings.appDisplayManagement = false;
    // startSettings.settings.stcChannel = NULL;
    startSettings.settings.pidChannel = statics.videoPlayer.pidChannel;
    startSettings.maxWidth = 3840;
    startSettings.maxHeight = 2160;

    statics.videoConfig = *config;
    NEXUS_SimpleVideoDecoder_SetStcChannel(statics.getVideoDecoder(), statics.stcChannel);

    int ret = NEXUS_SimpleVideoDecoder_Start(statics.getVideoDecoder(), &startSettings);
    LOG_DEBUG(TAG_RT_MEDIA, "SimpleVideo Started returns %d", ret);
    statics.videoStartSettings = startSettings;


    statics.videoPlayer.start();
    statics.videoPlayer.run();

    *handle = reinterpret_cast<int>(&statics.videoPlayer);
    statics.videoPlayer.steamboatDecoderHandle = handle;
    if (config->config_done) {
        config->config_done(handle);
    }
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_decode(sb_media_decoder_handle handle, uint8_t * pes, uint32_t size, sb_media_timestamp_t pts, sb_media_output_buffer_t * out, sb_media_decrypt_t * key)
{
    if (pes == nullptr || size == 0) {
        LOG_WARN(TAG_RT_MEDIA, "Pause : Calling with PES null!!!!");
    }
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_decode %x %p %d %lld %p %p", handle, pes, size, pts, out, key);

    LOG_DEBUG(TAG_RT_MEDIA, "DecoderHandle %p... A:%p. V:%p", handle, statics.audioPlayer.steamboatDecoderHandle, statics.videoPlayer.steamboatDecoderHandle);

    if (handle == statics.audioPlayer.steamboatDecoderHandle) {
        LOG_DEBUG(TAG_RT_MEDIA, "Decode audio data pts %f (%f) size %d",  pts/1000000000.0, (pts - statics.audioPlayer.lastBufferedPts)/1000000000.0, size); 
        if (pes != nullptr && std::llabs(pts - statics.audioPlayer.lastBufferedPts) < 5000000000) {
            LOG_DEBUG(TAG_RT_MEDIA, "AudioBuffered %lldms", statics.audioPlayer.getBufferedRangeMs());
            if (statics.audioPlayer.getBufferedRangeMs() > 5000) {
                return SB_MEDIA_RESULT_OVERFLOW; 
            }
            if (statics.audioPlayer.bufferedBytes + size > AUDIO_LOOP_BUFFER_SIZE) {
                return SB_MEDIA_RESULT_OVERFLOW; 
            }
            if (statics.audioPlayer.paused) {
                statics.resumeAudio(pts);
            }
            int count = statics.audioPlayer.push(pes, size, pts);
            if (statics.audioConfig.decode_done) {
                statics.audioConfig.decode_done(handle);
            }
        } else {
            statics.pauseAudio();
            statics.resumeAudio(pts);
            return SB_MEDIA_RESULT_OVERFLOW;
        }
    } else {
        LOG_DEBUG(TAG_RT_MEDIA, "Decode video data pts %f (%f) size %d",  pts/1000000000.0, (pts - statics.videoPlayer.lastBufferedPts)/1000000000.0, size); 
        if (pes != nullptr && std::llabs(pts - statics.videoPlayer.lastBufferedPts) < 5000000000) {
            LOG_DEBUG(TAG_RT_MEDIA, "VideoBuffered %lldms", statics.videoPlayer.getBufferedRangeMs());

            if (statics.videoPlayer.paused) {
                statics.resumeVideo(pts);
            }
            int count = 0;
            if (key == nullptr) {
                count = statics.videoPlayer.push(pes, size, pts);
            } else {
                LOG_DEBUG(TAG_RT_MEDIA, "DecryptKey (%02x %02x %02x %02x) %d %d %d", key->keyID[0], key->keyID[1], key->keyID[2], key->keyID[3],
                        key->iv_size, key->key_size, key->number_of_subsamples);
                int allClearBytes = 0;
                int allEncryptedBytes = 0;
                for (int i = 0; i < key->number_of_subsamples; i++) {
                    allClearBytes += key->sub_samples[i].clear_data;
                    allEncryptedBytes += key->sub_samples[i].encrypted_data;
                }
                int offset = size - (allClearBytes + allEncryptedBytes);
                if (offset < 0) {
                    offset = 0;
                }
                int remainedSize = size;
                for (int i = 0; i < key->number_of_subsamples; i++) {
                    LOG_DEBUG(TAG_RT_MEDIA, "Decrypt subsample[%d] clearData %d encryptedData %d size %d sessions %d", i, key->sub_samples[i].clear_data + offset, key->sub_samples[i].encrypted_data, size, statics.drmSystem.ocdmSessions.size());
                    offset += key->sub_samples[i].clear_data;
                    if (key->sub_samples[i].encrypted_data == 0) {
                        count += statics.videoPlayer.push(pes, offset, pts);
                    }
                    else if (offset + key->sub_samples[i].encrypted_data <= size) {
                        int sessionIndex = -1;
                        for (auto& session : statics.drmSystem.ocdmSessions) {
                            sessionIndex++;
                            if (session->hasKey(key->keyID, key->key_size)) {
                                LOG_DEBUG(TAG_RT_MEDIA, "DecryptSession found..  sesison %d", sessionIndex);
                                LOG_DEBUG(TAG_RT_MEDIA, "Before Decrypt %02x %02x %02x %02x", pes[offset], pes[offset+1], pes[offset+2], pes[offset+3]);

                                uint8_t* buf = pes;                           
                                int retValue = opencdm_session_decrypt(session->session,
                                                        pes + offset, key->sub_samples[i].encrypted_data,
                                                        key->iv, key->iv_size,
                                                        key->keyID, key->key_size);
                                LOG_DEBUG(TAG_RT_MEDIA, "After Decrypt %02x %02x %02x %02x", pes[offset], pes[offset+1], pes[offset+2], pes[offset+3]);
                                void* token;
                                memcpy(&token, pes + offset, 4);
                                LOG_DEBUG(TAG_RT_MEDIA, "Token %p", token);
                                NEXUS_MemoryBlockHandle block = NEXUS_MemoryBlock_Clone ((NEXUS_MemoryBlockTokenHandle)token);
                                if (!block) {
                                    LOG_ERROR(TAG_RT_MEDIA, "Can not allocate memory");
                                } else {
                                    uint8_t* ptr_generic;
                                    NEXUS_Error rc = NEXUS_MemoryBlock_Lock(block, (void**)&ptr_generic);
                                    if (rc == 0) {
                                        LOG_DEBUG(TAG_RT_MEDIA, "Memory locked %p", ptr_generic);
                                        count += statics.videoPlayer.push(pes, offset, ptr_generic, key->sub_samples[i].encrypted_data, pts);
                                    } else {
                                        LOG_DEBUG(TAG_RT_MEDIA, "Can not lock block %x", rc);
                                    }
                                }                         
                                if (retValue != 0) {
                                    LOG_DEBUG(TAG_RT_MEDIA, "Returning SB_MEDIA_RESULT_OVERFLOW");
                                    return SB_MEDIA_RESULT_OVERFLOW;
                                }
                                break;
                            } else {
                                if (memcmp(session->KID, key->keyID, key->key_size) == 0 || sessionIndex == 1) {
                                    usleep(100*1000);
                                    return SB_MEDIA_RESULT_OVERFLOW;
                                }
                            }
                        }
                    } else {
                        LOG_ERROR(TAG_RT_MEDIA, "Invalid Size!!!!!!!!");
                    }
                    offset += key->sub_samples[i].encrypted_data;
                }
            }
            LOG_DEBUG(TAG_RT_MEDIA, "video buffer count %d", count);
            if (statics.videoConfig.decode_done) {
                statics.videoConfig.decode_done(handle);
            }
        } else {
            statics.pauseVideo();
            statics.resumeVideo(pts);
            return SB_MEDIA_RESULT_OVERFLOW;
        }
    }
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_set_video_output_rectangle(sb_media_decoder_handle handle, const sb_media_rect_t * rect)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_reset_decoder(sb_media_decoder_handle handle)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_get_time(sb_media_decoder_handle handle, sb_media_timestamp_t * presentation_time)
{
    if (statics.isAudioDecoder(handle)) {
        NEXUS_AudioDecoderStatus status;
        NEXUS_SimpleAudioDecoder_GetStatus(statics.getAudioDecoder(), &status);
        uint64_t pts = status.pts;
        *presentation_time = BRCM_PTS_TO_NANO_SECONDS(pts);
    } else if (statics.isVideoDecoder(handle)) {
        NEXUS_VideoDecoderStatus status;
        NEXUS_SimpleVideoDecoder_GetStatus(statics.getVideoDecoder(), &status);
        uint64_t pts = status.pts;
        *presentation_time = BRCM_PTS_TO_NANO_SECONDS(pts);
    }

    // LOG_DEBUG(TAG_RT_MEDIA, "PTS %lld", *presentation_time);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_set_drm(sb_media_drm_flag_t drm)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d DRM %d", __FILE__, __func__, __LINE__, drm);
    return statics.drmSystem.init(drm);
}

sb_media_result_t sb_media_generate_challenge(const uint8_t * object, const uint16_t object_size, sb_media_challenge_callback callback)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d objectSize %d SessionCount %d TID %d", __FILE__, __func__, __LINE__, object_size, statics.drmSystem.ocdmSessions.size(), syscall(__NR_gettid));

                // FILE* fp = fopen("/userdata/challenge.bin", "w");
                // if (fp) {
                //     fwrite(object, 1, object_size, fp);
                //     fclose(fp);
                // }

    statics.drmSystem.initPlayReadySessions(object, object_size, callback);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_process_key_message_response(const uint8_t * response, const uint16_t response_size)
{
    FILE* fp = fopen("/userdata/response.bin", "w");
    if (fp) {
        fwrite(response, 1, response_size, fp);
        fclose(fp);
    }
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d responseSize %d TID %d", __FILE__, __func__, __LINE__, response_size, syscall(__NR_gettid));
    std::string licenseNonce = parseTag((const char*)response, "LicenseNonce");
    LOG_DEBUG(TAG_RT_MEDIA, "LicenseNonce %s", licenseNonce.c_str());
    for (auto session : statics.drmSystem.ocdmSessions) {
        LOG_DEBUG(TAG_RT_MEDIA, "Session LicenseNonce %s", session->licenseNonce.c_str());
        if(session->licenseNonce == licenseNonce) {
            KeyStatus status = opencdm_session_status(session->session, session->KID, 16);
            LOG_DEBUG(TAG_RT_MEDIA, "License & Key status %d", status);
            if (status != Usable) {
                session->update(response, response_size);
            }
            return SB_MEDIA_RESULT_SUCCESS;
        }
    }

/*
    if (statics.drmSystem.ocdmCurrentSession && statics.drmSystem.ocdmCurrentSession->licenseNonce == licenseNonce) {
        statics.drmSystem.ocdmCurrentSession->update(response, response_size);
        return SB_MEDIA_RESULT_SUCCESS;    
    }
*/
    LOG_ERROR(TAG_RT_MEDIA, "Can not find session for nonce %s", licenseNonce.c_str());
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_get_decoder_capabilities(sb_media_decoder_capabilities_t * capabilities)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    capabilities->audio_codecs = SB_MEDIA_AUDIO_CODEC_EAC3 | SB_MEDIA_AUDIO_CODEC_AAC | SB_MEDIA_AUDIO_CODEC_AC3;
    capabilities->profile = SB_MEDIA_PROFILE_H264_MAIN | SB_MEDIA_PROFILE_H264_HIGH | SB_MEDIA_PROFILE_HEVC_MAIN | SB_MEDIA_PROFILE_HEVC_MAIN_10;
    capabilities->level = SB_MEDIA_LEVEL_3_1 | SB_MEDIA_LEVEL_4;
    capabilities->dynamic_range = SB_MEDIA_VIDEO_DYNAMIC_RANGE_HDR10;
    capabilities->video_output_resolution = SB_MEDIA_PLAYER_VIDEO_OUT_SD | SB_MEDIA_PLAYER_VIDEO_OUT_720P | 
                SB_MEDIA_PLAYER_VIDEO_OUT_1080P | SB_MEDIA_PLAYER_VIDEO_OUT_1440P | SB_MEDIA_PLAYER_VIDEO_OUT_4K;
    capabilities->video_output_rate = SB_MEDIA_PLAYER_VIDEO_RATE_25;
    capabilities->hdcp_level = SB_MEDIA_HDCP_2_2;
    capabilities->drm = SB_MEDIA_DRM_PLAYREADY | SB_MEDIA_DRM_WIDEVINE;
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_set_event_callback(sb_media_event_callback callback)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    statics.event_callback = callback;
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_get_stats(sb_media_statistics_t * stats)
{
    // LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    NEXUS_VideoDecoderStatus status;
    NEXUS_SimpleVideoDecoder_GetStatus(statics.getVideoDecoder(), &status);
    NEXUS_AudioDecoderStatus audioStatus;
    NEXUS_SimpleAudioDecoder_GetStatus(statics.getAudioDecoder(), &audioStatus);

    switch (status.frameRate) {
        case NEXUS_VideoFrameRate_e23_976:
            stats->frame_rate = 23.976;
            break;
        case NEXUS_VideoFrameRate_e24:
            stats->frame_rate = 24;
            break;
        case NEXUS_VideoFrameRate_e25:
        default:
            stats->frame_rate = 25;
            break;
        case NEXUS_VideoFrameRate_e29_97:
            stats->frame_rate = 29.97;
            break;
        case NEXUS_VideoFrameRate_e50:
            stats->frame_rate = 50;
            break;
        case NEXUS_VideoFrameRate_e59_94:
            stats->frame_rate = 59.94;
            break;
        case NEXUS_VideoFrameRate_e60:
            stats->frame_rate = 60;
            break;
    }
    stats->dropped_frames = status.numDisplayDrops;
    stats->audio_buffer_bytes = audioStatus.fifoDepth + statics.audioPlayer.bufferedBytes;
    stats->video_buffer_bytes = status.fifoSize + statics.videoPlayer.bufferedBytes;
    uint64_t pts = status.pts;
    stats->pts = BRCM_PTS_TO_NANO_SECONDS(pts);
    stats->frames_displayed = status.numDisplayed;
    stats->frames_decoded = status.numDecoded;
    // LOG_DEBUG(TAG_RT_MEDIA, "PTS %lld", stats->pts);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_get_decoder_stats(sb_media_decoder_handle handle, sb_media_decoder_stats_t * stats)
{
    // LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    if (statics.isAudioDecoder(handle)) {
        NEXUS_AudioDecoderStatus status;
        NEXUS_SimpleAudioDecoder_GetStatus(statics.getAudioDecoder(), &status);
        stats->frames_displayed = status.framesDecoded;
        stats->free_buffer_bytes = status.fifoSize - status.fifoDepth + 
                            (AUDIO_LOOP_BUFFER_SIZE > statics.audioPlayer.bufferedBytes ? (AUDIO_LOOP_BUFFER_SIZE - statics.audioPlayer.bufferedBytes) : 0);
        stats->active = status.started;
        stats->used = 1;
        // LOG_DEBUG(TAG_RT_MEDIA, "Audio status frames displayed %d free buffer %d active %d", stats->frames_displayed,stats->free_buffer_bytes, stats->active);
    } else if (statics.isVideoDecoder(handle)) {
        NEXUS_VideoDecoderStatus status;
        NEXUS_SimpleVideoDecoder_GetStatus(statics.getVideoDecoder(), &status);
        stats->raw_width = status.source.width;
        stats->raw_height = status.source.height;
        stats->displayed_width = status.display.width;
        stats->displayed_height = status.display.height;
        stats->frames_displayed = status.numDisplayed;
        stats->free_buffer_bytes = status.fifoSize - status.fifoDepth + 
                            (VIDEO_LOOP_BUFFER_SIZE > statics.videoPlayer.bufferedBytes ? (VIDEO_LOOP_BUFFER_SIZE - statics.videoPlayer.bufferedBytes) : 0);
        stats->active = status.started;
        stats->used = 1;
        // LOG_DEBUG(TAG_RT_MEDIA, "VideoDecoder status frames displayed %d free buffer %d active %d", stats->frames_displayed,stats->free_buffer_bytes, stats->active);
    }
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_set_playback_rate(sb_media_decoder_handle handle, int8_t playback_rate)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_set_drm_certificate(const uint8_t *certificate, uint32_t size)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    return SB_MEDIA_RESULT_NOT_SUPPORTED;
}

sb_media_result_t sb_media_audio_set_volume(const float volume)
{
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_get_hdmi_capabilities(sb_media_hdmi_capabilities_t *capabilities)
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_get_hdmi_capabilities" );   
    capabilities->audio_capabilities = SB_MEDIA_AUDIO_LPCM | SB_MEDIA_AUDIO_DOLBY_DIGITAL | SB_MEDIA_AUDIO_DOLBY_ATMOS; 
    capabilities->video_output_resolution = SB_MEDIA_PLAYER_VIDEO_OUT_4K;
    capabilities->video_output_rate = SB_MEDIA_PLAYER_VIDEO_RATE_50;
    capabilities->color_format = SB_MEDIA_COLOR_FORMAT_4_2_0;
    capabilities->color_depth = SB_MEDIA_COLOR_DEPTH_12BPC;
    capabilities->color_space = SB_MEDIA_COLOR_SPACE_ITU_R_BT2020; 
    return SB_MEDIA_RESULT_SUCCESS;
}
