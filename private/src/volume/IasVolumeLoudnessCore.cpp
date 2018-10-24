/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasVolumeLoudnessCore.cpp
 * @date 2012
 * @brief the core implementation of the volume module
 */

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "volume/IasVolumeHelper.hpp"
#include "filter/IasAudioFilter.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "rtprocessingfwx/IasStreamParams.hpp"
 // #define IAS_ASSERT()
#include <malloc.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <sstream>

namespace IasAudio {

static const std::string cClassName = "IasVolumeLoudnessCore::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

static const uint32_t cIasAudioMaxLoudnessTableLength = 20;
static const float cVolumeCutOffLinear = static_cast<float>(6.309573445e-08);

IasVolumeLoudnessCore::IasVolumeLoudnessCore(const IasIGenericAudioCompConfig *config, const std::string &componentName)
  :IasGenericAudioCompCore(config, componentName)
  ,mNumStreams(0)
  ,mNumFilterBands(0)
  ,mNumFilterBandsMax(3)
  ,mLoudnessTableLengthMax(cIasAudioMaxLoudnessTableLength)
  ,mSDVTableLength(0)
  ,mSDVTableLengthMax(cIasAudioMaxSDVTableLength)
  ,mNumBundles(0)
  ,mCurrentSpeed(0)
  ,mNewSpeed(0)
  ,mSpeedChanged(false)
  ,mSDV_HysteresisState(eSpeedInc_HysOff)
  ,mFilters(nullptr)
  ,mBandFilters()
  ,mFilterMap()
  ,mNumFilters(0)
  ,mVolumeParamsMap()
  ,mGains()
  ,mGainsSDV()
  ,mGainsMute()
  ,mStreams()
  ,mBundleIndexMap()
  ,mLoudnessTableVector()
  ,mCallbackMap()
  ,mSDVTable()
  ,mMaxVolume(0.0f)
  ,mVolumeSDV_Critical(1.0f)
  ,mLoudnessFilterParams()
  ,mVolumeQueue()
  ,mMuteQueue()
  ,mLoudnessStateQueue()
  ,mSpeedQueue()
  ,mSDVStateQueue()
  ,mSDVTableQueue()
  ,mLoudnessTableQueue()
  ,mLoudnessFilterQueue()
  ,mTypeName("")
  ,mInstanceName("")
  ,mLogContext(IasAudioLogging::getDltContext("_VOL"))
  ,mStreamFilterMap()
{
}

IasVolumeLoudnessCore::~IasVolumeLoudnessCore()
{
  for (uint32_t i=0; i< mGains.size();++i)
  {
    free(mGains[i]);
  }
  mGains.clear();
  for (uint32_t i=0; i< mGainsSDV.size();++i)
  {
    free(mGainsSDV[i]);
  }
  mGainsSDV.clear();
  for (uint32_t i=0; i< mGainsMute.size();++i)
   {
     free(mGainsMute[i]);
   }
  mGainsMute.clear();
  for(uint32_t i=0; i<(mNumFilters);++i)
  {
    delete mFilters[i];
  }
  free(mFilters);
  IasVolumeRampMap::iterator it;
  for(it = mVolumeParamsMap.begin(); it != mVolumeParamsMap.end(); ++it )
  {
    for(uint32_t i=0;i < it->second.mVolumeRampParamsVector.size();i++)
    {
      delete(it->second.mVolumeRampParamsVector[i].rampVol);
      delete(it->second.mVolumeRampParamsVector[i].rampSDV);
    }
  }
  mVolumeParamsMap.clear();
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}

IasAudioProcessingResult IasVolumeLoudnessCore::setMuteState(int32_t Id,
                                                             bool  isMuted,
                                                             uint32_t rampTime,
                                                             IasAudio::IasRampShapes rampShape)
{
  IasVolumeRampMap::iterator it_map;
  if(Id < 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Invalid sinkId: Id=", Id);
    return eIasAudioProcInvalidParam;
  }
  IasVolumeCallbackMap::iterator callbackIt = mCallbackMap.find(Id);
  if (callbackIt == mCallbackMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "unable to find sinkId: Id=", Id);
    return eIasAudioProcInvalidParam;
  }

