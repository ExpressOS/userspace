LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	bootanimation_main.cpp \
	BootAnimation.cpp

# need "-lrt" on Linux simulator to pick up clock_gettime
ifeq ($(TARGET_SIMULATOR),true)
	ifeq ($(HOST_OS),linux)
		LOCAL_LDLIBS += -lrt
	endif
endif

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_SHARED_LIBRARIES := \
    libui \
	libskia \
    libEGL \
    libGLESv1_CM \
    libsurfaceflinger_client

LOCAL_STATIC_LIBRARIES := \
	libcutils \
	libutils \
	libbinder

LOCAL_C_INCLUDES := \
	$(call include-path-for, corecg graphics)

LOCAL_MODULE:= bench-bootanim


include $(BUILD_EXECUTABLE)
