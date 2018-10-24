/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBufferPool.cpp
 * @date   2013
 * @brief  The definition of the IasAudioBufferPool class
 */

#include "rtprocessingfwx/IasAudioBufferPool.hpp"

namespace IasAudio {

IasAudioBufferPool::IasAudioBufferPool(uint32_t bufferSize)
  :mBufferSize(bufferSize)
{
}

IasAudioBufferPool::~IasAudioBufferPool()
{
  while(!mRingBuffers.empty())
  {
    delete mRingBuffers.front();
    mRingBuffers.pop_front();
  }
}

IasAudioBuffer* IasAudioBufferPool::getBuffer()
{
  IasAudioBuffer* buffer = NULL;
  if (mRingBuffers.empty() == true)
  {
    // No buffer available, so create a new one
    buffer = new IasAudioBuffer(mBufferSize);
    IAS_ASSERT(buffer != nullptr);
    IasAudioProcessingResult result = buffer->init();
    (void)result;
    IAS_ASSERT(result == eIasAudioProcOK);
    buffer->setHomePool(this);
  }
  else
  {
    buffer = mRingBuffers.back();
    IAS_ASSERT(buffer != NULL);
    mRingBuffers.pop_back();
  }
  return buffer;
}

void IasAudioBufferPool::doReturnBuffer(IasAudioBuffer* buffer)
{
  // Is already checked in the static function returnBuffer
  IAS_ASSERT(buffer != NULL);
  mRingBuffers.push_back(buffer);
}

} // namespace IasAudio