  it_map = mVolumeParamsMap.find(Id);
  if(it_map == mVolumeParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "unable to find sinkId: Id=", Id);
    return eIasAudioProcInvalidParam;
  }

  IasMuteQueueEntry queueEntry;
  queueEntry.mute = isMuted;
  queueEntry.rampShape = rampShape;
  queueEntry.rampTime = rampTime;
  queueEntry.streamId = Id;
  mMuteQueue.push(queueEntry);

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasVolumeLoudnessCore::setVolume(int32_t   Id,
                                                          float volume,
                                                          int32_t   rampTime,
                                                          IasAudio::IasRampShapes rampShape)
{
  IasVolumeRampMap::iterator it_map;
  IasVolumeRampMap::iterator it_map_all;
  if ((Id < 0) || (volume < 0.0f) || (volume > 1.0f) || (rampTime < 0) || rampShape == eIasRampShapeLogarithm )
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid parameter(s):",
                    "sinkId=", Id, "volume=", volume, "rampTime=", rampTime, "rampShape=", toString(rampShape));
    return eIasAudioProcInvalidParam;
  }
  it_map = mVolumeParamsMap.find(Id);
  if(it_map == mVolumeParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "unable to find sinkId: Id=", Id);
    return eIasAudioProcInvalidParam;
  }

  IasVolumeQueueEntry queueEntry;
  queueEntry.volume = volume;
  queueEntry.rampShape = rampShape;
  queueEntry.rampTime = rampTime;
  queueEntry.streamId = Id;
  mVolumeQueue.push(queueEntry);

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasVolumeLoudnessCore::init()
{
  const IasBundlePointerList& bundles = mConfig->getBundles();
  const IasStreamPointerList& streams = mConfig->getStreams();
  const IasBundleAssignmentVector bundleVector;
  IasStreamPointerList::const_iterator it_stream;

  // Get the configuration properties.
  const IasProperties &properties = mConfig->getProperties();

  // Retrieve typeName and instanceName from the configuration properties.
  IasProperties::IasResult result;
  result = properties.get<std::string>("typeName", &mTypeName);
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

  int32_t numFilterBands;
  properties.get("numFilterBands", &numFilterBands);
  if (numFilterBands == 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Number of filter stages may not be 0");
    return eIasAudioProcInvalidParam;
  }
  mNumFilterBands = static_cast<uint32_t>(numFilterBands);
  mNumStreams = static_cast<uint32_t>(streams.size());
  mNumBundles = static_cast<uint32_t>(bundles.size());
  mNumFilters = mNumBundles * mNumFilterBands;

  // allocate memory for all the filter objects. One filter object contains four biquads,
  // so it can process the data of one complete bundle.
  mFilters = (IasAudioFilter**) malloc( mNumFilters * sizeof(IasAudioFilter*) );
  if(mFilters == NULL)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
    return eIasAudioProcNotEnoughMemory;
  }
  IasAudioFilterConfigParams dummyInitLoudnessFilterParams;
  dummyInitLoudnessFilterParams.freq = 0;
  dummyInitLoudnessFilterParams.gain = 0.0f;
  dummyInitLoudnessFilterParams.order = 0;
  dummyInitLoudnessFilterParams.quality = 0.0f;
  dummyInitLoudnessFilterParams.type = eIasFilterTypeFlat;

  for(uint32_t i=0; i<mNumFilterBands;++i)
  {
    mBandFilters.push_back(mFilters+i*bundles.size());
    mLoudnessFilterParams.push_back(dummyInitLoudnessFilterParams);
  }
  IasAudioFilter** tempFilter = mFilters;
  for(uint32_t i=0; i<(mNumFilters);++i)
  {
    tempFilter[i]= new IasAudioFilter(mSampleRate,mFrameLength);
    if(tempFilter[i] == NULL)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
    if(tempFilter[i]->init())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error while calling IasAudioFilter::init()!");
      return eIasAudioProcNotEnoughMemory;
    }
  }

  // allocate memory for the bundles
  uint32_t bundleIndex = 0;
  IasBundlePointerList::const_iterator bundleListIt;
  for(bundleListIt = bundles.begin(); bundleListIt != bundles.end(); ++bundleListIt)
  {
    float* temp = (float*)memalign(16 /*aligned to 4*32 bit*/,
                                                 mFrameLength * sizeof(float) * cIasNumChannelsPerBundle);
    if(temp == NULL)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
    // Preset gains to 1.0. Gains for processed streams
    // will be set to 0.0f afterwards.
    for(uint32_t j=0; j<(mFrameLength*cIasNumChannelsPerBundle);j++)
    {
      temp[j] = 1.0f;
    }
    mGains.push_back(temp);

    float* tempSDV = (float*)memalign(16 /*aligned to 4*32 bit*/,
                                                    mFrameLength * sizeof(float) * cIasNumChannelsPerBundle);
    if(tempSDV == NULL)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
    for(uint32_t j=0; j<(mFrameLength*cIasNumChannelsPerBundle);j++)
    {
      tempSDV[j] = 1.0f;
    }
    mGainsSDV.push_back(tempSDV);

    float* tempMute = (float*)memalign(16 /*aligned to 4*32 bit*/,
                                                    mFrameLength * sizeof(float) * cIasNumChannelsPerBundle);
    if(tempMute == NULL)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }
    for(uint32_t j=0; j<(mFrameLength*cIasNumChannelsPerBundle);j++)
    {
      tempMute[j] = 1.0f;
    }
    mGainsMute.push_back(tempMute);

    IasBundleIndexPair tempPair( (*bundleListIt),bundleIndex);
    mBundleIndexMap.insert(tempPair);
    for(uint32_t j=0; j<mBandFilters.size();++j)
    {
      (mBandFilters[j])[bundleIndex]->setBundlePointer((*bundleListIt));
    }
    bundleIndex++;
  }
  uint32_t streamCounter = 0;

  // create volumeParams
  for(it_stream = streams.begin(); it_stream!=streams.end(); ++it_stream)
  {
    const IasBundleAssignmentVector &bundleAssignments = (*it_stream)->asBundledStream()->getBundleAssignments();
    for (uint32_t index=0; index<bundleAssignments.size(); ++index)
    {
      uint32_t chanIdx = bundleAssignments[index].getIndex();
      uint32_t numberChannels = bundleAssignments[index].getNumberChannels();
      while (numberChannels > 0)
      {
        IasBundleIndexMap::iterator mapIt = mBundleIndexMap.find(bundleAssignments[index].getBundle());
        if (mapIt != mBundleIndexMap.end())
        {
          bundleIndex = (*mapIt).second;
          for(uint32_t j=0; j<(mFrameLength*cIasNumChannelsPerBundle);j+=4)
          {
            mGains[bundleIndex][j+chanIdx] = 0.0f;
          }
          numberChannels--;
          chanIdx++;
        }
      }
    }
    mStreams.push_back(*it_stream);

    // do all the things for the volume ramping and filtering
    uint32_t streamId = (*it_stream)->getId();
    IasVolumeParams volumeParams;

    for(uint32_t associatedBundle = 0; associatedBundle < bundleAssignments.size(); associatedBundle++)
    {
      IasVolumeRampParams rampParams;
      IasBundleIndexMap::iterator mapIt = mBundleIndexMap.find(bundleAssignments[associatedBundle].getBundle());

      rampParams.bundleIndex = (*mapIt).second;
      rampParams.channelIndex =bundleAssignments[associatedBundle].getIndex();
      rampParams.nChannels = bundleAssignments[associatedBundle].getNumberChannels();
      rampParams.rampVol = new IasRamp(mSampleRate, mFrameLength);
      if(rampParams.rampVol == NULL)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
        return eIasAudioProcNotEnoughMemory;
      }
      rampParams.rampSDV = new IasRamp(mSampleRate, mFrameLength);
      if(rampParams.rampSDV == NULL)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
        delete(rampParams.rampVol);
        return eIasAudioProcNotEnoughMemory;
      }
      rampParams.rampMute = new IasRamp(mSampleRate, mFrameLength);
      if(rampParams.rampMute == NULL)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "not enough memory!");
        delete(rampParams.rampVol);
        delete(rampParams.rampSDV);
        return eIasAudioProcNotEnoughMemory;
      }
      volumeParams.mVolumeRampParamsVector.push_back(rampParams);
    }
    volumeParams.currentVolume = 0.0f;
    volumeParams.currentSDVGain = 1.0f;
    volumeParams.currentMuteGain = 1.0f;
    volumeParams.muteVolume = 0.0f;
    volumeParams.destinationVolume = 0.0f;
    volumeParams.numSamplesLeftToRamp = 0;
    volumeParams.numSDVSamplesLeftToRamp = 0;
    volumeParams.numMuteSamplesLeftToRamp = 0;
    volumeParams.isMuted = false;
    volumeParams.volRampActive = false;
    volumeParams.sdvRampActive = false;
    volumeParams.muteRampActive = false;
    volumeParams.isSDVon = false;
    volumeParams.lastRunVol = false;
    volumeParams.lastRunSDV = false;
    volumeParams.lastRunMute = false;
    volumeParams.activeLoudness = true;

    std::vector<uint32_t> filterBands;

    for(uint32_t i=0; i< mNumFilterBands; i++)
    {
      if(checkStreamActiveForBand(streamId, i))
      {
        volumeParams.filters.push_back(mBandFilters[i]);
      }
    }

    mVolumeParamsMap.insert( std::pair<uint32_t,IasVolumeParams>(streamId,volumeParams) );

    IasVolumeCallbackParams callbackParams;
    callbackParams.streamId = streamId;
    callbackParams.markerVolume = false;
    callbackParams.markerLoudness = false;
    callbackParams.markerMute = false;
    callbackParams.loudnessState = false;
    callbackParams.muteState = false;
    callbackParams.volume = 0.0f;
    callbackParams.markerSDV = false;
    callbackParams.stateSDV = false;
    mCallbackMap.insert(std::pair<int32_t,IasVolumeCallbackParams>(streamId,callbackParams));

    streamCounter++;
  }
  for(uint32_t i=0;i<mNumFilterBands;++i)
  {
    IasVolumeLoudnessTable loudnessTable;
    mLoudnessTableVector.push_back(loudnessTable);
  }

  return initFromConfiguration();
}

void IasVolumeLoudnessCore::getStreamActiveForFilterBand(uint32_t band, IasInt32Vector &streamIds)
{
  const IasProperties &properties = mConfig->getProperties();
  IasStringVector activePinsForBand;
  std::string activePinsPrefix = "activePinsForBand.";
  std::stringstream apfp;
  apfp << activePinsPrefix << std::to_string(band);
  properties.get(apfp.str(), &activePinsForBand);
  for (auto pinName: activePinsForBand)
  {
    int32_t streamId;
    IasAudioProcessingResult result = mConfig->getStreamId(pinName, streamId);
    if (result == eIasAudioProcOK)
    {
      streamIds.push_back(streamId);
    }
  }
}

