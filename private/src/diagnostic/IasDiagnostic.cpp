/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * File:   IasDiagnostic.cpp
 */

#include <dlt/dlt_types.h>

#include "diagnostic/IasDiagnostic.hpp"
#include "diagnostic/IasDiagnosticStream.hpp"
#include "diagnostic/IasDiagnosticLogWriter.hpp"

namespace IasAudio {

static const std::string cClassName = "IasDiagnostic::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasDiagnostic::IasDiagnostic()
  :mLog(IasAudioLogging::registerDltContext("AHD", "ALSA Handler"))
  ,mStreams()
  ,mLogWriter()
  ,mMutex()
  ,mIsInitialized(false)
{
}

IasDiagnostic::~IasDiagnostic()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX);
}

IasDiagnostic* IasDiagnostic::getInstance()
{
  static IasDiagnostic theInstance;
  return &theInstance;
}

void IasDiagnostic::setConfigParameters(std::uint32_t logPeriodTime, std::uint32_t numEntriesPerMsg)
{
  mMutex.lock();
  if (mIsInitialized == false)
  {
    mLogWriter.setConfigParameters(logPeriodTime, numEntriesPerMsg);
    mIsInitialized = true;
  }
  mMutex.unlock();
}

IasDiagnostic::IasResult IasDiagnostic::registerStream(const struct IasDiagnosticStream::IasParams& params, IasDiagnosticStreamPtr* newStream)
{
  if (newStream == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "newStream == nullptr");
    return IasResult::eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
    "deviceName=", params.deviceName,
    "portName=", params.portName,
    "copyTo=", params.copyTo,
    "periodTime=", params.periodTime,
    "errorThreshold=", params.errorThreshold);
  IasDiagnosticStreamPtr createdStream = std::make_shared<IasDiagnosticStream>(params, mLogWriter);
  IAS_ASSERT(createdStream != nullptr);
  IasDiagnosticListItem newItem = {params.deviceName, params.portName, createdStream};
  bool found = false;
  mMutex.lock();
  for (auto iterItem = mStreams.begin(); iterItem != mStreams.end(); )
  {
    if (iterItem->deviceName.compare(params.deviceName) == 0)
    {
      iterItem = mStreams.erase(iterItem);
      mStreams.emplace_back(newItem);
      found = true;
      break;
    }
    else
    {
      ++iterItem;
    }
  }
  if (found == false)
  {
    mStreams.emplace_back(newItem);
  }
  *newStream = createdStream;
  mMutex.unlock();
  return IasResult::eIasOk;
}

IasDiagnostic::IasResult IasDiagnostic::startStream(const std::string& name)
{
  IasDiagnosticStream::IasResult streamRes = IasDiagnosticStream::eIasFailed;
  for (auto item : mStreams)
  {
    if (item.portName.compare(name) == 0)
    {
      streamRes = item.stream->startStream();
      break;
    }
  }
  return static_cast<IasResult>(streamRes);
}

IasDiagnostic::IasResult IasDiagnostic::stopStream(const std::string& name)
{
  IasDiagnosticStream::IasResult streamRes = IasDiagnosticStream::eIasFailed;
  for (auto item : mStreams)
  {
    if (item.portName.compare(name) == 0)
    {
      streamRes = item.stream->stopStream();
      break;
    }
  }
  return static_cast<IasResult>(streamRes);
}

bool IasDiagnostic::isThreadFinished()
{
  return mLogWriter.isThreadFinished();
}

} /* namespace IasAudio */
