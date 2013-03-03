LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

EXPRESSOS_SRC_DIR := /home/mai4/work/expressos/src

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := binder.c \
	fs.c \
	futex.c \
	init.c \
	main.c \
	mm.c \
	net.c \
	upcall.c \
        sys_futex.S

LOCAL_MODULE := venus

LOCAL_C_INCLUDES := $(EXPRESSOS_SRC_DIR)/kernel/expressos/native/include
LOCAL_CFLAGS = -Wall -Wextra

#LOCAL_LDFLAGS += -Bstatic

#LOCAL_FORCE_STATIC_EXECUTABLE := true
#LOCAL_STATIC_LIBRARIES := libcutils libc
#LOCAL_SHARED_LIBRARIES += liblog 
#LOCAL_SHARED_LIBRARIES += libutils

include $(BUILD_EXECUTABLE)