IasAudioProcessingResult IasVolumeLoudnessCore::initFromConfiguration()
{
  using namespace IasAudio::IasVolumeHelper;

  const IasProperties &properties = mConfig->getProperties();
  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasVolumeLoudnessTable loudnessTable;
    bool found = getLoudnessTableFromProperties(properties, i, loudnessTable);
    if (found == false)
    {
      getDefaultLoudnessTable(loudnessTable);
    }
    setLoudnessTable(i, &loudnessTable, true);
  }

  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasInt32Vector streamIds;
    getStreamActiveForFilterBand(i, streamIds);
    for (auto streamId: streamIds)
    {
      setStreamActiveForFilterband(i, streamId);
    }
  }

  IasVolumeSDVTable sdvTable;
  bool found = getSDVTableFromProperties(properties, sdvTable);
  if (found == false)
  {
    getDefaultSDVTable(sdvTable);
  }
  mSDVTableLength = sdvTable.speed.size();
  setSDVTable(&sdvTable, true);

  for (uint32_t i = 0; i < mNumFilterBands; i++)
  {
    IasAudioFilterConfigParams filterParams;
    bool found = getLoudnessFilterParamsFromProperties(properties, i, filterParams);
    if (found == false)
    {
      getDefaultLoudnessFilterParams(i, mNumFilterBands, filterParams);
    }
    else
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                  "loudness filter params for band", i,
                  ": freq =",  filterParams.freq,
                  "order =",   filterParams.order,
                  "quality =", filterParams.quality,
                  "type =",    toString(filterParams.type));
      if (checkLoudnessFilterParams(filterParams))
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid loudness filter params");
        return eIasAudioProcInvalidParam;
      }
    }
    setLoudnessFilterAllStreams(i, &filterParams, true);
  }

  return eIasAudioProcOK;
}



