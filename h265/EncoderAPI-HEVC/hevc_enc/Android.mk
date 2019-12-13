LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS    := -lm -llog
LOCAL_SRC_FILES := \
    libvp_hevc_codec.cpp \
    AML_HEVCEncoder.cpp \
    vdi.cpp


#define MAKEANDROID

LOCAL_CFLAGS = -Wno-multichar -Wno-unused
LOCAL_SHARED_LIBRARIES  += libcutils libutils
LOCAL_STATIC_LIBRARIES  += libge2d

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	$(LOCAL_PATH)/libge2d

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libvp_hevc_codec
LOCAL_MODULE_TAGS := optional
#LOCAL_PRELINK_MODULE := false
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))