/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestPluginLibrary2.cpp
 * @date   2016
 * @brief
 */

#include "audio/smartx/rtprocessingfwx/IasAudioPlugin.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
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
  IasPluginLibrary *pluginLibrary = new IasPluginLibrary("TestPluginLibrary2");
  pluginLibrary->registerFactoryMethods("limiter", testCompCreate, genericDestroy);
  pluginLibrary->registerFactoryMethods("delay", testCompCreate, genericDestroy);
  pluginLibrary->registerFactoryMethods("telephonemixer", testCompCreate, genericDestroy);
  pluginLibrary->registerFactoryMethods("downmixer", testCompCreate, genericDestroy);
  return pluginLibrary;
}

IAS_AUDIO_PUBLIC void destroy(IasModuleInterface *module)
{
  delete module;
}

} // extern "C"
