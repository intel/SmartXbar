/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundledAudioStream.cpp
 * @date   2012
 * @brief  This is the implementation of the IasBundledAudioStream class.
 */

#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleSequencer.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"


#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"

namespace IasAudio {

IasBundledAudioStream::IasBundledAudioStream(const std::string& name,
                                             int32_t id,
                                             uint32_t numberChannels,
                                             IasAudioStreamTypes type,
                                             bool sidAvailable)
  :IasBaseAudioStream(name, id, numberChannels, type, sidAvailable)
{
}


IasBundledAudioStream::~IasBundledAudioStream()
{
  // Do not delete the pointers in the audio frame because they are managed by the bundle
  mAudioFrame.clear();
}

IasAudioProcessingResult IasBundledAudioStream::init(IasBundleSequencer *bundleSequencer, IasAudioChainEnvironmentPtr env)
{
  IAS_ASSERT(bundleSequencer != nullptr);
  IAS_ASSERT(env != nullptr);
  mEnv = env;
  IasAudioProcessingResult result = bundleSequencer->getBundleAssignments(mNumberChannels, &mBundleAssignments);
  if (result != eIasAudioProcOK)
  {
    // Log already done inside bundleSequencer->getBundleAssignments method
    return result;
  }
  IasBundleAssignmentVector::iterator bundleAssignmentIt;
  for (auto bundleAssignment: mBundleAssignments)
  {
    float *audioData = bundleAssignment.getBundle()->getAudioDataPointer();
    audioData += bundleAssignment.getIndex();
    for (uint32_t chanCnt=0; chanCnt<bundleAssignment.getNumberChannels(); ++chanCnt, ++audioData)
    {
      mAudioFrame.push_back(audioData);
    }
  }
  mSampleLayout = eIasBundled;
  return result;
}

IasAudioProcessingResult IasBundledAudioStream::writeFromNonInterleaved(const IasAudioFrame& audioFrame)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  uint32_t numberChannels = mNumberChannels;
  uint32_t bundleIndex = 0;
  uint32_t frameIndex = 0;

  while (numberChannels)
  {
    uint32_t currentNumberChannels;
    if (numberChannels > cIasNumChannelsPerBundle)
    {
      currentNumberChannels = cIasNumChannelsPerBundle;
    }
    else
    {
      currentNumberChannels = numberChannels;
    }
    IasBundleAssignment &bundleAssignment = mBundleAssignments[bundleIndex];
    IasAudioChannelBundle *bundle = bundleAssignment.getBundle();
    uint32_t offset = bundleAssignment.getIndex();

    // Reduce the number of channels if the current bundle does not provide enough free channels.
    uint32_t numFreeChannels = cIasNumChannelsPerBundle - offset;
    currentNumberChannels = std::min(currentNumberChannels, numFreeChannels);

    switch (currentNumberChannels)
    {
      case 1:
        bundle->writeOneChannelFromNonInterleaved(offset,
                                audioFrame[frameIndex]);
        break;
      case 2:
        bundle->writeTwoChannelsFromNonInterleaved(offset,
                                 audioFrame[frameIndex],
                                 audioFrame[frameIndex+1]);
        break;
      case 3:
        bundle->writeThreeChannelsFromNonInterleaved(offset,
                                   audioFrame[frameIndex],
                                   audioFrame[frameIndex+1],
                                   audioFrame[frameIndex+2]);
        break;
      case 4:
        bundle->writeFourChannelsFromNonInterleaved(audioFrame[frameIndex],
                                  audioFrame[frameIndex+1],
                                  audioFrame[frameIndex+2],
                                  audioFrame[frameIndex+3]);
        break;
      default:
        result = eIasAudioProcInvalidParam;
        break;
    }
    numberChannels -= currentNumberChannels;
    bundleIndex++;
    frameIndex += currentNumberChannels;
  }

  return result;
}

