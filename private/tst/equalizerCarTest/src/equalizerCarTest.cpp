/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
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

#define PROFILE 0
#define BENCHMARK 0
#define ADD_ROOT(filename) "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-02/" filename

static const int32_t NUM_CHANNELS_STREAM1 { 6 };
static const int32_t NUM_CHANNELS_STREAM2 { 2 };
static const int32_t NUM_CHANNELS_STREAM3 { 2 };
static const int32_t NUM_CHANNELS_STREAM4 { 2 };

static const uint32_t cIasFrameLength { 64 };
static const uint32_t cIasSampleRate  { 44100 };
static const uint32_t cSectionLength  { 5*cIasSampleRate/cIasFrameLength };  // approximately 5 seconds
static const uint32_t cNumStreams     { 4 };
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

struct IasAudioFilterParamsTest
{
  std::string streamId;
  int32_t  channelIdx;
  int32_t  filterIdx;
  int32_t  freq;
  int32_t  gain;
  int32_t  quality;
  int32_t  type;
  int32_t  order;
};

namespace IasAudio {

class IasEqualizerCarApp : public ::testing::Test
{
  /**
   * The same as IasAudioFilterParams, but typed as Int32 to make
   * it compatible with command interface, without section,
   * section is hard coded to one in cmd interface
   */

protected:
  virtual void SetUp();
  virtual void TearDown();

  IasIModuleId::IasResult setCarEqualizerFilterParams(IasAudioFilterParamsTest params);

  IasIModuleId::IasResult setCarEqualizerNumFilters(const char* pin,
                                                    int32_t channelIdx,
                                                    int32_t numFilters);

  IasIModuleId* mCmd = nullptr;
};

void IasEqualizerCarApp::SetUp()
{
}

void IasEqualizerCarApp::TearDown()
{
}

IasIModuleId::IasResult IasEqualizerCarApp::setCarEqualizerFilterParams(IasAudioFilterParamsTest params)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", params.streamId);
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams);
  cmdProperties.set<int32_t>("channelIdx", params.channelIdx);
  cmdProperties.set<int32_t>("filterId",   params.filterIdx);
  cmdProperties.set<int32_t>("freq",       params.freq);
  cmdProperties.set<int32_t>("gain",       params.gain);
  cmdProperties.set<int32_t>("quality",    params.quality);
  cmdProperties.set<int32_t>("type",       params.type);
  cmdProperties.set<int32_t>("order",      params.order);
  return mCmd->processCmd(cmdProperties, returnProperties);
}

IasIModuleId::IasResult IasEqualizerCarApp::setCarEqualizerNumFilters(const char* pin,
                                                  int32_t channelIdx, int32_t numFilters)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerNumFilters);
  cmdProperties.set<int32_t>("channelIdx", channelIdx);
  cmdProperties.set<int32_t>("numFilters", numFilters);

  return mCmd->processCmd(cmdProperties, returnProperties);
}