IasAudioProcessingResult IasVolumeLoudnessCore::reset()
{
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasVolumeLoudnessCore::processChild()
{
  std::pair<IasStreamParamsMultimap::const_iterator,IasStreamParamsMultimap::const_iterator>  paramIt;

  checkQueues();

  for(uint32_t numStream = 0; numStream < mStreams.size();numStream++)
  {
    // Just to trigger the conversion
    (void)mStreams[numStream]->asBundledStream();
    int32_t streamId = mStreams[numStream]->getId();
    IasVolumeRampMap::iterator it_map = mVolumeParamsMap.find(streamId);
    if (it_map != mVolumeParamsMap.end())
    {
      if (it_map->second.volRampActive)
      {
        for(uint32_t i=0; i < it_map->second.mVolumeRampParamsVector.size(); i++)
        {
          uint32_t gainBundleIndex = it_map->second.mVolumeRampParamsVector[i].bundleIndex;
          uint32_t chanIndex = it_map->second.mVolumeRampParamsVector[i].channelIndex;
          uint32_t nChannels = it_map->second.mVolumeRampParamsVector[i].nChannels;;
          it_map->second.mVolumeRampParamsVector[i].rampVol->getRampValues(mGains[gainBundleIndex],chanIndex,nChannels,&(it_map->second.numSamplesLeftToRamp));
          updateLoudness(streamId,mGains[gainBundleIndex][0+chanIndex]);
        }
        if(it_map->second.lastRunVol == true)
        {
          it_map->second.volRampActive = false;
          it_map->second.lastRunVol = false;
          IasVolumeCallbackMap::iterator it = mCallbackMap.find(streamId);
          if (it != mCallbackMap.end())
          {
            // Send volume fade finished event
            float currentVolume = mGains[it_map->second.mVolumeRampParamsVector[0].bundleIndex][0+it_map->second.mVolumeRampParamsVector[0].channelIndex];
            sendVolumeEvent(streamId, currentVolume);
          }
        }
        if(it_map->second.numSamplesLeftToRamp == 0 && it_map->second.volRampActive)
        {
          it_map->second.lastRunVol = true;
        }
        it_map->second.mVolumeRampParamsVector[0].rampVol->getCurrentValue(&(it_map->second.currentVolume));
      }
      if (it_map->second.muteRampActive)
      {

        for(uint32_t i=0; i < it_map->second.mVolumeRampParamsVector.size(); i++)
        {
          uint32_t gainBundleIndex = it_map->second.mVolumeRampParamsVector[i].bundleIndex;
          uint32_t chanIndex = it_map->second.mVolumeRampParamsVector[i].channelIndex;
          uint32_t nChannels = it_map->second.mVolumeRampParamsVector[i].nChannels;;
          it_map->second.mVolumeRampParamsVector[i].rampMute->getRampValues(mGainsMute[gainBundleIndex],chanIndex,nChannels,&(it_map->second.numMuteSamplesLeftToRamp));
        }
        if(it_map->second.lastRunMute == true)
        {
          it_map->second.muteRampActive = false;
          it_map->second.lastRunMute = false;
          IasVolumeCallbackMap::iterator it = mCallbackMap.find(streamId);
          if (it != mCallbackMap.end())
          {
            it->second.markerMute = true;
          }
        }
        if(it_map->second.numMuteSamplesLeftToRamp == 0 && it_map->second.muteRampActive)
        {
          it_map->second.lastRunMute = true;
        }
        it_map->second.mVolumeRampParamsVector[0].rampMute->getCurrentValue(&(it_map->second.currentMuteGain));
      }
      if(it_map->second.sdvRampActive)
      {
        for(uint32_t i=0; i < it_map->second.mVolumeRampParamsVector.size(); i++)
        {
          uint32_t gainBundleIndex = it_map->second.mVolumeRampParamsVector[i].bundleIndex;
          uint32_t chanIndex = it_map->second.mVolumeRampParamsVector[i].channelIndex;
          uint32_t nChannels = it_map->second.mVolumeRampParamsVector[i].nChannels;;
          it_map->second.mVolumeRampParamsVector[i].rampSDV->getRampValues(mGainsSDV[gainBundleIndex],chanIndex,nChannels,&(it_map->second.numSDVSamplesLeftToRamp));
        }

        if(it_map->second.lastRunSDV == true)
        {
          it_map->second.sdvRampActive = false;
          it_map->second.lastRunSDV = false;
        }
        if(it_map->second.numSDVSamplesLeftToRamp == 0 && it_map->second.sdvRampActive)
        {
          it_map->second.lastRunSDV = true;
        }
        it_map->second.mVolumeRampParamsVector[0].rampSDV->getCurrentValue(&(it_map->second.currentSDVGain));
      }
    }
  }

  const IasSamplePointerList &audioFrame = mConfig->getAudioFrame();
  IasSamplePointerList::const_iterator audioIt;
  uint32_t i=0;
  for (audioIt=audioFrame.begin(); audioIt!=audioFrame.end(); ++audioIt)
  {
    float *data = *audioIt;
    float *gains = mGains[i];
    float *gainsSDV = mGainsSDV[i];
    float *gainsMute = mGainsMute[i];
    for(uint32_t j=0;j<cIasNumChannelsPerBundle*mFrameLength;j++)
    {
      *data = *data * (*gains) * (*gainsSDV) *(*gainsMute);
      data++;
      gains++;
      gainsSDV++;
      gainsMute++;
    }
    i++;
  }

  //calculate filters
  for(uint32_t j=0;j<(mNumFilterBands*mNumBundles);++j)
  {
    mFilters[j]->calculate();
  }

  for(IasVolumeCallbackMap::iterator it=mCallbackMap.begin();it != mCallbackMap.end(); it++)
  {
    if(it->second.markerVolume)
    {
      it->second.markerVolume = false;
      sendVolumeEvent(it->first, it->second.volume);
    }
    if(it->second.markerLoudness)
    {
      it->second.markerLoudness = false;
      sendLoudnessStateEvent(it->first, it->second.loudnessState);
    }
    if(it->second.markerMute)
    {
      it->second.markerMute = false;
      sendMuteStateEvent(it->first, it->second.muteState);
    }
    if(it->second.markerSDV)
    {
      it->second.markerSDV = false;
      sendSpeedControlledVolumeEvent(it->first, it->second.stateSDV);
    }
  }

  return eIasAudioProcOK;
}

void IasVolumeLoudnessCore::sendVolumeEvent(int32_t streamId, float currentVolume)
{
  int32_t volumeInt;
  if(currentVolume > cVolumeCutOffLinear)
  {
    // convert the volume from the linear format to the logarithmic format of [dB/10]
    // so calculate the formular (200*log10(volume))*10
    // the subration of 0.5 is for the correct rounding
    volumeInt = (int32_t)(200.0f*log10(currentVolume) -0.5f);
  }
  else
  {
    //set volume to -1440 [dB/10], which is the minimum value
    volumeInt = -1440;
  }


  IasProperties properties;
  properties.set<int32_t>("eventType", IasVolume::IasVolumeEventTypes::eIasVolumeFadingFinished);
  properties.set<std::string>("typeName",     mTypeName);
  properties.set<std::string>("instanceName", mInstanceName);
  std::string pinName;
  mConfig->getPinName(streamId, pinName);
  properties.set<std::string>("pin", pinName);
  properties.set<int32_t>("volume", volumeInt);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
  IasEventProvider::getInstance()->destroyModuleEvent(event);
}

void IasVolumeLoudnessCore::sendMuteStateEvent(int32_t streamId, bool muteState)
{
  IasProperties properties;
  properties.set<int32_t>("eventType", IasVolume::IasVolumeEventTypes::eIasSetMuteStateFinished);
  properties.set<std::string>("typeName",     mTypeName);
  properties.set<std::string>("instanceName", mInstanceName);
  std::string pinName;
  mConfig->getPinName(streamId, pinName);
  properties.set("pin", pinName);
  properties.set<int32_t>("muteState", muteState);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
  IasEventProvider::getInstance()->destroyModuleEvent(event);
}

void IasVolumeLoudnessCore::sendLoudnessStateEvent(int32_t streamId, bool loudnessState)
{
  IasProperties properties;
  properties.set<int32_t>("eventType", IasVolume::IasVolumeEventTypes::eIasLoudnessSwitchFinished);
  properties.set<std::string>("typeName",     mTypeName);
  properties.set<std::string>("instanceName", mInstanceName);
  std::string pinName;
  mConfig->getPinName(streamId, pinName);
  properties.set("pin", pinName);
  properties.set<int32_t>("loudnessState", loudnessState);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
  IasEventProvider::getInstance()->destroyModuleEvent(event);
}

void IasVolumeLoudnessCore::sendSpeedControlledVolumeEvent(int32_t streamId, bool sdvState)
{
  IasProperties properties;
  properties.set<int32_t>("eventType", IasVolume::IasVolumeEventTypes::eIasSpeedControlledVolumeFinished);
  properties.set<std::string>("typeName",     mTypeName);
  properties.set<std::string>("instanceName", mInstanceName);
  std::string pinName;
  mConfig->getPinName(streamId, pinName);
  properties.set("pin", pinName);
  properties.set<int32_t>("sdvState", sdvState);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
  IasEventProvider::getInstance()->destroyModuleEvent(event);
}

IasAudioProcessingResult IasVolumeLoudnessCore::setLoudnessFilter(int32_t streamId, uint32_t band, IasAudioFilterParams *params)
{
  const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
  auto streamParamsRange = streamParams.equal_range(streamId);
  IasStreamParamsMultimap::const_iterator streamParamsIt;
  IasVolumeRampMap::iterator it_map;
  bool bFound = checkStreamActiveForBand(streamId, band);

  if(bFound == true)
  {
    it_map = mVolumeParamsMap.find(streamId);
    if(it_map == mVolumeParamsMap.end())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "unable to find sinkId for loudness gain update: Id=", streamId);
      return eIasAudioProcInvalidParam;
    }
    // get gain for filter depending on current volume and update filter params with gain
    float gain = getLoudnessGain(it_map->second.currentVolume,&mLoudnessTableVector[band]);
    params->gain = gain;

    for (streamParamsIt=streamParamsRange.first; streamParamsIt!=streamParamsRange.second; ++streamParamsIt)
    {
      uint32_t bundleIndex = (*streamParamsIt).second->getBundleIndex();
      uint32_t channelIndex = (*streamParamsIt).second->getChannelIndex();
      uint32_t numberChannels = (*streamParamsIt).second->getNumberChannels();
      IasAudioFilter** filters = mBandFilters[band];
      for(uint32_t j=0; j<numberChannels; ++j)
      {
        filters[bundleIndex]->setChannelFilter(j+channelIndex, params);
      }
    }
    it_map = mVolumeParamsMap.find(streamId);

    if(!it_map->second.volRampActive && !it_map->second.muteRampActive)
    {
      updateLoudness(streamId, it_map->second.currentVolume);
    }
    else
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "no loudness gain update due to active ramp");
    }
  }

  return eIasAudioProcOK;
}

IasAudioProcessingResult IasVolumeLoudnessCore::setLoudnessFilterAllStreams(uint32_t band,
                                                                            IasAudio::IasAudioFilterConfigParams *filterParams,
                                                                            bool directInit)
{
  if(band >= mNumFilterBands || filterParams->order > 2)
  {
    return eIasAudioProcInvalidParam;
  }

  if (directInit == true)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "directly setting loudness filter for all streams for band", band);
    updateLoudnessFilter(band, *filterParams);
  }
  else
  {
    IasLoudnessFilterQueueEntry queueEntry;

    queueEntry.band = band;
    queueEntry.params.freq = filterParams->freq;
    queueEntry.params.gain = 1.0f;
    queueEntry.params.order = filterParams->order;
    queueEntry.params.quality = filterParams->quality;
    queueEntry.params.type = filterParams->type;

    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "setting loudness filter for all streams for band", band);
    mLoudnessFilterQueue.push(queueEntry);
  }

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasVolumeLoudnessCore::getLoudnessFilterAllStreams(uint32_t band,
                                                                            IasAudio::IasAudioFilterConfigParams &filterParams)
{
  if(band >= mNumFilterBands)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "band not valid: ", band);
    return eIasAudioProcInvalidParam;
  }
  filterParams = mLoudnessFilterParams[band];
  return eIasAudioProcOK;
}


IasAudioProcessingResult IasVolumeLoudnessCore::setLoudnessOnOff(int32_t streamId,
                                                                 bool  isOn)
{
  IasVolumeRampMap::iterator it_map = mVolumeParamsMap.find(streamId);
  if(it_map == mVolumeParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "unable to find sink Id=", streamId);
    return eIasAudioProcInvalidParam;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                  "setting loudness for sink Id=", streamId, "new state=", isOn);
  IasLoudnessStateQueueEntry queueEntry;
  queueEntry.streamId = streamId;
  queueEntry.loudnessActive = isOn;

  mLoudnessStateQueue.push(queueEntry);

  return eIasAudioProcOK;
}



