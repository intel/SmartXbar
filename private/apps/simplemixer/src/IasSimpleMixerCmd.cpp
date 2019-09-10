/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasSimpleMixerCmd.cpp
 * @date  2016
 * @brief The implementation of the command interface of the simple mixer module
 */

#include <math.h>
#include <stdio.h>
#include "audio/simplemixer/IasSimpleMixerCmd.hpp"
#include "audio/simplemixer/IasSimpleMixerCore.hpp"

DLT_DECLARE_CONTEXT(logCtxSimpleMixer);

namespace IasAudio {

static const std::string cClassName = "IasSimpleMixerCmd::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasSimpleMixerCmd::IasSimpleMixerCmd(const IasIGenericAudioCompConfig *config, IasSimpleMixerCore *core)
  :IasIModuleId(config)
  ,mCore(core)
  ,mCmdFuncTable()
{
  DLT_REGISTER_CONTEXT(logCtxSimpleMixer, "SM", "Simple mixer audio module");
}

IasSimpleMixerCmd::~IasSimpleMixerCmd()
{
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}

IasIModuleId::IasResult IasSimpleMixerCmd::init()
{
  const uint32_t maxSize = IasSimpleMixer::eIasLastEntry;
  mCmdFuncTable.resize(maxSize);
  mCmdFuncTable[IasSimpleMixer::eIasSetModuleState] = &IasSimpleMixerCmd::setModuleState;
  mCmdFuncTable[IasSimpleMixer::eIasSetInPlaceGain] = &IasSimpleMixerCmd::setInPlaceGain;
  mCmdFuncTable[IasSimpleMixer::eIasSetXMixerGain]  = &IasSimpleMixerCmd::setXMixerGain;

  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasSimpleMixerCmd::processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  int32_t cmdId;
  IasProperties::IasResult status = cmdProperties.get("cmd", &cmdId);
  if (status == IasProperties::eIasOk)
  {
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "Property with key \"cmd\" found, cmdId=", cmdId);
    if (static_cast<uint32_t>(cmdId) < mCmdFuncTable.size())
    {
      return mCmdFuncTable[cmdId](this, cmdProperties, returnProperties);
    }
    else
    {
      DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX, "Cmd with cmdId", cmdId, "not registered");
      return IasIModuleId::eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX, "Property with key \"cmd\" not found");
    return IasIModuleId::eIasFailed;
  }
}

IasIModuleId::IasResult IasSimpleMixerCmd::setModuleState(const IasProperties& cmdProperties,
                                                          IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  std::string moduleState;
  IasProperties::IasResult result = cmdProperties.get("moduleState", &moduleState);
  if (result == IasProperties::eIasOk)
  {
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "moduleState:", moduleState);
    if (moduleState.compare("on") == 0)
    {
      mCore->enableProcessing();  // function call is handled by IasGenericAudioCompCore
      returnProperties.set<std::string>("moduleState", "on");
    }
    else
    {
      mCore->disableProcessing(); // function call is handled by IasGenericAudioCompCore
      returnProperties.set<std::string>("moduleState", "off");
    }
    return IasIModuleId::eIasOk;
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}

IasIModuleId::IasResult IasSimpleMixerCmd::getGain(const IasProperties &cmdProperties, float &gain)
{
  int32_t gaindb;
  IasProperties::IasResult result = cmdProperties.get("gain", &gaindb);
  if (result != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "Gain [dB/10] =", gaindb);

  // Calculate the linear gain.
  if (gaindb > -1440)
  {
    gain = powf(10.0f, static_cast<float>(gaindb)/200.0f);
  }
  else
  {
    gain = 0.0f;
  }

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasSimpleMixerCmd::getStreamId(const IasProperties &cmdProperties, int32_t &streamId)
{
  std::string pinName;
  IasProperties::IasResult result = cmdProperties.get("pin", &pinName);
  if (result != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  IasAudioProcessingResult procres = mConfig->getStreamId(pinName, streamId);
  if (procres == eIasAudioProcOK)
  {
    return IasIModuleId::eIasOk;
  }
  else
  {
    // Log already done in method getStreamId
    return IasIModuleId::eIasFailed;
  }
}


IasIModuleId::IasResult IasSimpleMixerCmd::getStreamIds(const IasProperties &cmdProperties,
                                                        int32_t &inputStreamId,
                                                        int32_t &outputStreamId)
{
  std::string pinName;
  IasProperties::IasResult result = cmdProperties.get("input_pin", &pinName);
  if (result != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  IasAudioProcessingResult procres = mConfig->getStreamId(pinName, inputStreamId);
  if (procres != eIasAudioProcOK)
  {
    // Log already done in method getStreamId
    return IasIModuleId::eIasFailed;
  }

  result = cmdProperties.get("output_pin", &pinName);
  if (result != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  procres = mConfig->getStreamId(pinName, outputStreamId);
  if (procres != eIasAudioProcOK)
  {
    // Log already done in method getStreamId
    return IasIModuleId::eIasFailed;
  }

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasSimpleMixerCmd::setInPlaceGain(const IasProperties& cmdProperties,
                                                          IasProperties& returnProperties)
{
  (void)returnProperties;

  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t streamId;
  IasIModuleId::IasResult modres = getStreamId(cmdProperties, streamId);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "streamId=", streamId);

  float gain;
  modres = getGain(cmdProperties, gain);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "gain (linear) =", gain);

  mCore->setInPlaceGain(streamId, gain);
  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasSimpleMixerCmd::setXMixerGain(const IasProperties& cmdProperties,
                                                         IasProperties& returnProperties)
{
  (void)returnProperties;

  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t inputStreamId;
  int32_t outputStreamId;
  IasIModuleId::IasResult modres = getStreamIds(cmdProperties, inputStreamId, outputStreamId);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "inputStreamId =", inputStreamId, "outputStreamId =", outputStreamId);

  float gain;
  modres = getGain(cmdProperties, gain);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "gain (linear) =", gain);

  mCore->setXMixerGain(inputStreamId, outputStreamId, gain);
  return IasIModuleId::eIasOk;
}

} // namespace IasAudio
