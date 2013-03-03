LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := bench-binder.c binder.c

LOCAL_MODULE := bench-binder
LOCAL_SHARED_LIBRARIES = libcutils

# for now, until I do a full rebuild.
#LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)

