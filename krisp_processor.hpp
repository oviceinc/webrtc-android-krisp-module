#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "modules/audio_processing/include/audio_processing.h"

#include "audio_frame_processor.hpp"
#include "krisp-audio-sdk-nc-c.h"

namespace Krisp
{

bool LoadKrisp(const char* krispDllPath);
bool UnloadKrisp();

class KrispNoiseFilter : public AudioFrameProcessor
{
public:
    KrispNoiseFilter();
    KrispNoiseFilter(const KrispNoiseFilter&) = delete;
    KrispNoiseFilter(KrispNoiseFilter&&) = delete;
    KrispNoiseFilter& operator=(const KrispNoiseFilter&) = delete;
    KrispNoiseFilter& operator=(KrispNoiseFilter&&) = delete;
    ~KrispNoiseFilter() override;

    bool Init(const char* modelPath);
    bool Init(const void* modelAddr, unsigned int modelSize);
    void DeInit();

    void ProcessFrame(webrtc::AudioBuffer* audioBuffer) override;
    void InitializeSession(int sampleRate, int numberOfChannels) override;
    void Enable(bool isEnable) override;
    bool IsEnabled() const override;

protected:
    float GetVoiceConfidence(const int16_t* data, size_t len) override;

private:
    std::atomic<bool> m_isEnabled;
    int m_numberOfChannels;
    std::wstring m_modelPath;
    std::vector<uint8_t> m_modelData;
    std::vector<float> m_bufferOut;
    KrispModelInfo m_modelInfo;
    KrispNcSessionConfig m_sessionConfig;
    krispNcHandle m_ncCachedHandle;

    float m_voiceEnergySum = 0.f;
    uint32_t m_voiceEnergyFrameCount = 0;
};

class KrispAdapter : public webrtc::CustomProcessing
{
public:
    explicit KrispAdapter(const std::shared_ptr<AudioFrameProcessor>& processor);
    KrispAdapter(const KrispAdapter&) = delete;
    KrispAdapter& operator=(const KrispAdapter&) = delete;
    KrispAdapter(KrispAdapter&&) = default;
    KrispAdapter& operator=(KrispAdapter&&) = default;
    ~KrispAdapter() override = default;

private:
    void Initialize(int sampleRate, int numOfChannels) override;
    void Process(webrtc::AudioBuffer* audioBuffer) override;
    std::string ToString() const override;
    void SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting) override;

    std::shared_ptr<AudioFrameProcessor> m_processor;
};

struct NativeKrispModule {
    std::shared_ptr<AudioFrameProcessor> proc;
    rtc::scoped_refptr<webrtc::AudioProcessing> apm;
    KrispNoiseFilter* krispFilter = nullptr;

    static std::unique_ptr<NativeKrispModule> Create();
    static std::unique_ptr<NativeKrispModule> CreateWithModelPath(const char* modelPath);
    static std::unique_ptr<NativeKrispModule> CreateWithModelData(
        const void* modelData, unsigned int modelSize);
};

}
