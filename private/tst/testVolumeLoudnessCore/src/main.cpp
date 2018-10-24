/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file main.cpp
 * @date 2012
 * @brief the main testfile for the volume file based test
 */

#include <gtest/gtest.h>
#include <sndfile.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <iostream>

#include "internal/audio/smartx_test_support/IasTimeStampCounter.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

/*
 *  Set PROFILE to 1 to enable perf measurement
 */
#define PROFILE 0

#ifndef NFS_PATH
#define NFS_PATH "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2016-06-08/"
#endif

#define ADD_ROOT(filename) NFS_PATH filename

static const uint32_t cIasFrameLength  =    64;
static const uint32_t cIasSampleRate   = 44100;
static const uint32_t cNumStreams      =     4;


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

class IasVolumeLoudnessCoreTest : public ::testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};


TEST_F(IasVolumeLoudnessCoreTest, main_test)
{
  const char *inputFileName1     = ADD_ROOT("Grummelbass_44100.wav");
  const char *inputFileName2     = ADD_ROOT("Pachelbel_inspired_44100.wav");
  const char *inputFileName3     = ADD_ROOT("wav3_44100.wav");
  const char *inputFileName4     = ADD_ROOT("Grummelbass_44100_6chan.wav");
  const char *outputFileName1    = "test_outstream0.wav";
  const char *outputFileName2    = "test_outstream1.wav";
  const char *outputFileName3    = "test_outstream2.wav";
  const char *outputFileName4    = "test_outstream3.wav";
  const char *referenceFileName1 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream0.wav");
  const char *referenceFileName2 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream1.wav");
  const char *referenceFileName3 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream2.wav");
  const char *referenceFileName4 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream3.wav");

  const char *inputFileName[cNumStreams]     = { inputFileName1, inputFileName2, inputFileName3, inputFileName4 };
  const char *outputFileName[cNumStreams]    = { outputFileName1, outputFileName2, outputFileName3, outputFileName4 };
  const char *referenceFileName[cNumStreams] = { referenceFileName1, referenceFileName2, referenceFileName3, referenceFileName4 };

  float   difference[cNumStreams]     = { 0.0f, 0.0f, 0.0f, 0.0f };
  float  *ioData[cNumStreams];        // pointers to buffers with interleaved samples
  float  *referenceData[cNumStreams]; // pointers to buffers with interleaved samples

  SNDFILE *inputFile1     = NULL;
  SNDFILE *inputFile2     = NULL;
  SNDFILE *inputFile3     = NULL;
  SNDFILE *inputFile4     = NULL;
  SNDFILE *outputFile1    = NULL;
  SNDFILE *outputFile2    = NULL;
  SNDFILE *outputFile3    = NULL;
  SNDFILE *outputFile4    = NULL;
  SNDFILE *referenceFile1 = NULL;
  SNDFILE *referenceFile2 = NULL;
  SNDFILE *referenceFile3 = NULL;
  SNDFILE *referenceFile4 = NULL;

  SNDFILE *inputFile[cNumStreams]     = { inputFile1,  inputFile2,  inputFile3,  inputFile4 };
  SNDFILE *outputFile[cNumStreams]    = { outputFile1, outputFile2, outputFile3, outputFile4 };
  SNDFILE *referenceFile[cNumStreams] = { referenceFile1, referenceFile2, referenceFile3, referenceFile4 };

  SF_INFO  inputFileInfo[cNumStreams];
  SF_INFO  outputFileInfo[cNumStreams];
  SF_INFO  referenceFileInfo[cNumStreams];

  // Open the input files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    inputFileInfo[cntFiles].format = 0;
    inputFile[cntFiles] = sf_open(inputFileName[cntFiles], SFM_READ, &inputFileInfo[cntFiles]);
    if (NULL == inputFile[cntFiles])
    {
      std::cout<<"Could not open file " <<inputFileName[cntFiles] <<std::endl;
      ASSERT_FALSE(true);
    }
  }

  // Open the output files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    outputFileInfo[cntFiles].samplerate = inputFileInfo[cntFiles].samplerate;
    outputFileInfo[cntFiles].channels   = inputFileInfo[cntFiles].channels;
    outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
    outputFile[cntFiles] = sf_open(outputFileName[cntFiles], SFM_WRITE, &outputFileInfo[cntFiles]);
    if (NULL == outputFile[cntFiles])
    {
      std::cout<<"Could not open file " <<outputFileName[cntFiles] <<std::endl;
      ASSERT_FALSE(true);
    }
  }

  // Open the reference files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    referenceFileInfo[cntFiles].format = 0;
    referenceFile[cntFiles] = sf_open(referenceFileName[cntFiles], SFM_READ, &referenceFileInfo[cntFiles]);
    if (NULL == referenceFile[cntFiles])
    {
      std::cout<<"Could not open file " <<referenceFileName[cntFiles] <<std::endl;
      ASSERT_FALSE(true);
    }
    if (referenceFileInfo[cntFiles].channels != inputFileInfo[cntFiles].channels)
    {
      std::cout<<"File " <<referenceFileName[cntFiles] << ": number of channels does not match!" << std::endl;
      ASSERT_FALSE(true);
    }
  }


  uint32_t nInputChannels = 0;
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    uint32_t numChannels = static_cast<uint32_t>(inputFileInfo[cntFiles].channels);
    ioData[cntFiles]        = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);
    referenceData[cntFiles] = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);
    nInputChannels += numChannels;
  }
  float** ioFrame = (float**)malloc(sizeof(float*) * nInputChannels);

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    ioFrame[channels]    = (float* )malloc(sizeof(float)*cIasFrameLength);
  }

  IasAudioChain *myAudioChain = new IasAudioChain();
  ASSERT_TRUE(myAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;
  IasAudioChain::IasResult chainres = myAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  IasAudioStream *hSink0 = myAudioChain->createInputAudioStream("Sink0", 0, 2,false);
  IasAudioStream *hSink1 = myAudioChain->createInputAudioStream("Sink1", 1, 2,false);
  IasAudioStream *hSink2 = myAudioChain->createInputAudioStream("Sink2", 2, 2,false);
  IasAudioStream *hSink3 = myAudioChain->createInputAudioStream("Sink3", 3, 6,false);
  ASSERT_TRUE(hSink0 != nullptr);
  ASSERT_TRUE(hSink1 != nullptr);
  ASSERT_TRUE(hSink2 != nullptr);
  ASSERT_TRUE(hSink3 != nullptr);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);
  IasCmdDispatcherPtr cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEngine pluginEngine(cmdDispatcher);
  IasAudioProcessingResult procres = pluginEngine.loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);
  IasIGenericAudioCompConfig *config = nullptr;
  procres = pluginEngine.createModuleConfig(&config);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != config);

  IasProperties properties;
  properties.set("numFilterBands", 3);
  config->addStreamToProcess(hSink0, "Sink0");
  config->addStreamToProcess(hSink1, "Sink1");
  config->addStreamToProcess(hSink2, "Sink2");
  config->addStreamToProcess(hSink3, "Sink3");
  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");
  properties.set("activePinsForBand.0", activePins);
  properties.set("activePinsForBand.1", activePins);
  properties.set("activePinsForBand.2", activePins);
  config->setProperties(properties);

  IasGenericAudioComp *myVolume;
  pluginEngine.createModule(config, "ias.volume", "MyVolume", &myVolume);
  myAudioChain->addAudioComponent(myVolume);
  IasVolumeLoudnessCore *volumeCore = reinterpret_cast<IasVolumeLoudnessCore*>(myVolume->getCore());
  volumeCore->reset();

  //the command interface is not used here, but call the function to have test coverage
  IasIModuleId *volumeCmd   = myVolume->getCmdInterface();
  ASSERT_TRUE(volumeCmd != nullptr);

  volumeCore->enableProcessing();

  uint32_t nReadSamples[cNumStreams] = { 0, 0, 0, 0 };
  uint32_t nWrittenSamples = 0;
  uint32_t frameCounter    = 0;

  IasAudioFrame dataStream0, dataStream1, dataStream2, dataStream3;

