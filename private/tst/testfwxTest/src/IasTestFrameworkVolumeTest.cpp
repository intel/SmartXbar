/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasTestFrameworkVolumeTest.cpp
 * @date  2017
 * @brief Unit test for the SmartX test framework.
 *
 * This unit test applies the Volume/Loudness module, which is executed within
 * a pipeline within the test framework.
 */

#include "gtest/gtest.h"
#include <sndfile.h>
#include <string.h>
#include <iostream>
#include "audio/testfwx/IasTestFramework.hpp"
#include "audio/testfwx/IasTestFrameworkSetup.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "model/IasAudioPin.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"


#ifndef NFS_PATH
#define NFS_PATH "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2016-06-08/"
#endif

#define ADD_ROOT(filename) NFS_PATH filename

static const uint32_t cIasFrameLength  =    64;
static const uint32_t cIasSampleRate   = 44100;
static const uint32_t cNumStreams      =     4;


namespace IasAudio {

class IasTestFrameworkVolumeTest : public ::testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};


int32_t CompareWithReferenceFile(std::string outputFileName, std::string referenceFileName, float& difference)
{
  SNDFILE *outputFile    = NULL;
  SNDFILE *referenceFile = NULL;

  SF_INFO  outputFileInfo;
  SF_INFO  referenceFileInfo;

  outputFileInfo.format = 0;
  referenceFileInfo.format = 0;

  //open output file
  outputFile = sf_open(outputFileName.c_str(), SFM_READ, &outputFileInfo);
  if (NULL == outputFile)
  {
    std::cout<<"Could not open file " << outputFileName <<std::endl;
    return 0;
  }

  //open reference file
  referenceFile = sf_open(referenceFileName.c_str(), SFM_READ, &referenceFileInfo);
  if (NULL == referenceFile)
  {
    std::cout<<"Could not open file " << referenceFileName <<std::endl;
    return 0;
  }

  if (referenceFileInfo.channels != outputFileInfo.channels)
  {
    std::cout<<"File " <<referenceFileName << ": number of channels does not match!" << std::endl;
    return 0;
  }

  float *outputData;
  float *referenceData;

  uint32_t numChannels = static_cast<uint32_t>(outputFileInfo.channels);
  outputData = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);
  referenceData = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);

  uint32_t nOutputSamples = 0;
  uint32_t nReferenceSamples = 0;

  //read from files
  do
  {
    memset((void*)outputData, 0, numChannels * cIasFrameLength * sizeof(float));
    memset((void*)referenceData, 0, numChannels * cIasFrameLength * sizeof(float));

    nOutputSamples    = static_cast<uint32_t>(sf_readf_float(outputFile, outputData, cIasFrameLength));
    nReferenceSamples = static_cast<uint32_t>(sf_readf_float(referenceFile, referenceData, cIasFrameLength));

    // Compare output samples with reference stream.
    for (uint32_t cnt = 0; cnt < std::min(nOutputSamples, nReferenceSamples); cnt++)
    {
      for (uint32_t channel = 0; channel < numChannels; channel++)
      {
        difference = std::max(difference, (float)(fabs(outputData[cnt*numChannels+channel] -
                                                              referenceData[cnt*numChannels+channel])));
      }
    }

    if (nReferenceSamples != nOutputSamples)
    {
      printf("Reference samples number and output samples number does not match for file %s\n", referenceFileName.c_str());
      return 0;
    }
  }
  while(nReferenceSamples == cIasFrameLength && nOutputSamples == cIasFrameLength);

  return 1;
}


