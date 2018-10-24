/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestComp.cpp
 * @date   Sept 24, 2012
 * @brief
 */


#include <iostream>
// This ../../ is just used to allow Eclipse to find the correct header and not
// accidentally using the one from the rtprocessingfwIntegration tst for doing
// syntax checks
#include "../../plugins/inc/IasTestComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

DLT_IMPORT_CONTEXT(tstCtx);

namespace IasAudio {

#undef LOG_PREFIX
static const std::string cClassName1 = "IasTestCompCore::";
#define LOG_PREFIX cClassName1 + __func__ + "(" + std::to_string(__LINE__) + "):"

IasTestCompCore::IasTestCompCore(const IasIGenericAudioCompConfig *config, const std::string &typeName)
  :IasGenericAudioCompCore(config, typeName)
{
  DLT_REGISTER_CONTEXT(tstCtx, "TST", "IasPluginsTest context");
}

IasTestCompCore::~IasTestCompCore()
{
}

IasAudioProcessingResult IasTestCompCore::reset()
{
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasTestCompCore::init()
{
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasTestCompCore::processChild()
{
  return eIasAudioProcOK;
}

IasIModuleId::IasResult translate(IasProperties::IasResult propres)
{
  if (propres == IasProperties::eIasOk)
  {
    return IasIModuleId::eIasOk;
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}

#undef LOG_PREFIX
static const std::string cClassName2 = "IasTestCompCmd::";
#define LOG_PREFIX cClassName2 + __func__ + "(" + std::to_string(__LINE__) + "):"

IasIModuleId::IasResult IasTestCompCmd::init()
{
  mCmdFuncTable.push_back(&IasTestCompCmd::setVolume);
  mCmdFuncTable.push_back(&IasTestCompCmd::getVolume);
  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasTestCompCmd::processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  int32_t cmdId;
  IasProperties::IasResult status = cmdProperties.get("cmd", &cmdId);
  if (status == IasProperties::eIasOk)
  {
    DLT_LOG_CXX(tstCtx, DLT_LOG_INFO, LOG_PREFIX, "Property with key \"cmd\" found, cmdId=", cmdId);
    if (static_cast<uint32_t>(cmdId) < mCmdFuncTable.size())
    {
      return mCmdFuncTable[cmdId](this, cmdProperties, returnProperties);
    }
    else
    {
      DLT_LOG_CXX(tstCtx, DLT_LOG_ERROR, LOG_PREFIX, "Cmd with cmdId", cmdId, "not registered");
      return IasIModuleId::eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(tstCtx, DLT_LOG_ERROR, LOG_PREFIX, "Property with key \"cmd\" not found");
    return IasIModuleId::eIasFailed;
  }
}

IasIModuleId::IasResult IasTestCompCmd::setVolume(const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  float newVolume = 0.0f;
  IasProperties::IasResult res = cmdProperties.get("volume", &newVolume);
  if (res != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(tstCtx, DLT_LOG_ERROR, LOG_PREFIX, "Key \"volume\" not found");
    return IasIModuleId::eIasFailed;
  }
  DLT_LOG_CXX(tstCtx, DLT_LOG_INFO, LOG_PREFIX, "Got request to set volume to", newVolume);
  returnProperties.set("volume", newVolume);
  mCurrentVolume = newVolume;
  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasTestCompCmd::getVolume(const IasProperties &, IasProperties &returnProperties)
{
  DLT_LOG_CXX(tstCtx, DLT_LOG_INFO, LOG_PREFIX, "Got request to get volume. Current volume=", mCurrentVolume);
  returnProperties.set("volume", mCurrentVolume);
  return IasIModuleId::eIasOk;
}


}//end namespace IasAudio
