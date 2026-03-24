#include "krisp_processor.hpp"

#include <cstring>
#include <memory>

#include "sdk/android/src/jni/jni_helpers.h"

namespace {

JNIEnv* GetCachedJNIEnv(JavaVM* jvm) {
	struct JNIEnvGuard {
		JavaVM* vm = nullptr;
		JNIEnv* env = nullptr;
		bool attached = false;
		~JNIEnvGuard() {
			if (attached && vm) {
				vm->DetachCurrentThread();
			}
		}
	};
	static thread_local JNIEnvGuard guard;

	if (guard.env && guard.vm == jvm) {
		return guard.env;
	}

	JNIEnv* env = nullptr;
	jint res = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
	if (res == JNI_EDETACHED) {
		if (jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
			return nullptr;
		}
		guard.attached = true;
	} else if (res != JNI_OK) {
		return nullptr;
	}
	guard.vm = jvm;
	guard.env = env;
	return env;
}

}  // namespace

namespace Krisp {
class Module {
public:
	explicit Module(std::unique_ptr<NativeKrispModule> module)
		: module_(std::move(module)) {}

	~Module() {
		ClearListener(nullptr);
	}

	jlong GetAudioProcessorModule(JNIEnv* env);
	jboolean Init(JNIEnv* env, const webrtc::JavaParamRef<jstring>& modelPathRef);
	jboolean InitWithData(JNIEnv* env, const webrtc::JavaParamRef<jbyteArray>& modelDataRef);
	void Enable(JNIEnv* env, jboolean enable);
	jboolean IsEnabled(JNIEnv* env);
	void SetAudioDataListener(JNIEnv* env,
		const webrtc::JavaParamRef<jobject>& factoryObj);
	void Destroy(JNIEnv* env);

private:
	void ClearListener(JNIEnv* env);

	std::unique_ptr<NativeKrispModule> module_;
	JavaVM* jvm_ = nullptr;
	jobject javaFactoryRef_ = nullptr;
	jmethodID onAudioDataMethodId_ = nullptr;
	jmethodID onVADStateChangeMethodId_ = nullptr;
	jshortArray javaShortArrayRef_ = nullptr;
};
}

#if __has_include("webrtc-android-krisp-module/generated_krisp_jni/KrispAudioProcessingFactory_jni.h")
#include "webrtc-android-krisp-module/generated_krisp_jni/KrispAudioProcessingFactory_jni.h"
#endif

