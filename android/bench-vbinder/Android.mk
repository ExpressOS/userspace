LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES = libcutils
#LOCAL_FORCE_STATIC_EXECUTABLE := true
#LOCAL_STATIC_LIBRARIES := liblog libcutils libc

LOCAL_SRC_FILES := vbinder-common.c main.c vbinder.S

LOCAL_MODULE := bench-vbinder

include $(BUILD_EXECUTABLE)

