/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
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
#include "IasSimpleVolumeCmd.hpp"
#include "IasSimpleVolumeCore.hpp"

using namespace IasAudio;

IasGenericAudioComp* simpleVolumeCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasSimpleVolumeCore, IasSimpleVolumeCmd>(config, typeName, instanceName);
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
  return pluginLibrary;
}

IAS_AUDIO_PUBLIC void destroy(IasModuleInterface *module)
{
  delete module;
}

} // extern "C"
