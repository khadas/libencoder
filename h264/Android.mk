LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

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

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
