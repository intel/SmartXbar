# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

###########################
# Audio Helper
#IasInitLibrary( audio helperx ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-helperx
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true

LOCAL_SRC_FILES := ../private/src/helper/IasRamp.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/helper

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../daemon2/public/inc

LOCAL_STATIC_LIBRARIES := \
    libdlt

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libcutils

LOCAL_CLANG := true
LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Werror -Wpointer-arith

include $(BUILD_STATIC_LIBRARY)

###########################
# Audio Filter
#IasInitLibrary( audio filterx ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-filterx
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := \
    ../private/src/filter/IasAudioFilter.cpp \
    ../private/src/filter/IasAudioFilterCallback.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/filter

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../daemon2/public/inc/

LOCAL_STATIC_LIBRARIES := \
    libias-audio-helperx \
    libdlt \
    libtbb

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libcutils

LOCAL_CLANG := true
LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Werror -Wpointer-arith

include $(BUILD_STATIC_LIBRARY)


###########################
# Audio Volume
#IasInitLibrary( audio volumex ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-volumex
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := \
    ../private/src/volume/IasVolumeLoudnessCore.cpp \
    ../private/src/volume/IasVolumeCmdInterface.cpp \
    ../private/src/volume/IasVolumeHelper.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/volume

#  IasAddHeadersApiPublic( IasVolumeCmd.hpp )
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libias-audio-helperx \
    libias-audio-filterx \
    libdlt \
    libtbb

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libcutils

LOCAL_CLANG := true

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Werror -Wpointer-arith

include $(BUILD_STATIC_LIBRARY)

###########################
# Mixer
#IasInitLibrary( audio mixerx ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-mixer
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := \
    ../private/src/mixer/IasMixerCmdInterface.cpp \
    ../private/src/mixer/IasMixerCore.cpp \
    ../private/src/mixer/IasMixerElementary.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/mixer

#IasAddHeadersApiPublic(IasMixerCmd.hpp)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libias-audio-helperx \
    libias-audio-filterx \
    libdlt \
    libtbb

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libcutils

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Werror -Wpointer-arith

include $(BUILD_STATIC_LIBRARY)

###########################
# equalizer
#IasInitLibrary( audio equalizer ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-equalizer
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := \
    ../private/src/equalizer/IasEqualizerCmdInterface.cpp \
    ../private/src/equalizer/IasEqualizerCore.cpp \
    ../private/src/equalizer/IasEqualizerConfiguration.cpp \
    ../private/src/filter/IasAudioFilterCallback.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/equalizer

#IasAddHeadersApiPublic(IasEqualizerCmd.hpp)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libias-audio-helperx \
    libias-audio-filterx \
    libdlt \
    libtbb

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libcutils

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Wno-unused-parameter

include $(BUILD_STATIC_LIBRARY)

###########################
# diagnostic
#IasInitLibrary( audio diagnostic ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-diagnostic
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers

LOCAL_SRC_FILES := \
    ../private/src/diagnostic/IasDiagnostic.cpp \
    ../private/src/diagnostic/IasDiagnosticLogWriter.cpp \
    ../private/src/diagnostic/IasDiagnosticStream.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/diagnostic

#IasAddHeadersApiPublic(IasDiagnosticCmd.hpp)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libdlt \
    libtbb

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libcutils

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Wno-unused-parameter

include $(BUILD_STATIC_LIBRARY)

###########################
# Audio Modules
###########################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-modules
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := ../private/src/module_library/IasModuleLibrary.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc

LOCAL_STATIC_LIBRARIES := \
    libias-audio-volumex \
    libias-audio-filterx \
    libias-audio-helperx \
    libias-audio-mixer \
    libias-audio-equalizer \
    libdlt \
    libtbb \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libias-audio-common \
    libias-audio-smartx

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -ffast-math -msse -msse2 -O3 -std=c++11 \
    -Werror -Wpointer-arith

LOCAL_MODULE_RELATIVE_PATH := audio/plugin

include $(BUILD_SHARED_LIBRARY)
