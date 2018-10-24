/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * equalizerCoreTest.cpp
 *
 *  Created on: August 2016
 */
#include <cstdio>
#include <cstring>
#include <iostream>

extern "C"  {
#include <sndfile.h>
}

#include <gtest/gtest.h>

#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

#include "equalizer/IasEqualizerCore.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"

using namespace IasAudio;

struct EqualizerParams
{
  int32_t  filterId;
  int32_t  freq;
  int32_t  quality;
  int32_t  type;
  int32_t  order;
};

#define BENCHMARK 0
#define ADD_ROOT(filename) "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-02/" filename

static const int32_t NUM_CHANNELS_STREAM1 { 6 };
static const int32_t NUM_CHANNELS_STREAM2 { 2 };
static const int32_t NUM_CHANNELS_STREAM3 { 2 };
static const int32_t NUM_CHANNELS_STREAM4 { 2 };

static const uint32_t cIasFrameLength { 64 };
static const uint32_t cIasSampleRate  { 44100 };
static const uint32_t cSectionLength  { 5*cIasSampleRate/cIasFrameLength };  // approximately 5 seconds
static       uint32_t cntFrames       { 0 };

int32_t main(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;
//  setenv("DLT_INITIAL_LOG_LEVEL", "::4", true);
  DLT_REGISTER_APP("TST", "Test Application");
//  DLT_ENABLE_LOCAL_PRINT();
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}

namespace IasAudio {

class IasEqualizerCoreTest : public ::testing::Test
{
protected:
  virtual void SetUp();
  virtual void TearDown();

  IasIModuleId::IasResult setEqualizer(const char* pin,
                                       int32_t filterId,
                                       int32_t targetGain);

  IasIModuleId::IasResult setEqualizerParams(const char* pin,
                                             const EqualizerParams params);

  IasIModuleId::IasResult setRampGradientSingleStream(const char* pin,
                                                      int32_t gradient);

  IasIModuleId* mCmd = nullptr;
};

void IasEqualizerCoreTest::SetUp()
{
}

void IasEqualizerCoreTest::TearDown()
{
}

IasIModuleId::IasResult IasEqualizerCoreTest::setRampGradientSingleStream(const char* pin, int32_t gradient)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetRampGradientSingleStream);
  cmdProperties.set<int32_t>("gradient", gradient);
  return mCmd->processCmd(cmdProperties, returnProperties);
}

IasIModuleId::IasResult IasEqualizerCoreTest::setEqualizer(const char* pin, int32_t filterId, int32_t targetGain)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<int32_t>("filterId", filterId);
  cmdProperties.set<int32_t>("gain", targetGain);
  return mCmd->processCmd(cmdProperties, returnProperties);
}

IasIModuleId::IasResult IasEqualizerCoreTest::setEqualizerParams(const char* pin,
                                                  const EqualizerParams params)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams);
  cmdProperties.set<int32_t>("filterId", params.filterId);
  cmdProperties.set<int32_t>("freq",     params.freq);
  cmdProperties.set<int32_t>("quality",  params.quality);
  cmdProperties.set<int32_t>("type",     params.type);
  cmdProperties.set<int32_t>("order",    params.order);

  return mCmd->processCmd(cmdProperties, returnProperties);
}

