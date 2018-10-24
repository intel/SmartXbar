/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * File:   IasDiagnosticStream.cpp
 */

#include <assert.h>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include "diagnostic/IasDiagnosticStream.hpp"
#include "diagnostic/IasDiagnosticLogWriter.hpp"

namespace IasAudio {

#define TMP_PATH "/tmp/"

static const std::string cClassName = "IasDiagnosticStream::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + mParams.deviceName + ":"
#define LOG_STATE "[" + toString(mStreamState) + "]"


IasDiagnosticStream::IasDiagnosticStream(const struct IasParams& params, IasDiagnosticLogWriter& logWriter)
  :mLog(IasAudioLogging::registerDltContext("AHD", "ALSA Handler"))
  ,mLogThread(IasAudioLogging::registerDltContext("AHDD", "ALSA Handler Diagnostic File Operations Thread"))
  ,mParams(params)
  ,mPeriodCounter(0)
  ,mMaxCounter(0)
  ,mTmpFile()
  ,mTmpFileName()
  ,mTmpFileNameFull()
  ,mStreamState(eIasStreamStateIdle)
  ,mStreamStateMutex()
  ,mFileOpen()
  ,mFileClose()
  ,mCondWaitForStart()
  ,mMutexWaitForStart()
  ,mStreamStarted(false)
  ,mErrorCounter(0)
  ,mLogWriter(logWriter)
  ,mFileIdx(0)
{
  if (mParams.periodTime == 0)
  {
    // Set default to 5333us
    mParams.periodTime = 5333;
  }
  std::uint32_t bytesPerSecond = cBytesPerPeriod * 1000 * 1000 / mParams.periodTime;
  std::uint32_t bytesPerHour = bytesPerSecond * 60 * 60;
  mMaxCounter = bytesPerHour / cBytesPerPeriod;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Maximum bytes recorded per hour=", bytesPerHour, "maximum counter=", mMaxCounter);
}

IasDiagnosticStream::~IasDiagnosticStream()
{
  changeState(IasTriggerStateChange::eIasStop);
  isStopped();  // this call really waits until the stream is stopped and the file is closed
  if (mFileOpen.joinable())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "File open thread will be joined now");
    mFileOpen.join();
  }
  if (mFileClose.joinable())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "File close thread will be joined now");
    mFileClose.join();
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX);
}

void IasDiagnosticStream::writeAlsaHandlerData(std::uint64_t timestampDeviceBuffer, std::uint64_t numTransmittedFramesDevice, std::uint64_t timestampAsrcBuffer, std::uint64_t numTransmittedFramesAsrc, std::uint32_t asrcBufferNumFramesAvailable, std::uint32_t numTotalFrames, std::float_t ratioAdaptive)
{
  if (mStreamState != eIasStreamStateStarted)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_STATE, "Stream not started yet");
    return;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Writing data to file, period #", mPeriodCounter);
  char tmpBuffer[cBytesPerPeriod];
  std::uint64_t* ptr_ui64 = reinterpret_cast<std::uint64_t*>(tmpBuffer);
  *ptr_ui64++ = timestampDeviceBuffer;
  *ptr_ui64++ = numTransmittedFramesDevice;
  *ptr_ui64++ = timestampAsrcBuffer;
  *ptr_ui64++ = numTransmittedFramesAsrc;
  std::uint32_t* ptr_ui32 = reinterpret_cast<std::uint32_t*>(ptr_ui64);
  *ptr_ui32++ = asrcBufferNumFramesAvailable;
  *ptr_ui32++ = numTotalFrames;
  std::float_t* ptr_float = reinterpret_cast<std::float_t*>(ptr_ui32);
  *ptr_float = ratioAdaptive;
  mTmpFile.write(tmpBuffer, cBytesPerPeriod);
  ++mPeriodCounter;
  if (mPeriodCounter > mMaxCounter)
  {
    mPeriodCounter = 0;
    changeState(eIasStop);
  }
}

IasDiagnosticStream::IasResult IasDiagnosticStream::startStream()
{
  IasStreamState newState = changeState(eIasStart);
  if ((newState == eIasStreamStateOpening) || (newState == eIasStreamStatePendingOpen))
  {
    return IasResult::eIasOk;
  }
  else
  {
    return IasResult::eIasFailed;
  }
}

IasDiagnosticStream::IasResult IasDiagnosticStream::stopStream()
{
  IasStreamState newState = changeState(eIasStop);
  if ((newState == eIasStreamStateClosing) || (newState == eIasStreamStatePendingClose))
  {
    return IasResult::eIasOk;
  }
  else
  {
    return IasResult::eIasFailed;
  }
}

