/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file  IasAlsaHandlerTestPlayback.cpp
 *
 * @date  2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "avbaudiomodules/internal/audio/common/IasAlsaTypeConversion.hpp"
#include "IasAlsaHandlerTest.hpp"

// Name of the ALSA device, defined and initialized in IasAlsaHandlerTestMain.cpp
extern std::string deviceName;

// Other global variables, defined and initialized in IasAlsaHandlerTestMain.cpp
extern uint32_t numPeriodsToProcess;

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


TEST_F(IasAlsaHandlerTest, streamPlaybackStereo)
{
  std::cout << "==============================================" << std::endl;
  std::cout << "Test of the ALSA handler in playback direction" << std::endl;
  std::cout << "==============================================" << std::endl;

  // Parameters for the audio device configuration.
  const IasAudioDeviceParams cSinkDeviceParams =
  {
    /*.name        =*/ "invalid",   // will be overwritten later
    /*.numChannels =*/ 2,
    /*.samplerate  =*/ 48000,
    /*.dataFormat  =*/ IasAudio::eIasFormatInt16,
    /*.clockType   =*/ IasAudio::eIasClockReceived,
    /*.periodSize  =*/ 2400,
    /*.numPeriods  =*/ 4
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

  uint32_t  cntPeriods = 0;
  uint32_t  samples = 0;
  double phase = 0.0;

  while (cntPeriods < numPeriodsToProcess)
  {
    uint32_t size = periodSize;
    rbResult = myRingBuffer->updateAvailable(IasRingBufferAccess::eIasRingBufferAccessWrite, &samples);
    if (rbResult == IasAudioRingBufferResult::eIasRingBuffTimeOut)
    {
      std::cout << "timeout; trying to continue" << std::endl;
      size = 0;
    }
    else
    {
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
    }
    std::cout << "samples = " << samples << std::endl;

    while (size > 0)
    {
      uint32_t offset;
      uint32_t frames = size;
      IasAudio::IasAudioArea *audioAreas;

      uint32_t numChannels = myRingBuffer->getNumChannels();
      ASSERT_TRUE(numChannels != 0);

      rbResult = myRingBuffer->beginAccess(eIasRingBufferAccessWrite, &audioAreas, &offset, &frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

      // Generate the sine wave.
      generate_sine(audioAreas, offset, static_cast<int>(frames), &phase, actualDataFormat, cSinkDeviceParams.samplerate, numChannels);

      rbResult = myRingBuffer->endAccess(IasRingBufferAccess::eIasRingBufferAccessWrite, offset, frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);
      size -= frames;
    }
    cntPeriods++;
  }

  delete myAlsaHandler;
}


}
