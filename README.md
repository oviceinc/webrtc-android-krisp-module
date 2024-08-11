# WebRTC Android Krisp Module

## Step 1: Checkout WebRTC Repository
```
mkdir workdir
cd workdir
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:$PATH
fetch --nohooks webrtc_android
cd src
git checkout branch-heads/6478
git checkout -b 6478-modified a18e38fed2307edd6382760213fa3ddf199fa181
cd ..
gclient sync
cd src
```

## Step 2: Checkout the WebRTC Android Krisp Module Into The Repo
```
git clone git@github.com:krispai/webrtc-android-krisp-module.git
```

## Step 3: Modify the sdk/andorid/BUILD.gn file
```
Add krisp_processor library into the dependency list of the rtc_shared_library(“libjingle_peerconnection_so”)
deps = [ “../../webrtc-android-krisp-module:krisp_processor”, ….]
Add krisp_java library into the dependency list of the dist_jar(“libwebrtc”), for creating public java modules
deps = [ “../../webrtc-android-krisp-module:krisp_java”, ….]
```
See the screenshot.


<img width="662" alt="BUILD gn-mod" src="https://github.com/user-attachments/assets/adb3d064-923a-4dcb-8c35-bb04a5501015">

## Step 4: Build the WebRTC Android library
```
python3  tools_webrtc/android/build_aar.py
```

## Step 5: Get the WebRTC Android library
```
ls libwebrtc.aar
```
