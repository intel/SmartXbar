/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    mixerMultiChannelTest.cpp
 * @brief   Test application for the mixer with multi-channel input stream.
 * @date    August 2016
 *
 *  This test application implements the following mixer configuration with
 *  1 output zone.
 *  \verbatim
 *
 *  input streams                                    |  output stream
 *  -------------------------------------------------+------------------
 *  0 (one channel), 1(6 channels), 2(2 channels)    |  0 (6 channels)
 *
 *  \endverbatim
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>

extern "C"  {
#include <sndfile.h>
}

#include <gtest/gtest.h>

#include "avbaudiomodules/internal/audio/smartx_test_support/IasTimeStampCounter.hpp"
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

static const uint32_t cIasFrameLength    { 64 };
static const uint32_t cIasSampleRate     { 44100 };
static const uint32_t cSectionLength     { (3*cIasSampleRate)/cIasFrameLength };  // approximately 3 seconds
static const uint32_t cNumInputFiles     { 3 };
static const uint32_t cNumOutputFiles    { 1 };
static const uint32_t cNumOutputChannels[cNumOutputFiles] = {6};

/*
 *  Set PROFILE to 1 to enable perf measurement
 */
#define PROFILE 0
#define USE_REFERENCE 1

#define ADD_ROOT(filename) "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-02/" filename


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

class IasMixerMultiChannelTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();

    void setInputGainOffset(const char* pin, int32_t gain);
    void setBalance(const char* pin, int32_t balance);
    void setFader(const char* pin, int32_t fader);

    IasIModuleId *mCmd = nullptr;

};

void IasMixerMultiChannelTest::SetUp()
{
}

void IasMixerMultiChannelTest::TearDown()
{
}

void IasMixerMultiChannelTest::setInputGainOffset(const char* pin, int32_t gain)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetInputGainOffset);
  cmdProperties.set<int32_t>("gain", gain);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

void IasMixerMultiChannelTest::setFader(const char* pin, int32_t fader)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetFader);
  cmdProperties.set<int32_t>("fader", fader);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

void IasMixerMultiChannelTest::setBalance(const char* pin, int32_t balance)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetBalance);
  cmdProperties.set<int32_t>("balance", balance);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}


TEST_F(IasMixerMultiChannelTest, main_test)
{
  const char* inputFileName0  = ADD_ROOT("TA_mono_loop_44100.wav");
  const char* inputFileName1  = ADD_ROOT("Grummelbass_44100_6chan.wav");
  const char* inputFileName2  = ADD_ROOT("Pachelbel_inspired_44100.wav");

  const char* outputFileName0 = "mixer_out0.wav";
#if USE_REFERENCE
  const char* referenceFileName0 = ADD_ROOT("reference_mixerMultiChannelTest_out0.wav");
  const char* referenceFileName[cNumOutputFiles] = { referenceFileName0 };
#endif
  const char* inputFileName[cNumInputFiles]      = { inputFileName0, inputFileName1, inputFileName2 };
  const char* outputFileName[cNumOutputFiles]    = { outputFileName0 };

  SNDFILE* inputFile0     = nullptr;
  SNDFILE* inputFile1     = nullptr;
  SNDFILE* inputFile2     = nullptr;
  SNDFILE* outputFile0    = nullptr;
#if USE_REFERENCE
  SNDFILE *referenceFile0 = nullptr;
  SNDFILE *referenceFile[cNumOutputFiles] = { referenceFile0 };
  SF_INFO  referenceFileInfo[cNumOutputFiles];
#endif
  SNDFILE* inputFile[cNumInputFiles]      = { inputFile0,  inputFile1, inputFile2 };
  SNDFILE* outputFile[cNumOutputFiles]    = { outputFile0 };
  SF_INFO  inputFileInfo[cNumInputFiles];
  SF_INFO  outputFileInfo[cNumOutputFiles];


  const float gain[cNumInputFiles]  = { 0.8f, 0.125f, 0.25f };
#if USE_REFERENCE
  float difference[cNumOutputFiles] = { 0.0f };
#endif
  // Open the input files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    inputFileInfo[cntFiles].format = 0;
    inputFile[cntFiles] = sf_open(inputFileName[cntFiles], SFM_READ, &inputFileInfo[cntFiles]);
    if (nullptr == inputFile[cntFiles])
    {
      std::cout << "Could not open file " << inputFileName[cntFiles] << std::endl;
      ASSERT_TRUE(false);
    }
  }

  // Open the output files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    outputFileInfo[cntFiles].samplerate = cIasSampleRate; // use the global sample rate for all output files.
    outputFileInfo[cntFiles].channels   = cNumOutputChannels[cntFiles];
    outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
    outputFile[cntFiles] = sf_open(outputFileName[cntFiles], SFM_WRITE, &outputFileInfo[cntFiles]);

    if (nullptr == outputFile[cntFiles])
    {
      std::cout << "Could not open file " << outputFileName[cntFiles] << std::endl;
      ASSERT_TRUE(false);
    }
  }