IasAudioProcessingResult IasBundledAudioStream::writeFromInterleaved(const IasAudioFrame& audioFrame)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  uint32_t numberChannels = mNumberChannels;
  uint32_t bundleIndex = 0;
  uint32_t frameIndex = 0;
  uint32_t srcStride = static_cast<uint32_t>(audioFrame.size());

  while (numberChannels)
  {
    uint32_t currentNumberChannels;
    if (numberChannels > cIasNumChannelsPerBundle)
    {
      currentNumberChannels = cIasNumChannelsPerBundle;
    }
    else
    {
      currentNumberChannels = numberChannels;
    }
    IasBundleAssignment &bundleAssignment = mBundleAssignments[bundleIndex];
    IasAudioChannelBundle *bundle = bundleAssignment.getBundle();
    uint32_t offset = bundleAssignment.getIndex();

    // Reduce the number of channels if the current bundle does not provide enough free channels.
    uint32_t numFreeChannels = cIasNumChannelsPerBundle - offset;
    currentNumberChannels = std::min(currentNumberChannels, numFreeChannels);

    switch (currentNumberChannels)
    {
      case 1:
        // We can use the non-interleaved version here because for one channel this is the same
        // implementation
        bundle->writeOneChannelFromNonInterleaved(offset,
                                                  audioFrame[frameIndex]);
        break;
      case 2:
        bundle->writeTwoChannelsFromInterleaved(offset,
                                                srcStride,
                                                audioFrame[frameIndex]);
        break;
      case 3:
        bundle->writeThreeChannelsFromInterleaved(offset,
                                                  srcStride,
                                                  audioFrame[frameIndex]);
        break;
      case 4:
        bundle->writeFourChannelsFromInterleaved(srcStride,
                                                 audioFrame[frameIndex]);
        break;
      default:
        result = eIasAudioProcInvalidParam;
        break;
    }
    numberChannels -= currentNumberChannels;
    bundleIndex++;
    frameIndex += currentNumberChannels;
  }

  return result;
}

IasAudioProcessingResult IasBundledAudioStream::read(const IasAudioFrame &audioFrame) const
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  uint32_t numberChannels = mNumberChannels;
  uint32_t bundleIndex = 0;
  uint32_t frameIndex = 0;
  uint32_t frameLength = mEnv->getFrameLength();

  while (numberChannels)
  {
    uint32_t currentNumberChannels;
    const IasBundleAssignment &bundleAssignment = mBundleAssignments[bundleIndex];
    const IasAudioChannelBundle *bundle = bundleAssignment.getBundle();
    uint32_t offset = bundleAssignment.getIndex();

    if (numberChannels > cIasNumChannelsPerBundle)
    {
      currentNumberChannels = cIasNumChannelsPerBundle - offset;
    }
    else
    {
      currentNumberChannels = numberChannels;
    }

    switch (currentNumberChannels)
    {
      case 1:
        bundle->readOneChannel(offset,
                               audioFrame[frameIndex]);
        break;
      case 2:
        bundle->readTwoChannels(offset,
                                audioFrame[frameIndex],
                                audioFrame[frameIndex+1]);
        break;
      case 3:
        bundle->readThreeChannels(offset,
                                  audioFrame[frameIndex],
                                  audioFrame[frameIndex+1],
                                  audioFrame[frameIndex+2]);
        break;
      case 4:
        bundle->readFourChannels(audioFrame[frameIndex],
                                 audioFrame[frameIndex+1],
                                 audioFrame[frameIndex+2],
                                 audioFrame[frameIndex+3]);
        break;
      default:
        result = eIasAudioProcInvalidParam;
        break;
    }
    numberChannels -= currentNumberChannels;
    bundleIndex++;
    frameIndex += currentNumberChannels;
  }
  // check if the mSidAvailable flag is set
  // if yes then fill the last buffer with SID values
  if(mSidAvailable == true)
  {
    // cast the pointer to UInt32 because the SID is actually a UInt32 value.
    uint32_t* lSidBuffer = (uint32_t*)audioFrame[frameIndex];
    for(uint16_t samplecount = 0; samplecount < frameLength; samplecount++)
    {
      //set all samples in this channel to the SID value
      lSidBuffer[samplecount] = mSid;
    }
  }
  return result;
}

