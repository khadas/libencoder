LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_LDLIBS    := -lm -llog
LOCAL_SRC_FILES := \
    libvp_hevc_codec.cpp \
    AML_HEVCEncoder.cpp \
    vpuapi/common.c \
    vpuapi/product.c \
    vpuapi/vdi.c \
    vpuapi/vdi_osal.c \
    vpuapi/vpuapi.c \
    vpuapi/vpuapifunc.c \
    vpuapi/wave4.c \
    h265bitstream.c

#define MAKEANDROID

LOCAL_CFLAGS = -Wno-multichar -Wno-unused
LOCAL_SHARED_LIBRARIES  += libcutils libutils libge2d-2.0

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/vpuapi \
    $(LOCAL_PATH)/vpuapi/include \
    $(TOP)/vendor/amlogic/common/system/libge2d/v2/include

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libvp_hevc_codec_new
LOCAL_MODULE_TAGS := optional
#LOCAL_PRELINK_MODULE := false
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
