/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioStream.cpp
 * @date   2013
 * @brief
 */

#include <sstream>
#include <iomanip>

#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "rtprocessingfwx/IasBundleSequencer.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

IasAudioStream::IasAudioStream(const std::string &name,
                               int32_t id,
                               uint32_t numberChannels,
                               IasBaseAudioStream::IasAudioStreamTypes type,
                               bool sidAvailable)
  :mName(name)
  ,mId(id)
  ,mNumberChannels(numberChannels)
  ,mType(type)
  ,mSidAvailable(sidAvailable)
  ,mAudioFrameIn()
  ,mAudioFrameOut()
  ,mAudioFrameInternal()
  ,mCopyFromInput(false)
  ,mBundleSequencer(nullptr)
  ,mBundled(nullptr)
  ,mNonInterleaved(nullptr)
  ,mInterleaved(nullptr)
  ,mCurrentRepresentation(nullptr)
  ,mConnectedDevice(nullptr)
  ,mLogContext(IasAudioLogging::getDltContext("PFW"))
  ,mLogCounterGADP1(0)
  ,mLogCounterGADP2(0)
{
}

IasAudioStream::~IasAudioStream()
{
  // Delete all different representations of the stream here as we are the base stream.
  // BundleSequencer will be deleted outside
  mBundleSequencer = nullptr;
  delete mBundled;
  mBundled = nullptr;
  delete mNonInterleaved;
  mNonInterleaved = nullptr;
  delete mInterleaved;
  mInterleaved = nullptr;
  // Do not delete the current representation because this is one of the other pointers already deleted
  mCurrentRepresentation = nullptr;
}

IasAudioProcessingResult IasAudioStream::init(IasAudioChainEnvironmentPtr env)
{
  mEnv = env;
  if (env == nullptr)
  {
    return eIasAudioProcInvalidParam;
  }
  uint32_t fullNumberChannels = mNumberChannels;
  if (mSidAvailable == true)
  {
    fullNumberChannels++;
  }
  mAudioFrameIn.resize(fullNumberChannels, nullptr);
  mAudioFrameOut.resize(fullNumberChannels, nullptr);
  mAudioFrameInternal.resize(fullNumberChannels, nullptr);

  return eIasAudioProcOK;
}

IasSimpleAudioStream* IasAudioStream::getNonInterleavedStream()
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  if (mNonInterleaved != nullptr)
  {
    return mNonInterleaved;
  }
  else
  {
    // The stream was not used in non-interleaved representation before,
    // so we create it here
    mNonInterleaved = new IasSimpleAudioStream();
    if (mNonInterleaved == nullptr)
    {
      result = eIasAudioProcNotEnoughMemory;
      DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioStream::getNonInterleavedStream: Not enough memory to create IasSimpleAudioStream");
    }
    if (result == eIasAudioProcOK)
    {
      uint32_t frameLength = mEnv->getFrameLength();
      result = mNonInterleaved->setProperties(mName,
                                              mId,
                                              mNumberChannels,
                                              IasBaseAudioStream::eIasNonInterleaved,
                                              frameLength,
                                              mType,
                                              mSidAvailable);
      if (result == eIasAudioProcOK)
      {
        return mNonInterleaved;
      }
      else
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioStream::getNonInterleavedStream: Error during setProperties for", mName, ":", static_cast<int32_t>(result));
      }
    }
  }
  return nullptr;
}

IasSimpleAudioStream* IasAudioStream::getInterleavedStream()
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  if (mInterleaved != nullptr)
  {
    return mInterleaved;
  }
  else
  {
    // The stream was not used in interleaved representation before,
    // so we create it here
    mInterleaved = new IasSimpleAudioStream();
    if (mInterleaved == nullptr)
    {
      result = eIasAudioProcNotEnoughMemory;
      DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioStream::getInterleavedStream: Not enough memory to create IasSimpleAudioStream");
    }
    if (result == eIasAudioProcOK)
    {
      uint32_t frameLength = mEnv->getFrameLength();
      result = mInterleaved->setProperties(mName,
                                           mId,
                                           mNumberChannels,
                                           IasBaseAudioStream::eIasInterleaved,
                                           frameLength,
                                           mType,
                                           mSidAvailable);
      if (result == eIasAudioProcOK)
      {
        return mInterleaved;
      }
      else
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioStream::getInterleavedStream: Error during setProperties for", mName, ":", static_cast<int32_t>(result));
      }
    }
  }
  return nullptr;
}

