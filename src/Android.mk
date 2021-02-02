LOCAL_PATH      := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := ASILoader
LOCAL_SRC_FILES := main.c
LOCAL_CFLAGS    := -Wall -Wextra
LOCAL_LDLIBS    := -llog -ldl

include $(BUILD_SHARED_LIBRARY)
