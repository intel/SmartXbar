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

#include <sndfile.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <iostream>
#include <gtest/gtest.h>

#include "internal/audio/smartx_test_support/IasTimeStampCounter.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

/*
 *  Set PROFILE to 1 to enable perf measurement
 */
#define PROFILE 0
#define REFERENCE 1

#ifndef NFS_PATH
#define NFS_PATH "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-02/"
#endif

#define ADD_ROOT(filename) NFS_PATH filename

using namespace IasAudio;

static const uint32_t cIasFrameLength  =    64;
static const uint32_t cIasSampleRate   = 44100;
static const uint32_t cNumStreams      =     4;
static const uint32_t cNumTestData     =     3;

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

class IasVolumeLoudnessSDVTest : public ::testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

    void setLoudness(const char *pin, const char *state);
    void setVolume(const char *pin, int32_t volume, int32_t rampTime, IasRampShapes rampShape);
    void setSpeed(int32_t speed);
    void setSdv(const char *pin, const char *state);

    IasIModuleId *mCmd;
};

void setSDVTableInProperties(const IasVolumeSDVTable &sdvTable, IasProperties &properties)
{
  IasInt32Vector speed;
  IasInt32Vector gain_inc;
  IasInt32Vector gain_dec;

  ASSERT_TRUE(   sdvTable.speed.size() > 0
              && sdvTable.speed.size() == sdvTable.gain_inc.size()
              && sdvTable.speed.size() == sdvTable.gain_dec.size());
  for (uint32_t i = 0; i < sdvTable.speed.size(); i++)
  {
    speed.push_back(sdvTable.speed[i]);
    gain_inc.push_back(sdvTable.gain_inc[i]);
    gain_dec.push_back(sdvTable.gain_dec[i]);
  }
  properties.set("sdv.speed", speed);
  properties.set("sdv.gain_inc", gain_inc);
  properties.set("sdv.gain_dec", gain_dec);
}

void IasVolumeLoudnessSDVTest::setLoudness(const char *pin, const char *state)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudness);
  cmdProperties.set("pin", std::string(pin));
  cmdProperties.set("loudness", std::string(state));
  IasIModuleId::IasResult cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}

void IasVolumeLoudnessSDVTest::setVolume(const char *pin, int32_t volume, int32_t rampTime, IasRampShapes rampShape)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set("pin", std::string(pin));
  cmdProperties.set("volume", volume);
  IasInt32Vector ramp;
  ramp.push_back(rampTime);
  ramp.push_back(static_cast<int32_t>(rampShape));
  cmdProperties.set("ramp", ramp);
  IasIModuleId::IasResult res = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, res);
}

void IasVolumeLoudnessSDVTest::setSpeed(int32_t speed)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSpeed);
  cmdProperties.set("speed", speed);
  IasIModuleId::IasResult res = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, res);
}

void IasVolumeLoudnessSDVTest::setSdv(const char *pin, const char *state)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSpeedControlledVolume);
  cmdProperties.set("pin", std::string(pin));
  cmdProperties.set("sdv", std::string(state));
  IasIModuleId::IasResult res = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, res);
}

