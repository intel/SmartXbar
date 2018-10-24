/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasGenericAudioCompCore.cpp
 * @date  2012
 * @brief This is the implementation of the IasGenericAudioCompCore class.
 */

#include <audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp>
#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "internal/audio/common/IasDataProbeHelper.hpp"

namespace IasAudio {

static const std::string cClassName = "IasGenericAudioCompCore::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasGenericAudioCompCore::IasGenericAudioCompCore(const IasIGenericAudioCompConfig *config, const std::string &componentName)
  :mFrameLength(0)
  ,mSampleRate(0)
  ,mComponentIndex(-1)
  ,mLogContext(IasAudioLogging::getDltContext("PFW"))
  ,mConfig(config)
  ,mProcessingState(eIasAudioCompEnabled)
  ,mComponentName(componentName)
{

}

IasGenericAudioCompCore::~IasGenericAudioCompCore()
{
  mInputDataProbes.clear();
  mOutputDataProbes.clear();
  for (auto &entry : mInputDataProbesAreas)
  {
    delete[] entry.second;
  }
  for (auto &entry : mOutputDataProbesAreas)
  {
    delete[] entry.second;
  }
}

IasAudioProcessingResult IasGenericAudioCompCore::init(IasAudioChainEnvironmentPtr env)
{
  mFrameLength = env->getFrameLength();
  mSampleRate = env->getSampleRate();

  IasStreamPointerList streams =  mConfig->getStreams();

  for (auto &entry : streams)
  {
    mInputDataProbes[entry->getId()] = nullptr;
    mOutputDataProbes[entry->getId()] = nullptr;
    mInputDataProbesAreas[entry->getId()] = new IasAudioArea[entry->getNumberChannels()];
    mOutputDataProbesAreas[entry->getId()] = new IasAudioArea[entry->getNumberChannels()];
  }

  IasStreamMap streamMap = mConfig->getStreamMapping();
  for (auto &entry : streamMap )
  {
    mInputDataProbes[entry.first->getId()] = nullptr;
    mOutputDataProbes[entry.first->getId()] = nullptr;
    if(mInputDataProbesAreas.find(entry.first->getId()) == mInputDataProbesAreas.end())
    {
      mInputDataProbesAreas[entry.first->getId()] = new IasAudioArea[entry.first->getNumberChannels()];
    }
    if(mOutputDataProbesAreas.find(entry.first->getId()) == mOutputDataProbesAreas.end())
    {
      mOutputDataProbesAreas[entry.first->getId()] = new IasAudioArea[entry.first->getNumberChannels()];
    }
    for (auto &li : entry.second)
    {
      mInputDataProbes[li->getId()] = nullptr;
      mOutputDataProbes[li->getId()] = nullptr;
      if(mInputDataProbesAreas.find(li->getId()) == mInputDataProbesAreas.end())
      {
        mInputDataProbesAreas[li->getId()] = new IasAudioArea[li->getNumberChannels()];
      }
      if(mOutputDataProbesAreas.find(li->getId()) == mOutputDataProbesAreas.end())
      {
        mOutputDataProbesAreas[li->getId()] = new IasAudioArea[li->getNumberChannels()];
      }
    }
  }

  // Call the init method of the audio module implementation
  return init();
}

void IasGenericAudioCompCore::enableProcessing()
{
  // First, we have to call our reset() function, which calls the reset() function
  // of the customer module to clear any state variables.
  reset();

  // Now we can enable the module.
  mProcessingState = eIasAudioCompEnabled;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasGenericAudioCompCore::enableProcessing: Component index=", mComponentIndex);
}

void IasGenericAudioCompCore::disableProcessing()
{
  mProcessingState = eIasAudioCompDisabled;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,"Component index=", mComponentIndex);
}

