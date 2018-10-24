/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasRoutingZoneTest.cpp
 *
 *  Created 2015
 */

#include <iostream>
#include "IasRoutingZoneTest.hpp"
#include "model/IasRoutingZone.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasRoutingZoneWorkerThread.hpp"
#include "alsahandler/IasAlsaHandler.hpp"

#include "model/IasAudioPort.hpp"
#include "model/IasAudioPortOwner.hpp"

#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "switchmatrix/IasBufferTask.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"     // the header file of the volume plug-in module

#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/smartx_test_support/IasRingBufferTestWriter.hpp"

#include "IasMySimplePipeline.hpp"
#include "IasMyComplexPipeline.hpp"

#define AUDIO_PLUGIN_DIR1  "."
#define AUDIO_PLUGIN_DIR2  "../../.."

#ifndef NFS_PATH
#define NFS_PATH "/nfs/ka/proj/ias/organisation/teams/audio/TestWavRefFiles/"
#endif

using namespace std;

// Names of the ALSA devices, defined and initialized in main.cpp
extern std::string deviceName1;
extern std::string deviceName2;

// Other global variables, defined and initialized in main.cpp
extern uint32_t periodSizeMultiple;
extern uint32_t numPeriodsToProcess;
extern bool   useNfsPath;

// Constants shared between several methods
const uint32_t cSamplerate = 48000;
const uint32_t cPeriodSize = 2400;

namespace IasAudio
{

using IasAudioPortPtrVector = vector<IasAudioPortPtr>;


void IasRoutingZoneTest::SetUp()
{
}

void IasRoutingZoneTest::TearDown()
{
}


static void createSource(IasAudioPortPtr          *sourcePort,
                         IasRingBufferTestWriter **ringBufferTestWriter,
                         IasAudioRingBuffer      **sourceRingBuffer,
                         uint32_t               numChannels,
                         const std::string&        filename)
{
  // Create source ringbuffer
  IasAudioRingBufferFactory *rbFactory = IasAudioRingBufferFactory::getInstance();
  ASSERT_TRUE(rbFactory != NULL);
  IasAudioRingBuffer* mySourceRingBuffer = nullptr;
  std::string sourceRingBufferName = "AudioPortTest_Source_0_RingBuffer";
  IasAudioCommonResult cmResult = rbFactory->createRingBuffer(&mySourceRingBuffer,
                                                              2400, 4, numChannels, eIasFormatInt16,
                                                              eIasRingBufferLocalReal,
                                                              sourceRingBufferName.c_str());
  ASSERT_EQ(eIasResultOk, cmResult);
  ASSERT_TRUE(mySourceRingBuffer != nullptr);

  IasRingBufferTestWriter* writer = new IasRingBufferTestWriter(mySourceRingBuffer,
                                                                2400,
                                                                numChannels,
                                                                eIasFormatInt16,
                                                                filename);

  cmResult = writer->init();
  ASSERT_EQ(eIasResultOk, cmResult);


  // Create source device
  IasAudioDeviceParamsPtr srcDeviceParams = std::make_shared<IasAudioDeviceParams>();
  srcDeviceParams->clockType = eIasClockReceived;
  srcDeviceParams->dataFormat = eIasFormatInt16;
  srcDeviceParams->name = "DummyTestSourceDevice";
  srcDeviceParams->numChannels = numChannels;
  srcDeviceParams->periodSize = 2400;
  srcDeviceParams->samplerate = 48000;
  srcDeviceParams->numPeriods = 4;
  IasAudioSourceDevicePtr srcDevice = std::make_shared<IasAudioSourceDevice>(srcDeviceParams);

  // Create source port
  IasAudioPortParamsPtr sourcePortParams = std::make_shared<IasAudioPortParams>();
  sourcePortParams->name = "AudioPortTest_Stereo_Source_1";
  sourcePortParams->numChannels = numChannels;
  sourcePortParams->id = 1;
  sourcePortParams->direction = eIasPortDirectionInput;
  sourcePortParams->index = 0;
  IasAudioPortPtr mySourcePort = std::make_shared<IasAudioPort>(sourcePortParams);
  ASSERT_TRUE(mySourcePort != nullptr);

  ASSERT_EQ(IasAudioPort::eIasOk, mySourcePort->setRingBuffer(mySourceRingBuffer) );
  ASSERT_EQ(IasAudioPort::eIasOk, mySourcePort->setOwner(srcDevice));

  *sourcePort = mySourcePort;
  *ringBufferTestWriter = writer;
  *sourceRingBuffer = mySourceRingBuffer;
}


static void destroySource(IasAudioPortPtr          sourcePort,
                          IasRingBufferTestWriter *ringBufferTestWriter,
                          IasAudioRingBuffer      *sourceRingBuffer)
{
  delete ringBufferTestWriter;
  IasAudioRingBufferFactory *rbFactory = IasAudioRingBufferFactory::getInstance();
  rbFactory->destroyRingBuffer(sourceRingBuffer);
  sourcePort.reset();
}

TEST_F(IasRoutingZoneTest, routingZoneWorkerStates)
{
  IasRoutingZoneParams routingZoneParams;
  routingZoneParams.name = "MyRoutingZone";
  IasRoutingZoneParamsPtr myRoutingZoneParams = std::make_shared<IasRoutingZoneParams>();
  ASSERT_TRUE(myRoutingZoneParams != nullptr);
  *myRoutingZoneParams = routingZoneParams;

  IasRoutingZoneWorkerThreadPtr rzwt = std::make_shared<IasRoutingZoneWorkerThread>(myRoutingZoneParams);
  ASSERT_TRUE(rzwt != nullptr);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasActivate);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasInactivate);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasPrepare);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == true);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasInactivate);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasPrepare);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == true);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasPrepare);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == true);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasActivate);
  ASSERT_TRUE(rzwt->isActive() == true);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasActivate);
  ASSERT_TRUE(rzwt->isActive() == true);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasPrepare);
  ASSERT_TRUE(rzwt->isActive() == true);
  ASSERT_TRUE(rzwt->isActivePending() == false);
  rzwt->changeState(IasRoutingZoneWorkerThread::eIasInactivate);
  ASSERT_TRUE(rzwt->isActive() == false);
  ASSERT_TRUE(rzwt->isActivePending() == false);
}

