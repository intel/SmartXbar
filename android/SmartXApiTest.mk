# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/smartx_api/inc

LOCAL_SRC_FILES := \
    ../private/tst/smartx_api/src/main.cpp \
    ../private/tst/smartx_api/src/IasSmartX_API_Test.cpp \
    ../private/tst/smartx_api/src/IasCustomerEventHandler.cpp \
    ../private/src/smartx/IasDebugMutexDecorator.cpp

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

SMARTX_PLUGIN_DIR := $(SYSTEM_LIBRARY_PATH)/audio/plugin
NULL_ALSA_DEVICE_NAME := plughw_31_0
SMARTX_CONFIG_DIR := /system/vendor/test/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DALSA_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DPLUGHW_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\" \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\" \
    -DSMARTX_CONFIG_DIR=\"$(SMARTX_CONFIG_DIR)\"

LOCAL_REQUIRED_MODULES := \
    asound.conf \
    asound.rc \
    rr/smartx_config.txt \
    cfs/smartx_config.txt cfs_1/smartx_config.txt \
    fifo/smartx_config.txt fifo_100/smartx_config.txt \
    inv/smartx_config.txt \
    empty_file/smartx_config.txt \
    corrupt/smartx_config.txt \
    cpu_affinities_1/smartx_config.txt

LOCAL_MODULE := audio_smartxapi_test
LOCAL_MODULE_OWNER := intel

LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

#######################################################################
# Build for SmartX API test resource file
#######################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := rr/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := cfs/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := cfs_1/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := fifo/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := fifo_100/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := inv/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := empty_file/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := corrupt/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := cpu_affinities_1/smartx_config.txt
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := ../private/tst/smartx_api/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
