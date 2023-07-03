# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/bin
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif
LOCAL_SRC_FILES := test.c test_dma.c IONmem.c

LOCAL_SHARED_LIBRARIES := libion \
    lib_amvenc

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \


LOCAL_CFLAGS := -Wall -Wno-unused-variable -Wno-sign-compare
LOCAL_CFLAGS += -Wno-multichar -Wno-sometimes-uninitialized -Wno-format

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= amv_enc_test
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_EXECUTABLE)