TEST_F(IasRoutingZoneTest, routingZoneWorkerGetConversionBufferParams)
{
  IasRoutingZoneParams routingZoneParams;
  routingZoneParams.name = "MyRoutingZone";
  IasRoutingZoneParamsPtr myRoutingZoneParams = std::make_shared<IasRoutingZoneParams>();
  ASSERT_TRUE(myRoutingZoneParams != nullptr);
  *myRoutingZoneParams = routingZoneParams;

  IasRoutingZoneWorkerThreadPtr rzwt = std::make_shared<IasRoutingZoneWorkerThread>(myRoutingZoneParams);
  ASSERT_TRUE(rzwt != nullptr);
  const IasRoutingZoneWorkerThread::IasConversionBufferParamsMap& convBufMap = rzwt->getConversionBufferParamsMap();
  ASSERT_EQ(0u, convBufMap.size());
}

TEST_F(IasRoutingZoneTest, addDerivedZoneFailure)
{
  // Parameters for the routing zone configuration.
  IasRoutingZoneParams routingZoneParams;
  routingZoneParams.name = "plughw:0,0";
  IasRoutingZoneParamsPtr myRoutingZoneParams = std::make_shared<IasRoutingZoneParams>();
  *myRoutingZoneParams = routingZoneParams;

  // Generate the routing zone with the parameters metioned above.
  // IasRoutingZone* baseZone    = new IasRoutingZone(myRoutingZoneParams);
  IasRoutingZonePtr baseZone    = std::make_shared<IasRoutingZone>(myRoutingZoneParams);
  IasRoutingZonePtr derivedZone = std::make_shared<IasRoutingZone>(myRoutingZoneParams);


  // Create an audio sink device, which is used for linking it with a routing zone later.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        = */deviceName1, // defined and initialized in IasAlsaHandlerTestMain
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };
  IasAudioDeviceParamsPtr mySinkDeviceParams = std::make_shared<IasAudioDeviceParams>();
  *mySinkDeviceParams = cSinkDeviceParams;
  IasAlsaHandlerPtr alsaHandler = std::make_shared<IasAlsaHandler>(mySinkDeviceParams);
  ASSERT_TRUE(alsaHandler != nullptr);
  IasAudioSinkDevicePtr sink1 = std::make_shared<IasAudioSinkDevice>(mySinkDeviceParams);
  ASSERT_TRUE(sink1 != nullptr);
  sink1->setConcreteDevice(alsaHandler);

  IasRoutingZone::IasResult result = IasRoutingZone::eIasOk;;

  std::string zoneName = baseZone->getName();
  EXPECT_TRUE(zoneName.compare(routingZoneParams.name) == 0);

  // Try to start the base zone, although it has not been initialized. This must result in an error.
  result = baseZone->start();
  std::cout << "result of IasRoutingZone::start(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasNotInitialized);

  // Try to stop the base zone, although it has not been initialized. This must result in an error.
  result = baseZone->stop();
  std::cout << "result of IasRoutingZone::stop(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasNotInitialized);

  // Try to ask the base zone about its period size. This must be 0, due to the missing linkage to a sink device.
  uint32_t tempPeriodSize = baseZone->getPeriodSize();
  ASSERT_TRUE(tempPeriodSize == 0);

  // Try to ask the base zone about its data format. This must be Undefined, due to the missing linkage to a sink device.
  IasAudioCommonDataFormat tempDataFormat = baseZone->getSampleFormat();
  ASSERT_TRUE(tempDataFormat == IasAudioCommonDataFormat::eIasFormatUndef);

  // Try to ask the base zone about its sample rate. This must be 0, due to the missing linkage to a sink device.
  uint32_t tempSampleRate = baseZone->getSampleRate();
  ASSERT_TRUE(tempSampleRate == 0);

  // Try to set an (invalid) switchmatrix worker thread, although the routing zone has not been initialized.
  // Although this method returns always void, this must not result in an seg fault.
  baseZone->setSwitchMatrix(nullptr);
  baseZone->clearSwitchMatrix();


  // Try to add a derived zone with invalid handle. This must result in an error.
  result = baseZone->addDerivedZone(nullptr);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasInvalidParam);

  // Try to add a derived zone, while the base zone has not been initialized. This must result in an error.
  result = baseZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasNotInitialized);

  // Initialize the base zone.
  result = baseZone->init();
  std::cout << "result of IasRoutingZone::init(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);

  // A re-initialization must not result in an error.
  result = baseZone->init();
  std::cout << "result of IasRoutingZone::init(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);

  // Try to add a derived zone that has not been initialized. This must result in an error.
  result = baseZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to add a derived zone, which has not been linked with a sink device. This must result in an error.
  derivedZone->init();
  result = baseZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to add a derived zone. Base zone has not been linked with a sink device. This must result in an error.
  result = derivedZone->linkAudioSinkDevice(sink1);
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);
  result = baseZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to add a derived zone. The ALSA handler of the base zone has not been initialized. This must result in an error.
  result = baseZone->linkAudioSinkDevice(sink1);
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);
  result = baseZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to add a derived zone. Base zone and derived zone are initialized appropriately. This test must pass.
  alsaHandler->init(eIasDeviceTypeSink);
  result = baseZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);

  // Try to add a derived zone to a derived zone. This must result in an error.
  result = derivedZone->addDerivedZone(derivedZone);
  std::cout << "result of IasRoutingZone::addDerivedZone(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to set an (invalid) switchmatrix worker thread.
  // Although this method returns always void, this must not result in an seg fault.
  baseZone->setSwitchMatrix(nullptr);
  baseZone->clearSwitchMatrix();

  // Try to start the derived zone.
  result = derivedZone->start();
  std::cout << "result of IasRoutingZone::start(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to start the base zone. This must result in an error, because we have not set the switchMatrixWorkerThread.
  result = baseZone->start();
  std::cout << "result of IasRoutingZone::start(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to stop the base zone. This test must pass.
  result = baseZone->stop();
  std::cout << "result of IasRoutingZone::stop(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);

  IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();
  // Try to set an (invalid) switchmatrix worker thread.
  // Although this method returns always void, this must not result in an seg fault.
  baseZone->setSwitchMatrix(switchMatrix);

  // Try to start the base zone. This must result in an error, because we have not added any conversion buffers.
  result = baseZone->start();
  std::cout << "result of IasRoutingZone::start(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasFailed);

  // Try to stop the base zone. This test must pass.
  result = baseZone->stop();
  std::cout << "result of IasRoutingZone::stop(): " << toString(result) << std::endl;
  ASSERT_TRUE(result == IasRoutingZone::eIasOk);
}


