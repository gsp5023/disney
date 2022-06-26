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
#include <open_cdm_adapter.h>

#define TAG_RT_MEDIA FOURCC('S', 'T', 'M', 'E')
#define BRCM_PTS_TO_NANO_SECONDS(a) (a*1000000/45)
#define NANO_SECONDS_TO_BRCM_PTS(a) (a*45/1000000)
static void print_adts(int size);

template <typename Decoder> class NexusSteamboatPlayHelper {
public:
    NexusSteamboatPlayHelper() {
        stopped = false;
    }

    ~NexusSteamboatPlayHelper() {
    }

    void init() {
        event = NEXUS_Event_Create();
        lastConsumedPts = 0;
        lastBufferedPts = 0;
        bufferedBytes = 0;
    }

    struct Buffer {
        Buffer(uint8_t* b, int len, uint64_t pts) : buffer(nullptr), len(0), pts(pts) {
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
    std::deque<Buffer*> buffers;
    bool stopped;
    uint64_t lastConsumedPts;
    uint64_t lastBufferedPts;
    int bufferedBytes;
    bool paused = false;
    
    Decoder getDecoder() {
        return decoder;
    }


    void start() {
        paused = false;
        NEXUS_Playpump_Start(playpump);
    }

    void pause() {
        paused = true;
        NEXUS_Playpump_Flush(playpump);
        NEXUS_Playpump_Stop(playpump);
        NEXUS_Event_Signal(event);
        std::unique_lock<std::mutex> lk(lock);
        for (auto b : buffers) {
            delete b;
        }
        buffers.clear();
        lastConsumedPts = 0;
        lastBufferedPts = 0;
        bufferedBytes = 0;
    }

    int push(uint8_t* buf, int len, uint64_t pts) {
        std::unique_lock<std::mutex> lk(lock);
        buffers.push_back(new Buffer(buf, len, pts));
        lastBufferedPts = pts;
        bufferedBytes += len;
        waitForData.notify_one();
        return buffers.size();
    }

    int64_t getBufferedRangeMs() {
        return (lastBufferedPts - lastConsumedPts)/10000000;
    }

    void run() {
        thread = std::thread([this]{
            while (!stopped) {
                std::unique_lock<std::mutex> lk(lock);
                if (buffers.size() == 0) {
                    waitForData.wait(lk, [this] {
                        return buffers.size() != 0;
                    });
                }

                Buffer* buf = buffers.front();
                buffers.pop_front();
                lastConsumedPts = buf->pts;
                bufferedBytes -= buf->len;
                lk.unlock();

                int written = 0;
                /*
                print_adts(buf->len);

                LOG_DEBUG(TAG_RT_MEDIA, "Buffer %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                buf->buffer[0], buf->buffer[1], buf->buffer[2], buf->buffer[3], buf->buffer[4], buf->buffer[5], buf->buffer[6], buf->buffer[7],
                buf->buffer[8+0], buf->buffer[8+1], buf->buffer[8+2], buf->buffer[8+3], buf->buffer[8+4], buf->buffer[8+5], buf->buffer[8+6], buf->buffer[8+7],
                buf->buffer[16+0], buf->buffer[16+1], buf->buffer[16+2], buf->buffer[16+3], buf->buffer[16+4], buf->buffer[16+5], buf->buffer[16+6], buf->buffer[16+7]);
                */

                while ((written < buf->len) && !stopped) {
                    // LOG_DEBUG(TAG_RT_MEDIA, "Write %d %d", written, buf->len);
                    void *nexus_buffer;
                    size_t nexus_space;
                    int ret = NEXUS_Playpump_GetBuffer(playpump, &nexus_buffer, &nexus_space);
                    // LOG_DEBUG(TAG_RT_MEDIA, "Nexus space %d", nexus_space);
                    if (nexus_space > 0) {
                        int writeBytes = nexus_space;
                        if (writeBytes > (buf->len - written)) {
                            writeBytes = (buf->len - written);
                        }
                        memcpy(nexus_buffer, buf->buffer + written, writeBytes);
                        NEXUS_Playpump_WriteComplete(playpump, 0, writeBytes);
                        written += writeBytes;
                    } else {
                        if (event) {
                            NEXUS_Event_Wait(event, 750);
                        } else {
                            LOG_ERROR(TAG_RT_MEDIA, "Event is null!!");
                            usleep(30*1000);
                        }
                    }
                }

                delete buf;
            }
        });
        thread.detach();
    }
};

class OpenCDMContext {
    public:
    private:
    
};

enum {
    BRCM_AUDIO_STREAM_ID = 0xC0,
    BRCM_VIDEO_STREAM_ID = 0xE0,
    AUDIO_LOOP_BUFFER_SIZE = 0x100000,
    VIDEO_LOOP_BUFFER_SIZE = 0x500000,
};
// trim from start (in place)
// static inline void ltrim(std::string &s) {
//     s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
//         return !std::isspace(ch);
//     }));
// }

// // trim from end (in place)
// static inline void rtrim(std::string &s) {
//     s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
//         return !std::isspace(ch);
//     }).base(), s.end());
// }

// // trim from both ends (in place)
// static inline void trim(std::string &s) {
//     ltrim(s);
//     rtrim(s);
// }

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
    bool keyUpdated;
    std::condition_variable waitForKeyUpdate;
    std::mutex lock;
    std::string licenseNonce;
    int keySize;
    uint8_t keyData[16];

    DRMSession (OpenCDMSystem* system, sb_media_challenge_callback callback) : system(system), session(nullptr), keyState(KeyState::KEY_INIT), 
                challengeCallback(callback), keyUpdated(false) {}

    ~DRMSession () {
        opencdm_destruct_session(session);
    }

    bool init(const uint8_t* initData, uint32_t initDataSize) {
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

	void processOCDMChallenge(const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize) {
        LOG_DEBUG(TAG_RT_MEDIA, "processOCDMChallenge Size %d", challengeSize);
        licenseNonce = parseTag((const char*)challenge, "LicenseNonce");
        LOG_DEBUG(TAG_RT_MEDIA, "LicenseNonce %s", licenseNonce.c_str());

        challengeCallback(challenge, challengeSize);
    }

    void update(uint8_t* response, int response_size) {
        std::unique_lock<std::mutex> lk(lock);
        keyUpdated = true;
        waitForKeyUpdate.notify_one();
        opencdm_session_update(session, response, response_size);
    }

    void waitForKey() {
        std::unique_lock<std::mutex> lk(lock);
        if (!keyUpdated) {
            waitForKeyUpdate.wait_for(lk, std::chrono::milliseconds(1000), [this]() {return keyUpdated;});
        }
    }

	void keysUpdatedOCDM() {
        std::unique_lock<std::mutex> lk(lock);
        if (!keyUpdated) {
            keyUpdated = true;
            waitForKeyUpdate.notify_one();
        }
    }

	void keyUpdateOCDM(const uint8_t key[], const uint8_t keySize){
        LOG_DEBUG(TAG_RT_MEDIA, "KeyUpdated keySize %d licenseNonce %s", keySize, licenseNonce.c_str());
        this->keySize = keySize;
        uint16_t copySize = keySize;
        if (copySize > sizeof(keyData)) {
            copySize = sizeof(keyData);
        }
        memcpy(keyData, key, copySize);
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
        ocdmCurrentSession.reset();
        for (auto& session : ocdmSessions) {
            session.reset();
        }
        ocdmSessions.clear();
        opencdm_destruct_system(ocdmSystem);
    }


    void initPlayReadySessions(const uint8_t* initData, int initDataLength, sb_media_challenge_callback callback) {
        initDrmThread = std::thread([initData, initDataLength, callback, this] () {
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
            if (data != initDataLength) {
                LOG_ERROR(TAG_RT_MEDIA, "Invalid length? %d %d", data, initDataLength);
            }
            READ_WORD();
            LOG_DEBUG(TAG_RT_MEDIA, "PRO has %d objects", data);
            int objCount = data;

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

                if (i == 0) {
                std::shared_ptr<DRMSession> session = std::make_shared<DRMSession>(ocdmSystem, callback);
                if (session->init(initData + offset, recordLength)) {
                    ocdmCurrentSession = session;
                    ocdmSessions.push_back(session);
                    // session->waitForKey();
                }
                }
                remainedSize -= recordLength;
                offset += recordLength;
            }
        });

        initDrmThread.join();
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
        audioPlayer.pause();
        NEXUS_SimpleAudioDecoder_Stop(audioPlayer.getDecoder());
    }
    
    void pauseVideo() {
        LOG_DEBUG(TAG_RT_MEDIA, "pauseVideo");
        videoPlayer.pause();
        NEXUS_SimpleVideoDecoder_Stop(videoPlayer.getDecoder());
    }

    void resumeAudio(uint64_t pts) {
        LOG_DEBUG(TAG_RT_MEDIA, "resumeAudio %lld", pts);
        audioPlayer.start();
        NEXUS_SimpleAudioDecoder_Start(audioPlayer.getDecoder(), &audioStartSettings);
    }

    void resumeVideo(uint64_t pts) {
        LOG_DEBUG(TAG_RT_MEDIA, "resumeVideo %lld", pts);
        videoPlayer.start();
        NEXUS_SimpleVideoDecoder_SetStartPts(videoPlayer.getDecoder(), NANO_SECONDS_TO_BRCM_PTS(pts));
        NEXUS_SimpleVideoDecoder_Start(videoPlayer.getDecoder(), &videoStartSettings);
    }

    DRMSystem drmSystem;

    struct OpenCDMSystem* getOcdmSystem() {
        return drmSystem.ocdmSystem;
    }
} statics;


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
    statics.videoPlayer.decoder = NEXUS_SimpleVideoDecoder_Acquire(statics.allocResults.simpleVideoDecoder[0].id);
    statics.audioPlayer.decoder = NEXUS_SimpleAudioDecoder_Acquire(statics.allocResults.simpleAudioDecoder.id);

    NxClient_ConnectSettings connectSettings;
    NxClient_GetDefaultConnectSettings(&connectSettings);
    connectSettings.simpleVideoDecoder[0].id = statics.allocResults.simpleVideoDecoder[0].id;
    connectSettings.simpleVideoDecoder[0].surfaceClientId = statics.allocResults.surfaceClient[0].id;
    connectSettings.simpleAudioDecoder.id = statics.allocResults.simpleAudioDecoder.id;
    rc = NxClient_Connect(&connectSettings, &statics.connectId);
    BDBG_ASSERT(!rc);

    statics.audioPlayer.init();
    statics.videoPlayer.init();

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

    NEXUS_PlaypumpOpenSettings playpumpOpenSettings;
    NEXUS_Playpump_GetDefaultOpenSettings(&playpumpOpenSettings);
    playpumpOpenSettings.fifoSize = 2048*16;
    statics.audioPlayer.playpump = NEXUS_Playpump_Open(NEXUS_ANY_ID, &playpumpOpenSettings);

    NEXUS_PlaypumpSettings playpumpSettings;
    NEXUS_Playpump_GetSettings(statics.audioPlayer.playpump, &playpumpSettings);
    playpumpSettings.transportType = NEXUS_TransportType_eMpeg2Pes;
    playpumpSettings.dataCallback.event = statics.audioPlayer.event;
    int ret = NEXUS_Playpump_SetSettings(statics.audioPlayer.playpump, &playpumpSettings);

    NEXUS_PlaypumpOpenPidChannelSettings pidSettings;
    NEXUS_Playpump_GetDefaultOpenPidChannelSettings(&pidSettings);
    pidSettings.pidType = NEXUS_PidType_eAudio;
    statics.audioPlayer.pidChannel = NEXUS_Playpump_OpenPidChannel(statics.audioPlayer.playpump, BRCM_AUDIO_STREAM_ID, &pidSettings);

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

    statics.audioPlayer.steamboatDecoderHandle = handle;
    if (config->config_done) {
        config->config_done(handle);
    }
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_init_video_config(sb_media_video_config_t * config)
{
    LOG_DEBUG(TAG_RT_MEDIA, "sb_media_init_video_config");   
    NEXUS_PlaypumpOpenSettings playpumpOpenSettings;
    NEXUS_Playpump_GetDefaultOpenSettings(&playpumpOpenSettings);
    statics.videoPlayer.playpump = NEXUS_Playpump_Open(NEXUS_ANY_ID, &playpumpOpenSettings);

    NEXUS_PlaypumpSettings playpumpSettings;
    NEXUS_Playpump_GetSettings(statics.videoPlayer.playpump, &playpumpSettings);
    playpumpSettings.transportType = NEXUS_TransportType_eMpeg2Pes;
    playpumpSettings.dataCallback.event = statics.videoPlayer.event;
    int ret = NEXUS_Playpump_SetSettings(statics.videoPlayer.playpump, &playpumpSettings);

    NEXUS_PlaypumpOpenPidChannelSettings pidSettings;
    NEXUS_Playpump_GetDefaultOpenPidChannelSettings(&pidSettings);
    pidSettings.pidType = NEXUS_PidType_eVideo;
    statics.videoPlayer.pidChannel = NEXUS_Playpump_OpenPidChannel(statics.videoPlayer.playpump, BRCM_VIDEO_STREAM_ID, &pidSettings);

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
    startSettings.settings.stcChannel = NULL;
    startSettings.settings.pidChannel = statics.videoPlayer.pidChannel;
    startSettings.maxWidth = 3840;
    startSettings.maxHeight = 2160;

    // NEXUS_SimpleVideoDecoder_SetStcChannel(statics.getVideoDecoder(), statics.stcChannel);

    statics.videoConfig = *config;

    int ret = NEXUS_SimpleVideoDecoder_Start(statics.getVideoDecoder(), &startSettings);
    LOG_DEBUG(TAG_RT_MEDIA, "SimpleVideo Started returns %d", ret);
    statics.videoStartSettings = startSettings;

    statics.videoPlayer.start();
    statics.videoPlayer.run();

    statics.videoPlayer.steamboatDecoderHandle = handle;
    if (config->config_done) {
        config->config_done(handle);
    }
    return SB_MEDIA_RESULT_SUCCESS;
}

#define B_SET_BIT(name,v,b)  (((unsigned)((v)!=0)<<(b)))
#define B_SET_BITS(name,v,e,b)  (((unsigned)(v))<<(b))
#define B_GET_BIT(w,b)  ((w)&(1<<(b)))
#define B_GET_BITS(w,e,b)  (((w)>>(b))&(((unsigned)(-1))>>((sizeof(unsigned))*8-(e+1-b))))

static void print_adts(int size) {
    const static uint32_t config_sample_rate_table[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    #define CONFIG_SAMPLE_RATE_COUNT  (sizeof(config_sample_rate_table) / sizeof(uint32_t))
    int samplingFrequencyIndex = 3;
    int frameLength = size - 14;
    int channelConfiguration = 2;
    int profile = 1;

    for (int i = 0; i < (int)CONFIG_SAMPLE_RATE_COUNT; i++) {
        if (statics.audioConfig.sample_rate == config_sample_rate_table[i]) {
            LOG_DEBUG(TAG_RT_MEDIA, "Sampling Frequency %d index %d", statics.audioConfig.sample_rate, i);
            samplingFrequencyIndex = i;
            break;
        }
    }    

    uint8_t pData[16];
    pData[0] =
        0xFF; /* sync word */
    pData[1] =
        0xF0 |  /* sync word */
        B_SET_BIT( ID, 0, 3) | /* MPEG-2 AAC */
        B_SET_BITS( "00", 0, 2, 1) |
        B_SET_BIT( protection_absent, 1, 0);

    pData[2] =
        B_SET_BITS( profile, profile, 7, 6) |
        B_SET_BITS( sampling_frequency_index, samplingFrequencyIndex, 5, 2 ) |
        B_SET_BIT( private_bit, 0, 1) |
        B_SET_BIT( channel_configuration[2], B_GET_BIT(channelConfiguration, 2), 0);

    pData[3] = /* 4'th byte is shared */
        B_SET_BITS( channel_configuration[2], B_GET_BITS(channelConfiguration, 1, 0), 7, 6) |
        B_SET_BIT( original_copy, 0, 5) |
        B_SET_BIT( home, 0, 4) |
        /* IS 13818-7 1.2.2 Variable Header of ADTS */
        B_SET_BIT( copyright_identification_bit, 0, 3) |
        B_SET_BIT( copyright_identification_start, 0, 2) |
        B_SET_BITS( aac_frame_length[12..11], B_GET_BITS(frameLength, 12, 11), 1, 0);
    pData[4] =
            B_SET_BITS(aac_frame_length[10..3], B_GET_BITS(frameLength, 10, 3), 7, 0);
    pData[5] =
            B_SET_BITS(aac_frame_length[2..0], B_GET_BITS(frameLength, 2, 0), 7, 5) |
            B_SET_BITS(adts_buffer_fullness[10..6], B_GET_BITS( 0x7FF /* VBR */, 10, 6), 4, 0);
    pData[6] =
            B_SET_BITS(adts_buffer_fullness[5..0], B_GET_BITS( 0x7FF /* VBR */, 5, 0), 7, 2) |
            B_SET_BITS(no_raw_data_blocks_in_frame, 0, 2, 0);

    LOG_DEBUG(TAG_RT_MEDIA, "ADTS Header %02x %02x %02x %02x %02x %02x %02x", pData[0], pData[1], 
        pData[2], pData[3], pData[4], pData[5], pData[6]);

}

sb_media_result_t sb_media_decode(sb_media_decoder_handle handle, uint8_t * pes, uint32_t size, sb_media_timestamp_t pts, sb_media_output_buffer_t * out, sb_media_decrypt_t * key)
{
    if (pes == nullptr) {
        LOG_WARN(TAG_RT_MEDIA, "Calling with PES null!!!!");
    }
    LOG_DEBUG(TAG_RT_MEDIA, "DecoderHandle %p... A:%p. V:%p", handle, statics.audioPlayer.steamboatDecoderHandle, statics.videoPlayer.steamboatDecoderHandle);
    if (key){
        LOG_DEBUG(TAG_RT_MEDIA, "DecryptKey %s %d %d %d", key->keyID, key->iv_size, key->key_size, key->number_of_subsamples);
        int offset = 16;
        int remainedSize = size;
        if (statics.drmSystem.ocdmCurrentSession) {
            for (int i = 0; i < key->number_of_subsamples; i++) {
                LOG_DEBUG(TAG_RT_MEDIA, "Decrypt subsample[%d] clearData %d encryptedData %d size %d", i, key->sub_samples[i].clear_data, key->sub_samples[i].encrypted_data, size);
                offset += key->sub_samples[i].clear_data;
                if (offset + key->sub_samples[i].encrypted_data <= size) {
                    for (auto& session : statics.drmSystem.ocdmSessions) {
                        if (session->keySize == key->key_size && memcmp(session->keyData, key->keyID, key->key_size) == 0) {
                            LOG_DEBUG(TAG_RT_MEDIA, "Session found..");
                            LOG_DEBUG(TAG_RT_MEDIA, "Before Decrypt %02x %02x %02x %02x", pes[offset], pes[offset+1], pes[offset+2], pes[offset+3]);

                            uint8_t* buf = pes;
                LOG_DEBUG(TAG_RT_MEDIA, "Buffer %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
                " %02x %02x %02x %02x %02x %02x %02x %02x",
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
                buf[8+0], buf[8+1], buf[8+2], buf[8+3], buf[8+4], buf[8+5], buf[8+6], buf[8+7],
                buf[16+0], buf[16+1], buf[16+2], buf[16+3], buf[16+4], buf[16+5], buf[16+6], buf[16+7], 
                buf[24+0], buf[24+1], buf[24+2], buf[24+3], buf[24+4], buf[24+5], buf[24+6], buf[24+7],
                buf[32+0], buf[32+1], buf[32+2], buf[32+3], buf[32+4], buf[32+5], buf[32+6], buf[32+7],
                buf[40+0], buf[40+1], buf[40+2], buf[40+3], buf[40+4], buf[40+5], buf[40+6], buf[40+7]);                            
                            int retValue = opencdm_session_decrypt(statics.drmSystem.ocdmCurrentSession->session,
                                                    pes + offset, key->sub_samples[i].encrypted_data,
                                                    key->iv, key->iv_size,
                                                    key->keyID, key->key_size);

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
                        LOG_DEBUG(TAG_RT_MEDIA, "locked %02x %02x %02x %02x", ptr_generic[0], ptr_generic[1], ptr_generic[2], ptr_generic[3]);
                    } else {
                        LOG_DEBUG(TAG_RT_MEDIA, "Can not lock block %x", rc);
                    }
                }

                LOG_DEBUG(TAG_RT_MEDIA, "Buffer After decrypt %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"
                " %02x %02x %02x %02x %02x %02x %02x %02x",
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
                buf[8+0], buf[8+1], buf[8+2], buf[8+3], buf[8+4], buf[8+5], buf[8+6], buf[8+7],
                buf[16+0], buf[16+1], buf[16+2], buf[16+3], buf[16+4], buf[16+5], buf[16+6], buf[16+7], 
                buf[24+0], buf[24+1], buf[24+2], buf[24+3], buf[24+4], buf[24+5], buf[24+6], buf[24+7],
                buf[32+0], buf[32+1], buf[32+2], buf[32+3], buf[32+4], buf[32+5], buf[32+6], buf[32+7],
                buf[40+0], buf[40+1], buf[40+2], buf[40+3], buf[40+4], buf[40+5], buf[40+6], buf[40+7]);                            
                            LOG_DEBUG(TAG_RT_MEDIA, "decrypt called %d %02x %02x %02x %02x", retValue, pes[offset], pes[offset+1], pes[offset+2], pes[offset+3]);
                            if (retValue != 0) {
                                LOG_DEBUG(TAG_RT_MEDIA, "Returning SB_MEDIA_RESULT_OVERFLOW");
                                return SB_MEDIA_RESULT_OVERFLOW;
                            }
                        }
                    }
                }
                offset += key->sub_samples[i].encrypted_data;
            }
        }
    }
    if (handle == statics.audioPlayer.steamboatDecoderHandle) {
        LOG_DEBUG(TAG_RT_MEDIA, "Decode audio data pts %lld, size %d",  pts, size); 
        if (pes != nullptr) {
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
            LOG_DEBUG(TAG_RT_MEDIA, "audio buffer count %d", count);
            if (statics.audioConfig.decode_done) {
                statics.audioConfig.decode_done(handle);
            }
        } else {
            statics.pauseAudio();
        }
    } else {
        LOG_DEBUG(TAG_RT_MEDIA, "Decode video data pts %lld, size %d",  pts, size); 
        if (pes != nullptr) {
            LOG_DEBUG(TAG_RT_MEDIA, "VideoBuffered %lldms", statics.videoPlayer.getBufferedRangeMs());
            if (statics.videoPlayer.getBufferedRangeMs() > 5000) {
                return SB_MEDIA_RESULT_OVERFLOW; 
            }
            if (statics.videoPlayer.bufferedBytes + size > VIDEO_LOOP_BUFFER_SIZE) {
                return SB_MEDIA_RESULT_OVERFLOW; 
            }
            if (statics.videoPlayer.paused) {
                statics.resumeVideo(pts);
            }
            int count = statics.videoPlayer.push(pes, size, pts);
            LOG_DEBUG(TAG_RT_MEDIA, "video buffer count %d", count);
            if (statics.videoConfig.decode_done) {
                statics.videoConfig.decode_done(handle);
            }
        } else {
            statics.pauseVideo();
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
    // LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d", __FILE__, __func__, __LINE__);
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
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d objectSize %d SessionCount %d", __FILE__, __func__, __LINE__, object_size, statics.drmSystem.ocdmSessions.size());

                // FILE* fp = fopen("/userdata/challenge.bin", "w");
                // if (fp) {
                //     fwrite(object, 1, object_size, fp);
                //     fclose(fp);
                // }

    statics.drmSystem.initPlayReadySessions(object, object_size, callback);
    usleep(500*1000);
    return SB_MEDIA_RESULT_SUCCESS;
}

sb_media_result_t sb_media_process_key_message_response(const uint8_t * response, const uint16_t response_size)
{
    FILE* fp = fopen("/userdata/response.bin", "w");
    if (fp) {
        fwrite(response, 1, response_size, fp);
        fclose(fp);
    }
    LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d responseSize %d", __FILE__, __func__, __LINE__, response_size);
    std::string licenseNonce = parseTag((const char*)response, "LicenseNonce");
    LOG_DEBUG(TAG_RT_MEDIA, "LicenseNonce %s", licenseNonce.c_str());
    for (auto session : statics.drmSystem.ocdmSessions) {
        LOG_DEBUG(TAG_RT_MEDIA, "Session LicenseNonce %s", session->licenseNonce.c_str());
        if(session->licenseNonce == licenseNonce) {
            opencdm_session_update(session->session, response, response_size);
            return SB_MEDIA_RESULT_SUCCESS;
        }
    }
    if (statics.drmSystem.ocdmCurrentSession) {
        LOG_DEBUG(TAG_RT_MEDIA, "%s %s %d currentSession Found, count %d", __FILE__, __func__, __LINE__, statics.drmSystem.ocdmSessions.size());
        opencdm_session_update(statics.drmSystem.ocdmCurrentSession->session, response, response_size);
    }
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
        stats->free_buffer_bytes = status.fifoSize - status.fifoDepth +  + AUDIO_LOOP_BUFFER_SIZE - statics.audioPlayer.bufferedBytes;
        stats->active = status.started;
        stats->used = 1;
        // LOG_DEBUG(TAG_RT_MEDIA, "Audio status frames displayed %d free buffer %d", stats->frames_displayed,stats->free_buffer_bytes);
    } else if (statics.isVideoDecoder(handle)) {
        NEXUS_VideoDecoderStatus status;
        NEXUS_SimpleVideoDecoder_GetStatus(statics.getVideoDecoder(), &status);
        stats->raw_width = status.source.width;
        stats->raw_height = status.source.height;
        stats->displayed_width = status.display.width;
        stats->displayed_height = status.display.height;
        stats->frames_displayed = status.numDisplayed;
        stats->free_buffer_bytes = status.fifoSize - status.fifoDepth + VIDEO_LOOP_BUFFER_SIZE - statics.videoPlayer.bufferedBytes;
        stats->active = status.started;
        stats->used = 1;
        // LOG_DEBUG(TAG_RT_MEDIA, "VideoDecoder status frames displayed %d free buffer %d", stats->frames_displayed,stats->free_buffer_bytes);
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
