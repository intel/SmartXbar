# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/volumeTest/inc

LOCAL_SRC_FILES := \
    ../private/tst/volumeTest/src/volumeTestMain.cpp \
    ../private/tst/volumeTest/src/IasVolumeTest.cpp

LOCAL_STATIC_LIBRARIES := \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-audio-smartx \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

LOCAL_CLANG := true

ifeq ($(TARGET_IS_64_BIT),true)
SYSTEM_LIBRARY_PATH=/system/lib64
else
SYSTEM_LIBRARY_PATH=/system/lib
endif
SMARTX_PLUGIN_DIR := $(SYSTEM_LIBRARY_PATH)/audio/plugin

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\"

LOCAL_MODULE := audio_volumeTest_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