TEST_F(IasVolumeLoudnessSDVTest, main_test)
{
  IasVolumeSDVTable sdvTable;

  const char *inputFileName1     = ADD_ROOT("sin500.wav");
  const char *inputFileName2     = ADD_ROOT("sin500.wav");
  const char *inputFileName3     = ADD_ROOT("Grummelbass_44100.wav");
  const char *inputFileName4     = ADD_ROOT("Grummelbass_44100_6chan.wav");
  const char *outputFileName1    = "test_outstream0.wav";
  const char *outputFileName2    = "test_outstream1.wav";
  const char *outputFileName3    = "test_outstream2.wav";
  const char *outputFileName4    = "test_outstream3.wav";
  const char *outputFileName5    = "speed.wav";
  const char *outputFileName6    = "speed_dependent_gain.wav";
  const char *outputFileName7    = "user_volume.wav";
#if REFERENCE
  const char *referenceFileName1 = ADD_ROOT("reference_SDV_outstream0x.wav");
  const char *referenceFileName2 = ADD_ROOT("reference_SDV_outstream1.wav");
  const char *referenceFileName3 = ADD_ROOT("reference_SDV_outstream2.wav");
  const char *referenceFileName4 = ADD_ROOT("reference_SDV_outstream3.wav");
#endif

  const char *inputFileName[cNumStreams]     = { inputFileName1, inputFileName2, inputFileName3, inputFileName4 };
  const char *outputFileName[cNumStreams+cNumTestData]    = { outputFileName1, outputFileName2, outputFileName3, outputFileName4, outputFileName5,outputFileName6, outputFileName7 };

  float   difference[cNumStreams]     = { 0.0f, 0.0f, 0.0f, 0.0f };

  SNDFILE *inputFile1     = NULL;
  SNDFILE *inputFile2     = NULL;
  SNDFILE *inputFile3     = NULL;
  SNDFILE *inputFile4     = NULL;
  SNDFILE *outputFile1    = NULL;
  SNDFILE *outputFile2    = NULL;
  SNDFILE *outputFile3    = NULL;
  SNDFILE *outputFile4    = NULL;
  SNDFILE *outputFile5    = NULL;
  SNDFILE *outputFile6    = NULL;
  SNDFILE *outputFile7    = NULL;

#if REFERENCE
  const char *referenceFileName[cNumStreams] = { referenceFileName1, referenceFileName2, referenceFileName3, referenceFileName4 };
  SNDFILE *referenceFile1 = NULL;
  SNDFILE *referenceFile2 = NULL;
  SNDFILE *referenceFile3 = NULL;
  SNDFILE *referenceFile4 = NULL;
  SNDFILE *referenceFile[cNumStreams] = { referenceFile1, referenceFile2, referenceFile3, referenceFile4 };
  SF_INFO  referenceFileInfo[cNumStreams];

  float  *referenceData[cNumStreams]; // pointers to buffers with interleaved samples
#endif

  float  *ioData[cNumStreams+cNumTestData];   // pointers to buffers with interleaved samples
  SNDFILE *inputFile[cNumStreams]       = { inputFile1, inputFile2, inputFile3, inputFile4 };
  SNDFILE *outputFile[cNumStreams+cNumTestData]    = { outputFile1, outputFile2, outputFile3, outputFile4, outputFile5, outputFile6, outputFile7};

  SF_INFO  inputFileInfo[cNumStreams];
  SF_INFO  outputFileInfo[cNumStreams+cNumTestData];



  uint32_t speed = 0;
  float sdvGain = 0;
  float volume = 0;

  // Open the input files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    inputFileInfo[cntFiles].format = 0;
    inputFile[cntFiles] = sf_open(inputFileName[cntFiles], SFM_READ, &inputFileInfo[cntFiles]);
    if (NULL == inputFile[cntFiles])
    {
      std::cout<<"Could not open file " <<inputFileName[cntFiles] <<std::endl;
      ASSERT_TRUE(false);
    }
  }

  // Open the output files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < (cNumStreams+cNumTestData); cntFiles++)
  {
    if(cntFiles < cNumStreams)
    {
      outputFileInfo[cntFiles].samplerate = inputFileInfo[cntFiles].samplerate;
      outputFileInfo[cntFiles].channels   = inputFileInfo[cntFiles].channels;
      outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
      outputFile[cntFiles] = sf_open(outputFileName[cntFiles], SFM_WRITE, &outputFileInfo[cntFiles]);
      if (NULL == outputFile[cntFiles])
      {
        std::cout<<"Could not open file " <<outputFileName[cntFiles] <<std::endl;
        ASSERT_TRUE(false);
      }
    }
    else
    {
      outputFileInfo[cntFiles].samplerate = inputFileInfo[cntFiles-1].samplerate;
      outputFileInfo[cntFiles].channels   = 1;
      if(cntFiles >= (cNumStreams+1))
      {
        outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);
      }
      else
      {
        outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
      }
      outputFile[cntFiles] = sf_open(outputFileName[cntFiles], SFM_WRITE, &outputFileInfo[cntFiles]);
      if (NULL == outputFile[cntFiles])
      {
        std::cout<<"Could not open file " <<outputFileName[cntFiles] <<std::endl;
        ASSERT_TRUE(false);
      }
    }
  }
#if REFERENCE
  // Open the reference files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    referenceFileInfo[cntFiles].format = 0;
    referenceFile[cntFiles] = sf_open(referenceFileName[cntFiles], SFM_READ, &referenceFileInfo[cntFiles]);
    if (NULL == referenceFile[cntFiles])
    {
      std::cout<<"Could not open file " <<referenceFileName[cntFiles] <<std::endl;
      ASSERT_TRUE(false);
    }
    if (referenceFileInfo[cntFiles].channels != inputFileInfo[cntFiles].channels)
    {
      std::cout<<"File " <<referenceFileName[cntFiles] << ": number of channels does not match!" << std::endl;
      ASSERT_TRUE(false);
    }
  }
