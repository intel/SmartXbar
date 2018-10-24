# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/pipeline/inc \
    system/core/libutils/include \
    system/core/liblog/include

LOCAL_SRC_FILES := \
    ../private/tst/pipeline/src/IasSimpleVolumeCmd.cpp \
    ../private/tst/pipeline/src/IasSimpleVolumeCore.cpp \
    ../private/tst/pipeline/src/IasMyPluginLibrary.cpp \

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

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := ias-audio-test_pipeline_plugin_library
LOCAL_CLANG := true

LOCAL_MODULE_RELATIVE_PATH := audio/plugin/pipelinetest

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/pipeline/inc \
    system/core/libutils/include \
    system/core/liblog/include

LOCAL_SRC_FILES := \
    ../private/tst/pipeline/src/IasPipelineTestMain.cpp \
    ../private/tst/pipeline/src/IasPipelineTest.cpp

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

ifeq ($(TARGET_IS_64_BIT),true)
SYSTEM_LIBRARY_PATH=/system/lib64
else
SYSTEM_LIBRARY_PATH=/system/lib
endif
SMARTX_PLUGIN_DIR := $(SYSTEM_LIBRARY_PATH)/audio/plugin/pipelinetest

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\"

LOCAL_MODULE := audio_pipeline_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
