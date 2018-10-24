/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEventProvider.cpp
 * @date   2015
 * @brief
 */

#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"

#include "audio/smartx/IasEventProvider.hpp"

namespace IasAudio {

static const std::string cClassName = "IasEventProvider::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasEventProvider::IasEventProvider()
  :mLog(IasAudioLogging::registerDltContext("EVT", "SmartX Events"))
  ,mEventQueue()
  ,mMutex()
  ,mCondition()
  ,mNrEvents(0)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Event provider singleton created");
}

IasEventProvider::~IasEventProvider()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Event provider singleton destroyed");
}

IasEventProvider* IasEventProvider::getInstance()
{
  static IasEventProvider theInstance;
  return &theInstance;
}

void IasEventProvider::clearEventQueue()
{
  mEventQueue.clear();
  mNrEvents = 0;
}

IasConnectionEventPtr IasEventProvider::createConnectionEvent()
{
  return std::make_shared<IasConnectionEvent>();
}

void IasEventProvider::destroyConnectionEvent(IasConnectionEventPtr event)
{
  // Nothing to do here currently. However this method can be useful when switching from shared_ptr to
  // normal pointer and using the boost pool implementation for efficient handling of events
  event = nullptr;
}

IasSetupEventPtr IasEventProvider::createSetupEvent()
{
  return std::make_shared<IasSetupEvent>();
}

void IasEventProvider::destroySetupEvent(IasSetupEventPtr event)
{
  // Nothing to do here currently. However this method can be useful when switching from shared_ptr to
  // normal pointer and using the boost pool implementation for efficient handling of events
  event = nullptr;
}

IasModuleEventPtr IasEventProvider::createModuleEvent()
{
  return std::make_shared<IasModuleEvent>();
}

void IasEventProvider::destroyModuleEvent(IasModuleEventPtr event)
{
  // Nothing to do here currently. However this method can be useful when switching from shared_ptr to
  // normal pointer and using the boost pool implementation for efficient handling of events
  event = nullptr;
}

void IasEventProvider::send(IasEventPtr event)
{
  mEventQueue.push(event);

  // Keep the lock only inside this scope
  {
    std::lock_guard<std::mutex> lk(mMutex);
    mNrEvents++;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Event put into queue");
  mCondition.notify_one();
}

IasEventProvider::IasResult IasEventProvider::waitForEvent(uint32_t timeout)
{
  std::unique_lock<std::mutex> lk(mMutex);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Waiting for event");
  bool isEventReceived = mCondition.wait_for(lk, std::chrono::milliseconds(timeout), [this] { return mNrEvents != 0; });
  if (isEventReceived == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Event received. Nr events in queue=", mNrEvents);
    mNrEvents--;
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Timeout while waiting for events", mNrEvents);
    return eIasTimeout;
  }
}

IasEventProvider::IasResult IasEventProvider::getNextEvent(IasEventPtr* event)
{
  if (event == nullptr)
  {
    /**
     * @log Destination for return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "event == nullptr");
    return eIasFailed;
  }
  IasEventPtr eventPtr;
  if (mEventQueue.try_pop(eventPtr))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Event removed from event queue");
    *event = eventPtr;
    return eIasOk;
  }
  else
  {
    /**
     * @log There is currently no event in the event queue.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "No event in event queue");
    return eIasNoEventAvailable;
  }
}



} //namespace IasAudio
