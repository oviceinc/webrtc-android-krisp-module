#include "krisp_audio_sdk_api_wrapper.h"

#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <syslog.h>
#include <string>

namespace Krisp {

void* KrispAudioSDKWrapper::m_handle = nullptr;

void* KrispAudioSDKWrapper::GetHandle()
{
    return KrispAudioSDKWrapper::m_handle;
}

bool KrispAudioSDKWrapper::LoadDLL(const char* krispDllPath) {
    ClearCache();
    m_handle = dlopen(krispDllPath, RTLD_LAZY);
    if (!m_handle) {
        syslog(LOG_ERR, "KrispAudioSDKWrapper::LoadDLL: Failed to load the library = %s\n", krispDllPath);
        return false;
    }
    return true;
}

void KrispAudioSDKWrapper::UnLoadDLL() {
    AudioGlobalDestroy();
    if (m_handle) {
        dlclose(m_handle); 
    }
    ClearCache();
}

bool KrispAudioSDKWrapper::AudioGlobalInit(void* param) {
    int result = InvokeFunction<int>("krispAudioGlobalInit", param);
    return  result == 0 ? true: false;
}

int KrispAudioSDKWrapper::AudioSetModel(const wchar_t*  weightFilePath, const char* modelName) {
    int result = InvokeFunction<int>("krispAudioSetModel", weightFilePath, modelName);
    return result;
}

int KrispAudioSDKWrapper::AudioSetModelBlob(const void* modelAddress, unsigned int modelSize, const char* modelName) {
    int result = InvokeFunction<int>("krispAudioSetModelBlob", modelAddress, modelSize, modelName);
    return result;
}

int KrispAudioSDKWrapper::AudioGlobalDestroy() {
   int result = InvokeFunction<int>("krispAudioGlobalDestroy");
   return result;
}

int KrispAudioSDKWrapper::AudioNcCloseSession(void* m_session) {
   int result = InvokeFunction<int>("krispAudioNcCloseSession", m_session);
   return result;
}

KrispAudioSessionID KrispAudioSDKWrapper::AudioNcCreateSession(KrispAudioSamplingRate inputSampleRate,
    KrispAudioSamplingRate    outputSampleRate,
    KrispAudioFrameDuration   frameDuration,
    const char*               modelName) {
    
    KrispAudioSessionID result = InvokeFunction<KrispAudioSessionID>("krispAudioNcCreateSession", 
        inputSampleRate, 
        outputSampleRate,
        frameDuration,
        modelName);

    return result;
}

 int KrispAudioSDKWrapper::AudioNcCleanAmbientNoiseFloat(KrispAudioSessionID  pSession,
                                   const float*         pFrameIn,
                                   unsigned int         frameInSize,
                                   float*               pFrameOut,
                                   unsigned int         frameOutSize) {

    int result = InvokeFunction<int>("krispAudioNcCleanAmbientNoiseFloat",
        pSession,
        pFrameIn,
        frameInSize,
        pFrameOut,
        frameOutSize);

    return result;
}
std::unordered_map<std::string, void*>& KrispAudioSDKWrapper::GetFunctionPointerCache() {
    static std::unordered_map<std::string, void*>* functionCache = new std::unordered_map<std::string, void*>();
    return *functionCache;
}

void KrispAudioSDKWrapper::ClearCache()
{
    GetFunctionPointerCache().clear();
}

template<typename ReturnType, typename... Args>
ReturnType KrispAudioSDKWrapper::InvokeFunction(const char* functionName, Args... args) {

    dlerror();    
    if (!KrispAudioSDKWrapper::m_handle) {
        syslog(LOG_INFO, "KrispAudioSDKWrapper::InvokeFunction invalid handle: %p", KrispAudioSDKWrapper::m_handle);
        return ReturnType(); 
    }

    using FuncType = ReturnType (*)(Args...);
    std::string key = functionName;
    auto& functionCache = GetFunctionPointerCache();
    if (functionCache.find(key) == functionCache.end()) {
        FuncType function = reinterpret_cast<FuncType>(dlsym(KrispAudioSDKWrapper::m_handle, functionName));
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            syslog(LOG_ERR, "KrispAudioSDKWrapper::InvokeFunction: Error finding symbol: %s", dlsym_error);
            return ReturnType(); 
        }
        functionCache[key] = reinterpret_cast<void*>(function);
    }
 
    FuncType function = reinterpret_cast<FuncType>(functionCache[key]);
    return function(args...);
}

} 