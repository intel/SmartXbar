# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/filterTest/inc

LOCAL_SRC_FILES := \
    ../private/tst/filterTest/src/main.cpp

LOCAL_STATIC_LIBRARIES := \
    libias-audio-helperx \
    libias-audio-filterx \
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
    libasound \
    liblog

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
  -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE := audio_filter_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
