/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasAlsaHandlerTestPlaybackAsync.cpp
 *
 * @brief Test of the asynchronous ALSA handler in playback direction.
 *
 * To verify the behavior of the asynchronous ALSA handler, this unit test applies
 * a timer that controls the transmission rate of the periods of PCM frames that
 * are written into the input buffer of the ALSA handler. The unit test applies
 * the class IasDrift in order to adjust the transmission rate according to the
 * profile defined in the text file drift.txt.
 *
 * @date  2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <time.h>
#include <sys/time.h>


#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/IasAlsaTypeConversion.hpp"
#include "IasAlsaHandlerTest.hpp"
#include "IasDrift.hpp"


// Activate this to simulate a jittering timing for the transmission of the
// PCM frames from the sine generator to the intput buffer of the ALSA handler.
#define SIMULATE_JITTER 0


// Name of the ALSA device, defined and initialized in IasAlsaHandlerTestMain.cpp
extern std::string deviceName;

// Other global variables, defined and initialized in IasAlsaHandlerTestMain.cpp
extern uint32_t numPeriodsToProcess;

// Name of the text file that defines the drift profile.
#ifndef DRIFT_FILE
#define DRIFT_FILE "drift.txt"
#endif
const std::string driftFileName(DRIFT_FILE);

namespace IasAudio
{

static void generate_sine(const IasAudio::IasAudioArea       *audioAreas,
                          uint32_t                         offset,
                          uint32_t                         numSamples,
                          double                       *_phase,
                          IasAudio::IasAudioCommonDataFormat  dataFormat,
                          uint32_t                         rate,
                          uint32_t                         channels)
{
  uint8_t   *samples[channels];
  int32_t    steps[channels];
  double  phaseIncr = 2.0 * M_PI * 440.0 / static_cast<double>(rate);

  IAS_ASSERT(audioAreas != nullptr);
  IAS_ASSERT(_phase     != nullptr);

  /* Set up pointers for addressing the channel buffers. */
  for (uint32_t chn = 0; chn < channels; chn++)
  {
    steps[chn]   = audioAreas[chn].step / 8;
    samples[chn] = ((uint8_t*)audioAreas[chn].start) + (audioAreas[chn].first / 8) + offset * steps[chn];
  }

  /* Fill the channel buffers with sine waves. */
  for (uint32_t count = 0; count < numSamples; count++)
  {
    *_phase = *_phase + phaseIncr;
    for (uint32_t chn = 0; chn < channels; chn++)
    {
      switch (dataFormat)
      {
        case IasAudio::eIasFormatInt16:
        {
          int16_t* currentSample = (int16_t*)samples[chn];
          *currentSample = static_cast<int16_t>(32767.0 * cos((chn+1) * *_phase));
          break;
        }
        case IasAudio::eIasFormatInt32:
        {
          int32_t* currentSample = (int32_t*)samples[chn];
          *currentSample = static_cast<int32_t>(0x7fffffff * cos((chn+1) * *_phase));
          break;
        }
        case IasAudio::eIasFormatFloat32:
        {
          float* currentSample = (float*)samples[chn];
          *currentSample = static_cast<float>(cos((chn+1) * *_phase));
          break;
        }
        default:
        {
          break;
        }
      }
      samples[chn] += steps[chn];
    }
  }
}


TEST_F(IasAlsaHandlerTest, streamPlaybackAsyncStereo)
{
  std::cout << "===========================================================" << std::endl;
  std::cout << "Test of the asynchronous ALSA handler in playback direction" << std::endl;
  std::cout << "===========================================================" << std::endl;

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name                 =*/ "invalid",   // will be overwritten later
    /*.numChannels          =*/ 2,
    /*.samplerate           =*/ 48000,
    /*.dataFormat           =*/ IasAudio::eIasFormatInt16,
    /*.clockType            =*/ IasAudio::eIasClockReceivedAsync,
    /*.periodSize           =*/ 240,
    /*.numPeriods           =*/ 4,
    /*.numPeriodsAsrcBuffer =*/ 4
  };

  IasAlsaHandler::IasResult result = IasAlsaHandler::eIasOk;

  IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
  *devParams = cSinkDeviceParams;
  devParams->name = deviceName; // adopt the actual device name (provided by IasAlsaHandlerTestMain)

