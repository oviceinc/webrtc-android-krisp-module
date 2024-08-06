package org.webrtc;
import org.webrtc.AudioProcessingFactory;

public class KrispAudioProcessingImpl implements AudioProcessingFactory {	
  @Override
  public long createNative() {
    return nativeGetAudioProcessorModule();
  }

  public boolean Init(String modelPath, String dllPath) {
      return nativeInit(modelPath, dllPath);
  }

  public boolean InitWithData(byte[] modelData, String dllPath) {
      return nativeInitWithData(modelData, dllPath);
  }

  public void Enable(boolean enable) {
      nativeEnable(enable);
  }

  public boolean IsEnabled() {
      return nativeIsEnabled();
  }

  public void Destroy() {
      nativeDestroy();
  }

  private static native void nativeEnable(boolean disable);

  private static native boolean nativeIsEnabled();

  private static native boolean nativeInit(String modelPath, String dllPath);

  private static native boolean nativeInitWithData(byte[] dataData,  String dllPath);

  private static native void nativeDestroy();

  private static native long nativeGetAudioProcessorModule();
}