IasDiagnosticStream::IasStreamState IasDiagnosticStream::changeState(IasTriggerStateChange trigger)
{
  std::lock_guard<std::mutex> lock(mStreamStateMutex);
  switch (mStreamState)
  {
    case eIasStreamStateIdle:
      if (trigger == IasTriggerStateChange::eIasStart)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStateOpening));
        mStreamState = eIasStreamStateOpening;
        if (mFileOpen.joinable())
        {
          mFileOpen.join();
        }
        mFileOpen = std::thread(&IasDiagnosticStream::openFile, this);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Ignoring trigger", toString(trigger));
      }
      break;

    case eIasStreamStateStarted:
      if (trigger == IasTriggerStateChange::eIasStop)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStateClosing));
        mStreamState = eIasStreamStateClosing;
        if (mFileClose.joinable())
        {
          mFileClose.join();
        }
        mFileClose = std::thread(&IasDiagnosticStream::closeFile, this);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Ignoring trigger", toString(trigger));
      }
      break;

    case eIasStreamStateOpening:
      if (trigger == IasTriggerStateChange::eIasOpeningFinished)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStateStarted));
        mStreamState = eIasStreamStateStarted;
        {
          std::lock_guard<std::mutex> lk(mMutexWaitForStart);
          mStreamStarted = true;
        }
        mCondWaitForStart.notify_one();
      }
      else if (trigger == IasTriggerStateChange::eIasStop)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStatePendingClose));
        mStreamState = eIasStreamStatePendingClose;
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Ignoring trigger", toString(trigger));
      }
      break;

    case eIasStreamStatePendingClose:
      if (trigger == IasTriggerStateChange::eIasOpeningFinished)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStateClosing));
        mStreamState = eIasStreamStateClosing;
        {
          std::lock_guard<std::mutex> lk(mMutexWaitForStart);
          mStreamStarted = true;
        }
        mCondWaitForStart.notify_one();
        if (mFileClose.joinable())
        {
          mFileClose.join();
        }
        mFileClose = std::thread(&IasDiagnosticStream::closeFile, this);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Ignoring trigger", toString(trigger));
      }
      break;

    case eIasStreamStateClosing:
      if (trigger == IasTriggerStateChange::eIasClosingFinished)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStateIdle));
        mStreamState = eIasStreamStateIdle;
        {
          std::lock_guard<std::mutex> lk(mMutexWaitForStart);
          mStreamStarted = false;
        }
        mCondWaitForStart.notify_one();
      }
      else if (trigger == IasTriggerStateChange::eIasStart)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStatePendingOpen));
        mStreamState = eIasStreamStatePendingOpen;
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Ignoring trigger", toString(trigger));
      }
      break;

    case eIasStreamStatePendingOpen:
      if (trigger == IasTriggerStateChange::eIasClosingFinished)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStateOpening));
        mStreamState = eIasStreamStateOpening;
        {
          std::lock_guard<std::mutex> lk(mMutexWaitForStart);
          mStreamStarted = false;
        }
        mCondWaitForStart.notify_one();
        if (mFileOpen.joinable())
        {
          mFileOpen.join();
        }
        mFileOpen = std::thread(&IasDiagnosticStream::openFile, this);
      }
      else if (trigger == IasTriggerStateChange::eIasStop)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Trigger=", toString(trigger), ". New state=", toString(eIasStreamStatePendingClose));
        mStreamState = eIasStreamStatePendingClose;
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_STATE, "Ignoring trigger", toString(trigger));
      }
      break;
  }
  return mStreamState;
}

void IasDiagnosticStream::openFile()
{
  if (mTmpFile.is_open() == false)
  {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    char currentTime[20] = {'\0'};
    if (tm != nullptr)
    {
      std::strftime(currentTime, sizeof(currentTime), "%T", tm);
    }
    mTmpFileName = std::string(currentTime) + "_" + mParams.deviceName + "_asrc_diag_" + std::to_string(mFileIdx) + ".bin";
    mFileIdx++;
    // Replace all commas with underscore to have a cleaner filename
    std::replace(mTmpFileName.begin(), mTmpFileName.end(), ',', '_');
    mTmpFileNameFull = std::string(TMP_PATH) + mTmpFileName;
    DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, "Opening tmp file", mTmpFileNameFull, "for writing diagnostic log");
    mTmpFile.open(mTmpFileNameFull, std::ios::binary|std::ios::out|std::ios::trunc);
    mErrorCounter = 0;
  }
  changeState(eIasOpeningFinished);
}

