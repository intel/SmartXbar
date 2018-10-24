/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasSwitchMatrixTest.cpp
 *
 *  Created 2015
 */

#include <iostream>
#include <iomanip>
#include <boost/pool/object_pool.hpp>

#include "switchmatrix/IasSwitchMatrix.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "switchmatrix/IasSwitchMatrixJob.hpp"
#include "model/IasAudioDevice.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "IasSwitchMatrixTest.hpp"
#include "internal/audio/smartx_test_support/IasRingBufferTestWriter.hpp"
#include "internal/audio/smartx_test_support/IasRingBufferTestReader.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "audio/smartx/IasEventProvider.hpp"

#ifndef NFS_PATH
#define NFS_PATH "/nfs/ka/proj/ias/organisation/teams/audio/TestWavRefFiles/2014-06-02/"
#endif

using namespace std;



namespace IasAudio
{

IasConnectEventHandler::IasConnectEventHandler()
  :IasEventHandler()
  ,mCnt(0)
{
}

IasConnectEventHandler::~IasConnectEventHandler()
{
}

void IasConnectEventHandler::receivedConnectionEvent(IasConnectionEvent *event)
{
  ASSERT_TRUE(event != nullptr);
  ASSERT_TRUE(mLog != nullptr);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "[IasCustomerEventHandler::receivedConnectionEvent] Event received:", toString(event->getEventType()));
  ASSERT_TRUE(event != nullptr);
}

void IasSwitchMatrixTest::SetUp()
{

}

void IasSwitchMatrixTest::TearDown()
{
}

TEST_F(IasSwitchMatrixTest, simple_interface_test)
{
  std::string workerName = "dummyWorker";


  IasSwitchMatrix *switchMatrix = new IasSwitchMatrix();
  ASSERT_TRUE(switchMatrix != NULL);

  EXPECT_EQ(IasSwitchMatrix::eIasOk,switchMatrix->trigger());
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->init(workerName,0,48000));
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->init(workerName,0,0));
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->init(workerName,192,0));
  EXPECT_EQ(IasSwitchMatrix::eIasOk,switchMatrix->init(workerName,192,48000));
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->init(workerName,192,48000));
  EXPECT_EQ(IasSwitchMatrix::eIasOk,switchMatrix->trigger());
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->setCopySize(0));
  delete switchMatrix;

}

TEST_F(IasSwitchMatrixTest, workerThread)
{

  IasAudioRingBufferFactory *rbFactory = IasAudioRingBufferFactory::getInstance();
  ASSERT_TRUE(rbFactory != NULL);

  IasAudioRingBuffer* mySourceRingBuffer = nullptr;
  IasAudioCommonResult result = rbFactory->createRingBuffer(&mySourceRingBuffer,192, 4, 2, eIasFormatInt16, eIasRingBufferShared,
                                                            "src_stereo0");
  ASSERT_EQ(eIasResultOk, result);
  ASSERT_TRUE(mySourceRingBuffer != nullptr);

  IasSwitchMatrix *switchMatrix = new IasSwitchMatrix();
  ASSERT_TRUE(switchMatrix != NULL);

  IasSwitchMatrix::IasResult workres = switchMatrix->init("WorkerThread_1", 192, 48000);
  EXPECT_EQ(workres, IasSwitchMatrix::eIasOk);

  delete switchMatrix;
  rbFactory->destroyRingBuffer(mySourceRingBuffer);

}

