#include "inc/krisp-audio-sdk.hpp"
#include "inc/krisp-audio-sdk-nc.hpp"

#include <unordered_map>
#include <stdexcept>
#include <dlfcn.h>

namespace Krisp {
    class KrispAudioSDKWrapper {
        static void* m_handle;
    public:
        static void* GetHandle(); 
        
        static bool LoadDLL(const char* dllPath);
        
        static void UnLoadDLL();
        
        static bool AudioGlobalInit(void* param);

        static int AudioSetModel(const wchar_t*  weightFilePath, const char* modelName);

        static int AudioSetModelBlob(const void* weightBlob, unsigned int blobSize, const char* modelName);

        static int AudioGlobalDestroy();

        static int AudioNcCloseSession(void* m_session); 

        static  KrispAudioSessionID AudioNcCreateSession(KrispAudioSamplingRate inputSampleRate,
                KrispAudioSamplingRate    outputSampleRate,
                KrispAudioFrameDuration   frameDuration,
                const char*               modelName);

        static int AudioNcCleanAmbientNoiseFloat(KrispAudioSessionID  pSession,
                                   const float*         pFrameIn,
                                   unsigned int         frameInSize,
                                   float*               pFrameOut,
                                   unsigned int         frameOutSize);

    private:
        static std::unordered_map<std::string, void*>& GetFunctionPointerCache();
        static void ClearCache();
     
        template<typename ReturnType, typename... Args>
        static ReturnType InvokeFunction(const char* functionName, Args... args);
    };
}