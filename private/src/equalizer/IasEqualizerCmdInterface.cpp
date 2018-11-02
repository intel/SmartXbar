/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasEqualizerCmdInterface.cpp
 * @date October 2016
 * @brief implementation of the equalizer command interface
 */

#include <cstdio>
#include <cmath>

#include "equalizer/IasEqualizerCmdInterface.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"
#include "equalizer/IasEqualizerCore.hpp"
#include "filter/IasAudioFilter.hpp"

namespace IasAudio {


static const uint32_t cIasUserEqualizerNumFilterStagesMax = 128; // This should be enough for everyone


static const IasAudioFilterParams flatFilterParams = { 0, 0.0f, 1.0f, eIasFilterTypeFlat, 2, 0 };


static const std::string cClassName = "IasEqualizerCmdInterface::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

// Define an empty Vector. Since all channels belonging to one stream share
// the same filter parameters, we do not need to fill out this Vector.
const std::vector<uint32_t> cEmptyChannelIdTable;


/*
 * Function to get a IasAudioPin::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

static std::string toString(const IasEqualizer::IasEqualizerMode&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerMode::eIasUser);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerMode::eIasCar);
    DEFAULT_STRING("IasEqualizerMode::eIasEqualizerModeInvalid");
  }
}

static std::string toString(const IasEqualizer::IasEqualizerCmdIds&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasSetModuleState);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetRampGradientSingleStream);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerNumFilters);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerNumFilters);
    STRING_RETURN_CASE(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerFilterParams);
    DEFAULT_STRING("IasEqualizerCmdIds::eIasCmdInvalid");
  }
}


IasEqualizerCmdInterface::IasEqualizerCmdInterface(const IasIGenericAudioCompConfig* config, IasEqualizerCore* core) :
  IasIModuleId{config}
  ,mCore{core}
  ,mCoreConfiguration{config}
  ,mNumFilterStages{}
  ,mFilterParamsMap{}
  ,mCmdFuncTable{}
  ,mMode{IasEqualizer::IasEqualizerMode::eIasUser}
  ,mLogContext{IasAudioLogging::getDltContext("_EQU")}
{
}


IasEqualizerCmdInterface::~IasEqualizerCmdInterface()
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}


IasIModuleId::IasResult IasEqualizerCmdInterface::init()
{
  using CmdIds = IasEqualizer::IasEqualizerCmdIds;

  uint32_t const maxSize = CmdIds::eIasLastEntry;
  mCmdFuncTable.resize(maxSize);
  mCmdFuncTable[CmdIds::eIasSetModuleState]                  = &IasEqualizerCmdInterface::setModuleState;
  mCmdFuncTable[CmdIds::eIasSetConfigFilterParamsStream]     = &IasEqualizerCmdInterface::setConfigFilterParamsStream;
  mCmdFuncTable[CmdIds::eIasGetConfigFilterParamsStream]     = &IasEqualizerCmdInterface::getConfigFilterParamsStream;
  mCmdFuncTable[CmdIds::eIasUserModeSetEqualizer]            = &IasEqualizerCmdInterface::setUserEqualizer;
  mCmdFuncTable[CmdIds::eIasUserModeSetEqualizerParams]      = &IasEqualizerCmdInterface::setUserEqualizerParams;
  mCmdFuncTable[CmdIds::eIasUserModeSetRampGradientSingleStream]= &IasEqualizerCmdInterface::setRampGradientSingleStream;
  mCmdFuncTable[CmdIds::eIasCarModeSetEqualizerNumFilters]   = &IasEqualizerCmdInterface::setCarEqualizerNumFilters;
  mCmdFuncTable[CmdIds::eIasCarModeSetEqualizerFilterParams] = &IasEqualizerCmdInterface::setCarEqualizerFilterParams;
  mCmdFuncTable[CmdIds::eIasCarModeGetEqualizerNumFilters]   = &IasEqualizerCmdInterface::getCarEqualizerNumFilters;
  mCmdFuncTable[CmdIds::eIasCarModeGetEqualizerFilterParams] = &IasEqualizerCmdInterface::getCarEqualizerFilterParams;

  int32_t tmpMode = -1;

  mConfig->getProperties().get("EqualizerMode", &tmpMode);

  if(tmpMode > static_cast<int32_t>(IasEqualizer::IasEqualizerMode::eIasLastEntry) || tmpMode < 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Equalizer mode not known");
    return IasIModuleId::eIasFailed;
  }
  mMode = static_cast<decltype(mMode)>(tmpMode);

  mCoreConfiguration.setNumFilterStagesMax(mCore->getNumFilterStagesMax());

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Equalizer Mode : ", toString(mMode));

  if(mMode == IasEqualizer::IasEqualizerMode::eIasUser)
  {
    return initUser();
  }

  else
  {
    return initCar();
  }
}


IasIModuleId::IasResult IasEqualizerCmdInterface::initCar()
{
  const auto& streams = mConfig->getStreams();

  IasIModuleId::IasResult status = IasIModuleId::eIasOk;

  IasStreamPointerList::const_iterator it_stream;
  for (it_stream = streams.begin(); it_stream != streams.end(); ++it_stream)
  {
    int32_t streamId     = (*it_stream)->getId();
    uint32_t numChannels = (*it_stream)->getNumberChannels();
    IasCarEqualizerStreamParams streamParams;
    streamParams.numChannels     = numChannels;
    streamParams.filterParamsVec = (IasCarEqualizerFilterParamsVec**)std::calloc(numChannels, sizeof(IasCarEqualizerFilterParamsVec *));
    if (streamParams.filterParamsVec == nullptr)
    {
     DLT_LOG_CXX(*mLogContext,  DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
     return IasResult::eIasFailed;
    }
    for (uint32_t chan=0; chan < numChannels; chan++)
    {
     streamParams.filterParamsVec[chan] = new (std::nothrow) IasCarEqualizerFilterParamsVec();
     if (streamParams.filterParamsVec[chan] == nullptr)
     {
       DLT_LOG_CXX(*mLogContext,  DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
       // Don't return immediately in case of an error, because otherwise all
       // memory spaces that have been allocated so far would be lost.
       status = IasIModuleId::IasResult::eIasFailed;
     }
     streamParams.filterParamsVec[chan]->resize(1, flatFilterParams);
    }
    mStreamParamsMap.insert(std::pair<int32_t, IasCarEqualizerStreamParams>(streamId, streamParams));
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Init Equalizer Cmd Interface");

  return status;
}


IasIModuleId::IasResult IasEqualizerCmdInterface::initUser()
{
  IasFilterParamsSingleStreamVec defaultFilterParamsVec;
  IasIModuleId::IasResult        error = IasIModuleId::IasResult::eIasOk;

  mNumFilterStages = mCoreConfiguration.getNumFilterStagesMax();

  // Check that we do not try to initialize thoudsands of filter stages...
  if (mNumFilterStages > cIasUserEqualizerNumFilterStagesMax)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": error, too many filter stages requested,",
                    "numFilterStages=", mNumFilterStages,
                    "must be smaller than", cIasUserEqualizerNumFilterStagesMax);
    return IasIModuleId::IasResult::eIasFailed;;
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": starting initialization with", mNumFilterStages, "filter bands");

   // Loop over all streams that have to be filtered.
  const auto& streams = mConfig->getStreams();

  IasStreamPointerList::const_iterator it_stream;
  if(streams.size() == 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": Streams are empty");
    return IasIModuleId::IasResult::eIasFailed;
  }

  for(it_stream = streams.begin(); it_stream!=streams.end(); ++it_stream)
  {
    int32_t const streamId = (*it_stream)->getId();
    error = translate(setDefaultFilterParams(&defaultFilterParamsVec));
    if(error)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": setDefaultFilterParams error");
      return error;
    }

    // Set the parameters of all filters used for this stream.
    error = translate(mCore->setFiltersSingleStream(streamId, cEmptyChannelIdTable, defaultFilterParamsVec));
    if (error)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": error while calling IasEqualizerCore::setFiltersSingleStream()");
      return IasIModuleId::IasResult::eIasFailed;
    }
    mFilterParamsMap.emplace(streamId, defaultFilterParamsVec);
  }

  return IasIModuleId::IasResult::eIasOk;
}


IasAudioProcessingResult IasEqualizerCmdInterface::setDefaultFilterParams(IasFilterParamsSingleStreamVec *vec)
{
  // Initialization of the default filter parameters. The default filter
  // parameters, which are set here, can be overridden by the command
  // eCommandSetEqualizerParams, which is provided by the tuning interface.
  // ---------------------------------------------------------------------------------------------------------------------
  //                                                            freq,  gain, qual, type,                   order, unused
  // ---------------------------------------------------------------------------------------------------------------------
  vec->clear();

 switch (mNumFilterStages)
 {

   case 0:
   // This is not supported
   DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "IasEqualizerCmdInterface::init: error, numFilterStages must not be 0");
   return eIasAudioProcInvalidParam;
   break;
   case 1:
   // If we have only one filter band, place it at 1000 Hz.
   vec->push_back((IasAudioFilterParams){  1000, 1.00f, 1.0f, eIasFilterTypePeak,         2, 0 });
   break;
   case 2:
   // If we have two filter bands, place them at 50 Hz and 10000 Hz.
   vec->push_back((IasAudioFilterParams){    50, 1.00f, 0.8f, eIasFilterTypePeak,         2, 0 });
   vec->push_back((IasAudioFilterParams){ 10000, 1.00f, 1.0f, eIasFilterTypeHighShelving, 2, 0 });
   break;
   case 3:
   // If we have three filter bands, place them at 50 Hz, 1000 Hz, and 10000 Hz.
   vec->push_back((IasAudioFilterParams){    50, 1.00f, 0.8f, eIasFilterTypePeak,         2, 0 });
   vec->push_back((IasAudioFilterParams){  1000, 1.00f, 1.0f, eIasFilterTypePeak,         2, 0 });
   vec->push_back((IasAudioFilterParams){ 10000, 1.00f, 1.0f, eIasFilterTypeHighShelving, 2, 0 });
   break;
   default:
   // Distribute filter bands equidistantly on a log-frequency axis between 50 Hz and 12000 Hz.
   float fMin = 50.0f;
   float fMax = 12000.0f;
   for (uint32_t filterId = 0; filterId < mNumFilterStages; filterId++)
   {
     float exponent = std::log10(fMin) + ((float)filterId/(float)(mNumFilterStages-1) *
       (std::log10(fMax) - std::log10(fMin)));
     uint32_t fCenter   = (uint32_t)(0.5f+std::pow(10.0f, exponent));
     vec->push_back((IasAudioFilterParams){ fCenter, 1.00f, 1.0f, eIasFilterTypePeak, 2, 0 });
   }
   break;
 }
 return eIasAudioProcOK;
}


IasIModuleId::IasResult IasEqualizerCmdInterface::processCmd(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  using CmdIds = IasEqualizer::IasEqualizerCmdIds;

  int32_t cmdId;
  IasProperties::IasResult status = cmdProperties.get("cmd", &cmdId);
  if (status != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Property with key \"cmd\" not found");
    return IasIModuleId::eIasFailed;
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Property with key \"cmd\" found, cmdId=", cmdId);

  const auto isUserCmd = [&]()
  {
    return (cmdId >= CmdIds::eIasUserModeFirst && cmdId <= CmdIds::eIasUserModeLast) ? true : false;
  };

  const auto isCarCmd = [&]()
  {
    return (cmdId >= CmdIds::eIasCarModeFirst && cmdId <= CmdIds::eIasCarModeLast) ? true : false;
  };

  if (static_cast<uint32_t>(cmdId) >= mCmdFuncTable.size())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Cmd with cmdId: ", cmdId, "not registered");
    return IasIModuleId::eIasFailed;
  }

  // Car Mode
  if (mMode == IasEqualizer::IasEqualizerMode::eIasCar)
  {
    if (isUserCmd())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Cmd with cmdId: ", cmdId,
                  ": ", toString(static_cast<CmdIds>(cmdId)),
                  " not registered in Car Mode");
      return IasIModuleId::eIasFailed;
    }
  }

  // User Mode
  else
  {
    if (isCarCmd())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Cmd with cmdId: ", cmdId,
                  ": ", toString(static_cast<CmdIds>(cmdId)),
                  "not registered in User Mode");
      return IasIModuleId::eIasFailed;
    }
  }

  return mCmdFuncTable[cmdId](this, cmdProperties, returnProperties);
}


IasIModuleId::IasResult IasEqualizerCmdInterface::setModuleState(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");
  std::string moduleState;
  IasProperties::IasResult result = cmdProperties.get("moduleState", &moduleState);

  if (result == IasProperties::eIasOk)
  {
    if (moduleState.compare("on") == 0)
    {
      mCore->enableProcessing();
    }
    else
    {
      mCore->disableProcessing();
    }
    return IasIModuleId::eIasOk;
  }
  else
  {
    return IasIModuleId::eIasFailed;
  }
}


IasIModuleId::IasResult IasEqualizerCmdInterface::getStreamId(const IasProperties& cmdProperties, int32_t& streamId)
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


IasIModuleId::IasResult IasEqualizerCmdInterface::translate(IasAudioProcessingResult result)
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


IasIModuleId::IasResult IasEqualizerCmdInterface::setRampGradientSingleStream(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, gradient;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  const auto res = cmdProperties.get("gradient", &gradient);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, ": ",
                  "streamId=", streamId,
                  "gradient=", gradient);

  const float gradientf = std::pow(20.0f, static_cast<float>(gradient)/1000.0f);

  return translate(mCore->setRampGradientSingleStream(streamId, gradientf));
}


IasIModuleId::IasResult IasEqualizerCmdInterface::setConfigFilterParamsStream(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, filterId, freq, quality, type, order;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  auto res = cmdProperties.get("filterId", &filterId);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("freq", &freq);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("quality", &quality);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("type", &type);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("order", &order);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  if (static_cast<uint32_t>(filterId) >= mNumFilterStages)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid parameters: filterBand=", filterId);
    return IasIModuleId::eIasFailed;
  }

  IasEqConfigFilterParams params;
  params.freq    = static_cast<decltype(params.freq)>(freq);
  params.gain    = 1.0f; // let us start with 0 dB.
  params.quality = static_cast<decltype(params.quality)>(quality) * 0.1f;
  params.type    = static_cast<decltype(params.type)>(type);
  params.order   = static_cast<decltype(params.order)>(order);

  const auto& streams = mConfig->getStreams();

  const auto it_stream = std::find_if(streams.begin(), streams.end(),
      [&](IasAudioStream* const el) { return el->getId() == streamId;});

  if (it_stream == streams.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid StreamId:",
                    "streamId=",   streamId);
    return IasIModuleId::eIasFailed;
  }

  return translate(mCoreConfiguration.setConfigFilterParamsStream((*it_stream), filterId, &params));

}


IasIModuleId::IasResult IasEqualizerCmdInterface::getConfigFilterParamsStream(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, filterId;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  const auto res = cmdProperties.get("filterId", &filterId);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  const auto& streams = mConfig->getStreams();

  const auto it_stream = std::find_if(streams.begin(), streams.end(),
      [&](IasAudioStream* const el) { return el->getId() == streamId;});

  if (it_stream == streams.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid StreamId:",
                    "streamId=",   streamId);
    return IasIModuleId::eIasFailed;
  }

  IasEqConfigFilterParams params;

  const auto err = translate(mCoreConfiguration.getConfigFilterParamsStream((*it_stream), filterId, &params));

  if(err != IasIModuleId::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  returnProperties.set<int32_t>("freq",    static_cast<int32_t>(params.freq));
  returnProperties.set<int32_t>("gain",    static_cast<int32_t>(params.gain));
  returnProperties.set<int32_t>("quality", static_cast<int32_t>(params.quality * 10.0f));
  returnProperties.set<int32_t>("type",    static_cast<int32_t>(params.type));
  returnProperties.set<int32_t>("order",   static_cast<int32_t>(params.order));

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasEqualizerCmdInterface::setUserEqualizer(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, filterId, gain;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  auto res = cmdProperties.get("filterId", &filterId);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("gain", &gain);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  const float gainLog = 0.1f * static_cast<float>(gain);
  const float gainLin = std::pow(10.0f, 0.05f * gainLog);

  return translate(mCore->rampGainSingleStreamSingleFilter(streamId, filterId, gainLin));
}


IasIModuleId::IasResult IasEqualizerCmdInterface::setUserEqualizerParams(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, filterId, freq, quality, type, order;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  auto res = cmdProperties.get("filterId", &filterId);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("freq", &freq);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("quality", &quality);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("type", &type);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("order", &order);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  IasAudioFilterParams filterParams;
  filterParams.freq    = static_cast<decltype(filterParams.freq)>(freq);
  filterParams.gain    = 1.0f; // let us start with 0 dB.
  filterParams.quality = static_cast<decltype(filterParams.quality)>(quality) * 0.1f;
  filterParams.type    = static_cast<decltype(filterParams.type)>(type);
  filterParams.order   = static_cast<decltype(filterParams.order)>(order);
  filterParams.section = 0;    // we support only filters consisting of one single section.


  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, ": processing command 'SetEqualizerParams',",
                  "filterBand=", filterId,
                  "filterType=", toString(filterParams.type),
                  "freq=", filterParams.freq);

  if ((filterParams.order != 1) && (filterParams.order != 2))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid parameters: filterOrder=", filterParams.order);
    return IasIModuleId::eIasFailed;
  }

  if (static_cast<uint32_t>(filterId) >= mNumFilterStages)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid parameters: filterBand=", filterId);
    return IasIModuleId::eIasFailed;
  }

  // Find the filter parameter set that is associated with this streamId.
  const auto filterParamsIt = mFilterParamsMap.find(streamId);
  if (filterParamsIt == mFilterParamsMap.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid StreamId:",
                    "streamId=",   streamId);
    return IasIModuleId::eIasFailed;
  }

  // Get access to the filter parameters of the filter cascade that belongs to this stream.
  IasFilterParamsSingleStreamVec& filterParamsVec = filterParamsIt->second;

  // Save the parameters of the affected filter stage, so that we are able
  // to restore them if the new parameters produce an error.
  const IasAudioFilterParams filterParamsBackup = filterParamsVec[filterId];

  // Update the parameters of the affected filter stage.
  filterParamsVec[filterId] = filterParams;

  // Set the parameters of all filters used for this stream.
  const auto error = translate(mCore->setFiltersSingleStream(streamId, cEmptyChannelIdTable, filterParamsVec));

  if (error != IasIModuleId::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": error while calling IasEqualizerCore::setFiltersSingleStream(), now restoring old parameters");

    // Restore the old parameters
    filterParamsVec[filterId] = filterParamsBackup;
    mCore->setFiltersSingleStream(streamId, cEmptyChannelIdTable, filterParamsVec);
    return IasIModuleId::eIasFailed;
  }

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasEqualizerCmdInterface::setCarEqualizerNumFilters(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, channelIdx, numFilters;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  auto res = cmdProperties.get("channelIdx", &channelIdx);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("numFilters", &numFilters);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, ": ",
                  "streamId=",   streamId,
                  "channelIdx=", channelIdx,
                  "numFilters=", numFilters);

  // Find the stream parameter set that is associated with this streamId.
  IasCarEqualizerStreamParamsMap::const_iterator streamParamsIt = mStreamParamsMap.find(streamId);
  if (streamParamsIt == mStreamParamsMap.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid StreamId:",
                    "streamId=",   streamId);
    return IasIModuleId::eIasFailed;
  }
  // Verify that numFilters does not exceed the numFilterStagesMax
  // provided by the IasEqualizerConfiguration.

  if (static_cast<uint32_t>(numFilters) > mCoreConfiguration.getNumFilterStagesMax())
  {
    // Invalid numFilters
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid number of filters:",
                    "numFilters=", numFilters,
                    "max. allowed=",
                    mCoreConfiguration.getNumFilterStagesMax() );
    return IasIModuleId::eIasFailed;
  }
  // Verify that channelIdx does not exceed the number of channels
  // belonging to this stream.
  if (static_cast<uint32_t>(channelIdx) >= streamParamsIt->second.numChannels)
  {
    // Invalid channelIdx
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, ":: invalid channelIdx:",
                    "channelIdx=", channelIdx,
                    "number of available channels=", streamParamsIt->second.numChannels);
    return IasIModuleId::eIasFailed;
  }
  // Get access to the filter parameters of the filter cascade that
  // belongs to this stream / this channel.
  IasCarEqualizerFilterParamsVec &filterParamsVec = *(streamParamsIt->second.filterParamsVec[channelIdx]);
  // Adjust the size of the filterParamsVec, which holds the filter parameters
  // for the complete chain. If new size is greater than the current size,
  // flat filters are inserted at the end of the cascade. If new size is smaller
  // than the current size, filters are dropped at the end of the cascade.
  filterParamsVec.resize(numFilters, flatFilterParams);
  // Let the EqualizerCore adopt the updated filter parameters for
  // this stream / this channel.
  std::vector<uint32_t> channelIdTable;
  channelIdTable.push_back(channelIdx);
  return translate(mCore->setFiltersSingleStream(streamId, channelIdTable, filterParamsVec));

}


IasIModuleId::IasResult IasEqualizerCmdInterface::getCarEqualizerNumFilters(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, channelIdx;
  uint32_t numFilters;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  const auto res = cmdProperties.get("channelIdx", &channelIdx);
  if(res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  const auto err = translate(mCore->getNumFiltersForChannel(streamId, channelIdx, &numFilters));
  if(err != IasIModuleId::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  returnProperties.set<int32_t>("numFilters", static_cast<int32_t>(numFilters));

  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasEqualizerCmdInterface::setCarEqualizerFilterParams(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  (void)returnProperties;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, channelIdx, filterId, freq, quality, type, order, gain_dB10;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  auto res = cmdProperties.get("channelIdx", &channelIdx);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("filterId", &filterId);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("freq", &freq);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("gain", &gain_dB10);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("quality", &quality);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("type", &type);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  res = cmdProperties.get("order", &order);
  if (res != IasProperties::eIasOk)
  {
    return IasIModuleId::eIasFailed;
  }

  IasAudioFilterParams filterParams;

  filterParams.freq    = static_cast<decltype(filterParams.freq)>(freq);
  filterParams.gain    = std::pow(10.0f, static_cast<decltype(filterParams.gain)>(gain_dB10) * (1.0f/200.0f));
  filterParams.quality = 0.1f * static_cast<decltype(filterParams.quality)>(quality);
  filterParams.type    = static_cast<decltype(filterParams.type)>(type);
  filterParams.order   = static_cast<decltype(filterParams.order)>(order);
  filterParams.section = 1;

  // Find the stream parameter set that is associated with this streamId.
  IasCarEqualizerStreamParamsMap::const_iterator streamParamsIt = mStreamParamsMap.find(streamId);
  if (streamParamsIt == mStreamParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid StreamId:",
                    "streamId=",   streamId);
    return IasIModuleId::eIasFailed;
  }
  // Verify that the channelIdx does not exceed the number of channels
  // belonging to this stream.
  if (static_cast<uint32_t>(channelIdx) >= streamParamsIt->second.numChannels)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid channelIdx:",
                    "channelIdx=", channelIdx,
                    "number of available channels=", streamParamsIt->second.numChannels);
    return IasIModuleId::eIasFailed;
  }
  // Get access to the filter parameters of the filter cascade that
  // belongs to this stream / this channel.
  IasCarEqualizerFilterParamsVec &filterParamsVec = *(streamParamsIt->second.filterParamsVec[channelIdx]);

  // Verify that the filterIdx does not exceed the number of filters that have
  // been declared for this stream / this channel.
  if (static_cast<uint32_t>(filterId) >= filterParamsVec.size())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid filterIdx:",
                    "filterIdx=", filterId,
                    "number of available filters=", filterParamsVec.size());
    return IasIModuleId::eIasFailed;
  }
  // Create a backup of the filter parameters of the filter stage whose
  // parameters shall be modified. We will need this backup if the filter
  // does not accept the new parameters.
  IasAudioFilterParams const filterParamsBackup = filterParamsVec[filterId];

  // Store the current filter parameters into the filterParamsVec.
  filterParamsVec[filterId] = filterParams;

  // Let the EqualizerCore adopt the updated filter parameters for
  // this stream / this channel.
  std::vector<uint32_t> channelIdTable;
  channelIdTable.emplace_back(channelIdx);

  const auto error = translate(mCore->setFiltersSingleStream(streamId, channelIdTable, filterParamsVec));

  if (error != IasIModuleId::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": error while calling ", LOG_PREFIX);
    // Restore the old parameters (using the backup).
    filterParamsVec[filterId] = filterParamsBackup;

    mCore->setFiltersSingleStream(streamId, channelIdTable, filterParamsVec);

    return IasIModuleId::eIasFailed;
  }
  return IasIModuleId::eIasOk;
}


IasIModuleId::IasResult IasEqualizerCmdInterface::getCarEqualizerFilterParams(const IasProperties& cmdProperties, IasProperties& returnProperties)
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "called");
  cmdProperties.dump("cmdProperties");

  int32_t streamId, channelIdx, filterId;

  const auto modres = getStreamId(cmdProperties, streamId);
  if(modres != IasIModuleId::eIasOk)
  {
    return modres;
  }

  auto res = cmdProperties.get("channelIdx", &channelIdx);
  if(res != IasProperties::eIasOk) { return IasIModuleId::eIasFailed; }

  res = cmdProperties.get("filterId", &filterId);
  if(res != IasProperties::eIasOk) { return IasIModuleId::eIasFailed; }

  IasAudioFilterParams filterParams;
  const auto err = translate(mCore->getFilterParamsForChannel(streamId, filterId, channelIdx, &filterParams));
  if(err != IasIModuleId::eIasOk) { return IasIModuleId::eIasFailed; }

  float const factor   = (filterParams.gain >= 1 ? 0.5f : -0.5f);
  int32_t   const gain_dB10 = (int32_t)(200.0f * std::log10(filterParams.gain) +factor);

  returnProperties.set("freq",    static_cast<int32_t>(filterParams.freq));
  returnProperties.set("gain",    static_cast<int32_t>(gain_dB10));
  returnProperties.set("quality", static_cast<int32_t>(filterParams.quality*10.0f));
  returnProperties.set("type",    static_cast<int32_t>(filterParams.type));
  returnProperties.set("order",   static_cast<int32_t>(filterParams.order));

  return IasIModuleId::eIasOk;
}


} // namespace IasAudio


