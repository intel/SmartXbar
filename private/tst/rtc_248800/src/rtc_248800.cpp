/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pthread.h>
#include <random>
#include <gtest/gtest.h>
#include <audio/smartx/IasSmartX.hpp>
#include <audio/smartx/IasSetupHelper.hpp>
#include <audio/smartx/IasIRouting.hpp>
#include <avbaudiomodules/internal/audio/common/IasAudioLogging.hpp>
#include <avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp>
#include <smartx/IasAudioTypedefs.hpp>
#include <switchmatrix/IasSwitchMatrix.hpp>
#include <model/IasAudioPort.hpp>
#include <model/IasAudioSourceDevice.hpp>
#include <model/IasAudioSinkDevice.hpp>
#include <avbaudiomodules/audio/common/IasAudioCommonTypes.hpp>

using namespace IasAudio;
using namespace std;

void trigger(IasSwitchMatrixPtr swm, bool &threadRunning)
{
  while(threadRunning)
  {
    swm->trigger();
    usleep(1);
  }
}

TEST(rtc_248800, removeConnections)
{
  DltContext *logCtx = IasAudioLogging::getDltContext("TEST");

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

  bool threadRunning = true;
  thread triggerThread(trigger, switchMatrix, ref(threadRunning));
  sched_param schedParams;
  schedParams.sched_priority = 25;
  pthread_setschedparam(triggerThread.native_handle(), SCHED_FIFO, &schedParams);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "removing connections from src with no connections");
  switchMatrix->removeConnections(srcPort);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink1");
  switchMatrix->connect(srcPort, sinkPort);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "removing connections from src with 1 connection");
  switchMatrix->removeConnections(srcPort);

  switchMatrix->disconnect(srcPort, sinkPort);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink1");
  switchMatrix->connect(srcPort, sinkPort);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "connecting src with sink2");
  switchMatrix->connect(srcPort, sink2Port);

  DLT_LOG_CXX(*logCtx, DLT_LOG_VERBOSE, "removing all connections from src");
  switchMatrix->removeConnections(srcPort);

  switchMatrix->disconnect(srcPort, sinkPort);
  switchMatrix->disconnect(srcPort, sink2Port);

  threadRunning = false;
  if (triggerThread.joinable())
  {
    triggerThread.join();
  }

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


int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(false);

  IasAudioLogging::addDltContextItem("TEST", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);

  IasAudioLogging::registerDltContext("TEST", "Test DLT Log Context");

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}