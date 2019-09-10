/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasPluginEngine.cpp
 * @date   2013
 * @brief
 */

#include <audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp>
#include <stdlib.h>
#include <sstream>
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "audio/smartx/rtprocessingfwx/IasPluginLibrary.hpp"

#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "audio/smartx/rtprocessingfwx/IasICmdRegistration.hpp"

namespace IasAudio {

static const std::string cClassName = "IasPluginEngine::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

static const char *cIasPluginDirEnv = "AUDIO_PLUGIN_DIR";
static const char *cIasModuleInfo = "smartx-audio-modules";

IasPluginEngine::IasPluginEngine(IasICmdRegistrationPtr cmdRegistration)
  :mModules()
  ,mCompConfigs()
  ,mModuleLoader()
  ,mLogContext(IasAudioLogging::getDltContext("PFW"))
  ,mCmdRegistration(cmdRegistration)
{
}

IasPluginEngine::~IasPluginEngine()
{
  for (auto module: mModules)
  {
    mModuleLoader.unloadModule(module);
  }
  mModules.clear();
  for (auto nodeConfig: mCompConfigs)
  {
    delete nodeConfig;
  }
  mCompConfigs.clear();
}

IasAudioProcessingResult IasPluginEngine::loadPluginLibraries()
{
  std::string pluginDir;
  const char *pluginDirEnv = getenv(cIasPluginDirEnv);
  if (pluginDirEnv == NULL)
  {
#if __GNUC__
	#if __x86_64__
		pluginDir = "/usr/lib64/smartx-plugin/";
	#else
		pluginDir = "/usr/lib/smartx-plugin/";
	#endif
#endif
  }
  else
  {
    pluginDir = pluginDirEnv;
    // Ensure that the plug-in dir ends with a "/".
    if (pluginDir.at(pluginDir.length() - 1) != '/')
    {
      pluginDir = pluginDir + "/";
    }
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Trying to load plugins from directory", pluginDir);
  IasModuleInfo moduleInfo = IasModuleInfo(cIasModuleInfo);
  mModules = mModuleLoader.loadModules(pluginDir, "create", "destroy", moduleInfo);

  if (mModules.size() > 0)
  {
    for (auto entry: mModules)
    {
      IasPluginLibrary *pluginLib = reinterpret_cast<IasPluginLibrary*>(entry);
      if (pluginLib->getModuleTypeNames().size() > 0)
      {
        mPluginLibs.push_back(pluginLib);
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Loaded audio-plugin library", pluginLib->getLibraryName(), "containing the following audio module types:");
        for (auto moduleType: pluginLib->getModuleTypeNames())
        {
          DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Module Type=", moduleType);
        }
      }
      else
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_WARN, LOG_PREFIX, "Ignoring audio-plugin library", pluginLib->getLibraryName(), "because it contains no valid audio module types");
      }
    }
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Error loading the plugin libraries from folder", pluginDir);
    return eIasAudioProcInvalidParam;
  }

  return eIasAudioProcOK;
}

IasAudioProcessingResult IasPluginEngine::createModuleConfig(IasIGenericAudioCompConfig **config)
{
  if (config == NULL)
  {
    return eIasAudioProcInvalidParam;
  }
  IasIGenericAudioCompConfig *newConfig = new IasGenericAudioCompConfig();
  if (newConfig != NULL)
  {
    *config = newConfig;
    mCompConfigs.push_back(newConfig);
    return eIasAudioProcOK;
  }
  else
  {
    return eIasAudioProcNotEnoughMemory;
  }
}

IasAudioProcessingResult IasPluginEngine::createModule(IasIGenericAudioCompConfig *config,
                                                       const std::string& typeName,
                                                       const std::string& instanceName,
                                                       IasGenericAudioComp** module)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "module==nullptr for module", instanceName, "of type", typeName);
    return eIasAudioProcInvalidParam;
  }

  if (config == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "config==nullptr for module", instanceName, "of type", typeName);
    return eIasAudioProcInvalidParam;
  }

  // Get the configuration properties and add typeName & instanceName to them.
  IasProperties properties = config->getProperties();
  properties.set<std::string>("typeName"    , typeName);
  properties.set<std::string>("instanceName", instanceName);
  config->setProperties(properties);

  IasGenericAudioComp *newModule = nullptr;
  for (auto lib: mPluginLibs)
  {
    newModule = lib->createModule(config, typeName, instanceName);
    if (newModule != nullptr)
    {
      break;
    }
  }
  if (newModule != nullptr)
  {
    IasICmdRegistration::IasResult regres = mCmdRegistration->registerModuleIdInterface(instanceName, newModule->getCmdInterface());
    if (regres == IasICmdRegistration::eIasFailed)
    {
      destroyModule(newModule);
      return eIasAudioProcInitializationFailed;
    }
    *module = newModule;
    return eIasAudioProcOK;
  }
  else
  {
    return eIasAudioProcInvalidParam;
  }
}

void IasPluginEngine::destroyModule(IasGenericAudioComp *module)
{
  if (module != nullptr)
  {
    mCmdRegistration->unregisterModuleIdInterface(module->getInstanceName());
    for (auto lib: mPluginLibs)
    {
      IasPluginLibrary::IasResult res = lib->destroyModule(module);
      if (res == IasPluginLibrary::eIasOk)
      {
        // After we deleted the module we have to exit the loop else we will provide an invalid pointer
        // to an already deleted module to antother IasPluginLibrary instance
        break;
      }
    }
  }
}


} // namespace IasAudio
