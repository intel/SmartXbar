/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasBufferTaskTest.cpp
 *
 *  Created 2016
 */

#include <iostream>
#include "IasBufferTaskTest.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
#include "switchmatrix/IasBufferTask.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "switchmatrix/IasSwitchMatrixJob.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"

namespace IasAudio
{

IasAudioPortParams srcPortParams = IasAudioPortParams("srcPort",
                                                      2,
                                                      0,
                                                      eIasPortDirectionOutput,
                                                      0);

IasAudioPortParams sinkPortParams = IasAudioPortParams("sinkPort",
                                                       2,
                                                       0,
                                                       eIasPortDirectionInput,
                                                       0);

IasAudioPortParams sinkPort1Params = IasAudioPortParams("sinkPort1",
                                                       2,
                                                       123,
                                                       eIasPortDirectionInput,
                                                       0);

IasAudioPortParams dummyPortParams = IasAudioPortParams("dummyPortPort",
                                                       1,
                                                       1,
                                                       eIasPortDirectionOutput,
                                                       0);

IasAudioDeviceParams deviceParams = IasAudioDeviceParams("sourceDevice",
                                                         2,
                                                         48000,
                                                         eIasFormatFloat32,
                                                         eIasClockProvided,
                                                         64,
                                                         4);

IasAudioRingBufferFactory* factory = IasAudioRingBufferFactory::getInstance();


void IasBufferTaskTest::SetUp()
{

}

void IasBufferTaskTest::TearDown()
{

}

TEST_F(IasBufferTaskTest, genral_test)
{
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortParamsPtr dummyPortParamsPtr = std::make_shared<IasAudioPortParams>(dummyPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasAudioPortPtr dummyPort = std::make_shared<IasAudioPort>(dummyPortParamsPtr);

  IasAudioDeviceParamsPtr deviceParamsPtr = std::make_shared<IasAudioDeviceParams>(deviceParams);
  IasAudioSourceDevicePtr srcDevicePtr = std::make_shared<IasAudioSourceDevice>(deviceParamsPtr);
  deviceParamsPtr->name = "sinkDevice";
  IasAudioSinkDevicePtr sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(deviceParamsPtr);

  srcPort->setOwner(srcDevicePtr);
  sinkPort->setOwner(sinkDevicePtr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer");
  srcPort->setRingBuffer(srcBuffer);

  sinkPort->setRingBuffer(sinkBuffer);


  IasBufferTask* myBufferTask = new IasBufferTask(srcPort,copySize,copySize,48000,false);

  ASSERT_TRUE(myBufferTask != nullptr);
  EXPECT_EQ(IasBufferTask::eIasFailed,myBufferTask->addJob(nullptr,sinkPort));
  EXPECT_EQ(IasBufferTask::eIasFailed,myBufferTask->addJob(srcPort,nullptr));
  EXPECT_EQ(IasBufferTask::eIasFailed,myBufferTask->addJob(dummyPort,sinkPort));

  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->addJob(srcPort,sinkPort));
  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->doJobs());

  ASSERT_TRUE(myBufferTask->findJob(srcPort) != nullptr);

  ASSERT_TRUE(myBufferTask->findJob(sinkPort) != nullptr);

  ASSERT_TRUE(myBufferTask->findJob(dummyPort) == nullptr);


  EXPECT_EQ(IasBufferTask::eIasOk, myBufferTask->startProbing("dummy",2,false,2,0,48000,eIasFormatFloat32));
  EXPECT_EQ(IasBufferTask::eIasOk, myBufferTask->doJobs());
  EXPECT_EQ(IasBufferTask::eIasFailed, myBufferTask->startProbing("dummy",2,false,2,0,48000,eIasFormatFloat32));
  EXPECT_EQ(IasBufferTask::eIasOk, myBufferTask->doJobs());
  myBufferTask->stopProbing();
  myBufferTask->stopProbing();
  EXPECT_EQ(IasBufferTask::eIasOk, myBufferTask->doJobs());
  EXPECT_EQ(IasBufferTask::eIasOk, myBufferTask->startProbing("dummy",2,false,2,0,48000,eIasFormatFloat32));

  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->triggerDeleteJob(srcPort,sinkPort));
  EXPECT_EQ(IasBufferTask::eIasFailed,myBufferTask->triggerDeleteJob(srcPort,dummyPort));

  // Test the functions makeDummy(), makeReal(), and isDummy()
  myBufferTask->makeDummy();
  ASSERT_TRUE(myBufferTask->isDummy());
  myBufferTask->makeReal();
  ASSERT_FALSE(myBufferTask->isDummy());

  srcPort = nullptr;
  sinkPort = nullptr;
  dummyPort = nullptr;
  srcDevicePtr = nullptr;
  sinkDevicePtr = nullptr;
  srcPortParamsPtr = nullptr;
  sinkPortParamsPtr = nullptr;

  delete myBufferTask;


}

TEST_F(IasBufferTaskTest, dummyTest)
{
  uint32_t copySize = 64;

  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);

  IasAudioRingBuffer *srcBuffer;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");

  srcPort->setRingBuffer(srcBuffer);
  IasBufferTask* myBufferTask = new IasBufferTask(srcPort,copySize,copySize,48000,true);

  myBufferTask->doDummy();

  factory->destroyRingBuffer(srcBuffer);

  delete myBufferTask;

}