TEST_F(IasEqualizerCoreTest, main_test)
{
  const char* inputFileName1  = ADD_ROOT("fft-noise_fft-noise_zeros.wav");
  const char* inputFileName2  = ADD_ROOT("fft-noise_fft-noise_zeros.wav");
  const char* inputFileName3  = ADD_ROOT("fft-noise_fft-noise_zeros.wav");
  const char* inputFileName4  = ADD_ROOT("Grummelbass_44100.wav");
  const char* outputFileName1 = "test_outstream1.wav";
  const char* outputFileName2 = "test_outstream2.wav";
  const char* outputFileName3 = "test_outstream3.wav";
  const char* outputFileName4 = "test_outstream4.wav";


  // Define the parameters of the filter cascades that shall be used for the individual streams.
  std::vector<IasAudioFilterParams> filterParamsTableStream1a;
  std::vector<IasAudioFilterParams> filterParamsTableStream1b;
  std::vector<IasAudioFilterParams> filterParamsTableStream2;
  std::vector<IasAudioFilterParams> filterParamsTableStream3;
  std::vector<IasAudioFilterParams> filterParamsTableStream4;

  std::vector<uint32_t> channelIdTableStream1a;
  std::vector<uint32_t> channelIdTableStream1b;
  std::vector<uint32_t> channelIdTableStream2;
  std::vector<uint32_t> channelIdTableStream3;
  std::vector<uint32_t> channelIdTableStream4;


  // -----------------------------------------------------------------------------------------------------------------
  //                                                          freq,  gain, gain_dB, qual, type,                   order, unused
  // -----------------------------------------------------------------------------------------------------------------


  // Parameters for stream 1, channel 0
  filterParamsTableStream1a.push_back(IasAudioFilterParams{ 200, 4.00f, 1.0f, eIasFilterTypeLowpass,      5, 0});

  // Parameters for stream 1, channels 1-5
  filterParamsTableStream1b.push_back(IasAudioFilterParams{ 200, 4.00f, 1.0f, eIasFilterTypeHighpass,     5, 0});
  filterParamsTableStream1b.push_back(IasAudioFilterParams{8000, 4.00f, 1.0f, eIasFilterTypeHighShelving, 2, 0});

  // Parameters for stream 2, all channels
  filterParamsTableStream2.push_back(IasAudioFilterParams{ 200, 4.00f, 4.0f, eIasFilterTypePeak,         2, 0});
  filterParamsTableStream2.push_back(IasAudioFilterParams{5000, 4.00f, 1.0f, eIasFilterTypePeak,         2, 0});

  // Parameters for stream 3, all channels
  filterParamsTableStream3.push_back(IasAudioFilterParams{ 200, 0.25f,  4.0f, eIasFilterTypePeak,         2, 0});
  filterParamsTableStream3.push_back(IasAudioFilterParams{5000, 0.25f,  1.0f, eIasFilterTypePeak,         2, 0});

  // Parameters for stream 4, all channels: High-pass filter, bass filter, and treble filter.
  // The gains of the bass filter and of the treble filter will be ramped up/down during the main processing loop
  const uint32_t cFilterIdStream4Bass   = 1;
  const uint32_t cFilterIdStream4Treble = 2;
  filterParamsTableStream4.push_back(IasAudioFilterParams{  20, 1.00f, 1.0f, eIasFilterTypeHighpass,     3, 0});
  filterParamsTableStream4.push_back(IasAudioFilterParams{ 100, 4.00f, 1.0f, eIasFilterTypeLowShelving,  2, 0});
  filterParamsTableStream4.push_back(IasAudioFilterParams{5000, 4.00f, 1.0f, eIasFilterTypeHighShelving, 2, 0});


  channelIdTableStream1a.push_back(0);
  channelIdTableStream1b.push_back(1);
  channelIdTableStream1b.push_back(2);
  channelIdTableStream1b.push_back(3);
  channelIdTableStream1b.push_back(4);
  channelIdTableStream1b.push_back(5);

  SNDFILE* inputFile1  = nullptr;
  SNDFILE* outputFile1 = nullptr;
  SNDFILE* inputFile2  = nullptr;
  SNDFILE* outputFile2 = nullptr;
  SNDFILE* inputFile3  = nullptr;
  SNDFILE* outputFile3 = nullptr;
  SNDFILE* inputFile4  = nullptr;
  SNDFILE* outputFile4 = nullptr;

  SF_INFO  inputFileInfo1;
  SF_INFO  outputFileInfo1;
  SF_INFO  inputFileInfo2;
  SF_INFO  outputFileInfo2;
  SF_INFO  inputFileInfo3;
  SF_INFO  outputFileInfo3;
  SF_INFO  inputFileInfo4;
  SF_INFO  outputFileInfo4;

  // Open the input files carrying PCM data.
  inputFileInfo1.format = 0;
  inputFile1 = sf_open(inputFileName1, SFM_READ, &inputFileInfo1);
  if (nullptr == inputFile1)
  {
    std::cout<<"Could not open file " <<inputFileName1 <<std::endl;
    ASSERT_TRUE(false);
  }
  if (inputFileInfo1.channels != 2)
  {
    std::cout<<"File file " <<inputFileName1 << " does not provide stereo PCM samples " << std::endl;
  }

  inputFileInfo2.format = 0;
  inputFile2 = sf_open(inputFileName2, SFM_READ, &inputFileInfo2);
  if (nullptr == inputFile2)
  {
    std::cout<<"Could not open file " <<inputFileName2 <<std::endl;
    ASSERT_TRUE(false);
  }
  if (inputFileInfo2.channels != 2)
  {
    std::cout<<"File file " <<inputFileName2 << " does not provide stereo PCM samples " << std::endl;
  }

  inputFileInfo3.format = 0;
  inputFile3 = sf_open(inputFileName3, SFM_READ, &inputFileInfo3);
  if (nullptr == inputFile3)
  {
    std::cout<<"Could not open file " <<inputFileName3 <<std::endl;
    ASSERT_TRUE(false);
  }
  if (inputFileInfo3.channels != 2)
  {
    std::cout<<"File file " <<inputFileName3 << " does not provide stereo PCM samples " << std::endl;
  }

  inputFileInfo4.format = 0;
  inputFile4 = sf_open(inputFileName4, SFM_READ, &inputFileInfo4);
  if (nullptr == inputFile4)
  {
    std::cout<<"Could not open file " <<inputFileName4 <<std::endl;
    ASSERT_TRUE(false);
  }
  if (inputFileInfo4.channels != 2)
  {
    std::cout<<"File file " <<inputFileName4 << " does not provide stereo PCM samples " << std::endl;
  }

  // Open the output files carrying PCM data.
  outputFileInfo1.samplerate = inputFileInfo1.samplerate;
  outputFileInfo1.channels   = NUM_CHANNELS_STREAM1;
  outputFileInfo1.format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
  outputFile1 = sf_open(outputFileName1, SFM_WRITE, &outputFileInfo1);
  if (nullptr == outputFile1)
  {
    std::cout<<"Could not open file " <<outputFileName1 <<std::endl;
    ASSERT_TRUE(false);
  }

  outputFileInfo2.samplerate = inputFileInfo2.samplerate;
  outputFileInfo2.channels   = NUM_CHANNELS_STREAM2;
  outputFileInfo2.format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
  outputFile2 = sf_open(outputFileName2, SFM_WRITE, &outputFileInfo2);
  if (nullptr == outputFile2)
  {
    std::cout<<"Could not open file " <<outputFileName2 <<std::endl;
    ASSERT_TRUE(false);
  }
  outputFileInfo3.samplerate = inputFileInfo3.samplerate;
  outputFileInfo3.channels   = NUM_CHANNELS_STREAM3;
  outputFileInfo3.format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
  outputFile3 = sf_open(outputFileName3, SFM_WRITE, &outputFileInfo3);
  if (nullptr == outputFile3)
  {
    std::cout<<"Could not open file " <<outputFileName3 <<std::endl;
    ASSERT_TRUE(false);
  }

  outputFileInfo4.samplerate = inputFileInfo4.samplerate;
  outputFileInfo4.channels   = NUM_CHANNELS_STREAM4;
  outputFileInfo4.format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
  outputFile4 = sf_open(outputFileName4, SFM_WRITE, &outputFileInfo4);
  if (nullptr == outputFile4)
  {
    std::cout<<"Could not open file " <<outputFileName4 <<std::endl;
    ASSERT_TRUE(false);
  }


  const uint32_t nInputChannels = NUM_CHANNELS_STREAM1 + NUM_CHANNELS_STREAM2 + NUM_CHANNELS_STREAM3 + NUM_CHANNELS_STREAM4;
  float*  ioData1 = (float* )std::malloc(sizeof(float) * cIasFrameLength * std::max(inputFileInfo1.channels, NUM_CHANNELS_STREAM1));
  float*  ioData2 = (float* )std::malloc(sizeof(float) * cIasFrameLength * std::max(inputFileInfo2.channels, NUM_CHANNELS_STREAM2));
  float*  ioData3 = (float* )std::malloc(sizeof(float) * cIasFrameLength * std::max(inputFileInfo3.channels, NUM_CHANNELS_STREAM3));
  float*  ioData4 = (float* )std::malloc(sizeof(float) * cIasFrameLength * std::max(inputFileInfo4.channels, NUM_CHANNELS_STREAM4));
  float** ioFrame = (float** )std::malloc(sizeof(float*) * nInputChannels);

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    ioFrame[channels]    = (float* )malloc(sizeof(float)*cIasFrameLength);
  }


  IasAudioChain* myAudioChain = new IasAudioChain();
  ASSERT_TRUE(myAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  IasAudioChain::IasResult chainres = myAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);
  auto* hSink1 = myAudioChain->createInputAudioStream("Sink1", 1, NUM_CHANNELS_STREAM1, false);
  auto* hSink2 = myAudioChain->createInputAudioStream("Sink2", 2, NUM_CHANNELS_STREAM2, false);
  auto* hSink3 = myAudioChain->createInputAudioStream("Sink3", 3, NUM_CHANNELS_STREAM3, false);
  auto* hSink4 = myAudioChain->createInputAudioStream("Sink4", 4, NUM_CHANNELS_STREAM4, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  const auto cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEngine pluginEngine(cmdDispatcher);
  IasAudioProcessingResult procres = pluginEngine.loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  IasIGenericAudioCompConfig* config = nullptr;
  procres = pluginEngine.createModuleConfig(&config);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != config);

  config->addStreamToProcess(hSink1, "Sink1");
  config->addStreamToProcess(hSink2, "Sink2");
  config->addStreamToProcess(hSink3, "Sink3");
  config->addStreamToProcess(hSink4, "Sink4");

  IasProperties properties;
  properties.set("EqualizerMode",
          static_cast<int32_t>(IasEqualizer::IasEqualizerMode::eIasUser));

  properties.set("numFilterStagesMax", 10);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");
  activePins.push_back("Sink4");

  config->setProperties(properties);

  IasGenericAudioComp* myEqualizer = nullptr;
  pluginEngine.createModule(config, "ias.equalizer", "MyEqualizer", &myEqualizer);
  myAudioChain->addAudioComponent(myEqualizer);

  auto* equalizerCore = myEqualizer->getCore();
  ASSERT_TRUE(nullptr != equalizerCore);

  equalizerCore->enableProcessing();

  mCmd = myEqualizer->getCmdInterface();

  uint32_t nReadSamples1   = 0;
  uint32_t nReadSamples2   = 0;
  uint32_t nReadSamples3   = 0;
  uint32_t nReadSamples4   = 0;
  uint32_t nWrittenSamples = 0;

  IasAudioFrame dataStream1, dataStream2, dataStream3, dataStream4;

