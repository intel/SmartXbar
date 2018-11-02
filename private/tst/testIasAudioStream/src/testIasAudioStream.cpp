/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    testIasAudioStream.cpp
 * @brief   Test application for IasAudioStream, especially for the bundle interleaver and deinterleaver
 * @date    May 08, 2013
 */

#include <math.h>
#include <malloc.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <gtest/gtest.h>

#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"

#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "rtprocessingfwx/IasAudioBufferPoolHandler.hpp"

/*
 *  Set PROFILE to 1 to enable perf measurement
 */
#define PROFILE 0

#if PROFILE
#include "avbaudiomodules/internal/audio/smartx_test_support/IasTimeStampCounter.hpp"
#endif

#define NUM_CHANNELS_STREAM1   6
#define NUM_CHANNELS_STREAM2   2
#define NUM_CHANNELS_STREAM3   2
#define NUM_CHANNELS_STREAM4   2

using namespace IasAudio;

static const uint32_t  cIasFrameLength =  64;
static const uint32_t  cIasSampleRate  =  44100;
static const uint32_t  cNumStreams     =  4;
static const float cNormalize       = 1.0f / (float)RAND_MAX;
static       uint32_t  cntFrames       =  0;

class IasAudioStreamTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};


int32_t main(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

TEST_F(IasAudioStreamTest, test1)
{
  /**
   * IasAudioBufferPoolHandler has 'static initialisation fiasco' issues, regarding the
   * construction/destruction order of the static globals and class statics from different compilational units.
   * It must be created in main()'s block scope, rather than file scope, otherwise it provokes a crash when the global
   * static destructors cleanup.
   */
  IasAudioBufferPoolHandler ringBufferPoolHandler;

  const uint32_t cNumChannels[cNumStreams] = { NUM_CHANNELS_STREAM1, NUM_CHANNELS_STREAM2, NUM_CHANNELS_STREAM3, NUM_CHANNELS_STREAM4 };
  float      difference[cNumStreams]   = { 0.0f, 0.0f, 0.0f, 0.0f };

  uint32_t nInputChannels = 0;
  for (uint32_t cntStreams = 0; cntStreams < cNumStreams; cntStreams++)
  {
    nInputChannels += cNumChannels[cntStreams];
  }

  float** ioFrame  = (float**)malloc(sizeof(float*) * nInputChannels);
  float** refFrame = (float**)malloc(sizeof(float*) * nInputChannels);

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    // 16-bytes aligned allocation of the channel buffers
    ioFrame[channels]  = (float* )memalign(16, sizeof(float)*cIasFrameLength);
    refFrame[channels] = (float* )memalign(16, sizeof(float)*cIasFrameLength);
  }

  IasAudioChain *myAudioChain = new IasAudioChain();
  ASSERT_TRUE(myAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;
  IasAudioChain::IasResult chainres = myAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);
  IasAudioStream *hSink1 = myAudioChain->createInputAudioStream("Sink1", 1, NUM_CHANNELS_STREAM1, false);
  IasAudioStream *hSink2 = myAudioChain->createInputAudioStream("Sink2", 2, NUM_CHANNELS_STREAM2, false);
  IasAudioStream *hSink3 = myAudioChain->createInputAudioStream("Sink3", 3, NUM_CHANNELS_STREAM3, false);
  IasAudioStream *hSink4 = myAudioChain->createInputAudioStream("Sink4", 4, NUM_CHANNELS_STREAM4, false);

  IasAudioFrame dataStream1, dataStream2, dataStream3, dataStream4;

#if PROFILE
  IasAudio::IasTimeStampCounter *timeStampCounter = new IasAudio::IasTimeStampCounter();
  uint64_t  cyclesSum   = 0;
#endif

  // Main processing loop.

  for (cntFrames = 0; cntFrames < 10000; cntFrames++)
  {
    // Fill the input frames with random data.
    for(uint32_t channel = 0; channel < nInputChannels; channel++)
    {
      for(uint32_t i=0; i < cIasFrameLength; i++)
      {
        ioFrame[channel][i]  = cNormalize * static_cast<float>(rand());
        refFrame[channel][i] = ioFrame[channel][i] ;
      }
    }

    dataStream1.resize(0);
    dataStream2.resize(0);
    dataStream3.resize(0);
    dataStream4.resize(0);

    uint32_t offset = 0;
    for (uint32_t channel = 0; channel < cNumChannels[0]; ++channel)
    {
      dataStream1.push_back(ioFrame[channel + offset]);
    }
    offset += cNumChannels[0];
    for (uint32_t channel = 0; channel < cNumChannels[1]; ++channel)
    {
      dataStream2.push_back(ioFrame[channel + offset]);
    }
    offset += cNumChannels[1];
    for (uint32_t channel = 0; channel < cNumChannels[2]; ++channel)
    {
      dataStream3.push_back(ioFrame[channel + offset]);
    }
    offset += cNumChannels[2];
    for (uint32_t channel = 0; channel < cNumChannels[3]; ++channel)
    {
      dataStream4.push_back(ioFrame[channel + offset]);
    }

#if PROFILE
    timeStampCounter->reset();
#endif

    // Execute the bundle interleaver
    hSink1->asBundledStream()->writeFromNonInterleaved(dataStream1);
    hSink2->asBundledStream()->writeFromNonInterleaved(dataStream2);
    hSink3->asBundledStream()->writeFromNonInterleaved(dataStream3);
    hSink4->asBundledStream()->writeFromNonInterleaved(dataStream4);

    // Execute the bundle deinterleaver
    hSink1->asBundledStream()->read(dataStream1);
    hSink2->asBundledStream()->read(dataStream2);
    hSink3->asBundledStream()->read(dataStream3);
    hSink4->asBundledStream()->read(dataStream4);


#if PROFILE
    uint32_t timeStamp = timeStampCounter->get();
    cyclesSum += static_cast<uint64_t>(timeStamp);
    printf("Time for one frame: %d clocks\n", timeStamp);
#endif

    for(uint32_t i=0; i< cIasFrameLength; i++)
    {
      offset = 0;
      for (uint32_t channel = 0; channel < cNumChannels[0]; ++channel)
      {
        ioFrame[channel + offset][i] = dataStream1[channel][i];
      }
      offset += cNumChannels[0];
      for (uint32_t channel = 0; channel < cNumChannels[1]; ++channel)
      {
        ioFrame[channel + offset][i] = dataStream2[channel][i];
      }
      offset += cNumChannels[1];
      for (uint32_t channel = 0; channel < cNumChannels[2]; ++channel)
      {
        ioFrame[channel + offset][i] = dataStream3[channel][i];
      }
      offset += cNumChannels[2];
      for (uint32_t channel = 0; channel < cNumChannels[3]; ++channel)
      {
        ioFrame[channel + offset][i] = dataStream4[channel][i];
      }
    }

    // Compare processed output samples with reference stream.
    uint32_t firstChannel = 0;
    for (uint32_t cntStreams = 0; cntStreams < cNumStreams; cntStreams++)
    {
      uint32_t nChannels = cNumChannels[cntStreams];
      for (uint32_t channel = 0; channel < nChannels; channel++)
      {
        for (uint32_t cnt = 0; cnt < cIasFrameLength; cnt++)
        {
          difference[cntStreams] = std::max(difference[cntStreams], (float)(fabs(ioFrame[firstChannel+channel][cnt] -
                                                                                        refFrame[firstChannel+channel][cnt])));
        }
      }
      firstChannel += nChannels;
    }

  }
  //end of main processing loop, start cleanup.
#if PROFILE
  printf("Average load: %f clocks/frame \n", ((double)cyclesSum/cntFrames));
#endif


  hSink4->asNonInterleavedStream()->writeFromNonInterleaved(dataStream4);
  hSink4->asInterleavedStream()->writeFromNonInterleaved(dataStream4);

  hSink4->asBundledStream()->writeFromNonInterleaved(dataStream4);
  hSink4->asInterleavedStream()->writeFromNonInterleaved(dataStream4);

  IasAudioFrame outFrame;
  for (unsigned int channel = 0; channel < cNumChannels[3]; ++channel)
  {
    outFrame.push_back((float*)malloc(sizeof(float) * cIasFrameLength));
  }
  hSink4->asNonInterleavedStream()->copyToOutputAudioChannels(outFrame);

  while (outFrame.begin() != outFrame.end())
  {
    free(outFrame.back());
    outFrame.pop_back();
  }

  /**
   * A bundled stream has four (4) channels interleaved in one large memory block.
   */
  const uint32_t cChannelsPerBundle = 4;
  float *memoryBlock = (float*)memalign(16, cChannelsPerBundle * sizeof(float) * cIasFrameLength);

  IasAudioFrame bundledStream;
  bundledStream.resize(0);
  for (unsigned int channel = 0; channel < cChannelsPerBundle; ++channel)
  {
    bundledStream.push_back(&memoryBlock[channel]);
  }
  std::string streamName("Sink5");
  IasAudioStream *hSink5 = myAudioChain->createInputAudioStream(streamName, 5, cChannelsPerBundle, false);
  hSink5->asNonInterleavedStream()->writeFromBundled(bundledStream);
  hSink5->asInterleavedStream()->writeFromBundled(bundledStream);

  free(memoryBlock);

  if (0 != strncmp(streamName.c_str(), hSink5->asNonInterleavedStream()->getName().c_str(), streamName.length()))
  {
    printf("Error stream name did not match expected [%s] != [%s]\n",
           streamName.c_str(),
           hSink5->asNonInterleavedStream()->getName().c_str());
  }

  if (IasBaseAudioStream::eIasNonInterleaved != hSink5->asNonInterleavedStream()->getSampleLayout())
  {
    printf("Error SampleLayout did not match eIasNonInterleaved.\n");
  }

  if (IasBaseAudioStream::eIasAudioStreamInput != hSink5->asNonInterleavedStream()->getType())
  {
    printf("Error Type did not match eIasAudioStreamInput.\n");
  }

  // Delete myAudioChain. The carEqualizer is deleted automatically, because it has been added to myAudioChain.
  delete(myAudioChain);

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    free(ioFrame[channels]);
    free(refFrame[channels]);
  }
  free(ioFrame);
  free(refFrame);

  printf("All frames have been processed.\n");

  for (uint32_t cntStreams = 0; cntStreams < cNumStreams; cntStreams++)
  {
    float differenceDB = 20.0f*static_cast<float>(log10(std::max(difference[cntStreams], (float)(1e-20))));
    printf("Stream %d (%d channels): peak difference between processed output data and reference: %7.2f dBFS\n",
           cntStreams+1, cNumChannels[cntStreams], differenceDB);
  }

  for (uint32_t cntStreams = 0; cntStreams < cNumStreams; cntStreams++)
  {
    float differenceDB = 20.0f*static_cast<float>(log10(std::max(difference[cntStreams], (float)(1e-20))));
    float thresholdDB  = -120.0f;

    if (differenceDB > thresholdDB)
    {
      printf("Error: Peak difference exceeds threshold of %7.2f dBFS.\n", thresholdDB);
    }
    ASSERT_TRUE(differenceDB <= thresholdDB);
  }

  printf("Test has been passed.\n");
  fflush(stdout);
}
