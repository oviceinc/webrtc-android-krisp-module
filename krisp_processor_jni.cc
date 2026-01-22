#include "krisp_processor.hpp"

#include <syslog.h>
#include <cstring>
#include <memory>

#include "sdk/android/src/jni/jni_helpers.h"


namespace Krisp
{

#if defined(__GNUC__)
#define JNI_UNUSED __attribute__((unused))
#else
#define JNI_UNUSED
#endif

static NativeKrispModule* GetModule(jlong native_module) {
	return reinterpret_cast<NativeKrispModule*>(native_module);
}

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModule(JNIEnv* env)
{
	auto module = NativeKrispModule::Create();
	return webrtc::jni::jlongFromPointer(module.release());
}

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_GetAudioProcessorModule(
	JNIEnv* env,
	jlong native_module)
{
	auto* module = GetModule(native_module);
	if (!module || !module->apm) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(module->apm.get());
}

static jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_LoadKrisp(
	JNIEnv* env,
	const webrtc::JavaParamRef<jstring>& krispDllPath)
{
	const char *dllPath = env->GetStringUTFChars(krispDllPath.obj(), nullptr);
	bool retValue = LoadKrisp(dllPath);
	env->ReleaseStringUTFChars(krispDllPath.obj(), dllPath);
	return static_cast<jboolean>(retValue);
}

static jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_UnloadKrisp(JNIEnv* env)
{
	return static_cast<jboolean>(UnloadKrisp());
}

static void JNI_UNUSED JNI_KrispAudioProcessingFactory_Enable(
	JNIEnv* env,
	jlong native_module,
	jboolean disable)
{
	auto* module = GetModule(native_module);
	if (module && module->apm) {
		module->apm->SetRuntimeSetting(
			webrtc::AudioProcessing::RuntimeSetting::CreateCaptureOutputUsedSetting(disable));
	}
}

static jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_IsEnabled(
	JNIEnv* env,
	jlong native_module)
{
	auto* module = GetModule(native_module);
	return module && module->proc ? module->proc->IsEnabled() : false;
}

static jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_Init(JNIEnv* env,
	jlong native_module,
	const webrtc::JavaParamRef<jstring>& modelPathRef)
{
	auto* module = GetModule(native_module);
	jstring javaModelPath = modelPathRef.obj();
	const char *modelFilePath = env->GetStringUTFChars(javaModelPath, nullptr);
	bool retValue = module && module->proc ? module->proc->Init(modelFilePath) : false;
	env->ReleaseStringUTFChars(javaModelPath, modelFilePath);
	return static_cast<jboolean>(retValue); 
}

static jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_InitWithData(JNIEnv* env,
	jlong native_module,
	const webrtc::JavaParamRef<jbyteArray>& modelDataRef)
{
	auto* module = GetModule(native_module);
	jbyteArray javaByteArray = modelDataRef.obj();
	jsize javaModelSize = env->GetArrayLength(javaByteArray);
	jbyte *javaModelData = env->GetByteArrayElements(javaByteArray, nullptr);
	size_t arraySize = static_cast<unsigned int>(javaModelSize);
	std::unique_ptr<char[]> modelData(new char[arraySize]);
	std::memcpy(modelData.get(), javaModelData, arraySize);
	bool retValue = module && module->proc ? module->proc->Init(modelData.get(), arraySize) : false;
	env->ReleaseByteArrayElements(javaByteArray, javaModelData, JNI_ABORT);
	return static_cast<jboolean>(retValue); 
}

static void JNI_UNUSED JNI_KrispAudioProcessingFactory_Destroy(
	JNIEnv* env,
	jlong native_module)
{
	delete GetModule(native_module);
}

#undef JNI_UNUSED

}
