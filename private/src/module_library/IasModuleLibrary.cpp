/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasModuleLibrary.cpp
 * @date   2016
 * @brief
 */

#include "audio/smartx/rtprocessingfwx/IasAudioPlugin.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "volume/IasVolumeCmdInterface.hpp"
#include "mixer/IasMixerCore.hpp"
#include "mixer/IasMixerCmdInterface.hpp"
#include "equalizer/IasEqualizerCore.hpp"
#include "equalizer/IasEqualizerCmdInterface.hpp"

using namespace IasAudio;

IasGenericAudioComp* volumeCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasVolumeLoudnessCore, IasVolumeCmdInterface>(config, typeName, instanceName);
}

IasGenericAudioComp* mixerCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasMixerCore, IasMixerCmdInterface>(config, typeName, instanceName);
}

IasGenericAudioComp* equalizerCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasEqualizerCore, IasEqualizerCmdInterface>(config, typeName, instanceName);
}

void genericDestroy(IasGenericAudioComp *module)
{
  delete module;
}


extern "C" {

IAS_AUDIO_PUBLIC IasModuleInfo getModuleInfo()
{
  return IasModuleInfo("smartx-audio-modules");
}

IAS_AUDIO_PUBLIC IasModuleInterface* create()
{
  IasPluginLibrary *pluginLibrary = new IasPluginLibrary("IAS Audio Module Library");
  pluginLibrary->registerFactoryMethods("ias.mixer",     mixerCreate,     genericDestroy);
  pluginLibrary->registerFactoryMethods("ias.volume",    volumeCreate,    genericDestroy);
  pluginLibrary->registerFactoryMethods("ias.equalizer", equalizerCreate, genericDestroy);

  return pluginLibrary;
}

IAS_AUDIO_PUBLIC void destroy(IasModuleInterface *module)
{
  delete module;
}

} // extern "C"
