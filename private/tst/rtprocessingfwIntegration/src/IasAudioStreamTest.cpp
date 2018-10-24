/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioStreamTest.cpp
 * @date   Sep 17, 2012
 * @brief  Contains all the integration tests for IasAudioStream class.
 */

#include "IasRtProcessingFwTest.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "model/IasAudioDevice.hpp"

namespace IasAudio {

// begin IasAudioStreamTest test cases //
TEST_F(IasRtProcessingFwTest, CreateAndInitAudioStreamTest)
{
  IasBundleSequencer bundleSequencer;

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   0 /*stream Id*/,
                                                   2 /*number channel*/,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);

  IasBaseAudioStream::IasAudioStreamTypes type = audioStream->getType();
  ASSERT_TRUE(IasBaseAudioStream::eIasAudioStreamInput == type);

  // Test of the set/get/remove/ConnectedDevice functions
  IasAudioDevice* device1 = nullptr;
  audioStream->setConnectedDevice(device1);
  IasAudioDevice* device2 = audioStream->getConnectedDevice();
  ASSERT_TRUE(device1 == device2);
  audioStream->removeConnectedDevice();
  IasAudioDeviceParamsPtr params = std::make_shared<IasAudioDeviceParams>();
  params->clockType = eIasClockProvided;
  params->dataFormat = eIasFormatInt16;
  params->name = "MyDummyDevice";
  params->numChannels = 2;
  params->numPeriods = 4;
  params->numPeriodsAsrcBuffer = 4;
  params->periodSize = 192;
  params->samplerate = 48000;
  device1 = new IasAudioDevice(params);
  ASSERT_TRUE(device1 != nullptr);
  audioStream->setConnectedDevice(device1);

  audioStream->setBundleSequencer(&bundleSequencer);

  delete audioStream;
}


