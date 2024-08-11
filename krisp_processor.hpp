#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/audio_processing_impl.h"
#include "modules/audio_processing/audio_buffer.h"


namespace Krisp
{

class KrispProcessor : public webrtc::CustomProcessing
{
public:

    KrispProcessor(const KrispProcessor&) = delete;
    KrispProcessor(KrispProcessor&&) = delete;
    KrispProcessor& operator=(const KrispProcessor&) = delete;
    KrispProcessor& operator=(KrispProcessor&&) = delete;
    ~KrispProcessor();

    static KrispProcessor* GetInstance();

    bool Init(const char* modelPath, const char* krispDllPath);
    bool Init(const void* modelAddr, unsigned int modelSize, const char* krispDllPath);
    void DeInit();
    void Enable(bool isEnable);
    bool IsEnabled() const;

private:
    KrispProcessor();

    static KrispProcessor* _singleton;

    bool m_isEnabled;
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
