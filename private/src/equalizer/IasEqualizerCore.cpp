/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 *  @file   IasEqualizerCore.cpp
 *  @date   October 2016
 *  @brief
 *
 */

#include <cstring>
#include <cstdlib>
#include <math.h>

#include "equalizer/IasEqualizerCore.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasStreamParams.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "audio/smartx/IasEventProvider.hpp"

namespace IasAudio {

static const std::string cClassName = "IasEqualizerCore::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasEqualizerCore::IasEqualizerCore(const IasIGenericAudioCompConfig* config, const std::string& componentName)
  :IasGenericAudioCompCore(config, componentName)
  ,mOutputBundlesList{}
  ,mNumFilterStagesMax{}
  ,mNumBundles{}
  ,mNumStreams{}
  ,mChannelParams{}
  ,mFilters{}
  ,mBundleParams{}
  ,mStreamStatusMap{}
  ,mTypeName{""}
  ,mInstanceName{""}
  ,mLogContext{IasAudioLogging::getDltContext("_EQU")}
{
}


IasEqualizerCore::~IasEqualizerCore()
{
  for (uint32_t cntBundles=0; cntBundles < mNumBundles; cntBundles++)
  {
    for (uint32_t cntChannels=0; cntChannels < cIasNumChannelsPerBundle; cntChannels++)
    {
      std::free(mChannelParams[cntBundles][cntChannels].filterStageIndexTab);
      std::free(mChannelParams[cntBundles][cntChannels].filterParamsTab);
      mChannelParams[cntBundles][cntChannels].filterStageIndexTab = nullptr;
      mChannelParams[cntBundles][cntChannels].filterParamsTab     = nullptr;
    }
    std::free(mChannelParams[cntBundles]);
    mChannelParams[cntBundles] = nullptr;
  }
  std::free(mChannelParams);
  std::free(mBundleParams);
  mChannelParams = nullptr;
  mBundleParams  = nullptr;
  for (uint32_t cntBundles=0; cntBundles < mNumBundles; cntBundles++)
  {
    for (uint32_t cntFilterStages = 0; cntFilterStages < mNumFilterStagesMax; cntFilterStages++)
    {
      delete(mFilters[cntBundles][cntFilterStages]);
      mFilters[cntBundles][cntFilterStages] = nullptr;
    }
    std::free(mFilters[cntBundles]);
    mFilters[cntBundles] = nullptr;
  }
  std::free(mFilters);
  mFilters = nullptr;
  IasEqualizerCoreStreamStatusMap::iterator streamStatusIt;
  for (streamStatusIt=mStreamStatusMap.begin(); streamStatusIt != mStreamStatusMap.end(); ++streamStatusIt)
  {
    if (streamStatusIt->second.isGainRamping != nullptr)
    {
      std::free(streamStatusIt->second.isGainRamping);
      streamStatusIt->second.isGainRamping = nullptr;
    }
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, ": Deleted");
}


IasAudioProcessingResult IasEqualizerCore::reset()
{
  for (uint32_t cntBundles = 0; cntBundles < mNumBundles; cntBundles++)
  {
    uint32_t numActiveFilterStages = mBundleParams[cntBundles].numActiveFilterStages;
    IasAudioFilter** filters          = mFilters[cntBundles];
    for (uint32_t cntFilterStages = 0; cntFilterStages < numActiveFilterStages; cntFilterStages++)
    {
      filters[cntFilterStages]->reset();
    }
  }
  return eIasAudioProcOK;
}


IasAudioProcessingResult IasEqualizerCore::init()
{
  const IasProperties &properties = mConfig->getProperties();

  auto result = properties.get<std::string>("typeName", &mTypeName);
  if (result != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'typeName' in module configuration properties");
    return eIasAudioProcInitializationFailed;
  }

  result = properties.get<std::string>("instanceName",  &mInstanceName);
  if (result != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'instanceName' in module configuration properties");
    return eIasAudioProcInitializationFailed;
  }

  int32_t tmpNumFilterStagesMax = 0;

  result = properties.get("numFilterStagesMax", &tmpNumFilterStagesMax);

  if (result != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'numFilterStagesMax' in module configuration properties");
    return eIasAudioProcInitializationFailed;
  }

  if (tmpNumFilterStagesMax <= 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Number of Filter Stages Max should not be zero");
    return eIasAudioProcInvalidParam;
  }

  mNumFilterStagesMax = tmpNumFilterStagesMax;

  const IasBundlePointerList& bundles = mConfig->getBundles();
  const IasStreamPointerList& streams = mConfig->getStreams();
  mNumBundles = static_cast<uint32_t>(bundles.size());
  mNumStreams = static_cast<uint32_t>(streams.size());

  // Allocate memory for all the pointers to the filter objects. We use a two-dimensional
  // array mFilters[mNumBundles][mNumFilterStagesMax], which will carry the pointers
  // to the filter objects.
  mFilters = (IasAudioFilter***) std::calloc( mNumBundles, sizeof(IasAudioFilter**) );
  if (mFilters == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, ": not enough memory!");
    return eIasAudioProcNotEnoughMemory;
  }

