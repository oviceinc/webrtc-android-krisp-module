package org.webrtc;
import org.webrtc.AudioProcessingFactory;

public class KrispAudioProcessingFactory implements AudioProcessingFactory {	
  private long nativeModule;

  @Override
  public long createNative() {
    nativeModule = nativeCreateModule();
    return nativeGetAudioProcessorModule(nativeModule);
  }

  public static boolean LoadKrisp(String dllPath) {
      return nativeLoadKrisp(dllPath);
  }

  public static boolean UnloadKrisp() {
      return nativeUnloadKrisp();
  }

  public boolean Init(String modelPath) {
      return nativeInit(nativeModule, modelPath);
  }

  public boolean InitWithData(byte[] modelData) {
      return nativeInitWithData(nativeModule, modelData);
  }

  public void Enable(boolean enable) {
      nativeEnable(nativeModule, enable);
  }

  public boolean IsEnabled() {
      return nativeIsEnabled(nativeModule);
  }

  public void Destroy() {
      nativeDestroy(nativeModule);
      nativeModule = 0;
  }

  private static native void nativeEnable(long nativeModule, boolean disable);

  private static native boolean nativeIsEnabled(long nativeModule);

  private static native boolean nativeInit(long nativeModule, String modelPath);

  private static native boolean nativeInitWithData(long nativeModule, byte[] dataData);

  private static native boolean nativeLoadKrisp(String dllPath);

  private static native boolean nativeUnloadKrisp();

  private static native void nativeDestroy(long nativeModule);

  private static native long nativeCreateModule();

  private static native long nativeGetAudioProcessorModule(long nativeModule);
}


