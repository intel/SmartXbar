/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasVolumeCmdInterface.cpp
 * @date 2012
 * @brief the implementation of the command interface of the volume module
 */

#include "volume/IasVolumeCmdInterface.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "helper/IasRamp.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "volume/IasVolumeHelper.hpp"
#include <math.h>
#include <stdio.h>

using namespace std;

namespace IasAudio {

using namespace IasVolumeHelper;

static const std::string cClassName = "IasVolumeCmdInterface::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasVolumeCmdInterface::IasVolumeCmdInterface(const IasIGenericAudioCompConfig *config, IasVolumeLoudnessCore *core)
  :IasIModuleId(config)
  ,mCore(core)
  ,mCmdFuncTable()
  ,mNumFilterBands(0)
  ,mLogContext(IasAudioLogging::getDltContext("_VOL"))
{
}

IasVolumeCmdInterface::~IasVolumeCmdInterface()
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}

IasIModuleId::IasResult IasVolumeCmdInterface::init()
{
  const uint32_t maxSize = IasVolume::eIasLastEntry;
  mCmdFuncTable.resize(maxSize);
  mCmdFuncTable[IasVolume::eIasSetModuleState] = &IasVolumeCmdInterface::setModuleState;
  mCmdFuncTable[IasVolume::eIasSetVolume] = &IasVolumeCmdInterface::setVolume;
  mCmdFuncTable[IasVolume::eIasSetMuteState] = &IasVolumeCmdInterface::setMuteState;
  mCmdFuncTable[IasVolume::eIasSetLoudness] = &IasVolumeCmdInterface::setLoudness;
  mCmdFuncTable[IasVolume::eIasSetSpeedControlledVolume] = &IasVolumeCmdInterface::setSpeedControlledVolume;
  mCmdFuncTable[IasVolume::eIasSetLoudnessTable] = &IasVolumeCmdInterface::setLoudnessTable;
  mCmdFuncTable[IasVolume::eIasGetLoudnessTable] = &IasVolumeCmdInterface::getLoudnessTable;
  mCmdFuncTable[IasVolume::eIasSetSpeed] = &IasVolumeCmdInterface::setSpeed;
  mCmdFuncTable[IasVolume::eIasSetLoudnessFilter] = &IasVolumeCmdInterface::setLoudnessFilter;
  mCmdFuncTable[IasVolume::eIasGetLoudnessFilter] = &IasVolumeCmdInterface::getLoudnessFilter;
  mCmdFuncTable[IasVolume::eIasSetSdvTable] = &IasVolumeCmdInterface::setSdvTable;
  mCmdFuncTable[IasVolume::eIasGetSdvTable] = &IasVolumeCmdInterface::getSdvTable;

  const IasProperties &properties = mConfig->getProperties();
  int32_t numFilterBands;
  properties.get("numFilterBands", &numFilterBands);
  if (numFilterBands == 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Number of filter stages may not be 0");
    return IasIModuleId::eIasFailed;
  }
  mNumFilterBands = static_cast<uint32_t>(numFilterBands);

  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasVolumeCmdInterface::processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties)
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
    else if (static_cast<std::uint32_t>(cmdId) == cGetParametersCmdId)
    {
      return IasVolumeCmdInterface::getParameters(cmdProperties, returnProperties);
    }
    else if (static_cast<std::uint32_t>(cmdId) == cSetParametersCmdId)
    {
      return IasVolumeCmdInterface::setParameters(cmdProperties, returnProperties);
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

IasIModuleId::IasResult IasVolumeCmdInterface::setModuleState(const IasProperties& cmdProperties,
                                                              IasProperties& returnProperties)
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

IasIModuleId::IasResult IasVolumeCmdInterface::getStreamId(const IasProperties &cmdProperties, int32_t &streamId)
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

static IasIModuleId::IasResult translate(IasAudioProcessingResult result)
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

IasIModuleId::IasResult IasVolumeCmdInterface::setVolume(const IasProperties& cmdProperties,
                                                         IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t streamId;
  IasIModuleId::IasResult modres = getStreamId(cmdProperties, streamId);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Stream Id=", streamId);
  int32_t volumedb;
  IasProperties::IasResult propres = cmdProperties.get("volume", &volumedb);
  if (propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Volume=", volumedb);
  IasInt32Vector rampParams;
  propres = cmdProperties.get("ramp", &rampParams);
  if (propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  if (rampParams.size() != 2)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Incorrect number of params. Expected 2 but received", rampParams.size());
    return IasIModuleId::eIasFailed;
  }
  int32_t rampTime = rampParams[0];
  if (rampTime < cMinRampTime || rampTime > cMaxRampTime)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Ramp time of",rampTime,"[ms] not valid, must be in the range of",cMinRampTime,"to",cMaxRampTime);
    return IasIModuleId::eIasFailed;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Ramp time=", rampTime, "ms");
  IasRampShapes rampShape = static_cast<IasRampShapes>(rampParams[1]);
  if ((rampShape != eIasRampShapeLinear) && (rampShape != eIasRampShapeExponential))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Ramp shape ",toString(rampShape),"is not supported");
    return IasIModuleId::eIasFailed;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Ramp shape=", rampParams[1]);
  float volume = static_cast<float>(volumedb);
  if(volumedb <= -1440)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Received volume of ",volumedb, "[db/10], will be treated as mute.");
    volume = 0.0f;
  }
  else
  {
    volume = powf(10.0f, volume/200.0f);
  }
  return translate(mCore->setVolume(streamId, volume, rampTime, rampShape));
}

IasIModuleId::IasResult IasVolumeCmdInterface::setMuteState(const IasProperties& cmdProperties,
                                                            IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t streamId;
  IasIModuleId::IasResult modres = getStreamId(cmdProperties, streamId);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  IasInt32Vector params;
  IasProperties::IasResult result = cmdProperties.get("params", &params);
  if (result != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  if (params.size() != 3)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Incorrect number of params. Expected 3 but received", params.size());
    return IasIModuleId::eIasFailed;
  }
  bool sinkOff = static_cast<bool>(params[0]);
  uint32_t rampTime = static_cast<uint32_t>(params[1]);
  if (rampTime < cMinRampTime || rampTime > cMaxRampTime)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Ramp time of",rampTime,"[ms] not valid, must be in the range of",cMinRampTime,"to",cMaxRampTime);
    return IasIModuleId::eIasFailed;
  }
  IasRampShapes rampShape = static_cast<IasRampShapes>(params[2]);
  if ((rampShape != eIasRampShapeLinear) && (rampShape != eIasRampShapeExponential))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Ramp shape ",toString(rampShape),"is not supported");
    return IasIModuleId::eIasFailed;
  }
  return translate(mCore->setMuteState(streamId, sinkOff, rampTime, rampShape));
}