  for (uint32_t cntBundles=0; cntBundles < mNumBundles; cntBundles++)
  {
    mFilters[cntBundles] = (IasAudioFilter**) std::calloc( mNumFilterStagesMax, sizeof(IasAudioFilter*) );
    if (mFilters[cntBundles] == nullptr)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
  }

  // Create the filter objects.
  for (uint32_t cntBundles=0; cntBundles < mNumBundles; cntBundles++)
  {
    for (uint32_t cntFilterStages = 0; cntFilterStages < mNumFilterStagesMax; cntFilterStages++)
    {
      mFilters[cntBundles][cntFilterStages] = new (std::nothrow) IasAudioFilter(mSampleRate, mFrameLength);
      if (mFilters[cntBundles][cntFilterStages] == nullptr)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
        return eIasAudioProcNotEnoughMemory;
      }
      if (mFilters[cntBundles][cntFilterStages]->init())
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,  ": not enough memory!");
        return eIasAudioProcNotEnoughMemory;
      }
      mFilters[cntBundles][cntFilterStages]->announceCallback(this);
    }
  }
  // Create the vector mBundleParams[mNumBundles], which contains for each
  // bundle the bundle-specific parameters.
  mBundleParams = (IasEqualizerCoreBundleParams*)std::calloc(mNumBundles, sizeof(IasEqualizerCoreBundleParams));
  if (mBundleParams == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
    return eIasAudioProcNotEnoughMemory;
  }

  // Create the two-dimensional array mChannelParams[mNumBundles][cIasNumChannelsPerBundle],
  // which contains for each bundle / each channel the channel-specific parameters.
  mChannelParams = (IasEqualizerCoreChannelParams**)std::calloc(mNumBundles, sizeof(IasEqualizerCoreChannelParams*));
  if (mChannelParams == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
    return eIasAudioProcNotEnoughMemory;
  }

  for (uint32_t cntBundles=0; cntBundles < mNumBundles; cntBundles++)
  {
    mChannelParams[cntBundles] = (IasEqualizerCoreChannelParams*)std::calloc(cIasNumChannelsPerBundle, sizeof(IasEqualizerCoreChannelParams));
    if (mChannelParams == nullptr)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
  }

  // Initialize the mChannelParams[mNumBundles][cIasNumChannelsPerBundle].
  for (uint32_t cntBundles=0; cntBundles < mNumBundles; cntBundles++)
  {
    for (uint32_t cntChannels=0; cntChannels < cIasNumChannelsPerBundle; cntChannels++)
    {
      mChannelParams[cntBundles][cntChannels].numActiveFilterStages = 0;
      mChannelParams[cntBundles][cntChannels].numActiveFilters      = 0;
      mChannelParams[cntBundles][cntChannels].filterParamsTab = (IasAudioFilterParams*)std::calloc(mNumFilterStagesMax, sizeof(IasAudioFilterParams));
      mChannelParams[cntBundles][cntChannels].filterStageIndexTab = (uint32_t*)std::calloc(mNumFilterStagesMax, sizeof(uint32_t));
      if ((mChannelParams[cntBundles][cntChannels].filterParamsTab == nullptr) ||
          (mChannelParams[cntBundles][cntChannels].filterStageIndexTab == nullptr))
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, " not enough memory!");
        return eIasAudioProcNotEnoughMemory;
      }
    }
  }
  // Tell each filter, on which bundle it shall work on.
  uint32_t bundleIndex = 0;
  IasBundlePointerList::const_iterator bundleListIt;
  for (bundleListIt = bundles.begin(); bundleListIt != bundles.end(); ++bundleListIt)
  {
    IasAudioFilter** filters = mFilters[bundleIndex];
    for (uint32_t cntFilterStages = 0; cntFilterStages < mNumFilterStagesMax; cntFilterStages++)
    {
      filters[cntFilterStages]->setBundlePointer((*bundleListIt));
    }
    bundleIndex++;
  }
  // Create a map mStreamStatusMap, which contains the module-internal
  // status variables for each individual stream. Within each map element
  // the key represents the streamId, while the object represents the
  // streamStatus. The streamStatus includes the vector isGainRamping
  // of length mNumFilterStagesMax, which indicates for each individual
  // filter of this stream, whether the gain is currently ramping.

  IasStreamPointerList::const_iterator it_stream;
  for (it_stream = streams.begin(); it_stream != streams.end(); ++it_stream)
  {
    int32_t const streamId = (*it_stream)->getId();

    IasEqualizerCoreStreamStatus streamStatus;
    streamStatus.isGainRamping = (bool*)std::calloc(mNumFilterStagesMax, sizeof(bool));
    if (streamStatus.isGainRamping == nullptr)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, " not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
    for (uint32_t cntFilters=0; cntFilters < mNumFilterStagesMax; cntFilters++)
    {
      streamStatus.isGainRamping[cntFilters] = false;
    }

    mStreamStatusMap.emplace(streamId, streamStatus);
  }

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasEqualizerCore::processChild()
{
  // This is required to convert the streams to the bundled format
  const IasStreamPointerList &streams = mConfig->getStreams();
  IasStreamPointerList::const_iterator streamIt;
  for (streamIt = streams.begin(); streamIt != streams.end(); ++streamIt)
  {
    // We don't need the pointer in this component, but just to trigger the conversion
    (void)(*streamIt)->asBundledStream();
  }
  for (uint32_t cntBundles = 0; cntBundles < mNumBundles; cntBundles++)
  {
    uint32_t numActiveFilterStages = mBundleParams[cntBundles].numActiveFilterStages;
    IasAudioFilter** filters          = mFilters[cntBundles];
    for (uint32_t cntFilterStages = 0; cntFilterStages < numActiveFilterStages; cntFilterStages++)
    {
      filters[cntFilterStages]->calculate();
    }
  }
  return eIasAudioProcOK;
}