IasAudioProcessingResult IasVolumeLoudnessCore::setSpeedControlledVolume(int32_t streamId, bool mode)
{
  if (mSDVTableLength == 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "SDV Table not set");
    return eIasAudioProcNotInitialization;
  }
  IasVolumeRampMap::iterator it_map = mVolumeParamsMap.find(streamId);

  IasSDVStateQueueEntry queueEntry;
  if (it_map == mVolumeParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "unable to find sink Id=", streamId);
    return eIasAudioProcInvalidParam;
  }

  queueEntry.sdvActive = mode;
  queueEntry.streamId = streamId;
  mSDVStateQueue.push(queueEntry);

  return eIasAudioProcOK;

}

float IasVolumeLoudnessCore::getLoudnessGain(float volume, IasVolumeLoudnessTable* table )
{
  uint32_t cnt = 1;
  int32_t volume_dB = 0;
  float gain;

  if( volume <= 6.309573445e-08f) // -144 dB = 6.309573445e-08f in linear
  {
    volume_dB = -1440;
  }
  else
  {
    volume_dB = static_cast<int32_t>(20.0f*log10f(volume)*10.0f);
  }
  if(volume_dB >= table->volumes[0])
  {
    gain =  powf(10.0f, static_cast<float>(table->gains[0])/200.0f);
    return gain;
  }

  do
  {
    if(volume_dB >= table->volumes[cnt])
    {
      break;
    }
    cnt++;
  } while(cnt<table->volumes.size());


  if(cnt == table->volumes.size())
  {
    // this formula calculates 10^((gain/20)/10) to have the gain in the logarithmic format of [dB/10]
    gain =  powf(10.0f, static_cast<float>(table->gains[cnt-1])/200.0f);
    return gain;
  }
  else
  {
    float deltaVolume = static_cast<float>(table->volumes[cnt-1] - table->volumes[cnt]);
    float deltaCoeff  = static_cast<float>(table->volumes[cnt-1] - volume_dB) / deltaVolume;
    float deltaGain   = static_cast<float>(table->gains[cnt] - table->gains[cnt-1]);
    float gain_dB     = static_cast<float>(table->gains[cnt-1]) + deltaGain*deltaCoeff;
    return (powf(10.0f, gain_dB/200.0f));
  }
}

void IasVolumeLoudnessCore::updateLoudness(int32_t streamId, float volume)
{
  IasVolumeRampMap::iterator it_map = mVolumeParamsMap.find(streamId);
  if (it_map != mVolumeParamsMap.end())
  {
    if ((*it_map).second.activeLoudness)
    {
      const IasStreamParamsMultimap &streamParams = mConfig->getStreamParams();
      auto streamParamsRange = streamParams.equal_range(streamId);
      IasStreamParamsMultimap::const_iterator streamParamsIt;
      for (uint32_t i=0; i<mBandFilters.size(); ++i)
      {
        if(checkStreamActiveForBand(streamId,i))
        {
          float gain = getLoudnessGain(volume, &mLoudnessTableVector[i]);
          IasAudioFilter **filter = mBandFilters[i];
          for (streamParamsIt=streamParamsRange.first; streamParamsIt!=streamParamsRange.second; ++streamParamsIt)
          {
            uint32_t bundleIndex = (*streamParamsIt).second->getBundleIndex();
            uint32_t channelIndex = (*streamParamsIt).second->getChannelIndex();
            uint32_t numberChannels = (*streamParamsIt).second->getNumberChannels();

            for (uint32_t k=0; k<numberChannels; ++k)
            {
              if(filter[bundleIndex]->updateGain(k+channelIndex, gain))
              {
                DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Error when updating filter gain for loudness");
              }
            }
          }
        }
      }
    }
  }
}


IasAudioProcessingResult IasVolumeLoudnessCore::setLoudnessTable(uint32_t band,
                                                                 IasVolumeLoudnessTable *loudnessTable,
                                                                 bool directInit)
{
  if((uint32_t)band >= mNumFilterBands)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid band=",
                    band, ", must be smaller than", mNumFilterBands);
    return eIasAudioProcInvalidParam;
  }
  if(loudnessTable->gains.size() != loudnessTable->volumes.size())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid loudness table,",
                    "number of gain parameters:",   loudnessTable->gains.size(),
                    "number of volume parameters:", loudnessTable->volumes.size());
    return eIasAudioProcInvalidParam;
  }
  if(loudnessTable->gains.size() > mLoudnessTableLengthMax || loudnessTable->gains.size() == 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid loudness table,",
                    "number of gain parameters:", loudnessTable->gains.size(),
                    ", must not be 0 and not bigger than", mLoudnessTableLengthMax);
    return eIasAudioProcInvalidParam;
  }

  if (directInit == true)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                "directly setting loudness table with", loudnessTable->gains.size(),
                "parameters for band=", band);
    updateLoudnessTable(band, *loudnessTable);
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                "setting loudness table with", loudnessTable->gains.size(),
                "parameters for band=", band);
    IasLoudnessTableQueueEntry queueEntry;
    queueEntry.band = band;
    queueEntry.table = *loudnessTable;
    mLoudnessTableQueue.push(queueEntry);
  }

  return eIasAudioProcOK;
}

IasAudioProcessingResult IasVolumeLoudnessCore::getLoudnessTable(uint32_t band, IasVolumeLoudnessTable &loudnessTable)
{
  if(band >= mNumFilterBands)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                "invalid band=", band,
                ", must be smaller than mNumFilterBands=", mNumFilterBands);
    return eIasAudioProcInvalidParam;
  }
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
              "getting loudness table with", mLoudnessTableVector[band].volumes.size(),
              "parameters for band=", band);

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
              "size of loudness table:",mLoudnessTableVector[band].gains.size());

  loudnessTable = mLoudnessTableVector[band];

  return eIasAudioProcOK;
}

