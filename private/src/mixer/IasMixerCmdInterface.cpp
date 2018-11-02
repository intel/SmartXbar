/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasMixerCmdInterface.cpp
 * @date August 2016
 * @brief implementation of the mixer command interface
 */

#include <cstdio>
#include <cmath>

#include "mixer/IasMixerCmdInterface.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/mixerx/IasMixerCmd.hpp"
#include "mixer/IasMixerCore.hpp"

namespace IasAudio {


static const std::string cClassName = "IasMixerCmdInterface::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

static const int32_t cMinInputGainOffset = -200; //minimum allowed input gain
static const int32_t cMaxInputGainOffset =  200; //maximum allowed input gain
static const int32_t cCutOffValue = 1440; //cut off value for balance and fader

IasMixerCmdInterface::IasMixerCmdInterface(const IasIGenericAudioCompConfig* config, IasMixerCore* core) :
  IasIModuleId{config}
  ,mCore{core}
  ,mCmdFuncTable{}
  ,mLogContext{IasAudioLogging::getDltContext("_MIX")}
{
}

IasMixerCmdInterface::~IasMixerCmdInterface()
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}


IasIModuleId::IasResult IasMixerCmdInterface::init()
{
  const std::size_t maxSize = IasMixer::IasMixerCmdIds::eIasLastEntry;
  mCmdFuncTable.resize(maxSize);
  mCmdFuncTable[IasMixer::IasMixerCmdIds::eIasSetModuleState]     = &IasMixerCmdInterface::setModuleState;
  mCmdFuncTable[IasMixer::IasMixerCmdIds::eIasSetInputGainOffset] = &IasMixerCmdInterface::setInputGainOffset;
  mCmdFuncTable[IasMixer::IasMixerCmdIds::eIasSetBalance]         = &IasMixerCmdInterface::setBalance;
  mCmdFuncTable[IasMixer::IasMixerCmdIds::eIasSetFader]           = &IasMixerCmdInterface::setFader;

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasMixerCmdInterface::processCmd(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  int32_t cmdId;
  IasProperties::IasResult status = cmdProperties.get("cmd", &cmdId);
  if (status == IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Property with key \"cmd\" found, cmdId=", cmdId);
    if (static_cast<uint32_t>(cmdId) < mCmdFuncTable.size())
    {
      return mCmdFuncTable[cmdId](this, cmdProperties, returnProperties);
    }
    else
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Cmd with cmdId", cmdId, "not registered");
      return IasIModuleId::eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Property with key \"cmd\" not found");
    return IasIModuleId::eIasFailed;
  }
}


IasIModuleId::IasResult IasMixerCmdInterface::setModuleState(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  std::string moduleState;
  IasProperties::IasResult result = cmdProperties.get("moduleState", &moduleState);

  if (result == IasProperties::eIasOk)
  {
    if (moduleState.compare("on") == 0)
    {
      mCore->enableProcessing();
      returnProperties.set<std::string>("moduleState", "on");
    }
    else
    {
      mCore->disableProcessing();
      returnProperties.set<std::string>("moduleState", "off");
    }
    return IasIModuleId::eIasOk;
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}


IasIModuleId::IasResult IasMixerCmdInterface::getStreamId(const IasProperties& cmdProperties, int32_t& streamId)
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

IasIModuleId::IasResult IasMixerCmdInterface::translate(IasAudioProcessingResult result)
{
  if (result == eIasAudioProcOK)
  {
    return IasIModuleId::eIasOk;
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}

IasIModuleId::IasResult IasMixerCmdInterface::setBalance(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  int32_t balance;

  const auto propres = cmdProperties.get("balance", &balance);
  if(propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  float balanceRight;
  float balanceLeft;

  const float balanceLog = 0.1f * static_cast<float>(balance);

  if(balance < 0)
  {
    if(balance <= -cCutOffValue)
    {
      balanceRight = 0.0f;
    }
    else
    {
      balanceRight = std::pow(10.0f, 0.05f * balanceLog);
    }
    balanceLeft = 1.0f;
  }

  else
  {
    if(balance >= cCutOffValue)
    {
      balanceLeft = 0.0f;
    }
    else
    {
      balanceLeft = std::pow(10.0f, -0.05f * balanceLog);
    }
    balanceRight = 1.0f;
  }

  return translate(mCore->setBalance(streamId, balanceLeft, balanceRight));
}


IasIModuleId::IasResult IasMixerCmdInterface::setFader(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  int32_t fader;

  const auto propres = cmdProperties.get("fader", &fader);
  if(propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  float faderFront;
  float faderRear;

  const float faderLog = 0.1f * static_cast<float>(fader);

  if(fader < 0)
  {
    if(fader <= -cCutOffValue)
    {
      faderFront = 0.0f;
    }
    else
    {
      faderFront = std::pow(10.0f, 0.05f * faderLog);
    }
    faderRear  = 1.0f;
  }

  else
  {
    if(fader >= cCutOffValue)
    {
      faderRear = 0.0f;
    }
    else
    {
      faderRear  = std::pow(10.0f, -0.05f * faderLog);
    }
    faderFront = 1.0f;
  }

  return translate(mCore->setFader(streamId, faderFront, faderRear));
}


IasIModuleId::IasResult IasMixerCmdInterface::setInputGainOffset(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId;
  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  int32_t gain;
  const auto propres = cmdProperties.get("gain", &gain);
  if(gain > cMaxInputGainOffset || gain < cMinInputGainOffset)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Error, parameter InputGainOffset was set to",gain,", which is out of valid range");
    return IasIModuleId::eIasFailed;
  }

  if(propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  const float gainLog = 0.1f * static_cast<float>(gain);
  const float gainLin = std::pow(10.0f, 0.05f * gainLog);

  return translate(mCore->setInputGainOffset(streamId, gainLin));
}

} // namespace IasAudio
