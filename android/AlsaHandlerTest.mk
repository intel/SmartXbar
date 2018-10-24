# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/alsahandler/inc \
    system/core/libutils/include \
    system/core/liblog/include

LOCAL_SRC_FILES := \
    ../private/tst/alsahandler/src/IasAlsaHandlerTestMain.cpp \
    ../private/tst/alsahandler/src/IasAlsaHandlerTest.cpp \
    ../private/tst/alsahandler/src/IasAlsaHandlerTestPlayback.cpp \
    ../private/tst/alsahandler/src/IasAlsaHandlerTestPlaybackStartStop.cpp \
    ../private/tst/alsahandler/src/IasAlsaHandlerTestPlaybackAsync.cpp \
    ../private/tst/alsahandler/src/IasAlsaHandlerTestCapture.cpp \
    ../private/tst/alsahandler/src/IasAlsaHandlerTestCaptureAsync.cpp \
    ../private/tst/alsahandler/src/IasDrift.cpp \


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
    libasound \
    liblog

NULL_ALSA_DEVICE_NAME := plughw_31_0
DRIFT_FILE := /etc/drift.txt
RW_PATH := /sdcard/

LOCAL_REQUIRED_MODULES := \
    asound.conf \
    asound.rc \
    drift.txt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DALSA_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DPLUGHW_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\" \
    -DDRIFT_FILE=\"$(DRIFT_FILE)\" \
    -DRW_PATH=\"$(RW_PATH)\"

LOCAL_MODULE := audio_alsahandler_unit_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

#######################################################################
# Build for alsa handler test resource file
#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := drift.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/alsahandler/resources/$(LOCAL_MODULE)

LOCAL_CLANG := true
include $(BUILD_PREBUILT)

