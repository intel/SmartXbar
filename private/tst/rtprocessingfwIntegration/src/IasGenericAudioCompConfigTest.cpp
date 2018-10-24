/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasGenericAudioCompConfigTest.cpp
 * @brief  Contains some test cases for the IasGenericAudioCompConfig class
 */

#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "IasRtProcessingFwTest.hpp"

namespace IasAudio {

TEST_F(IasRtProcessingFwTest, GenericAudioCompConfigTest)
{
  IasGenericAudioCompConfig *config = new IasGenericAudioCompConfig();
  ASSERT_TRUE(config != nullptr);
  std::string pinName;
  IasAudioProcessingResult res = config->getPinName(1234, pinName);
  ASSERT_EQ(eIasAudioProcInvalidParam, res);

  delete config;
}

TEST_F(IasRtProcessingFwTest, addStreamMapping)
{
  IasGenericAudioCompConfig *config = new IasGenericAudioCompConfig();
  ASSERT_TRUE(config != nullptr);

  IasAudioStream *inputStream = new IasAudioStream("InStream", 1234, 2, IasBaseAudioStream::eIasAudioStreamInput, false);
  ASSERT_TRUE(inputStream != nullptr);

  IasAudioStream *outputStream = new IasAudioStream("OutStream", 4321, 2, IasBaseAudioStream::eIasAudioStreamOutput, false);
  ASSERT_TRUE(inputStream != nullptr);

  config->addStreamMapping(inputStream, "InPin", outputStream, "outPin");
  config->addStreamMapping(inputStream, "InPin", outputStream, "outPin");

  delete outputStream;
  delete inputStream;
  delete config;
}

TEST_F(IasRtProcessingFwTest, addStreamToProcess)
{
  IasGenericAudioCompConfig *config = new IasGenericAudioCompConfig();
  ASSERT_TRUE(config != nullptr);

  IasBundleSequencer bundleSequencer;
  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(128);
  env->setSampleRate(48000);
  IasAudioProcessingResult result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *inputStream = new IasAudioStream("InStream", 1234, 2, IasBaseAudioStream::eIasAudioStreamInput, false);
  ASSERT_TRUE(inputStream != nullptr);
  inputStream->setBundleSequencer(&bundleSequencer);
  result = inputStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *inputStream2 = new IasAudioStream("InStream2", 1235, 2, IasBaseAudioStream::eIasAudioStreamInput, false);
  ASSERT_TRUE(inputStream2 != nullptr);
  inputStream2->setBundleSequencer(&bundleSequencer);
  result = inputStream2->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *inputStream3 = new IasAudioStream("InStream3", 1236, 2, IasBaseAudioStream::eIasAudioStreamInput, false);
  ASSERT_TRUE(inputStream3 != nullptr);
  inputStream3->setBundleSequencer(&bundleSequencer);
  result = inputStream3->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  config->addStreamToProcess(inputStream, "InPin");
  config->addStreamToProcess(inputStream, "InPin");
  config->addStreamToProcess(inputStream2, "InPin2");
  config->addStreamToProcess(inputStream3, "InPin3");

  delete inputStream;
  delete config;
}


}
