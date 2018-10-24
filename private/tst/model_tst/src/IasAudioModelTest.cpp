/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

/**
 * @file IasAudioModelTest.cpp
 * @brief Unit test to cover all uncovered Audio model methods
 */

#include "gtest/gtest.h"
#include "internal/audio/common/IasAudioLogging.hpp"

#include "audio/common/IasAudioCommonTypes.hpp"
#include "model/IasAudioDevice.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasPipeline.hpp"
#include "smartx/IasConfiguration.hpp"
#include "model/IasProcessingModule.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "model/IasRoutingZoneWorkerThread.hpp"
#include "alsahandler/IasAlsaHandler.hpp"
#include <alsa/asoundlib.h>

#ifndef PLUGHW_DEVICE_NAME
#define PLUGHW_DEVICE_NAME  "plughw:31,0"
#endif

using namespace IasAudio;

class IasAudioModelTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

class MyTestDevice : public IasAudioPortOwner
{
  public:
    MyTestDevice(){};
    virtual ~MyTestDevice(){};

    virtual uint32_t getSampleRate() const {return 0;}
    virtual IasAudioCommonDataFormat getSampleFormat() const {return eIasFormatUndef;}
    virtual uint32_t getPeriodSize() const {return 0;}
    virtual IasSwitchMatrixPtr getSwitchMatrix() const {return nullptr;}
    virtual std::string getName() const {return "dummy";}

};