/**
* Test IasBufferTask::deleteAllJobs
* Test eIasSourceDeleted events
*/
TEST_F(IasBufferTaskTest, deleteAllJobs_events)
{
  uint32_t copySize = 64;
  IasAudioPortParamsPtr srcPortParamsPtr = std::make_shared<IasAudioPortParams>(srcPortParams);
  IasAudioPortParamsPtr sinkPortParamsPtr = std::make_shared<IasAudioPortParams>(sinkPortParams);
  IasAudioPortParamsPtr sinkPort1ParamsPtr = std::make_shared<IasAudioPortParams>(sinkPort1Params);
  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParamsPtr);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParamsPtr);
  IasAudioPortPtr sinkPort1 = std::make_shared<IasAudioPort>(sinkPort1ParamsPtr);

  IasAudioDeviceParamsPtr deviceParamsPtr = std::make_shared<IasAudioDeviceParams>(deviceParams);
  IasAudioSourceDevicePtr srcDevicePtr = std::make_shared<IasAudioSourceDevice>(deviceParamsPtr);
  deviceParamsPtr->name = "sinkDevice0";
  IasAudioSinkDevicePtr sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(deviceParamsPtr);

  deviceParamsPtr->name = "sinkDevice1";
  deviceParamsPtr->samplerate = 44100;
  IasAudioSinkDevicePtr sinkDevice1Ptr = std::make_shared<IasAudioSinkDevice>(deviceParamsPtr);

  srcPort->setOwner(srcDevicePtr);
  sinkPort->setOwner(sinkDevicePtr);
  sinkPort1->setOwner(sinkDevice1Ptr);

  IasAudioRingBuffer *srcBuffer,*sinkBuffer, *sinkBuffer1;
  factory->createRingBuffer(&srcBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer0");
  factory->createRingBuffer(&sinkBuffer1,64,4,2,eIasFormatFloat32,eIasRingBufferLocalReal,"sinkBuffer1");
  srcPort->setRingBuffer(srcBuffer);
  sinkPort->setRingBuffer(sinkBuffer);
  sinkPort1->setRingBuffer(sinkBuffer1);

  IasBufferTask* myBufferTask = new IasBufferTask(srcPort,copySize,copySize,48000,false);
  ASSERT_TRUE(myBufferTask != nullptr);

  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->addJob(srcPort,sinkPort));
  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->doJobs());

  ASSERT_TRUE(myBufferTask->findJob(srcPort) != nullptr);
  ASSERT_TRUE(myBufferTask->findJob(sinkPort) != nullptr);

  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->addJob(srcPort,sinkPort1));
  EXPECT_EQ(IasBufferTask::eIasOk,myBufferTask->doJobs());

  ASSERT_TRUE(myBufferTask->findJob(srcPort) != nullptr);
  ASSERT_TRUE(myBufferTask->findJob(sinkPort) != nullptr);
  ASSERT_TRUE(myBufferTask->findJob(sinkPort1) != nullptr);

  // Test eIasSourceDeleted Event
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  IasEventPtr receivedEvent;
  IasSmartX::IasResult eventres;
  //Clear all previous events from queue
  IasEventProvider::getInstance()->clearEventQueue();
  EXPECT_EQ(IasSmartX::eIasNoEventAvailable, smartx->getNextEvent(&receivedEvent));

  // Delete All Jobs
  EXPECT_EQ(IasBufferTask::eIasOk, myBufferTask->deleteAllJobs(srcPort));
  EXPECT_EQ(IasBufferTask::eIasNoJobs,myBufferTask->doJobs());
  ASSERT_TRUE(myBufferTask->findJob(srcPort) == nullptr);
  ASSERT_TRUE(myBufferTask->findJob(sinkPort) == nullptr);
  ASSERT_TRUE(myBufferTask->findJob(sinkPort1) == nullptr);

  // Wait for event eIasSourceDeleted
  eventres = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, eventres);

  class : public IasEventHandler
  {
    virtual void receivedConnectionEvent(IasConnectionEvent* event)
    {
      std::cout << "Received connection event Source Id:" << event->getSourceId() << " Sink Id:" << event->getSinkId()  << std::endl;
      EXPECT_EQ(IasConnectionEvent::eIasSourceDeleted, event->getEventType());
      EXPECT_EQ(0, event->getSourceId());
    }
  } deleteSourceEventHandler;

  // Expect two events for two connections
  eventres = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasOk, eventres);
  receivedEvent->accept(deleteSourceEventHandler);

  eventres = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasOk, eventres);
  receivedEvent->accept(deleteSourceEventHandler);

  IasSmartX::destroy(smartx);

  srcPort = nullptr;
  sinkPort = nullptr;
  srcDevicePtr = nullptr;
  sinkDevicePtr = nullptr;
  sinkDevice1Ptr = nullptr;
  srcPortParamsPtr = nullptr;
  sinkPortParamsPtr = nullptr;
  sinkPort1ParamsPtr = nullptr;

  delete myBufferTask;
}
}
