#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

#include "common_audio/resampler/include/push_resampler.h"
#include "modules/audio_processing/audio_buffer.h"

namespace Krisp
{

class AudioFrameProcessor
{
public:
    using AudioFrameCallback = std::function<void(const int16_t*, size_t,
                                                    float confidence,
                                                    float voiceEnergy)>;
    using VADStateCallback = std::function<void(bool isSpeech)>;

    AudioFrameProcessor();
    AudioFrameProcessor(const AudioFrameProcessor&) = delete;
    AudioFrameProcessor(AudioFrameProcessor&&) = delete;
    AudioFrameProcessor& operator=(const AudioFrameProcessor&) = delete;
    AudioFrameProcessor& operator=(AudioFrameProcessor&&) = delete;
    virtual ~AudioFrameProcessor();

    virtual void ProcessFrame(webrtc::AudioBuffer* audioBuffer);
    virtual void InitializeSession(int sampleRate, int numberOfChannels);
    virtual void Enable(bool isEnable);
    virtual bool IsEnabled() const;

    void SetAudioFrameCallback(AudioFrameCallback callback);
    void SetVADStateCallback(VADStateCallback callback);

protected:
    static constexpr int kOutputSampleRate = 16000;
    static constexpr size_t kAccumFrameSize = 1600;  // 100ms @ 16kHz
    static constexpr uint32_t kAccumFrameDurationMs = 100;

    static constexpr float kPositiveSpeechThreshold = 0.5f;
    static constexpr float kNegativeSpeechThreshold = 0.35f;
    static constexpr uint32_t kRedemptionMs = 1400;

    void ResampleAndDeliver(const float* src, size_t srcLen, int srcRate);
    virtual float GetVoiceConfidence(const int16_t* data, size_t len);

    std::vector<float> m_bufferIn;

    std::mutex m_callbackMutex;
    AudioFrameCallback m_audioFrameCallback;
    VADStateCallback m_vadStateCallback;
    webrtc::PushResampler<float> m_resampler;
    std::vector<float> m_resampleBuffer;
    std::vector<int16_t> m_accumBuffer;
    size_t m_accumPos;

    bool m_isSpeaking = false;
    uint32_t m_silenceAccumMs = 0;
    float m_lastVoiceEnergy = 0.f;
};

}