TEST_F(IasSwitchMatrixTest, createSourceSink)
{
  std::string inputWaveDir = NFS_PATH;

  IasAudioRingBufferFactory *rbFactory = IasAudioRingBufferFactory::getInstance();
  ASSERT_TRUE(rbFactory != NULL);

  /********************************
   * Create source ringbuffer
   */
  uint32_t srcRingBufChannels = 4;
  IasAudioRingBuffer* mySourceRingBuffer = nullptr;
  std::string sourceRingBufferName = "AudioPortTest_Source_0_RingBuffer";
  IasAudioCommonResult result = rbFactory->createRingBuffer(&mySourceRingBuffer, 192, 4, srcRingBufChannels, eIasFormatInt16, eIasRingBufferLocalReal,
                                                            sourceRingBufferName.c_str());
  ASSERT_EQ(eIasResultOk, result);
  ASSERT_TRUE(mySourceRingBuffer != nullptr);
  /*******************************/

  /********************************
   * Create source ringbuffer1
   */
  uint32_t srcRingBuf1Channels = 2;
  IasAudioRingBuffer* mySourceRingBuffer1 = nullptr;
  std::string sourceRingBuffer1Name = "AudioPortTest_Source_1_RingBuffer";
  result = rbFactory->createRingBuffer(&mySourceRingBuffer1, 192, 4, srcRingBuf1Channels, eIasFormatInt16, eIasRingBufferLocalReal,
                                                            sourceRingBufferName.c_str());
  ASSERT_EQ(eIasResultOk, result);
  ASSERT_TRUE(mySourceRingBuffer1 != nullptr);
  /*******************************/


 /*********************************
  * Create sink ringbuffer
  */
  uint32_t sinkRingBufChannels = 4;
  IasAudioRingBuffer* mySinkRingBuffer = nullptr;
  std::string sinkRingBufferName = "AudioPortTest_Stereo_Sink_0_RingBuffer";
  result = rbFactory->createRingBuffer(&mySinkRingBuffer, 192, 4, sinkRingBufChannels, eIasFormatInt16, eIasRingBufferLocalReal,
                                       sinkRingBufferName.c_str());
  ASSERT_EQ(eIasResultOk, result);
  ASSERT_TRUE(mySinkRingBuffer != nullptr);
  /*******************************/

  /*******************************
   * Create source port 0
   */
  uint32_t srcPort_0_Channels = 2;
  uint32_t srcPort_0_Idx = 2;
  ASSERT_TRUE((srcPort_0_Channels+srcPort_0_Idx) <= srcRingBufChannels);
  IasAudioPortParamsPtr sourcePortParams = std::make_shared<IasAudioPortParams>();
  sourcePortParams->name = "AudioPortTest_Stereo_Source_0";
  sourcePortParams->numChannels = 2;
  sourcePortParams->id = 0;
  sourcePortParams->direction = eIasPortDirectionOutput;
  sourcePortParams->index = srcPort_0_Idx;
  IasAudioPortPtr source = std::make_shared<IasAudioPort>(sourcePortParams);
  ASSERT_TRUE(source != NULL);
  /******************************/

  IasAudioDeviceParamsPtr srcDeviceParams = std::make_shared<IasAudioDeviceParams>();
  srcDeviceParams->clockType = eIasClockReceived;
  srcDeviceParams->dataFormat = eIasFormatInt16;
  srcDeviceParams->name = "DummyTestSourceDevice";
  srcDeviceParams->numChannels = 4;
  srcDeviceParams->periodSize = 192;
  srcDeviceParams->samplerate = 48000;
  srcDeviceParams->numPeriods = 4;
  IasAudioSourceDevicePtr srcDevice = std::make_shared<IasAudioSourceDevice>(srcDeviceParams);


  /*******************************
   * Create source port 1
   */
  uint32_t srcPort_1_Channels = 2;
  uint32_t srcPort_1_Idx = 0;
  ASSERT_TRUE((srcPort_1_Channels+srcPort_1_Idx) <= srcRingBufChannels);
  IasAudioPortParamsPtr sourcePortParams1 = std::make_shared<IasAudioPortParams>();
  sourcePortParams1->name = "AudioPortTest_Stereo_Source_1";
  sourcePortParams1->numChannels = 2;
  sourcePortParams1->id = 1;
  sourcePortParams1->direction = eIasPortDirectionOutput;
  sourcePortParams1->index = srcPort_1_Idx;
  IasAudioPortPtr source1 = std::make_shared<IasAudioPort>(sourcePortParams1);
  ASSERT_TRUE(source1 != nullptr);

  EXPECT_EQ(IasAudioPort::eIasOk ,source1->setRingBuffer(mySourceRingBuffer) );
  EXPECT_EQ(IasAudioPort::eIasOk ,source1->setOwner(srcDevice));
  /******************************/

  IasAudioRingBuffer* dummyBuffer;
  IasAudioPortOwnerPtr dummyOwner;
  EXPECT_EQ(IasAudioPort::eIasFailed ,source->getOwner(&dummyOwner) );
  EXPECT_EQ(IasAudioPort::eIasFailed ,source->getRingBuffer(&dummyBuffer) );
  EXPECT_EQ(IasAudioPort::eIasOk ,source->setRingBuffer(mySourceRingBuffer) );
  EXPECT_EQ(IasAudioPort::eIasOk ,source->setOwner(srcDevice));


  /*******************************
   * Create source port 2
   */
  uint32_t srcPort_2_Channels = 2;
  uint32_t srcPort_2_Idx = 0;
  ASSERT_TRUE((srcPort_2_Channels+srcPort_2_Idx) <= srcRingBuf1Channels);
  IasAudioPortParamsPtr sourcePortParams2 = std::make_shared<IasAudioPortParams>();
  sourcePortParams2->name = "AudioPortTest_Stereo_Source_2";
  sourcePortParams2->numChannels = 2;
  sourcePortParams2->id = 1;
  sourcePortParams2->direction = eIasPortDirectionOutput;
  sourcePortParams2->index = srcPort_2_Idx;
  IasAudioPortPtr source2 = std::make_shared<IasAudioPort>(sourcePortParams2);
  ASSERT_TRUE(source1 != nullptr);

  IasAudioDeviceParamsPtr srcDeviceParams1 = std::make_shared<IasAudioDeviceParams>();
  srcDeviceParams1->clockType = eIasClockReceived;
  srcDeviceParams1->dataFormat = eIasFormatInt16;
  srcDeviceParams1->name = "DummyTestSourceDevice1";
  srcDeviceParams1->numChannels = 2;
  srcDeviceParams1->periodSize = 192;
  srcDeviceParams1->samplerate = 44100;
  srcDeviceParams1->numPeriods = 4;
  IasAudioSourceDevicePtr srcDevice1 = std::make_shared<IasAudioSourceDevice>(srcDeviceParams1);


  EXPECT_EQ(IasAudioPort::eIasOk ,source2->setRingBuffer(mySourceRingBuffer1) );
  EXPECT_EQ(IasAudioPort::eIasOk ,source2->setOwner(srcDevice1));

  /*******************************
   * Create sink port
   */
  uint32_t sinkPortChannels = 2;
  uint32_t sinkPortIdx = 2;
  ASSERT_TRUE((sinkPortChannels+sinkPortIdx) <= sinkRingBufChannels);
  IasAudioPortParamsPtr sinkPortParams = std::make_shared<IasAudioPortParams>();
  sinkPortParams->name = "AudioPortTest_Stereo_Sink_0";
  sinkPortParams->numChannels = 2;
  sinkPortParams->id = 42;
  sinkPortParams->direction = eIasPortDirectionInput;
  sinkPortParams->index = 2;
  IasAudioPortPtr sink = std::make_shared<IasAudioPort>(sinkPortParams);

  ASSERT_TRUE(sink != NULL);
  /********************************/

  IasAudioDeviceParamsPtr sinkDeviceParams = std::make_shared<IasAudioDeviceParams>();
  sinkDeviceParams->clockType = eIasClockReceived;
  sinkDeviceParams->dataFormat = eIasFormatInt16;
  sinkDeviceParams->name = "DummyTestSinkDevice";
  sinkDeviceParams->numChannels = 4;
  sinkDeviceParams->periodSize = 192;
  sinkDeviceParams->samplerate = 48000;
  sinkDeviceParams->numPeriods = 4;

  IasAudioSinkDevicePtr sinkDevice = std::make_shared<IasAudioSinkDevice>(sinkDeviceParams);


  EXPECT_EQ(IasAudioPort::eIasFailed ,sink->getOwner(&dummyOwner) );
  EXPECT_EQ(IasAudioPort::eIasFailed ,sink->getRingBuffer(&dummyBuffer) );
  EXPECT_EQ(IasAudioPort::eIasOk ,sink->setRingBuffer(mySinkRingBuffer) );
  EXPECT_EQ(IasAudioPort::eIasOk ,sink->setOwner(sinkDevice));


  IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();
  ASSERT_TRUE(switchMatrix != NULL);
  EXPECT_EQ( IasSwitchMatrix::eIasOk,switchMatrix->init("mySwitchMatrix", 192, 48000));

  switchMatrix->setCopySize(192);

  IasConnectEventHandler* myConnectHandler = new IasConnectEventHandler();
  IasEventProvider* eventProv = IasEventProvider::getInstance();

  usleep(5000);
  EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->trigger());
  usleep(5000);
  EXPECT_EQ(IasSwitchMatrix::eIasOk ,switchMatrix->connect(source, sink) );
  usleep(5000);
  EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->trigger());
  usleep(5000);
  IasEventPtr event;
  EXPECT_EQ( IasEventProvider::eIasOk, eventProv->waitForEvent(1));
  EXPECT_EQ( IasEventProvider::eIasOk, eventProv->getNextEvent(&event));
  event->accept(*myConnectHandler);
  EXPECT_EQ(IasSwitchMatrix::eIasFailed ,switchMatrix->connect(source, sink) );

  EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->trigger());

  EXPECT_EQ(IasSwitchMatrix::eIasFailed ,switchMatrix->connect(source1, sink) );

  EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->trigger());

  EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->startProbing(sink,
                                                                "Dummy",
                                                                false,
                                                                1,
                                                                mySinkRingBuffer,
                                                                48000));
  IasRingBufferTestWriter* writer = new IasRingBufferTestWriter(mySourceRingBuffer,
                                                                192,
                                                                srcPort_1_Channels,
                                                                eIasFormatInt16,
                                                                inputWaveDir+"jingle_stereo_16bit.wav");

  ASSERT_TRUE(writer->init() == eIasResultOk);


  IasRingBufferTestReader* reader = new IasRingBufferTestReader(mySinkRingBuffer,
                                                                192,
                                                                sinkPortChannels,
                                                                eIasFormatInt32,
                                                                "testout.wav");
  ASSERT_TRUE(reader->init() == eIasResultOk);

  while(1)
  {
    IasAudioCommonResult res = writer->writeToBuffer(srcPort_0_Idx);
    if (res != eIasResultOk)
    {
      fprintf(stderr,"writer result %s\n", toString(res).c_str());
      break;
    }
    EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->trigger());
    if(reader->readFromBuffer(sinkPortIdx))
    {
      fprintf(stderr,"Error in reader!!\n");
    }
  };


  EXPECT_EQ(IasSwitchMatrix::eIasOk ,switchMatrix->disconnect(source, sink) );
  switchMatrix->trigger();
  EXPECT_EQ( IasEventProvider::eIasOk, eventProv->waitForEvent(1));
  EXPECT_EQ( IasEventProvider::eIasOk, eventProv->getNextEvent(&event));
  event->accept(*myConnectHandler);

  const std::string filePrefix="/tmp/test";
  EXPECT_EQ(IasSwitchMatrix::eIasOk ,switchMatrix->connect(source2, sink) );
  EXPECT_EQ(IasSwitchMatrix::eIasOk, switchMatrix->trigger());
  EXPECT_EQ(IasAudioPort::eIasOk,sink->storeConnection(switchMatrix));
  EXPECT_EQ(IasSwitchMatrix::eIasOk,switchMatrix->startProbing(source2,filePrefix,true,5,mySourceRingBuffer1,48000));
  switchMatrix->stopProbing(source2);
  EXPECT_EQ(IasSwitchMatrix::eIasOk,switchMatrix->startProbing(sink,filePrefix,true,5,mySinkRingBuffer,48000));
  switchMatrix->stopProbing(sink);
  EXPECT_EQ(IasSwitchMatrix::eIasOk ,switchMatrix->disconnect(source2, sink) );
  EXPECT_EQ(IasAudioPort::eIasOk,sink->forgetConnection(switchMatrix));


  switchMatrix = nullptr;;

  rbFactory->destroyRingBuffer(mySourceRingBuffer);
  rbFactory->destroyRingBuffer(mySinkRingBuffer);
  delete writer;
  delete reader;
}

