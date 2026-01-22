#include "krisp_processor.hpp"

#include <cstring>

#include <syslog.h>
#include "krisp-audio-api-definitions-c.h"
#include "krisp-audio-sdk-nc-c.h"
#include "rtc_base/time_utils.h"

#include "krisp_sdk.h"


namespace Krisp
{

bool LoadKrisp(const char* krispDllPath)
{
    if (!KrispSDK::LoadDll(krispDllPath))
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Unable to load Krisp DLL");
        return false;
    }

    if (!KrispSDK::GlobalInit(nullptr, nullptr, KrispLogLevel::LogLevelOff))
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to initialize Krisp globals");
        return false;
	}
    return true;
}

bool UnloadKrisp() 
{
    if (KrispSDK::GlobalDestroy() != KrispRetValSuccess)
    {
        syslog(LOG_ERR, "KrispProcessor::Unload: Failed to destroy Krisp globals");
        return false;
    }
    KrispSDK::UnloadDll();
    return true;
}

static KrispSamplingRate GetSampleRate(size_t sampleRate)
{
    switch (sampleRate)
	{
    case 8000:
        return KrispSamplingRate::Sr8000Hz;
    case 16000:
        return KrispSamplingRate::Sr16000Hz;
    case 24000:
        return KrispSamplingRate::Sr24000Hz;
    case 32000:
        return KrispSamplingRate::Sr32000Hz;
    case 44100:
        return KrispSamplingRate::Sr44100Hz;
    case 48000:
        return KrispSamplingRate::Sr48000Hz;
    case 88200:
        return KrispSamplingRate::Sr88200Hz;
    case 96000:
        return KrispSamplingRate::Sr96000Hz;
    default:
		syslog(LOG_INFO, "KrispProcessor::GetSampleRate: The input sampling rate: %zu \
             is not supported. Using default 48khz.", sampleRate);
        return KrispSamplingRate::Sr48000Hz;
    }
}

inline std::wstring convertMBString2WString(const std::string& str)
{
  std::wstring w(str.begin(), str.end());
  return w;
}

KrispNoiseFilter::KrispNoiseFilter() :
    m_isEnabled(false),
	m_numberOfChannels(1),
    m_lastTimeStamp(0),
    m_modelPath(),
    m_modelData(),
	m_bufferIn(),
	m_bufferOut()
{
    m_modelInfo.path = nullptr;
    m_modelInfo.blob.data = nullptr;
    m_modelInfo.blob.size = 0;
    m_sessionConfig.enableSessionStats = false;
    m_sessionConfig.inputSampleRate = KrispSamplingRate::Sr16000Hz;
    m_sessionConfig.inputFrameDuration = KrispFrameDuration::Fd10ms;
    m_sessionConfig.outputSampleRate = KrispSamplingRate::Sr16000Hz;
    m_sessionConfig.modelInfo = &m_modelInfo;
    m_sessionConfig.ringtoneCfg = nullptr;
    m_ncCachedHandle = 0;
}

KrispNoiseFilter::~KrispNoiseFilter()
{
    syslog(LOG_INFO,"KrispProcessor::~KrispProcessor()");
    DeInit();
}

void KrispNoiseFilter::DeInit() {
    if (m_ncCachedHandle) 
    {
        KrispSDK::DestroyNcFloat(m_ncCachedHandle);
        m_ncCachedHandle = 0;
    }
    m_modelPath.clear();
    m_modelPath.shrink_to_fit();
    m_modelData.clear();
    m_modelData.shrink_to_fit();
    m_modelInfo.path = nullptr;
    m_modelInfo.blob.data = nullptr;
    m_modelInfo.blob.size = 0;
}

bool KrispNoiseFilter::Init(const char* modelPath)
{
    m_modelPath = convertMBString2WString(modelPath);
    m_modelInfo.path = m_modelPath.c_str();
    m_modelInfo.blob.data = nullptr;
    m_modelInfo.blob.size = 0;
    m_modelData.clear();
    if (m_ncCachedHandle)
    {
        KrispSDK::DestroyNcFloat(m_ncCachedHandle);
        m_ncCachedHandle = 0;
    }
    m_ncCachedHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
    if (m_ncCachedHandle == 0)
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to create Krisp NC session");
        return false;
    }
    return true;
}

bool KrispNoiseFilter::Init(const void* modelAddr, unsigned int modelSize)
{
    m_modelData.resize(modelSize);
    std::memcpy(m_modelData.data(), modelAddr, modelSize);
    m_modelInfo.path = nullptr;
    m_modelInfo.blob.data = m_modelData.data();
    m_modelInfo.blob.size = modelSize;

    m_ncCachedHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
    if (m_ncCachedHandle == 0)
    {
        syslog(LOG_ERR, "KrispProcessor::Init: Failed to create Krisp NC session");
        return false;
    }
	return true;
}

void KrispNoiseFilter::Enable(bool isEnable)
{
	m_isEnabled.store(isEnable, std::memory_order_release);
}

bool KrispNoiseFilter::IsEnabled() const
{
	return m_isEnabled.load(std::memory_order_acquire);
}

