# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/model_tst/inc

LOCAL_SRC_FILES := \
    ../private/tst/model_tst/src/IasAudioModelTest.cpp


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

NULL_ALSA_DEVICE_NAME := plughw_31_0

LOCAL_REQUIRED_MODULES := \
    asound.conf \
    asound.rc

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter  \
    -DALSA_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DPLUGHW_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\"

LOCAL_MODULE := audio_model_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