#if PROFILE
  IasAudio::IasTimeStampCounter *timeStampCounter = new IasAudio::IasTimeStampCounter();
  uint64_t  cyclesSum   = 0;
#endif

  // Main processing loop.
  do
  {
    // Read in all channels from the WAVE file. The interleaved samples are
    // stored within the linear vector ioData.
    for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
    {
      memset((void*)ioData[cntFiles], 0, inputFileInfo[cntFiles].channels * cIasFrameLength * sizeof(ioData[0][0]));
      nReadSamples[cntFiles] = (uint32_t)sf_readf_float(inputFile[cntFiles], ioData[cntFiles], cIasFrameLength);
    }

    float* tempData1 = ioData[0];
    float* tempData2 = ioData[1];
    float* tempData3 = ioData[2];
    float* tempData4 = ioData[3];

    for(uint32_t i=0; i < cIasFrameLength; i++)
    {
        ioFrame[0][i] = *tempData1++;
        ioFrame[1][i] = *tempData1++;
        ioFrame[2][i] = *tempData2++;
        ioFrame[3][i] = *tempData2++;
        ioFrame[4][i] = *tempData3++;
        ioFrame[5][i] = *tempData3++;
        ioFrame[6][i] = *tempData4++;
        ioFrame[7][i] = *tempData4++;
        ioFrame[8][i] = *tempData4++;
        ioFrame[9][i] = *tempData4++;
        ioFrame[10][i] = *tempData4++;
        ioFrame[11][i] = *tempData4++;

    }
    dataStream0.resize(0);
    dataStream1.resize(0);
    dataStream2.resize(0);
    dataStream3.resize(0);
    dataStream0.push_back(ioFrame[0]);
    dataStream0.push_back(ioFrame[1]);
    dataStream1.push_back(ioFrame[2]);
    dataStream1.push_back(ioFrame[3]);
    dataStream2.push_back(ioFrame[4]);
    dataStream2.push_back(ioFrame[5]);
    dataStream3.push_back(ioFrame[6]);
    dataStream3.push_back(ioFrame[7]);
    dataStream3.push_back(ioFrame[8]);
    dataStream3.push_back(ioFrame[9]);
    dataStream3.push_back(ioFrame[10]);
    dataStream3.push_back(ioFrame[11]);

    hSink0->asBundledStream()->writeFromNonInterleaved(dataStream0);
    hSink1->asBundledStream()->writeFromNonInterleaved(dataStream1);
    hSink2->asBundledStream()->writeFromNonInterleaved(dataStream2);
    hSink3->asBundledStream()->writeFromNonInterleaved(dataStream3);

    // command and control for test run
    IasProperties cmdProperties;
    IasProperties returnProperties;
    int32_t cmdId;
    IasIModuleId::IasResult cmdres;
    IasInt32Vector rampParams;
    IasInt32Vector muteParams;
    muteParams.resize(3);
    rampParams.resize(2);
    switch (frameCounter)
    {
      case 1:
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetLoudness;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set<std::string>("loudness", "off");
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(10*44100/64):
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", 0);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(20*44100/64):
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", -300);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(30*44100/64):
        std::cout << "activating loudness for all streams" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetLoudness;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set<std::string>("loudness", "on");
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(40*44100/64):
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", 0);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(50*44100/64):
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", -300);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(54*44100/64):
        std::cout <<"Starting 3 mutes of 0.1 sec ramptime" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetMuteState;
        cmdProperties.set("cmd", cmdId);
        muteParams[0] = true;
        muteParams[1] = 100;
        muteParams[2] = eIasRampShapeExponential;
        cmdProperties.set("params", muteParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case (uint32_t)(56*44100/64):
        std::cout <<"Starting 3 mutes of 0.1 sec ramptime" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetMuteState;
        cmdProperties.set("cmd", cmdId);
        muteParams[0] = false;
        muteParams[1] = 100;
        muteParams[2] = eIasRampShapeExponential;
        cmdProperties.set("params", muteParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      case(uint32_t)(60*44100/64):
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", -1440);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);
        cmdProperties.set("pin", std::string("Sink0"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink1"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink2"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        cmdProperties.set("pin", std::string("Sink3"));
        cmdres = volumeCmd->processCmd(cmdProperties, returnProperties);
        ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
        break;
      default:
        break;
    }

#if PROFILE
    timeStampCounter->reset();
#endif

    // call process of audio component under test.
    volumeCore->process();

#if PROFILE
    uint32_t timeStamp = timeStampCounter->get();
    cyclesSum += static_cast<uint64_t>(timeStamp);
    printf("Time for one frame: %d clocks\n", timeStamp);
#endif

    hSink0->asBundledStream()->read(dataStream0);
    hSink1->asBundledStream()->read(dataStream1);
    hSink2->asBundledStream()->read(dataStream2);
    hSink3->asBundledStream()->read(dataStream3);

    tempData1 = ioData[0];
    tempData2 = ioData[1];
    tempData3 = ioData[2];
    tempData4 = ioData[3];

    for(uint32_t i=0; i < cIasFrameLength; i++)
    {
      *tempData1++ = dataStream0[0][i];
      *tempData1++ = dataStream0[1][i];
      *tempData2++ = dataStream1[0][i];
      *tempData2++ = dataStream1[1][i];
      *tempData3++ = dataStream2[0][i];
      *tempData3++ = dataStream2[1][i];
      *tempData4++ = dataStream3[0][i];
      *tempData4++ = dataStream3[1][i];
      *tempData4++ = dataStream3[2][i];
      *tempData4++ = dataStream3[3][i];
      *tempData4++ = dataStream3[4][i];
      *tempData4++ = dataStream3[5][i];
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
      // stored within the linear vector refData.
      uint32_t nReferenceSamples = static_cast<uint32_t>(sf_readf_float(referenceFile[cntFiles], referenceData[cntFiles], (sf_count_t)nReadSamples[cntFiles]));
      if (nReferenceSamples < nReadSamples[cntFiles])
      {
        printf("Error while reading reference PCM samples from file %s\n", referenceFileName[cntFiles]);
        ASSERT_TRUE(false);
      }

      // Compare processed output samples with reference stream.
      for (uint32_t cnt = 0; cnt < nReadSamples[cntFiles]; cnt++)
      {
        uint32_t nChannels = static_cast<uint32_t>(inputFileInfo[cntFiles].channels);
        for (uint32_t channel = 0; channel < nChannels; channel++)
        {
          difference[cntFiles] = std::max(difference[cntFiles], (float)(fabs(ioData[cntFiles][cnt*nChannels+channel] -
                                                                                    referenceData[cntFiles][cnt*nChannels+channel])));
        }
      }
    }

    frameCounter++;

  } while ( (nReadSamples[0] == cIasFrameLength) ||
            (nReadSamples[1] == cIasFrameLength) ||
            (nReadSamples[2] == cIasFrameLength) ||
            (nReadSamples[3] == cIasFrameLength) );

  //end of main processing loop, start cleanup.

#if PROFILE
  printf("Average load: %f clocks/frame \n", ((double)cyclesSum/frameCounter));
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

  delete(myVolume);
  delete(myAudioChain);

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    free(ioData[cntFiles]);
    free(referenceData[cntFiles]);
  }

  for(uint32_t channels = 0; channels < nInputChannels; channels++)
  {
    free(ioFrame[channels]);
  }
  free(ioFrame);

  printf("All frames have been processed.\n");

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    float differenceDB = 20.0f*static_cast<float>(log10(std::max(difference[cntFiles], (float)(1e-20))));
    printf("Stream %d (%d channels): peak difference between processed output data and reference: %7.2f dBFS\n",
           cntFiles+1, inputFileInfo[cntFiles].channels, differenceDB);
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    float differenceDB = 20.0f*static_cast<float>(log10(std::max(difference[cntFiles], (float)(1e-20))));
    float thresholdDB  = -50.0f;

    if (differenceDB > thresholdDB)
    {
      printf("Error: Peak difference exceeds threshold of %7.2f dBFS.\n", thresholdDB);
      ASSERT_TRUE(false);
    }
  }
  printf("Test has been passed.\n");
}
} // namespace IasAudio