IasAudioProcessingResult IasEqualizerCore::rampGainSingleStreamSingleFilter(int32_t streamId,
                                                                            uint32_t   filterId,
                                                                            float  targetGain)
{
  const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
  const auto streamParamsRange = streamParams.equal_range(streamId);
  IasStreamParamsMultimap::const_iterator streamParamsIt;
  int32_t  status = 0;
  // Check whether the streamId is known.
  IasEqualizerCoreStreamStatusMap::const_iterator streamStatusIt = mStreamStatusMap.find(streamId);
  if (streamStatusIt == mStreamStatusMap.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", invalid streamId, streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }
  // Loop over all bundles that contain channels belonging to this stream.
  for (streamParamsIt=streamParamsRange.first; streamParamsIt != streamParamsRange.second; ++streamParamsIt)
  {
    uint32_t const bundleIndex    = streamParamsIt->second->getBundleIndex();
    uint32_t const channelIndex   = streamParamsIt->second->getChannelIndex();
    uint32_t const numberChannels = streamParamsIt->second->getNumberChannels();
    IasAudioFilter** filters   = mFilters[bundleIndex];

    // Loop over all channels within this bundle, which belong to this stream.
    for (uint32_t cntChannels=0; cntChannels < numberChannels; cntChannels++)
    {
      uint32_t const currentChannel = channelIndex+cntChannels;
      // Check that the filterId does not exceed the number of filters that are used for this channel
      if (filterId >= mChannelParams[bundleIndex][currentChannel].numActiveFilters)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", invalid filterId, filterId=", filterId);
        return eIasAudioProcInvalidParam;
      }
      // Identify the index of the associated 2nd order section and the number of 2nd order sections.
      uint32_t const section     =  mChannelParams[bundleIndex][currentChannel].filterStageIndexTab[filterId];
      uint32_t const numSections = (mChannelParams[bundleIndex][currentChannel].filterParamsTab[filterId].order+1)>>1;
      // One filter can consist of several sections (namely, if the filter order is > 2).
      // This is the loop over the filter sections, which all belong to one filter.
      for (uint32_t cntSections = 0; cntSections < numSections; cntSections++)
      {
        uint64_t const callbackUserData = (((uint64_t)streamId)<<32) | ((uint64_t)filterId);
        status |= filters[section+cntSections]->rampGain(currentChannel, targetGain, callbackUserData);
      }
      streamStatusIt->second.isGainRamping[filterId] = true;
    }
  }
  // Check whether one of the filters has reported an error.
  if (status)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error while calling IasAudioFilter::rampGain()");
    return eIasAudioProcInvalidParam;
  }
  return eIasAudioProcOK;
}


