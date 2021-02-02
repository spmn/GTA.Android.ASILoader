LOCAL_PATH      := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := RainbowHUD
LOCAL_SRC_FILES := main.c
LOCAL_CFLAGS    := -Wall -Wextra
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
