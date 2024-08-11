#include "krisp_sdk.h"

#include <stdarg.h>
#include <dlfcn.h>
#include <syslog.h>

#include <array>


namespace KrispSDK {

enum class KrispFunctionId
{
	krispAudioGlobalInit = 0,
	krispAudioGlobalDestroy = 1,
	krispAudioSetModel = 2,
	krispAudioSetModelBlob = 3,
	krispAudioRemoveModel = 4,
	krispAudioNcCreateSession = 5,
	krispAudioNcCloseSession = 6,
	krispAudioNcCleanAmbientNoiseFloat = 7
};

class KrispAudioSdkDllGate
{
public:

	static KrispAudioSdkDllGate * singleton()
	{
		if (!_singleton) 
		{
			_singleton = new KrispAudioSdkDllGate;
		}
		return _singleton;
	}

	bool LoadDll(const char* krispDllPath)
	{
		dlerror();
		_dllHandle = dlopen(krispDllPath, RTLD_LAZY);
		if (!_dllHandle) {
			syslog(LOG_ERR, "KrispSDK::LoadDll: Failed to load the library = %s\n", krispDllPath);
			return false;
		}
		syslog(LOG_INFO, "Krisp DLL loaded: %s", krispDllPath);
		return LoadFunctions();
	}

	void UnloadDll()
	{
		if (_dllHandle)
		{
			dlclose(_dllHandle);
			_dllHandle = nullptr;
		}
		for (auto & functionPtr : _functionPointers)
		{
			functionPtr = nullptr;
		}
	}

	template<typename ReturnType, typename... Args>
	ReturnType InvokeFunction(KrispFunctionId functionId, Args... args)
	{
		dlerror();    
		if (!_dllHandle) {
			syslog(LOG_INFO, "InvokeFunction invalid DLL handle: %p", _dllHandle);
			return ReturnType(); 
		}
		using FuncType = ReturnType (*)(Args...);
		void * functionPtr = _functionPointers[static_cast<size_t>(functionId)];
		FuncType function = reinterpret_cast<FuncType>(functionPtr);
		return function(args...);
	}

private:
	bool LoadFunctions() 
	{
		dlerror();
		for (size_t functionId = 0; functionId < _functionCount; ++functionId)
		{
			const char * functionName = _functionNames[functionId];
			syslog(LOG_INFO,"KrispAudioSdkDllGate::LoadFunctions dlsym the %s", functionName);
			void * functionPtr = dlsym(_dllHandle, functionName);
			const char* dlsym_error = dlerror();
			if (dlsym_error) {
				syslog(LOG_ERR, "KrispAudioSdkDllGate::LoadFunctions Error finding symbol: %s", dlsym_error);
				return false;
			}
			_functionPointers[functionId] = functionPtr;
		}
		return true;
	}

	static KrispAudioSdkDllGate * _singleton;
	void* _dllHandle = nullptr;
	static constexpr unsigned int _functionCount = 8;
	static constexpr std::array<const char *, _functionCount> _functionNames =
	{
		"krispAudioGlobalInit",
		"krispAudioGlobalDestroy",
		"krispAudioSetModel",
		"krispAudioSetModelBlob",
		"krispAudioRemoveModel",
		"krispAudioNcCreateSession",
		"krispAudioNcCloseSession",
		"krispAudioNcCleanAmbientNoiseFloat"
	};
	std::array<void *, _functionCount> _functionPointers = {};
};

KrispAudioSdkDllGate * KrispAudioSdkDllGate::_singleton = nullptr;

template<typename ReturnType, typename... Args>
ReturnType InvokeFunction(KrispFunctionId functionId, Args... args)
{
	return KrispAudioSdkDllGate::singleton()->InvokeFunction<ReturnType, Args...>(functionId, args...);
}

bool LoadDll(const char* krispDllPath)
{
	return KrispAudioSdkDllGate::singleton()->LoadDll(krispDllPath);
}

void UnloadDll()
{
	KrispAudioSdkDllGate::singleton()->UnloadDll();
}

bool GlobalInit(void* param)
{
	int result = InvokeFunction<int>(KrispFunctionId::krispAudioGlobalInit, param);
	return  result == 0 ? true: false;
}

int SetModel(const wchar_t* weightFilePath, const char* modelName)
{
	int result = InvokeFunction<int>(KrispFunctionId::krispAudioSetModel, weightFilePath, modelName);
	return result;
}

int SetModelBlob(const void* modelAddress, unsigned int modelSize, const char* modelName)
{
	int result = InvokeFunction<int>(KrispFunctionId::krispAudioSetModelBlob, modelAddress, modelSize, modelName);
	return result;
}

int RemoveModel(const char* modelName)
{
	return InvokeFunction<int>(KrispFunctionId::krispAudioRemoveModel, modelName);
}

int GlobalDestroy()
{
	int result = InvokeFunction<int>(KrispFunctionId::krispAudioGlobalDestroy);
	return result;
}

int NcCloseSession(void* session)
{
	int result = InvokeFunction<int>(KrispFunctionId::krispAudioNcCloseSession, session);
	return result;
}

KrispAudioSessionID NcCreateSession(
	KrispAudioSamplingRate inputSampleRate,
	KrispAudioSamplingRate outputSampleRate,
	KrispAudioFrameDuration frameDuration,
	const char* modelName)
{
    
    KrispAudioSessionID result = InvokeFunction<KrispAudioSessionID>(KrispFunctionId::krispAudioNcCreateSession,
        inputSampleRate, 
        outputSampleRate,
        frameDuration,
        modelName);

    return result;
}

int NcCleanAmbientNoiseFloat(
	KrispAudioSessionID pSession,
	const float* pFrameIn,
	unsigned int frameInSize,
	float* pFrameOut,
	unsigned int frameOutSize)
{
    int result = InvokeFunction<int>(KrispFunctionId::krispAudioNcCleanAmbientNoiseFloat,
		pSession,
		pFrameIn,
		frameInSize,
		pFrameOut,
		frameOutSize
	);
    return result;
}

} 