float IasVolumeLoudnessCore::calculateSDV_Gain(uint32_t speed,IasVolumeParams *params)
{
  float gain=1.0f;
  std::vector<uint32_t> *gainVec;

  if(speed > mCurrentSpeed && mSDV_HysteresisState == eSpeedDec_HysOff)
  {
    mSDV_HysteresisState = eSpeedInc_HysOn;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New state: eSpeedInc_HysOn!");
  }
  else if(speed < mCurrentSpeed && mSDV_HysteresisState == eSpeedInc_HysOff)
  {
    mSDV_HysteresisState = eSpeedDec_HysOn;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New state: eSpeedDec_HysOn!");
  }
  else if(speed > mCurrentSpeed && mSDV_HysteresisState == eSpeedDec_HysOn)
  {
    mSDV_HysteresisState = eSpeedInc_HysOn;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New state: eSpeedInc_HysOn!");
  }
  else if(speed < mCurrentSpeed && mSDV_HysteresisState == eSpeedInc_HysOn)
  {
    mSDV_HysteresisState = eSpeedDec_HysOn;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New state: eSpeedDec_HysOn!");
  }

  switch(mSDV_HysteresisState)
  {
    case eSpeedInc_HysOff:
      gainVec = &(mSDVTable.gain_inc);
      gain = getSDV_Gain(speed,gainVec);
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "sdv gain from table:",gain);
      if(mMaxVolume > mVolumeSDV_Critical )
      {
        IAS_ASSERT(mVolumeSDV_Critical != 1.0f);
        // we have to take care not to divide by zero when we calculate log10(mVolumeSDV_Critical)
        // this term would be zero if mVolumeSDV_Critical is equal to 1.0f
        // if this would be the case, then the if condition would not match, because mMaxVolume can not be bigger than 1.0f
        float logCrit = static_cast<float>(log10(mVolumeSDV_Critical));
        float logMax = static_cast<float>(log10(mMaxVolume));
        float logGain = static_cast<float>(log10(gain));
        gain = static_cast<float>(powf(10.0f,(logMax*logGain/logCrit)));
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Gain was reduced to",gain );
      }
      break;

    case eSpeedDec_HysOn:
      gainVec = &(mSDVTable.gain_dec);
      gain = getSDV_Gain(speed,gainVec);
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "sdv gain from table:",gain);
      if(mMaxVolume > mVolumeSDV_Critical )
      {
        IAS_ASSERT(mVolumeSDV_Critical != 1.0f);
        // we have to take care not to divide by zero when we calculate log10(mVolumeSDV_Critical)
        // this term would be zero if mVolumeSDV_Critical is equal to 1.0f
        // if this would be the case, then the if condition would not match, because mMaxVolume can not be bigger than 1.0f
        float logCrit = static_cast<float>(log10(mVolumeSDV_Critical));
        float logMax = static_cast<float>(log10(mMaxVolume));
        float logGain = static_cast<float>(log10(gain));
        gain = static_cast<float>(powf(10.0f,(logMax*logGain/logCrit)));
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Gain was reduced to",gain );
      }
      if(gain <= params->currentSDVGain )
      {
        mSDV_HysteresisState = eSpeedDec_HysOff;
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New state: eSpeedDec_HysOff");
      }
      else
      {
        gain = params->currentSDVGain;
      }
      break;

    case eSpeedDec_HysOff:
      gainVec = &(mSDVTable.gain_dec);
      gain = getSDV_Gain(speed,gainVec);
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "sdv gain from table:",gain);
      if(mMaxVolume > mVolumeSDV_Critical )
      {
        IAS_ASSERT(mVolumeSDV_Critical != 1.0f);
        // we have to take care not to divide by zero when we calculate log10(mVolumeSDV_Critical)
        // this term would be zero if mVolumeSDV_Critical is equal to 1.0f
        // if this would be the case, then the if condition would not match, because mMaxVolume can not be bigger than 1.0f
        float logCrit = static_cast<float>(log10(mVolumeSDV_Critical));
        float logMax = static_cast<float>(log10(mMaxVolume));
        float logGain = static_cast<float>(log10(gain));
        gain = static_cast<float>(powf(10.0f,(logMax*logGain/logCrit)));
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Gain was reduced to",gain );
      }
      break;

    case eSpeedInc_HysOn:
      gainVec = &(mSDVTable.gain_inc);
      gain = getSDV_Gain(speed,gainVec);
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "sdv gain from table:",gain);
      if(mMaxVolume > mVolumeSDV_Critical  )
      {
        IAS_ASSERT(mVolumeSDV_Critical != 1.0f);
        // we have to take care not to divide by zero when we calculate log10(mVolumeSDV_Critical)
        // this term would be zero if mVolumeSDV_Critical is equal to 1.0f
        // if this would be the case, then the if condition would not match, because mMaxVolume can not be bigger than 1.0f
        float logCrit = static_cast<float>(log10(mVolumeSDV_Critical));
        float logMax = static_cast<float>(log10(mMaxVolume));
        float logGain = static_cast<float>(log10(gain));
        gain = static_cast<float>(powf(10.0f,(logMax*logGain/logCrit)));
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Gain was reduced to",gain );
      }
      if(gain >= params->currentSDVGain)
      {
        mSDV_HysteresisState = eSpeedInc_HysOff;
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New state: eSpeedInc_HysOff");
      }
      else
      {
        gain = params->currentSDVGain;
      }
      break;
  }
  mCurrentSpeed = speed;
  return (gain);
}

float IasVolumeLoudnessCore::getSDV_Gain(uint32_t speed, std::vector<uint32_t> *gainVec)
{
  float gain = 0.0f;
  int32_t cnt = static_cast<int32_t>(mSDVTableLength-1);

  if (speed <= mSDVTable.speed[0])
  {
    return 1.0f;
  }
  else
  {
    do
    {
      if (speed >= mSDVTable.speed[cnt])
      {
        break;
      }
      cnt--;
    }
    while(cnt>=0);

    if((speed == mSDVTable.speed[cnt]) || ( cnt == (int32_t)(mSDVTableLength-1) ))
    {
      gain = (float)((*gainVec)[cnt]);
    }
    else
    {
      float deltaSpeed = static_cast<float>(mSDVTable.speed[cnt+1] - mSDVTable.speed[cnt]);
      float deltaGain  = static_cast<float>((*gainVec)[cnt+1] - (*gainVec)[cnt]);
      float deltaCoeff = deltaGain  / deltaSpeed;
      gain = static_cast<float>((*gainVec)[cnt]) + deltaCoeff* static_cast<float>(speed-mSDVTable.speed[cnt]);
    }
  }
  return (powf(10.0f, gain/200.0f));
}

void IasVolumeLoudnessCore::setSpeed(uint32_t speed)
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "setting new speed to", speed);
  mSpeedQueue.push(speed);

}

IasAudioProcessingResult IasVolumeLoudnessCore::setSDVTable(IasVolumeSDVTable* table, bool directInit)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  uint32_t i = 0;
  if(table == NULL)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error: 'table' parameter is NULL!");
    return eIasAudioProcInvalidParam;
  }
  else if((table->speed.size() > cIasAudioMaxSDVTableLength) || (table->speed.size() == 0))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid SDV table,",
                    "number of speed parameters:", table->speed.size(),
                    ", must not be 0 and not bigger than", cIasAudioMaxSDVTableLength);
    return eIasAudioProcInvalidParam;
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "setting SDV table with", table->speed.size(), "parameters");

    for(i=0; i<table->speed.size();i++)
    {
      if(table->gain_inc[i] > table->gain_dec[i])
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                                  "increasing table entry must be less or equal to decreasing table entry");
        result =  eIasAudioProcInvalidParam;
        break;
      }
      if(i!=0)
      {
        // Check whether the table values are monotonically increasing.
        if( (table->speed[i] <= table->speed[i-1])      ||
            (table->gain_inc[i] < table->gain_inc[i-1]) ||
            (table->gain_dec[i] < table->gain_dec[i-1]))
        {
          DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "table values are not monotonically increasing");
          result =  eIasAudioProcInvalidParam;
          break;
        }
      }
    }
  }
  if (eIasAudioProcOK == result)
  {
    if (directInit == true)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "directly setting sdv table of size", table->gain_dec.size());
      updateSDVTable(*table);
    }
    else
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "setting sdv table of size", table->gain_dec.size());
      mSDVTableQueue.push(*table);
    }
  }
  return result;
}

IasAudioProcessingResult IasVolumeLoudnessCore::getSDVTable(IasVolumeSDVTable &sdvTable)
{
  if(mSDVTable.speed.size() == 0 ||
     mSDVTable.speed.size() != mSDVTableLength)
  {
    return eIasAudioProcNotInitialization;
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "getting sdv table with ",mSDVTableLength ," parameters");

  sdvTable = mSDVTable;
  return eIasAudioProcOK;
}

