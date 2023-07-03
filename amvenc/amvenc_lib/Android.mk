LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif

LOCAL_CFLAGS := \
    -fPIC -D_POSIX_SOURCE -D_FILE_OFFSET_BITS=64

LOCAL_LDLIBS := -lm -llog

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/include \

LOCAL_SRC_FILES := \
    amvenc.c  \
    amvenc_common.c \
    amvenc_multi.c \
    amvenc_264.c \
    amvenc_265.c \
    amvenc_vers.c \

LOCAL_ARM_MODE := arm

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_CFLAGS=-Wno-error
LOCAL_MODULE := lib_amvenc
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_SHARED_LIBRARY)