static int set_hwparams(snd_pcm_t         *handle,
                        snd_pcm_access_t   access,
                        snd_pcm_sframes_t *buffer_size, // output
                        snd_pcm_sframes_t *period_size) // output
{
  snd_pcm_format_t format = SND_PCM_FORMAT_S16; /* sample format */
  unsigned int rate = 48000;      /* stream rate */
  unsigned int channels = 2;     /* count of channels */
  unsigned int buffer_time = 50000;    /* ring buffer length in us */
  unsigned int period_time = 10000;    /* period time in us */
  int resample = 1;       /* enable alsa-lib resampling */

  snd_pcm_hw_params_t* params;
  snd_pcm_hw_params_alloca(&params);

  unsigned int rrate;
  snd_pcm_uframes_t size;
  int err, dir;

  /* choose all parameters */
  err = snd_pcm_hw_params_any(handle, params);
  if (err < 0) {
    printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
    return err;
  }
  /* set hardware resampling */
  err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
  if (err < 0) {
    printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
    return err;
  }
  /* set the interleaved read/write format */
  err = snd_pcm_hw_params_set_access(handle, params, access);
  if (err < 0) {
    printf("Access type not available for playback: %s\n", snd_strerror(err));
    return err;
  }
  /* set the sample format */
  err = snd_pcm_hw_params_set_format(handle, params, format);
  if (err < 0) {
    printf("Sample format not available for playback: %s\n", snd_strerror(err));
    return err;
  }
  /* set the count of channels */
  err = snd_pcm_hw_params_set_channels(handle, params, channels);
  if (err < 0) {
    printf("Channels count (%i) not available for playbacks: %s\n", channels, snd_strerror(err));
    return err;
  }
  /* set the stream rate */
  rrate = rate;
  err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
  if (err < 0) {
    printf("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
    return err;
  }
  if (rrate != rate) {
    printf("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
    return -EINVAL;
  }
  /* set the buffer time */
  err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
  if (err < 0) {
    printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
    return err;
  }
  err = snd_pcm_hw_params_get_buffer_size(params, &size);
  if (err < 0) {
    printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
    return err;
  }
  *buffer_size = size;
  /* set the period time */
  err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
  if (err < 0) {
    printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
    return err;
  }
  err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
  if (err < 0) {
    printf("Unable to get period size for playback: %s\n", snd_strerror(err));
    return err;
  }
  *period_size = size;
  /* write the parameters to device */
  err = snd_pcm_hw_params(handle, params);
  if (err < 0) {
    printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
    return err;
  }
  return 0;
}

static int set_swparams(snd_pcm_t         *handle,
                        snd_pcm_sframes_t  buffer_size,
                        snd_pcm_sframes_t  period_size)
{
  int err;
  int period_event = 0;       /* produce poll event after each period */

  snd_pcm_sw_params_t* swparams;
  snd_pcm_sw_params_alloca(&swparams);

  /* get the current swparams */
  err = snd_pcm_sw_params_current(handle, swparams);
  if (err < 0) {
    printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
    return err;
  }
  /* start the transfer when the buffer is almost full: */
  /* (buffer_size / avail_min) * avail_min */
  err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 1);
  if (err < 0) {
    printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
    return err;
  }
  /* allow the transfer when at least period_size samples can be processed */
  /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
  err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
  if (err < 0) {
    printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
    return err;
  }
  /* enable period events when requested */
  if (period_event) {
    err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
    if (err < 0) {
      printf("Unable to set period event: %s\n", snd_strerror(err));
      return err;
    }
  }
  /* write the parameters to the playback device */
  err = snd_pcm_sw_params(handle, swparams);
  if (err < 0) {
    printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
    return err;
  }
  return 0;
}



int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(true);

  IasAudioLogging::addDltContextItem("SMJ", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("MDL", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("BFT", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

TEST_F(IasAudioModelTest, zoneWorkerThread)
{

  IasAudioRingBufferFactory* factory = IasAudioRingBufferFactory::getInstance();
  ASSERT_TRUE( factory != nullptr);

  IasRoutingZoneParamsPtr zoneParams = std::make_shared<IasRoutingZoneParams>("dummy");
  IasRoutingZoneWorkerThread* myWorker = new IasRoutingZoneWorkerThread(zoneParams);
  ASSERT_TRUE(myWorker != nullptr);
  IasRoutingZoneWorkerThread::IasResult res = myWorker->stop();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasNotInitialized);
  res = myWorker->start();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasNotInitialized);
  res = myWorker->init();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);
  res = myWorker->init();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);

  IasAudioDeviceParamsPtr devParamsPtr = std::make_shared<IasAudioDeviceParams>();
  devParamsPtr->dataFormat = eIasFormatFloat32;
  devParamsPtr->name = PLUGHW_DEVICE_NAME;
  devParamsPtr->numPeriods = 4;
  devParamsPtr->periodSize = 0;
  devParamsPtr->samplerate = 48000;

  IasAudioSinkDevicePtr audioDev = std::make_shared<IasAudioSinkDevice>(devParamsPtr);
  ASSERT_TRUE(audioDev != nullptr);

  res = myWorker->linkAudioSinkDevice(audioDev);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasFailed);
  audioDev = nullptr;

  devParamsPtr->periodSize = 192;
  audioDev = std::make_shared<IasAudioSinkDevice>(devParamsPtr);
  ASSERT_TRUE(audioDev != nullptr);
  res = myWorker->linkAudioSinkDevice(audioDev);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);


  myWorker->unlinkAudioSinkDevice();
  audioDev = nullptr;

  IasPipelineParamsPtr pipeParams = std::make_shared<IasPipelineParams>();
  pipeParams->name = "myPipe";
  pipeParams->periodSize = devParamsPtr->periodSize;
  pipeParams->samplerate = 48000;

  IasConfigurationPtr myConfig = std::make_shared<IasConfiguration>();
  ASSERT_TRUE(myConfig != nullptr);
  IasCmdDispatcherPtr myCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  ASSERT_TRUE(myCmdDispatcher != nullptr);
  IasPluginEnginePtr pluginEngine = std::make_shared<IasPluginEngine>(myCmdDispatcher);

  IasPipelinePtr pipeline = std::make_shared<IasPipeline>(pipeParams,pluginEngine,myConfig);
  ASSERT_TRUE(pipeline != nullptr);
  IasPipelinePtr pipeline2 = std::make_shared<IasPipeline>(pipeParams,pluginEngine,myConfig);
  ASSERT_TRUE(pipeline != nullptr);

  res = myWorker->addPipeline(pipeline);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);
  res = myWorker->addPipeline(pipeline2);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasFailed);
  res = myWorker->addPipeline(pipeline);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);

  devParamsPtr->periodSize = 64;
  audioDev = std::make_shared<IasAudioSinkDevice>(devParamsPtr);
  ASSERT_TRUE(audioDev != nullptr);
  res = myWorker->linkAudioSinkDevice(audioDev);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasFailed);
  audioDev = nullptr;

  devParamsPtr->periodSize = pipeParams->periodSize;
  devParamsPtr->samplerate = 0;
  audioDev = std::make_shared<IasAudioSinkDevice>(devParamsPtr);
  ASSERT_TRUE(audioDev != nullptr);
  res = myWorker->linkAudioSinkDevice(audioDev);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasFailed);
  audioDev = nullptr;

  devParamsPtr->samplerate =  pipeParams->samplerate;
  devParamsPtr->numChannels = 2;
  devParamsPtr->clockType = eIasClockReceived;

  audioDev = std::make_shared<IasAudioSinkDevice>(devParamsPtr);
  ASSERT_TRUE(audioDev != nullptr);
  std::shared_ptr<IasAlsaHandler> concreteDevice = std::make_shared<IasAlsaHandler>(devParamsPtr);
  ASSERT_TRUE(concreteDevice != nullptr);
  IasAlsaHandler::IasResult ahRes = concreteDevice->init(eIasDeviceTypeSink);
  ASSERT_TRUE(ahRes == IasAlsaHandler::eIasOk);
  audioDev->setConcreteDevice(concreteDevice);
  IasAudioPortParamsPtr portParams = std::make_shared<IasAudioPortParams>();
  portParams->name = "sinkPort";
  portParams->id = -1;
  portParams->direction = eIasPortDirectionInput;
  portParams->index = 0;
  portParams->numChannels = 2;
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(portParams);

  IasAudioRingBuffer* sinkBuf;

  //Covering situation when buffer is nullptr
  sinkBuf = nullptr;
  IasAudioPort::IasResult portRes = sinkPort->setRingBuffer(sinkBuf);
  ASSERT_TRUE(portRes == IasAudioPort::eIasFailed);

  factory->createRingBuffer(&sinkBuf,192,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuff");
  ASSERT_TRUE( sinkBuf != nullptr);
  portRes = sinkPort->setRingBuffer(sinkBuf);
  ASSERT_TRUE(portRes == IasAudioPort::eIasOk);

  IasAudioCommonResult sinkRes = audioDev->addAudioPort(sinkPort);
  ASSERT_TRUE(sinkRes == IasAudioCommonResult::eIasResultOk);

  //Cover double deletion of deleteAudioPort method
  audioDev->deleteAudioPort(sinkPort);
  audioDev->deleteAudioPort(sinkPort);

  //Add audioPort again for next tests
  sinkRes = audioDev->addAudioPort(sinkPort);
  ASSERT_TRUE(sinkRes == IasAudioCommonResult::eIasResultOk);

  res = myWorker->linkAudioSinkDevice(audioDev);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);


  myWorker->deletePipeline();
  res = myWorker->addPipeline(pipeline);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);
  myWorker->deletePipeline();

  portParams->name = "zoneIn";
  portParams->id = 42;
  portParams->direction = eIasPortDirectionInput;
  portParams->index = 0;
  portParams->numChannels = 2;
  IasAudioPortPtr rznPort = std::make_shared<IasAudioPort>(portParams);


  IasAudioRingBuffer* convBuf;
  factory->createRingBuffer(&convBuf,192,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"convBuff");
  ASSERT_TRUE( convBuf != nullptr);
  res = myWorker->addConversionBuffer(rznPort,convBuf);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);

  res = myWorker->startProbing("test",false,5,2,2);
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);

  IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();
  res = myWorker->start();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasFailed);
  myWorker->setSwitchMatrix(switchMatrix);

  res = myWorker->prepareStates();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);
  res = myWorker->start();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);

  myWorker->shutDown();

  res = myWorker->stop();
  ASSERT_TRUE(res == IasRoutingZoneWorkerThread::eIasOk);
  audioDev = nullptr;

  factory->destroyRingBuffer(sinkBuf);
  factory->destroyRingBuffer(convBuf);
  delete myWorker;

}

