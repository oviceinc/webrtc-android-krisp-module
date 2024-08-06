#include "krisp_processor.hpp"
#include "krisp-android-wrapper/generated_krisp_jni/KrispAudioProcessingImpl_jni.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/checks.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "sdk/android/src/jni/jni_helpers.h"
#include <syslog.h>
#include <cstring>

namespace Krisp {
webrtc::AudioProcessing* apm_ptr;
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
   apm_ptr = apm.release();
   return webrtc::jni::jlongFromPointer(apm_ptr);

}

static void JNI_KrispAudioProcessingImpl_Enable(JNIEnv* env, jboolean disable)
{
	KrispProcessor::Enable(disable);
}

static jboolean JNI_KrispAudioProcessingImpl_IsEnabled(JNIEnv* env)
{
	return KrispProcessor::IsEnabled();
}

static jboolean JNI_KrispAudioProcessingImpl_Init(JNIEnv* env, const webrtc::JavaParamRef<jstring>& model, const webrtc::JavaParamRef<jstring>& krispDllPath)
{
   jstring modelName = model.obj();
   jstring jsDllPath = krispDllPath.obj();
   const char *ch_name = env->GetStringUTFChars(modelName, nullptr);
   const char *dllPath = env->GetStringUTFChars(jsDllPath, nullptr);
   bool retValue = KrispProcessor::Init(ch_name, dllPath);
   env->ReleaseStringUTFChars(modelName, ch_name);
   
   return static_cast<jboolean>(retValue); 
}

static jboolean JNI_KrispAudioProcessingImpl_InitWithData(JNIEnv* env, const webrtc::JavaParamRef<jbyteArray>& data, const webrtc::JavaParamRef<jstring>& krispDllPath)
{
   jbyteArray jdata = data.obj();
   jstring jsDllPath = krispDllPath.obj();
   jsize size = env->GetArrayLength(jdata);

   jbyte *elements = env->GetByteArrayElements(jdata, nullptr);
   const char *dllPath = env->GetStringUTFChars(jsDllPath, nullptr);
   unsigned int csize = static_cast<unsigned int>(size);
   char* charArray = new char[csize];
   std::memcpy(charArray, elements, csize);

   bool retValue = KrispProcessor::Init(charArray,csize, dllPath);

   env->ReleaseByteArrayElements(jdata, elements, JNI_ABORT);
   delete []charArray;

   return static_cast<jboolean>(retValue); 
}

static void JNI_KrispAudioProcessingImpl_Destroy(JNIEnv* env)
{
	delete apm_ptr;
}

}//end of namespace Krisp
