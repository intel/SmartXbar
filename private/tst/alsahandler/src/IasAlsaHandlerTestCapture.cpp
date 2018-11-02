/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file  IasAlsaHandlerTestCapture.cpp
 *
 * @date  2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "avbaudiomodules/internal/audio/common/IasAlsaTypeConversion.hpp"
#include "IasAlsaHandlerTest.hpp"

#ifndef RW_PATH
#define RW_PATH "./"
#endif

// Name of the ALSA device, defined and initialized in IasAlsaHandlerTestMain.cpp
extern std::string deviceName;

// Other global variables, defined and initialized in IasAlsaHandlerTestMain.cpp
extern uint32_t numPeriodsToProcess;

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


TEST_F(IasAlsaHandlerTest, streamCaptureStereo)
{
  std::string outputFileName = {std::string(RW_PATH) + "output.wav"};

  std::cout << "=============================================" << std::endl;
  std::cout << "Test of the ALSA handler in capture direction" << std::endl;
  std::cout << "=============================================" << std::endl;
  std::cout << "Writing PCM frames to " << outputFileName      << std::endl;

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

  uint32_t cntPeriods = 0;
  uint32_t samples = 0;


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

      rbResult = myRingBuffer->beginAccess(eIasRingBufferAccessRead, &audioAreas, &offset, &frames);
      ASSERT_EQ(IasAudioRingBufferResult::eIasRingBuffOk, rbResult);

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
}


}
