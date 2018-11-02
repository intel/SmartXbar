/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    IasEqualizerConfiguration.cpp
 * @brief   Configuration class for the equalizer core module.
 * @date    2016
 */

#include <cstdlib>
#include "equalizer/IasEqualizerConfiguration.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "filter/IasAudioFilter.hpp"



namespace IasAudio {

IasEqualizerConfiguration::IasEqualizerConfiguration(const IasIGenericAudioCompConfig* config) :
   mConfig{config}
  ,mNumFilterStagesMax{}
  ,mNumFilterStagesMaxInitialized{}
  ,mEqStreamMap{}
{
}

IasEqualizerConfiguration::~IasEqualizerConfiguration()
{
}

void IasEqualizerConfiguration::setNumFilterStagesMax(const uint32_t numFilterStagesMax)
{
  mNumFilterStagesMax = numFilterStagesMax;
  mNumFilterStagesMaxInitialized = true;
}

IasAudioProcessingResult IasEqualizerConfiguration::setConfigFilterParamsStream(IasAudioStream* stream, uint32_t filterId, const IasEqConfigFilterParams *params)
{
  std::list<IasAudioStream*>::const_iterator it_list;
  int32_t streamId = 0;

  if(mNumFilterStagesMaxInitialized == false)
  {
    return eIasAudioProcNotInitialization;
  }

  if( (nullptr == stream) || (nullptr == params) || (filterId >= mNumFilterStagesMax) )
  {
    return eIasAudioProcInvalidParam;
  }

  const auto& activeStreamPointers = mConfig->getStreams();

  streamId = stream->getId();
  for(it_list=activeStreamPointers.begin(); it_list!=activeStreamPointers.end();it_list++)
  {
    if( (*it_list)->getId() == streamId )
    {
      break;
    }
  }
  if(it_list == activeStreamPointers.end())
  {
    return eIasAudioProcInvalidStream; //streamId not listed in active streams
  }

  IasEqConfigStreamMap::iterator configStreamIt = mEqStreamMap.find(streamId);
  if(configStreamIt == mEqStreamMap.end())
  {
    //if streamId is not inserted, create new vector with filter params and insert entry in map
    std::vector<IasEqConfigFilterInitParams> paramsVec;
    paramsVec.resize(mNumFilterStagesMax);
    paramsVec[filterId].isInit = true;
    paramsVec[filterId].params.freq =  params->freq;
    paramsVec[filterId].params.gain =  1.0f; //ths function is intended for user eq, so the gain should be set 1.0f initially
    paramsVec[filterId].params.quality =  params->quality;
    paramsVec[filterId].params.order =  params->order;
    paramsVec[filterId].params.type =  params->type;

    IasEqConfigStreamMapPair tempPair(streamId,paramsVec);
    mEqStreamMap.insert(tempPair);
  }
  else
  {
    //stream already added, so change params of vector
    configStreamIt->second[filterId].isInit = true;
    configStreamIt->second[filterId].params.freq =  params->freq;
    configStreamIt->second[filterId].params.gain =  1.0f;
    configStreamIt->second[filterId].params.quality =  params->quality;
    configStreamIt->second[filterId].params.order =  params->order;
    configStreamIt->second[filterId].params.type =  params->type;
  }

  return eIasAudioProcOK;
}

IasAudioProcessingResult IasEqualizerConfiguration::getConfigFilterParamsStream(IasAudioStream* stream, uint32_t filterId, IasEqConfigFilterParams *params) const
{
  std::list<IasAudioStream*>::const_iterator it_list;
  int32_t streamId = 0;

  if( (nullptr == stream) || (nullptr == params) || (filterId >= mNumFilterStagesMax) )
  {
    return eIasAudioProcInvalidParam;
  }

  const auto& activeStreamPointers = mConfig->getStreams();

  streamId = stream->getId();
  for(it_list=activeStreamPointers.begin(); it_list!=activeStreamPointers.end();it_list++)
  {
    if( (*it_list)->getId() == streamId )
    {
      break;
    }
  }
  if(it_list == activeStreamPointers.end())
  {
    return eIasAudioProcInvalidStream; //streamId not listed in active streams
  }

  IasEqConfigStreamMap::const_iterator it = mEqStreamMap.find(streamId);

  if(it == mEqStreamMap.end())
  {
    return eIasAudioProcNotInitialization;
  }

  if(it->second[filterId].isInit == false)
  {
    return eIasAudioProcNotInitialization;
  }

  params->freq    = it->second[filterId].params.freq;
  params->gain    = it->second[filterId].params.gain;
  params->quality = it->second[filterId].params.quality;
  params->type    = it->second[filterId].params.type;
  params->order   = it->second[filterId].params.order;

  return eIasAudioProcOK;
}

} // namespace IasAudio