TEST_F(IasAudioModelTest, tstAudioDevice)
{
  IasAudioDeviceParamsPtr devParamsPtr = std::make_shared<IasAudioDeviceParams>();
  devParamsPtr->dataFormat = eIasFormatFloat32;
  devParamsPtr->name = "test";
  devParamsPtr->numPeriods = 4;
  devParamsPtr->numChannels = 8;
  IasAudioDevice *audioDev = new IasAudioDevice(devParamsPtr);
  ASSERT_TRUE(audioDev != nullptr);
  IasAudioCommonDataFormat dataFormat = audioDev->getSampleFormat();
  ASSERT_EQ(eIasFormatFloat32, dataFormat);
  devParamsPtr->dataFormat = eIasFormatInt32;
  dataFormat = audioDev->getSampleFormat();
  ASSERT_EQ(eIasFormatInt32, dataFormat);
  std::string name = audioDev->getName();
  EXPECT_TRUE(name.compare(devParamsPtr->name) == 0);
  uint32_t numPeriods = audioDev->getNumPeriods();
  EXPECT_TRUE(numPeriods == devParamsPtr->numPeriods);
  uint32_t numChannels = audioDev->getNumChannels();
  EXPECT_TRUE(numChannels == devParamsPtr->numChannels);
  EXPECT_TRUE(nullptr == audioDev->getWorkerThread());
  delete audioDev;
}

