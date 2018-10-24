/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasAlsaHandlerTest.cpp
 *
 *  Created 2015
 */

#include <stdio.h>
#include <stdlib.h>

#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "IasAlsaHandlerTest.hpp"

// Name of the ALSA device, defined and initialized in IasAlsaHandlerTestMain
extern std::string deviceName;

// Other global variables, defined and initialized in main.cpp
extern uint32_t numPeriodsToProcess;

namespace IasAudio
{


void IasAlsaHandlerTest::SetUp()
{
}

void IasAlsaHandlerTest::TearDown()
{
}


TEST_F(IasAlsaHandlerTest, convertResultToString)
{
  std::cout << "Used result types are:" << std::endl;
  IasAlsaHandler::IasResult result = IasAlsaHandler::eIasOk;
  std::cout << toString(result) << std::endl;
  result = IasAlsaHandler::eIasInvalidParam;
  std::cout << toString(result) << std::endl;
  result = IasAlsaHandler::eIasInitFailed;
  std::cout << toString(result) << std::endl;
  result = IasAlsaHandler::eIasNotInitialized;
  std::cout << toString(result) << std::endl;
  result = IasAlsaHandler::eIasAlsaError;
  std::cout << toString(result) << std::endl;
  result = IasAlsaHandler::eIasTimeOut;
  std::cout << toString(result) << std::endl;
  result = IasAlsaHandler::eIasRingBufferError;
  std::cout << toString(result) << std::endl;
}


TEST_F(IasAlsaHandlerTest, convertDeviceTpyeToString)
{
  std::cout << "Possible IasDeviceType enum values:"  << std::endl;
  std::cout << "  " << toString(eIasDeviceTypeUndef)  << std::endl;
  std::cout << "  " << toString(eIasDeviceTypeSource) << std::endl;
  std::cout << "  " << toString(eIasDeviceTypeSink)   << std::endl;
}


TEST_F(IasAlsaHandlerTest, setupTest)
{
  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        = */"invalid",   // will be overwritten later
    /*.numChannels =*/ 2,
    /*.samplerate  =*/ 48000,
    /*.dataFormat  =*/ eIasFormatInt16,
    /*.clockType   =*/ eIasClockReceived,
    /*.periodSize  =*/ 2205,
    /*.numPeriods  =*/ 4
  };

  IasAlsaHandler::IasResult result  = IasAlsaHandler::eIasOk;
  IasAudioRingBufferResult rbResult = IasAudioRingBufferResult::eIasRingBuffOk;

  uint32_t offset = 0;
  uint32_t frames = 0;
  uint32_t numChannels = 0;
  IasAudio::IasAudioArea* audioAreas = nullptr;

  IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
  *devParams = cSinkDeviceParams;
  devParams->name = deviceName; // adopt the actual device name (provided by IasAlsaHandlerTestMain)

  IasAlsaHandler* myAlsaHandler = nullptr;
  myAlsaHandler = new IasAlsaHandler(devParams);

  // Try to call getRingBuffer, without initializing the AlsaHandler before.
  IasAudioRingBuffer* ringBuffer;
  result = myAlsaHandler->getRingBuffer(&ringBuffer);
  ASSERT_EQ(IasAlsaHandler::eIasNotInitialized, result);
  delete myAlsaHandler;

