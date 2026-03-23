#include "audio_frame_processor.hpp"

#include <algorithm>
#include <syslog.h>

namespace Krisp
{

AudioFrameProcessor::AudioFrameProcessor()
    : m_accumBuffer(kAccumFrameSize, 0),
      m_accumPos(0)
{
}

AudioFrameProcessor::~AudioFrameProcessor() = default;

void AudioFrameProcessor::ProcessFrame(webrtc::AudioBuffer* audioBuffer)
{
    const int audioBufferSampleRate = audioBuffer->num_frames() * 100;
    const size_t bufferSize = audioBuffer->num_frames();

    if (m_bufferIn.size() != bufferSize) {
        m_bufferIn.resize(bufferSize);
    }

    for (size_t i = 0; i < bufferSize; ++i) {
        m_bufferIn[i] = audioBuffer->channels()[0][i] / 32768.f;
    }

    ResampleAndDeliver(m_bufferIn.data(), bufferSize, audioBufferSampleRate);
}

void AudioFrameProcessor::InitializeSession(int /*sampleRate*/, int /*numberOfChannels*/)
{
}

void AudioFrameProcessor::Enable(bool /*isEnable*/)
{
}

bool AudioFrameProcessor::IsEnabled() const
{
    return false;
}

void AudioFrameProcessor::SetAudioFrameCallback(AudioFrameCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_audioFrameCallback = std::move(callback);
    m_accumPos = 0;
}

void AudioFrameProcessor::ResampleAndDeliver(const float* src, size_t srcLen, int srcRate)
{
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (!m_audioFrameCallback) {
            return;
        }
    }

    const float* convertSrc;
    size_t convertLen;

    if (srcRate == kOutputSampleRate) {
        convertSrc = src;
        convertLen = srcLen;
    } else {
        const size_t dstLen = (srcLen * kOutputSampleRate) / srcRate;
        if (m_resampleBuffer.size() != dstLen) {
            m_resampleBuffer.resize(dstLen);
        }
        m_resampler.InitializeIfNeeded(srcRate, kOutputSampleRate, /*num_channels=*/1);
        m_resampler.Resample(src, srcLen, m_resampleBuffer.data(), dstLen);
        convertSrc = m_resampleBuffer.data();
        convertLen = dstLen;
    }

    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (!m_audioFrameCallback) {
        return;
    }

    size_t i = 0;
    while (i < convertLen) {
        const size_t remaining = kAccumFrameSize - m_accumPos;
        const size_t chunk = std::min(remaining, convertLen - i);

        for (size_t j = 0; j < chunk; ++j) {
            float s = convertSrc[i + j] * 32768.f;
            s = std::max(-32768.f, std::min(32767.f, s));
            m_accumBuffer[m_accumPos + j] = static_cast<int16_t>(s);
        }
        m_accumPos += chunk;
        i += chunk;

        if (m_accumPos >= kAccumFrameSize) {
            m_audioFrameCallback(m_accumBuffer.data(), kAccumFrameSize);
            m_accumPos = 0;
        }
    }
}

}
