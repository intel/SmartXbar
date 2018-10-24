# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

###########################
# IasInitLibrary( audio smartx ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} NO_WARNING_AS_ERROR )
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-smartx
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_REQUIRED_MODULES := \
    libasound_module_pcm_smartx \
    smartx_config.txt

LOCAL_SRC_FILES := \
    ../private/src/smartx/IasSmartX.cpp \
    ../private/src/smartx/IasSmartXPriv.cpp \
    ../private/src/smartx/IasRoutingImpl.cpp \
    ../private/src/smartx/IasSetupImpl.cpp \
    ../private/src/smartx/IasProcessingImpl.cpp \
    ../private/src/smartx/IasDebugImpl.cpp \
    ../private/src/smartx/IasSetupHelper.cpp \
    ../private/src/smartx/IasConfiguration.cpp \
    ../private/src/smartx/IasSmartXClient.cpp \
    ../private/src/smartx/IasEvent.cpp \
    ../private/src/smartx/IasConnectionEvent.cpp \
    ../private/src/smartx/IasSetupEvent.cpp \
    ../private/src/smartx/IasEventHandler.cpp \
    ../private/src/smartx/IasToString.cpp \
    ../private/src/smartx/IasEventProvider.cpp \
    ../private/src/smartx/IasConfigFile.cpp \
    ../private/src/smartx/IasThreadNames.cpp \
    ../private/src/smartx/IasProperties.cpp \

LOCAL_SRC_FILES += \
    ../private/src/alsahandler/IasAlsaHandler.cpp \
    ../private/src/alsahandler/IasAlsaHandlerWorkerThread.cpp \

LOCAL_SRC_FILES += \
    ../private/src/model/IasAudioPort.cpp \
    ../private/src/model/IasAudioDevice.cpp \
    ../private/src/model/IasAudioSinkDevice.cpp \
    ../private/src/model/IasAudioSourceDevice.cpp \
    ../private/src/model/IasRoutingZone.cpp \
    ../private/src/model/IasRoutingZoneWorkerThread.cpp \
    ../private/src/model/IasAudioPin.cpp \
    ../private/src/model/IasPipeline.cpp \
    ../private/src/model/IasProcessingModule.cpp \

LOCAL_SRC_FILES += \
    ../private/src/switchmatrix/IasSwitchMatrix.cpp \
    ../private/src/switchmatrix/IasBufferTask.cpp \
    ../private/src/switchmatrix/IasSwitchMatrixJob.cpp \

LOCAL_SRC_FILES += \
    ../private/src/rtprocessingfwx/IasAudioChannelBundle.cpp \
    ../private/src/rtprocessingfwx/IasGenericAudioComp.cpp \
    ../private/src/rtprocessingfwx/IasGenericAudioCompCore.cpp \
    ../private/src/rtprocessingfwx/IasBaseAudioStream.cpp \
    ../private/src/rtprocessingfwx/IasAudioStream.cpp \
    ../private/src/rtprocessingfwx/IasBundleAssignment.cpp \
    ../private/src/rtprocessingfwx/IasBundledAudioStream.cpp \
    ../private/src/rtprocessingfwx/IasAudioChain.cpp \
    ../private/src/rtprocessingfwx/IasAudioChainEnvironment.cpp \
    ../private/src/rtprocessingfwx/IasGenericAudioCompConfig.cpp \
    ../private/src/rtprocessingfwx/IasBundleSequencer.cpp \
    ../private/src/rtprocessingfwx/IasStreamParams.cpp \
    ../private/src/rtprocessingfwx/IasSimpleAudioStream.cpp \
    ../private/src/rtprocessingfwx/IasPluginEngine.cpp \
    ../private/src/rtprocessingfwx/IasAudioBuffer.cpp \
    ../private/src/rtprocessingfwx/IasAudioBufferPool.cpp \
    ../private/src/rtprocessingfwx/IasAudioBufferPoolHandler.cpp \
    ../private/src/rtprocessingfwx/IasPluginLibrary.cpp \
    ../private/src/rtprocessingfwx/IasCmdDispatcher.cpp \
    ../private/src/rtprocessingfwx/IasProcessingTypes.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    $(LOCAL_PATH)/../private/inc/alsahandler \
    $(LOCAL_PATH)/../private/inc/filter \
    $(LOCAL_PATH)/../private/inc/helper \
    $(LOCAL_PATH)/../private/inc/model \
    $(LOCAL_PATH)/../private/inc/rtprocessingfwx \
    $(LOCAL_PATH)/../private/inc/smartx \
    $(LOCAL_PATH)/../private/inc/switchmatrix \
    $(LOCAL_PATH)/../private/inc/volume \
    bionic

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libtbb \
    libboost \
    libias-audio-diagnostic \
    libboost_program_options

LOCAL_SHARED_LIBRARIES := \
    libias-android-pthread \
    libdlt \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libsndfile-1.0.27 \
    libasound \
    liblog

ifeq ("$(shell [ $(ANDROID_VERSION) -eq $(ANDROID_M) ] && echo true)", "true")
RW_TMP_PATH=/mnt/eavb/misc/media/
else
RW_TMP_PATH=/mnt/eavb/misc/audioserver/
endif

LOCAL_CLANG := true

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -frtti -fexceptions \
    -Werror -Wpointer-arith \
    -ffast-math -msse -msse2 -O3 \
    -DSMARTX_CFG_DIR=\"/vendor/etc/\" \
    -DRW_TMP_PATH=\"$(RW_TMP_PATH)\"

include $(BUILD_SHARED_LIBRARY)

#######################################################################
# Target to export model private header required by privilegied client of SmartX
#######################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-smartx-model
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_CLANG := true

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/../private/inc/ \
    $(LOCAL_PATH)/../private/inc/smartx

include $(BUILD_STATIC_LIBRARY)

#######################################################################
# Build for configuration file
#  IasAddConfigurationFiles( "public/res" "etc" smartx_config.txt IS_ABSOLUTE_DESTINATION )
#######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := smartx_config.txt
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := res/$(LOCAL_MODULE)

include $(BUILD_PREBUILT)