  // Try to initialize the AlsaHandler with unknown format.
  devParams->samplerate = 48000;
  devParams->dataFormat = eIasFormatUndef;
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasInitFailed, result);
  myAlsaHandler->reset();
  delete myAlsaHandler;

  // Try to initialize and start the AlsaHandler with invalid sample rate.
  devParams->samplerate = 0;
  devParams->dataFormat = eIasFormatInt16;
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->start();
  ASSERT_EQ(IasAlsaHandler::eIasNotInitialized, result);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);
  result = myAlsaHandler->start();
  ASSERT_EQ(IasAlsaHandler::eIasInvalidParam, result);
  delete myAlsaHandler;

  // Try to initialize and start the AlsaHandler with invalid sample rate.
  devParams->samplerate = 0x7fffffff;
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);
  result = myAlsaHandler->start();
  ASSERT_EQ(IasAlsaHandler::eIasInvalidParam, result);
  delete myAlsaHandler;

  // Try to initialize and start the AlsaHandler with invalid device type.
  devParams->samplerate = 48000;
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeUndef);
  ASSERT_EQ(IasAlsaHandler::eIasInitFailed, result);
  delete myAlsaHandler;

  // Try to initialize and start the AlsaHandler successfully.
  devParams->dataFormat = eIasFormatInt16,
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);
  result = myAlsaHandler->start();
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  // Try to call getRingBuffer using an invalid parameter. This test must fail.
  result = myAlsaHandler->getRingBuffer(nullptr);
  ASSERT_EQ(IasAlsaHandler::eIasInvalidParam, result);

  // Try to call getRingBuffer. This test must be successful.
  result = myAlsaHandler->getRingBuffer(&ringBuffer);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  // Try to call getPeriodSize using an invalid parameter. This test must fail.
  result = myAlsaHandler->getPeriodSize(nullptr);
  ASSERT_EQ(IasAlsaHandler::eIasInvalidParam, result);

  // Try to call getPeriodSize. This test must be successful.
  uint32_t periodSize = 0;
  result = myAlsaHandler->getPeriodSize(&periodSize);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);
  std::cout << "periodSize = " << periodSize << std::endl;

  // Try to call getDataFormat using an invalid parameter. This test must fail.
  rbResult = ringBuffer->getDataFormat(nullptr);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffInvalidParam, rbResult);

  // Try to call getDataFormat. This test must be successful.
  IasAudioCommonDataFormat actualDataFormat;
  rbResult = ringBuffer->getDataFormat(&actualDataFormat);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
  std::cout << "Ring Buffer Data Format = " << toString(actualDataFormat) << ", Size = " << toSize(actualDataFormat) << " bytes" << std::endl;


  // Try to call beginAccess using an invalid parameter. This test must fail.
  rbResult = ringBuffer->beginAccess(eIasRingBufferAccessWrite, nullptr, &offset, &frames);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffInvalidParam, rbResult);

  // Try to call beginAccess using an invalid parameter. This test must fail.
  rbResult = ringBuffer->beginAccess(eIasRingBufferAccessWrite, &audioAreas, nullptr, &frames);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffInvalidParam, rbResult);

  // Try to call beginAccess using an invalid parameter. This test must fail.
  rbResult = ringBuffer->beginAccess(eIasRingBufferAccessWrite, &audioAreas, &offset, nullptr);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffInvalidParam, rbResult);

  // Try to call beginAccess. This test must be successful.
  rbResult = ringBuffer->beginAccess(eIasRingBufferAccessWrite, &audioAreas, &offset, &frames);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

  // Try to call getAreas using an invalid parameter. This test must fail.
  rbResult = ringBuffer->getAreas(nullptr);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffInvalidParam, rbResult);

  // Try to call getAreas. This test must be successful.
  rbResult = ringBuffer->getAreas(&audioAreas);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffNotAllowed, rbResult);

  numChannels = ringBuffer->getNumChannels();
  ASSERT_TRUE(numChannels != 0);

  // Try to call updateAvailable using an invalid parameter. This test must fail.
  rbResult = ringBuffer->updateAvailable(IasRingBufferAccess::eIasRingBufferAccessWrite, nullptr);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffInvalidParam, rbResult);
  myAlsaHandler->reset();

  delete myAlsaHandler;
}


TEST_F(IasAlsaHandlerTest, setupTestAsync)
{
  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name                 =*/ "invalid",   // will be overwritten later
    /*.numChannels          =*/ 2,
    /*.samplerate           =*/ 48000,
    /*.dataFormat           =*/ eIasFormatInt16,
    /*.clockType            =*/ eIasClockReceivedAsync,
    /*.periodSize           =*/ 2205,
    /*.numPeriods           =*/ 4,
    /*.numPeriodsAsrcBuffer =*/ 0 // invalid parameter!
  };

  IasAlsaHandler::IasResult result  = IasAlsaHandler::eIasOk;

  IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
  *devParams = cSinkDeviceParams;
  devParams->name = deviceName; // adopt the actual device name (provided by IasAlsaHandlerTestMain)

  IasAlsaHandler* myAlsaHandler = nullptr;

  // Try to initialize the AlsaHandler with invalid parameter numPeriodsAsrcBuffer.
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasInitFailed, result);
  delete myAlsaHandler;

  // Try to initialize and start the AlsaHandler successfully.
  devParams->numPeriodsAsrcBuffer = 4;
  myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);
  result = myAlsaHandler->start();
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  result = myAlsaHandler->setNonBlockMode(true);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  delete myAlsaHandler;
}


TEST_F(IasAlsaHandlerTest, testDefaultConstructors)
{
  // Test the default constructors of the structs that are used internally by the IasAlsaHandlerWorkerThread.
  IasAlsaHandlerWorkerThread::IasAudioBufferParams             bufferParams;
  IasAlsaHandlerWorkerThread::IasAlsaHandlerWorkerThreadParams workerThreadParams;
}