TEST_F(IasEqualizerCarApp, main_test)
{
  const char* inputFileName1     = ADD_ROOT("fft-noise_fft-noise_zeros.wav");
  const char* inputFileName2     = ADD_ROOT("fft-noise_fft-noise_zeros.wav");
  const char* inputFileName3     = ADD_ROOT("fft-noise_fft-noise_zeros.wav");
  const char* inputFileName4     = ADD_ROOT("Grummelbass_44100.wav");
  const char* outputFileName1    = "test_outstream1.wav";
  const char* outputFileName2    = "test_outstream2.wav";
  const char* outputFileName3    = "test_outstream3.wav";
  const char* outputFileName4    = "test_outstream4.wav";
  const char* referenceFileName1 = ADD_ROOT("reference_testIasCarEqualizer_outstream1.wav");
  const char* referenceFileName2 = ADD_ROOT("reference_testIasCarEqualizer_outstream2.wav");
  const char* referenceFileName3 = ADD_ROOT("reference_testIasCarEqualizer_outstream3.wav");
  const char* referenceFileName4 = ADD_ROOT("reference_testIasCarEqualizer_outstream4.wav");

  const char* inputFileName[cNumStreams]     = { inputFileName1, inputFileName2, inputFileName3, inputFileName4 };
  const char* outputFileName[cNumStreams]    = { outputFileName1, outputFileName2, outputFileName3, outputFileName4 };
  const char* referenceFileName[cNumStreams] = { referenceFileName1, referenceFileName2, referenceFileName3, referenceFileName4 };

  const uint32_t cNumChannels[cNumStreams] = { NUM_CHANNELS_STREAM1, NUM_CHANNELS_STREAM2, NUM_CHANNELS_STREAM3, NUM_CHANNELS_STREAM4 };
  float      difference[cNumStreams]   = { 0.0f, 0.0f, 0.0f, 0.0f };

  float  *ioData[cNumStreams];        // pointers to buffers with interleaved samples
  float  *referenceData[cNumStreams]; // pointers to buffers with interleaved samples

  SNDFILE* inputFile1     = nullptr;
  SNDFILE* inputFile2     = nullptr;
  SNDFILE* inputFile3     = nullptr;
  SNDFILE* inputFile4     = nullptr;
  SNDFILE* outputFile1    = nullptr;
  SNDFILE* outputFile2    = nullptr;
  SNDFILE* outputFile3    = nullptr;
  SNDFILE* outputFile4    = nullptr;
  SNDFILE* referenceFile1 = nullptr;
  SNDFILE* referenceFile2 = nullptr;
  SNDFILE* referenceFile3 = nullptr;
  SNDFILE* referenceFile4 = nullptr;

  SNDFILE* inputFile[cNumStreams]     = { inputFile1,  inputFile2,  inputFile3,  inputFile4 };
  SNDFILE* outputFile[cNumStreams]    = { outputFile1, outputFile2, outputFile3, outputFile4 };
  SNDFILE* referenceFile[cNumStreams] = { referenceFile1, referenceFile2, referenceFile3, referenceFile4 };

  SF_INFO  inputFileInfo[cNumStreams];
  SF_INFO  outputFileInfo[cNumStreams];
  SF_INFO  referenceFileInfo[cNumStreams];

  // Open the input files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
   inputFileInfo[cntFiles].format = 0;
   inputFile[cntFiles] = sf_open(inputFileName[cntFiles], SFM_READ, &inputFileInfo[cntFiles]);
   if (nullptr == inputFile[cntFiles])
   {
     std::cout<<"Could not open file " <<inputFileName[cntFiles] <<std::endl;
     ASSERT_TRUE(false);
   }
   if (inputFileInfo[cntFiles].channels != 2)
   {
     std::cout<<"File " <<inputFileName[cntFiles] << " does not provide stereo PCM samples " << std::endl;
   }
  }

  // Open the output files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
   outputFileInfo[cntFiles].samplerate = inputFileInfo[cntFiles].samplerate;
   outputFileInfo[cntFiles].channels   = cNumChannels[cntFiles];
   outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
   outputFile[cntFiles] = sf_open(outputFileName[cntFiles], SFM_WRITE, &outputFileInfo[cntFiles]);
   if (nullptr == outputFile[cntFiles])
   {
     std::cout<<"Could not open file " <<outputFileName[cntFiles] <<std::endl;
     ASSERT_TRUE(false);
   }
  }

  // Open the reference files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
   referenceFileInfo[cntFiles].format = 0;
   referenceFile[cntFiles] = sf_open(referenceFileName[cntFiles], SFM_READ, &referenceFileInfo[cntFiles]);
   if (nullptr == referenceFile[cntFiles])
   {
     std::cout<<"Could not open file " <<referenceFileName[cntFiles] <<std::endl;
     ASSERT_TRUE(false);
   }
   if (static_cast<uint32_t>(referenceFileInfo[cntFiles].channels) != cNumChannels[cntFiles])
   {
     std::cout<<"File " <<referenceFileName[cntFiles] << " does not provide stereo PCM samples " << std::endl;
     ASSERT_TRUE(false);
   }
  }


  uint32_t nInputChannels = 0;
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
   uint32_t maxChannels = std::max(static_cast<uint32_t>(inputFileInfo[cntFiles].channels), cNumChannels[cntFiles]);
   ioData[cntFiles]        = (float* )std::malloc(sizeof(float) * cIasFrameLength * maxChannels);
   referenceData[cntFiles] = (float* )std::malloc(sizeof(float) * cIasFrameLength * cNumChannels[cntFiles]);
   nInputChannels += cNumChannels[cntFiles];
  }
  float** ioFrame = (float**)std::malloc(sizeof(float*) * nInputChannels);

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
   ioFrame[channels]    = (float* )std::malloc(sizeof(float)*cIasFrameLength);
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

  auto cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEngine pluginEngine(cmdDispatcher);
  IasAudioProcessingResult procres = pluginEngine.loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  IasIGenericAudioCompConfig* config = nullptr;
  procres = pluginEngine.createModuleConfig(&config);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != config);

  IasProperties properties;
  properties.set("EqualizerMode",
          static_cast<int32_t>(IasEqualizer::IasEqualizerMode::eIasCar));

  properties.set("numFilterStagesMax", 10);

  config->addStreamToProcess(hSink1, "Sink1");
  config->addStreamToProcess(hSink2, "Sink2");
  config->addStreamToProcess(hSink3, "Sink3");
  config->addStreamToProcess(hSink4, "Sink4");

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

  mCmd = myEqualizer->getCmdInterface();

  IasAudioFilterParamsTest dataSetFilterParams;

  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerNumFilters("Sink1", 3, 3));

  dataSetFilterParams.streamId   = "Sink1";  // streamId
  dataSetFilterParams.channelIdx = 3;    // channelIdx
  dataSetFilterParams.filterIdx  = 0;    // filterIdx
  dataSetFilterParams.freq       = 80;   // frequency
  dataSetFilterParams.gain       = 120;  // gain / (dB/10)
  dataSetFilterParams.quality    = 10;   // quality / (1/10)
  dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeLowShelving); // type
  dataSetFilterParams.order      = 2;    // order
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

  dataSetFilterParams.streamId   = "Sink1";  // streamId
  dataSetFilterParams.channelIdx = 3;    // channelIdx
  dataSetFilterParams.filterIdx  = 1;    // filterIdx
  dataSetFilterParams.freq       = 1000; // frequency
  dataSetFilterParams.gain       = 120;  // gain / (dB/10)
  dataSetFilterParams.quality    = 10;   // quality / (1/10)
  dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypePeak); // type
  dataSetFilterParams.order      = 2;    // order
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

  dataSetFilterParams.streamId   = "Sink1";    // streamId
  dataSetFilterParams.channelIdx = 3;    // channelIdx
  dataSetFilterParams.filterIdx  = 2;    // filterIdx
  dataSetFilterParams.freq       = 10000;// frequency
  dataSetFilterParams.gain       = 120;  // gain / (dB/10)
  dataSetFilterParams.quality    = 10;   // quality / (1/10)
  dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeHighShelving); // type
  dataSetFilterParams.order      = 2;    // order
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerNumFilters("Sink1", 0, 3));

  dataSetFilterParams.streamId   = "Sink1";    // streamId
  dataSetFilterParams.channelIdx = 0;    // channelIdx
  dataSetFilterParams.filterIdx  = 0;    // filterIdx
  dataSetFilterParams.freq       = 250;  // frequency
  dataSetFilterParams.gain       = 120;  // gain / (dB/10)
  dataSetFilterParams.quality    = 10;   // quality / (1/10)
  dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeHighpass); // type
  dataSetFilterParams.order      = 5;    // order
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

  dataSetFilterParams.streamId   = "Sink1";    // streamId
  dataSetFilterParams.channelIdx = 0;    // channelIdx
  dataSetFilterParams.filterIdx  = 1;    // filterIdx
  dataSetFilterParams.freq       = 8000; // frequency
  dataSetFilterParams.gain       = 120;  // gain / (dB/10)
  dataSetFilterParams.quality    = 10;   // quality / (1/10)
  dataSetFilterParams.type       = (int32_t)eIasFilterTypeLowpass; // type
  dataSetFilterParams.order      = 5;    // order
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

  dataSetFilterParams.streamId   = "Sink1";    // streamId
  dataSetFilterParams.channelIdx = 0;    // channelIdx
  dataSetFilterParams.filterIdx  = 2;    // filterIdx
  dataSetFilterParams.freq       = 1000; // frequency
  dataSetFilterParams.gain       = -120; // gain / (dB/10)
  dataSetFilterParams.quality    = 40;   // quality / (1/10)
  dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypePeak); // type
  dataSetFilterParams.order      = 2;    // order
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerNumFilters("Sink4", 0, 2));
  EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerNumFilters("Sink4", 1, 2));


  equalizerCore->enableProcessing();

  uint32_t nReadSamples[cNumStreams] = { 0, 0, 0, 0 };
  uint32_t nWrittenSamples = 0;

  IasAudioFrame dataStream1, dataStream2, dataStream3, dataStream4;

  // Main processing loop.
  do
  {
    if ((cntFrames % (2*cSectionLength)) == 0)
    {
      // Set high-pass and low-pass filters for stream 4, channels 0+1
      dataSetFilterParams.streamId   = "Sink4";   // streamId
      dataSetFilterParams.channelIdx = 0;   // channelIdx
      dataSetFilterParams.filterIdx  = 0;   // filterIdx
      dataSetFilterParams.freq       = 300; // frequency
      dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeHighpass); // type
      dataSetFilterParams.order      = 4;    // order
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

      dataSetFilterParams.channelIdx = 1;   // channelIdx
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

      dataSetFilterParams.channelIdx = 0;    // channelIdx
      dataSetFilterParams.filterIdx  = 1;    // filterIdx
      dataSetFilterParams.freq       = 3400; // frequency
      dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeLowpass); // type
      dataSetFilterParams.order      = 4;    // order
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

      dataSetFilterParams.channelIdx = 1;  // channelIdx
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));
    }
    else if ((cntFrames % (2*cSectionLength)) == cSectionLength)
    {
      // Set high-pass and low-pass filters for stream 4, channels 0+1
      dataSetFilterParams.channelIdx = 0;   // channelIdx
      dataSetFilterParams.filterIdx  = 0;   // filterIdx
      dataSetFilterParams.freq       = 300; // frequency
      dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeFlat); // type
      dataSetFilterParams.order      = 2;    // order
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

      dataSetFilterParams.channelIdx = 1;   // channelIdx
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

      dataSetFilterParams.channelIdx = 0;    // channelIdx
      dataSetFilterParams.filterIdx  = 1;    // filterIdx
      dataSetFilterParams.freq       = 3400; // frequency
      dataSetFilterParams.type       = static_cast<int32_t>(eIasFilterTypeFlat); // type
      dataSetFilterParams.order      = 2;    // order
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));

      dataSetFilterParams.channelIdx = 1;   // channelIdx
      EXPECT_EQ(IasIModuleId::IasResult::eIasOk, setCarEqualizerFilterParams(dataSetFilterParams));
    }

    // Read in all channels from the WAVE file. The interleaved samples are
    // stored within the linear vector ioData.
    for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
    {
      std::memset((void*)ioData[cntFiles], 0, inputFileInfo[cntFiles].channels * cIasFrameLength * sizeof(ioData[0][0]));
      nReadSamples[cntFiles] = (uint32_t)sf_readf_float(inputFile[cntFiles], ioData[cntFiles], cIasFrameLength);
    }

    for(uint32_t i=0; i< cIasFrameLength; i++)
    {
      ioFrame[0][i]  = ioData[0][2*i];
      ioFrame[1][i]  = ioData[0][2*i+1];
      ioFrame[2][i]  = ioData[0][2*i];
      ioFrame[3][i]  = ioData[0][2*i+1];
      ioFrame[4][i]  = ioData[0][2*i];
      ioFrame[5][i]  = ioData[0][2*i+1];
      ioFrame[6][i]  = ioData[1][2*i];
      ioFrame[7][i]  = ioData[1][2*i+1];
      ioFrame[8][i]  = ioData[2][2*i];
      ioFrame[9][i]  = ioData[2][2*i+1];
      ioFrame[10][i] = ioData[3][2*i];
      ioFrame[11][i] = ioData[3][2*i+1];
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

#if PROFILE
    timeStampCounter->reset();
#endif

    // Call process method of audio component under test.
    equalizerCore->process();

#if PROFILE
    uint32_t timeStamp = timeStampCounter->get();
    cyclesSum += static_cast<uint64_t>(timeStamp);
    printf("Time for one frame: %d clocks\n", timeStamp);
#endif

    hSink1->asBundledStream()->read(dataStream1);
    hSink2->asBundledStream()->read(dataStream2);
    hSink3->asBundledStream()->read(dataStream3);
    hSink4->asBundledStream()->read(dataStream4);

    for(uint32_t i=0; i< cIasFrameLength; i++)
    {
      ioData[0][6*i  ] = 0.25f*dataStream1[0][i];
      ioData[0][6*i+1] = 0.25f*dataStream1[1][i];
      ioData[0][6*i+2] = 0.25f*dataStream1[2][i];
      ioData[0][6*i+3] = 0.25f*dataStream1[3][i];
      ioData[0][6*i+4] = 0.25f*dataStream1[4][i];
      ioData[0][6*i+5] = 0.25f*dataStream1[5][i];
      ioData[1][2*i  ] = 0.25f*dataStream2[0][i];
      ioData[1][2*i+1] = 0.25f*dataStream2[1][i];
      ioData[2][2*i  ] = 0.25f*dataStream3[0][i];
      ioData[2][2*i+1] = 0.25f*dataStream3[1][i];
      ioData[3][2*i  ] = 0.25f*dataStream4[0][i];
      ioData[3][2*i+1] = 0.25f*dataStream4[1][i];
    }

    for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
    {
      // Write the interleaved PCM samples into the output WAVE file.
      nWrittenSamples = (uint32_t) sf_writef_float(outputFile[cntFiles], ioData[cntFiles], (sf_count_t)nReadSamples[cntFiles]);
      if (nWrittenSamples != nReadSamples[cntFiles])
      {
        std::cout<< "write to file " << outputFileName[cntFiles] <<"failed" <<std::endl;
        ASSERT_TRUE(false);
      }

      // Read in all channels from the reference file. The interleaved samples are
      const auto nReferenceSamples = sf_readf_float(referenceFile[cntFiles], referenceData[cntFiles],
                                              static_cast<sf_count_t>(nReadSamples[cntFiles]));

      if (nReferenceSamples < nReadSamples[cntFiles])
      {
        printf("Error while reading reference PCM samples from file %s\n", referenceFileName[cntFiles]);
        ASSERT_TRUE(false);
      }

      // Compare processed output samples with reference stream.
      for (uint32_t cnt = 0; cnt < nReadSamples[cntFiles]; cnt++)
      {
        uint32_t nChannels = cNumChannels[cntFiles];
        for (uint32_t channel = 0; channel < nChannels; channel++)
        {
          difference[cntFiles] = std::max(difference[cntFiles], (float)(std::abs(ioData[cntFiles][cnt*nChannels+channel] -
                                                                                    referenceData[cntFiles][cnt*nChannels+channel])));
        }
      }
    }

    cntFrames++;

  } while ( (nReadSamples[0] == cIasFrameLength) ||
            (nReadSamples[1] == cIasFrameLength) ||
            (nReadSamples[2] == cIasFrameLength) ||
            (nReadSamples[3] == cIasFrameLength) );

  //end of main processing loop, start cleanup.

