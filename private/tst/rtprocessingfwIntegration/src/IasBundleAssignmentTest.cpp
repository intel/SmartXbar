/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundleAssignmentTest.cpp
 * @date  Sep 17, 2012
 * @brief
 */

#include "IasRtProcessingFwTest.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"

namespace IasAudio {

// begin IasBundleAssignment test cases //
TEST_F(IasRtProcessingFwTest, CreateAndDestroyBundleAssignment)
{
  IasAudioChannelBundle bundle(cIasTestAudioFrameLength);

  IasBundleAssignment* bundleAssignment = new IasBundleAssignment(&bundle, 0/*chanIdx*/, 2 /*number channel*/);
  ASSERT_TRUE(bundleAssignment != NULL);

  delete bundleAssignment;
}

TEST_F(IasRtProcessingFwTest, GetBundleBundleAssignment)
{
  IasAudioChannelBundle bundle(cIasTestAudioFrameLength);
  IasAudioChannelBundle* bundlePointer;

  IasBundleAssignment* bundleAssignment = new IasBundleAssignment(&bundle, 0/*chanIdx*/, 2 /*number channel*/);
  ASSERT_TRUE(bundleAssignment != NULL);

  bundlePointer = bundleAssignment->getBundle();
  ASSERT_TRUE(bundlePointer == &bundle);

  delete bundleAssignment;
}

TEST_F(IasRtProcessingFwTest, GetIndexBundleAssignment)
{
  IasAudioChannelBundle bundle(cIasTestAudioFrameLength);
  uint32_t index;
  const uint32_t channelIdx  = 0;
  const uint32_t numCchannel = 2;

  IasBundleAssignment* bundleAssignment = new IasBundleAssignment(&bundle, channelIdx, numCchannel);
  ASSERT_TRUE(bundleAssignment != NULL);

  index = bundleAssignment->getIndex();
  ASSERT_TRUE(index == channelIdx);

  delete bundleAssignment;
}

TEST_F(IasRtProcessingFwTest, GetNumberChannelsBundleAssignment)
{
  IasAudioChannelBundle bundle(cIasTestAudioFrameLength);
  uint32_t numChan;
  const uint32_t channelIdx  = 0;
  const uint32_t numCchannel = 2;

  IasBundleAssignment* bundleAssignment = new IasBundleAssignment(&bundle, channelIdx, numCchannel);
  ASSERT_TRUE(bundleAssignment != NULL);

  numChan = bundleAssignment->getNumberChannels();
  ASSERT_TRUE(numChan == numCchannel);

  delete bundleAssignment;
}
}
