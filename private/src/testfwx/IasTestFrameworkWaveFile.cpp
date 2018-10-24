/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkWaveFile.cpp
 * @date   2017
 * @brief  Class for reading / writing WAVE files.
 */

#include "testfwx/IasTestFrameworkWaveFile.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/helper/IasCopyAudioAreaBuffers.hpp"

#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasPipeline.hpp"


namespace IasAudio {


static const std::string cClassName = "IasTestFrameworkWaveFile::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasTestFrameworkWaveFile::IasTestFrameworkWaveFile(IasTestFrameworkWaveFileParamsPtr params)
:mLog(IasAudioLogging::registerDltContext("WF", "Wave File"))
,mParams(params)
,mDataFormat(eIasFormatFloat32)
,mBufferSize(0)
,mIntermediateBuffer(nullptr)
,mArea(nullptr)
,mFileIsOpen(false)
,mFilePtr(nullptr)
,mDummySourceDevice(nullptr)
,mDummySinkDevice(nullptr)
,mAudioPin(nullptr)
{
  IAS_ASSERT(mParams != nullptr);
  mFileInfo.channels = 0;
  mFileInfo.format = 0;
  mFileInfo.frames = 0;
  mFileInfo.samplerate = 0;
  mFileInfo.sections = 0;
  mFileInfo.seekable = 0;
}


IasTestFrameworkWaveFile::~IasTestFrameworkWaveFile()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  reset();
  mParams.reset();
  mDummySourceDevice.reset();
  mDummySinkDevice.reset();
  mAudioPin.reset();
}


IasTestFrameworkWaveFile::IasResult IasTestFrameworkWaveFile::linkAudioPin(IasAudioPinPtr audioPin)
{
  if (audioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Invalid parameter: audioPin == nullptr");
    return eIasFailed;
  }

  IasAudioPin::IasPinDirection pinDirection = audioPin->getDirection();
  switch (pinDirection)
  {
    case IasAudioPin::eIasPinDirectionPipelineInput:
    {
      //check if number of channels in audio pin and in file match
      SF_INFO  fileInfo;
      fileInfo.format = 0;
      SNDFILE* filePtr = sf_open(mParams->fileName.c_str(), SFM_READ, &fileInfo);
      if(filePtr == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Could not open file", mParams->fileName);
        return eIasFailed;
      }

      uint32_t pinNumChannels = audioPin->getParameters()->numChannels;
      if (pinNumChannels != static_cast<uint32_t>(fileInfo.channels))
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Number of channels not match, in file:", fileInfo.channels,"in audio pin:", pinNumChannels);
        return eIasFailed;
      }

      //check sample rate
      IasPipelinePtr pipeline = audioPin->getPipeline();
      IAS_ASSERT(pipeline != nullptr);

      if (static_cast<uint32_t>(fileInfo.samplerate) != pipeline->getParameters()->samplerate)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Sample rate does not match, in file:", mFileInfo.samplerate, "in pipeline:",
                    pipeline->getParameters()->samplerate);
      }

      //check data format
      if ((fileInfo.format & 0x0000F) != SF_FORMAT_FLOAT)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "format of wav file:", fileInfo.format, " does not match requested SF_FORMAT_PCM_FLOAT");
      }

      sf_close(filePtr);

      //create dummy source device
      IasAudioDeviceParamsPtr sourceParams = std::make_shared<IasAudioDeviceParams>();
      sourceParams->name = mParams->fileName;

      mDummySourceDevice = std::make_shared<IasAudioSourceDevice>(sourceParams);
      IAS_ASSERT(mDummySourceDevice != nullptr);

      break;
    }
    case IasAudioPin::eIasPinDirectionPipelineOutput:
    {
      //create dummy sink device
      IasAudioDeviceParamsPtr sinkParams = std::make_shared<IasAudioDeviceParams>();
      sinkParams->name = mParams->fileName;

      mDummySinkDevice = std::make_shared<IasAudioSinkDevice>(sinkParams);
      IAS_ASSERT(mDummySinkDevice != nullptr);

      break;
    }
    default:
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Invalid pin direction, wave file can be linked with pipeline input or output only.");
      return eIasFailed;
    }
  }

  mAudioPin = audioPin;

  return eIasOk;
}


void IasTestFrameworkWaveFile::reset()
{
  if(mFilePtr != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "closing file", mParams->fileName);
    sf_close(mFilePtr);
  }

  mFileInfo.channels = 0;
  mFileInfo.format = 0;
  mFileInfo.frames = 0;
  mFileInfo.samplerate = 0;
  mFileInfo.sections = 0;
  mFileInfo.seekable = 0;

  if (mIntermediateBuffer != nullptr)
  {
    delete[] mIntermediateBuffer;
    mIntermediateBuffer = nullptr;
  }

  if (mArea != nullptr)
  {
    delete[] mArea;
    mArea = nullptr;
  }

  mBufferSize = 0;
  mFileIsOpen = false;
}


IasTestFrameworkWaveFile::IasResult IasTestFrameworkWaveFile::openForReading(uint32_t bufferSize)
{
  if(bufferSize == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: buffer size must not be zero");
    return eIasFailed;
  }
  if (mFileIsOpen)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File is already open");
    return eIasAlreadyOpen;
  }

  if (!isInputFile())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File reading allowed for input files only");
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Opening file", mParams->fileName);
  mFilePtr = sf_open(mParams->fileName.c_str(), SFM_READ, &mFileInfo);
  if(mFilePtr == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Could not open file", mParams->fileName);
    return eIasFailed;
  }

  mBufferSize = bufferSize;

  IasResult res = allocateMemory();
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to allocate memory");
    reset();
    return eIasFailed;
  }

  mFileIsOpen = true;
  return eIasOk;
}


