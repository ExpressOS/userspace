LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := main_helloworldservice.cpp helloworldservice.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

LOCAL_CFLAGS += -DPLATFORM_ANDROID

LOCAL_MODULE := helloworldservice

# for now, until I do a full rebuild.
LOCAL_PRELINK_MODULE := false

#LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libbinder

include $(BUILD_EXECUTABLE)