TEST_F(IasAudioModelTest, tstAudioPort)
{
  IasAudioRingBufferFactory* factory = IasAudioRingBufferFactory::getInstance();

  IasAudioPortParamsPtr portParamsPtr = std::make_shared<IasAudioPortParams>("dummy",2,22,eIasPortDirectionInput,0);

  IasAudioPortPtr portPtr = std::make_shared<IasAudioPort>(portParamsPtr);

  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->setOwner(nullptr));

  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->getOwner(nullptr));

  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->getRingBuffer(nullptr));


  IasSwitchMatrixPtr switchMatrix1 = std::make_shared<IasSwitchMatrix>();
  IasSwitchMatrixPtr switchMatrix2 = std::make_shared<IasSwitchMatrix>();

  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->storeConnection(switchMatrix1));
  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->forgetConnection(switchMatrix2));
  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->forgetConnection(switchMatrix1));

  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->getCopyInformation(nullptr));

  IasAudioRingBuffer* ringBufR = nullptr;
  IasAudioRingBuffer* ringBufM = nullptr;
  EXPECT_EQ(eIasResultOk, factory->createRingBuffer(&ringBufR,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"dummyRingBufR"));
  EXPECT_EQ(eIasResultOk, factory->createRingBuffer(&ringBufM,64,4,2,eIasFormatInt16,eIasRingBufferLocalMirror,"dummyRingBufM"));

  snd_pcm_t* handle;
  snd_pcm_sframes_t buffer_size;
  snd_pcm_sframes_t period_size;
  ASSERT_EQ(0, snd_pcm_open(&handle, PLUGHW_DEVICE_NAME, SND_PCM_STREAM_PLAYBACK, 0));
  ASSERT_EQ(0, set_hwparams(handle, SND_PCM_ACCESS_MMAP_INTERLEAVED, &buffer_size, &period_size));
  ASSERT_EQ(0, set_swparams(handle, buffer_size, period_size));
  EXPECT_EQ(IasAudioRingBufferResult::eIasRingBuffOk,ringBufM->setDeviceHandle(handle,64,4));
  IasAudioPortCopyInformation copyInfos;
  copyInfos.areas = nullptr;
  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->getCopyInformation(&copyInfos));
  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->setRingBuffer(ringBufR));

  EXPECT_EQ(IasAudioPort::eIasFailed,portPtr->getCopyInformation(&copyInfos));
  IasAudioDeviceParamsPtr srcDevParamsPtr = std::make_shared<IasAudioDeviceParams>("dummyDevice",2,48000,eIasFormatInt16,eIasClockProvided,64,4);
  IasAudioSourceDevicePtr srcDevicePtr = std::make_shared<IasAudioSourceDevice>(srcDevParamsPtr);

  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->setOwner(srcDevicePtr));
  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->getCopyInformation(&copyInfos));

  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->setRingBuffer(ringBufM));
  EXPECT_EQ(IasAudioPort::eIasOk,portPtr->getCopyInformation(&copyInfos));

  factory->destroyRingBuffer(ringBufR);
  factory->destroyRingBuffer(ringBufM);
  portParamsPtr = nullptr;
  portPtr = nullptr;
  srcDevParamsPtr = nullptr;
  srcDevicePtr = nullptr;
}