IasAudioProcessingResult IasEqualizerCore::setRampGradientSingleStream(int32_t streamId, float  gradient)
{
  const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
  const auto streamParamsRange = streamParams.equal_range(streamId);
  IasStreamParamsMultimap::const_iterator streamParamsIt;
  int32_t status = 0;
  // Check whether the streamId is known.

  const auto streamStatusIt = mStreamStatusMap.find(streamId);
  if (streamStatusIt == mStreamStatusMap.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", invalid streamId, streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  // Loop over all bundles that contain channels belonging to this stream.
  for (streamParamsIt = streamParamsRange.first; streamParamsIt != streamParamsRange.second; ++streamParamsIt)
  {
    uint32_t const bundleIndex    = streamParamsIt->second->getBundleIndex();
    uint32_t const channelIndex   = streamParamsIt->second->getChannelIndex();
    uint32_t const numberChannels = streamParamsIt->second->getNumberChannels();
    IasAudioFilter** filters   = mFilters[bundleIndex];
    // Loop over all channels within this bundle, which belong to this stream.
    for (uint32_t cntChannels=0; cntChannels < numberChannels; cntChannels++)
    {
      // Loop over all filters
      for (uint32_t cntFilters=0; cntFilters < mNumFilterStagesMax; cntFilters++)
      {
        status |= filters[cntFilters]->setRampGradient(channelIndex + cntChannels, gradient);
      }
    }
  }
  if (status)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error while calling IasAudioFilter::setRampGradient()");
    return eIasAudioProcInvalidParam;
  }
  return eIasAudioProcOK;
}


IasAudioProcessingResult IasEqualizerCore::getFilterParamsForChannel(int32_t streamId, uint32_t filterIdx,
                                                                     uint32_t channel, IasAudioFilterParams* params)
{
  if(params == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, NULL pointer passed as parameter");
    return eIasAudioProcInvalidParam;
  }
  bool bChannelFound = false;
  const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
  const auto streamParamsRange = streamParams.equal_range(streamId);
  IasStreamParamsMultimap::const_iterator streamParamsIt;
  if(streamParams.find(streamId) == streamParams.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, streamId not found");
    return eIasAudioProcInvalidParam;
  }
  uint32_t bundleIndex = 0;
  uint32_t channelIndex = 0;
  uint32_t numberChannels = 0;
  uint32_t channelInBundle = 0;
  uint32_t bundleChannels = 0;
  for (streamParamsIt=streamParamsRange.first; streamParamsIt != streamParamsRange.second; ++streamParamsIt)
  {
    bundleIndex    = (*streamParamsIt).second->getBundleIndex();
    channelIndex   = (*streamParamsIt).second->getChannelIndex();
    numberChannels = (*streamParamsIt).second->getNumberChannels();
    if(channelIndex == channel)
    {
      bChannelFound = true;
      channelInBundle = channel;
      break;
    }
    else
    {
      for(uint32_t chan=0; chan<numberChannels; chan++)
      {
        if((bundleChannels + chan) == channel)
        {
          bChannelFound = true;
          channelInBundle = chan;
          break;
        }
      }
    }
    bundleChannels += cIasNumChannelsPerBundle;
  }
  if(bChannelFound == false)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, channel not valid");
    return eIasAudioProcInvalidParam;
  }
  uint32_t const numFilters = mChannelParams[bundleIndex][channelInBundle].numActiveFilters;
  if(filterIdx >= numFilters)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, filter index ",filterIdx,
                " not valid, max number is= ", numFilters);
    return eIasAudioProcInvalidParam;
  }
  *params = mChannelParams[bundleIndex][channelInBundle].filterParamsTab[filterIdx];

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasEqualizerCore::getNumFiltersForChannel(int32_t streamId, uint32_t channel, uint32_t* numFilters)
{
  if(numFilters == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, nullptr passed as parameter");
    return eIasAudioProcInvalidParam;
  }
  const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
  const auto streamParamsRange = streamParams.equal_range(streamId);
  IasStreamParamsMultimap::const_iterator streamParamsIt;
  uint32_t bundleChannels = 0;
  if(streamParams.find(streamId) == streamParams.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, streamId not found");
    return eIasAudioProcInvalidParam;
  }
  for (streamParamsIt=streamParamsRange.first; streamParamsIt != streamParamsRange.second; ++streamParamsIt)
  {
    uint32_t const bundleIndex    = streamParamsIt->second->getBundleIndex();
    uint32_t const channelIndex   = streamParamsIt->second->getChannelIndex();
    uint32_t const numberChannels = streamParamsIt->second->getNumberChannels();
    if(channelIndex == channel)
    {
       *numFilters = mChannelParams[bundleIndex][channelIndex].numActiveFilters;
       return eIasAudioProcOK;
    }
    else
    {
      for(uint32_t chan=0; chan<numberChannels; chan++)
      {
        if((bundleChannels + chan) == channel)
        {
          *numFilters = mChannelParams[bundleIndex][chan].numActiveFilters;
          return eIasAudioProcOK;
        }
      }
    }
    bundleChannels += cIasNumChannelsPerBundle;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ", error, channel: ", channel, " not valid");
  return eIasAudioProcInvalidParam;
}


