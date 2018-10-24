# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/switchmatrix/inc

LOCAL_SRC_FILES := \
    ../private/tst/switchmatrix/src/main.cpp \
    ../private/tst/switchmatrix/src/IasSwitchMatrixTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-audio-smartx_test_support \
    libias-audio-smartx \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

NFS_PATH := /sdcard/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DNFS_PATH=\"$(NFS_PATH)\"

LOCAL_MODULE := audio_switchmatrix_test
LOCAL_MODULE_OWNER := intel
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

#add_custom_command(
#  TARGET test_switchmatrix
#  POST_BUILD
#  COMMAND cp ARGS /nfs/ka/proj/ias/organisation/teams/audio/TestWavRefFiles/2016-02-18/test_ch0.wav /tmp
#)
#add_custom_command(
#  TARGET test_switchmatrix
#  POST_BUILD
#  COMMAND cp ARGS /nfs/ka/proj/ias/organisation/teams/audio/TestWavRefFiles/2016-02-18/test_ch1.wav /tmp
#)