TEST_F(IasAlsaHandlerTest, testToString)
{
  // Test of the toString() functions
  std::cout << "Possible enum values of IasAlsaHandler::IasResult:" << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasOk)              << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasInvalidParam)    << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasInitFailed)      << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasNotInitialized)  << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasAlsaError)       << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasTimeOut)         << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasRingBufferError) << std::endl;
  std::cout << "  " << toString(IasAlsaHandler::eIasFailed)          << std::endl;

  std::cout << "Possible enum values of IasAlsaHandlerWorkerThread::IasResult:" << std::endl;
  std::cout << "  " << toString(IasAlsaHandlerWorkerThread::eIasOk)             << std::endl;
  std::cout << "  " << toString(IasAlsaHandlerWorkerThread::eIasInvalidParam)   << std::endl;
  std::cout << "  " << toString(IasAlsaHandlerWorkerThread::eIasInitFailed)     << std::endl;
  std::cout << "  " << toString(IasAlsaHandlerWorkerThread::eIasNotInitialized) << std::endl;
  std::cout << "  " << toString(IasAlsaHandlerWorkerThread::eIasFailed)         << std::endl;
}


// Test the asynchronous ALSA handler in both directions, using all possible dataFormats.
// We use dummy ALSA devices for this test. Also the transmitted PCM frames are not recorded.
TEST_F(IasAlsaHandlerTest, testAsyncDummyStreaming)
{
  const uint32_t cNumUseCases = 6;

  const IasAudioCommonDataFormat cDataFormat[cNumUseCases] =
  {
    IasAudio::eIasFormatInt16,
    IasAudio::eIasFormatInt32,
    IasAudio::eIasFormatFloat32,
    IasAudio::eIasFormatInt16,
    IasAudio::eIasFormatInt32,
    IasAudio::eIasFormatFloat32
  };
  const IasDeviceType cDeviceType[cNumUseCases] =
  {
    IasAudio::eIasDeviceTypeSource,
    IasAudio::eIasDeviceTypeSource,
    IasAudio::eIasDeviceTypeSource,
    IasAudio::eIasDeviceTypeSink,
    IasAudio::eIasDeviceTypeSink,
    IasAudio::eIasDeviceTypeSink
  };
  const IasRingBufferAccess cRingBufferAcces[cNumUseCases] =
  {
    IasAudio::eIasRingBufferAccessRead,
    IasAudio::eIasRingBufferAccessRead,
    IasAudio::eIasRingBufferAccessRead,
    IasAudio::eIasRingBufferAccessWrite,
    IasAudio::eIasRingBufferAccessWrite,
    IasAudio::eIasRingBufferAccessWrite
  };

  for (uint32_t cntUseCases = 0; cntUseCases < cNumUseCases; cntUseCases++)
  {
    std::cout << "Testing asynchronous ALSA handler for "
              << toString(cDeviceType[cntUseCases]) << " using "
              << toString(cDataFormat[cntUseCases]) << std::endl;

    IasRingBufferAccess ringBufferAccessType = cRingBufferAcces[cntUseCases];

    // Parameters for the audio device configuration.
    const IasAudioDeviceParams cSinkDeviceParams =
    {
      /*.name        =*/ deviceName,
      /*.numChannels =*/ 2,
      /*.samplerate  =*/ 48000,
      /*.dataFormat  =*/ cDataFormat[cntUseCases],
      /*.clockType   =*/ IasAudio::eIasClockReceivedAsync,
      /*.periodSize  =*/ 192,
      /*.numPeriods  =*/ 4
    };

    IasAlsaHandler::IasResult result = IasAlsaHandler::eIasOk;

    IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
    *devParams = cSinkDeviceParams;

    IasAlsaHandler* myAlsaHandler = new IasAlsaHandler(devParams);
    result = myAlsaHandler->init(cDeviceType[cntUseCases]);
    ASSERT_EQ(IasAlsaHandler::eIasOk, result);

    result = myAlsaHandler->start();
    ASSERT_EQ(IasAlsaHandler::eIasOk, result);

    uint32_t periodSize = 0;
    myAlsaHandler->getPeriodSize(&periodSize);

    IasAudioRingBuffer* myRingBuffer = nullptr;
    result = myAlsaHandler->getRingBuffer(&myRingBuffer);
    ASSERT_TRUE(nullptr != myRingBuffer);
    ASSERT_EQ(IasAlsaHandler::eIasOk, result);

    uint32_t cntPeriods = 0;
    while (cntPeriods < 8)
    {
      uint32_t size = periodSize;
      uint32_t numFramesAvail = 0;
      IasAudioRingBufferResult rbResult = myRingBuffer->updateAvailable(ringBufferAccessType, &numFramesAvail);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

      if (numFramesAvail < devParams->periodSize)
      {
        continue;
      }

      usleep(4000); // wait for one period (4 ms)

      while (size > 0)
      {
        uint32_t offset;
        uint32_t frames = size;
        IasAudio::IasAudioArea *audioAreas;

        rbResult = myRingBuffer->beginAccess(ringBufferAccessType, &audioAreas, &offset, &frames);
        ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

        rbResult = myRingBuffer->endAccess(ringBufferAccessType, offset, frames);
        ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
        size -= frames;
      }
      cntPeriods++;
    }

    delete myAlsaHandler;
  }
}


}