TEST_F(IasRtProcessingFwTest, SetGetNameAudioStreamTest)
{
  IasBundleSequencer bundleSequencer;
  const std::string streamName = "testStream";
  std::string retStreamName;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream(streamName,
                                                   0 /*stream Id*/,
                                                   2 /*number channel*/,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  retStreamName = audioStream->getName();
  ASSERT_TRUE(retStreamName == streamName);

  IasBaseAudioStream::IasSampleLayout sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasUndefined, sampleLayout);
  IasBundledAudioStream *bundledStream = audioStream->asBundledStream();
  ASSERT_TRUE(bundledStream != NULL);
  sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasBundled, sampleLayout);

  int32_t sid = audioStream->getSid();
  ASSERT_TRUE(sid == 0);

  audioStream->setSid(1);
  sid = audioStream->getSid();
  ASSERT_TRUE(sid == 1);

  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, InterleavedInputStream)
{
  IasBundleSequencer bundleSequencer;
  const std::string streamName = "testStream";
  std::string retStreamName;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream(streamName,
                                                   0 /*stream Id*/,
                                                   2 /*number channel*/,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  IasAudioFrame *checkAudioFrame = nullptr;
  audioStream->getAudioDataPointers(&checkAudioFrame, nullptr);
  ASSERT_TRUE(checkAudioFrame == nullptr);

  retStreamName = audioStream->getName();
  ASSERT_TRUE(retStreamName == streamName);

  IasBaseAudioStream::IasSampleLayout sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasUndefined, sampleLayout);
  IasSimpleAudioStream *interleavedStream = audioStream->asInterleavedStream();
  ASSERT_TRUE(interleavedStream != NULL);
  sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasInterleaved, sampleLayout);

  IasBundledAudioStream *bundledAudioStream = audioStream->asBundledStream();
  ASSERT_TRUE(bundledAudioStream != NULL);

  audioStream->copyFromInputAudioChannels();
  IasAudioFrame* inputAudioFrame = audioStream->getInputAudioFrame();
  (*inputAudioFrame)[0] = new float[cIasTestAudioFrameLength];
  (*inputAudioFrame)[1] = new float[cIasTestAudioFrameLength];
  interleavedStream = audioStream->asInterleavedStream();
  ASSERT_TRUE(interleavedStream != NULL);

  delete [] (*inputAudioFrame)[0];
  delete [] (*inputAudioFrame)[1];
  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, NonInterleavedInputStream)
{
  IasBundleSequencer bundleSequencer;
  const std::string streamName = "testStream";
  std::string retStreamName;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream(streamName,
                                                   0 /*stream Id*/,
                                                   2 /*number channel*/,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  retStreamName = audioStream->getName();
  ASSERT_TRUE(retStreamName == streamName);

  IasBaseAudioStream::IasSampleLayout sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasUndefined, sampleLayout);
  IasSimpleAudioStream *nonInterleavedStream = audioStream->asNonInterleavedStream();
  ASSERT_TRUE(nonInterleavedStream != NULL);
  sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasNonInterleaved, sampleLayout);

  audioStream->copyFromInputAudioChannels();
  IasAudioFrame* inputAudioFrame = audioStream->getInputAudioFrame();
  (*inputAudioFrame)[0] = new float[cIasTestAudioFrameLength];
  (*inputAudioFrame)[1] = new float[cIasTestAudioFrameLength];
  nonInterleavedStream = audioStream->asNonInterleavedStream();
  ASSERT_TRUE(nonInterleavedStream != NULL);

  delete [] (*inputAudioFrame)[0];
  delete [] (*inputAudioFrame)[1];
  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, InterleavedOutputStream)
{
  IasBundleSequencer bundleSequencer;
  const std::string streamName = "testStream";
  std::string retStreamName;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream(streamName,
                                                   0 /*stream Id*/,
                                                   2 /*number channel*/,
                                                   IasBaseAudioStream::eIasAudioStreamOutput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  retStreamName = audioStream->getName();
  ASSERT_TRUE(retStreamName == streamName);

  IasBaseAudioStream::IasSampleLayout sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasUndefined, sampleLayout);
  IasSimpleAudioStream *interleavedStream = audioStream->asInterleavedStream();
  ASSERT_TRUE(interleavedStream != NULL);
  sampleLayout = audioStream->getSampleLayout();
  ASSERT_EQ(IasBaseAudioStream::eIasInterleaved, sampleLayout);

  IasAudioFrame* outputAudioFrame = audioStream->getOutputAudioFrame();
  (*outputAudioFrame)[0] = new float[cIasTestAudioFrameLength];
  (*outputAudioFrame)[1] = new float[cIasTestAudioFrameLength];
  audioStream->copyToOutputAudioChannels();
  interleavedStream = audioStream->asInterleavedStream();
  ASSERT_TRUE(interleavedStream != NULL);

  delete [] (*outputAudioFrame)[0];
  delete [] (*outputAudioFrame)[1];
  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, GetIdAudioStreamTest)
{
  IasBundleSequencer bundleSequencer;
  const uint32_t streamId = 42;
  uint32_t retStreamId;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   streamId,
                                                   2 /*number channel*/,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  retStreamId = audioStream->getId();
  ASSERT_TRUE(retStreamId == streamId);

  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, GetNumChannelsAudioStreamTest)
{
  IasBundleSequencer bundleSequencer;
  const uint32_t numChannels = 2;
  uint32_t retNumChannels;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   0 /*stream Id*/,
                                                   numChannels,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  retNumChannels = audioStream->getNumberChannels();
  ASSERT_TRUE(retNumChannels == numChannels);

  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, GetAudioFrameTest)
{
  IasBundleSequencer bundleSequencer;
  const uint32_t numChannels = 2;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   0 /*stream Id*/,
                                                   numChannels,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  IasAudioFrame audioFrame;
  ASSERT_TRUE(audioFrame.size() == 0);
  audioStream->asBundledStream()->getAudioFrame(&audioFrame);
  ASSERT_TRUE(audioFrame.size() == 2);

  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, GetBundleAssignmentAudioStreamTest)
{
  IasBundleSequencer bundleSequencer;
  const uint32_t numChannels = 6;
  IasAudioProcessingResult result;

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   0 /*stream Id*/,
                                                   numChannels,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);


  const IasBundleAssignmentVector& mBundleAssignmentVector = audioStream->asBundledStream()->getBundleAssignments();
  // must be to bundles, because one bundle can only carry 4 channels
  ASSERT_TRUE(mBundleAssignmentVector.size() == 2);

  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, writeReadDataAudioStreamTest)
{
  IasAudioProcessingResult result;
  IasBundleSequencer bundleSequencer;
  const uint32_t numChannels = 4;// this test is only for four channels. Do not change the number of channels without further adaptations.
  float writeTestData1[cIasTestAudioFrameLength];
  float writeTestData2[cIasTestAudioFrameLength];
  float writeTestData3[cIasTestAudioFrameLength];
  float writeTestData4[cIasTestAudioFrameLength];

  float readTestData1[cIasTestAudioFrameLength];
  float readTestData2[cIasTestAudioFrameLength];
  float readTestData3[cIasTestAudioFrameLength];
  float readTestData4[cIasTestAudioFrameLength];
  IasAudioFrame audioFrame;
  IasAudioFrame retAudioFrame;

  for (uint32_t sampCnt = 0; sampCnt < cIasTestAudioFrameLength; sampCnt++)
  {
    writeTestData1[sampCnt] = static_cast<float>(sampCnt);
    writeTestData2[sampCnt] = static_cast<float>(sampCnt + cIasTestAudioFrameLength);
    writeTestData3[sampCnt] = static_cast<float>(sampCnt + (2*cIasTestAudioFrameLength));
    writeTestData4[sampCnt] = static_cast<float>(sampCnt + (3*cIasTestAudioFrameLength));
  }

  audioFrame.push_back(writeTestData1); // test chan1
  audioFrame.push_back(writeTestData2); // test chan2
  audioFrame.push_back(writeTestData3); // test chan3
  audioFrame.push_back(writeTestData4); // test chan4

  retAudioFrame.push_back(readTestData1); // test chan1
  retAudioFrame.push_back(readTestData2); // test chan2
  retAudioFrame.push_back(readTestData3); // test chan3
  retAudioFrame.push_back(readTestData4); // test chan4

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   0 /*stream Id*/,
                                                   numChannels,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  result = audioStream->asBundledStream()->writeFromNonInterleaved(audioFrame);
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioStream->asBundledStream()->read(retAudioFrame);
  ASSERT_TRUE(result == eIasAudioProcOK);

  for (uint32_t sampCnt = 0; sampCnt < cIasTestAudioFrameLength; sampCnt++)
  {
    ASSERT_TRUE(writeTestData1[sampCnt] == readTestData1[sampCnt]);
    ASSERT_TRUE(writeTestData2[sampCnt] == readTestData2[sampCnt]);
    ASSERT_TRUE(writeTestData3[sampCnt] == readTestData3[sampCnt]);
    ASSERT_TRUE(writeTestData4[sampCnt] == readTestData4[sampCnt]);
  }

  delete audioStream;
}

