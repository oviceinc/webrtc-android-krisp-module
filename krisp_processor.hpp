#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/audio_processing_impl.h"
#include "modules/audio_processing/audio_buffer.h"

namespace Krisp {

class KrispProcessor : public webrtc::CustomProcessing {
public:

    KrispProcessor(const KrispProcessor&) = delete;
    KrispProcessor(KrispProcessor&&) = delete;
    KrispProcessor& operator=(const KrispProcessor&) = delete;
    KrispProcessor& operator=(KrispProcessor&&) = delete;
    ~KrispProcessor();

    static KrispProcessor* GetInstance();
    static bool Init(const char* modelPath, const char* krispDllPath);
    static bool Init(const void* modelAddr, unsigned int modelSize, const char* krispDllPath);
    static void DeInit();
    static void Enable(bool isEnable);
    static bool IsEnabled();

private:
    KrispProcessor();
    void UnloadDLL();

    static void* sdkHandle;
    static bool m_isEnabled;
    static KrispProcessor* m_instance;

    void* m_session;
    int m_sampleRate;
    int m_numberOfChannels;
    long m_lastTimeStamp;
    std::vector<float> m_bufferIn;
    std::vector<float> m_bufferOut;

    static void * CreateAudioSession(int sampleRate);

    void Initialize(int sampleRate, int numOfChannels) override ;
    void Process(webrtc::AudioBuffer* audioBuffer) override;
    std::string ToString() const override;
    void SetRuntimeSetting(webrtc::AudioProcessing::RuntimeSetting setting) override;
};
}
