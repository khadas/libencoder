LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/bin
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif

LOCAL_SRC_FILES:= test.cpp test_dma.c IONmem.c

LOCAL_SHARED_LIBRARIES := \
        libion \
        libstagefright_foundation \
        lib_avc_vpcodec

LOCAL_C_INCLUDES := $(TOP)/frameworks/av/media/libstagefright/foundation/include \
                    $(TOP)/system/memory/libion \
                    $(TOP)/system/memory/libion/include \
                    $(TOP)/system/memory/libion/kernel-headers \
                    $(TOP)/system/memory/include/ion \
                    $(LOCAL_PATH)/bjunion_enc/include
LOCAL_CFLAGS += -Wno-multichar -Wno-unused


LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= testEncApi

LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
