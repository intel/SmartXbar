/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBuffer.cpp
 * @date   2013
 * @brief
 */

#include <malloc.h>
#include "rtprocessingfwx/IasAudioBuffer.hpp"


#ifdef __linux__
#define MS_VC  0
#else
#define MS_VC  1
#endif

#if !(MS_VC)
#include <malloc.h>
#endif

namespace IasAudio {

IasAudioBuffer::IasAudioBuffer(uint32_t bufferSize)
  :mBufferSize(bufferSize)
  ,mHome(NULL)
  ,mData(NULL)
{
}

IasAudioBuffer::~IasAudioBuffer()
{
#if MS_VC
  _aligned_free(mData);
#else
  free(mData);
#endif
}

IasAudioProcessingResult IasAudioBuffer::init()
{
#if MS_VC
  mData = (float*) _aligned_malloc(mBufferSize*sizeof(float), 16);
#else
  mData = (float*)memalign(16, mBufferSize*sizeof(float));
#endif
  IAS_ASSERT(mData != nullptr);
  return eIasAudioProcOK;
}

} // namespace IasAudio
