#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/audio_buffer.h"

#include "krisp-audio-sdk-nc-c.h"

namespace Krisp
{

class KrispProcessor : public webrtc::CustomProcessing
{
public:

    KrispProcessor(const KrispProcessor&) = delete;
    KrispProcessor(KrispProcessor&&) = delete;
    KrispProcessor& operator=(const KrispProcessor&) = delete;
    KrispProcessor& operator=(KrispProcessor&&) = delete;
    virtual ~KrispProcessor();

    static KrispProcessor* GetInstance();

    bool Init(const char* modelPath, const char* krispDllPath);
    bool Init(const void* modelAddr, unsigned int modelSize, const char* krispDllPath);
    void DeInit();
    void Enable(bool isEnable);
    bool IsEnabled() const;

private:
    KrispProcessor();

    static KrispProcessor* _singleton;
    static std::once_flag _initFlag;

    std::atomic<bool> m_isEnabled;
    int m_numberOfChannels;
    long m_lastTimeStamp;
    std::wstring m_modelPath;
    std::vector<uint8_t> m_modelData;
    std::vector<float> m_bufferIn;
    std::vector<float> m_bufferOut;
    KrispModelInfo m_modelInfo;
    KrispNcSessionConfig m_sessionConfig;
    krispNcHandle m_ncCachedHandle;


    void Initialize(int sampleRate, int numOfChannels) override ;
    void Process(webrtc::AudioBuffer* audioBuffer) override;
    std::string ToString() const override;
    void SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting) override;
};
}