#endif

  uint32_t nInputChannels = 0;
  for (uint32_t cntFiles = 0; cntFiles < (cNumStreams+cNumTestData); cntFiles++)
  {
    if(cntFiles < cNumStreams)
    {
      uint32_t numChannels = static_cast<uint32_t>(inputFileInfo[cntFiles].channels);
      ioData[cntFiles]        = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);
#if REFERENCE
      referenceData[cntFiles] = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);
#endif
      nInputChannels += numChannels;
    }
    else
    {
      uint32_t numChannels = 1;
      ioData[cntFiles]        = (float* )malloc(sizeof(float) * cIasFrameLength * numChannels);
      nInputChannels += numChannels;
    }
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
  mCmd   = myVolume->getCmdInterface();
  ASSERT_TRUE(mCmd != nullptr);

  volumeCore->reset();

  sdvTable.speed.push_back(0);
  sdvTable.gain_dec.push_back(0);
  sdvTable.gain_inc.push_back(0);

  sdvTable.speed.push_back(25);
  sdvTable.gain_dec.push_back(20);
  sdvTable.gain_inc.push_back(20);

  sdvTable.speed.push_back(50);
  sdvTable.gain_dec.push_back(30);
  sdvTable.gain_inc.push_back(30);

  sdvTable.speed.push_back(75);
  sdvTable.gain_dec.push_back(40);
  sdvTable.gain_inc.push_back(40);

  sdvTable.speed.push_back(100);
  sdvTable.gain_dec.push_back(50);
  sdvTable.gain_inc.push_back(50);

  sdvTable.speed.push_back(125);
  sdvTable.gain_dec.push_back(60);
  sdvTable.gain_inc.push_back(60);

  sdvTable.speed.push_back(150);
  sdvTable.gain_dec.push_back(70);
  sdvTable.gain_inc.push_back(70);

  sdvTable.speed.push_back(175);
  sdvTable.gain_dec.push_back(80);
  sdvTable.gain_inc.push_back(80);

  sdvTable.speed.push_back(200);
  sdvTable.gain_dec.push_back(120);
  sdvTable.gain_inc.push_back(120);

  IasProperties cmdProperties;
  IasProperties returnProperties;
  setSDVTableInProperties(sdvTable, cmdProperties);
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSdvTable);
  IasIModuleId::IasResult cmdres;
  cmdres = mCmd->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);

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
    float* tempData5 = ioData[4];
    float* tempData6 = ioData[5];
    float* tempData7 = ioData[6];

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
    switch (frameCounter)
    {
      case 1:
        setLoudness("Sink0", "off");
        setLoudness("Sink1", "off");
        setLoudness("Sink2", "off");
        setLoudness("Sink3", "off");
        std::cout <<std::endl <<"##### Starting ramp for stream 0 of 200 ms to 0.5" <<std::endl <<std::endl;
        setVolume("Sink0", -60, 200, eIasRampShapeLinear);
        std::cout <<std::endl <<"##### Starting ramp for stream 1 of 200 ms to 0.5" <<std::endl <<std::endl;
        setVolume("Sink1", -60, 200, eIasRampShapeExponential);
        std::cout <<std::endl <<"##### Starting ramp for stream 2 of 200 ms to 0.5" <<std::endl <<std::endl;
        setVolume("Sink2", -60, 200, eIasRampShapeExponential);
        std::cout <<std::endl <<"##### Starting ramp for stream 3 of 200 ms to 0.5" <<std::endl <<std::endl;
        setVolume("Sink3", -60, 200, eIasRampShapeExponential);
      break;

      case (uint32_t)(8*cIasSampleRate/64):
         speed = 200;
         std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
         setSpeed(speed);
      break;
      case (uint32_t)(10*cIasSampleRate/64):
         volume = 0.8f;
         std::cout <<std::endl <<"##### Starting ramp for stream 0 of 200 ms to 0.8" <<std::endl <<std::endl;
         setVolume("Sink0", -19, 2000, eIasRampShapeLinear);
      break;
      case (uint32_t)(12*cIasSampleRate/64):
         volume = 0.4f;
         std::cout <<std::endl <<"##### Starting ramp for stream 0 of 200 ms to 0.4" <<std::endl <<std::endl;
         setVolume("Sink0", -79, 200, eIasRampShapeLinear);
      break;
      case (uint32_t)(14*cIasSampleRate/64):
         speed = 100;
         std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
         setSpeed(speed);
      break;
      case (uint32_t)(16*cIasSampleRate/64):
         speed = 150;
         std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
         setSpeed(speed);
      break;


      case (uint32_t)(22*cIasSampleRate/64):
        std::cout <<std::endl <<"Disabling SDV for stream 0"  <<std::endl<<std::endl;
        setSdv("Sink0", "off");
        std::cout  <<std::endl<<"Disabling SDV for stream 1"  <<std::endl <<std::endl;
        setSdv("Sink1", "off");
        std::cout <<std::endl <<"Disabling SDV for stream 2" <<std::endl <<std::endl;
        setSdv("Sink2", "off");
        std::cout <<std::endl <<"Disabling SDV for stream 3" <<std::endl <<std::endl;
        setSdv("Sink3", "off");
        break;
      case (uint32_t)(24*cIasSampleRate/64):
        std::cout <<std::endl <<"Enabling SDV for stream 0" <<std::endl <<std::endl;
        setSdv("Sink0", "on");
        std::cout <<std::endl <<"Enabling SDV for stream 1" <<std::endl <<std::endl;
        setSdv("Sink1", "on");
        std::cout <<std::endl <<"Enabling SDV for stream 2" <<std::endl <<std::endl;
        setSdv("Sink2", "on");
        std::cout <<std::endl <<"Enabling SDV for stream 3" <<std::endl <<std::endl;
        setSdv("Sink3", "on");
        break;
      case (uint32_t)(26*cIasSampleRate/64):
        speed = 100;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
        break;
      case (uint32_t)(28*cIasSampleRate/64):
        speed = 50;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(30*cIasSampleRate/64):
        speed = 25;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(32*cIasSampleRate/64):
        speed = 200;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(34*cIasSampleRate/64):
        speed = 175;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(36*cIasSampleRate/64):
        speed = 150;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(38*cIasSampleRate/64):
        speed = 125;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(40*cIasSampleRate/64):
        speed = 100;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(42*cIasSampleRate/64):
        speed = 75;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(44*cIasSampleRate/64):
        speed = 50;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(46*cIasSampleRate/64):
        speed = 25;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
      break;
      case (uint32_t)(48*cIasSampleRate/64):
        speed = 0;
        std::cout <<std::endl <<"###### Setting new speed:" <<speed <<"km/h" <<std::endl <<std::endl;
        setSpeed(speed);
        break;
      default:
        break;
    }