#if USE_REFERENCE
  // Open the reference files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    referenceFileInfo[cntFiles].format = 0;
    referenceFile[cntFiles] = sf_open(referenceFileName[cntFiles], SFM_READ, &referenceFileInfo[cntFiles]);

    if (nullptr == referenceFile[cntFiles])
    {
      std::cout<<"Could not open file " << referenceFileName[cntFiles] << std::endl;
      ASSERT_TRUE(false);
    }

    if (static_cast<uint32_t>(referenceFileInfo[cntFiles].channels) != cNumOutputChannels[cntFiles])
    {
      std::cout << "Number of channels stored in file " << referenceFileName[cntFiles]
                << " does not match, expected: %d, actual: %d"
                << cNumOutputChannels[cntFiles] << referenceFileInfo[cntFiles].channels << std::endl;
      ASSERT_TRUE(false);
    }
  }
#endif
  uint32_t    nInputChannels = 0;
  float*  inputData[cNumInputFiles];      // pointers to buffers with interleaved samples
  float*  outputData[cNumOutputFiles];    // pointers to buffers with interleaved samples
#if USE_REFERENCE
  float  *referenceData[cNumOutputFiles]; // pointers to buffers with interleaved samples
#endif

  // Allocate the buffers with interleaved samples.
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    nInputChannels   += inputFileInfo[cntFiles].channels;
    inputData[cntFiles]  = (float*)std::malloc(sizeof(float) * cIasFrameLength * inputFileInfo[cntFiles].channels);
    if (nullptr == inputData[cntFiles])
    {
      std::cout << "Error while allocating inputData[%d] " << cntFiles << std::endl;
      ASSERT_TRUE(false);
    }
  }
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    outputData[cntFiles]  = (float*)std::malloc(sizeof(float) * cIasFrameLength * outputFileInfo[cntFiles].channels);

    if (nullptr == outputData[cntFiles])
    {
      std::cout << "Error while allocating outputData[%d] " << cntFiles << std::endl;
      ASSERT_TRUE(false);
    }
  }
#if USE_REFERENCE
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    referenceData[cntFiles]  = (float*)std::malloc(sizeof(float) * cIasFrameLength * referenceFileInfo[cntFiles].channels);
    if (nullptr == referenceData[cntFiles])
    {
      std::cout<<"Error while allocating referenceData[%d] " <<cntFiles <<std::endl;
      ASSERT_TRUE(false);
    }
  }
#endif

  // Allocate the inputFrame, which represents the samples of all audio channels
  // (organized as de-interleaved buffers).
  float** inputFrame = (float** )std::malloc(sizeof(float*) * nInputChannels);
  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    inputFrame[channels] = (float* )std::malloc(sizeof(float)*cIasFrameLength);
  }

  IasAudioChain* myAudioChain = new IasAudioChain();
  ASSERT_TRUE(myAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  IasAudioChain::IasResult chainres = myAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  // Each sink represents a logical source, which corresponds to one input file.
  IasAudioStream* hSink[cNumInputFiles];

  for (uint32_t cntStreams = 0; cntStreams < cNumInputFiles; cntStreams++)
  {
    hSink[cntStreams] = myAudioChain->createInputAudioStream(
    std::string{"Sink" + std::to_string(cntStreams)},
    cntStreams, inputFileInfo[cntStreams].channels, false);

    printf("Created input stream with %d channels!\n",inputFileInfo[cntStreams].channels);
  }

  // Each speaker represents one output zone, which corresponds to one output file.
  IasAudioStream *hSpeaker[cNumOutputFiles];
  for (uint32_t cntStreams = 0; cntStreams < cNumOutputFiles; cntStreams++)
  {
    hSpeaker[cntStreams] = myAudioChain->createOutputAudioStream(
        std::string{"Speaker" + std::to_string(cntStreams)},
        cntStreams, cNumOutputChannels[cntStreams],false);
  }

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  IasCmdDispatcherPtr cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEngine pluginEngine(cmdDispatcher);
  IasAudioProcessingResult procres = pluginEngine.loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  IasIGenericAudioCompConfig* myMixerConfiguration = nullptr;
  procres = pluginEngine.createModuleConfig(&myMixerConfiguration);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != myMixerConfiguration);


  myMixerConfiguration->addStreamMapping(hSink[0], "Sink0", hSpeaker[0], "Speaker0");
  myMixerConfiguration->addStreamMapping(hSink[1], "Sink1", hSpeaker[0], "Speaker0");
  myMixerConfiguration->addStreamMapping(hSink[2], "Sink2", hSpeaker[0], "Speaker0");

  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Speaker0");

  IasGenericAudioComp* myMixer;
  pluginEngine.createModule(myMixerConfiguration, "ias.mixer", "MyMixer", &myMixer);
  myAudioChain->addAudioComponent(myMixer);

  auto* mixerCore = myMixer->getCore();
  ASSERT_TRUE(nullptr != mixerCore);

  mCmd = myMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != mCmd);

  mixerCore->enableProcessing();

  uint32_t      frameCounter = 0;
  uint32_t      nReadSamples = 0;
  uint32_t      nReadSamplesThisFile = 0;
  IasAudioFrame    dataStream[cNumInputFiles]; // each input file represents one data stream

  const char* monoPin = "Sink0";
  const char* multichannelPin = "Sink1";
  const char* stereoPin = "Sink2";

