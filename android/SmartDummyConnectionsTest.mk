# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/smartx_dummyConnections/inc

LOCAL_SRC_FILES := \
    ../private/tst/smartx_dummyConnections/src/main.cpp \
    ../private/tst/smartx_dummyConnections/src/IasDummyConnectionTest.cpp

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

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
  -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions

LOCAL_MODULE := audio_smartx_dummyConnections_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