IasAudioProcessingResult IasEqualizerCore::setFiltersSingleStream(int32_t streamId,std::vector<uint32_t> const &channelIdTable, std::vector<IasAudioFilterParams> const &filterParamsTable)
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, ": setting parameters for stream with streamId=", streamId);
  const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
  const auto streamParamsRange = streamParams.equal_range(streamId);
  IasStreamParamsMultimap::const_iterator streamParamsIt;
  IasAudioFilterParams const flatFilterParams = { 0, 0.0f, 1.0f, eIasFilterTypeFlat, 2, 0 };
  int32_t  status = 0;
  uint32_t cntChannelsOfThisStream = 0;                                      // This counter might span over several bundles.
  uint32_t const numFilters = static_cast<uint32_t>(filterParamsTable.size());  // Number of filters that are part of the cascade.
  // Check whether the streamId is known.
  IasEqualizerCoreStreamStatusMap::const_iterator streamStatusIt = mStreamStatusMap.find(streamId);

  if (streamStatusIt == mStreamStatusMap.end())
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid streamId, streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }
  // Check whether the filter order is correct (for all filters of the cascade).
  for (uint32_t cntFilters=0; cntFilters < numFilters; cntFilters++)
  {
    IasAudioFilterParams const tempFilterParams = filterParamsTable[cntFilters];

    if ((tempFilterParams.order < cIasAudioFilterParamsMinValues.order) ||
        (tempFilterParams.order > cIasAudioFilterParamsMaxValues.order))
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": invalid filter params, order=", tempFilterParams.order);
      return eIasAudioProcInvalidParam;
    }
  }
  // Loop over all bundles that contain channels belonging to this stream.
  for (streamParamsIt=streamParamsRange.first; streamParamsIt != streamParamsRange.second; ++streamParamsIt)
  {
    uint32_t const bundleIndex    = streamParamsIt->second->getBundleIndex();
    uint32_t const channelIndex   = streamParamsIt->second->getChannelIndex();
    uint32_t const numberChannels = streamParamsIt->second->getNumberChannels();
    IasAudioFilter** filters   = mFilters[bundleIndex];
    // Loop over all channels within this bundle, which belong to this stream.
    for (uint32_t cntChannels=0; cntChannels < numberChannels; cntChannels++)
    {
      // The current channel is updated only, if it is listed in channelIdTable,
      // or if channelIdTable is empty (which means that every channel of this
      // stream shall be updated).
      if ((channelIdTable.size() == 0) ||
          (std::find(channelIdTable.begin(), channelIdTable.end(), cntChannelsOfThisStream) != channelIdTable.end()))
      {
        // Loop over all filters that are part of the cascade.
        uint32_t cntFilterStages = 0;
        for (uint32_t cntFilters=0; cntFilters < numFilters; cntFilters++)
        {
          IasAudioFilterParams tempFilterParams = filterParamsTable[cntFilters];

          uint32_t          numSections      = (tempFilterParams.order+1)>>1;
          mChannelParams[bundleIndex][channelIndex+cntChannels].filterStageIndexTab[cntFilters] = cntFilterStages;
          mChannelParams[bundleIndex][channelIndex+cntChannels].filterParamsTab[cntFilters]     = tempFilterParams;
          // One filter can consist of several sections (namely, if the filter order is > 2).
          // This is the loop over the filter sections, which all belong to one filter.
          for (uint32_t cntSections = 0; cntSections < numSections; cntSections++)
          {
            if (cntFilterStages >= mNumFilterStagesMax)
            {
              DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": too many filter stages");
              return eIasAudioProcNoSpaceLeft;
            }
            tempFilterParams.section = cntSections+1;

            status |= filters[cntFilterStages]->setChannelFilter(channelIndex+cntChannels, &tempFilterParams);
            cntFilterStages++;
          }
        }
        mChannelParams[bundleIndex][channelIndex+cntChannels].numActiveFilterStages = cntFilterStages;
        mChannelParams[bundleIndex][channelIndex+cntChannels].numActiveFilters      = numFilters;
        // Loop over all remaining filter stages (inactive stages)
        for (  ; cntFilterStages < mNumFilterStagesMax; cntFilterStages++)
        {
          filters[cntFilterStages]->setChannelFilter(channelIndex+cntChannels, &flatFilterParams);
        }
      }
      cntChannelsOfThisStream++;
    }
  }
  // Now we have to update for each bundle the number of active filter stages.
  for (uint32_t cntBundles = 0; cntBundles < mNumBundles; cntBundles++)
  {
    uint32_t numActiveFilterStages = 0;
    for (uint32_t cntChannels = 0; cntChannels < cIasNumChannelsPerBundle; cntChannels++)
    {
      if (numActiveFilterStages < mChannelParams[cntBundles][cntChannels].numActiveFilterStages)
      {
        numActiveFilterStages = mChannelParams[cntBundles][cntChannels].numActiveFilterStages;
      }
    }
    mBundleParams[cntBundles].numActiveFilterStages = numActiveFilterStages;
  }
  // Check whether one of the filters has reported an error.
  if (status)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": error while calling IasAudioFilter::setChannelFilter()");
    return eIasAudioProcInvalidParam;
  }
  return eIasAudioProcOK;
}


