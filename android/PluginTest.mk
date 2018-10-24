# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

###########################################
# Common variable
###########################################
plugins_local_c_includes := \
    external/v8/testing/gtest/include \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/tst/plugins/inc \
    system/core/libutils/include \
    system/core/liblog/include

plugins_static_libraries := \
    libtbb \
    libboost

plugins_shared_libraries := \
    libias-audio-smartx \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

###########################################
# TEST PLUGIN 1
###########################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(plugins_local_c_includes)

LOCAL_SRC_FILES := \
    ../private/tst/plugins/src/IasTestComp.cpp \
    ../private/tst/plugins/src/IasTestPluginLibrary1.cpp

LOCAL_STATIC_LIBRARIES := $(plugins_static_libraries)
LOCAL_SHARED_LIBRARIES := $(plugins_shared_libraries)

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE_OWNER := intel
LOCAL_MODULE := ias-audio-test_plugin_library
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := audio/plugin/plugintest
LOCAL_CLANG := true

include $(BUILD_SHARED_LIBRARY)

###########################################
# TEST PLUGIN 2
###########################################
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(plugins_local_c_includes)

LOCAL_SRC_FILES := \
    ../private/tst/plugins/src/IasTestComp.cpp \
    ../private/tst/plugins/src/IasTestPluginLibrary2.cpp

LOCAL_STATIC_LIBRARIES := $(plugins_static_libraries)
LOCAL_SHARED_LIBRARIES := $(plugins_shared_libraries)

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE_OWNER := intel
LOCAL_MODULE := ias-audio-test_plugin_library2
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := audio/plugin/plugintest
LOCAL_CLANG := true

include $(BUILD_SHARED_LIBRARY)

###########################################
# TEST PLUGIN 3
###########################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(plugins_local_c_includes)

LOCAL_SRC_FILES := \
    ../private/tst/plugins/src/IasTestComp.cpp \
    ../private/tst/plugins/src/IasTestPluginLibrary3.cpp

LOCAL_STATIC_LIBRARIES := $(plugins_static_libraries)
LOCAL_SHARED_LIBRARIES := $(plugins_shared_libraries)

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_MODULE_OWNER := intel
LOCAL_MODULE := ias-audio-test_plugin_library3
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := audio/plugin/plugintest
LOCAL_CLANG := true

include $(BUILD_SHARED_LIBRARY)

###########################################
# TEST APP
###########################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(plugins_local_c_includes)

LOCAL_SRC_FILES := \
    ../private/tst/plugins/src/IasPluginsTest.cpp

LOCAL_STATIC_LIBRARIES := $(plugins_static_libraries)

LOCAL_SHARED_LIBRARIES := \
    $(plugins_shared_libraries)

ifeq ($(TARGET_IS_64_BIT),true)
SYSTEM_LIBRARY_PATH=/system/lib64
else
SYSTEM_LIBRARY_PATH=/system/lib
endif
SMARTX_PLUGIN_DIR := $(SYSTEM_LIBRARY_PATH)/audio/plugin/plugintest

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wno-unused-parameter -frtti -fexceptions \
    -DAUDIO_PLUGIN_DIR=\"$(SMARTX_PLUGIN_DIR)\"

LOCAL_MODULE := audio_plugins_unit_test
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true

LOCAL_REQUIRED_MODULES := \
    ias-audio-test_plugin_library \
    ias-audio-test_plugin_library2 \
    ias-audio-test_plugin_library3

include $(BUILD_EXECUTABLE)