  IasAlsaHandler* myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSink);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  result = myAlsaHandler->start();
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  uint32_t periodSize = 0;
  myAlsaHandler->getPeriodSize(&periodSize);
  std::cout << "periodSize = " << periodSize << std::endl;

  IasAudioRingBuffer* myRingBuffer = nullptr;
  result = myAlsaHandler->getRingBuffer(&myRingBuffer);
  ASSERT_TRUE(nullptr != myRingBuffer);
  ASSERT_EQ(IasAlsaHandler::eIasOk, result);

  IasAudioCommonDataFormat actualDataFormat;
  IasAudioRingBufferResult rbResult = myRingBuffer->getDataFormat(&actualDataFormat);
  ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
  std::cout << "Ring Buffer Data Format = " << toString(actualDataFormat) << ", Size = " << toSize(actualDataFormat) << " bytes" << std::endl;

  IasDrift *myDrift = new IasDrift();
  myDrift->init(driftFileName);

  uint32_t  cntPeriods = 0;
  uint32_t  samples = 0;
  double phase = 0.0;
  (void) phase;

  // time measurements
  timeval time1, time2;

  // boost timer
  boost::asio::io_service     io_service;
  boost::asio::deadline_timer timer(io_service);

  // initial wait
  timer.expires_from_now(boost::posix_time::milliseconds(1));
  timer.wait();

  // timer parameters
  uint32_t hiResTimerUpdate = 0;
  uint32_t loResTimer = 0;
  uint32_t hiResTimer = 0;
  uint32_t waitInterval = 0;

  // drift parameters
  int32_t drift = 0;
  uint32_t playtime = 0;
  uint32_t cntSamples = 0;

  uint32_t samplesPerMilliSecond = cSinkDeviceParams.samplerate / 1000;

  // loop until certain number of periods processed
  while (cntPeriods < numPeriodsToProcess)
  {
    uint32_t size = periodSize;

    rbResult = myRingBuffer->updateAvailable(IasRingBufferAccess::eIasRingBufferAccessWrite, &samples);
    ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
    //ASSERT_TRUE(samples >= devParams->periodSize);

    // simulate waiting for the routing zone thread to write a period of samples
    playtime = cntSamples / samplesPerMilliSecond;
    myDrift->updateCurrentDrift(playtime, &drift);
    cntSamples += devParams->periodSize;

    hiResTimerUpdate = (1000000 + drift) * devParams->periodSize / samplesPerMilliSecond;
    hiResTimer += hiResTimerUpdate;

    waitInterval = (hiResTimer - loResTimer) / 1000;

#if SIMULATE_JITTER
    // To simulate some jitter, reduce the wait interval by up to 30 %.
    waitInterval = waitInterval - waitInterval * (rand() % 3000) / 10000;
#endif

    loResTimer += 1000 * waitInterval;
    if (loResTimer > 2000000000)
    {
      loResTimer -= 2000000000;
      hiResTimer -= 2000000000;
    }

    timer.expires_at(timer.expires_at() + boost::posix_time::microseconds(waitInterval));
    timer.wait();
    time1 = time2;
    gettimeofday(&time2, NULL);

    std::cout << "Drift: " << drift << " [ppm], "
              << "Wait intervall: " << waitInterval << " [us], "
              << "Actually waited: " << time2.tv_usec - time1.tv_usec << "[us]" << std::endl;

    while (size > 0)
    {
      uint32_t offset;
      uint32_t frames = size;
      IasAudio::IasAudioArea *audioAreas;

      uint32_t numChannels = myRingBuffer->getNumChannels();
      ASSERT_TRUE(numChannels != 0);

      rbResult = myRingBuffer->beginAccess(eIasRingBufferAccessWrite, &audioAreas, &offset, &frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

      // Verify that ALSA handler in asynchronous mode always provides enough free samples in its input buffer.
      ASSERT_TRUE(frames == size);

      // Generate the sine wave.
      generate_sine(audioAreas, offset, static_cast<int>(frames), &phase, actualDataFormat, cSinkDeviceParams.samplerate, numChannels);

      rbResult = myRingBuffer->endAccess(IasRingBufferAccess::eIasRingBufferAccessWrite, offset, frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
      size -= frames;
    }
    cntPeriods++;
  }

  delete myAlsaHandler;
  delete myDrift;
}


}