#if PROFILE
  IasAudio::IasTimeStampCounter *timeStampCounter = new IasAudio::IasTimeStampCounter();
  uint64_t  cyclesSum   = 0;
  uint64_t  framesCount = 0;
#endif

  do
  {
#if USE_REFERENCE
    uint32_t   nReferenceSamples    = 0;
#endif
    uint32_t   nWrittenSamples      = 0;
    uint32_t   cntChannels          = 0;
    float *tempData[cNumInputFiles];

    // Fill all output bundles with zeros. This has to be done here, because
    // the mixer only adds the new samples to the content of the existing output
    // buffers. This is due to the bundle-based processing of the mixer.
    myAudioChain->clearOutputBundleBuffers();

    // Read the PCM samples from all input files.
    nReadSamples = 0;
    for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
    {
      std::memset((void*)inputData[cntFiles], 0, inputFileInfo[cntFiles].channels * cIasFrameLength * sizeof(inputData[0][0]));
      nReadSamplesThisFile = (uint32_t)sf_readf_float(inputFile[cntFiles], inputData[cntFiles], cIasFrameLength);
      nReadSamples = std::max(nReadSamples, nReadSamplesThisFile);
      tempData[cntFiles] = inputData[cntFiles];
    }

    // De-interleave the file-specific buffers inputData[] and write the
    // PCM samples into inputFrame.
    for(uint32_t i=0; i < nReadSamples; i++)
    {
      cntChannels = 0;
      for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
      {
        for (uint32_t cntChannelsThisFile = 0;
             cntChannelsThisFile < (uint32_t)(inputFileInfo[cntFiles].channels);
             cntChannelsThisFile++)
        {
          inputFrame[cntChannels][i] = *(tempData[cntFiles]) * gain[cntFiles];
          (tempData[cntFiles])++;
          cntChannels++;
        }
      }
    }

    // Write the samples from the inputFrame into the individual streams.
    cntChannels = 0;
    for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
    {
      dataStream[cntFiles].resize(0);
      for (uint32_t cntChannelsThisFile = 0;
           cntChannelsThisFile < (uint32_t)(inputFileInfo[cntFiles].channels);
           cntChannelsThisFile++)
      {
        dataStream[cntFiles].push_back(inputFrame[cntChannels]);
        cntChannels++;
      }
      hSink[cntFiles]->asBundledStream()->writeFromNonInterleaved(dataStream[cntFiles]);
    }


#if PROFILE
    timeStampCounter->reset();
#endif

    switch (frameCounter)
    {
      case 1 * cSectionLength:
        printf("Set mono input balance 100%% to right!\n");
        setBalance(monoPin, 10000);

        printf("Set stereo input balance 100%% to left!\n");
        setBalance(stereoPin, -10000);
        break;

      case 2 * cSectionLength:
        printf("Set mono input balance 100%% to left!\n");
        setBalance(monoPin, -10000);

        printf("Set stereo input balance 100%% to right!\n");
        setBalance(stereoPin, 10000);
        break;

      case 3 * cSectionLength:
        printf("Set mono input fader 100%% to rear!\n");
        setFader(monoPin, 10000);

        printf("Set stereo input fader 100%% to rear!\n");
        setFader(stereoPin, 10000);
        break;

      case 4 * cSectionLength:
        printf("Set mono input fader 100%% to front!\n");
        setFader(monoPin, -10000);

        printf("Set stereo input fader 100%% to front!\n");
        setFader(stereoPin, -10000);
        break;

      case 5 * cSectionLength:
        printf("Set mono input fader to neutral!\n");
        setFader(monoPin, 0);

        printf("Set stereo input fader to neutral!\n");
        setFader(stereoPin, 0);

        printf("Set six-channel input fader 100%% to front!\n");
        setFader(multichannelPin, -10000);
        break;

      case 6 * cSectionLength:
        printf("Set mono input balance to neutral!\n");
        setBalance(monoPin, 0);

        printf("Set stereo input balance to neutral!\n");
        setBalance(stereoPin, 0);

        printf("Set six-channel input fader 100%% to rear!\n");
        setFader(multichannelPin, 10000);
        break;

      case 7 * cSectionLength:
        printf("Set six-channel input fader to neutral!\n");
        setFader(multichannelPin, 0);

        printf("Set multichannel input balance 50%% to the left!\n");
        setBalance(multichannelPin, 60);
        break;

      case 10 * cSectionLength:
        printf("Set multichannel input balance 50%% to the right!\n");
        setBalance(multichannelPin, -60);
        break;

      case 13 * cSectionLength:
        printf("Set multichannel input balance to neutral!\n");
        setBalance(multichannelPin, 0);
        break;
    }

    // Execute the mixer for the current frame.
    mixerCore->process();
    
#if PROFILE
    uint32_t timeStamp = timeStampCounter->get();
    cyclesSum += static_cast<uint64_t>(timeStamp);
    framesCount++;
    //printf("Time for one frame: %d clocks\n", timeStamp);
#endif

    // Write the processed PCM samples into the output file.
    for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
    {
      hSpeaker[cntFiles]->asBundledStream()->read(outputData[cntFiles]);

      // Write the interleaved PCM samples into the output WAVE file.
      nWrittenSamples = (uint32_t) sf_writef_float(outputFile[cntFiles], outputData[cntFiles], (sf_count_t)nReadSamples);
      if (nWrittenSamples != nReadSamples)
      {
        std::cout<< "write to file " << outputFileName[cntFiles] <<"failed" <<std::endl;
        ASSERT_TRUE(false);
      }
#if USE_REFERENCE
      // Read in all channels from the reference file. The interleaved samples are
      // stored within the linear vector refData.
      nReferenceSamples = static_cast<decltype(nReferenceSamples)>(sf_readf_float(referenceFile[cntFiles], referenceData[cntFiles], (sf_count_t)nReadSamples));
      if (nReferenceSamples < nReadSamples)
      {
        printf("Error while reading reference PCM samples from file %s\n", referenceFileName[cntFiles]);
        ASSERT_TRUE(false);
      }

      // Compare processed output samples with reference stream.
      for (uint32_t cnt = 0; cnt < nReadSamples; cnt++)
      {
        uint32_t nChannels = cNumOutputChannels[cntFiles];
        for (uint32_t channel = 0; channel < nChannels; channel++)
        {
          difference[cntFiles] = std::max(difference[cntFiles], (float)(fabs(outputData[cntFiles][cnt*nChannels+channel] - referenceData[cntFiles][cnt*nChannels+channel])));
        }
      }
#endif
    }
    frameCounter++;

  } while (nReadSamples == cIasFrameLength);

