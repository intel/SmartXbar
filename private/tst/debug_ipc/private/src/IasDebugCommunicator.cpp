/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasDebugCommunicator.cpp
 * @date 2017
 * @brief The definition of the IasDebugCommunicator.
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <boost/filesystem.hpp>

#include "IasDebugCommunicator.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "smartx/IasSetupImpl.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "smartx/IasSmartXPriv.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "smartx/IasDebugMutexDecorator.hpp"

static IasAudio::IasDebugCommunicator *debugCommunicator = nullptr;

extern "C" {

IAS_AUDIO_PUBLIC IasAudio::IasIDebug* debugHook(IasAudio::IasIDebug *debug)
{
  IasAudio::IasIDebug* debugDecorated = nullptr;
  if (debugCommunicator == nullptr)
  {
    debugDecorated = new IasAudio::IasDebugMutexDecorator(debug);
    debugCommunicator = new IasAudio::IasDebugCommunicator(debugDecorated);
    IAS_ASSERT(debugCommunicator != nullptr);
    DltContext *dltLog = IasAudio::IasAudioLogging::registerDltContext("SMX", "SmartX Common");
    IAS_ASSERT(dltLog != nullptr);
    DLT_LOG_CXX(*dltLog, DLT_LOG_ERROR, "*** enabled debug_ipc ***");
  }
  else
  {
    debugDecorated = debugCommunicator->getDebugDecorated();
  }
  return debugDecorated;
}

} // extern "C"

namespace fs = boost::filesystem;

namespace IasAudio {

#ifndef FD_SIGNAL_PATH
#define FD_SIGNAL_PATH "/run/smartx/"
#endif

static const std::string cRuntimeDir(FD_SIGNAL_PATH);
static const std::string cPipePath(cRuntimeDir + "debug");


IasDebugCommunicator::IasDebugCommunicator(IasIDebug *debug)
  :mDebug(debug)
  ,mCheckForPipeThread(nullptr)
  ,mReadInputFromPipe(nullptr)
{
  mCheckForPipeThread = new std::thread(&IasDebugCommunicator::checkForPipeExistence, this);
}

IasDebugCommunicator::~IasDebugCommunicator()
{
  if (mCheckForPipeThread->joinable() == true)
  {
    mCheckForPipeThread->detach();
  }
  if (mReadInputFromPipe->joinable() == true)
  {
    mReadInputFromPipe->detach();
  }
  // No need to delete it here, it is owned and deleted by IasSmartXPriv
  mDebug = nullptr;
}

IasIDebug* IasDebugCommunicator::getDebugDecorated()
{
  return mDebug;
}

void IasDebugCommunicator::checkForPipeExistence()
{
  bool found = false;
  std::string pipePath= cRuntimeDir + "debug";
  while (found == false)
  {
    if (fs::exists(pipePath) == false)
    {
      sleep(1);
    }
    else
    {
      found = true;
    }
  }
  mReadInputFromPipe = new std::thread(&IasDebugCommunicator::readInputFromPipe, this);
}

void IasDebugCommunicator::readInputFromPipe()
{
  bool running = true;

  while (running == true)
  {
    std::ifstream myPipe(cPipePath);
    std::string cmd;
    while (myPipe >> cmd)
    {
      if (cmd.compare("rpd") == 0)
      {
        std::string filePrefix;
        myPipe >> filePrefix;
        if (filePrefix.empty())
        {
          break;
        }
        std::string portName;
        myPipe >> portName;
        if (portName.empty())
        {
          break;
        }
        std::uint32_t duration = 0;
        myPipe >> duration;
        if (duration == 0)
        {
          break;
        }
        mDebug->startRecord(filePrefix, portName, duration);
      }
      else if (cmd.compare("spd") == 0)
      {
        std::string portName;
        myPipe >> portName;
        if (portName.empty())
        {
          break;
        }
        mDebug->stopProbing(portName);
      }
      else if (cmd.compare("close") == 0)
      {
        running = false;
      }
    }
  }
}

} /* namespace IasAudio */