TEST_F(IasAudioModelTest, audioPin)
{

  IasAudioPinParamsPtr pinParamsPtr = std::make_shared<IasAudioPinParams>("myPin",2);
  IasAudioPinPtr myPin = std::make_shared<IasAudioPin>(pinParamsPtr);

  ASSERT_TRUE(myPin != nullptr);

 // setDirection(IasPinDirection direction)

  IasAudioPin::IasResult res = myPin->setDirection(IasAudioPin::eIasPinDirectionPipelineInput);
  std::cout << "Result of setDirection:" <<toString(res).c_str() <<std::endl;
  ASSERT_TRUE(res == IasAudioPin::eIasOk);
  res = myPin->setDirection(IasAudioPin::eIasPinDirectionPipelineOutput);
  ASSERT_TRUE(res != IasAudioPin::eIasOk);

  //create a pipeline !!!
  IasCmdDispatcherPtr myCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEnginePtr myPluginEngine = std::make_shared<IasPluginEngine>(myCmdDispatcher);
  ASSERT_TRUE(myPluginEngine != nullptr);
  IasConfigurationPtr myConfig = std::make_shared<IasConfiguration>();
  ASSERT_TRUE(myConfig != nullptr);
  IasPipelineParamsPtr pipeParams = std::make_shared<IasPipelineParams>("myPipe",48000,192);
  IasPipelinePtr myPipeline = std::make_shared<IasPipeline>(pipeParams,myPluginEngine,myConfig);
  ASSERT_TRUE(myPipeline != nullptr);
  pipeParams->name = "myPipe2";
  IasPipelinePtr myPipeline2 = std::make_shared<IasPipeline>(pipeParams,myPluginEngine,myConfig);
  ASSERT_TRUE(myPipeline != nullptr);

  res = myPin->addToPipeline(myPipeline,"Kasperle");
  ASSERT_TRUE(res == IasAudioPin::eIasOk);

  res = myPin->addToPipeline(myPipeline2,"Seppl");
  ASSERT_TRUE(res != IasAudioPin::eIasOk);

  myPin = nullptr;
  pinParamsPtr = nullptr;
}


