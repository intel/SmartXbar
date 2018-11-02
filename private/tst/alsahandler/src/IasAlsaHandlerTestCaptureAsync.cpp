/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasAlsaHandlerTestCaptureAsync.cpp
 *
 * @brief Test of the asynchronous ALSA handler in capture direction.
 *
 * To verify the behavior of the asynchronous ALSA handler, this unit test applies
 * a timer that controls the transmission rate of the periods of PCM frames that
 * are read from the output buffer of the ALSA handler. The unit test applies
 * the class IasDrift in order to adjust the transmission rate according to the
 * profile defined in the text file drift.txt.
 *
 * @date  2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <time.h>
#include <sys/time.h>

#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "avbaudiomodules/internal/audio/common/IasAlsaTypeConversion.hpp"
#include "IasAlsaHandlerTest.hpp"
#include "IasDrift.hpp"


// Activate this to simulate a jittering timing for the transmission of the
// PCM frames from the output buffer of the ALSA handler to the output file.
#define SIMULATE_JITTER 0

#ifndef RW_PATH
#define RW_PATH "./"
#endif

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

static int dumpWaveform(const IasAudio::IasAudioArea *audioAreas,
                        snd_pcm_uframes_t offset,
                        uint32_t numFrames,
                        IasAudioCommonDataFormat dataFormat,
                        unsigned int numChannels,
                        SNDFILE *sndfile)
{
  unsigned char  *samples[numChannels];
  uint32_t     steps[numChannels];
  uint32_t     numWrittenFrames = 0;

  /* verify and prepare the contents of areas */
  for (uint32_t cntChannels = 0; cntChannels < numChannels; cntChannels++)
  {
    if ((audioAreas[cntChannels].first % 8) != 0)
    {
      std::cout << "Invalid audioAreas, aborting..." << std::endl;
      return -1;
    }
    samples[cntChannels] = /*(signed short *)*/(((unsigned char *)audioAreas[cntChannels].start) + (audioAreas[cntChannels].first / 8));
    if ((audioAreas[cntChannels].step % 16) != 0)
    {
      std::cout << "Invalid audioAreas, aborting..." << std::endl;
      return -1;
    }
    steps[cntChannels] = audioAreas[cntChannels].step / 8;
    samples[cntChannels] += offset * steps[cntChannels];
  }


  switch (dataFormat)
  {
    case IasAudioCommonDataFormat::eIasFormatInt16:
    {
      int16_t  ioData[numChannels * numFrames];
      int16_t* ioDataPtr = ioData;

      for (uint32_t cntFrames = 0; cntFrames < numFrames; cntFrames++)
      {
        for (uint32_t cntChannels = 0; cntChannels < numChannels; cntChannels++)
        {
          int16_t* samplePtr = (int16_t*)samples[cntChannels];
          *ioDataPtr = *samplePtr;
          ioDataPtr++;
          samples[cntChannels] += steps[cntChannels];
        }
      }
      numWrittenFrames = static_cast<uint32_t>(sf_writef_short(sndfile, ioData, static_cast<sf_count_t>(numFrames)));

      break;
    }
    default:
      break;
  }



  if (numWrittenFrames != numFrames)
  {
    std::cout << "Error while writing to output files. Only " << numWrittenFrames
              << " frames have been written (instead of " << numFrames << ")" << std::endl;
  }
  return 0;
}


TEST_F(IasAlsaHandlerTest, streamCaptureAsyncStereo)
{
  std::string outputFileName = {std::string(RW_PATH) + "output.wav"};

  std::cout << "==========================================================" << std::endl;
  std::cout << "Test of the asynchronous ALSA handler in capture direction" << std::endl;
  std::cout << "==========================================================" << std::endl;
  std::cout << "Writing PCM frames to " << outputFileName                   << std::endl;

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        =*/ "invalid",   // will be overwritten later
    /*.numChannels =*/ 2,
    /*.samplerate  =*/ 48000,
    /*.dataFormat  =*/ IasAudio::eIasFormatInt16,
    /*.clockType   =*/ IasAudio::eIasClockReceivedAsync,
    /*.periodSize  =*/ 240,
    /*.numPeriods  =*/ 4
  };

  IasAlsaHandler::IasResult result = IasAlsaHandler::eIasOk;

  IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
  *devParams = cSinkDeviceParams;
  devParams->name = deviceName; // adopt the actual device name (provided by IasAlsaHandlerTestMain)

  IasAlsaHandler* myAlsaHandler = new IasAlsaHandler(devParams);
  result = myAlsaHandler->init(eIasDeviceTypeSource);
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

  uint32_t cntPeriods = 0;
  uint32_t samples = 0;

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
  int32_t  drift = 0;
  uint32_t playtime = 0;
  uint32_t cntSamples = 0;

  uint32_t samplesPerMilliSecond = cSinkDeviceParams.samplerate / 1000;


  SNDFILE* outputFile = nullptr;
  SF_INFO  outputFileInfo;

  outputFileInfo.samplerate = 48000;
  outputFileInfo.channels   = 2;
  outputFileInfo.format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_24);
  outputFile = sf_open(outputFileName.c_str(), SFM_WRITE, &outputFileInfo);
  if (outputFile == NULL)
  {
    std::cout << "File cannot be opened, " << outputFileName << std::endl;
    exit(1);
  }

  while (cntPeriods < numPeriodsToProcess)
  {
    uint32_t size = periodSize;
    rbResult = myRingBuffer->updateAvailable(IasRingBufferAccess::eIasRingBufferAccessRead, &samples);
    ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
    //ASSERT_TRUE(samples >= devParams->periodSize);

    if (samples < devParams->periodSize)
    {
      continue;
    }
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

      rbResult = myRingBuffer->beginAccess(eIasRingBufferAccessRead, &audioAreas, &offset, &frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

      // Verify that ALSA handler in asynchronous mode always provides enough free samples in its input buffer.
      ASSERT_TRUE(frames == size);

      // Write the received PCM frames into the output file.
      int err = dumpWaveform(audioAreas, offset, static_cast<int>(frames), actualDataFormat, numChannels, outputFile);
      ASSERT_EQ(0, err);

      rbResult = myRingBuffer->endAccess(IasRingBufferAccess::eIasRingBufferAccessRead, offset, frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
      size -= frames;
    }
    cntPeriods++;
  }

  if (sf_close(outputFile))
  {
    std::cout << "File cannot be closed, " << outputFileName << std::endl;
  }

  delete myAlsaHandler;
  delete myDrift;
}


}
