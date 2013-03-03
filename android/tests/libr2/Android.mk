LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += ioctl.c

LOCAL_MODULE := libr2

# for now, until I do a full rebuild.
#LOCAL_PRELINK_MODULE := false

LOCAL_LDFLAGS += -ldl

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
