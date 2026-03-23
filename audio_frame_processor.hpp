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
    using AudioFrameCallback = std::function<void(const int16_t*, size_t)>;

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

protected:
    static constexpr int kOutputSampleRate = 16000;
    static constexpr size_t kAccumFrameSize = 3200;  // 200ms @ 16kHz

    void ResampleAndDeliver(const float* src, size_t srcLen, int srcRate);

    std::vector<float> m_bufferIn;

    std::mutex m_callbackMutex;
    AudioFrameCallback m_audioFrameCallback;
    webrtc::PushResampler<float> m_resampler;
    std::vector<float> m_resampleBuffer;
    std::vector<int16_t> m_accumBuffer;
    size_t m_accumPos;
};

}
