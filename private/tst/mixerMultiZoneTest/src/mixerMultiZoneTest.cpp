/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    mixerMultiZoneTest.cpp
 * @brief   Test application for the mixer with multi-zone output configuration.
 * @date    August 2016
 *
 *  This test application implements the following mixer configuration with
 *  3 output zones (i.e. 3 output stremas, i.e. 3 elementary mixers). Each
 *  input stream consists of two channels.
 *  \verbatim
 *
 *  input streams  |  output stream
 *  ---------------+------------------
 *  1              |  0 (6 channels)
 *  3, 4, 5        |  1 (4 channels)
 *  0, 2           |  2 (2 channels)
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
static const uint32_t cIasSampleRate     { 48000 };
static const uint32_t cSectionLength     { 5*cIasSampleRate/cIasFrameLength };  // approximately 5 seconds
static const uint32_t cNumInputFiles     { 6 };
static const uint32_t cNumOutputFiles    { 3 };
static const uint32_t cNumOutputChannels[cNumOutputFiles] = { 6, 4, 2};

/*
 *  Set PROFILE to 1 to enable perf measurement
 */
#define PROFILE 0

#define ADD_ROOT(filename) "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2016-10-19/" filename

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

class IasMixerMultiZoneTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();

    void setInputGainOffset(const char* pin, int32_t gain);
    void setBalance(const char* pin, int32_t balance);
    void setFader(const char* pin, int32_t fader);

    IasIModuleId *mCmd = nullptr;

};

void IasMixerMultiZoneTest::SetUp()
{
}

void IasMixerMultiZoneTest::TearDown()
{
}

void IasMixerMultiZoneTest::setInputGainOffset(const char* pin, int32_t gain)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetInputGainOffset);
  cmdProperties.set<int32_t>("gain", gain);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

void IasMixerMultiZoneTest::setFader(const char* pin, int32_t fader)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetFader);
  cmdProperties.set<int32_t>("fader", fader);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

void IasMixerMultiZoneTest::setBalance(const char* pin, int32_t balance)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetBalance);
  cmdProperties.set<int32_t>("balance", balance);
  auto cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