IasAudioProcessingResult IasBundledAudioStream::read(float* audioFrame) const
{
  std::vector<float*> data;

  for(uint32_t bundleCnt = 0; bundleCnt < mBundleAssignments.size(); bundleCnt++)
  {
	  const IasBundleAssignment &bundleAssignment = mBundleAssignments[bundleCnt];
	  data.push_back(bundleAssignment.getBundle()->getAudioDataPointer());
	  data[bundleCnt]+= bundleAssignment.getIndex();
  }

  uint32_t frameLength = mEnv->getFrameLength();
  for(uint32_t audioSampleCnt = 0; audioSampleCnt < (mNumberChannels * frameLength);)
  {
	  for(uint32_t bundleCnt = 0; bundleCnt < mBundleAssignments.size(); bundleCnt++)
	  {
		  const IasBundleAssignment &bundleAssignment = mBundleAssignments[bundleCnt];
		  for(uint32_t bundleChanCnt = 0; bundleChanCnt < bundleAssignment.getNumberChannels(); bundleChanCnt++)
		  {
			  audioFrame[audioSampleCnt++] = data[bundleCnt][bundleChanCnt];
		  }
		  data[bundleCnt] += cIasNumChannelsPerBundle;
	  }
  }
  return eIasAudioProcOK;
}

void IasBundledAudioStream::getAudioFrame(IasAudioFrame* audioFrame)
{
  audioFrame->insert(audioFrame->end(), mAudioFrame.begin(), mAudioFrame.end());
}

void IasBundledAudioStream::asNonInterleavedStream(IasSimpleAudioStream *nonInterleaved)
{
  nonInterleaved->setProperties(mName, mId, mNumberChannels, eIasNonInterleaved, mEnv->getFrameLength(), mType, mSidAvailable);
  nonInterleaved->writeFromBundled(mAudioFrame);
  if (mSidAvailable == true)
  {
    nonInterleaved->setSid(mSid);
  }
}

void IasBundledAudioStream::asInterleavedStream(IasSimpleAudioStream *interleaved)
{
  interleaved->setProperties(mName, mId, mNumberChannels, eIasInterleaved, mEnv->getFrameLength(), mType, mSidAvailable);
  interleaved->writeFromBundled(mAudioFrame);
  if (mSidAvailable == true)
  {
    interleaved->setSid(mSid);
  }
}

void IasBundledAudioStream::asBundledStream(IasBundledAudioStream *bundled)
{
  // Nothing to do, we are already bundled
  (void)bundled;
}

void IasBundledAudioStream::copyToOutputAudioChannels(const IasAudioFrame &outAudioFrame) const
{
  read(outAudioFrame);
}


IasAudioProcessingResult IasBundledAudioStream::getAudioDataPointers(IasAudioFrame &audioFrame, uint32_t *stride) const
{
  IAS_ASSERT(stride != nullptr);
  *stride = cIasNumChannelsPerBundle;

  if (audioFrame.size() != mNumberChannels)
  {
    return eIasAudioProcInvalidParam;
  }

  uint32_t numberChannels = mNumberChannels;
  uint32_t bundleIndex  = 0;
  uint32_t channelIndex = 0;

  while (numberChannels)
  {
    uint32_t currentNumberChannels;
    const IasBundleAssignment &bundleAssignment = mBundleAssignments[bundleIndex];
    const IasAudioChannelBundle *bundle = bundleAssignment.getBundle();
    uint32_t offset = bundleAssignment.getIndex();

    if (numberChannels > cIasNumChannelsPerBundle)
    {
      currentNumberChannels = cIasNumChannelsPerBundle - offset;
    }
    else
    {
      currentNumberChannels = numberChannels;
    }

    if (currentNumberChannels > 4)
    {
      return eIasAudioProcInvalidParam;
    }

    for (uint32_t channel = 0; channel < currentNumberChannels; channel++)
    {
      float* pointer = bundle->getAudioDataPointer() + offset + channel;
      audioFrame[channelIndex + channel] = pointer;
    }

    numberChannels -= currentNumberChannels;
    bundleIndex++;
    channelIndex += currentNumberChannels;
  }

  return eIasAudioProcOK;
}


} // namespace IasAudio