IasBundledAudioStream* IasAudioStream::getBundledStream()
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  if (mBundled != nullptr)
  {
    return mBundled;
  }
  else
  {
    // The stream was not used in bundled representation before,
    // so we create it here
    mBundled = new IasBundledAudioStream(mName, mId, mNumberChannels, mType, mSidAvailable);
    if (mBundled == nullptr)
    {
      result = eIasAudioProcNotEnoughMemory;
      DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioStream::getBundledStream: Not enough memory to create IasBundledAudioStream");
    }
    if (result == eIasAudioProcOK)
    {
      result = mBundled->init(mBundleSequencer, mEnv);

      if (result == eIasAudioProcOK)
      {
        return mBundled;
      }
      else
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioStream::getBundledStream: Error during setProperties for", mName, ":", static_cast<int32_t>(result));
      }
    }
  }
  return nullptr;
}


IasSimpleAudioStream* IasAudioStream::asNonInterleavedStream()
{
  IasSimpleAudioStream *nonInterleaved;
  if (mCurrentRepresentation != nullptr)
  {
    nonInterleaved = getNonInterleavedStream();
    IAS_ASSERT(nonInterleaved != nullptr);
    mCurrentRepresentation->asNonInterleavedStream(nonInterleaved);
    mCurrentRepresentation = nonInterleaved;
  }
  else
  {
    nonInterleaved = getNonInterleavedStream();
    IAS_ASSERT(nonInterleaved != nullptr);
    mCurrentRepresentation = nonInterleaved;
  }
  // If the flag was set by call to the method copyFromInputChannels we have to initially copy the
  // samples from the input audio frame into the stream buffer pointer.
  if (mCopyFromInput == true)
  {
    // Now copy the samples from the input audio frame to the buffers of the stream
    IasAudioProcessingResult result = nonInterleaved->writeFromNonInterleaved(mAudioFrameIn);
    if (result != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioStream::asNonInterleavedStream: Error writing initial audio frame to non-interleaved stream:", static_cast<int32_t>(result));
    }
    updateCurrentSid();
    // Don't forget to clear the flag because the following components need the samples from the previous component
    // and not from the input.
    mCopyFromInput = false;
  }

  return nonInterleaved;
}

IasSimpleAudioStream* IasAudioStream::asInterleavedStream()
{
  IasSimpleAudioStream *interleaved;
  if (mCurrentRepresentation != nullptr)
  {
    interleaved = getInterleavedStream();
    IAS_ASSERT(interleaved != nullptr);
    mCurrentRepresentation->asInterleavedStream(interleaved);
    mCurrentRepresentation = interleaved;
  }
  else
  {
    interleaved = getInterleavedStream();
    IAS_ASSERT(interleaved != nullptr);
    mCurrentRepresentation = interleaved;
  }
  // If the flag was set by call to the method copyFromInputChannels we have to initially copy the
  // samples from the input audio frame into the stream buffer pointer.
  if (mCopyFromInput == true)
  {
    // Now copy the samples from the input audio frame to the buffers of the stream
    IasAudioProcessingResult result = interleaved->writeFromNonInterleaved(mAudioFrameIn);
    if (result != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioStream::asInterleavedStream: Error writing initial audio frame to interleaved stream:", static_cast<int32_t>(result));
    }
    updateCurrentSid();
    // Don't forget to clear the flag because the following components need the samples from the previous component
    // and not from the input.
    mCopyFromInput = false;
  }

  return interleaved;
}

IasBundledAudioStream* IasAudioStream::asBundledStream()
{
  IasBundledAudioStream *bundled;
  if (mCurrentRepresentation != nullptr)
  {
    bundled = getBundledStream();
    IAS_ASSERT(bundled != nullptr);
    mCurrentRepresentation->asBundledStream(bundled);
    mCurrentRepresentation = bundled;
  }
  else
  {
    bundled = getBundledStream();
    mCurrentRepresentation = bundled;
  }
  // If the flag was set by call to the method copyFromInputChannels we have to initially copy the
  // samples from the input audio frame into the stream buffer pointer.
  if (mCopyFromInput == true)
  {
    // Now copy the samples from the input audio frame to the buffers of the stream
    IasAudioProcessingResult result = bundled->writeFromNonInterleaved(mAudioFrameIn);
    if (result != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioStream::asBundledStream: Error writing initial audio frame to bundled stream:", static_cast<int32_t>(result));
    }
    updateCurrentSid();
    // Don't forget to clear the flag because the following components need the samples from the previous component
    // and not from the input.
    mCopyFromInput = false;
  }

  return bundled;
}