#if PROFILE
  printf("Average load: %f clocks/frame \n", ((double)cyclesSum/frameCounter));
#endif

  // Close all files.
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    if (sf_close(inputFile[cntFiles]))
    {
      std::cout<< "close of file " << inputFileName[cntFiles] <<"failed" <<std::endl;
      ASSERT_TRUE(false);
    }
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    if (sf_close(outputFile[cntFiles]))
    {
      std::cout<< "close of file " << outputFileName[cntFiles] <<"failed" <<std::endl;
      ASSERT_TRUE(false);
    }
  }
#if USE_REFERENCE
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    if (sf_close(referenceFile[cntFiles]))
    {
      std::cout<< "close of file " << referenceFileName[cntFiles] <<"failed" <<std::endl;
      ASSERT_TRUE(false);
    }
  }
#endif

  delete myMixer;
  delete myAudioChain;

#if PROFILE
  delete timeStampCounter;
#endif

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    std::free(inputFrame[channels]);
    inputFrame[channels] = nullptr;
  }
  std::free(inputFrame);
  inputFrame = nullptr;
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    std::free(outputData[cntFiles]);
    outputData[cntFiles] = nullptr;
  }
#if USE_REFERENCE
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    std::free(referenceData[cntFiles]);
    referenceData[cntFiles] = nullptr;
  }
#endif
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    std::free(inputData[cntFiles]);
    inputData[cntFiles] = nullptr;
  }

  printf("All frames have been processed.\n");
#if USE_REFERENCE
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    auto differenceDB = 20.0f*std::log10(std::max(difference[cntFiles], (float)(1e-20)));
    printf("Zone %d (%d channels): peak difference between processed output data and reference: %7.2f dBFS\n",
           cntFiles, cNumOutputChannels[cntFiles], differenceDB);
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    auto differenceDB = 20.0f*std::log10(std::max(difference[cntFiles], (float)(1e-20)));
    float thresholdDB  = -120.0f;
    if (differenceDB > thresholdDB)
    {
      printf("Error: Peak difference exceeds threshold of %7.2f dBFS.\n", thresholdDB);
      ASSERT_TRUE(false);
    }
  }
#endif
  printf("Test has been passed.\n");


}


} // namespace IasAudio
