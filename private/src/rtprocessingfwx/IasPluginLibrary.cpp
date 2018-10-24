/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasPluginLibrary.cpp
 * @date   2016
 * @brief
 */

#include "audio/smartx/rtprocessingfwx/IasPluginLibrary.hpp"

#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp" ///
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

static const std::string cClassName = "IasPluginLibrary::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + ")[" + mLibraryName + "]:"

IasPluginLibrary::IasPluginLibrary(const std::string &libraryName)
  :mLibraryName(libraryName)
  ,mModuleMap()
  ,mLog(IasAudioLogging::registerDltContext("PFW", "Log of rtprocessing framework"))
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Loaded plugin library", mLibraryName);
}

IasPluginLibrary::~IasPluginLibrary()
{
  mModuleMap.clear();
}

void IasPluginLibrary::registerFactoryMethods(const std::string& typeName, IasCreateFunc create, IasDestroyFunc destroy)
{
  if (typeName.empty() == true || typeName.compare("") == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Empty plugin typeName not allowed");
    return;
  }
  if (create == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Create function for module type", typeName,"== nullptr");
    return;
  }
  if (destroy == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Destroy function for module type", typeName,"== nullptr");
    return;
  }
  if (mModuleMap.find(typeName) != mModuleMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Module type", typeName,"already registered. Please unregister first");
    return;
  }
  IasFactoryFunctions factoryFunctions;
  factoryFunctions.create = create;
  factoryFunctions.destroy = destroy;
  mModuleMap[typeName] = factoryFunctions;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module type", typeName,"successfully registered");
}

void IasPluginLibrary::unregisterFactoryMethods(const std::string& typeName)
{
  if (mModuleMap.erase(typeName) == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module type", typeName,"wasn't registered before, so nothing to unregister");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module type", typeName,"successfully unregistered");
  }
}

IasGenericAudioComp* IasPluginLibrary::createModule(const IasIGenericAudioCompConfig *config, const std::string& typeName, const std::string& instanceName)
{
  auto entry = mModuleMap.find(typeName);

  if (entry == mModuleMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module type", typeName,"not found in this library");
    return nullptr;
  }
  else
  {
    IAS_ASSERT(entry->second.create != nullptr);
    IasGenericAudioComp *newModule = entry->second.create(config, typeName, instanceName);
    IAS_ASSERT(newModule != nullptr);

    return newModule;
  }
}

IasPluginLibrary::IasResult IasPluginLibrary::destroyModule(IasGenericAudioComp* module)
{
  // module != nullptr already checked in IasPluginEngine
  IAS_ASSERT(module != nullptr);
  std::string typeName = module->getTypeName();
  std::string instanceName = module->getInstanceName();
  auto entry = mModuleMap.find(typeName);
  if (entry == mModuleMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module with name", instanceName, "of module type", typeName,"not found, so no module will be destroyed");
    return eIasFailed;
  }
  else
  {
    IAS_ASSERT(entry->second.destroy != nullptr);
    entry->second.destroy(module);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Instance with name", instanceName, "of module type", typeName,"successfully destroyed");
    return eIasOk;
  }
}

IasTypeNameList IasPluginLibrary::getModuleTypeNames() const
{
  IasTypeNameList registeredTypeNames;
  for (auto entry: mModuleMap)
  {
    registeredTypeNames.push_back(entry.first);
  }
  return registeredTypeNames;
}

} /* namespace IasAudio */