void KrispNoiseFilter::InitializeSession(int sampleRate, int numberOfChannels)
{
	syslog(LOG_INFO, "KrispProcessor::Initialize: sampleRate: %i\
        numberOfChannels: %i", sampleRate, numberOfChannels);

    m_numberOfChannels = numberOfChannels;
    m_sessionConfig.inputSampleRate = GetSampleRate(sampleRate);
    m_sessionConfig.outputSampleRate = m_sessionConfig.inputSampleRate;

    krispNcHandle newNcHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
    if (newNcHandle == 0)
    {
        syslog(LOG_ERR, "KrispProcessor::Initialize: Failed to create Krisp NC session");
        return;
    }
    if (m_ncCachedHandle && KrispSDK::DestroyNcFloat(m_ncCachedHandle) != KrispRetValSuccess) {
        syslog(LOG_ERR, "KrispProcessor::Initialize: Failed to destroy Krisp NC session");
        // TODO: handle memory leak
    }
    m_ncCachedHandle = newNcHandle;
}

void KrispNoiseFilter::ProcessFrame(webrtc::AudioBuffer* audioBuffer)
{

	if(!KrispNoiseFilter::IsEnabled())
    {
        syslog(LOG_DEBUG, "KrispProcessor::Process: Bypassing NoiseSuppressor::Process");
	    return;
	}

    auto now = rtc::TimeMillis();
    if (now - m_lastTimeStamp > 10000)
    {
        syslog(LOG_INFO,"KrispProcessor::Process: Num Frames: %zu\
             num Bands: %zu  Num Channels: %zu ", audioBuffer->num_frames(),
             audioBuffer->num_bands(), audioBuffer->num_channels());
      m_lastTimeStamp = now;
    }

    int audioBufferSampleRate = audioBuffer->num_frames() * 100;
	if(audioBufferSampleRate != static_cast<int>(m_sessionConfig.inputSampleRate))
    {
        m_sessionConfig.inputSampleRate = GetSampleRate(audioBufferSampleRate);
        m_sessionConfig.outputSampleRate = m_sessionConfig.inputSampleRate;
        krispNcHandle newNcHandle = KrispSDK::CreateNcFloat(&m_sessionConfig);
        if (newNcHandle == 0)
        {
            syslog(LOG_ERR, "KrispProcessor::Process: Failed to create Krisp NC session");
            return;
        }
        if (m_ncCachedHandle)
        {
            if (KrispSDK::DestroyNcFloat(m_ncCachedHandle) != KrispRetValSuccess) {
                syslog(LOG_ERR, "KrispProcessor::Process: Failed to destroy Krisp NC session");
                // TODO: handle memory leak
            }
        }
        m_ncCachedHandle = newNcHandle;
	}

    size_t bufferSize = audioBuffer->num_frames();
    if (m_bufferIn.size() != bufferSize) {
        m_bufferIn.resize(bufferSize);
    }
    if (m_bufferOut.size() != bufferSize) {
        m_bufferOut.resize(bufferSize);
    }

    for (size_t i = 0; i < bufferSize; ++i)
    {
        m_bufferIn[i] = audioBuffer->channels()[0][i] / 32768.f;
    }

    auto returnCode = KrispSDK::ProcessNcFloat(
        m_ncCachedHandle,
        m_bufferIn.data(), bufferSize,
        m_bufferOut.data(), bufferSize, 100.0f, nullptr);

    if (returnCode != KrispRetValSuccess)
    {
        syslog(LOG_INFO, "KrispProcessor::Process: Krisp noise cleanup error");
        return;
    }

    for (size_t i = 0; i < bufferSize; ++i)
    {
        audioBuffer->channels()[0][i] = m_bufferOut[i] * 32768.f;
    }
}


KrispAdapter::KrispAdapter(const std::shared_ptr<KrispNoiseFilter>& krispProcessor) :
    m_krispProcessor(krispProcessor)
{
}

void KrispAdapter::Initialize(int sampleRate, int numOfChannels)
{
    m_krispProcessor->InitializeSession(sampleRate, numOfChannels);
}

void KrispAdapter::Process(webrtc::AudioBuffer* audioBuffer)
{
    m_krispProcessor->ProcessFrame(audioBuffer);
}

std::string KrispAdapter::ToString() const
{
    return "KrispAudioProcessor";
}

void KrispAdapter::SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting)
{
    if (setting.type() ==
        webrtc::AudioProcessing::RuntimeSetting::Type::kCaptureOutputUsed)
    {
        bool enable = false;
        setting.GetBool(&enable);
        m_krispProcessor->Enable(enable);
    }
}

std::unique_ptr<NativeKrispModule> NativeKrispModule::Create()
{
    auto m = std::make_unique<NativeKrispModule>();
    m->proc = std::make_shared<KrispNoiseFilter>();
    m->apm = webrtc::AudioProcessingBuilder()
        .SetCapturePostProcessing(std::make_unique<KrispAdapter>(m->proc))
        .Create();
    webrtc::AudioProcessing::Config config;
    config.echo_canceller.enabled = false;
    config.echo_canceller.mobile_mode = true;
    m->apm->ApplyConfig(config);
    return m;
}


}