IasTestFrameworkWaveFile::IasResult IasTestFrameworkWaveFile::openForWriting(uint32_t bufferSize)
{
  if (bufferSize == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: buffer size must not be zero");
    return eIasFailed;
  }
  if (mFileIsOpen)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File is already open");
    return eIasAlreadyOpen;
  }

  if (isInputFile())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File writing allowed for output files only");
    return eIasFailed;
  }

  mFileInfo.channels = mAudioPin->getParameters()->numChannels;;
  mFileInfo.samplerate = mAudioPin->getPipeline()->getParameters()->samplerate;
  mFileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Opening file", mParams->fileName);
  mFilePtr = sf_open(mParams->fileName.c_str(), SFM_WRITE, &mFileInfo);
  if(mFilePtr == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Could not open file", mParams->fileName);
    return eIasFailed;
  }

  mBufferSize = bufferSize;
  IasResult res = allocateMemory();
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to allocate memory");
    reset();
    return eIasFailed;
  }

  mFileIsOpen = true;
  return eIasOk;
}


void IasTestFrameworkWaveFile::close()
{
  if (!mFileIsOpen)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "File is already closed");
    return;
  }

  reset();
}


IasTestFrameworkWaveFile::IasResult IasTestFrameworkWaveFile::allocateMemory()
{
  uint32_t sampleSize = toSize(mDataFormat);
  uint32_t numChannels = static_cast<uint32_t>(mFileInfo.channels);
  uint32_t memorySize = numChannels * mBufferSize * sampleSize;

  mIntermediateBuffer = new float[memorySize];
  IAS_ASSERT(mIntermediateBuffer != nullptr);

  mArea = new IasAudioArea[numChannels];
  IAS_ASSERT(mArea != nullptr);

  for(uint32_t channel = 0; channel < numChannels; channel++)
  {
    // Setup areas for interleaved channel layout, since libsndfile reads/writes PCM frames in interleaved format.
    mArea[channel].start = mIntermediateBuffer;
    mArea[channel].first = channel * sampleSize * 8; //expressed in bits
    mArea[channel].step  = numChannels * sampleSize * 8; //expressed in bits
    mArea[channel].index = channel;
    mArea[channel].maxIndex = numChannels - 1;
  }

  return eIasOk;
}


IasTestFrameworkWaveFile::IasResult IasTestFrameworkWaveFile::readFrames(IasAudioArea* area, uint32_t offset, uint32_t numFrames, uint32_t& numFramesRead)
{
  if (area == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: area is nullptr");
    return eIasFailed;
  }
  if(numFrames > mBufferSize)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Buffer size is only", mBufferSize,", can't process", numFrames,"in one call");
    return eIasFailed;
  }
  if (!mFileIsOpen)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File was not open yet");
    return eIasFileNotOpen;
  }

  if (!isInputFile())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Read frames allowed for input files only");
    return eIasFailed;
  }

  numFramesRead = static_cast<uint32_t>(sf_readf_float(mFilePtr, mIntermediateBuffer, numFrames));
  copyAudioAreaBuffers(area, mDataFormat, offset, mFileInfo.channels, 0, numFrames, mArea, mDataFormat, 0, mFileInfo.channels, 0, numFramesRead);

  return eIasOk;
}


IasTestFrameworkWaveFile::IasResult IasTestFrameworkWaveFile::writeFrames(IasAudioArea* area, uint32_t offset, uint32_t numFrames, uint32_t& numFramesWritten)
{
  if (area == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: area is nullptr");
    return eIasFailed;
  }
  if(numFrames > mBufferSize)
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Buffer size is only", mBufferSize,", can't process", numFrames,"in one call");
     return eIasFailed;
   }
  if (!mFileIsOpen)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File was not open yet");
    return eIasFileNotOpen;
  }

  if (isInputFile())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Write frames allowed for output files only");
    return eIasFailed;
  }

  copyAudioAreaBuffers(mArea, mDataFormat, 0, mFileInfo.channels, 0, numFrames, area, mDataFormat, offset, mFileInfo.channels, 0, numFrames);
  numFramesWritten = static_cast<uint32_t>(sf_writef_float(mFilePtr, mIntermediateBuffer, numFrames));

  if (numFramesWritten != numFrames)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error on writing to file, stopping now");
    reset();
    return eIasFailed;
  }

  return eIasOk;
}


bool IasTestFrameworkWaveFile::isInputFile() const
{
  if (mAudioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Wave file is not linked with any pin yet.");
    return false;
  }

  if (mAudioPin->getDirection() == IasAudioPin::eIasPinDirectionPipelineInput)
  {
    return true;
  }

  return false;
}


/*
 * Function to get a IasTestFrameworkWaveFile::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasTestFrameworkWaveFile::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasTestFrameworkWaveFile::eIasOk);
    STRING_RETURN_CASE(IasTestFrameworkWaveFile::eIasFailed);
    STRING_RETURN_CASE(IasTestFrameworkWaveFile::eIasFinished);
    STRING_RETURN_CASE(IasTestFrameworkWaveFile::eIasFileNotOpen);
    STRING_RETURN_CASE(IasTestFrameworkWaveFile::eIasAlreadyOpen);
    DEFAULT_STRING("Invalid IasTestFrameworkWaveFile::IasResult => " + std::to_string(type));
  }
}

} // namespace IasAudio