namespace Krisp
{

#if defined(__GNUC__)
#define JNI_UNUSED __attribute__((unused))
#else
#define JNI_UNUSED
#endif

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModule(JNIEnv* env)
{
	auto module = NativeKrispModule::Create();
	return webrtc::jni::jlongFromPointer(new Module(std::move(module)));
}

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModuleWithModelPath(
	JNIEnv* env,
	const webrtc::JavaParamRef<jstring>& modelPathRef)
{
	const char *modelFilePath = env->GetStringUTFChars(modelPathRef.obj(), nullptr);
	auto module = NativeKrispModule::CreateWithModelPath(modelFilePath);
	env->ReleaseStringUTFChars(modelPathRef.obj(), modelFilePath);
	if (!module) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(new Module(std::move(module)));
}

static jlong JNI_UNUSED JNI_KrispAudioProcessingFactory_CreateModuleWithModelData(
	JNIEnv* env,
	const webrtc::JavaParamRef<jbyteArray>& modelDataRef)
{
	jbyteArray javaByteArray = modelDataRef.obj();
	jsize javaModelSize = env->GetArrayLength(javaByteArray);
	jbyte *javaModelData = env->GetByteArrayElements(javaByteArray, nullptr);
	auto module = NativeKrispModule::CreateWithModelData(
		javaModelData, static_cast<unsigned int>(javaModelSize));
	env->ReleaseByteArrayElements(javaByteArray, javaModelData, JNI_ABORT);
	if (!module) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(new Module(std::move(module)));
}

jlong Module::GetAudioProcessorModule(JNIEnv* env)
{
	if (!module_ || !module_->apm) {
		return 0;
	}
	return webrtc::jni::jlongFromPointer(module_->apm.get());
}

jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_LoadKrisp(
	JNIEnv* env,
	const webrtc::JavaParamRef<jstring>& krispDllPath)
{
	const char *dllPath = env->GetStringUTFChars(krispDllPath.obj(), nullptr);
	bool retValue = LoadKrisp(dllPath);
	env->ReleaseStringUTFChars(krispDllPath.obj(), dllPath);
	return static_cast<jboolean>(retValue);
}

jboolean JNI_UNUSED JNI_KrispAudioProcessingFactory_UnloadKrisp(JNIEnv* env)
{
	return static_cast<jboolean>(UnloadKrisp());
}

void Module::Enable(JNIEnv* env, jboolean enable)
{
	if (!module_ || !module_->proc) {
		return;
	}
	module_->proc->Enable(static_cast<bool>(enable));
}

jboolean Module::IsEnabled(JNIEnv* env)
{
	if (!module_ || !module_->proc) {
		return false;
	}
	return module_->proc->IsEnabled();
}

jboolean Module::Init(JNIEnv* env, const webrtc::JavaParamRef<jstring>& modelPathRef)
{
	if (!module_ || !module_->krispFilter) {
		return false;
	}
	jstring javaModelPath = modelPathRef.obj();
	const char *modelFilePath = env->GetStringUTFChars(javaModelPath, nullptr);
	bool retValue = module_->krispFilter->Init(modelFilePath);
	env->ReleaseStringUTFChars(javaModelPath, modelFilePath);
	return static_cast<jboolean>(retValue);
}

jboolean Module::InitWithData(JNIEnv* env,
	const webrtc::JavaParamRef<jbyteArray>& modelDataRef)
{
	if (!module_ || !module_->krispFilter) {
		return false;
	}
	jbyteArray javaByteArray = modelDataRef.obj();
	jsize javaModelSize = env->GetArrayLength(javaByteArray);
	jbyte *javaModelData = env->GetByteArrayElements(javaByteArray, nullptr);
	size_t arraySize = static_cast<unsigned int>(javaModelSize);
	std::unique_ptr<char[]> modelData(new char[arraySize]);
	std::memcpy(modelData.get(), javaModelData, arraySize);
	bool retValue = module_->krispFilter->Init(modelData.get(), arraySize);
	env->ReleaseByteArrayElements(javaByteArray, javaModelData, JNI_ABORT);
	return static_cast<jboolean>(retValue);
}

void Module::ClearListener(JNIEnv* env) {
	if (module_ && module_->proc) {
		module_->proc->SetAudioFrameCallback(nullptr);
		module_->proc->SetVADStateCallback(nullptr);
	}
	if (!env && jvm_) {
		jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
	}
	if (javaShortArrayRef_) {
		if (env) {
			env->DeleteGlobalRef(javaShortArrayRef_);
		}
		javaShortArrayRef_ = nullptr;
	}
	if (javaFactoryRef_) {
		if (env) {
			env->DeleteGlobalRef(javaFactoryRef_);
		}
		javaFactoryRef_ = nullptr;
	}
	onAudioDataMethodId_ = nullptr;
	onVADStateChangeMethodId_ = nullptr;
}

void Module::SetAudioDataListener(JNIEnv* env,
	const webrtc::JavaParamRef<jobject>& factoryObj)
{
	ClearListener(env);

	if (!factoryObj.obj() || !module_ || !module_->proc) {
		return;
	}

	env->GetJavaVM(&jvm_);
	javaFactoryRef_ = env->NewGlobalRef(factoryObj.obj());
	jclass cls = env->GetObjectClass(factoryObj.obj());
	onAudioDataMethodId_ = env->GetMethodID(cls, "onAudioDataFromNative", "([SFF)V");
	onVADStateChangeMethodId_ = env->GetMethodID(cls, "onVADStateChangeFromNative", "(Z)V");

	static constexpr jsize kJniFrameSize = 1600;  // matches kAccumFrameSize
	jshortArray localArray = env->NewShortArray(kJniFrameSize);
	javaShortArrayRef_ = static_cast<jshortArray>(env->NewGlobalRef(localArray));
	env->DeleteLocalRef(localArray);

	JavaVM* jvm = jvm_;
	jobject ref = javaFactoryRef_;
	jmethodID mid = onAudioDataMethodId_;
	jshortArray jdata = javaShortArrayRef_;

	module_->proc->SetAudioFrameCallback(
		[jvm, ref, mid, jdata](const int16_t* data, size_t count,
			float confidence, float voiceEnergy) {
			JNIEnv* cbEnv = GetCachedJNIEnv(jvm);
			if (!cbEnv) {
				return;
			}

			cbEnv->SetShortArrayRegion(jdata, 0, static_cast<jsize>(count),
				reinterpret_cast<const jshort*>(data));
			cbEnv->CallVoidMethod(ref, mid, jdata,
				static_cast<jfloat>(confidence),
				static_cast<jfloat>(voiceEnergy));

			if (cbEnv->ExceptionCheck()) {
				cbEnv->ExceptionClear();
			}
		});

	jmethodID vadMid = onVADStateChangeMethodId_;
	module_->proc->SetVADStateCallback(
		[jvm, ref, vadMid](bool isSpeech) {
			JNIEnv* cbEnv = GetCachedJNIEnv(jvm);
			if (!cbEnv) {
				return;
			}

			cbEnv->CallVoidMethod(ref, vadMid,
				static_cast<jboolean>(isSpeech));

			if (cbEnv->ExceptionCheck()) {
				cbEnv->ExceptionClear();
			}
		});
}

void Module::Destroy(JNIEnv* env)
{
	ClearListener(env);
	delete this;
}

#undef JNI_UNUSED

}
