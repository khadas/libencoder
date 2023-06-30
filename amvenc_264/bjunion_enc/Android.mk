LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif

LOCAL_CFLAGS := \
	-fPIC -D_POSIX_SOURCE -D_FILE_OFFSET_BITS=64

LOCAL_LDLIBS    := -lm -llog
LOCAL_CFLAGS += -Wno-multichar -Wno-unused -Wno-unused-parameter
LOCAL_SRC_FILES := \
    libvpcodec.cpp \
    h264bitstream.c \

LOCAL_SRC_FILES += AML_HWEncoder.cpp \
	enc_api.cpp \
	enc/gx_enc_fast/rate_control_gx_fast.cpp \
	enc/gx_enc_fast/gxvenclib_fast.cpp \
	enc/gx_enc_fast/parser.cpp \
#	enc/common/fill_buffer.cpp \

#LOCAL_SRC_FILES += enc/intra_search/pred.cpp \
#	enc/intra_search/pred_neon_asm.s

ifneq (,$(wildcard vendor/amlogic/frameworks/av/LibPlayer))
LIBPLAYER_DIR:=$(TOP)/vendor/amlogic/frameworks/av/LibPlayer
else
LIBPLAYER_DIR:=$(TOP)/packages/amlogic/LibPlayer
endif

LOCAL_SHARED_LIBRARIES  += libcutils \
	libutils  \
	libion \
	liblog


LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	$(TOP)/hardware/amlogic/gralloc \
	$(TOP)/system/core/include
	#$(LIBPLAYER_DIR)/amcodec/include

LOCAL_CFLAGS += -Wno-multichar -Wno-error

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= lib_avc_vpcodec
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_SHARED_LIBRARY)