uint32_t IasAudioStream::getSid() const
{
  IAS_ASSERT(mCurrentRepresentation != nullptr);
  return mCurrentRepresentation->getSid();
}

void IasAudioStream::setSid(uint32_t newSid)
{
  IAS_ASSERT(mCurrentRepresentation != nullptr);
  mCurrentRepresentation->setSid(newSid);
}

void IasAudioStream::copyFromInputAudioChannels()
{
  mCopyFromInput = true;
}


void IasAudioStream::copyToOutputAudioChannels()
{
  if (mCurrentRepresentation != nullptr)
  {
    mCurrentRepresentation->copyToOutputAudioChannels(mAudioFrameOut);
    // Reset the current representation because we finished this round and will start over again.
    // When resetting the current representation we avoid copying the old data back into a currently
    // active stream.
    mCurrentRepresentation = nullptr;
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioStream::copyToOutputAudioChannels: Output stream", getName(), "was not used by any component, so no samples will be delivered");
  }
}


void IasAudioStream::getAudioDataPointers(IasAudioFrame** audioFrame, uint32_t *stride)
{
  IAS_ASSERT(audioFrame != nullptr);
  if (mCurrentRepresentation != nullptr)
  {
    mCurrentRepresentation->getAudioDataPointers(mAudioFrameInternal, stride);
    // Reset the current representation because we finished this round and will start over again.
    // When resetting the current representation we avoid copying the old data back into a currently
    // active stream.
    mCurrentRepresentation = nullptr;
    *audioFrame = &mAudioFrameInternal;
    mLogCounterGADP1 = 0;
  }
  else
  {
    if ((mLogCounterGADP1 % 100) == 0)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioStream::getAudioDataPointers", getName(), "was not used by any component, so no samples will be delivered, cnt=", mLogCounterGADP1);
    }
    mLogCounterGADP1++;
    *audioFrame = nullptr;
  }
}

void IasAudioStream::getAudioDataPointers(IasAudioFrame& audioFrame )
{
  if (mCurrentRepresentation != nullptr)
  {
    uint32_t stride;
    mCurrentRepresentation->getAudioDataPointers(audioFrame, &stride);
    (void)stride;
    mLogCounterGADP2 = 0;
  }
  else
  {
    if ((mLogCounterGADP2 % 100) == 0)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioStream::getAudioDataPointers", getName(), "was not used by any component, so no samples will be delivered, cnt=", mLogCounterGADP2);
    }
    mLogCounterGADP2++;
  }
}

void IasAudioStream::updateCurrentSid()
{
  // Set the SID value of the stream
  // check if the mSidAvailable flag is set
  // if yes take a sample out of the last buffer and
  if(mSidAvailable == true)
  {
    //cast the pointer to UInt32 because the SID is actually a UInt32 value.
    uint32_t * tmpData = (uint32_t*)mAudioFrameIn.back();
    // get the sid from the last channel and set it in the current representation of the stream
    setSid(tmpData[0]);
  }
}

void IasAudioStream::setConnectedDevice(IasAudioDevice* device)
{
  if(device != nullptr)
  {
    mConnectedDevice = device;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioStream:: set connected device for stream", getName()," with ID: ", getId());
  }
}

void IasAudioStream::removeConnectedDevice()
{
  mConnectedDevice = nullptr;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioStream:: remove connected device for stream", getName()," with ID: ", getId()," ");
}

IasBaseAudioStream::IasSampleLayout IasAudioStream::getSampleLayout() const
{
  if (mCurrentRepresentation != nullptr)
  {
    return mCurrentRepresentation->getSampleLayout();
  }
  else
  {
    return IasBaseAudioStream::eIasUndefined;
  }
}


} // namespace IasAudio