IasIModuleId::IasResult IasVolumeCmdInterface::setLoudness(const IasProperties& cmdProperties,
                                                           IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t streamId;
  IasIModuleId::IasResult modres = getStreamId(cmdProperties, streamId);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  std::string loudnessState;
  IasProperties::IasResult propres = cmdProperties.get("loudness", &loudnessState);
  if (propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  bool loudnessOn;
  if (loudnessState.compare("on") == 0)
  {
    loudnessOn = true;
  }
  else
  {
    loudnessOn = false;
  }
  return translate(mCore->setLoudnessOnOff(streamId, loudnessOn));
}

IasIModuleId::IasResult IasVolumeCmdInterface::setSpeedControlledVolume(const IasProperties& cmdProperties,
                                                                        IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t streamId;
  IasIModuleId::IasResult modres = getStreamId(cmdProperties, streamId);
  if (modres != IasIModuleId::eIasOk)
  {
    return modres;
  }
  std::string sdvState;
  IasProperties::IasResult propres = cmdProperties.get("sdv", &sdvState);
  if (propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }
  bool sdvOn;
  if (sdvState.compare("on") == 0)
  {
    sdvOn = true;
  }
  else
  {
    sdvOn = false;
  }
  return translate(mCore->setSpeedControlledVolume(streamId, sdvOn));
}

IasIModuleId::IasResult IasVolumeCmdInterface::setLoudnessTable(const IasProperties& cmdProperties,
                                                                IasProperties& returnProperties)
{
  (void)returnProperties;
  IasIModuleId::IasResult result = IasIModuleId::eIasOk;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasVolumeLoudnessTable loudnessTable;
    if (getLoudnessTableFromProperties(cmdProperties, i, loudnessTable) == true)
    {
      // Only set the loudness table when it was set in the properties and ignore the default values
      IasAudioProcessingResult procres = mCore->setLoudnessTable(i, &loudnessTable);
      if (procres != eIasAudioProcOK)
      {
        result = IasIModuleId::eIasFailed;
      }
    }
    else
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "error getting loudness table for band", i, "from properties");
    }
  }

  return result;
}

