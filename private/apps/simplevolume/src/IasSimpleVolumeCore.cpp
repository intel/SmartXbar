/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSimpleVolumeCore.cpp
 * @date   2013
 * @brief
 */

#include <audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp>
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "audio/simplevolume/IasSimpleVolumeCore.hpp"

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
  const IasStreamPointerList& streams = mConfig->getStreams();
  IasStreamPointerList::const_iterator streamIt;
  float volume = 1.0f;
  while (mVolumeMsgQueue.try_pop(volume))
  {
    mVolume = volume;
  }

  // Iterate over all streams we have to process
  for (streamIt = streams.begin(); streamIt != streams.end(); ++streamIt)
  {
    IasSimpleAudioStream *nonInterleaved = (*streamIt)->asNonInterleavedStream();
    const IasAudioFrame &buffers = nonInterleaved->getAudioBuffers();
    for (uint32_t chanIdx = 0; chanIdx < nonInterleaved->getNumberChannels(); ++chanIdx)
    {
      float *channel = buffers[chanIdx];
      for (uint32_t frameIdx = 0; frameIdx < mFrameLength; ++frameIdx)
      {
        *channel = *channel * mVolume;
        channel++;
      }
    }
  }
  return eIasAudioProcOK;
}

} // namespace IasAudio
