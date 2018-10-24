/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundleSequencerTest.cpp
 * @date   Sep 17, 2012
 * @brief
 */

#include "IasRtProcessingFwTest.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasBundleSequencer.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"

namespace IasAudio {

// begin IasBundleSequencer test cases //
TEST_F(IasRtProcessingFwTest, getBundleAssignmentWithoutSettingFramelenghtBundleSequencer)
{
  IasAudioProcessingResult result;

  IasBundleSequencer *bundleSequencer = new IasBundleSequencer();
  ASSERT_TRUE(bundleSequencer != NULL);
  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  ASSERT_TRUE(env != nullptr);
  result = bundleSequencer->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasBundleAssignmentVector bundleAssignmentVector;
  result = bundleSequencer->getBundleAssignments(1/*numChan*/, &bundleAssignmentVector);
  ASSERT_EQ(eIasAudioProcNotInitialization, result);

  delete bundleSequencer;
}

TEST_F(IasRtProcessingFwTest, getMonoBundleAssignmentBundleSequencer)
{
  IasAudioProcessingResult result;

  IasBundleSequencer *bundleSequencer = new IasBundleSequencer();
  ASSERT_TRUE(bundleSequencer != NULL);

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasBundleAssignmentVector bundleAssignmentVector;
  result = bundleSequencer->getBundleAssignments(1/*numChan*/, &bundleAssignmentVector);
  ASSERT_EQ(eIasAudioProcOK, result);

  delete bundleSequencer;
}

TEST_F(IasRtProcessingFwTest, getStereoBundleAssignmentBundleSequencer)
{
  IasAudioProcessingResult result;

  IasBundleSequencer *bundleSequencer = new IasBundleSequencer();
  ASSERT_TRUE(bundleSequencer != NULL);

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasBundleAssignmentVector bundleAssignmentVector;
  result = bundleSequencer->getBundleAssignments(2/*numChan*/, &bundleAssignmentVector);
  ASSERT_TRUE(result == eIasAudioProcOK);

  delete bundleSequencer;
}

TEST_F(IasRtProcessingFwTest, getMcBundleAssignmentBundleSequencer)
{
  IasAudioProcessingResult result;

  IasBundleSequencer *bundleSequencer = new IasBundleSequencer();
  ASSERT_TRUE(bundleSequencer != NULL);

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasBundleAssignmentVector bundleAssignmentVector;
  result = bundleSequencer->getBundleAssignments(6/*numChan*/, &bundleAssignmentVector);
  ASSERT_TRUE(result == eIasAudioProcOK);

  delete bundleSequencer;
}

TEST_F(IasRtProcessingFwTest, clearAllBundles)
{
  IasAudioProcessingResult result;

  IasBundleSequencer *bundleSequencer = new IasBundleSequencer();
  ASSERT_TRUE(bundleSequencer != NULL);

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasBundleAssignmentVector bundleAssignmentVector;
  result = bundleSequencer->getBundleAssignments(2/*numChan*/, &bundleAssignmentVector);
  ASSERT_TRUE(result == eIasAudioProcOK);

  bundleSequencer->clearAllBundleBuffers();
  uint32_t *buffer = reinterpret_cast<uint32_t*>(bundleAssignmentVector[0].getBundle()->getAudioDataPointer());
  for (uint32_t sample=0; sample<cIasTestAudioFrameLength; ++sample)
  {
    ASSERT_EQ(buffer[sample], 0u);
  }

  delete bundleSequencer;
}

TEST_F(IasRtProcessingFwTest, invalidCalls)
{
  IasBundleSequencer *bundleSequencer = new IasBundleSequencer();
  ASSERT_TRUE(bundleSequencer != nullptr);
  IasAudioProcessingResult result = bundleSequencer->init(nullptr);
  ASSERT_EQ(eIasAudioProcInvalidParam, result);
}


}
