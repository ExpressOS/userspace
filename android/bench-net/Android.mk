LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := main.c

LOCAL_MODULE := bench-nc
LOCAL_SHARED_LIBRARIES = libcutils

include $(BUILD_EXECUTABLE)

