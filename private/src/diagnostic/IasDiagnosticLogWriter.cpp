/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * File:   IasDiagnosticLogWriter.cpp
 */

#include <chrono>
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "diagnostic/IasDiagnosticLogWriter.hpp"
#include "diagnostic/IasDiagnosticStream.hpp"

namespace IasAudio {

static const std::string cClassName = "IasDiagnosticLogWriter::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasDiagnosticLogWriter::IasDiagnosticLogWriter()
  :mLog(IasAudioLogging::registerDltContext("AHD", "ALSA Handler"))
  ,mLogCtxWorker(IasAudioLogging::registerDltContext("AHDL", "ALSA Handler Log Writer Worker"))
  ,mReadBuffer(nullptr)
  ,mFilesToLog()
  ,mLogWorker()
  ,mMutexFilesToLog()
  ,mLogWorkerRunning(false)
  ,mCondFinished()
  ,mCondFinishedMutex()
  ,mFileIdx(0)
  ,mLineIdx(0)
  ,mLogPeriodTime(0)
  ,mNumEntriesPerMsg(0)
  ,mSizeOfReadBuffer(0)
{
}

IasDiagnosticLogWriter::~IasDiagnosticLogWriter()
{
  mLogWorkerRunning = false;
  if (mLogWorker.joinable())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Log worker thread will be joined now");
    mLogWorker.join();
  }
  delete[] mReadBuffer;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX);
}

void IasDiagnosticLogWriter::setConfigParameters(std::uint32_t logPeriodTime, std::uint32_t numEntriesPerMsg)
{
  mLogPeriodTime = logPeriodTime;
  mNumEntriesPerMsg = numEntriesPerMsg;
  if (mNumEntriesPerMsg > 19)
  {
    // Limit to maximum of 18 entries, as DLT has a message length limit.
    mNumEntriesPerMsg = 18;
  }
  mSizeOfReadBuffer = static_cast<std::uint16_t>(mNumEntriesPerMsg*cBytesPerPeriod);
  if (mSizeOfReadBuffer > 1024)
  {
    // Limit to maximum of 1024 bytes rounded to the nearest multiple of cBytesPerPeriod
    mSizeOfReadBuffer = static_cast<std::uint16_t>((1024/cBytesPerPeriod)*1024);
  }
  mReadBuffer = new char[mSizeOfReadBuffer];
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "logPeriodTime(ms)=", mLogPeriodTime, "numEntriesPerMsg=", mNumEntriesPerMsg);
}


void IasDiagnosticLogWriter::addFile(const std::string& fileName)
{
  mMutexFilesToLog.lock();
  mFilesToLog.emplace_back(fileName);
  mMutexFilesToLog.unlock();
  if (mReadBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setConfigParameters not called yet. Don't start worker thread.");
    return;
  }
  if (mLogWorkerRunning == false)
  {
    if (mLogWorker.joinable() == true)
    {
      mLogWorker.join();
    }
    mCondFinishedMutex.lock();
    mLogWorkerRunning = true;
    mCondFinishedMutex.unlock();
    mLogWorker = std::thread(&IasDiagnosticLogWriter::logFile, this);
  }
}

void IasDiagnosticLogWriter::logFile()
{
  do
  {
    mMutexFilesToLog.lock();
    if (mFilesToLog.empty() == false)
    {
      auto fileName = mFilesToLog.front();
      mFilesToLog.pop_front();
      mMutexFilesToLog.unlock();
      DLT_LOG_CXX(*mLogCtxWorker, DLT_LOG_INFO, LOG_PREFIX, "Opening file", fileName, "for logging to DLT");
      std::ifstream srcFile;
      srcFile.open(fileName, std::ios::binary);
      do
      {
        srcFile.read(mReadBuffer, mSizeOfReadBuffer);
        std::uint32_t numEntries = static_cast<std::uint32_t>(srcFile.gcount()) / cBytesPerPeriod;
        if(mLogCtxWorker->log_level_ptr && ((DLT_LOG_INFO)<=(int)*(mLogCtxWorker->log_level_ptr)))
        {
          DltContextData log_local;
          int dlt_local;
          dlt_local = dlt_user_log_write_start(mLogCtxWorker, &log_local, DLT_LOG_INFO);
          if (dlt_local > 0)
          {
            DLT_CSTRING("f"); DLT_UINT32(mFileIdx);
            DLT_CSTRING("l"); DLT_UINT32(mLineIdx);
            std::uint64_t* ptr_ui64 = reinterpret_cast<std::uint64_t*>(mReadBuffer);
            for (std::uint32_t count=0; count<numEntries; count++)
            {
              std::uint64_t timestampDeviceBuffer;
              std::uint64_t numTransmittedFramesDevice;
              std::uint64_t timestampAsrcBuffer;
              std::uint64_t numTransmittedFramesAsrc;
              std::uint32_t asrcBufferNumFramesAvailable;
              std::uint32_t numTotalFrames;
              std::float_t ratioAdaptive;
              timestampDeviceBuffer = *ptr_ui64++;
              numTransmittedFramesDevice = *ptr_ui64++;
              timestampAsrcBuffer = *ptr_ui64++;
              numTransmittedFramesAsrc = *ptr_ui64++;
              std::uint32_t* ptr_ui32 = reinterpret_cast<std::uint32_t*>(ptr_ui64);
              asrcBufferNumFramesAvailable = *ptr_ui32++;
              numTotalFrames = *ptr_ui32++;
              std::float_t* ptr_float = reinterpret_cast<std::float_t*>(ptr_ui32);
              ratioAdaptive = *ptr_float++;
              ptr_ui64 = reinterpret_cast<std::uint64_t*>(ptr_float);
              DLT_HEX64(timestampDeviceBuffer);
              DLT_HEX64(numTransmittedFramesDevice);
              DLT_HEX64(timestampAsrcBuffer);
              DLT_HEX64(numTransmittedFramesAsrc);
              DLT_HEX32(asrcBufferNumFramesAvailable);
              DLT_HEX32(numTotalFrames);
              DLT_FLOAT32(ratioAdaptive);
            }
            (void)dlt_user_log_write_finish(&log_local);
          }
        }
        mLineIdx++;
        std::this_thread::sleep_for(std::chrono::milliseconds(mLogPeriodTime));
      }
      while((srcFile.eof() == false) && (mLogWorkerRunning == true));
      srcFile.close();
      mFileIdx++;
      mLineIdx = 0;
      std::remove(fileName.c_str());
      DLT_LOG_CXX(*mLogCtxWorker, DLT_LOG_INFO, LOG_PREFIX, "Removed", fileName);
    }
    else
    {
      mMutexFilesToLog.unlock();
      {
        std::lock_guard<std::mutex> lk(mCondFinishedMutex);
        mLogWorkerRunning = false;
      }
      DLT_LOG_CXX(*mLogCtxWorker, DLT_LOG_INFO, LOG_PREFIX, "No more files to write to log, exiting worker thread");
      break;
    }

  }
  while(mLogWorkerRunning == true);
  mCondFinished.notify_one();
}

bool IasDiagnosticLogWriter::isThreadFinished()
{
  std::unique_lock<std::mutex> lk(mCondFinishedMutex);
  bool status = mCondFinished.wait_for(lk, std::chrono::seconds(60), [this] { return mLogWorkerRunning == false; });
  if (status == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Log writer thread is finished");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Log writer thread is not finished after timeout of 60s");
  }
  return status;
}


} /* namespace IasAudio */
