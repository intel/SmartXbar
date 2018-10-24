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
    libsndfile-1.0.27 \
    libdlt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libaudio_test_routingzone_plugin_library
LOCAL_MODULE_RELATIVE_PATH := audio/plugin/rztest
LOCAL_CLANG := true

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/routingzone/inc \
    system/core/libutils/include \
    system/core/liblog/include

LOCAL_SRC_FILES := \
    ../private/tst/routingzone/src/main.cpp \
    ../private/tst/routingzone/src/IasRoutingZoneTest.cpp \
    ../private/tst/routingzone/src/IasMyComplexPipeline.cpp \
    ../private/tst/routingzone/src/IasMySimplePipeline.cpp

LOCAL_STATIC_LIBRARIES := \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
     \
    libias-audio-smartx \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libias-core_libraries-test_support \
    libdlt \
    libsndfile-1.0.27 \
    libasound

ifeq ($(TARGET_IS_64_BIT),true)
SYSTEM_LIBRARY_PATH=/system/lib64
else
SYSTEM_LIBRARY_PATH=/system/lib
endif
SMARTX_PLUGIN_DIR := $(SYSTEM_LIBRARY_PATH)/audio/plugin/rztest

NFS_PATH := /sdcard/
NULL_ALSA_DEVICE_NAME := plughw_31_0_

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\" \
    -DPLUGHW_DEVICE_PREFIX_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DNFS_PATH=\"$(NFS_PATH)\"

LOCAL_MODULE := audio_routingzone_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

LOCAL_REQUIRED_MODULES := asound.conf

include $(BUILD_EXECUTABLE)