#if PROFILE
    timeStampCounter->reset();
#endif

    // call process of audio component under test.
    volumeCore->process();
    // Not possible to retrieve from cmd interface.
    sdvGain = 0;
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
    tempData5 = ioData[4];
    tempData6 = ioData[5];
    tempData7 = ioData[6];

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
      *tempData5++ = ((float)speed) *0.001f;
      *tempData6++ = sdvGain * 0.1f;
      *tempData7++ = volume;
    }

    for (uint32_t cntFiles = 0; cntFiles < (cNumStreams+cNumTestData); cntFiles++)
    {
      // Write the interleaved PCM samples into the output WAVE file.
      if(cntFiles < cNumStreams)
      {
        nWrittenSamples = (uint32_t) sf_writef_float(outputFile[cntFiles], ioData[cntFiles], (sf_count_t)nReadSamples[cntFiles]);
        if (nWrittenSamples != nReadSamples[cntFiles])
        {
          std::cout<< "write to file " << outputFileName[cntFiles] <<"failed" <<std::endl;
          ASSERT_TRUE(false);
        }
#if REFERENCE
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
#endif
      }
      else
      {
        nWrittenSamples = (uint32_t) sf_writef_float(outputFile[cntFiles], ioData[cntFiles], (sf_count_t)cIasFrameLength);

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

  printf("####### Closing all files!!!!\n");

  // Close all files.
  for (uint32_t cntFiles = 0; cntFiles < (cNumStreams+cNumTestData); cntFiles++)
  {
    if(cntFiles < cNumStreams)
    {
      if (sf_close(inputFile[cntFiles]))
      {
        std::cout<< "close of file " << inputFileName[cntFiles] <<" failed" <<std::endl;
        ASSERT_TRUE(false);
      }
#if REFERENCE
      if (sf_close(referenceFile[cntFiles]))
      {
        std::cout<< "close of file " << referenceFileName[cntFiles] <<" failed" <<std::endl;
        ASSERT_TRUE(false);
      }
#endif
    }
    if (sf_close(outputFile[cntFiles]))
    {
      std::cout<< "close of file " << outputFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_TRUE(false);
    }

  }

  pluginEngine.destroyModule(myVolume);
  delete(myAudioChain);

  for (uint32_t cntFiles = 0; cntFiles < (cNumStreams+cNumTestData); cntFiles++)
  {
    free(ioData[cntFiles]);
#if REFERENCE
    if(cntFiles < cNumStreams)
    {
      free(referenceData[cntFiles]);
    }
#endif
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