void IasVolumeLoudnessCore::updateMute(int32_t streamId, bool mute, uint32_t rampTime, IasRampShapes rampShape)
{
  IasVolumeRampMap::iterator it_map;
  it_map = mVolumeParamsMap.find(streamId);
  IAS_ASSERT(it_map != mVolumeParamsMap.end());
  IasVolumeCallbackMap::iterator callbackIt = mCallbackMap.find(streamId);
  IAS_ASSERT(callbackIt != mCallbackMap.end());
  it_map->second.isMuted = mute;
  for(uint32_t i=0; i< it_map->second.mVolumeRampParamsVector.size();i++)
  {
    IasRamp* ramp = it_map->second.mVolumeRampParamsVector[i].rampMute;

    if(mute)
    {
        ramp->setTimedRamp(it_map->second.currentMuteGain,0.0f,(uint32_t)rampTime, rampShape);
    }
    else
    {
        ramp->setTimedRamp(it_map->second.currentMuteGain,1.0f,(uint32_t)rampTime, rampShape);
    }
  }
  it_map->second.lastRunMute = false;
  it_map->second.muteRampActive = true;
  callbackIt->second.muteState = mute;

}

void IasVolumeLoudnessCore::updateVolume( int32_t streamId, float volume, uint32_t rampTime, IasRampShapes rampShape)
{
  IasVolumeRampMap::iterator it_map;
  IasVolumeRampMap::iterator it_map_all;
  float sdvGain;
  it_map = mVolumeParamsMap.find(streamId);
  IAS_ASSERT(it_map != mVolumeParamsMap.end())
  it_map->second.destinationVolume = volume;
  float mMaxVolumeOld = mMaxVolume;
  mMaxVolume= 0.0f;
  for(it_map_all = mVolumeParamsMap.begin();it_map_all!=mVolumeParamsMap.end();it_map_all++)
  {
    if(it_map_all->second.destinationVolume > mMaxVolume)
    {
      mMaxVolume = it_map_all->second.destinationVolume;
    }
  }
  if(mSDVTable.speed.size()!=0) //Only try to calculate gain if a table was set
  {
    sdvGain = calculateSDV_Gain(mCurrentSpeed,&(it_map->second));
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "SDV gain:",sdvGain);
  }
  else
  {
    sdvGain = 1.0f;
  }
  if((mMaxVolume != mMaxVolumeOld) && (mMaxVolume > mMaxVolumeOld))
  {
    for(it_map_all = mVolumeParamsMap.begin();it_map_all!=mVolumeParamsMap.end();it_map_all++)
    {
      if(it_map_all->second.isSDVon == true)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "ramping SDV gain from %f to %f\n",it_map_all->second.currentSDVGain, sdvGain);
        for(uint32_t i=0; i< it_map_all->second.mVolumeRampParamsVector.size();i++)
        {
          IasRamp* ramp = it_map_all->second.mVolumeRampParamsVector[i].rampSDV;
          ramp->setTimedRamp(it_map_all->second.currentSDVGain,sdvGain,(uint32_t)rampTime, rampShape);
        }
        it_map_all->second.sdvRampActive = true;
        it_map_all->second.lastRunSDV = false;
      }
    }
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                  "ramp volume from",it_map->second.currentVolume, "to", volume);
  for(uint32_t i=0; i< it_map->second.mVolumeRampParamsVector.size();i++)
  {
    IasRamp* ramp = it_map->second.mVolumeRampParamsVector[i].rampVol;
    float startVolume = it_map->second.currentVolume;
    ramp->setTimedRamp(startVolume,volume,(uint32_t)rampTime, rampShape);
  }
  it_map->second.destinationVolume = volume;
  it_map->second.muteVolume = volume;
  it_map->second.lastRunVol = false;
  if(it_map->second.volRampActive)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                    "last ramp still ongoing, interrupt it and start new one, sinkId=", streamId);
  }
  it_map->second.volRampActive = true;
}

void IasVolumeLoudnessCore::updateLoudnessParams(int32_t streamId, bool active)
{
  IasVolumeRampMap::iterator it_map;
  const IasStreamParamsMultimap& streamParams = mConfig->getStreamParams();
  it_map = mVolumeParamsMap.find(streamId);
  IAS_ASSERT(it_map != mVolumeParamsMap.end())

  if(active != it_map->second.activeLoudness)
  {
    it_map->second.activeLoudness = active;
    if(it_map->second.activeLoudness)
    {
      //get current volume and update filters
      float currentVolume = it_map->second.currentVolume;
      updateLoudness(streamId,currentVolume);
    }
    else
    {
      auto streamParamsRange = streamParams.equal_range(streamId);
      IasStreamParamsMultimap::const_iterator streamParamsIt;
      for (uint32_t k=0; k<mBandFilters.size(); ++k)
      {
        IasAudioFilter **filters = mBandFilters[k];
        for (streamParamsIt=streamParamsRange.first; streamParamsIt!=streamParamsRange.second; ++streamParamsIt)
        {
          uint32_t bundleIndex = (*streamParamsIt).second->getBundleIndex();
          uint32_t channelIndex = (*streamParamsIt).second->getChannelIndex();
          uint32_t numberChannels = (*streamParamsIt).second->getNumberChannels();
          for (uint32_t j=0; j<numberChannels; ++j)
          {
            //flat the filter by setting the gain to 1.0
            filters[bundleIndex]->updateGain(j+channelIndex,1.0f);
          }
        }
      }
    }
  }
  IasVolumeCallbackMap::iterator it = mCallbackMap.find(streamId);
  if (it != mCallbackMap.end())
  {
    it->second.loudnessState = it_map->second.activeLoudness;
    sendLoudnessStateEvent(it_map->first, it->second.loudnessState);
  }
}

void IasVolumeLoudnessCore::updateSDV_State(int32_t streamId, bool active)
{
  IasVolumeRampMap::iterator it_map = mVolumeParamsMap.find(streamId);
  float gain = 1.0f;
  if (it_map != mVolumeParamsMap.end())
  {
    if( (bool)active != it_map->second.isSDVon )
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                      "setting speed controlled volume for sink Id=", streamId, "new mode=", active);

      it_map->second.isSDVon = (bool)active;
      if(it_map->second.isSDVon)
      {
        gain = calculateSDV_Gain(mNewSpeed, &(it_map->second));
      }
      else
      {
        //when SDV is turned off we "reset" it as if we start from 0 km/h when it reactivated
        mSDV_HysteresisState = eSpeedInc_HysOff;
        mCurrentSpeed = 0;
        gain = 1.0f;
      }
      for(uint32_t i=0; i < it_map->second.mVolumeRampParamsVector.size(); i++)
      {
        it_map->second.mVolumeRampParamsVector[i].rampSDV->setTimedRamp(it_map->second.currentSDVGain,gain,1000,eIasRampShapeLinear);
      }
      it_map->second.sdvRampActive = true;
      it_map->second.lastRunSDV = false;
    }

    IasVolumeCallbackMap::iterator it = mCallbackMap.find(streamId);
    if (it != mCallbackMap.end())
    {
      it->second.markerSDV = true;
      it->second.stateSDV = it_map->second.isSDVon;
    }
  }
}

