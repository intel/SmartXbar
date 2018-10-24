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
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "mixer/IasMixerCore.hpp"
#include "audio/mixerx/IasMixerCmd.hpp"

using namespace IasAudio;

#define ADD_ROOT(filename) "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-02/" filename

static const uint32_t cIasFrameLength    { 64 };
static const uint32_t cIasSampleRate     { 48000 };
static const uint32_t cNumOutputChannels { 4 };

int32_t main(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;
  DLT_REGISTER_APP("TST", "Test Application");
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}

namespace IasAudio {

class IasMixerSetBalanceTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();

    void setBalance(const char* pin, int32_t balance);
    void setFader(const char* pin, int32_t fader);

    IasIModuleId* mCmd = nullptr;
};

void IasMixerSetBalanceTest::SetUp()
{
}

void IasMixerSetBalanceTest::TearDown()
{
}

void IasMixerSetBalanceTest::setBalance(const char* pin, int32_t balance)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetBalance);
  cmdProperties.set<int32_t>("balance", balance);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

void IasMixerSetBalanceTest::setFader(const char* pin, int32_t fader)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetFader);
  cmdProperties.set<int32_t>("fader", fader);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

TEST_F(IasMixerSetBalanceTest, main_test)
{
  const char* inputFileName1  = ADD_ROOT("sin200.wav");
  const char* inputFileName2  = ADD_ROOT("sin400.wav");
  const char* inputFileName3  = ADD_ROOT("sin1200.wav");
  const char* outputFileName1 = "mixer_out0.wav";

  SNDFILE* inputFile1 = nullptr;
  SNDFILE* inputFile2 = nullptr;
  SNDFILE* inputFile3 = nullptr;
  SNDFILE* outputFile1 = nullptr;
  SF_INFO  inputFileInfo1;
  SF_INFO  inputFileInfo2;
  SF_INFO  inputFileInfo3;
  SF_INFO  outputFileInfo1;

  // Open the input files carrying PCM data.
  inputFileInfo1.format = 0;
  inputFile1 = sf_open(inputFileName1, SFM_READ, &inputFileInfo1);
  if (nullptr == inputFile1)
  {
    std::cout << "Could not open file " << inputFileName1 << std::endl;
    ASSERT_TRUE(false);
  }

  // Open the input files carrying PCM data.
  inputFileInfo2.format = 0;
  inputFile2 = sf_open(inputFileName2, SFM_READ, &inputFileInfo2);
  if (nullptr == inputFile2)
  {
    std::cout << "Could not open file " << inputFileName2 << std::endl;
    ASSERT_TRUE(false);
  }

  // Open the input files carrying PCM data.
  inputFileInfo3.format = 0;
  inputFile3 = sf_open(inputFileName3, SFM_READ, &inputFileInfo3);
  if (nullptr == inputFile3)
  {
    std::cout << "Could not open file " << inputFileName3 << std::endl;
    ASSERT_TRUE(false);
  }

  // Open the output files carrying PCM data.
  outputFileInfo1.samplerate = inputFileInfo1.samplerate;
  outputFileInfo1.channels   = cNumOutputChannels;
  outputFileInfo1.format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
  outputFile1 = sf_open(outputFileName1, SFM_WRITE, &outputFileInfo1);
  if (nullptr == outputFile1)
  {
    std::cout<<"Could not open file " <<outputFileName1 << std::endl;
    ASSERT_TRUE(false);
  }

  uint32_t nInputChannels = inputFileInfo1.channels + inputFileInfo2.channels + inputFileInfo3.channels;
  float*  ioData1 = (float*  )std::malloc(sizeof(float)  * cIasFrameLength * inputFileInfo1.channels);
  float*  ioData2 = (float*  )std::malloc(sizeof(float)  * cIasFrameLength * inputFileInfo2.channels);
  float*  ioData3 = (float*  )std::malloc(sizeof(float)  * cIasFrameLength * inputFileInfo3.channels);
  float** ioFrame = (float** )std::malloc(sizeof(float*) * nInputChannels);
  float*  outData = (float*  )std::malloc(sizeof(float)  * cIasFrameLength * cNumOutputChannels);

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    ioFrame[channels] = (float* )std::malloc(sizeof(float)*cIasFrameLength);
  }

  IasAudioChain* myAudioChain = new IasAudioChain();
  ASSERT_TRUE(myAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  IasAudioChain::IasResult chainres = myAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);
  auto* hSink0 = myAudioChain->createInputAudioStream("Sink0", 0, 2, false);
  auto* hSink1 = myAudioChain->createInputAudioStream("Sink1", 1, 2, false);
  auto* hSink2 = myAudioChain->createInputAudioStream("Sink2", 2, 2, false);
  auto* hSink3 = myAudioChain->createInputAudioStream("Sink3", 3, 2, false);
  auto* hSpeaker = myAudioChain->createOutputAudioStream("Speaker", 0, 4,false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  IasCmdDispatcherPtr cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEngine pluginEngine(cmdDispatcher);
  IasAudioProcessingResult procres = pluginEngine.loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  IasIGenericAudioCompConfig* config = nullptr;
  procres = pluginEngine.createModuleConfig(&config);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != config);

  config->addStreamToProcess(hSink0, "Sink0");
  config->addStreamToProcess(hSink1, "Sink1");
  config->addStreamToProcess(hSink2, "Sink2");
  config->addStreamToProcess(hSink3, "Sink3");
  config->addStreamToProcess(hSpeaker, "Speaker");

  config->addStreamMapping(hSink0, "Sink0", hSpeaker, "Speaker");
  config->addStreamMapping(hSink1, "Sink1", hSpeaker, "Speaker");
  config->addStreamMapping(hSink2, "Sink2", hSpeaker, "Speaker");
  config->addStreamMapping(hSink3, "Sink3", hSpeaker, "Speaker");

  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");
  activePins.push_back("Speaker");


  IasGenericAudioComp* myMixer;
  pluginEngine.createModule(config, "ias.mixer", "MyMixer", &myMixer);
  myAudioChain->addAudioComponent(myMixer);

  auto* mixerCore = myMixer->getCore();
  ASSERT_TRUE(nullptr != mixerCore);

  mCmd = myMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != mCmd);

  mixerCore->enableProcessing();

  uint32_t nReadSamples    = 0;
  uint32_t nWrittenSamples = 0;
  uint32_t frameCounter    = 0;

  IasAudioFrame dataStream0;
  IasAudioFrame dataStream1;
  IasAudioFrame dataStream2;

  do
  {
    // Fill all output bundles with zeros. This has to be done here, because
    // the mixer only adds the new samples to the content of the existing output
    // buffers. This is due to the bundle-based processing of the mixer.
    myAudioChain->clearOutputBundleBuffers();

    std::memset((void*)ioData1, 0, inputFileInfo1.channels * cIasFrameLength * sizeof(ioData1[0]));
    nReadSamples = (uint32_t) sf_readf_float(inputFile1, ioData1, cIasFrameLength);
    std::memset((void*)ioData2, 0, inputFileInfo2.channels * cIasFrameLength * sizeof(ioData2[0]));
    nReadSamples = (uint32_t) sf_readf_float(inputFile2, ioData2, cIasFrameLength);
    std::memset((void*)ioData3, 0, inputFileInfo3.channels * cIasFrameLength * sizeof(ioData3[0]));
    nReadSamples = (uint32_t) sf_readf_float(inputFile3, ioData3, cIasFrameLength);

    float* tempData1 = ioData1;
    float* tempData2 = ioData2;
    float* tempData3 = ioData3;

    for(uint32_t i=0; i< nReadSamples; i++)
    {
      ioFrame[0][i] = *tempData1++;
      ioFrame[1][i] = *tempData1++;
      ioFrame[2][i] = *tempData2++;
      ioFrame[3][i] = *tempData2++;
      ioFrame[4][i] = *tempData3++;
      ioFrame[5][i] = *tempData3++;
    }

    dataStream0.resize(0);
    dataStream1.resize(0);
    dataStream2.resize(0);
    dataStream0.push_back(ioFrame[0]);
    dataStream0.push_back(ioFrame[1]);
    dataStream1.push_back(ioFrame[2]);
    dataStream1.push_back(ioFrame[3]);
    dataStream2.push_back(ioFrame[4]);
    dataStream2.push_back(ioFrame[5]);
    hSink0->asBundledStream()->writeFromNonInterleaved(dataStream0);
    hSink1->asBundledStream()->writeFromNonInterleaved(dataStream1);
    hSink2->asBundledStream()->writeFromNonInterleaved(dataStream2);

    switch(frameCounter)
    {
      case 1000:
        setFader("Sink0", 140);

        break;
      case 6000:
        setFader("Sink2", -140);

        break;
      case 11000:
        setFader("Sink0", -140);
        break;
      case 16000:
        setFader("Sink2", 140);
        break;
      case 21000:
        setFader("Sink0", 0);
        break;
      case 26000:
        setFader("Sink2", 0);
        break;
    }

    mixerCore->process();


    hSpeaker->asBundledStream()->read(outData);
    tempData1 = ioData1;

    // Write the interleaved PCM samples into the output WAVE file.
    nWrittenSamples = (uint32_t) sf_writef_float(outputFile1, outData, (sf_count_t)nReadSamples);
    if (nWrittenSamples != nReadSamples)
    {
      std::cout<< "write to file " << outputFileName1 <<"failed" <<std::endl;
      ASSERT_TRUE(false);
    }
    frameCounter++;
  } while (nReadSamples == cIasFrameLength);


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
  if (sf_close(outputFile1))
  {
    std::cout<< "close of file " << outputFileName1 <<"failed" <<std::endl;
    ASSERT_TRUE(false);
  }

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    std::free(ioFrame[channels]);
    ioFrame[channels] = nullptr;
  }

  std::free(ioData1);
  std::free(ioData2);
  std::free(ioData3);
  std::free(ioFrame);
  std::free(outData);
  delete myMixer;
  delete myAudioChain;

  ioData1 = nullptr;
  ioData2 = nullptr;
  ioData3 = nullptr;
  ioFrame = nullptr;
  outData = nullptr;
  myMixer = nullptr;
  myAudioChain = nullptr;

  printf("test complete!\n");

}


} // namespace IasAudio
