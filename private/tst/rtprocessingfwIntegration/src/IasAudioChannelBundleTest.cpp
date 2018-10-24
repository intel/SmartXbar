/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChannelBundleTest.cpp
 * @date   Sep 17, 2012
 * @brief Contains all the integration tests for the IasAudioChannelBundle class.
 */
#include "IasRtProcessingFwTest.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"

namespace IasAudio {

// begin IasAudioChannelBundle test cases //

TEST_F(IasRtProcessingFwTest, CreateAndDestroyAudioChannelBundle)
{
  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  ASSERT_TRUE(audioChannelBundle != NULL);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, CreateInitAndDestroyAudioChannelBundle)
{
  IasAudioProcessingResult result;
  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, CreateInitAndDestroyAudioChannelBundleNok)
{
  IasAudioProcessingResult result;
  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(0xFFFFFFFF);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcInitializationFailed);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, ReserveChannelsInAudioChannelBundle)
{
  IasAudioProcessingResult result;
  uint32_t index;
  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioChannelBundle->reserveChannels(cIasNumChannelsPerBundle, &index);
  ASSERT_TRUE(index == 0);
  ASSERT_TRUE(result == eIasAudioProcOK);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, ReserveToManyChannelsInAudioChannelBundle)
{
  IasAudioProcessingResult result;
  uint32_t index;
  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioChannelBundle->reserveChannels(cIasNumChannelsPerBundle+1, &index);
  ASSERT_TRUE(result == eIasAudioProcNoSpaceLeft);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, ResetAudioChannelBundle)
{
  uint32_t  index;
  uint32_t freeChannels;
  uint32_t usedChannels;
  IasAudioProcessingResult result;

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioChannelBundle->reserveChannels(cIasNumChannelsPerBundle, &index);
  ASSERT_TRUE(index == 0);
  ASSERT_TRUE(result == eIasAudioProcOK);

  freeChannels = audioChannelBundle->getNumFreeChannels();
  ASSERT_TRUE(freeChannels == 0);

  audioChannelBundle->reset();
  usedChannels = audioChannelBundle->getNumUsedChannels();
  ASSERT_TRUE(usedChannels == 0);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, GetDataPointerAudioChannelBundle)
{
  IasAudioProcessingResult result;
  float* audioData;

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  audioData = audioChannelBundle->getAudioDataPointer();
  ASSERT_TRUE(audioData != NULL);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, GetDataPointerWithoutInitAudioChannelBundle)
{
  float* audioData;

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);

  audioData = audioChannelBundle->getAudioDataPointer();
  ASSERT_TRUE(audioData == NULL);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, WriteOneAudioChannelBundle)
{
  uint32_t index;
  IasAudioProcessingResult result;
  float testData[cIasTestAudioFrameLength];
  float bundleData[cIasTestAudioFrameLength];

  // generate simple test data
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    testData[sampleCnt] = static_cast<float>(sampleCnt);
  }

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  index = audioChannelBundle->reserveChannels(cIasNumChannelsPerBundle, &index);
  ASSERT_TRUE(index == 0);
  ASSERT_TRUE(result == eIasAudioProcOK);

  // write test data to bundle
  audioChannelBundle->writeOneChannelFromNonInterleaved(index, testData);

  // read the written test data
  audioChannelBundle->readOneChannel(index, bundleData);

  // check if read data equals written data.
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    ASSERT_TRUE(testData[sampleCnt] == bundleData[sampleCnt]);
  }

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, WriteTwoAudioChannelBundle)
{
  uint32_t index;
  IasAudioProcessingResult result;
  float testData[cIasTestAudioFrameLength];
  float bundleDataCh1[cIasTestAudioFrameLength];
  float bundleDataCh2[cIasTestAudioFrameLength];

  // generate simple test data
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    testData[sampleCnt] = static_cast<float>(sampleCnt);
  }

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioChannelBundle->reserveChannels(cIasNumChannelsPerBundle, &index);
  ASSERT_TRUE(index == 0);
  ASSERT_TRUE(result == eIasAudioProcOK);

  // write test data to bundle
  audioChannelBundle->writeTwoChannelsFromNonInterleaved(index, testData, testData);

  // read the written test data
  audioChannelBundle->readTwoChannels(index, bundleDataCh1, bundleDataCh2);

  // check if read data equals written data.
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh1[sampleCnt]);
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh2[sampleCnt]);
  }

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, WriteThreeAudioChannelBundle)
{
  uint32_t index;
  IasAudioProcessingResult result;
  float testData[cIasTestAudioFrameLength];
  float bundleDataCh1[cIasTestAudioFrameLength];
  float bundleDataCh2[cIasTestAudioFrameLength];
  float bundleDataCh3[cIasTestAudioFrameLength];

  // generate simple test data
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    testData[sampleCnt] = static_cast<float>(sampleCnt);
  }

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  result = audioChannelBundle->reserveChannels(cIasNumChannelsPerBundle, &index);
  ASSERT_TRUE(index == 0);
  ASSERT_TRUE(result == eIasAudioProcOK);

  // write test data to bundle
  audioChannelBundle->writeThreeChannelsFromNonInterleaved(index, testData, testData, testData);

  // read the written test data
  audioChannelBundle->readThreeChannels(index, bundleDataCh1, bundleDataCh2, bundleDataCh3);

  // check if read data equals written data.
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh1[sampleCnt]);
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh2[sampleCnt]);
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh3[sampleCnt]);
  }

  // Simply call the API
  audioChannelBundle->writeThreeChannelsFromInterleaved(0, 0, testData);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, WriteFourAudioChannelBundle)
{
  IasAudioProcessingResult result;
  float testData[cIasTestAudioFrameLength];
  float bundleDataCh1[cIasTestAudioFrameLength];
  float bundleDataCh2[cIasTestAudioFrameLength];
  float bundleDataCh3[cIasTestAudioFrameLength];
  float bundleDataCh4[cIasTestAudioFrameLength];

  // generate simple test data
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    testData[sampleCnt] = static_cast<float>(sampleCnt);
  }

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  // write test data to bundle
  audioChannelBundle->writeFourChannelsFromNonInterleaved(testData, testData, testData, testData);

  // read the written test data
  audioChannelBundle->readFourChannels(bundleDataCh1, bundleDataCh2, bundleDataCh3, bundleDataCh4);

  // check if read data equals written data.
  for(uint32_t sampleCnt = 0; sampleCnt < cIasTestAudioFrameLength; sampleCnt++)
  {
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh1[sampleCnt]);
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh2[sampleCnt]);
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh3[sampleCnt]);
    ASSERT_TRUE(testData[sampleCnt] == bundleDataCh4[sampleCnt]);
  }

  // Simply call the API
  audioChannelBundle->writeFourChannelsFromInterleaved(0, testData);

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, ClearAudioChannelBundle)
{
  IasAudioProcessingResult result;
  uint32_t* audioData;

  IasAudioChannelBundle *audioChannelBundle = new IasAudioChannelBundle(cIasTestAudioFrameLength);
  result = audioChannelBundle->init();
  ASSERT_TRUE(result == eIasAudioProcOK);

  audioData = reinterpret_cast<uint32_t*>(audioChannelBundle->getAudioDataPointer());
  ASSERT_TRUE(audioData != NULL);

  // Set the content of the audio data buffer to a value != 0, in this case to 0x64646464
  memset(audioData, 0x64, sizeof(float)*cIasNumChannelsPerBundle*cIasTestAudioFrameLength);

  for (uint32_t sample = 0; sample < cIasNumChannelsPerBundle*cIasTestAudioFrameLength; ++sample)
  {
    ASSERT_EQ(audioData[sample], 0x64646464u);
  }

  audioChannelBundle->clear();

  for (uint32_t sample = 0; sample < cIasNumChannelsPerBundle*cIasTestAudioFrameLength; ++sample)
  {
    ASSERT_EQ(audioData[sample], 0u);
  }

  delete audioChannelBundle;
}

TEST_F(IasRtProcessingFwTest, CreateInvalidFrameLength)
{
  IasAudioProcessingResult result;

  for(int frameLength = 0; frameLength < 2048; frameLength++)
  {
    IasAudioChannelBundle audioChannelBundle(frameLength);
    result = audioChannelBundle.init();

#if INTEL_COMPILER
    if((frameLength % 8) != 0)
#else
    if((frameLength % 4) != 0)
#endif
    {
      EXPECT_NE(result, eIasAudioProcOK);
    }
    else
    {
      EXPECT_EQ(result, eIasAudioProcOK);
    }

  }
}


// end IasAudioChannelBundle test cases //
}

