/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChainTest.cpp
 * @date   Sep20, 2012
 */

#include "IasRtProcessingFwTest.hpp"
#include "IasTestComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioPlugin.hpp"

namespace IasAudio {

class IasTestCompCmd;

// begin IasAudioChain test cases //

TEST_F(IasRtProcessingFwTest, AudioChainTestAudioChain)
{
  IasAudioStream* inputStream;
  IasAudioStream* outputStream;
  IasAudioStream* intermediateInStream;
  IasAudioStream* intermediateOutStream;

  IasAudioChain *audioChain = new IasAudioChain();
  ASSERT_TRUE(audioChain != NULL);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = 192;
  initParams.sampleRate = 48000;
  IasAudioChain::IasResult chainres = audioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  IasGenericAudioCompConfig *testCompConfig = new IasGenericAudioCompConfig();
  ASSERT_TRUE(testCompConfig != NULL);

  inputStream  = audioChain->createInputAudioStream("inStream1", 0 /*identifier*/, 2 /*num Channels*/,false);
  ASSERT_TRUE(inputStream != NULL);
  outputStream = audioChain->createOutputAudioStream("outStream1", 0 /*identifier*/, 3 /*num Channels*/,false);
  ASSERT_TRUE(outputStream != NULL);
  intermediateInStream = audioChain->createIntermediateInputAudioStream("intermediateInStream1", 0 /*identifier*/, 3 /*num Channels*/,false);
  ASSERT_TRUE(intermediateInStream != NULL);
  intermediateOutStream = audioChain->createIntermediateOutputAudioStream("intermediateOutStream1", 1 /*identifier*/, 3 /*num Channels*/,false);
  ASSERT_TRUE(intermediateOutStream != NULL);

  testCompConfig->addStreamToProcess(inputStream, "inStream1");

  /********************************************
   *  Start test CompConfig setProperty methods
   ********************************************/
  const IasProperties &properties = testCompConfig->getProperties();
  std::string testFloat32Key("testFloat32Key");
  float testFloat32 = 0.0f;
  properties.get(testFloat32Key, &testFloat32);

  std::string testInt32Key("testInt32Key");
  int32_t testInt32 = 0;
  properties.get(testInt32Key, &testInt32);

  std::string testStringKey("testStringKey");
  std::string testString("testString");
  properties.get(testStringKey, &testString);

  /******************************************
   *  End test CompConfig setProperty methods
   ******************************************/

  // Test of the IasGenericAudioCompConfig::setModuleId and ::getModuleId functions.
  testCompConfig->setModuleId(42);
  int32_t moduleId = testCompConfig->getModuleId();
  ASSERT_EQ(moduleId, 42);

  IasGenericAudioComp* testComp = new IasModule<IasTestCompCore, IasTestCompCmd>(testCompConfig, "testComp", "MyTestComp");
  ASSERT_TRUE(testComp != nullptr);

  IasAudioCompState state = testComp->getCore()->getProcssingState();
  ASSERT_TRUE(state == eIasAudioCompEnabled);
  testComp->getCore()->disableProcessing();
  state = testComp->getCore()->getProcssingState();
  ASSERT_TRUE(state == eIasAudioCompDisabled);

  int32_t componentIndex = testComp->getCore()->getComponentIndex();
  ASSERT_TRUE(componentIndex == -1);

  audioChain->addAudioComponent(testComp);


  const IasAudioStreamVector& inputStreamVec = audioChain->getInputStreams();
  ASSERT_TRUE(inputStreamVec[0] == inputStream);

  const IasAudioStreamVector& outputStreamVec = audioChain->getOutputStreams();
  ASSERT_TRUE(outputStreamVec[0] == outputStream);

  const IasAudioStreamVector& intermediateInStreamVec = audioChain->getIntermediateInputStreams();
  ASSERT_TRUE(intermediateInStreamVec[0] == intermediateInStream);

  const IasAudioStreamVector& intermediateOutStreamVec = audioChain->getIntermediateOutputStreams();
  ASSERT_TRUE(intermediateOutStreamVec[0] == intermediateOutStream);

  const IasAudioStream* outputStreamId0  = audioChain->getOutputStream(0);
  ASSERT_TRUE(outputStreamId0 != nullptr);

  const IasAudioStream* outputStreamId42 = audioChain->getOutputStream(42);
  ASSERT_TRUE(outputStreamId42 == nullptr);

  const IasGenericAudioCompVector& audioComp = audioChain->getAudioComponents();
  ASSERT_TRUE(audioComp[0] == testComp);

  audioChain->process();

  audioChain->clearOutputBundleBuffers();

  delete audioChain;


  // Test of the toString() functions

  std::cout << "Possible enum values of IasAudioChain::IasResult:" << std::endl;
  std::cout << "  " << toString(IasAudioChain::eIasOk)     << std::endl;
  std::cout << "  " << toString(IasAudioChain::eIasFailed)    << std::endl;
}

TEST_F(IasRtProcessingFwTest, AudioChainTestInvalidCalls)
{
  IasAudioChain *audioChain = new IasAudioChain();
  ASSERT_TRUE(audioChain != nullptr);
  IasAudioStream *stream = audioChain->createInputAudioStream("Stream", 1234, 2, false);
  ASSERT_TRUE(stream == nullptr);
  stream = audioChain->createIntermediateInputAudioStream("Stream", 1234, 2, false);
  ASSERT_TRUE(stream == nullptr);
  stream = audioChain->createOutputAudioStream("Stream", 1234, 2, false);
  ASSERT_TRUE(stream == nullptr);
  stream = audioChain->createIntermediateOutputAudioStream("Stream", 1234, 2, false);
  ASSERT_TRUE(stream == nullptr);

  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = 16;
  initParams.sampleRate = 48000;
  IasAudioChain::IasResult chainres = audioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);
}



// end IasAudioChain test cases //
}
