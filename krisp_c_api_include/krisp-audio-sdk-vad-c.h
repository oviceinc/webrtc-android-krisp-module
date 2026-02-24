///
/// Copyright Krisp, Inc
///
#pragma once

#include "krisp-audio-api-definitions-c.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief VAD session configuration.
typedef struct
{
    /// @brief Sampling frequency of the input data.
    KrispSamplingRate inputSampleRate;

    /// @brief Input audio frame duration.
    KrispFrameDuration inputFrameDuration;

    /// @brief VAD model configuration.
    KrispModelInfo* modelInfo;
} KrispVadSessionConfig;

typedef uint64_t krispVadHandle;

/// @brief Creates a new instance of Vad session for int16 stream processing.
///        AI technology detects voice activity in real-time audio streams
/// @param[in] config Configuration for the Vad Session.
/// @retval Valid pointer on success, otherwise NULL.
KRISP_AUDIO_API krispVadHandle krispCreateVadInt16(const KrispVadSessionConfig* config);

/// @brief Creates a new instance of Vad session for float stream processing.
///        AI technology detects voice activity in real-time audio streams
/// @param[in] config Configuration for the Vad Session.
/// @retval Valid pointer on success, otherwise NULL.
KRISP_AUDIO_API krispVadHandle krispCreateVadFloat(const KrispVadSessionConfig* config);

/// @brief Destroys the Vad instance.
///        Should be called if the Vad instance is no longer needed, before krispGlobalDestroy()
/// @param vadSessionHandle The handle of Vad instance to destroy.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispDestroyVad(const krispVadHandle vadSessionHandle);

/// @brief Processes an input frame of audio data with int16 samples.
/// @param[in] vadSessionHandle The handle of Vad instance to process the audio data.
/// @param[in] inputSamples Pointer to the input buffer containing audio samples.
///                         The buffer should hold enough samples to fill a frame of audio data,
///                         calculated as frameDuration * inputSampleRate / 1000 of FrameDataType samples.
/// @param[in] numInputSamples The number of samples in the input buffer.
///                            Must be sufficient to match the expected input frame size.
/// @param[out] vadOutput voice activity status in range [0, 1].
///                       The default threshold indicating the state of voice availability is vadOutput >= 0.5.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispProcessVadInt16(
    const krispVadHandle vadSessionHandle,
    const int16_t* inputSamples,
    size_t numInputSamples,
    float* vadOutput);

/// @brief Processes an input frame of audio data with float samples.
/// @param[in] vadSessionHandle The handle of Vad instance to process the audio data.
/// @param[in] inputSamples Pointer to the input buffer containing audio samples.
///                         The buffer should hold enough samples to fill a frame of audio data,
///                         calculated as frameDuration * inputSampleRate / 1000 of FrameDataType samples.
/// @param[in] numInputSamples The number of samples in the input buffer.
///                            Must be sufficient to match the expected input frame size.
/// @param[out] vadOutput voice activity status in range [0, 1].
///                       The default threshold indicating the state of voice availability is vadOutput >= 0.5.
/// @retval KrispRetValSuccess on success
KRISP_AUDIO_API KrispRetVal krispProcessVadFloat(
    const krispVadHandle vadSessionHandle,
    const float* inputSamples,
    size_t numInputSamples,
    float* vadOutput);

#ifdef __cplusplus
} // extern "C"
#endif