TEST_F(IasRtProcessingFwTest, writeReadInterleavedDataAudioStreamTest)
{
  IasAudioProcessingResult result;
  IasBundleSequencer bundleSequencer;
  const uint32_t numChannels = 2; // this test is only for two channels. Do not change the number of channels without further adaptations.
  float writeTestData1[cIasTestAudioFrameLength];
  float writeTestData2[cIasTestAudioFrameLength];

  float readTestData[(numChannels*cIasTestAudioFrameLength)];

  IasAudioFrame audioFrame;

  for (uint32_t sampCnt = 0; sampCnt < cIasTestAudioFrameLength; sampCnt++)
  {
    writeTestData1[sampCnt] = static_cast<float>(sampCnt);
    writeTestData2[sampCnt] = static_cast<float>(sampCnt + cIasTestAudioFrameLength);
  }

  audioFrame.push_back(writeTestData1); // test chan1
  audioFrame.push_back(writeTestData2); // test chan2

  IasAudioChainEnvironmentPtr env = std::make_shared<IasAudioChainEnvironment>();
  env->setFrameLength(cIasTestAudioFrameLength);
  result = bundleSequencer.init(env);
  ASSERT_EQ(eIasAudioProcOK, result);

  IasAudioStream *audioStream = new IasAudioStream("Stream1",
                                                   0 /*stream Id*/,
                                                   numChannels,
                                                   IasBaseAudioStream::eIasAudioStreamInput,false);
  ASSERT_TRUE(audioStream != NULL);
  result = audioStream->init(env);
  ASSERT_EQ(eIasAudioProcOK, result);
  audioStream->setBundleSequencer(&bundleSequencer);

  result = audioStream->asBundledStream()->writeFromNonInterleaved(audioFrame);
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioStream->asBundledStream()->read(readTestData);
  ASSERT_TRUE(result == eIasAudioProcOK);

  float* data = readTestData;
  for (uint32_t sampCnt = 0; sampCnt < cIasTestAudioFrameLength; sampCnt++)
  {
    ASSERT_TRUE(writeTestData1[sampCnt] == *data);
    data++;
    ASSERT_TRUE(writeTestData2[sampCnt] == *data);
    data++;
  }

  delete audioStream;
}



}
