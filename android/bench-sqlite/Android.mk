LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := bench.c sqlite3.c

LOCAL_MODULE := bench-sqlite
LOCAL_SHARED_LIBRARIES = libcutils
LOCAL_C_INCLUDES := external/sqlite/dist

#LOCAL_LDFLAGS += -Bstatic

#LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := liblog libcutils libc


include $(BUILD_EXECUTABLE)