IasIModuleId::IasResult IasVolumeCmdInterface::getLoudnessTable(const IasProperties& cmdProperties,
                                                                IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasVolumeLoudnessTable loudnessTable;
    mCore->getLoudnessTable(i, loudnessTable);
    setLoudnessTableInProperties(i, loudnessTable, returnProperties);
  }
  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasVolumeCmdInterface::setSpeed(const IasProperties& cmdProperties,
                                                        IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  int32_t speed;
  IasProperties::IasResult propres = cmdProperties.get("speed", &speed);
  if (propres != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  mCore->setSpeed(static_cast<uint32_t>(speed));

  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasVolumeCmdInterface::setLoudnessFilter(const IasProperties& cmdProperties,
                                                                 IasProperties& returnProperties)
{
  (void)returnProperties;
  IasIModuleId::IasResult result = IasIModuleId::eIasOk;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasAudioFilterConfigParams filterParams;
    if (getLoudnessFilterParamsFromProperties(cmdProperties, i, filterParams) == true)
    {
      if( checkLoudnessFilterParams(filterParams) )
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Error in loudness filter params, invalid paramter range");
        return IasIModuleId::eIasFailed;
      }
      filterParams.gain = 1.0f;
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                  "Frequency=", filterParams.freq,
                  "Order=", filterParams.order,
                  "Type=", static_cast<uint32_t>(filterParams.type),
                  "Gain=", filterParams.gain,
                  "Quality=", filterParams.quality);
      IasAudioProcessingResult procres = mCore->setLoudnessFilterAllStreams(i, &filterParams);
      if (procres != eIasAudioProcOK)
      {
        result = IasIModuleId::eIasFailed;
      }
    }

  }
  return result;
}

IasIModuleId::IasResult IasVolumeCmdInterface::getLoudnessFilter(const IasProperties& cmdProperties,
                                                                 IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasAudioFilterConfigParams filterParams;
    mCore->getLoudnessFilterAllStreams(i, filterParams);
    setLoudnessFilterParamsInProperties(i, filterParams, returnProperties);
  }
  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasVolumeCmdInterface::setSdvTable(const IasProperties& cmdProperties,
                                                           IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  IasVolumeSDVTable table;
  if (getSDVTableFromProperties(cmdProperties, table) == true)
  {
    IasAudioProcessingResult procres = mCore->setSDVTable(&table);
    return translate(procres);
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}

IasIModuleId::IasResult IasVolumeCmdInterface::getSdvTable(const IasProperties& cmdProperties,
                                                           IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  IasVolumeSDVTable sdvTable;
  IasAudioProcessingResult procres = mCore->getSDVTable(sdvTable);
  if (procres == eIasAudioProcOK)
  {
    setSDVTableInProperties(sdvTable, returnProperties);
    return IasIModuleId::eIasOk;
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}

IasIModuleId::IasResult IasVolumeCmdInterface::getParameters(const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  cmdProperties.dump("cmdProperties");

  returnProperties.set("MinVol", mCore->getMinVol());
  returnProperties.set("MaxVol", mCore->getMaxVol());

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasVolumeCmdInterface::setParameters(const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  cmdProperties.dump("cmdProperties");

  (void) returnProperties;
  std::int32_t minVol;
  std::int32_t maxVol;
  if (cmdProperties.get("MinVol", &minVol) != IasProperties::eIasOk ||
    cmdProperties.get("MaxVol", &maxVol) != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  mCore->updateMinVol(minVol);
  mCore->updateMaxVol(maxVol);


  return IasIModuleId::eIasOk;
}

} // namespace IasAudio