#if PROFILE
  printf("Average load: %f clocks/frame \n", ((double)cyclesSum/cntFrames));
#endif

  // Close all files.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    if (sf_close(inputFile[cntFiles]))
    {
      std::cout<< "close of file " << inputFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_TRUE(false);
    }
    if (sf_close(outputFile[cntFiles]))
    {
      std::cout<< "close of file " << outputFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_TRUE(false);
    }
    if (sf_close(referenceFile[cntFiles]))
    {
      std::cout<< "close of file " << referenceFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_TRUE(false);
    }
  }

  // Delete myAudioChain. The carEqualizer is deleted automatically,
  // because it has been added to myAudioChain.
  delete(myAudioChain);

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    std::free(ioData[cntFiles]);
    std::free(referenceData[cntFiles]);
  }

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    std::free(ioFrame[channels]);
  }
  std::free(ioFrame);

  printf("All frames have been processed.\n");

  bool result [cNumStreams];

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    float differenceDB = 20.0f*std::log10(std::max(difference[cntFiles], (float)(1e-20)));
    printf("Stream %d (%d channels): peak difference between processed output data and reference: %7.2f dBFS\n",
           cntFiles+1, cNumChannels[cntFiles], differenceDB);
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    float differenceDB = 20.0f*std::log10(std::max(difference[cntFiles], (float)(1e-20)));
    float thresholdDB  = -90.0f;

    if (differenceDB > thresholdDB)
    {
      std::cout << "Error  : Peak difference exceeds threshold of " << thresholdDB << "dBFS, in file: " << "test_outstream" << (cntFiles+1) << ".wav" << std::endl;
      result [cntFiles] = false;
    }

    else
    {
      std::cout << "Passed : Peak difference exceeds threshold of " << thresholdDB << "dBFS, in file: " << "test_outstream" << (cntFiles+1) << ".wav" << std::endl;
      result [cntFiles] = true;
    }
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    ASSERT_EQ(true, result[cntFiles]);
  }
  std::cout << "Test complete!" << std::endl;;

}


} // namespace IasAudio
