/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSimpleVolumeCore.cpp
 * @date   2016
 * @brief  Simple Volume Audio Prcessing Component, used for IasRoutingZoneTest.
 */

#include <iostream>

#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "IasSimpleVolumeCore.hpp"

namespace IasAudio {

IasSimpleVolumeCore::IasSimpleVolumeCore(const IasIGenericAudioCompConfig *config, std::string componentName)
  :IasGenericAudioCompCore(config, componentName)
  ,mVolume(0.90f)
{
}

IasSimpleVolumeCore::~IasSimpleVolumeCore()
{
}

IasAudioProcessingResult IasSimpleVolumeCore::reset()
{
  mVolume = 0.90f;
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasSimpleVolumeCore::init()
{
  mVolume = 0.90f;
  return eIasAudioProcOK;
}

void IasSimpleVolumeCore::setVolume(float newVolume)
{
  mVolumeMsgQueue.push(newVolume);
}

IasAudioProcessingResult IasSimpleVolumeCore::processChild()
{
  std::cout << "IasTestCompCore::processChild of component " << getComponentName() << std::endl;

  float volume = 1.0f;
  while (mVolumeMsgQueue.try_pop(volume))
  {
    mVolume = volume;
    this->sendSimpleVolumeEvent(volume);
  }

  // Iterate over all streams we have to process in-place.
  const IasStreamPointerList& streams = mConfig->getStreams();
  for (IasStreamPointerList::const_iterator streamIt = streams.begin(); streamIt != streams.end(); ++streamIt)
  {
    std::cout << "  in-place processing on stream " << (*streamIt)->getName() << std::endl;

    IasSimpleAudioStream* nonInterleaved = (*streamIt)->asNonInterleavedStream();
    (void)nonInterleaved;
    IAS_ASSERT(nonInterleaved != nullptr);
    const IasAudioFrame& buffers = nonInterleaved->getAudioBuffers();

    for (uint32_t chanIdx = 0; chanIdx < nonInterleaved->getNumberChannels(); ++chanIdx)
    {
      float *channel = buffers[chanIdx];
      IAS_ASSERT(channel != nullptr);
      for (uint32_t frameIdx = 0; frameIdx < mFrameLength; ++frameIdx)
      {
        *channel = *channel * mVolume;
        channel++;
      }
    }
  }

  // Iterate over all stream mappings.
  const IasStreamMap& streamMap = mConfig->getStreamMapping();
  for (IasStreamMap::const_iterator mapIt = streamMap.begin(); mapIt != streamMap.end(); ++mapIt)
  {
    IasAudioStream* outputStream = mapIt->first;
    IasSimpleAudioStream *nonInterleavedOutput = outputStream->asNonInterleavedStream();
    const IasAudioFrame &outputBuffers = nonInterleavedOutput->getAudioBuffers();

    // Iterate over all input streams that are mapped to this output stream.
    const IasStreamPointerList& inputStreamPointerList = mapIt->second;
    float inPlaceGain = 0.0f; // gain is 0.0 during the first input stream, 1.0 during all other input streams
    for (IasAudioStream* inputStream :inputStreamPointerList)
    {
      std::cout << "  processing stream mapping from input stream " << inputStream->getName()
                << " to output stream " << outputStream->getName() << std::endl;

      IasSimpleAudioStream *nonInterleavedInput = inputStream->asNonInterleavedStream();
      const IasAudioFrame &inputBuffers = nonInterleavedInput->getAudioBuffers();
      for (uint32_t chanIdx = 0; chanIdx < nonInterleavedInput->getNumberChannels(); ++chanIdx)
      {
        float *inputChannel  = inputBuffers[chanIdx];
        float *outputChannel = outputBuffers[chanIdx];
        for (uint32_t frameIdx = 0; frameIdx < mFrameLength; ++frameIdx)
        {
          *outputChannel = inPlaceGain * *outputChannel + *inputChannel * mVolume;
          (void)inPlaceGain;
          inputChannel++;
          outputChannel++;
        }
      }
      inPlaceGain = 1.0f;
    }

  }

  // To simulate some heavy CPU load, sleep some time before we return from this function.
  // The sleep period that we use here, corresponds to a CPU load of 10 %.
  usleep(1000000 * mFrameLength / mSampleRate / 10);

  return eIasAudioProcOK;
}


void IasSimpleVolumeCore::sendSimpleVolumeEvent(float volume)
{
  IasProperties properties;
  properties.set<float>("volume", volume);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
}


} // namespace IasAudio