TEST_F(IasSwitchMatrixTest, error_coverage)
{
  IasAudioRingBufferFactory* factory;
  IasAudioRingBuffer* srcBuffer;
  IasAudioRingBuffer* sinkBuffer;
  IasAudioRingBuffer* dummySinkBuffer;
  IasAudioRingBuffer* dummySrcBuffer;

  IasAudioDeviceParams sourceDeviceParams = IasAudioDeviceParams("sourceDevice1",
                                                                 2,
                                                                 48000,
                                                                 eIasFormatFloat32,
                                                                 eIasClockProvided,
                                                                 192,
                                                                 4);

  IasAudioDeviceParams sinkDeviceParams = IasAudioDeviceParams("sinkDevice1",
                                                               2,
                                                               48000,
                                                               eIasFormatFloat32,
                                                               eIasClockProvided,
                                                               192,
                                                               4);

  IasAudioDeviceParamsPtr srcDevice1ParamsPtr;
  IasAudioDeviceParamsPtr srcDevice2ParamsPtr;
  IasAudioSourceDevicePtr srcDevice1Ptr;
  IasAudioSourceDevicePtr srcDevice2Ptr;
  IasAudioDeviceParamsPtr sinkDevice1ParamsPtr;
  IasAudioSinkDevicePtr   sinkDevice1Ptr;
  IasAudioDeviceParamsPtr sinkDevice2ParamsPtr;
  IasAudioSinkDevicePtr   sinkDevice2Ptr;

  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");

  srcDevice1ParamsPtr = std::make_shared<IasAudioDeviceParams>(sourceDeviceParams);
  srcDevice1Ptr       = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  sourceDeviceParams.name = "sourceDevice2";
  sourceDeviceParams.samplerate = 44100;
  srcDevice2ParamsPtr = std::make_shared<IasAudioDeviceParams>(sourceDeviceParams);
  srcDevice2Ptr       = std::make_shared<IasAudioSourceDevice>(srcDevice2ParamsPtr);
  sourceDeviceParams.name = "sourceDevice1";
  sourceDeviceParams.samplerate = 48000;

  sinkDevice1ParamsPtr = std::make_shared<IasAudioDeviceParams>(sinkDeviceParams);
  sinkDevice1Ptr = std::make_shared<IasAudioSinkDevice>(sinkDevice1ParamsPtr);

  sinkDeviceParams.name = "sinkDevice2";
  sinkDeviceParams.samplerate = 44100;
  sinkDevice2ParamsPtr = std::make_shared<IasAudioDeviceParams>(sinkDeviceParams);
  sinkDevice2Ptr = std::make_shared<IasAudioSinkDevice>(sinkDevice2ParamsPtr);
  sinkDeviceParams.name = "sinkDevice1";
  sinkDeviceParams.samplerate = 48000;

  factory = IasAudioRingBufferFactory::getInstance();
  factory->createRingBuffer(&srcBuffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sinkBuffer");
  factory->createRingBuffer(&dummySrcBuffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"dummySrcBuffer");
  factory->createRingBuffer(&dummySinkBuffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"dummySinkBuffer");

  IasAudioPortParamsPtr srcPortParams = std::make_shared<IasAudioPortParams>("srcPort",
                                                                             2,
                                                                             0,
                                                                             eIasPortDirectionOutput,
                                                                             0);

  IasAudioPortParamsPtr sinkPortParams = std::make_shared<IasAudioPortParams>("sinkPort",
                                                                              2,
                                                                              0,
                                                                              eIasPortDirectionInput,
                                                                              0);

  IasAudioPortParamsPtr dummyOutputPortParams = std::make_shared<IasAudioPortParams>("dummyOutPort",
                                                                                     2,
                                                                                     1,
                                                                                     eIasPortDirectionOutput,
                                                                                     0);

  IasAudioPortParamsPtr dummyInputPortParams = std::make_shared<IasAudioPortParams>("dummyInPort",
                                                                                    2,
                                                                                    1,
                                                                                    eIasPortDirectionInput,
                                                                                    0);


  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParams);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParams);
  IasAudioPortPtr dummySinkPort = std::make_shared<IasAudioPort>(dummyInputPortParams);
  IasAudioPortPtr dummySrcPort = std::make_shared<IasAudioPort>(dummyOutputPortParams);

  IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();

  EXPECT_EQ( switchMatrix->connect(srcPort, sinkPort), IasSwitchMatrix::eIasFailed);
  EXPECT_EQ( switchMatrix->disconnect(srcPort, sinkPort), IasSwitchMatrix::eIasFailed);

  EXPECT_EQ( switchMatrix->setCopySize(192), IasSwitchMatrix::eIasFailed);
  EXPECT_EQ( switchMatrix->init("testWorker",192, 48000), IasSwitchMatrix::eIasOk);

  EXPECT_EQ( switchMatrix->connect(nullptr, sinkPort), IasSwitchMatrix::eIasFailed);
  EXPECT_EQ( switchMatrix->connect(srcPort, nullptr), IasSwitchMatrix::eIasFailed);
  EXPECT_EQ( switchMatrix->disconnect(nullptr, sinkPort), IasSwitchMatrix::eIasFailed);
  EXPECT_EQ( switchMatrix->disconnect(srcPort, nullptr), IasSwitchMatrix::eIasFailed);


  EXPECT_EQ( switchMatrix->connect(srcPort, sinkPort), IasSwitchMatrix::eIasFailed);

  EXPECT_EQ( switchMatrix->disconnect(srcPort, sinkPort), IasSwitchMatrix::eIasFailed);

  srcPort->setRingBuffer(srcBuffer);
  srcPort->setOwner(srcDevice1Ptr);

  sinkPort->setRingBuffer(sinkBuffer);
  sinkPort->setOwner(sinkDevice1Ptr);

  dummySinkPort->setRingBuffer(dummySinkBuffer);
  dummySinkPort->setOwner(sinkDevice2Ptr);

  dummySrcPort->setRingBuffer(dummySrcBuffer);
  dummySrcPort->setOwner(srcDevice2Ptr);


  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->startProbing(dummySrcPort,"dummy",false,3,dummySrcBuffer,48000));
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->startProbing(dummySinkPort,"dummy",false,3,dummySinkBuffer,48000));

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink");
  EXPECT_EQ( switchMatrix->connect(srcPort, sinkPort), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();

  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->startProbing(dummySrcPort,"dummy",false,3,dummySrcBuffer,48000));
  EXPECT_EQ(IasSwitchMatrix::eIasFailed,switchMatrix->startProbing(dummySinkPort,"dummy",false,3,dummySinkBuffer,48000));
  switchMatrix->stopProbing(dummySrcPort);
  switchMatrix->stopProbing(dummySinkPort);


  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "disconnecting src with sink");
  EXPECT_EQ( switchMatrix->disconnect(srcPort, sinkPort), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();
  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "disconnecting src with dummy sink");
  EXPECT_EQ( switchMatrix->connect(dummySrcPort, dummySinkPort), IasSwitchMatrix::eIasFailed);
  switchMatrix->trigger();

  switchMatrix = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;
  srcPortParams = nullptr;
  sinkPortParams = nullptr;

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  factory->destroyRingBuffer(dummySinkBuffer);
  factory->destroyRingBuffer(dummySrcBuffer);

  srcDevice1ParamsPtr = nullptr;
  srcDevice2ParamsPtr = nullptr;
  srcDevice1Ptr = nullptr;
  srcDevice2Ptr = nullptr;
  sinkDevice1Ptr = nullptr;
  sinkDevice2Ptr = nullptr;
}


