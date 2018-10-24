# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/rtprocessingfwIntegration/inc \
    system/core/libutils/include \
    system/core/liblog/include

LOCAL_SRC_FILES := \
    ../private/tst/rtprocessingfwIntegration/src/IasRtProcessingFwTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasRtProcessingFwTestMain.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasAudioChainEnvironmentTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasAudioChannelBundleTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasAudioStreamTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasBundleAssignmentTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasBundleSequencerTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasTestComp.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasAudioChainTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasPropertiesTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasModuleEventTest.cpp \
    ../private/tst/rtprocessingfwIntegration/src/IasAudioBufferTest.cpp \

LOCAL_STATIC_LIBRARIES := \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-audio-smartx \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE := audio_rtprocessingfwIntegration_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
