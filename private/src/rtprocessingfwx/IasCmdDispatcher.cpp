/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCmdDispatcher.cpp
 * @date   2016
 * @brief
 */

#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

static const std::string cClassName = "IasCmdDispatcher::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasCmdDispatcher::IasCmdDispatcher()
  :mLog(IasAudioLogging::registerDltContext("PFW", "Log of rtprocessing framework"))
{
}

IasCmdDispatcher::~IasCmdDispatcher()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "deleted");
}

IasICmdRegistration::IasResult IasCmdDispatcher::registerModuleIdInterface(const std::string& instanceName, IasIModuleId* interface)
{
  if (instanceName.empty() == true || instanceName.compare("") == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Instance name may not be empty");
    return IasICmdRegistration::eIasFailed;
  }
  if (interface == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cmd interface to be registered for instance", instanceName, "== nullptr");
    return IasICmdRegistration::eIasFailed;
  }
  auto ret = mCmdMap.insert(std::make_pair(instanceName, interface));
  if (ret.second == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cmd interface for instance", instanceName, "already registered");
    return IasICmdRegistration::eIasFailed;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Cmd interface for instance", instanceName, "successfully registered");
    return IasICmdRegistration::eIasOk;
  }
}

void IasCmdDispatcher::unregisterModuleIdInterface(const std::string &instanceName)
{
  if (instanceName.empty() == true || instanceName.compare("") == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Instance name may not be empty");
    return;
  }
  auto numberErased = mCmdMap.erase(instanceName);
  if (numberErased > 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Cmd interface for instance", instanceName, "successfully unregistered");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Couldn't unregister cmd interface for instance", instanceName);
  }
}

IasICmdRegistration::IasResult translate(IasIModuleId::IasResult res)
{
  if (res == IasIModuleId::eIasOk)
  {
    return IasICmdRegistration::eIasOk;
  }
  else
  {
    return IasICmdRegistration::eIasFailed;
  }
}

IasICmdRegistration::IasResult IasCmdDispatcher::dispatchCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  auto cmdInterface = mCmdMap.find(instanceName);
  if (cmdInterface == mCmdMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cmd interface for instance", instanceName, "not registered");
    return IasICmdRegistration::eIasFailed;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Passing cmd to instance", instanceName);
    cmdProperties.dump("cmdProperties");
    returnProperties.clearAll();
    IasIModuleId::IasResult modres = cmdInterface->second->processCmd(cmdProperties, returnProperties);
    returnProperties.dump("returnProperties");
    return translate(modres);
  }
}


} /* namespace IasAudio */
