/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSimpleAudioStream.cpp
 * @date   2013
 * @brief  The definition of the IasSimpleAudioStream class.
 */

#include <string.h>


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "rtprocessingfwx/IasAudioBuffer.hpp"
#include "rtprocessingfwx/IasAudioBufferPool.hpp"
#include "rtprocessingfwx/IasAudioBufferPoolHandler.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"

namespace IasAudio {

IasSimpleAudioStream::IasSimpleAudioStream()
  :IasBaseAudioStream("", -1, 0, eIasAudioStreamInput, false)
  ,mAudioFrame()
  ,mFrameLength(0)
  ,mBuffer(NULL)
{
}

IasSimpleAudioStream::~IasSimpleAudioStream()
{
  cleanup();
}


IasAudioProcessingResult IasSimpleAudioStream::setProperties(const std::string &name,
                                                              int32_t id,
                                                              uint32_t numberChannels,
                                                              IasSampleLayout sampleLayout,
                                                              uint32_t frameLength,
                                                              IasAudioStreamTypes type,
                                                              bool sidAvailable)
{
  // Before setting new properties clean-up current settings
  cleanup();

  // Then just save the given params. Maybe we should add some checks here.
  mName = name;
  mId = id;
  mNumberChannels = numberChannels;
  mSampleLayout = sampleLayout;
  mFrameLength = frameLength;
  mType = type;
  mSidAvailable = sidAvailable;

  IasAudioBufferPoolHandler *bufferPoolHandler = IasAudioBufferPoolHandler::getInstance();
  IasAudioBufferPool *bufferPool = bufferPoolHandler->getBufferPool(mFrameLength * mNumberChannels);
  IAS_ASSERT(bufferPool != NULL);
  IasAudioBuffer *buffer = bufferPool->getBuffer();
  IAS_ASSERT(buffer != NULL);
  mBuffer = buffer;
  float *audioBuffer = buffer->getData();
  for (uint32_t index = 0; index < mNumberChannels; ++index)
  {
    if (mSampleLayout == eIasNonInterleaved)
    {
      // The first entry of mAudioFrame is the base address to be freed in the destructor
      mAudioFrame.push_back(&audioBuffer[index*mFrameLength]);
    }
    else if (mSampleLayout == eIasInterleaved)
    {
      // The first entry of mAudioFrame is the base address to be freed in the destructor
      mAudioFrame.push_back(&audioBuffer[index]);
    }
    else
    {
      // Bundled is currently not supported
      return eIasAudioProcUnsupportedFormat;
    }
  }
  return eIasAudioProcOK;
}

void IasSimpleAudioStream::cleanup()
{
  if (mBuffer != NULL)
  {
    (void)IasAudioBufferPool::returnBuffer(mBuffer);
    mBuffer = NULL;
  }
  mAudioFrame.clear();
}


IasAudioProcessingResult IasSimpleAudioStream::writeFromNonInterleaved(const IasAudioFrame &audioFrame)
{
  // @note: This method may only be called the first time each processing cycle when the data is copied initially
  // to the stream buffer. That's why it is safe to increase the number of channels by one
  // when the flag mSidAvailable is set to true.
  uint32_t numberChannels = mNumberChannels;
  if (mSidAvailable == true)
  {
    numberChannels++;
  }
  if (audioFrame.size() == numberChannels)
  {
    if (mSampleLayout == eIasNonInterleaved)
    {
      for (uint32_t index = 0; index < mNumberChannels; ++index)
      {
        IasAudioCommonResult result = ias_safe_memcpy(mAudioFrame[index],
                                                      sizeof(float) * mFrameLength,
                                                      audioFrame[index],
                                                      sizeof(float) * mFrameLength);
        IAS_ASSERT(result == eIasResultOk);
        (void)result;
      }
    }
    else if (mSampleLayout == eIasInterleaved)
    {
      float *destination = mAudioFrame[0];
      for (uint32_t index = 0; index < mNumberChannels; ++index)
      {
        for (uint32_t frameIndex = 0; frameIndex < mFrameLength; ++frameIndex)
        {
          destination[frameIndex*mNumberChannels+index] = audioFrame[index][frameIndex];
        }
      }
    }
    else
    {
      return eIasAudioProcInitializationFailed;
    }
    //check if the sidavailable flag is set
    // if yes take a sample out of the last buffer and
    if(mSidAvailable == true)
    {//cast the pointer to UInt32 because the sid is actually a UInt32 value.
      int32_t * tmpData = (int32_t*)audioFrame.back();
      //get the sid from the last channel
      mSid = tmpData[0];
    }

    return eIasAudioProcOK;
  }
  else
  {
    return eIasAudioProcInvalidParam;
  }
}

IasAudioProcessingResult IasSimpleAudioStream::writeFromBundled(const IasAudioFrame &audioFrame)
{
  IAS_ASSERT(audioFrame.size() == mNumberChannels);
  if (mSampleLayout == eIasNonInterleaved)
  {
    for (uint32_t index = 0; index < mNumberChannels; ++index)
    {
      float *source = audioFrame[index];
      float *destination = mAudioFrame[index];
      for (uint32_t frameIndex = 0; frameIndex < mFrameLength; ++frameIndex)
      {
        *destination++ = *source;
        source += cIasNumChannelsPerBundle;
      }
    }
  }
  else if (mSampleLayout == eIasInterleaved)
  {
    float *destination = mAudioFrame[0];
    for (uint32_t index = 0; index < mNumberChannels; ++index)
    {
      float *source = audioFrame[index];
      for (uint32_t frameIndex = 0; frameIndex < mFrameLength; ++frameIndex)
      {
        destination[frameIndex*mNumberChannels+index] = *source;
        source += cIasNumChannelsPerBundle;
      }
    }
  }
  else
  {
    return eIasAudioProcInitializationFailed;
  }
  return eIasAudioProcOK;
}

void IasSimpleAudioStream::asNonInterleavedStream(IasSimpleAudioStream *nonInterleaved)
{
  if (mSampleLayout == eIasInterleaved)
  {
    // Convert from interleaved => non-interleaved
    nonInterleaved->setProperties(mName, mId, mNumberChannels, eIasNonInterleaved, mFrameLength, mType, mSidAvailable);
    const IasAudioFrame &destination = nonInterleaved->getAudioBuffers();
    const float *source = mAudioFrame[0];
    for (uint32_t index = 0; index < mNumberChannels; ++index)
    {
      for (uint32_t frameIndex = 0; frameIndex < mFrameLength; ++frameIndex)
      {
        destination[index][frameIndex] = source[frameIndex*mNumberChannels+index];
      }
    }
    if (mSidAvailable == true)
    {
      nonInterleaved->setSid(mSid);
    }
  }
}

void IasSimpleAudioStream::asInterleavedStream(IasSimpleAudioStream *interleaved)
{
  if (mSampleLayout == eIasNonInterleaved)
  {
    // Convert from non-interleaved => interleaved
    interleaved->setProperties(mName, mId, mNumberChannels, eIasInterleaved, mFrameLength, mType, mSidAvailable);
    float *destination = (interleaved->getAudioBuffers())[0];
    const IasAudioFrame &source = mAudioFrame;
    for (uint32_t index = 0; index < mNumberChannels; ++index)
    {
      for (uint32_t frameIndex = 0; frameIndex < mFrameLength; ++frameIndex)
      {
        destination[frameIndex*mNumberChannels+index] = source[index][frameIndex];
      }
    }
    if (mSidAvailable == true)
    {
      interleaved->setSid(mSid);
    }
  }
}

void IasSimpleAudioStream::asBundledStream(IasBundledAudioStream *bundled)
{
  if (mSampleLayout == eIasNonInterleaved)
  {
    // Convert from non-interleaved => bundled
    IAS_ASSERT(mAudioFrame.size() == bundled->getNumberChannels());
    bundled->writeFromNonInterleaved(mAudioFrame);
    if (mSidAvailable == true)
    {
      bundled->setSid(mSid);
    }
  }
  else if (mSampleLayout == eIasInterleaved)
  {
    // Convert from interleaved => bundled
    IAS_ASSERT(mAudioFrame.size() == bundled->getNumberChannels());
    bundled->writeFromInterleaved(mAudioFrame);
    if (mSidAvailable == true)
    {
      bundled->setSid(mSid);
    }
  }
}

void IasSimpleAudioStream::copyToOutputAudioChannels(const IasAudioFrame &outAudioFrame) const
{
  if (mSampleLayout == eIasNonInterleaved)
  {
    for (uint32_t chanIdx = 0; chanIdx < mNumberChannels; ++chanIdx)
    {
      IasAudioCommonResult result = ias_safe_memcpy(outAudioFrame[chanIdx],
                                                    sizeof(float) * mFrameLength,
                                                    mAudioFrame[chanIdx],
                                                    sizeof(float) * mFrameLength);
      IAS_ASSERT(result == eIasResultOk);
      (void)result;
    }
  }
  else if (mSampleLayout == eIasInterleaved)
  {
    float *source = mAudioFrame[0];
    for (uint32_t chanIdx = 0; chanIdx < mNumberChannels; ++chanIdx)
    {
      for (uint32_t frameIndex = 0; frameIndex < mFrameLength; ++frameIndex)
      {
        outAudioFrame[chanIdx][frameIndex] = source[frameIndex*mNumberChannels+chanIdx];
      }
    }
  }
  if(mSidAvailable == true)
  {
    // Cast the pointer to UInt32 because the SID is actually a UInt32 value.
    uint32_t *sidBuffer = (uint32_t*)outAudioFrame.back();
    for(uint32_t sampleIdx = 0; sampleIdx < mFrameLength; sampleIdx++)
    {
      // Set all samples in this channel to the SID value
      sidBuffer[sampleIdx] = mSid;
    }
  }
}

IasAudioProcessingResult IasSimpleAudioStream::getAudioDataPointers(IasAudioFrame &audioFrame, uint32_t *stride) const
{
  IAS_ASSERT(stride != nullptr);

  if (audioFrame.size() != mNumberChannels)
  {
    return eIasAudioProcInvalidParam;
  }

  if (mSampleLayout == eIasNonInterleaved)
  {
    *stride = 1;
    for (uint32_t chanIdx = 0; chanIdx < mNumberChannels; ++chanIdx)
    {
      audioFrame[chanIdx] = mAudioFrame[chanIdx];
    }
  }
  else
  {
    *stride = mNumberChannels;
    for (uint32_t chanIdx = 0; chanIdx < mNumberChannels; ++chanIdx)
    {
      audioFrame[chanIdx] = mAudioFrame[0] + chanIdx;
    }
  }

  return eIasAudioProcOK;
}


} //namespace IasAudio
