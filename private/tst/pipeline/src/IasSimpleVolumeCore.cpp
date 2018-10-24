/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSimpleVolumeCore.cpp
 * @date   2016
 * @brief  Simple Volume Audio Prcessing Component, used for IasPipelineTest.
 *
 * This component does not implement the actual streaming and processing, because
 * we do not have an audio data provider in the IasPipelineTest. The streaming
 * and processing is tested by the IasRoutingZoneTest instead.
 */

#include <iostream>
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "IasSimpleVolumeCore.hpp"

namespace IasAudio {

IasSimpleVolumeCore::IasSimpleVolumeCore(const IasIGenericAudioCompConfig *config, std::string componentName)
  :IasGenericAudioCompCore(config, componentName)
  ,mVolume(1.0f)
{
}

IasSimpleVolumeCore::~IasSimpleVolumeCore()
{
}

IasAudioProcessingResult IasSimpleVolumeCore::reset()
{
  mVolume = 1.0f;
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasSimpleVolumeCore::init()
{
  mVolume = 1.0f;
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
  }

  // Iterate over all streams we have to process in-place.
  const IasStreamPointerList& streams = mConfig->getStreams();
  for (IasStreamPointerList::const_iterator streamIt = streams.begin(); streamIt != streams.end(); ++streamIt)
  {
    std::cout << "  in-place processing on stream " << (*streamIt)->getName() << std::endl;
    // This is the place where the actual streaming and processing would take place.
  }

  // Iterate over all stream mappings.
  const IasStreamMap& streamMap = mConfig->getStreamMapping();
  for (IasStreamMap::const_iterator mapIt = streamMap.begin(); mapIt != streamMap.end(); ++mapIt)
  {
    IasAudioStream* outputStream = mapIt->first;
    const IasStreamPointerList& inputStreamPointerList = mapIt->second;
    for (IasAudioStream* inputStream :inputStreamPointerList)
    {
      std::cout << "  processing stream mapping from input stream " << inputStream->getName()
                << " to output stream " << outputStream->getName() << std::endl;
      // This is the place where the actual streaming and processing would take place.
    }
  }

  return eIasAudioProcOK;
}

} // namespace IasAudio
