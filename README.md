# krisp-android-wrapper

Step 1: Install Google depot_tools.
```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Step 2: Add the depot_tools installation root folder to the PATH variable.
```
export PATH=`pwd`/depot_tools:$PATH
```

Step 3: Get webrtc sources
```
fetch --nohooks webrtc_android
gclient sync
cd src
```

Step 4: Modify sdk/andorid/BUILD.gn file
```
Add krisp_processor library into the dependency list of rtc_shared_library(“libjingle_peerconnection_so”)
deps = [ “../../krisp-android-wrapper:krisp_processor”, ….]

Add krisp_java library into the dependency list of dist_jar(“libwebrtc”), for creating public java modules
deps = [ “../../krisp-android-wrapper:krisp_java”, ….]

```

Step 5: Clone  krisp-android-wrapper directory
```
git@github.com:krispai/krisp-android-wrapper.git
```

Step 6: Build libwebrtc.aar
```
python3  tools_webrtc/android/build_aar.py
```
