LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SRC_FILES:= test.cpp \
                  test_dma.c
LOCAL_SHARED_LIBRARIES := libutils libcutils libion libge2d-2.0 libvp_hevc_codec_new
#LOCAL_STATIC_LIBRARIES := libge2d

LOCAL_C_INCLUDES:= \
    hevc_enc/include \
    $(TOP)/vendor/amlogic/common/system/libge2d/v2/include

LOCAL_CFLAGS += -Wno-multichar -Werror -Wall -g -Wno-sometimes-uninitialized

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := testHevcEncApi

LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