TEST_F(IasAudioModelTest, processingModule)
{
  IasAudioPinParamsPtr pinParamsPtr = std::make_shared<IasAudioPinParams>("myPin",2);
  IasAudioPinPtr myPin = std::make_shared<IasAudioPin>(pinParamsPtr);

  IasAudioPinParamsPtr pinParamsPtr2 = std::make_shared<IasAudioPinParams>("myPin2",2);
  IasAudioPinPtr myPin2 = std::make_shared<IasAudioPin>(pinParamsPtr2);

  IasAudioPinParamsPtr pinParamsPtr3 = std::make_shared<IasAudioPinParams>("myPin3",2);
  IasAudioPinPtr myPin3 = std::make_shared<IasAudioPin>(pinParamsPtr3);

  IasAudioPinParamsPtr pinParamsPtr4 = std::make_shared<IasAudioPinParams>("myPin4",2);
  IasAudioPinPtr myPin4 = std::make_shared<IasAudioPin>(pinParamsPtr4);

  IasAudioPinParamsPtr pinParamsPtr5 = std::make_shared<IasAudioPinParams>("myPin5",2);
  IasAudioPinPtr myPin5 = std::make_shared<IasAudioPin>(pinParamsPtr5);

  IasAudioPinParamsPtr pinParamsPtr6 = std::make_shared<IasAudioPinParams>("myPin6",2);
  IasAudioPinPtr myPin6 = std::make_shared<IasAudioPin>(pinParamsPtr6);

  ASSERT_TRUE(myPin != nullptr);
  ASSERT_TRUE(myPin2 != nullptr);
  ASSERT_TRUE(myPin3 != nullptr);
  ASSERT_TRUE(myPin4 != nullptr);
  ASSERT_TRUE(myPin5 != nullptr);
  ASSERT_TRUE(myPin6 != nullptr);

  IasProcessingModuleParamsPtr params = std::make_shared<IasProcessingModuleParams>("ias.volume","myTestVol");
  IasProcessingModule* myModule = new IasProcessingModule(params);
  ASSERT_TRUE(myModule != nullptr);

  //create a pipeline !!!
  IasCmdDispatcherPtr myCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEnginePtr myPluginEngine = std::make_shared<IasPluginEngine>(myCmdDispatcher);
  ASSERT_TRUE(myPluginEngine != nullptr);
  IasConfigurationPtr myConfig = std::make_shared<IasConfiguration>();
  ASSERT_TRUE(myConfig != nullptr);
  IasPipelineParamsPtr pipeParams = std::make_shared<IasPipelineParams>("myPipe",48000,192);
  IasPipelinePtr myPipeline = std::make_shared<IasPipeline>(pipeParams,myPluginEngine,myConfig);
  ASSERT_TRUE(myPipeline != nullptr);

  pipeParams->name = "myPipe2";
  IasPipelinePtr myPipeline2 = std::make_shared<IasPipeline>(pipeParams,myPluginEngine,myConfig);
  ASSERT_TRUE(myPipeline2 != nullptr);

  myPin3->addToPipeline(myPipeline2,"Kasperle");
  myPin4->setDirection(IasAudioPin::eIasPinDirectionModuleInputOutput);
  myPin5->setDirection(IasAudioPin::eIasPinDirectionModuleOutput);


  IasProcessingModule::IasResult procres = myModule->addToPipeline(myPipeline,"Sepp");
  ASSERT_TRUE(procres == IasProcessingModule::eIasOk);
  procres = myModule->addToPipeline(myPipeline,"Sepp");
  toString(procres);
  ASSERT_TRUE(procres != IasProcessingModule::eIasOk);

  procres = myModule->addAudioPinMapping(myPin, myPin2);
  ASSERT_TRUE(procres == IasProcessingModule::eIasOk);
  procres = myModule->addAudioPinMapping(myPin2, myPin);
  ASSERT_TRUE(procres != IasProcessingModule::eIasOk);

  procres = myModule->addAudioPinMapping(myPin3, myPin);
  ASSERT_TRUE(procres != IasProcessingModule::eIasOk);

  procres = myModule->addAudioPinMapping(myPin, myPin4);
  ASSERT_TRUE(procres != IasProcessingModule::eIasOk);

  procres = myModule->addAudioInOutPin(myPin5);
  ASSERT_TRUE(procres != IasProcessingModule::eIasOk);

  myModule->deleteAudioPinMapping(myPin2, myPin );
  myModule->deleteAudioPinMapping(myPin, myPin );
  myModule->deleteAudioPinMapping(myPin, myPin5 );

  myModule->deleteAudioInOutPin(nullptr);

  procres = myModule->addAudioInOutPin(myPin6);
  ASSERT_TRUE(procres == IasProcessingModule::eIasOk);

  myModule->deleteAudioInOutPin(myPin6);
  myModule->deleteAudioInOutPin(myPin5);
  myModule->deleteAudioInOutPin(myPin4);

  delete myModule;
  myPluginEngine = nullptr;
  myConfig = nullptr;
  myPipeline = nullptr;

  myPin  = nullptr;
  myPin2 = nullptr;
  myPin3 = nullptr;
  myPin4 = nullptr;
  myPin5 = nullptr;
  myPin6 = nullptr;

  pinParamsPtr  = nullptr;
  pinParamsPtr2 = nullptr;
  pinParamsPtr3 = nullptr;
  pinParamsPtr4 = nullptr;
  pinParamsPtr5 = nullptr;
  pinParamsPtr6 = nullptr;
  params = nullptr;
  myPipeline  = nullptr;
  myPipeline2 = nullptr;
}

