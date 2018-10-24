/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEnvironmentTest.cpp
 * @date   Sep20, 2012
 * @brief Contains all integration tests for the IasAudioChainEnvironment class.
 */
#include "IasRtProcessingFwTest.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"


namespace IasAudio {

// begin IasAudioChainEnvironment test cases //
TEST_F(IasRtProcessingFwTest, CreateAndDestroyAudioChainEnvironment)
{
  IasAudioChainEnvironment *audioChainEnvironment = new IasAudioChainEnvironment();
  ASSERT_TRUE(audioChainEnvironment != NULL);
  delete audioChainEnvironment;
}

TEST_F(IasRtProcessingFwTest, setGetFrameLengthOfAudioChainEnvironment)
{
  uint32_t frameLength = 0;
  IasAudioChainEnvironment *audioChainEnvironment = new IasAudioChainEnvironment();
  audioChainEnvironment->setFrameLength(cIasTestAudioFrameLength);
  frameLength = audioChainEnvironment->getFrameLength();
  ASSERT_TRUE(frameLength == cIasTestAudioFrameLength);
  delete audioChainEnvironment;
}

TEST_F(IasRtProcessingFwTest, setGetSampleRateOfAudioChainEnvironment)
{
  uint32_t sampleRate = 0;
  IasAudioChainEnvironment *audioChainEnvironment = new IasAudioChainEnvironment();
  audioChainEnvironment->setSampleRate(44100);
  sampleRate = audioChainEnvironment->getSampleRate();
  ASSERT_TRUE(sampleRate == 44100);
  delete audioChainEnvironment;
}

TEST_F(IasRtProcessingFwTest, callStoString)
{
  ASSERT_TRUE( 0 == strcmp("eIasAudioProcOK", toString(eIasAudioProcOK)) );
}




// end IasAudioChainEnvironment test cases //
}