void IasDiagnosticStream::closeFile()
{
  if (mTmpFile.is_open())
  {
    std::uint32_t fileSize = static_cast<std::uint32_t>(mTmpFile.tellp());
    DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, mTmpFileNameFull, "file size=", fileSize);
    mTmpFile.close();
  }
  DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, "Closing file", mTmpFileNameFull, "finished");
  bool writeToLog = false;
  if (mParams.copyTo.compare("log") == 0)
  {
    writeToLog = true;
  }
  if (mErrorCounter >= mParams.errorThreshold)
  {
    if (writeToLog)
    {
      mLogWriter.addFile(mTmpFileNameFull);
    }
    else
    {
      copyFile();
    }
  }
  else
  {
    std::remove(mTmpFileNameFull.c_str());
    DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, "Removed", mTmpFileNameFull);
  }
  mErrorCounter = 0;

  changeState(eIasClosingFinished);
}

void IasDiagnosticStream::copyFile()
{
  std::ofstream dstFile;
  std::ifstream srcFile;
  std::string dstFileName = mParams.copyTo;
  if (dstFileName.back() != '/')
  {
    dstFileName.append("/");
  }
  dstFileName += mTmpFileName;
  DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, "Copying from", mTmpFileNameFull, "to", dstFileName);
  dstFile.open(dstFileName, std::ios::binary|std::ios::out|std::ios::trunc);
  if (dstFile.is_open())
  {
    srcFile.open(mTmpFileNameFull, std::ios::binary);

    dstFile<<srcFile.rdbuf();
    srcFile.close();
    dstFile.close();
    DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, "Copying from", mTmpFileNameFull, "to", dstFileName, "finished successfully.");
  }
  else
  {
    DLT_LOG_CXX(*mLogThread, DLT_LOG_ERROR, LOG_PREFIX, "Destination file", dstFileName, "couldn't be created");
  }
  std::remove(mTmpFileNameFull.c_str());
  DLT_LOG_CXX(*mLogThread, DLT_LOG_INFO, LOG_PREFIX, "Removed", mTmpFileNameFull);
}

bool IasDiagnosticStream::isStarted()
{
  std::unique_lock<std::mutex> lk(mMutexWaitForStart);
  bool status = mCondWaitForStart.wait_for(lk, std::chrono::seconds(1), [this] { return mStreamStarted == true; });
  if (status == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stream is started");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stream is not started after timeout of 1s");
  }
  return status;
}

bool IasDiagnosticStream::isStopped()
{
  std::unique_lock<std::mutex> lk(mMutexWaitForStart);
  bool status = mCondWaitForStart.wait_for(lk, std::chrono::seconds(1), [this] { return mStreamStarted == false; });
  if (status == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stream is stopped");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stream is not stopped after timeout of 1s");
  }
  return status;
}

void IasDiagnosticStream::errorOccurred()
{
  ++mErrorCounter;
}


#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

std::string IasDiagnosticStream::toString(const IasStreamState& streamState)
{
  switch(streamState)
  {
    STRING_RETURN_CASE(IasDiagnosticStream::IasStreamState::eIasStreamStateIdle);
    STRING_RETURN_CASE(IasDiagnosticStream::IasStreamState::eIasStreamStateStarted);
    STRING_RETURN_CASE(IasDiagnosticStream::IasStreamState::eIasStreamStateOpening);
    STRING_RETURN_CASE(IasDiagnosticStream::IasStreamState::eIasStreamStateClosing);
    STRING_RETURN_CASE(IasDiagnosticStream::IasStreamState::eIasStreamStatePendingClose);
    STRING_RETURN_CASE(IasDiagnosticStream::IasStreamState::eIasStreamStatePendingOpen);
    DEFAULT_STRING("Invalid IasStreamState => " + std::to_string(streamState));
  }
}

std::string IasDiagnosticStream::toString(const IasTriggerStateChange& trigger)
{
  switch(trigger)
  {
    STRING_RETURN_CASE(IasDiagnosticStream::IasTriggerStateChange::eIasStart);
    STRING_RETURN_CASE(IasDiagnosticStream::IasTriggerStateChange::eIasStop);
    STRING_RETURN_CASE(IasDiagnosticStream::IasTriggerStateChange::eIasOpeningFinished);
    STRING_RETURN_CASE(IasDiagnosticStream::IasTriggerStateChange::eIasClosingFinished);
    DEFAULT_STRING("Invalid IasTriggerStateChange => " + std::to_string(trigger));
  }
}


} /* namespace IasAudio */
