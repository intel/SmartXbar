/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasMyPluginLibrary.cpp
 * @date   2016
 * @brief
 */

#include "audio/smartx/rtprocessingfwx/IasAudioPlugin.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

#include "audio/simplevolume/IasSimpleVolumeCmd.hpp"
#include "audio/simplevolume/IasSimpleVolumeCore.hpp"

#include "audio/simplemixer/IasSimpleMixerCmd.hpp"
#include "audio/simplemixer/IasSimpleMixerCore.hpp"

using namespace IasAudio;

IasAudio::IasGenericAudioComp* simpleVolumeCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasSimpleVolumeCore, IasSimpleVolumeCmd>(config, typeName, instanceName);
}
IasAudio::IasGenericAudioComp* simpleMixerCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasSimpleMixerCore, IasSimpleMixerCmd>(config, typeName, instanceName);
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
  IasPluginLibrary *pluginLibrary = new IasPluginLibrary("MyPluginLibrary");
  pluginLibrary->registerFactoryMethods("simplevolume", simpleVolumeCreate, genericDestroy);
  pluginLibrary->registerFactoryMethods("simplemixer",  simpleMixerCreate,  genericDestroy);
  return pluginLibrary;
}

IAS_AUDIO_PUBLIC void destroy(IasModuleInterface *module)
{
  delete module;
}

} // extern "C"