void IasVolumeLoudnessCore::updateSpeed(uint32_t speed)
{
  IasVolumeRampMap::iterator it_map;
  float gain = 1.0f;
  if(mCurrentSpeed != speed)
  {
    mNewSpeed = speed;
    for(it_map=mVolumeParamsMap.begin();it_map!= mVolumeParamsMap.end();it_map++)
    {
      if(it_map->second.isSDVon)
      {
        gain = calculateSDV_Gain(mNewSpeed, &(it_map->second));
        DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "ramping sdvGain from" ,it_map->second.currentSDVGain,
                        "to",gain);
        for(uint32_t i=0; i < it_map->second.mVolumeRampParamsVector.size(); i++)
        {
          it_map->second.mVolumeRampParamsVector[i].rampSDV->setTimedRamp(it_map->second.currentSDVGain,gain,1000,eIasRampShapeLinear);
        }
        it_map->second.lastRunSDV = false;
        it_map->second.sdvRampActive = true;
      }
    }
  }
}

void IasVolumeLoudnessCore::updateSDVTable(IasVolumeSDVTable table)
{
  uint32_t i = 0;
  mSDVTable.speed.resize(0);
  mSDVTable.gain_inc.resize(0);
  mSDVTable.gain_dec.resize(0);
  for(i=0; i<table.speed.size();i++)
  {
    mSDVTable.speed.push_back(table.speed[i]);
    mSDVTable.gain_inc.push_back(table.gain_inc[i]);
    mSDVTable.gain_dec.push_back(table.gain_dec[i]);
  }
  mSDVTableLength = i;
  mVolumeSDV_Critical = 1.0f/(powf(10,(float)(table.gain_inc[mSDVTableLength-1])/200.0f));
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
              "Critical volume when the SDV gain has to be reduced = ",mVolumeSDV_Critical);

}

void IasVolumeLoudnessCore::updateLoudnessTable(uint32_t band, IasVolumeLoudnessTable table)
{
  IasVolumeLoudnessTable* currentTable = &(mLoudnessTableVector[band]);
  currentTable->gains.resize(0);
  currentTable->volumes.resize(0);

  for(uint32_t i=0;i< static_cast<uint32_t>(table.gains.size());i++)
  {
    currentTable->gains.push_back(table.gains[i]);
    currentTable->volumes.push_back(table.volumes[i]);
  }
}

void IasVolumeLoudnessCore::updateLoudnessFilter(uint32_t band, IasAudioFilterConfigParams params)
{
  IasAudioFilterParams tempParams;
  IasStreamPointerList::const_iterator streamIt;

  tempParams.freq = params.freq;
  tempParams.gain = params.gain;
  tempParams.order = params.order;
  tempParams.quality = params.quality;
  tempParams.type = params.type;
  tempParams.section = 1;

  for(streamIt=mConfig->getStreams().begin(); streamIt!=mConfig->getStreams().end(); ++streamIt)
  {
    setLoudnessFilter((*streamIt)->getId(), (int32_t)band, &tempParams);
  }
  mLoudnessFilterParams[band] = params;

}

void IasVolumeLoudnessCore::updateMinVol(std::uint32_t minVol)
{
  mMinVol = minVol;
}

void IasVolumeLoudnessCore::updateMaxVol(std::uint32_t maxVol)
{
  mMaxVol = maxVol;
}

std::int32_t IasVolumeLoudnessCore::getMinVol()
{
  return mMinVol;
}

std::int32_t IasVolumeLoudnessCore::getMaxVol()
{
  return mMaxVol;
}

void IasVolumeLoudnessCore::checkQueues()
{

  IasMuteQueueEntry muteQueueEntry;
  muteQueueEntry.mute = false;
  muteQueueEntry.rampShape = eIasRampShapeLinear;
  muteQueueEntry.rampTime = 0;
  muteQueueEntry.streamId = 0;
  while(mMuteQueue.try_pop(muteQueueEntry))
  {
    updateMute( muteQueueEntry.streamId, muteQueueEntry.mute, muteQueueEntry.rampTime, muteQueueEntry.rampShape);
  }

  IasVolumeQueueEntry volumeQueueEntry;
  volumeQueueEntry.volume = 0.0f;
  volumeQueueEntry.rampShape = eIasRampShapeLinear;
  volumeQueueEntry.rampTime = 0;
  volumeQueueEntry.streamId = 0;
  while(mVolumeQueue.try_pop(volumeQueueEntry))
  {
    updateVolume( volumeQueueEntry.streamId, volumeQueueEntry.volume, volumeQueueEntry.rampTime, volumeQueueEntry.rampShape);
  }

  IasLoudnessStateQueueEntry loudnessStateQueueEntry;
  loudnessStateQueueEntry.loudnessActive = false;
  loudnessStateQueueEntry.streamId = 0;
  while(mLoudnessStateQueue.try_pop(loudnessStateQueueEntry))
   {
    updateLoudnessParams(loudnessStateQueueEntry.streamId, loudnessStateQueueEntry.loudnessActive);
   }

  IasSDVStateQueueEntry sdvStateQueueEntry;
  sdvStateQueueEntry.sdvActive = false;
  sdvStateQueueEntry.streamId = 0;
  while(mSDVStateQueue.try_pop(sdvStateQueueEntry))
  {
    updateSDV_State(sdvStateQueueEntry.streamId,sdvStateQueueEntry.sdvActive);
  }

  uint32_t speed = 0;
  while(mSpeedQueue.try_pop(speed))
  {
    updateSpeed(speed);
  }

  IasVolumeSDVTable table;
  while(mSDVTableQueue.try_pop(table))
  {
    updateSDVTable(table);
  }

  IasLoudnessTableQueueEntry loudnessTableQueueEntry;
  while(mLoudnessTableQueue.try_pop(loudnessTableQueueEntry))
  {
    updateLoudnessTable(loudnessTableQueueEntry.band,loudnessTableQueueEntry.table);
  }

  IasLoudnessFilterQueueEntry loudnessFilterQueueEntry;
  while(mLoudnessFilterQueue.try_pop(loudnessFilterQueueEntry))
  {
    updateLoudnessFilter(loudnessFilterQueueEntry.band, loudnessFilterQueueEntry.params);
  }

}

bool IasVolumeLoudnessCore::checkStreamActiveForBand(int32_t streamId, uint32_t band) const
{
  auto it_pair = mStreamFilterMap.equal_range(band);
  IasVolumeFilterAssignmentMap::const_iterator loop_it;

  for(loop_it = it_pair.first; loop_it != it_pair.second; ++loop_it)
  {
    if(loop_it->second == streamId)
    {
      return true;
    }
  }

  return false;
}

IasAudioProcessingResult IasVolumeLoudnessCore::setStreamActiveForFilterband(uint32_t band, int32_t streamId)
{
  if (band >= mNumFilterBands)
  {
    return eIasAudioProcInvalidParam;
  }

  bool bFound = false;
  for (auto stream: mStreams)
  {
    if (stream->getId() == streamId)
    {
      bFound = true;
      break;
    }
  }
  if (bFound == false)
  {
    return eIasAudioProcInvalidStream; //streamId not listed in active streams
  }
  auto it_pair = mStreamFilterMap.equal_range(band);
  if(it_pair.first == it_pair.second)
  {
    //filterband is not yet present, first insertion of a pair with this key
    auto tempPair = std::make_pair(band, streamId);
    mStreamFilterMap.insert(tempPair);
  }
  else
  {
    bFound = false;
    IasVolumeFilterAssignmentMap::const_iterator loop_it;
    for(loop_it = it_pair.first; loop_it != it_pair.second; ++loop_it)
    {
      if(loop_it->second == streamId)
      {
        bFound = true;
        break;
      }
    }
    if(bFound == false)
    {
      auto tempPair = std::make_pair(band, streamId);
      mStreamFilterMap.insert(tempPair);
    }
  }
  return eIasAudioProcOK;
}




} // namespace IasAudio