TEST_F(IasAudioModelTest, audiodevice)
{
  IasAudioDeviceParamsPtr params = std::make_shared<IasAudioDeviceParams>("myDevice",2,48000,eIasFormatInt16,eIasClockProvided,192,4);

  IasAudioSinkDevice* mySinkDevice = new IasAudioSinkDevice(params);

  IasSmartXClientPtr myClient = nullptr;
  IasAlsaHandlerPtr  myHandler = nullptr;

  mySinkDevice->setConcreteDevice(myClient);
  mySinkDevice->setConcreteDevice(myHandler);

  mySinkDevice->getConcreteDevice(static_cast<IasAlsaHandlerPtr*>(nullptr));
  mySinkDevice->getConcreteDevice(static_cast<IasSmartXClientPtr*>(nullptr));

  //Cover getRingBuffer 'else if'
  IasAudioRingBuffer* tst = mySinkDevice->getRingBuffer();
  ASSERT_TRUE( tst == nullptr);

  delete mySinkDevice;

}

TEST_F(IasAudioModelTest, portOwnerCoverage)
{
  MyTestDevice* myTestDev = new MyTestDevice();

  ASSERT_TRUE( myTestDev->isSmartXClient() == false);
  ASSERT_TRUE( myTestDev->isAlsaHandler() == false);
  ASSERT_TRUE( myTestDev->getRoutingZone() == nullptr);

  delete myTestDev;
}

TEST_F(IasAudioModelTest, runnerThreadCoverage)
{
  IasRunnerThread *myRunner = new IasRunnerThread(4, "TheParentZone");
  ASSERT_TRUE(myRunner != nullptr);
  uint32_t multiple = myRunner->getPeriodSizeMultiple();
  ASSERT_EQ(4u, multiple);
  ASSERT_STREQ("IasRunnerThread::eIasOk", toString(IasRunnerThread::eIasOk).c_str());
  ASSERT_STREQ("IasRunnerThread::eIasInvalidParam", toString(IasRunnerThread::eIasInvalidParam).c_str());
  ASSERT_STREQ("IasRunnerThread::eIasInitFailed", toString(IasRunnerThread::eIasInitFailed).c_str());
  ASSERT_STREQ("IasRunnerThread::eIasNotInitialized", toString(IasRunnerThread::eIasNotInitialized).c_str());
  ASSERT_STREQ("IasRunnerThread::eIasFailed", toString(IasRunnerThread::eIasFailed).c_str());

  delete myRunner;
}