#if BENCHMARK
  uint32_t cycles_high,cycles_low,cycles_high1,cycles_low1;
  uint64_t start,end,delta;
#endif

  const char* processingPin = "Sink4";

  EqualizerParams params;
  params.filterId = 0;   // filter band
  params.freq     = 50;              // frequency = 50 Hz
  params.quality  = 10;              // quality = 1.0
  params.type     = static_cast<int32_t>(eIasFilterTypeLowShelving); // type
  params.order    = 2;               // order

  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizerParams(processingPin, params));

  // Main processing loop.
   do
   {
     if ((cntFrames % (4*cSectionLength)) == 0)
     {
       // Set bass+treble filters for stream 4 to flat.
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Bass,   1));
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Treble, 1));

     }
     else if ((cntFrames % (4*cSectionLength)) == cSectionLength)
     {
       // Set bass filter for stream 4 to +12 dB.
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Bass,   122));
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Treble, 1));
     }
     else if ((cntFrames % (4*cSectionLength)) == 2*cSectionLength)
     {
       // Set bass+treble filters for stream 4 to flat.
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Bass,   1));
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Treble, 1));
     }
     else if ((cntFrames % (4*cSectionLength)) == 3*cSectionLength)
     {
       // Set treble filter for stream 4 to +12 dB.
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Bass,   1));
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setEqualizer(processingPin, cFilterIdStream4Treble, 122));
     }

     // After 40 seconds: decrease the speed for ramping the gain to 0.02 dB per frame.
     if (cntFrames == 8*cSectionLength)
     {
       EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setRampGradientSingleStream(processingPin, -1305));
     }

     // Read in all channels from the WAVE file. The interleaved samples are
     // stored within the linear vector ioData.
     std::memset((void*)ioData1, 0, inputFileInfo1.channels * cIasFrameLength * sizeof(ioData1[0]));
     nReadSamples1 = (uint32_t) sf_readf_float(inputFile1, ioData1, cIasFrameLength);
     std::memset((void*)ioData2, 0, inputFileInfo2.channels * cIasFrameLength * sizeof(ioData2[0]));
     nReadSamples2 = (uint32_t) sf_readf_float(inputFile2, ioData2, cIasFrameLength);
     std::memset((void*)ioData3, 0, inputFileInfo3.channels * cIasFrameLength * sizeof(ioData3[0]));
     nReadSamples3 = (uint32_t) sf_readf_float(inputFile3, ioData3, cIasFrameLength);
     std::memset((void*)ioData4, 0, inputFileInfo4.channels * cIasFrameLength * sizeof(ioData4[0]));
     nReadSamples4 = (uint32_t) sf_readf_float(inputFile4, ioData4, cIasFrameLength);

     for(uint32_t i=0; i< cIasFrameLength; i++)
     {
       ioFrame[0][i] = ioData1[2*i];
       ioFrame[1][i] = ioData1[2*i+1];
       ioFrame[2][i] = ioData1[2*i];
       ioFrame[3][i] = ioData1[2*i+1];
       ioFrame[4][i] = ioData1[2*i];
       ioFrame[5][i] = ioData1[2*i+1];
       ioFrame[6][i] = ioData2[2*i];
       ioFrame[7][i] = ioData2[2*i+1];
       ioFrame[8][i] = ioData3[2*i];
       ioFrame[9][i] = ioData3[2*i+1];
       ioFrame[10][i] = ioData4[2*i];
       ioFrame[11][i] = ioData4[2*i+1];
     }

     dataStream1.resize(0);
     dataStream2.resize(0);
     dataStream3.resize(0);
     dataStream4.resize(0);

     dataStream1.push_back(ioFrame[0]);
     dataStream1.push_back(ioFrame[1]);
     dataStream1.push_back(ioFrame[2]);
     dataStream1.push_back(ioFrame[3]);
     dataStream1.push_back(ioFrame[4]);
     dataStream1.push_back(ioFrame[5]);

     dataStream2.push_back(ioFrame[6]);
     dataStream2.push_back(ioFrame[7]);

     dataStream3.push_back(ioFrame[8]);
     dataStream3.push_back(ioFrame[9]);

     dataStream4.push_back(ioFrame[10]);
     dataStream4.push_back(ioFrame[11]);

     hSink1->asBundledStream()->writeFromNonInterleaved(dataStream1);
     hSink2->asBundledStream()->writeFromNonInterleaved(dataStream2);
     hSink3->asBundledStream()->writeFromNonInterleaved(dataStream3);
     hSink4->asBundledStream()->writeFromNonInterleaved(dataStream4);


     // call process of audio component under test.
 #if BENCHMARK
     uint64_t flags;
     preempt_disable();
     raw_local_irq_save(flags);
     asm volatile (
                   "RDTSC\n\t"
                   "mov %%edx, %0\n\t"
                   "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low));
 #endif
     equalizerCore->process();
 #if BENCHMARK
     raw_local_irq_restore(flags);
     preempt_enable();
     asm volatile (
                   "RDTSC\n\t"
                   "mov %%edx, %0\n\t"
                   "mov %%eax, %1\n\t": "=r" (cycles_high1), "=r" (cycles_low1));
     start = ( ((uint64_t)cycles_high << 32) | cycles_low );
     end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
     delta = end-start;
     //std::cout<< "delta = " <<delta <<std::endl;
 #endif

     hSink1->asBundledStream()->read(dataStream1);
     hSink2->asBundledStream()->read(dataStream2);
     hSink3->asBundledStream()->read(dataStream3);
     hSink4->asBundledStream()->read(dataStream4);

     for(uint32_t i=0; i< cIasFrameLength; i++)
     {
       ioData1[6*i  ] = 0.25f*dataStream1[0][i];
       ioData1[6*i+1] = 0.25f*dataStream1[1][i];
       ioData1[6*i+2] = 0.25f*dataStream1[2][i];
       ioData1[6*i+3] = 0.25f*dataStream1[3][i];
       ioData1[6*i+4] = 0.25f*dataStream1[4][i];
       ioData1[6*i+5] = 0.25f*dataStream1[5][i];
       ioData2[2*i  ] = 0.25f*dataStream2[0][i];
       ioData2[2*i+1] = 0.25f*dataStream2[1][i];
       ioData3[2*i  ] = 0.25f*dataStream3[0][i];
       ioData3[2*i+1] = 0.25f*dataStream3[1][i];
       ioData4[2*i  ] = 0.25f*dataStream4[0][i];
       ioData4[2*i+1] = 0.25f*dataStream4[1][i];
     }

     // Write the interleaved PCM samples into the output WAVE file.
     nWrittenSamples = (uint32_t) sf_writef_float(outputFile1, ioData1, (sf_count_t)nReadSamples1);
     if (nWrittenSamples != nReadSamples1)
     {
       std::cout<< "write to file " << outputFileName1 <<"failed" <<std::endl;
       ASSERT_TRUE(false);
     }
     nWrittenSamples = (uint32_t) sf_writef_float(outputFile2, ioData2, (sf_count_t)nReadSamples2);
     if (nWrittenSamples != nReadSamples2)
     {
       std::cout<< "write to file " << outputFileName2 <<"failed" <<std::endl;
       ASSERT_TRUE(false);
     }
     nWrittenSamples = (uint32_t) sf_writef_float(outputFile3, ioData3, (sf_count_t)nReadSamples3);
     if (nWrittenSamples != nReadSamples3)
     {
       std::cout<< "write to file " << outputFileName3 <<"failed" <<std::endl;
       ASSERT_TRUE(false);
     }
     nWrittenSamples = (uint32_t) sf_writef_float(outputFile4, ioData4, (sf_count_t)nReadSamples4);
     if (nWrittenSamples != nReadSamples4)
     {
       std::cout<< "write to file " << outputFileName4 <<"failed" <<std::endl;
       ASSERT_TRUE(false);
     }

     cntFrames++;

   } while ( (nReadSamples1 == cIasFrameLength) ||
             (nReadSamples2 == cIasFrameLength) ||
             (nReadSamples3 == cIasFrameLength) ||
             (nReadSamples4 == cIasFrameLength) );

   //end of main processing loop, start cleanup.


   // Close all files.
   if (sf_close(inputFile1))
   {
     std::cout<< "close of file " << inputFileName1 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(inputFile2))
   {
     std::cout<< "close of file " << inputFileName2 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(inputFile3))
   {
     std::cout<< "close of file " << inputFileName3 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(inputFile4))
   {
     std::cout<< "close of file " << inputFileName4 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(outputFile1))
   {
     std::cout<< "close of file " << outputFileName1 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(outputFile2))
   {
     std::cout<< "close of file " << outputFileName2 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(outputFile3))
   {
     std::cout<< "close of file " << outputFileName3 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }
   if (sf_close(outputFile4))
   {
     std::cout<< "close of file " << outputFileName4 <<"failed" <<std::endl;
     ASSERT_TRUE(false);
   }

   delete(equalizerCore);
   delete(myAudioChain);

   std::free(ioData1);
   std::free(ioData2);
   std::free(ioData3);
   std::free(ioData4);

   for(uint32_t channels = 0; channels < nInputChannels; channels++)
   {
     std::free(ioFrame[channels]);
   }
   std::free(ioFrame);

  (void) cFilterIdStream4Bass;
  (void) cFilterIdStream4Treble;

  std::cout << "test complete!" << std::endl;;

}


} // namespace IasAudio
