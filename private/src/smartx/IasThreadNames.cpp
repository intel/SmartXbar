/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasThreadNames.cpp
 * @date   2016
 * @brief
 */

#include <sstream>
#include <iomanip>
#include <sched.h>
#include "smartx/IasThreadNames.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"


#ifndef RW_TMP_PATH
#define RW_TMP_PATH "/tmp/"
#endif

using namespace std;

namespace IasAudio {

static const std::string cClassName = "IasThreadNames::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

static const std::string cFileName = std::string(RW_TMP_PATH) + "smartx_threads.txt";
static const std::string cRtThreadPrefix = "xbar_rt_";
static const std::string cStdThreadPrefix = "xbar_std_";

IasThreadNames::IasThreadNames()
  :mLog(IasAudioLogging::registerDltContext("SMX", "SmartX Common"))
  ,mRtIndex(0)
  ,mStdIndex(0)
  ,mMutex()
  ,mFile()
{
}

IasThreadNames::~IasThreadNames()
{
  if (mFile.is_open())
  {
    mFile.close();
  }
}

IasThreadNames* IasThreadNames::getInstance()
{
  static IasThreadNames theInstance;
  return &theInstance;
}

void IasThreadNames::setThreadName(IasThreadType type, const std::string &description)
{
  lock_guard<mutex> lock(mMutex);
  if (!mFile.is_open())
  {
    // First clear the file if it already exists
    mFile.open(cFileName, ofstream::trunc);
    if (mFile.is_open())
    {
      mFile.close();
    }
    // Then open again for appending
    mFile.open(cFileName, ofstream::app);
    if (mFile.good())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully opened file", cFileName, "for writing");
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error opening the file", cFileName, "for writing");
    }
  }
  stringstream threadName;
  if (type == eIasRealTime)
  {
    threadName << cRtThreadPrefix << setw(3) << setfill('0') << mRtIndex++ << " ";
  }
  else
  {
    threadName << cStdThreadPrefix << setw(3) << setfill('0') << mStdIndex++;
  }
  stringstream fulltext;
  fulltext << threadName.str() << " => " << description;
  mFile << fulltext.str() << endl;
  mFile.flush();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, fulltext.str());
  int32_t result = pthread_setname_np(pthread_self(), threadName.str().c_str());
  IAS_ASSERT(result == 0);
  (void)result;
}



} //namespace IasAudio
