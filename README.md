# SA.Android.ASILoader

Proof of concept shared library (`.so`) loader for GTA:SA

## Quick info

Android provides two locations for apps to store files into:

  1. internal storage: no permissions needed, **only** the owning app can access the files (not even the user!)
  2. external storage: permissions needed, any app can access the files (including the user)

For security reasons, Android apps can load shared libraries from these locations only:

  1. APK (hard/time consuming to patch)
  2. system directories (not writable by the user/app)
  3. internal storage (readable/writable by the app only)

Usually, mods circumvent this loader restriction by packing the `.so` files directly into the APK.
However, this approach comes with a big downside: you have to rebuild and reinstall the APK every time you make a change to the mod.

## Solution

A master mod (ASI Loader) is packed into the APK like a normal mod.
Everytime the app starts, the ASI Loader scans `Android/data/com.rockstargames.gtasa/ASI/` directory for `.asi` files,
copies them to internal storage then loads them into the process.

This way libs can be updated with a simple `adb push` to the app external directory.

## Requirements

- GTA:SA .apk file (preferably the latest version, otherwise the patch file might fail)
- Android NDK
- apktool
- uber-apk-signer

## Installation

1. Unpack the APK (apktool)
2. Apply `src/WarMedia.patch` (patch)
3. Build the ASI Loader (ndk-build)
4. Copy `libASILoader.so` to `<unpack>/lib/armeabi-v7a/`
5. Rebuild the APK (apktool)
6. Sign the APK (uber-apk-signer)
7. Copy and install the signed APK

If everything went good, you should see a message in `logcat` from the ASI Loader.
You can also build and load the example mod.