TEST_F(IasTestFrameworkVolumeTest, main_test)
{
  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  IasTestFrameworkSetup::IasResult setupResult = IasTestFrameworkSetup::eIasOk;
  IasTestFramework::IasResult      testfwxResult = IasTestFramework::eIasOk;

  IasPipelineParams pipelineParams;    //pipeline params

  IasProcessingModulePtr    moduleVolume = nullptr;
  IasProcessingModuleParams moduleVolumeParams;

  IasTestFrameworkWaveFileParams inputFileParams [cNumStreams];   //input wave file params
  IasTestFrameworkWaveFileParams outputFileParams[cNumStreams];   //output wave file params

  IasAudioPinParams pipelineInputPinParams [cNumStreams];    //pipeline input pin params
  IasAudioPinParams pipelineOutputPinParams[cNumStreams];    //pipeline output pin params
  IasAudioPinParams moduleInOutPinParams   [cNumStreams];    //module input/output pin params

  IasAudioPinPtr pipelineInputPin [cNumStreams];    //pipeline input pin pointers
  IasAudioPinPtr pipelineOutputPin[cNumStreams];    //pipeline output pin pointers
  IasAudioPinPtr moduleInOutPin   [cNumStreams];    //module input/output pin pointers

  pipelineParams.name = "TestFwxVolumeTestPipeline";
  pipelineParams.samplerate = cIasSampleRate;
  pipelineParams.periodSize = cIasFrameLength;

  moduleVolumeParams.typeName     = "ias.volume";
  moduleVolumeParams.instanceName = "VolumeLoudness";

  inputFileParams[0].fileName = ADD_ROOT("Grummelbass_44100.wav");
  inputFileParams[1].fileName = ADD_ROOT("Pachelbel_inspired_44100.wav");
  inputFileParams[2].fileName = ADD_ROOT("wav3_44100.wav");
  inputFileParams[3].fileName = ADD_ROOT("Grummelbass_44100_6chan.wav");

  outputFileParams[0].fileName = "output_TestFrameworkVolumeTest_outstream0.wav";
  outputFileParams[1].fileName = "output_TestFrameworkVolumeTest_outstream1.wav";
  outputFileParams[2].fileName = "output_TestFrameworkVolumeTest_outstream2.wav";
  outputFileParams[3].fileName = "output_TestFrameworkVolumeTest_outstream3.wav";

  pipelineInputPinParams[0].name = "pipelineInputPin0";
  pipelineInputPinParams[1].name = "pipelineInputPin1";
  pipelineInputPinParams[2].name = "pipelineInputPin2";
  pipelineInputPinParams[3].name = "pipelineInputPin3";

  pipelineOutputPinParams[0].name = "pipelineOutputPin0";
  pipelineOutputPinParams[1].name = "pipelineOutputPin1";
  pipelineOutputPinParams[2].name = "pipelineOutputPin2";
  pipelineOutputPinParams[3].name = "pipelineOutputPin3";

  moduleInOutPinParams[0].name = "moduleInOutPin0";
  moduleInOutPinParams[1].name = "moduleInOutPin1";
  moduleInOutPinParams[2].name = "moduleInOutPin2";
  moduleInOutPinParams[3].name = "moduleInOutPin3";

  //obtain number of channels of input wave files and update pin properties accordingly
  for (uint32_t i = 0; i < cNumStreams; i++)
  {
    SNDFILE *inputFile = nullptr;
    SF_INFO inputFileInfo;
    inputFileInfo.format = 0;

    inputFile = sf_open(inputFileParams[i].fileName.c_str(), SFM_READ, &inputFileInfo);
    ASSERT_TRUE(inputFile != nullptr);

    pipelineInputPinParams[i].numChannels = static_cast<uint32_t>(inputFileInfo.channels);
    pipelineOutputPinParams[i].numChannels = pipelineInputPinParams[i].numChannels;
    moduleInOutPinParams[i].numChannels = pipelineInputPinParams[i].numChannels;

    sf_close(inputFile);
  }

  //create test framework
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);

  //obtain test framework setup interface
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //obtain processing interface
  IasIProcessing *processing = testfwx->processing();
  ASSERT_TRUE(processing != nullptr);

  //obtain debug interface
  IasIDebug *debug = testfwx->debug();
  ASSERT_TRUE(debug != nullptr);

  //create Volume processing module
  setupResult = setup->createProcessingModule(moduleVolumeParams, &moduleVolume);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);
  ASSERT_TRUE(moduleVolume != nullptr);

  IasProperties volumeProperties;
  volumeProperties.set("numFilterBands", 3);
  IasStringVector activePins;
  activePins.push_back(moduleInOutPinParams[0].name);
  activePins.push_back(moduleInOutPinParams[1].name);
  activePins.push_back(moduleInOutPinParams[2].name);
  activePins.push_back(moduleInOutPinParams[3].name);
  volumeProperties.set("activePinsForBand.0", activePins);
  volumeProperties.set("activePinsForBand.1", activePins);
  volumeProperties.set("activePinsForBand.2", activePins);
  setup->setProperties(moduleVolume,volumeProperties);

  //add Volume processing module to pipeline
  setupResult = setup->addProcessingModule(moduleVolume);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

  for (uint32_t i = 0; i < cNumStreams; i++)
  {
    //create and add pipeline input pin
    setupResult = setup->createAudioPin(pipelineInputPinParams[i], &pipelineInputPin[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);
    ASSERT_TRUE(pipelineInputPin[i] != nullptr);

    setupResult = setup->addAudioInputPin(pipelineInputPin[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

    //link input pin with input wave file
    setupResult = setup->linkWaveFile(pipelineInputPin[i], inputFileParams[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

    //create and add pipeline output pin
    setupResult = setup->createAudioPin(pipelineOutputPinParams[i], &pipelineOutputPin[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);
    ASSERT_TRUE(pipelineOutputPin[i] != nullptr);

    setupResult = setup->addAudioOutputPin(pipelineOutputPin[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

    //link output pin with output wave file
    setupResult = setup->linkWaveFile(pipelineOutputPin[i], outputFileParams[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

    //create and add input/output pin for Volume processing module
    setupResult = setup->createAudioPin(moduleInOutPinParams[i], &moduleInOutPin[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);
    ASSERT_TRUE(moduleInOutPin[i] != nullptr);

    setupResult = setup->addAudioInOutPin(moduleVolume, moduleInOutPin[i]);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

    //link pipeline input pin with module input/output pin
    setupResult = setup->link(pipelineInputPin[i], moduleInOutPin[i] ,eIasAudioPinLinkTypeImmediate);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);

    //link module input/output pin with pipeline output pin
    setupResult = setup->link(moduleInOutPin[i],  pipelineOutputPin[i], eIasAudioPinLinkTypeImmediate);
    ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setupResult);
  }

  //start processing
  testfwxResult = testfwx->start();
  ASSERT_EQ(IasTestFramework::eIasOk, testfwxResult);

  uint32_t periodsCounter = 0; //number of periods already processed
  uint32_t numPeriodsToProcess = 1; //number of periods to process in one iteration

  //main processing loop
  do
  {
    // command and control for test run
    IasProperties cmdProperties;
    IasProperties returnProperties;
    int32_t cmdId;
    IasInt32Vector rampParams;
    IasInt32Vector muteParams;
    muteParams.resize(3);
    rampParams.resize(2);

    switch (periodsCounter)
    {
      case 1:
      {
        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetLoudness;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set<std::string>("loudness", "off");

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case 2:
      {
        IasIDebug::IasResult res = debug->startRecord("testFW_Probe",moduleInOutPin[0]->getParameters()->name,2);
        ASSERT_EQ(IasIDebug::eIasOk, res);
        break;
      }

      case 10:
      {
        IasIDebug::IasResult res = debug->stopProbing(moduleInOutPin[0]->getParameters()->name);
        ASSERT_EQ(IasIDebug::eIasOk, res);
        break;
      }

      case (uint32_t)(10*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", 0);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case (uint32_t)(20*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", -300);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case (uint32_t)(30*cIasSampleRate/cIasFrameLength):
      {
        std::cout << "activating loudness for all streams" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetLoudness;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set<std::string>("loudness", "on");

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case (uint32_t)(40*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", 0);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case (uint32_t)(50*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", -300);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case (uint32_t)(54*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 mutes of 0.1 sec ramptime" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetMuteState;
        cmdProperties.set("cmd", cmdId);
        muteParams[0] = true;
        muteParams[1] = 100;
        muteParams[2] = eIasRampShapeExponential;
        cmdProperties.set("params", muteParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case (uint32_t)(56*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 mutes of 0.1 sec ramptime" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetMuteState;
        cmdProperties.set("cmd", cmdId);
        muteParams[0] = false;
        muteParams[1] = 100;
        muteParams[2] = eIasRampShapeExponential;
        cmdProperties.set("params", muteParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      case(uint32_t)(60*cIasSampleRate/cIasFrameLength):
      {
        std::cout <<"Starting 3 ramps of 2 sec" <<std::endl;

        cmdProperties.clearAll();
        cmdId = IasVolume::eIasSetVolume;
        cmdProperties.set("cmd", cmdId);
        cmdProperties.set("volume", -1440);
        rampParams[0] = 2000;
        rampParams[1] = eIasRampShapeExponential;
        cmdProperties.set("ramp", rampParams);

        for (uint32_t i = 0; i < cNumStreams; i++)
        {
          cmdProperties.set("pin", moduleInOutPinParams[i].name);
          processing->sendCmd("VolumeLoudness", cmdProperties, returnProperties);
        }

        break;
      }
      default:
      {
        break;
      }
    }

    //process given number of periods
    testfwxResult = testfwx->process(numPeriodsToProcess);
    ASSERT_TRUE(IasTestFramework::eIasOk == testfwxResult || IasTestFramework::eIasFinished == testfwxResult);

    periodsCounter += numPeriodsToProcess;

  } while ( testfwxResult == IasTestFramework::eIasOk );

  //stop processing
  testfwxResult = testfwx->stop();
  ASSERT_EQ(IasTestFramework::eIasOk, testfwxResult);

  std::cout <<"All periods have been processed." << std::endl;

  //clean up
  for (uint32_t i = 0; i< cNumStreams; i++)
  {
    //unlink wave files from pins
    setup->unlinkWaveFile(pipelineInputPin[i]);
    setup->unlinkWaveFile(pipelineOutputPin[i]);

    setup->unlink(pipelineInputPin[i], moduleInOutPin[i]);
    setup->unlink(moduleInOutPin[i],  pipelineOutputPin[i]);

    //destroy audio pins
    setup->destroyAudioPin(&pipelineInputPin[i]);
    setup->destroyAudioPin(&pipelineOutputPin[i]);
    setup->destroyAudioPin(&moduleInOutPin[i]);
  }

  //destroy Volume processing module
  setup->destroyProcessingModule(&moduleVolume);

  //destroy test framework
  IasTestFramework::destroy(testfwx);

  //compare output files with reference files
  const char *referenceFileName0 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream0.wav");
  const char *referenceFileName1 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream1.wav");
  const char *referenceFileName2 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream2.wav");
  const char *referenceFileName3 = ADD_ROOT("reference_testVolumeLoudnessCore_outstream3.wav");

  float   difference[cNumStreams]     = { 0.0f, 0.0f, 0.0f, 0.0f };
  CompareWithReferenceFile(outputFileParams[0].fileName, referenceFileName0, difference[0]);
  CompareWithReferenceFile(outputFileParams[1].fileName, referenceFileName1, difference[1]);
  CompareWithReferenceFile(outputFileParams[2].fileName, referenceFileName2, difference[2]);
  CompareWithReferenceFile(outputFileParams[3].fileName, referenceFileName3, difference[3]);

  float thresholdDB  = -50.0f;

  for (uint32_t i = 0; i < cNumStreams; i++)
  {
    float differenceDB = 20.0f*static_cast<float>(log10(std::max(difference[i], (float)(1e-20))));
    printf("Stream %d: peak difference between processed output data and reference: %7.2f dBFS\n", i+1, differenceDB);

    if (differenceDB > thresholdDB)
    {
      printf("Error: Stream %d: Peak difference exceeds threshold of %7.2f dBFS.\n", i+1, thresholdDB);
    }
  }

  for (uint32_t i = 0; i < cNumStreams; i++)
  {
    float differenceDB = 20.0f*static_cast<float>(log10(std::max(difference[i], (float)(1e-20))));

    if (differenceDB > thresholdDB)
    {
      ASSERT_TRUE(false);
    }
  }
}
} // namespace IasAudio
