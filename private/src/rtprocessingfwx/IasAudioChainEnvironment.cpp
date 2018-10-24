/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChainEnvironment.cpp
 * @date   2012
 * @brief  This is the implementation of the IasAudioChainEnvironment class.
 */

#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

IasAudioChainEnvironment::IasAudioChainEnvironment()
  :mFrameLength(0)
  ,mSamplerate(0)
  ,mLogContext(IasAudioLogging::getDltContext("PFW"))
{
}

IasAudioChainEnvironment::~IasAudioChainEnvironment()
{
}

void IasAudioChainEnvironment::setFrameLength(uint32_t frameLength)
{
  mFrameLength = frameLength;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioChainEnvironment::setFrameLength: Set Frame length to", frameLength);
}

uint32_t IasAudioChainEnvironment::getFrameLength()
{
  return mFrameLength;
}

void IasAudioChainEnvironment::setSampleRate(uint32_t sampleRate)
{
  mSamplerate = sampleRate;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioChainEnvironment::setSampleRate: Set sample-rate to", sampleRate);
}

uint32_t IasAudioChainEnvironment::getSampleRate()
{
  return mSamplerate;
}


} // namespace IasAudio