TEST_F(IasSwitchMatrixTest, switchMatrixJob)
{
  uint32_t framesStillToConsume = 0;
  uint32_t framesConsumed = 0;
  IasAudioRingBufferFactory* factory = IasAudioRingBufferFactory::getInstance();
  IasAudioRingBuffer *ringBufSource,*ringBufSink;
  std::string ringBufName = "inputBuf";
  factory->createRingBuffer(&ringBufSource,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,ringBufName);
  ringBufName = "outputBuf";
  factory->createRingBuffer(&ringBufSink,64,4,2,eIasFormatInt16,eIasRingBufferLocalReal,ringBufName);

  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionInput;
  portParams.id = 0;
  portParams.index = 0;
  portParams.name = "InputPort";
  portParams.numChannels = 2;

  IasAudioPortParamsPtr inputPortParamsPtr = std::make_shared<IasAudioPortParams>(portParams);
  IasAudioPortPtr inputPortPtr = std::make_shared<IasAudioPort>(inputPortParamsPtr);

  IasAudioDeviceParams deviceParams;
  deviceParams.clockType =eIasClockProvided;
  deviceParams.dataFormat = eIasFormatInt16;
  deviceParams.name = "inputDevice";
  deviceParams.numChannels = 2;
  deviceParams.numPeriods = 4;
  deviceParams.periodSize = 64;
  deviceParams.samplerate = 48000;
  IasAudioDeviceParamsPtr inputDeviceParamsPtr = std::make_shared<IasAudioDeviceParams>(deviceParams);

  IasAudioSourceDevicePtr sourceDevicePtr = std::make_shared<IasAudioSourceDevice>(inputDeviceParamsPtr);

  deviceParams.name = "inputDevice";
  IasAudioDeviceParamsPtr outputDeviceParamsPtr = std::make_shared<IasAudioDeviceParams>(deviceParams);
  IasAudioSinkDevicePtr sinkDevicePtr = std::make_shared<IasAudioSinkDevice>(outputDeviceParamsPtr);

  portParams.direction = eIasPortDirectionOutput;
  portParams.id = 0;
  portParams.index = 0;
  portParams.name = "OutputPort";
  portParams.numChannels = 2;

  IasAudioPortParamsPtr outputPortParamsPtr = std::make_shared<IasAudioPortParams>(portParams);
  IasAudioPortPtr outputPortPtr = std::make_shared<IasAudioPort>(outputPortParamsPtr);

  inputPortPtr->setRingBuffer(ringBufSource);
  inputPortPtr->setOwner(sourceDevicePtr);
  outputPortPtr->setRingBuffer(ringBufSink);
  outputPortPtr->setOwner(sinkDevicePtr);
  IasSwitchMatrixJob* myJob = new IasSwitchMatrixJob(inputPortPtr, outputPortPtr);
  myJob->init(64,48000);
  myJob->execute(0, 64, &framesStillToConsume, &framesConsumed);

  delete myJob;
  factory->destroyRingBuffer(ringBufSource);
  factory->destroyRingBuffer(ringBufSink);
}