TEST_F(IasMixerMultiZoneTest, main_test)
{
  const char* inputFileName0  = ADD_ROOT("sin200_48000.wav");
  const char* inputFileName1  = ADD_ROOT("sin1200_48000.wav");
  const char* inputFileName2  = ADD_ROOT("sin400_48000.wav");
  const char* inputFileName3  = ADD_ROOT("Grummelbass_48000.wav");
  const char* inputFileName4  = ADD_ROOT("Pachelbel_inspired_48000.wav");
  const char* inputFileName5  = ADD_ROOT("wav3_48000.wav");

  const char* outputFileName0 = "mixer_out0.wav";
  const char* outputFileName1 = "mixer_out1.wav";
  const char* outputFileName2 = "mixer_out2.wav";

  const char* referenceFileName0 = ADD_ROOT("reference_mixerMultiZoneTest_out0_new.wav");
  const char* referenceFileName1 = ADD_ROOT("reference_mixerMultiZoneTest_out1_new.wav");
  const char* referenceFileName2 = ADD_ROOT("reference_mixerMultiZoneTest_out2_new.wav");


  const char* inputFileName[cNumInputFiles]      = { inputFileName0, inputFileName1, inputFileName2, inputFileName3, inputFileName4, inputFileName5 };
  const char* outputFileName[cNumOutputFiles]    = { outputFileName0, outputFileName1, outputFileName2 };
  const char* referenceFileName[cNumOutputFiles] = { referenceFileName0, referenceFileName1, referenceFileName2 };

  SNDFILE* inputFile0     = nullptr;
  SNDFILE* inputFile1     = nullptr;
  SNDFILE* inputFile2     = nullptr;
  SNDFILE* inputFile3     = nullptr;
  SNDFILE* inputFile4     = nullptr;
  SNDFILE* inputFile5     = nullptr;
  SNDFILE* outputFile0    = nullptr;
  SNDFILE* outputFile1    = nullptr;
  SNDFILE* outputFile3    = nullptr;
  SNDFILE* referenceFile0 = nullptr;
  SNDFILE* referenceFile1 = nullptr;
  SNDFILE* referenceFile2 = nullptr;

  SNDFILE* inputFile[cNumInputFiles]      = { inputFile0,  inputFile1,  inputFile2, inputFile3, inputFile4, inputFile5 };
  SNDFILE* outputFile[cNumOutputFiles]    = { outputFile0, outputFile1, outputFile3 };
  SNDFILE* referenceFile[cNumOutputFiles] = { referenceFile0, referenceFile1, referenceFile2 };

  SF_INFO  inputFileInfo[cNumInputFiles];
  SF_INFO  outputFileInfo[cNumOutputFiles];
  SF_INFO  referenceFileInfo[cNumOutputFiles];

  const float gain[cNumInputFiles]  = { 1.0f, 1.0f, 1.0f, 0.33f, 0.33f, 0.33f };
  float difference[cNumOutputFiles] = { 0.0f, 0.0f, 0.0f };

  // Open the input files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    inputFileInfo[cntFiles].format = 0;
    inputFile[cntFiles] = sf_open(inputFileName[cntFiles], SFM_READ, &inputFileInfo[cntFiles]);
    if (nullptr == inputFile[cntFiles])
    {
      std::cout<<"Could not open file " << inputFileName[cntFiles] <<std::endl;
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
      std::cout<<"Could not open file " << outputFileName[cntFiles] <<std::endl;
      ASSERT_TRUE(false);
    }
  }

  // Open the reference files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    referenceFileInfo[cntFiles].format = 0;
    referenceFile[cntFiles] = sf_open(referenceFileName[cntFiles], SFM_READ, &referenceFileInfo[cntFiles]);
    if (nullptr == referenceFile[cntFiles])
    {
      std::cout<<"Could not open file " << referenceFileName[cntFiles] <<std::endl;
      ASSERT_TRUE(false);
    }
    if (static_cast<uint32_t>(referenceFileInfo[cntFiles].channels) != cNumOutputChannels[cntFiles])
    {
      std::cout<< "Number of channels stored in file " << referenceFileName[cntFiles]
               << " does not match, expected: %d, actual: %d"
               << cNumOutputChannels[cntFiles] << referenceFileInfo[cntFiles].channels << std::endl;
      ASSERT_TRUE(false);
    }
  }

  uint32_t    nInputChannels = 0;
  float  *inputData[cNumInputFiles];      // pointers to buffers with interleaved samples
  float  *outputData[cNumOutputFiles];    // pointers to buffers with interleaved samples
  float  *referenceData[cNumOutputFiles]; // pointers to buffers with interleaved samples

  // Allocate the buffers with interleaved samples.
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    nInputChannels   += inputFileInfo[cntFiles].channels;
    inputData[cntFiles]  = (float*)std::malloc(sizeof(float) * cIasFrameLength * inputFileInfo[cntFiles].channels);
    if (nullptr == inputData[cntFiles])
    {
      std::cout<<"Error while allocating inputData[%d] " <<cntFiles <<std::endl;
      ASSERT_TRUE(false);
    }
  }
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    outputData[cntFiles]  = (float*)std::malloc(sizeof(float) * cIasFrameLength * outputFileInfo[cntFiles].channels);
    if (nullptr == outputData[cntFiles])
    {
      std::cout<<"Error while allocating outputData[%d] " <<cntFiles <<std::endl;
      ASSERT_TRUE(false);
    }
  }
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    referenceData[cntFiles]  = (float*)std::malloc(sizeof(float) * cIasFrameLength * referenceFileInfo[cntFiles].channels);
    if (nullptr == referenceData[cntFiles])
    {
      std::cout<<"Error while allocating referenceData[%d] " <<cntFiles <<std::endl;
      ASSERT_TRUE(false);
    }
  }

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
        cntStreams, 2, false);
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

  // Connect input stream 1           with output stream 0.
  // Connect input stream 3, 4, and 5 with output stream 1.
  // Connect input stream 0 and 2     with output stream 2.
  myMixerConfiguration->addStreamMapping(hSink[1], "Sink1", hSpeaker[0], "Speaker0");
  myMixerConfiguration->addStreamMapping(hSink[3], "Sink3", hSpeaker[1], "Speaker1");
  myMixerConfiguration->addStreamMapping(hSink[4], "Sink4", hSpeaker[1], "Speaker1");
  myMixerConfiguration->addStreamMapping(hSink[5], "Sink5", hSpeaker[1], "Speaker1");
  myMixerConfiguration->addStreamMapping(hSink[0], "Sink0", hSpeaker[2], "Speaker2");
  myMixerConfiguration->addStreamMapping(hSink[2], "Sink2", hSpeaker[2], "Speaker2");

  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");
  activePins.push_back("Sink4");
  activePins.push_back("Sink5");
  activePins.push_back("Speaker0");
  activePins.push_back("Speaker1");
  activePins.push_back("Speaker2");

  IasGenericAudioComp* myMixer;
  pluginEngine.createModule(myMixerConfiguration, "ias.mixer", "MyMixer", &myMixer);

  myAudioChain->addAudioComponent(myMixer);

  auto* mixerCore = myMixer->getCore();
  ASSERT_TRUE(nullptr != mixerCore);

  mCmd = myMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != mCmd);

  mixerCore->enableProcessing();

  uint32_t      frameCounter = 0;
  uint32_t      nReadSamples;
  uint32_t      nReadSamplesThisFile;
  IasAudioFrame    dataStream[cNumInputFiles]; // each input file represents one data stream


#if PROFILE
  IasAudio::IasTimeStampCounter *timeStampCounter = new IasAudio::IasTimeStampCounter();
  uint64_t  cyclesSum   = 0;
  uint64_t  framesCount = 0;
