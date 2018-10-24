# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/plugin_use_cases_tst/inc

LOCAL_SRC_FILES := \
    ../private/tst/plugin_use_cases_tst/src/IasAlsaPluginUseCasesTest.cpp

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
NFS_PATH := /sdcard/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DPLUGHW_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DNFS_PATH=\"$(NFS_PATH)\"

LOCAL_MODULE := audio_plugin_use_cases_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

#if ( "$ENV{IAS_CONTINUOUS_INTEGRATION_BUILD}" STREQUAL "" )

#  add_dependencies( test_plugin_use_cases libasound_module_pcm_smartx )

#  add_custom_command(
#    TARGET test_plugin_use_cases
#    POST_BUILD
#    COMMAND cp ARGS $<TARGET_FILE:libasound_module_pcm_smartx> /tmp
#  )

#endif()