IasAudioProcessingResult IasGenericAudioCompCore::process()
{
  if(eIasAudioCompDisabled == mProcessingState)
  {
    return eIasAudioProcOff;
  }
  IasProbingQueueEntry entry;
  while(mProbingQueue.try_pop(entry))
  {
    int32_t streamId = entry.params.pinParams.streamId;
    std::string tmpName = entry.params.name;
    if(entry.params.pinParams.input)
    {
      if(entry.params.isInject == false)
      {
        entry.params.name = tmpName +"_input";
      }
      else
      {
        entry.params.pinParams.output = false; // when we inject we ONLY inject at input ( before processing) and we do nothing on output
      }
      IasDataProbe::IasResult probeRes = IasDataProbeHelper::processQueueEntry(entry, &(mInputDataProbes[streamId]), mFrameLength);

      if (probeRes != IasDataProbe::eIasOk)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,"Error during ", toString(entry.action)," to probe streamId",streamId, ":", toString(probeRes));
      }
      if(mInputDataProbes[streamId] != nullptr)
      {
        IasAudioStream* stream;
        mConfig->getStreamById(&stream,streamId);
        setupProbingInfo(stream,mInputDataProbesAreas[streamId]);
        mActiveInputDataProbes[streamId] =mInputDataProbes[streamId];
      }
      else
      {
        mActiveInputDataProbes.erase(streamId);
      }
    }

    if(entry.params.pinParams.output)
    {
      if(entry.params.isInject == false)
      {
        entry.params.name = tmpName +"_output";
      }
      IasDataProbe::IasResult probeRes = IasDataProbeHelper::processQueueEntry(entry, &(mOutputDataProbes[streamId]), mFrameLength);

      if (probeRes != IasDataProbe::eIasOk)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,"Error during ", toString(entry.action)," to probe streamId",streamId, ":", toString(probeRes));
      }
      if(mOutputDataProbes[streamId] != nullptr)
      {
        IasAudioStream* stream;
        mConfig->getStreamById(&stream,streamId);
        setupProbingInfo(stream,mOutputDataProbesAreas[streamId]);
        mActiveOutputDataProbes[streamId] = mOutputDataProbes[streamId];
      }
      else
      {
        mActiveOutputDataProbes.erase(streamId);
      }
    }

  }

  IasGenericCoreProbeMap::iterator it;
  for ( it = mActiveInputDataProbes.begin(); it != mActiveInputDataProbes.end(); )
  {
    IasAudioStream* stream;
    mConfig->getStreamById(&stream,it->first);
    stream->asBundledStream();
    it->second->process(mInputDataProbesAreas[it->first],0 ,mFrameLength);
    if (it->second->isStarted() == false)
    {
      if ( mInputDataProbes.find(it->first) != mInputDataProbes.end() )
      {
        mInputDataProbes[it->first] = nullptr;
      }
      it = mActiveInputDataProbes.erase(it);
    }
    else
    {
      it++;
    }
  }
  IasAudioProcessingResult res = processChild();
  if (res != eIasAudioProcOK)
  {
    return res;
  }

  for ( it = mActiveOutputDataProbes.begin(); it != mActiveOutputDataProbes.end();)
  {
    IasAudioStream* stream;
    mConfig->getStreamById(&stream,it->first);
    stream->asBundledStream();
    it->second->process(mOutputDataProbesAreas[it->first],0 ,mFrameLength);
    if (it->second->isStarted() == false)
    {
      if ( mOutputDataProbes.find(it->first) != mOutputDataProbes.end() )
      {
        mOutputDataProbes[it->first] = nullptr;
      }
      it=mActiveOutputDataProbes.erase(it);
    }
    else
    {
      it++;
    }
  }

  return res;
}

void IasGenericAudioCompCore::setupProbingInfo(IasAudioStream* stream, IasAudioArea* area)
{
  if(stream == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR,LOG_PREFIX,": nullptr");
    return;
  }

  stream->asBundledStream();
  IasAudioFrame audioFrame;
  audioFrame.resize(stream->getNumberChannels());
  stream->getAudioDataPointers(audioFrame);

  for(uint32_t i=0; i< audioFrame.size(); i++)
  {
    area[i].start = (void*)(audioFrame[i]);
    area[i].step = 128; // bundled format: 32 bits per sample, 4 channels in a bundle -> step is 128
    area[i].first = 0;
    area[i].index = i;
    area[i].maxIndex = static_cast<uint32_t>(audioFrame.size()-1);
  }
}

IasAudioProcessingResult IasGenericAudioCompCore::stopProbe(int32_t streamId,
                                                            bool input,
                                                            bool output)
{
  IasAudioStream* stream = nullptr;
  mConfig->getStreamById(&stream, streamId);

  if(stream == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,"no stream found for streamId",streamId);
    return eIasAudioProcOK;
  }

  IasProbingQueueEntry entry;
  entry.action = eIasProbingStop;
  IasPinProbingParams pinParams;
  pinParams.streamId = streamId;
  pinParams.input = input;
  pinParams.output = output;
  entry.params.pinParams = pinParams;
  mProbingQueue.push(entry);
  
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasGenericAudioCompCore::startProbe(const std::string& fileNamePrefix,
                                                             bool bInject,
                                                             uint32_t numSeconds,
                                                             uint32_t streamId,
                                                             bool input,
                                                             bool output)
{
  IasAudioStream* stream = nullptr;
  mConfig->getStreamById(&stream, streamId);
 
  if(stream == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasGenericAudioCompCore::startProbe: no stream found for streamId",streamId);
    return eIasAudioProcInvalidStream;
  }

  IasPinProbingParams pinParams;
  pinParams.streamId = streamId;
  pinParams.input = input;
  pinParams.output = output;

  IasProbingQueueEntry entry;
  entry.params.name = fileNamePrefix;
  entry.params.duration = numSeconds;
  entry.params.isInject = bInject;
  entry.params.numChannels = stream->getNumberChannels();
  entry.params.startIndex = 0;
  entry.params.sampleRate = mSampleRate;
  entry.params.dataFormat = eIasFormatFloat32;
  entry.action = eIasProbingStart;
  entry.params.pinParams = pinParams;

  mProbingQueue.push(entry);

  return eIasAudioProcOK;
}

} // namespace IasAudio
