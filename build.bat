@ECHO OFF

SET NDK_PROJECT_PATH=.
SET REMOTE_APK_PATH=/storage/emulated/0
SET REMOTE_ASI_PATH=/storage/emulated/0/Android/data/com.rockstargames.gta%1/files/ASI

SET APK=base-%1
SET BUILD=%2

IF "%APK%"=="base-3"   SET REMOTE_ASI_PATH=/storage/emulated/0/Android/data/com.rockstar.gta3/files/ASI
IF "%APK%"=="base-"    GOTO :usage
IF "%BUILD%"=="asi"    GOTO :build_asi
IF "%BUILD%"=="apk"    GOTO :build_loader
IF "%BUILD%"=="loader" GOTO :build_loader
GOTO :build_loader


:build_asi
CD examples\RainbowHUD
CALL ndk-build NDK_APPLICATION_MK=Application.mk
CD ..\..
adb push examples\RainbowHUD\libs\armeabi-v7a\libRainbowHUD.so %REMOTE_ASI_PATH%/Rainbow.asi
GOTO :eof


:build_loader
CD src
CALL ndk-build NDK_APPLICATION_MK=Application.mk
CD ..
IF "%BUILD%"=="loader" GOTO :eof


:build_apk
COPY src\libs\armeabi-v7a\libASILoader.so apks\%APK%\lib\armeabi-v7a\
CALL apktool b apks\%APK%
CALL uber-apk-signer -a apks\%APK%\dist\%APK%.apk
adb push apks\%APK%\dist\%APK%-aligned-debugSigned.apk %REMOTE_APK_PATH%
GOTO :eof


:usage
ECHO %0 [3/vc/sa] [apk/asi/loader]
