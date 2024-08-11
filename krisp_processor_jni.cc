#include "krisp_processor.hpp"

#include <syslog.h>
#include <cstring>
#include <memory>

#include "rtc_base/time_utils.h"
#include "rtc_base/checks.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "sdk/android/src/jni/jni_helpers.h"
#include "webrtc-android-krisp-module/generated_krisp_jni/KrispAudioProcessingImpl_jni.h"


namespace Krisp
{

static webrtc::AudioProcessing* apmPtr;

static jlong JNI_KrispAudioProcessingImpl_GetAudioProcessorModule(JNIEnv* env)
{

   std::unique_ptr<webrtc::CustomProcessing> krisp_processor(
   			KrispProcessor::GetInstance());
   auto apm = webrtc::AudioProcessingBuilder()
    		.SetCapturePostProcessing(std::move(krisp_processor))
   			.Create();
   webrtc::AudioProcessing::Config config;
   config.echo_canceller.enabled = false;
   config.echo_canceller.mobile_mode = true;
   apm->ApplyConfig(config);
   apmPtr = apm.release();
   return webrtc::jni::jlongFromPointer(apmPtr);

}

static void JNI_KrispAudioProcessingImpl_Enable(JNIEnv* env, jboolean disable)
{
	KrispProcessor::GetInstance()->Enable(disable);
}

static jboolean JNI_KrispAudioProcessingImpl_IsEnabled(JNIEnv* env)
{
	return KrispProcessor::GetInstance()->IsEnabled();
}

static jboolean JNI_KrispAudioProcessingImpl_Init(JNIEnv* env,
	const webrtc::JavaParamRef<jstring>& modelPathRef,
	const webrtc::JavaParamRef<jstring>& krispDllPath)
{
	jstring javaModelPath = modelPathRef.obj();
	jstring javaDllPath = krispDllPath.obj();
	const char *modelFilePath = env->GetStringUTFChars(javaModelPath, nullptr);
	const char *dllPath = env->GetStringUTFChars(javaDllPath, nullptr);
	bool retValue = KrispProcessor::GetInstance()->Init(modelFilePath, dllPath);
	env->ReleaseStringUTFChars(javaModelPath, modelFilePath);
	return static_cast<jboolean>(retValue); 
}

static jboolean JNI_KrispAudioProcessingImpl_InitWithData(JNIEnv* env,
	const webrtc::JavaParamRef<jbyteArray>& modelDataRef,
	const webrtc::JavaParamRef<jstring>& krispDllPath)
{
	jbyteArray javaByteArray = modelDataRef.obj();
	jstring javaDllPath = krispDllPath.obj();
	jsize javaModelSize = env->GetArrayLength(javaByteArray);
	jbyte *javaModelData = env->GetByteArrayElements(javaByteArray, nullptr);
	const char *dllPath = env->GetStringUTFChars(javaDllPath, nullptr);
	size_t arraySize = static_cast<unsigned int>(javaModelSize);
	std::unique_ptr<char[]> modelData(new char[arraySize]);
	std::memcpy(modelData.get(), javaModelData, arraySize);
	bool retValue = KrispProcessor::GetInstance()->Init(modelData.get(), arraySize, dllPath);
	env->ReleaseByteArrayElements(javaByteArray, javaModelData, JNI_ABORT);
	return static_cast<jboolean>(retValue); 
}

static void JNI_KrispAudioProcessingImpl_Destroy(JNIEnv* env)
{
	delete apmPtr;
	apmPtr = nullptr;
}

}
