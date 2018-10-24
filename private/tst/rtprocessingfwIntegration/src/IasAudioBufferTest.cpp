/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBufferTest.cpp
 * @date   June 29, 2016
 * @brief  Contains some test cases for covering the IasAudioBuffer and IasAudioBufferPool classes.
 */

#include "rtprocessingfwx/IasAudioBuffer.hpp"
#include "rtprocessingfwx/IasAudioBufferPool.hpp"
#include "IasRtProcessingFwTest.hpp"

namespace IasAudio {

TEST_F(IasRtProcessingFwTest, AudioBufferTest)
{
  IasAudioBuffer*     audioBuffer     = new IasAudioBuffer(42);
  IasAudioBufferPool* audioBufferPool = new IasAudioBufferPool(42);

  audioBuffer->setHomePool(nullptr);
  audioBuffer->setHomePool(audioBufferPool);
  audioBuffer->setHomePool(audioBufferPool);

  IasAudioProcessingResult result;
  result = audioBufferPool->returnBuffer(nullptr);
  ASSERT_TRUE(eIasAudioProcInvalidParam == result);
  result = audioBufferPool->returnBuffer(audioBuffer);
  ASSERT_TRUE(eIasAudioProcOK == result);

  delete audioBuffer;
}


} // namespace IasAudio