#endif


  do
  {
    uint32_t   nReferenceSamples    = 0;
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

    // Trigger balance or fader for input streams 0 and 2.
    // This influences output stream 2.
    switch(frameCounter)
    {
      case 1*cSectionLength:
        setBalance("Sink0", 140);  // not sure abou balance and fader value
        break;
      case 2*cSectionLength:
        setBalance("Sink2", -140);
        setFader("Sink0", -140);
        setFader("Sink2", -140);
        break;
      case 3*cSectionLength:
        setBalance("Sink0", -140);
        setFader("Sink0", 0);
        setFader("Sink2", 0);
        break;
      case 4*cSectionLength:
        setBalance("Sink2", 140);
        break;
      case 5*cSectionLength:
        setBalance("Sink0", 0);
        setBalance("Sink2", 0);
        break;
      case 6*cSectionLength:
        setBalance("Sink0", 0);
        setBalance("Sink2", 0);
        break;
    }

    // Trigger balance or fader for input stream 1.
    // This influences output stream 0.
    switch(frameCounter)
    {
      case 1*cSectionLength:
        setBalance("Sink1", -140);
        setFader("Sink1",   -140);
        break;
      case 2*cSectionLength:
        setBalance("Sink1", -140);
        setFader("Sink1",   140);
        break;
      case 3*cSectionLength:
        setBalance("Sink1", 140);
        setFader("Sink1",   140);
        break;
      case 4*cSectionLength:
        setBalance("Sink1", 140);
        setFader("Sink1",   -140);
        break;
      case 5*cSectionLength:
        setBalance("Sink1", -140);
        setFader("Sink1",   -140);
        break;
      case 6*cSectionLength:
        setBalance("Sink1", -140);
        setFader("Sink1",   140);
        break;
    }

    // Trigger balance or fader for input streams 3, 4, and 5.
    // This influences output stream 1.
    if ((frameCounter % (4*cSectionLength)) == 0)
    {
      setFader("Sink3", 10000);
      setFader("Sink4", 10000);
      setFader("Sink5", -10000);
      setBalance("Sink3", 200);
      setBalance("Sink4", -200);
      setBalance("Sink5", -200);
    }
    else if ((frameCounter % (4*cSectionLength)) == cSectionLength)
    {
      setBalance("Sink3", -200);
      setBalance("Sink4", 200);
      setBalance("Sink5", 200);
    }
    else if ((frameCounter % (4*cSectionLength)) == 2*cSectionLength)
    {
      setFader("Sink3", 10000);
      setFader("Sink4", 10000);
      setFader("Sink5", -10000);
      setBalance("Sink3", 200);
      setBalance("Sink4", -200);
      setBalance("Sink5", -200);
    }
    else if ((frameCounter % (4*cSectionLength)) == 3*cSectionLength)
    {
      setBalance("Sink3", -200);
      setBalance("Sink4", 200);
      setBalance("Sink5", 200);
    }

#if PROFILE
    timeStampCounter->reset();
#endif

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
      nWrittenSamples = static_cast<uint32_t>(sf_writef_float(outputFile[cntFiles], outputData[cntFiles], (sf_count_t)nReadSamples));
      if (nWrittenSamples != nReadSamples)
      {
        std::cout<< "write to file " << outputFileName[cntFiles] <<"failed" <<std::endl;
        ASSERT_TRUE(false);
      }

      // Read in all channels from the reference file. The interleaved samples are
      // stored within the linear vector refData.
      nReferenceSamples = static_cast<uint32_t>(sf_readf_float(referenceFile[cntFiles], referenceData[cntFiles], (sf_count_t)nReadSamples));
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
          difference[cntFiles] = std::max(difference[cntFiles], (float)(std::abs(outputData[cntFiles][cnt*nChannels+channel] - referenceData[cntFiles][cnt*nChannels+channel])));
        }
      }
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

  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    if (sf_close(referenceFile[cntFiles]))
    {
      std::cout<< "close of file " << referenceFileName[cntFiles] <<"failed" <<std::endl;
      ASSERT_TRUE(false);
    }
  }

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
  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    std::free(referenceData[cntFiles]);
    referenceData[cntFiles] = nullptr;
  }
  for (uint32_t cntFiles = 0; cntFiles < cNumInputFiles; cntFiles++)
  {
    std::free(inputData[cntFiles]);
    inputData[cntFiles] = nullptr;
  }

  printf("All frames have been processed.\n");

  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    float differenceDB = 20.0f*static_cast<float>(std::log10(std::max(difference[cntFiles], (float)(1e-20))));
    printf("Zone %d (%d channels): peak difference between processed output data and reference: %7.2f dBFS\n",
           cntFiles, cNumOutputChannels[cntFiles], differenceDB);
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumOutputFiles; cntFiles++)
  {
    float differenceDB = 20.0f*static_cast<float>(std::log10(std::max(difference[cntFiles], (float)(1e-20))));
    float thresholdDB  = -87.0f;

    if (differenceDB > thresholdDB)
    {
      printf("Error: Peak difference exceeds threshold of %7.2f dBFS.\n", thresholdDB);
      ASSERT_TRUE(false);
    }
  }
  printf("Test has been passed.\n");

}


} // namespace IasAudio