TEST_F(IasRoutingZoneTest, RoutingZone_Setup)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        = */deviceName1, // defined and initialized in IasAlsaHandlerTestMain
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDeviceParams, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink1PortParams =
  {
    /*.name = */"mySink1Port",
    /*.numChannels = */2,
    /*.id = */123,
    /*.direction = */eIasPortDirectionInput,
    /*.index = */0
  };
  IasAudioPortPtr sink1Port = nullptr;
  result = setup->createAudioPort(sink1PortParams, &sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create the routing zone
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  IasRoutingZone::IasResult rzResult = IasRoutingZone::eIasOk;
  IasAudioRingBuffer* conversionBuffer = nullptr;

  // Call some low-level functions, which must all fail due to invalid parameters.
  rzResult = routingZone1->createConversionBuffer(nullptr,   IasAudioCommonDataFormat::eIasFormatUndef, nullptr);
  ASSERT_EQ(IasRoutingZone::eIasInvalidParam, rzResult);
  rzResult = routingZone1->createConversionBuffer(sink1Port, IasAudioCommonDataFormat::eIasFormatUndef, nullptr);
  ASSERT_EQ(IasRoutingZone::eIasInvalidParam, rzResult);
  rzResult = routingZone1->createConversionBuffer(sink1Port, IasAudioCommonDataFormat::eIasFormatUndef, &conversionBuffer);
  ASSERT_EQ(IasRoutingZone::eIasNotInitialized, rzResult);
  rzResult = routingZone1->linkAudioSinkDevice(nullptr);
  ASSERT_EQ(IasRoutingZone::eIasInvalidParam, rzResult);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, result);
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionInput;
  portParams.id = 1234;
  portParams.index = 0;
  portParams.name = "zone_in_port";
  portParams.numChannels = 2;

  IasAudioPortPtr routingZone1InputPort = nullptr;
  result = setup->createAudioPort(portParams, &routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Add the audio input port to the routing zone.
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Verify that we cannot create a second conversion buffer for an already existing port.
  rzResult = routingZone1->createConversionBuffer(routingZone1InputPort, IasAudioCommonDataFormat::eIasFormatUndef, &conversionBuffer);
  ASSERT_EQ(IasRoutingZone::eIasFailed, rzResult);

  setup->stopRoutingZone(routingZone1);

  setup->unlink(routingZone1, nullptr);
  setup->unlink(routingZone1, sink1);

  IasSmartX::destroy(smartx);
}


