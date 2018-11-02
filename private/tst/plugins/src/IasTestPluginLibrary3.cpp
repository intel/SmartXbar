/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestPluginLibrary3.cpp
 * @date   2016
 * @brief
 */

#include "audio/smartx/rtprocessingfwx/IasAudioPlugin.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "IasTestComp.hpp"

using namespace IasAudio;

IasAudio::IasGenericAudioComp* testCompCreate(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
{
  return new IasModule<IasTestCompCore, IasTestCompCmd>(config, typeName, instanceName);
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
  IasPluginLibrary *pluginLibrary = new IasPluginLibrary("TestPluginLibrary3");
  std::string emptyString;
  pluginLibrary->registerFactoryMethods(emptyString, nullptr, nullptr);
  pluginLibrary->registerFactoryMethods("", nullptr, nullptr);
  pluginLibrary->registerFactoryMethods("test", nullptr, nullptr);
  pluginLibrary->registerFactoryMethods("test", testCompCreate, nullptr);
  pluginLibrary->registerFactoryMethods("test", testCompCreate, genericDestroy);
  pluginLibrary->registerFactoryMethods("test", testCompCreate, genericDestroy);
  return pluginLibrary;
}

IAS_AUDIO_PUBLIC void destroy(IasModuleInterface *module)
{
  delete module;
}

} // extern "C"
