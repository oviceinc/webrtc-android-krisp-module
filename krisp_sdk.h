#include "inc/krisp-audio-sdk.hpp"
#include "inc/krisp-audio-sdk-nc.hpp"


namespace KrispSDK
{
	bool LoadDll(const char* dllPath);
	void UnloadDll();
	bool GlobalInit(void* param);
	int SetModel(const wchar_t*  weightFilePath, const char* modelName);
	int SetModelBlob(const void* weightBlob, unsigned int blobSize, const char* modelName);
	int RemoveModel(const char* modelName);
	int GlobalDestroy();
	int NcCloseSession(void* m_session); 
	KrispAudioSessionID NcCreateSession(
		KrispAudioSamplingRate inputSampleRate,
		KrispAudioSamplingRate outputSampleRate,
		KrispAudioFrameDuration frameDuration,
		const char* modelName);
	int NcCleanAmbientNoiseFloat(
		KrispAudioSessionID pSession,
		const float* pFrameIn,
		unsigned int frameInSize,
		float* pFrameOut,
		unsigned int frameOutSize);
}