TEST_F(IasRoutingZoneTest, RoutingZone_FailureClockUndefined)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        = */"myAlsaSmartxPlugin", // defined and initialized in IasAlsaHandlerTestMain
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockProvided,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDeviceParams, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink1PortParams =
  {
    /*.name         = */"mySink1Port",
    /*.numChannels  = */2,
    /*.id           = */123,
    /*.direction    = */eIasPortDirectionInput,
    /*.index        = */0
  };
  IasAudioPortPtr sink1Port = nullptr;
  result = setup->createAudioPort(sink1PortParams, &sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create the routing zone
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  // Try to start the routing zone. This must fail, because wa have not linked a sink device to the routing zone.
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionInput;
  portParams.id = 1234;
  portParams.index = 0;
  portParams.name = "zone_in_port";
  portParams.numChannels = 2;

  IasAudioPortPtr routingZone1InputPort = nullptr;
  result = setup->createAudioPort(portParams, &routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Add the audio input port to the routing zone.
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create the link from the zone input port to the sink device input port.
  result = setup->link(routingZone1InputPort, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Try to start the routing zone. This must fail, because base routing zones can
  // only be linked with sink devices, whose clockType is eIasClockReceived.
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Wait for 2 ms. If the undefined clock issue is not detected by the start method,
  // the routing zone would spin without any trigger now.
  usleep(2000);

  setup->stopRoutingZone(routingZone1);
  IasSmartX::destroy(smartx);
}


TEST_F(IasRoutingZoneTest, SwitchMatrix_RoutingZone_AlsaHandler)
{
  //
  // Test of a chain Source Device -> Switch Matrix -> Routing Zone -> Alsa Handler
  //

  // Create the audio source, including sourcePort and ringBuffer
  IasAudioPortPtr sourcePort = nullptr;
  IasRingBufferTestWriter *ringBufferTestWriter = nullptr;
  IasAudioRingBuffer      *sourceRingBuffer = nullptr;

  std::string filename;
  if (useNfsPath)
  {
    filename = std::string(NFS_PATH) + "2014-06-27/Grummelbass_48000Hz_60s.wav";
  }
  else
  {
    filename = "Grummelbass_48000Hz_60s.wav";
  }
  createSource(&sourcePort, &ringBufferTestWriter, &sourceRingBuffer, 2, filename);

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        = */deviceName1, // defined and initialized in IasAlsaHandlerTestMain
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDeviceParams, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink1PortParams =
  {
    /*.name = */"mySink1Port",
    /*.numChannels = */2,
    /*.id = */123,
    /*.direction = */eIasPortDirectionInput,
    /*.index = */0
  };
  IasAudioPortPtr sink1Port = nullptr;
  result = setup->createAudioPort(sink1PortParams, &sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create the routing zone
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionInput;
  portParams.id = 1234;
  portParams.index = 0;
  portParams.name = "zone_in_port";
  portParams.numChannels = 2;

  IasAudioPortPtr routingZone1InputPort = nullptr;
  result = setup->createAudioPort(portParams, &routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Try to create invalid links. This must fail.
  result = setup->link(sink1Port, sink1Port);
  ASSERT_EQ(IasISetup::eIasFailed, result);
  result = setup->link(routingZone1InputPort, routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Create the link from the zone input port to the sink device input port.
  result = setup->link(routingZone1InputPort, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasSwitchMatrixPtr switchMatrix = routingZone1->getSwitchMatrix();

  IasSwitchMatrix::IasResult smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  IasAudioCommonResult cmResult;
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);

  std::cout << "Now starting the routing zone..." << std::endl;
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  for (uint32_t cntPeriods = 0; cntPeriods < numPeriodsToProcess; cntPeriods++)
  {
    cmResult = ringBufferTestWriter->writeToBuffer(0);
    usleep(50000);
  }

  setup->stopRoutingZone(routingZone1);

  setup->unlink(routingZone1, sink1);

  IasSmartX::destroy(smartx);
  destroySource(sourcePort, ringBufferTestWriter, sourceRingBuffer);

  std::cout << std::endl << "########## Test #1 has been completed ##########" << std::endl << std::endl;
  usleep(1000000);
}


TEST_F(IasRoutingZoneTest, Two_Zones)
{
  mLog = IasAudioLogging::registerDltContext("TST", "Routing Zone Tesz");
  //                                                  +-> Routing Zone 1 -> Alsa Handler 1
  //                                                  |
  // Test of a chain Source Device -> Switch Matrix --*
  //                                                  |
  //                                                  +-> Routing Zone 2 -> Alsa Handler 2

  // Create the audio source, including sourcePort and ringBuffer
  IasAudioPortPtr sourcePort = nullptr;
  IasRingBufferTestWriter *ringBufferTestWriter = nullptr;
  IasAudioRingBuffer      *sourceRingBuffer = nullptr;

  std::string filename;
  if (useNfsPath)
  {
    filename = std::string(NFS_PATH) + "2014-06-27/Grummelbass_48000Hz_60s.wav";
  }
  else
  {
    filename = "Grummelbass_48000Hz_60s.wav";
  }
  createSource(&sourcePort, &ringBufferTestWriter, &sourceRingBuffer, 2, filename);

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDevice1Params =
  {
    /*.name        = */deviceName1,
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDevice1Params, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink1PortParams =
  {
    /*.name = */"mySink1Port",
    /*.numChannels = */2,
    /*.id = */123,
    /*.direction = */eIasPortDirectionInput,
    /*.index = */0
  };
  IasAudioPortPtr sink1Port = nullptr;
  result = setup->createAudioPort(sink1PortParams, &sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone #1
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasAudioPortParams port1Params;
  port1Params.direction = eIasPortDirectionInput;
  port1Params.id = 1234;
  port1Params.index = 0;
  port1Params.name = "zone1_in_port";
  port1Params.numChannels = 2;

  IasAudioPortPtr routingZone1InputPort = nullptr;
  result = setup->createAudioPort(port1Params, &routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);


  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDevice2Params =
  {
    /*.name        = */deviceName2,
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400 * periodSizeMultiple,
    /*.numPeriods  = */4
  };

  // Create audio sink2
  IasAudioSinkDevicePtr sink2 = nullptr;

  result = setup->createAudioSinkDevice(cSinkDevice2Params, &sink2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink2 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink2PortParams =
  {
    /*.name = */"mySink2Port",
    /*.numChannels = */2,
    /*.id = */456,
    /*.direction = */eIasPortDirectionInput,
    /*.index = */0
  };
  IasAudioPortPtr sink2Port = nullptr;
  result = setup->createAudioPort(sink2PortParams, &sink2Port);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink2Port != nullptr);
  result = setup->addAudioInputPort(sink2, sink2Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone #2
  IasRoutingZoneParams rzparams2 =
  {
    /*.name = */"routingZone2"
  };
  IasRoutingZonePtr routingZone2 = nullptr;
  result = setup->createRoutingZone(rzparams2, &routingZone2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone2 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone2, sink2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasAudioPortParams port2Params;
  port2Params.direction = eIasPortDirectionInput;
  port2Params.id = 5678;
  port2Params.index = 0;
  port2Params.name = "zone2_in_port";
  port2Params.numChannels = 2;

  IasAudioPortPtr routingZone2InputPort = nullptr;
  result = setup->createAudioPort(port2Params, &routingZone2InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->addAudioInputPort(routingZone2, routingZone2InputPort);
  ASSERT_EQ(IasISetup::eIasOk, result);


  // Create the links within the routing zone (from zone input ports to sink device input ports.
  result = setup->link(routingZone1InputPort, sink1Port);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->link(routingZone2InputPort, sink2Port);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Let routingZone2 be a derived zone, which depends on routingZone1
  result = setup->addDerivedZone(routingZone1, routingZone2);
  ASSERT_EQ(IasISetup::eIasOk, result);


  IasSwitchMatrixPtr switchMatrix = routingZone1->getSwitchMatrix();

  IasSwitchMatrix::IasResult smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  smwtResult = switchMatrix->connect(sourcePort, routingZone2InputPort);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  IasAudioCommonResult cmResult;
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);

  std::cout << "Now starting the routing zone 1..." << std::endl;
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  std::cout << "Now starting the routing zone 2..." << std::endl;
  result = setup->startRoutingZone(routingZone2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  for (uint32_t cntPeriods = 0; cntPeriods < numPeriodsToProcess; cntPeriods++)
  {
    cmResult = ringBufferTestWriter->writeToBuffer(0);
    usleep(50000);
  }

  // Try to unlink sink2, while routingZone2 is still running. This must not result in a crash.
  setup->unlink(routingZone2, sink2);

  setup->deleteDerivedZone(routingZone1, routingZone2);
  setup->unlink(routingZone2, sink2);
  setup->unlink(routingZone1, sink1);

  // We intentionally forget to call setup->stopRoutingZone(routingZone1),
  // since we want to check that setup->destroyRoutingZone(routingZone1)
  // does not crash, although the real-time thread is still running.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "destroy routing zone1");
  setup->destroyRoutingZone(routingZone1);
  routingZone1.reset();

  IasSmartX::destroy(smartx);
  destroySource(sourcePort, ringBufferTestWriter, sourceRingBuffer);

  std::cout << std::endl << "########## Test #2 has been completed ##########" << std::endl << std::endl;
  usleep(1000000);
}


TEST_F(IasRoutingZoneTest, OneZone_TwoPorts)
{
  mLog = IasAudioLogging::registerDltContext("TST", "Routing Zone Test");

  // Test of a chain:                                     Routing Zone
  //                     Switch Matrix    +------------------------------------+
  //                   +---------------+  |                    Alsa Handler    |
  //                   |               |  |                 +----------------+ |
  //                   |      +------->|->| input port 1  ->| input port 1   | |
  //   Source Device ->|------*        |  |                 |                | |
  //                   |      +------->|->| input port 2  ->| input port 2   | |
  //                   |               |  |                 +----------------+ |
  //                   +---------------+  |                                    |
  //                                      +------------------------------------+

  // Create the audio source, including sourcePort and ringBuffer
  IasAudioPortPtr sourcePort = nullptr;
  IasRingBufferTestWriter *ringBufferTestWriter = nullptr;
  IasAudioRingBuffer      *sourceRingBuffer = nullptr;

  std::string filename;
  if (useNfsPath)
  {
    filename = std::string(NFS_PATH) + "2014-06-02/Pachelbel_inspired_mono_48000.wav";
  }
  else
  {
    filename = "Pachelbel_inspired_mono_48000.wav";
  }
  createSource(&sourcePort, &ringBufferTestWriter, &sourceRingBuffer, 1, filename);

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDevice1Params =
  {
    /*.name        = */deviceName1,
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDevice1Params, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port1 and add to sink device
  IasAudioPortParams sink1Port1Params =
  {
    /*.name        = */"mySink1Port1",
    /*.numChannels = */1,
    /*.id          = */1,
    /*.direction   = */eIasPortDirectionInput,
    /*.index       = */0
  };
  IasAudioPortPtr sink1Port1 = nullptr;
  result = setup->createAudioPort(sink1Port1Params, &sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port1 != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create sink port2 and add to sink device
  IasAudioPortParams sink1Port2Params =
  {
    /*.name         = */"mySink1Port2",
    /*.numChannels  = */1,
    /*.id           = */2,
    /*.direction    = */eIasPortDirectionInput,
    /*.index        = */1
  };
  IasAudioPortPtr sink1Port2 = nullptr;
  result = setup->createAudioPort(sink1Port2Params, &sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port2 != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone #1
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone input port1 and add to routing zone.
  IasAudioPortParams port1Params;
  port1Params.direction = eIasPortDirectionInput;
  port1Params.id = 1001;
  port1Params.index = 0;
  port1Params.name = "zone1_in_port1";
  port1Params.numChannels = 1;
  IasAudioPortPtr routingZone1InputPort1 = nullptr;
  result = setup->createAudioPort(port1Params, &routingZone1InputPort1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone input port2 and add to routing zone.
  IasAudioPortParams port2Params;
  port2Params.direction = eIasPortDirectionInput;
  port2Params.id = 1002;
  port2Params.index = 0;
  port2Params.name = "zone1_in_port2";
  port2Params.numChannels = 1;
  IasAudioPortPtr routingZone1InputPort2 = nullptr;
  result = setup->createAudioPort(port2Params, &routingZone1InputPort2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Establish the links within the routing zone (from zone input ports to sink device input ports).
  result = setup->link(routingZone1InputPort1, sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->link(routingZone1InputPort2, sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasSwitchMatrixPtr switchMatrix = routingZone1->getSwitchMatrix();

  IasSwitchMatrix::IasResult smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort1);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort2);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  IasAudioCommonResult cmResult;
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);

  std::cout << "Now starting the routing zone 1..." << std::endl;
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  for (uint32_t cntPeriods = 0; cntPeriods < numPeriodsToProcess; cntPeriods++)
  {
    cmResult = ringBufferTestWriter->writeToBuffer(0);
    usleep(50000);
  }
  
  //call the delete function for the pipeline
  routingZone1->deleteBasePipeline();

  // We intentionally forget to call setup->stopRoutingZone(routingZone1),
  // since we want to check that setup->destroyRoutingZone(routingZone1)
  // does not crash, although the real-time thread is still running.
  // Also, we intentionally forget to call setup->unlink(routingZone1, sink1),
  // since we want to check that setup->destroyRoutingZone(routingZone1)
  // does it for us.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "destroy routing zone1");
  setup->destroyRoutingZone(routingZone1);
  routingZone1.reset();

  IasSmartX::destroy(smartx);
  destroySource(sourcePort, ringBufferTestWriter, sourceRingBuffer);

  std::cout << std::endl << "########## Test #3 has been completed ##########" << std::endl << std::endl;
  usleep(1000000);
}


TEST_F(IasRoutingZoneTest, OneZone_TwoPorts_Pipeline)
{
  mLog = IasAudioLogging::registerDltContext("TST", "Routing Zone Test");

  // Test of a chain:                                     Routing Zone
  //                     Switch Matrix    +--------------------------------------------------+
  //                   +---------------+  |                    Alsa Handler                  |
  //                   |               |  |                +----------+   +----------------+ |
  //                   |      +------->|->| input port 1 ->|          |-->| input port 1   | |
  //   Source Device ->|------*        |  |                | pipeline |   |                | |
  //                   |      +------->|->| input port 2 ->|          |-->| input port 2   | |
  //                   |               |  |                +----------+   +----------------+ |
  //                   +---------------+  |                                                  |
  //                                      +--------------------------------------------------+

  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR1, true);

  // Create the audio source, including sourcePort and ringBuffer
  IasAudioPortPtr sourcePort = nullptr;
  IasRingBufferTestWriter *ringBufferTestWriter = nullptr;
  IasAudioRingBuffer      *sourceRingBuffer = nullptr;

  std::string filename;
  if (useNfsPath)
  {
    filename = std::string(NFS_PATH) + "2014-06-02/Pachelbel_inspired_mono_48000.wav";
  }
  else
  {
    filename = "Pachelbel_inspired_mono_48000.wav";
  }
  createSource(&sourcePort, &ringBufferTestWriter, &sourceRingBuffer, 1, filename);

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);


  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDevice1Params =
  {
    /*.name        = */deviceName1,
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDevice1Params, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port1 and add to sink device
  IasAudioPortParams sink1Port1Params =
  {
    /*.name         = */"mySink1Port1",
    /*.numChannels  = */1,
    /*.id           = */1,
    /*.direction    = */eIasPortDirectionInput,
    /*.index        = */0
  };
  IasAudioPortPtr sink1Port1 = nullptr;
  result = setup->createAudioPort(sink1Port1Params, &sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port1 != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create sink port2 and add to sink device
  IasAudioPortParams sink1Port2Params =
  {
    /*.name         = */"mySink1Port2",
    /*.numChannels  = */1,
    /*.id           = */2,
    /*.direction    = */eIasPortDirectionInput,
    /*.index        = */1
  };
  IasAudioPortPtr sink1Port2 = nullptr;
  result = setup->createAudioPort(sink1Port2Params, &sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port2 != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone #1
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone input port1 and add to routing zone.
  IasAudioPortParams port1Params;
  port1Params.direction = eIasPortDirectionInput;
  port1Params.id = 1001;
  port1Params.index = 0;
  port1Params.name = "zone1_in_port1";
  port1Params.numChannels = 1;
  IasAudioPortPtr routingZone1InputPort1 = nullptr;
  result = setup->createAudioPort(port1Params, &routingZone1InputPort1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone input port2 and add to routing zone.
  IasAudioPortParams port2Params;
  port2Params.direction = eIasPortDirectionInput;
  port2Params.id = 1002;
  port2Params.index = 0;
  port2Params.name = "zone1_in_port2";
  port2Params.numChannels = 1;
  IasAudioPortPtr routingZone1InputPort2 = nullptr;
  result = setup->createAudioPort(port2Params, &routingZone1InputPort2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  using IasMyComplexPipelinePtr = std::shared_ptr<IasMyComplexPipeline>;
  IasMyComplexPipelinePtr myPipeline = std::make_shared<IasMyComplexPipeline>(setup,
                                                                              cSinkDevice1Params.samplerate,
                                                                              cSinkDevice1Params.periodSize,
                                                                              routingZone1InputPort1, routingZone1InputPort2,
                                                                              sink1Port1, sink1Port2);
  // The pipeline has to be added to a routing zone before the pipeline pins can be linked.
  IasPipelinePtr pipeline = myPipeline->getPipeline();
  result = setup->addPipeline(routingZone1, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, result);

  myPipeline->init();

  IasSwitchMatrixPtr switchMatrix = routingZone1->getSwitchMatrix();
  IasSwitchMatrix::IasResult smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort1);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);
  smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort2);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  IasAudioCommonResult cmResult;
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);

  std::cout << "Now starting the routing zone 1..." << std::endl;
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  for (uint32_t cntPeriods = 0; cntPeriods < numPeriodsToProcess; cntPeriods++)
  {
    cmResult = ringBufferTestWriter->writeToBuffer(0);
    usleep(50000);
  }

  setup->stopRoutingZone(routingZone1);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "delete pipeline of routing zone1");
  setup->deletePipeline(routingZone1);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "destroy routing zone1");
  setup->destroyRoutingZone(routingZone1);

  setup->destroyAudioPort(sink1Port1);
  setup->destroyAudioPort(sink1Port2);
  setup->destroyAudioPort(routingZone1InputPort1);
  setup->destroyAudioPort(routingZone1InputPort2);
  routingZone1.reset();

  std::cout << "Now resetting the pipeline." << std::endl;
  myPipeline.reset();
  pipeline.reset();

  IasSmartX::destroy(smartx);
  destroySource(sourcePort, ringBufferTestWriter, sourceRingBuffer);

  std::cout << std::endl << "########## Test #4 has been completed ##########" << std::endl << std::endl;
  usleep(1000000);

}


static void createSinkDevice(IasISetup *setup, const std::string &deviceName, uint32_t numberPorts, IasAudioSinkDevicePtr *sinkDevice, IasAudioPortPtrVector *ports, uint32_t *sinkId)
{
  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        = */deviceName,
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDeviceParams, sinkDevice);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(*sinkDevice != nullptr);

  for (uint32_t count=0; count<numberPorts; ++count)
  {
    std::string portName = "mySink" + std::to_string(*sinkId) + "Port" + std::to_string(count);
    // Create sink port(s) and add to sink device
    IasAudioPortParams sinkPortParams =
    {
      /*.name         = */portName,
      /*.numChannels  = */1,
      /*.id           = */-1,
      /*.direction    = */eIasPortDirectionInput,
      /*.index        = */count
    };
    IasAudioPortPtr sinkPort = nullptr;
    result = setup->createAudioPort(sinkPortParams, &sinkPort);
    ASSERT_EQ(IasISetup::eIasOk, result);
    ASSERT_TRUE(sinkPort != nullptr);
    result = setup->addAudioInputPort(*sinkDevice, sinkPort);
    ASSERT_EQ(IasISetup::eIasOk, result);
    ports->push_back(sinkPort);
  }
  (*sinkId)++;
}

static void createRoutingZone(IasISetup *setup, uint32_t rznId, IasRoutingZonePtr *rzn)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  std::string rznName = "routingZone" + std::to_string(rznId);
  // Create routing zone
  IasRoutingZoneParams rzparams;
  rzparams.name = rznName;
  result = setup->createRoutingZone(rzparams, rzn);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(*rzn != nullptr);

}

static void createRoutingZonePorts(IasISetup *setup, uint32_t numberPorts, IasRoutingZonePtr rzn, IasAudioPortPtrVector *ports, uint32_t *rznId, int32_t *portId)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  for (uint32_t count=0; count<numberPorts; ++count)
  {
    IasAudioPortParams portParams;
    portParams.direction = eIasPortDirectionInput;
    portParams.id = *portId;
    portParams.index = 0;
    portParams.name = "zone" + std::to_string(*rznId) + "_in_port" + std::to_string(*portId);
    portParams.numChannels = 1;
    IasAudioPortPtr routingZoneInputPort = nullptr;
    result = setup->createAudioPort(portParams, &routingZoneInputPort);
    ASSERT_EQ(IasISetup::eIasOk, result);
    result = setup->addAudioInputPort(rzn, routingZoneInputPort);
    ASSERT_EQ(IasISetup::eIasOk, result);
    (*portId)++;
    ports->push_back(routingZoneInputPort);
  }
  (*rznId)++;
}

TEST_F(IasRoutingZoneTest, TwoZones_TwoSinks_TwoPorts_Pipeline)
{
  mLog = IasAudioLogging::registerDltContext("TST", "Routing Zone Test");

  // Test of a chain:                                   Routing Zone 1
  //                     Switch Matrix    +---------------------------------------------------+
  //                   +---------------+  |                                   Alsa Handler    |
  //                   |               |  |                +----------+    +----------------+ |
  //                   |      +------->|->| input port 1 ->|          |--->| input port 1   | |
  //   Source Device ->|------*        |  |                | pipeline |    |                | |
  //                   |      +------->|->| input port 2 ->|          |-+  |                | |
  //                   |               |  |                +----------+ |  +----------------+ |
  //                   +---------------+  |                             |                     |
  //                                      +-----------------------------|---------------------+
  //                                                    Routing Zone 2  |
  //                                      +-----------------------------|---------------------+
  //                                      |                             |     Alsa Handler    |
  //                                      |                             |  +----------------+ |
  //                                      |                             +->| input port 1   | |
  //                                      |                                |                | |
  //                                      |                                |                | |
  //                                      |                                +----------------+ |
  //                                      |                                                   |
  //                                      +---------------------------------------------------+

  // Create the audio source, including sourcePort and ringBuffer
  IasAudioPortPtr sourcePort = nullptr;
  IasRingBufferTestWriter *ringBufferTestWriter = nullptr;
  IasAudioRingBuffer      *sourceRingBuffer = nullptr;

  std::string filename;
  if (useNfsPath)
  {
    filename = std::string(NFS_PATH) + "2014-06-02/Pachelbel_inspired_mono_48000.wav";
  }
  else
  {
    filename = "Pachelbel_inspired_mono_48000.wav";
  }
  createSource(&sourcePort, &ringBufferTestWriter, &sourceRingBuffer, 1, filename);

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != NULL);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != NULL);

  // Ids will be incremented by the static methods being called
  uint32_t sinkId = 1;
  uint32_t rznId = 1;
  int32_t rznPortId = 1001;
  IasAudioPortPtrVector sink1Ports;
  IasAudioPortPtrVector rzn1Ports;

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  createSinkDevice(setup, deviceName1, 1, &sink1, &sink1Ports, &sinkId);
  ASSERT_EQ(2u, sinkId);
  ASSERT_EQ(1u, sink1Ports.size());

  IasRoutingZonePtr routingZone1 = nullptr;
  createRoutingZone(setup, rznId, &routingZone1);

  IasISetup::IasResult result = IasISetup::eIasOk;

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  createRoutingZonePorts(setup, 2, routingZone1, &rzn1Ports, &rznId, &rznPortId);
  ASSERT_EQ(2u, rznId);
  ASSERT_EQ(1003, rznPortId);
  ASSERT_EQ(2u, rzn1Ports.size());

  // Create audio sink2
  IasAudioSinkDevicePtr sink2 = nullptr;
  IasAudioPortPtrVector sink2Ports;
  IasAudioPortPtrVector rzn2Ports;
  createSinkDevice(setup, deviceName2, 2, &sink2, &sink2Ports, &sinkId);

  IasRoutingZonePtr routingZone2 = nullptr;
  createRoutingZone(setup, rznId, &routingZone2);

  result = setup->link(routingZone2, sink2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  createRoutingZonePorts(setup, 2, routingZone2, &rzn2Ports, &rznId, &rznPortId);
  ASSERT_EQ(3u, rznId);
  ASSERT_EQ(1005, rznPortId);
  ASSERT_EQ(2u, rzn2Ports.size());

  result = setup->addDerivedZone(routingZone1, routingZone2);

  using IasMyComplexPipelinePtr = std::shared_ptr<IasMyComplexPipeline>;
  IasMyComplexPipelinePtr myPipeline = std::make_shared<IasMyComplexPipeline>(setup,
                                                                              cSamplerate,
                                                                              cPeriodSize,
                                                                              rzn1Ports[0], rzn1Ports[1],
                                                                              sink1Ports[0], sink2Ports[1]);
  // The pipeline has to be added to a routing zone before the pipeline pins can be linked.
  IasPipelinePtr pipeline = myPipeline->getPipeline();
  result = setup->addPipeline(routingZone1, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, result);

  myPipeline->init();

  IasSwitchMatrixPtr switchMatrix = routingZone1->getSwitchMatrix();
  IasSwitchMatrix::IasResult smwtResult;
  smwtResult = switchMatrix->connect(sourcePort, rzn1Ports[0]);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);
  smwtResult = switchMatrix->connect(sourcePort, rzn1Ports[1]);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  const std::string cProbeName1 = "mySink1Port0";
  const std::string cProbeName2 = "mySink2Port1";
  for (auto &port : setup->getAudioPorts())
  {
    std::cout << "Registered port: " << port->getParameters()->name << std::endl;
  }
  IasAudioCommonResult cmResult;
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);

  std::cout << "Now starting the routing zone 1..." << std::endl;
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  std::cout << "Now starting the routing zone 2..." << std::endl;
  result = setup->startRoutingZone(routingZone2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  smartx->debug()->startRecord(cProbeName1, cProbeName1, 10);
  smartx->debug()->startRecord(cProbeName2, cProbeName2, 10);

  for (uint32_t cntPeriods = 0; cntPeriods < numPeriodsToProcess; cntPeriods++)
  {
    cmResult = ringBufferTestWriter->writeToBuffer(0);
    usleep(50000);
  }
  smartx->debug()->stopProbing(cProbeName1);
  smartx->debug()->stopProbing(cProbeName2);

  myPipeline->unlinkPorts();

  setup->stopRoutingZone(routingZone1);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "delete pipeline of routing zone1");
  setup->deletePipeline(routingZone1);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "destroy routing zone1");
  setup->destroyRoutingZone(routingZone1);

  for (auto &port : sink1Ports)
  {
    setup->destroyAudioPort(port);
  }
  sink1Ports.clear();
  for (auto &port : sink2Ports)
  {
    setup->destroyAudioPort(port);
  }
  sink2Ports.clear();
  for (auto &port : rzn1Ports)
  {
    setup->destroyAudioPort(port);
  }
  rzn1Ports.clear();
  routingZone1.reset();

  std::cout << "Now resetting the pipeline." << std::endl;
  myPipeline.reset();
  pipeline.reset();

  IasSmartX::destroy(smartx);
  destroySource(sourcePort, ringBufferTestWriter, sourceRingBuffer);

  std::cout << std::endl << "########## Test #5 has been completed ##########" << std::endl << std::endl;
  usleep(1000000);

}


TEST_F(IasRoutingZoneTest, OneZone_TwoPorts_Pipeline_DirectConnect)
{
  mLog = IasAudioLogging::registerDltContext("TST", "Routing Zone Test");

  // Test of a chain:                                     Routing Zone
  //                     Switch Matrix    +--------------------------------------------------+
  //                   +---------------+  |                    Alsa Handler                  |
  //                   |               |  |                +----------+   +----------------+ |
  //                   |      +------->|->| input port 1 ->| pipeline |-->| input port 1   | |
  //   Source Device ->|------*        |  |                +----------+   |                | |
  //                   |      +------->|->| input port 2 ---------------->| input port 2   | |
  //                   |               |  |                               +----------------+ |
  //                   +---------------+  |                                                  |
  //                                      +--------------------------------------------------+

  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR2, true);

  // Create the audio source, including sourcePort and ringBuffer
  IasAudioPortPtr sourcePort = nullptr;
  IasRingBufferTestWriter *ringBufferTestWriter = nullptr;
  IasAudioRingBuffer      *sourceRingBuffer = nullptr;

  std::string filename;
  if (useNfsPath)
  {
    filename = std::string(NFS_PATH) + "2014-06-02/Pachelbel_inspired_mono_48000.wav";
  }
  else
  {
    filename = "Pachelbel_inspired_mono_48000.wav";
  }
  createSource(&sourcePort, &ringBufferTestWriter, &sourceRingBuffer, 1, filename);

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasIProcessing *processing = smartx->processing();
  ASSERT_TRUE(processing != nullptr);

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDevice1Params =
  {
    /*.name        = */deviceName1,
    /*.numChannels = */2,
    /*.samplerate  = */48000,
    /*.dataFormat  = */eIasFormatInt16,
    /*.clockType   = */eIasClockReceived,
    /*.periodSize  = */2400,
    /*.numPeriods  = */4
  };

  // Create audio sink
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasISetup::IasResult result = IasISetup::eIasOk;

  result = setup->createAudioSinkDevice(cSinkDevice1Params, &sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1 != nullptr);

  // Create sink port1 and add to sink device
  IasAudioPortParams sink1Port1Params =
  {
    /*.name         = */"mySink1Port1",
    /*.numChannels  = */1,
    /*.id           = */1,
    /*.direction    = */eIasPortDirectionInput,
    /*.index        = */0
  };
  IasAudioPortPtr sink1Port1 = nullptr;
  result = setup->createAudioPort(sink1Port1Params, &sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port1 != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create sink port2 and add to sink device
  IasAudioPortParams sink1Port2Params =
  {
    /*.name         = */"mySink1Port2",
    /*.numChannels  = */1,
    /*.id           = */2,
    /*.direction    = */eIasPortDirectionInput,
    /*.index        = */1
  };
  IasAudioPortPtr sink1Port2 = nullptr;
  result = setup->createAudioPort(sink1Port2Params, &sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(sink1Port2 != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone #1
  IasRoutingZoneParams rzparams1 =
  {
    /*.name = */"routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone input port1 and add to routing zone.
  IasAudioPortParams port1Params;
  port1Params.direction = eIasPortDirectionInput;
  port1Params.id = 1001;
  port1Params.index = 0;
  port1Params.name = "zone1_in_port1";
  port1Params.numChannels = 1;
  IasAudioPortPtr routingZone1InputPort1 = nullptr;
  result = setup->createAudioPort(port1Params, &routingZone1InputPort1);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Create routing zone input port2 and add to routing zone.
  IasAudioPortParams port2Params;
  port2Params.direction = eIasPortDirectionInput;
  port2Params.id = 1002;
  port2Params.index = 0;
  port2Params.name = "zone1_in_port2";
  port2Params.numChannels = 1;
  IasAudioPortPtr routingZone1InputPort2 = nullptr;
  result = setup->createAudioPort(port2Params, &routingZone1InputPort2);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(routingZone1, routingZone1InputPort2);
  ASSERT_EQ(IasISetup::eIasOk, result);


  // Establish a link via the pipeline from routingZone1InputPort1 to sink1Port1.
  using IasMySimplePipelinePtr = std::shared_ptr<IasMySimplePipeline>;
  IasMySimplePipelinePtr myPipeline = std::make_shared<IasMySimplePipeline>(setup,
                                                                            cSinkDevice1Params.samplerate,
                                                                            cSinkDevice1Params.periodSize,
                                                                            routingZone1InputPort1,
                                                                            sink1Port1);
  // The pipeline has to be added to a routing zone before the pipeline pins can be linked.
  IasPipelinePtr pipeline = myPipeline->getPipeline();
  result = setup->addPipeline(routingZone1, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, result);

  myPipeline->init();

  // Establish a direct link from routingZone1InputPort2 to sink1Port2.
  result = setup->link(routingZone1InputPort2, sink1Port2);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasSwitchMatrixPtr switchMatrix = routingZone1->getSwitchMatrix();
  IasSwitchMatrix::IasResult smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort1);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);
  smwtResult = switchMatrix->connect(sourcePort, routingZone1InputPort2);
  ASSERT_EQ(smwtResult, IasSwitchMatrix::eIasOk);

  IasAudioCommonResult cmResult;
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);
  cmResult = ringBufferTestWriter->writeToBuffer(0);
  ASSERT_EQ(eIasResultOk, cmResult);

  std::cout << "Now starting the routing zone 1..." << std::endl;
  result = setup->startRoutingZone(routingZone1);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasEventHandler eventHandler;

  for (uint32_t cntPeriods = 0; cntPeriods < numPeriodsToProcess; cntPeriods++)
  {
    // Send a command every second. Toggle the volume between 0 dB and -20 dB
    if ((cntPeriods % 20) == 0)
    {
      int32_t newVolume = -200 * static_cast<int32_t>((cntPeriods / 20) & 0x01); // expressed in dB/10

      IasProperties cmdProperties;
      cmdProperties.set<int32_t>("cmd", static_cast<int32_t>(IasAudio::IasVolume::eIasSetVolume));
      cmdProperties.set<std::string>("pin", "module pin 0");
      cmdProperties.set<int32_t>("volume", newVolume);
      IasAudio::IasInt32Vector ramp;
      ramp.push_back(200); // 200 ms ramp time
      ramp.push_back(0);   // linear ramp shape
      cmdProperties.set<IasInt32Vector>("ramp", ramp);

      IasProperties returnProps;
      processing->sendCmd("Volume instance 0", cmdProperties, returnProps);
    }

    // We use the default event handler to receive the IasModuleEvent that is generated by the ias.volume module.
    IasEventPtr newEvent;
    IasSmartX::IasResult smxres = smartx->getNextEvent(&newEvent);
    if (smxres == IasSmartX::eIasOk)
    {
      newEvent->accept(eventHandler);
    }

    cmResult = ringBufferTestWriter->writeToBuffer(0);
    usleep(50000);
  }

  setup->unlink(routingZone1InputPort2, sink1Port2);

  // We intentionally forget to call setup->stopRoutingZone(routingZone1),
  // since we want to check that setup->destroyRoutingZone(routingZone1)
  // does not crash, although the real-time thread is still running.
  // Also, we intentionally forget to call setup->unlink(routingZone1, sink1),
  // since we want to check that setup->destroyRoutingZone(routingZone1)
  // does it for us.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "destroy routing zone1");
  setup->destroyRoutingZone(routingZone1);

  setup->destroyAudioPort(sink1Port1);
  setup->destroyAudioPort(sink1Port2);
  setup->destroyAudioPort(routingZone1InputPort1);
  setup->destroyAudioPort(routingZone1InputPort2);
  routingZone1.reset();

  std::cout << "Now resetting the pipeline." << std::endl;
  myPipeline.reset();
  pipeline.reset();

  IasSmartX::destroy(smartx);
  destroySource(sourcePort, ringBufferTestWriter, sourceRingBuffer);

  std::cout << std::endl << "########## Test #6 has been completed ##########" << std::endl << std::endl;
  usleep(1000000);
}


}