TEST_F(IasSwitchMatrixTest, removeConnections)
{
  IasAudioRingBufferFactory* factory;
  IasAudioRingBuffer* srcBuffer;
  IasAudioRingBuffer* sinkBuffer;
  IasAudioRingBuffer* sink2Buffer;

  IasAudioDeviceParams sourceDeviceParams = IasAudioDeviceParams("sourceDevice1",
                                                                 2,
                                                                 48000,
                                                                 eIasFormatFloat32,
                                                                 eIasClockProvided,
                                                                 192,
                                                                 4);

  IasAudioDeviceParams sinkDeviceParams = IasAudioDeviceParams("sinkDevice1",
                                                               2,
                                                               48000,
                                                               eIasFormatFloat32,
                                                               eIasClockProvided,
                                                               192,
                                                               4);

  IasAudioDeviceParams sink2DeviceParams = IasAudioDeviceParams("sinkDevice2",
                                                               2,
                                                               48000,
                                                               eIasFormatFloat32,
                                                               eIasClockProvided,
                                                               192,
                                                               4);



  IasAudioDeviceParamsPtr srcDevice1ParamsPtr;
  IasAudioDeviceParamsPtr srcDevice2ParamsPtr;
  IasAudioSourceDevicePtr srcDevice1Ptr;
  IasAudioSourceDevicePtr srcDevice2Ptr;
  IasAudioDeviceParamsPtr sinkDevice1ParamsPtr;
  IasAudioSinkDevicePtr   sinkDevice1Ptr;
  IasAudioDeviceParamsPtr sinkDevice2ParamsPtr;
  IasAudioSinkDevicePtr   sinkDevice2Ptr;

  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");

  srcDevice1ParamsPtr = std::make_shared<IasAudioDeviceParams>(sourceDeviceParams);
  srcDevice1Ptr       = std::make_shared<IasAudioSourceDevice>(srcDevice1ParamsPtr);

  sourceDeviceParams.name = "sourceDevice2";
  sourceDeviceParams.samplerate = 44100;
  srcDevice2ParamsPtr = std::make_shared<IasAudioDeviceParams>(sourceDeviceParams);
  srcDevice2Ptr       = std::make_shared<IasAudioSourceDevice>(srcDevice2ParamsPtr);
  sourceDeviceParams.name = "sourceDevice1";
  sourceDeviceParams.samplerate = 48000;

  sinkDevice1ParamsPtr = std::make_shared<IasAudioDeviceParams>(sinkDeviceParams);
  sinkDevice1Ptr = std::make_shared<IasAudioSinkDevice>(sinkDevice1ParamsPtr);


  sinkDeviceParams.name = "sinkDevice2";
  sinkDeviceParams.samplerate = 44100;
  sinkDevice2ParamsPtr = std::make_shared<IasAudioDeviceParams>(sinkDeviceParams);
  sinkDevice2Ptr = std::make_shared<IasAudioSinkDevice>(sinkDevice2ParamsPtr);
  sinkDeviceParams.name = "sinkDevice1";
  sinkDeviceParams.samplerate = 48000;

  factory = IasAudioRingBufferFactory::getInstance();
  factory->createRingBuffer(&srcBuffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"srcBuffer");
  factory->createRingBuffer(&sinkBuffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sinkBuffer");
  factory->createRingBuffer(&sink2Buffer,192,4,2,eIasFormatInt16,eIasRingBufferLocalReal,"sink2Buffer");

  IasAudioPortParamsPtr srcPortParams = std::make_shared<IasAudioPortParams>("srcPort",
                                                                             2,
                                                                             0,
                                                                             eIasPortDirectionOutput,
                                                                             0);

  IasAudioPortParamsPtr sinkPortParams = std::make_shared<IasAudioPortParams>("sinkPort",
                                                                              2,
                                                                              0,
                                                                              eIasPortDirectionInput,
                                                                              0);

  IasAudioPortParamsPtr sink2PortParams = std::make_shared<IasAudioPortParams>("sink2Port",
                                                                              2,
                                                                              123,
                                                                              eIasPortDirectionInput,
                                                                              0);

  IasAudioPortPtr srcPort = std::make_shared<IasAudioPort>(srcPortParams);
  IasAudioPortPtr sinkPort = std::make_shared<IasAudioPort>(sinkPortParams);
  IasAudioPortPtr sink2Port = std::make_shared<IasAudioPort>(sink2PortParams);

  IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();

  EXPECT_EQ( switchMatrix->init("testWorker",192, 48000), IasSwitchMatrix::eIasOk);

  srcPort->setRingBuffer(srcBuffer);
  srcPort->setOwner(srcDevice1Ptr);

  sinkPort->setRingBuffer(sinkBuffer);
  sinkPort->setOwner(sinkDevice1Ptr);

  sink2Port->setRingBuffer(sink2Buffer);
  sink2Port->setOwner(sinkDevice2Ptr);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "removing connections from src with no connections");
  EXPECT_EQ( switchMatrix->removeConnections(srcPort), IasSwitchMatrix::eIasFailed);
  switchMatrix->trigger();

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink1");
  EXPECT_EQ( switchMatrix->connect(srcPort, sinkPort), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "removing connections from src with 1 connection");
  EXPECT_EQ( switchMatrix->removeConnections(srcPort), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();

  EXPECT_EQ( switchMatrix->disconnect(srcPort, sinkPort), IasSwitchMatrix::eIasFailed);
  switchMatrix->trigger();

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink1");
  EXPECT_EQ( switchMatrix->connect(srcPort, sinkPort), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink2");
  EXPECT_EQ( switchMatrix->connect(srcPort, sink2Port), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "removing all connections from src");
  EXPECT_EQ( switchMatrix->removeConnections(srcPort), IasSwitchMatrix::eIasOk);
  switchMatrix->trigger();

  EXPECT_EQ( switchMatrix->disconnect(srcPort, sinkPort), IasSwitchMatrix::eIasFailed);
  switchMatrix->trigger();
  EXPECT_EQ( switchMatrix->disconnect(srcPort, sink2Port), IasSwitchMatrix::eIasFailed);
  switchMatrix->trigger();

  switchMatrix = nullptr;
  srcPort = nullptr;
  sinkPort = nullptr;
  srcPortParams = nullptr;
  sinkPortParams = nullptr;

  factory->destroyRingBuffer(srcBuffer);
  factory->destroyRingBuffer(sinkBuffer);
  factory->destroyRingBuffer(sink2Buffer);

  srcDevice1ParamsPtr = nullptr;
  srcDevice2ParamsPtr = nullptr;
  srcDevice1Ptr = nullptr;
  srcDevice2Ptr = nullptr;
  sinkDevice1Ptr = nullptr;
  sinkDevice2Ptr = nullptr;
}

}
