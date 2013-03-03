LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES = libcutils
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libcutils libc

LOCAL_SRC_FILES := own-tty.c

LOCAL_MODULE := own-tty

include $(BUILD_EXECUTABLE)