void IasEqualizerCore::gainRampingFinished(uint32_t channel, float gain, uint64_t callbackUserData)
{
  (void) channel;

  const auto sendEvent = [this](int32_t streamId, IasProperties& properties)
  {
    std::string pinName;
    mConfig->getPinName(streamId, pinName);
    properties.set("pin", pinName);
    properties.set<std::string>("typeName",     mTypeName);
    properties.set<std::string>("instanceName", mInstanceName);
    IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
    event->setProperties(properties);
    IasEventProvider::getInstance()->send(event);
    IasEventProvider::getInstance()->destroyModuleEvent(event);
  };

  int32_t  const streamId = (int32_t) (callbackUserData >> 32);
  uint32_t const filterId = (uint32_t) (callbackUserData & 0x00000000FFFFFFFF);

  // Verify that streamId is valid. In practice, the assert condition
  // will never fail, because the streamId has been already verified by
  // the method IasEqualizerCore::rampGainSingleStreamSingleFilter().

  IasEqualizerCoreStreamStatusMap::const_iterator streamStatusIt = mStreamStatusMap.find(streamId);
  if(streamStatusIt == mStreamStatusMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, ": Error, streamId :", streamId, "not found");
    return;
  }

  // Check whether this filter in this stream is currently ramping.
  // If it is not ramping, we can assume that we have already called
  // the callback function (originating from another channel from
  // the same stream).
  if (streamStatusIt->second.isGainRamping[filterId])
  {
    IasProperties properties;
    properties.set<int32_t>("eventType", IasEqualizer::IasEqualizerEventTypes::eIasGainRampingFinished);

    float const factor   = (gain >= 1 ? 0.5f : -0.5f);
    int32_t   const gain_lin = (int32_t)(200.0f * log10(gain) +factor);

    properties.set<int32_t>("gain", gain_lin);
    sendEvent(streamId, properties);

    // Avoid that the callback function is called several times
    // (which can result from the other channels).
    streamStatusIt->second.isGainRamping[filterId] = false;
  }
}



} // namespace IasAudio
