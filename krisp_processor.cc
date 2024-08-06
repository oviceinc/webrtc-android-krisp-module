#include "inc/krisp-audio-sdk.hpp"
#include "inc/krisp-audio-sdk-nc.hpp"
#include "krisp_processor.hpp"
#include "rtc_base/time_utils.h"

#include "krisp_audio_sdk_api_wrapper.h"
#include <syslog.h>

namespace Krisp {

bool KrispProcessor::m_isEnabled = false;
void* KrispProcessor::sdkHandle = nullptr;
KrispProcessor* KrispProcessor::m_instance = nullptr;

namespace {
inline std::wstring convertMBString2WString(const std::string& str)
{
  std::wstring w(str.begin(), str.end());
  return w;
}

KrispAudioFrameDuration GetFrameDuration(size_t duration)
{
    switch (duration) {
    case 10:
        return KRISP_AUDIO_FRAME_DURATION_10MS;
    default:
		  syslog(LOG_INFO, "KrispProcessor::GetFrameDuration: Frame duration: %zu \
                is not supported. Switching to default 10ms",  duration);
        return KRISP_AUDIO_FRAME_DURATION_10MS;
    }
}

KrispAudioSamplingRate GetSampleRate(size_t sampleRate)
{
    switch (sampleRate) {
    case 8000:
        return KRISP_AUDIO_SAMPLING_RATE_8000HZ;
    case 16000:
        return KRISP_AUDIO_SAMPLING_RATE_16000HZ;
    case 24000:
        return KRISP_AUDIO_SAMPLING_RATE_24000HZ;
    case 32000:
        return KRISP_AUDIO_SAMPLING_RATE_32000HZ;
    case 44100:
        return KRISP_AUDIO_SAMPLING_RATE_44100HZ;
    case 48000:
        return KRISP_AUDIO_SAMPLING_RATE_48000HZ;
    case 88200:
        return KRISP_AUDIO_SAMPLING_RATE_88200HZ;
    case 96000:
        return KRISP_AUDIO_SAMPLING_RATE_96000HZ;
    default:
		syslog(LOG_INFO, "KrispProcessor::GetSampleRate: The input sampling rate: %zu \
             is not supported. Using default 48khz.", sampleRate);
        return KRISP_AUDIO_SAMPLING_RATE_48000HZ;
    }
}
}//end of namespace

KrispProcessor::KrispProcessor():
    m_session(nullptr), m_sampleRate(KRISP_AUDIO_SAMPLING_RATE_16000HZ), m_numberOfChannels(1),
    m_lastTimeStamp(0), m_bufferIn(), m_bufferOut()
{
}

KrispProcessor::~KrispProcessor() {
    syslog(LOG_INFO,"KrispProcessor:: Krisp Global Destroy");
    KrispAudioSDKWrapper::AudioNcCloseSession(m_session);
    DeInit();
}

KrispProcessor* KrispProcessor::GetInstance()
{
    if(m_instance == nullptr) {
        m_instance = new KrispProcessor();
    }
    return m_instance;
}

void KrispProcessor::DeInit() {
    KrispAudioSDKWrapper::UnLoadDLL();
}

bool KrispProcessor::Init(const char* modelPath, const char* krispDllPath) {
    if (!KrispAudioSDKWrapper::LoadDLL(krispDllPath)) {
        syslog(LOG_ERR, "KrispProcessor::Init: Unable to find Krisp DLL");
        return false;
    }

    if (!KrispAudioSDKWrapper::AudioGlobalInit(nullptr)) {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to initialize Krisp globals");
        return false;
	}

    if (KrispAudioSDKWrapper::AudioSetModel(convertMBString2WString(modelPath).c_str(), "default") != 0) {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to set model file %s", modelPath);
        return false;
	}

    return true;
}

bool KrispProcessor::Init(const void* modelAddr, unsigned int modelSize, const char* krispDllPath)
{
    if (!KrispAudioSDKWrapper::LoadDLL(krispDllPath)) {
        syslog(LOG_ERR, "KrispProcessor::Init: Unable to find Krisp DLL");
        return false;
    }

    if (!KrispAudioSDKWrapper::AudioGlobalInit(nullptr)) {
        syslog(LOG_ERR, "KrispProcessor::Init Failed to initialize Krisp globals");
        return false;
	}

    if (KrispAudioSDKWrapper::AudioSetModelBlob(modelAddr, modelSize, "default") != 0) {
        syslog(LOG_ERR, "KrispProcessor::Init: Krisp failed to set model via blob api");
        return false;
	}

	return true;
}

void KrispProcessor::Enable(bool isEnable) {
	m_isEnabled = isEnable;
}

bool KrispProcessor::IsEnabled() {
	return m_isEnabled;
}

void KrispProcessor::Initialize(int sampleRate, int numberOfChannels) {
	syslog(LOG_INFO, "KrispProcessor::Initialize: sampleRate: %i\
        numberOfChannels: %i", sampleRate, numberOfChannels);
    m_numberOfChannels = numberOfChannels;
    if (m_sampleRate != sampleRate || m_session == nullptr) {
        if (m_session) {
            KrispAudioSDKWrapper::AudioNcCloseSession(m_session);
        }
        m_session = CreateAudioSession(sampleRate);
        m_sampleRate = sampleRate;
        if (m_session == nullptr) {
            // TODO: throw a valid WebRTC exception for error handling
            syslog(LOG_ERR, "KrispProcessor::Initialize: Failed creating Krisp AudioSession");
            return;
        }
    }
}

void KrispProcessor::Process(webrtc::AudioBuffer* audioBuffer) {

	if(!KrispProcessor::IsEnabled()) {
        syslog(LOG_DEBUG, "KrispProcessor::Process: Bypassing NoiseSuppressor::Process");
	    return;
	}

    auto now = rtc::TimeMillis();
    if (now - m_lastTimeStamp > 10000) {
        syslog(LOG_INFO,"KrispProcessor::Process: Num Frames: %zu\
             num Bands: %zu  Num Channels: %zu ", audioBuffer->num_frames(),
             audioBuffer->num_bands(), audioBuffer->num_channels());
      m_lastTimeStamp = now;
    }

    int audioBufferSampleRate = audioBuffer->num_frames() * 1000;
	if(audioBufferSampleRate != m_sampleRate) {
        if (m_session) {
            KrispAudioSDKWrapper::AudioNcCloseSession(m_session);
        }
        m_session = CreateAudioSession(audioBufferSampleRate);
        m_sampleRate = audioBufferSampleRate;
        if (m_session == nullptr) {
            syslog(LOG_ERR, "KrispProcessor::Process: Failed creating AudioSession");
	        return;
	    }
	}

    constexpr size_t kNsFrameSize = 160;
    size_t bufferSize = kNsFrameSize * audioBuffer->num_bands();
    m_bufferIn.resize(bufferSize);
    m_bufferOut.resize(bufferSize);

    for (size_t i = 0; i < bufferSize; ++i) {
        m_bufferIn[i] = audioBuffer->channels()[0][i] / 32768.f;
    }

    auto returnCode = KrispAudioSDKWrapper::AudioNcCleanAmbientNoiseFloat(
      m_session, m_bufferIn.data(), bufferSize,
      m_bufferOut.data(), bufferSize);

    if (returnCode != 0) {
        syslog(LOG_INFO, "KrispProcessor::Process: Krisp noise cleanup error");
        return;
    }

    for (size_t i = 0; i < bufferSize; ++i) {
        audioBuffer->channels()[0][i] = m_bufferOut[i] * 32768.f;
    }
}

std::string KrispProcessor::ToString() const {
    return "KrispAudioProcessor";
}

void KrispProcessor::SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting) {
}

void * KrispProcessor::CreateAudioSession(int sampleRate) {
    auto krispSampleRate = GetSampleRate(sampleRate);
    auto krispFrameDuration = GetFrameDuration(KRISP_AUDIO_FRAME_DURATION_10MS);
    return KrispAudioSDKWrapper::AudioNcCreateSession(krispSampleRate, krispSampleRate, krispFrameDuration, "default");
}

void KrispProcessor::UnloadDLL() {
    KrispAudioSDKWrapper::UnLoadDLL();
}

} //end of namespace Krisp
