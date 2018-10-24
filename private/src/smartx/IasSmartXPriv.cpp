/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXPriv.cpp
 * @date   2015
 * @brief
 */

#include <iostream>
#include "version.h"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasCommonVersion.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "smartx/IasSmartXPriv.hpp"
#include "smartx/IasRoutingImpl.hpp"
#include "smartx/IasSetupImpl.hpp"
#include "smartx/IasRoutingImpl.hpp"
#include "smartx/IasProcessingImpl.hpp"
#include "smartx/IasDebugImpl.hpp"
#include "smartx/IasConfiguration.hpp"
#include "audio/smartx/IasEventProvider.hpp"

#include "smartx/IasConfigFile.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"

extern "C" {

IAS_AUDIO_PUBLIC IasAudio::IasIDebug* debugHook(IasAudio::IasIDebug* debug)
{
  return debug;
}

IAS_AUDIO_PUBLIC IasAudio::IasISetup* setupHook(IasAudio::IasISetup* setup)
{
  return setup;
}

IAS_AUDIO_PUBLIC IasAudio::IasIProcessing* processingHook(IasAudio::IasIProcessing* processing)
{
  return processing;
}

IAS_AUDIO_PUBLIC IasAudio::IasIRouting* routingHook(IasAudio::IasIRouting* routing)
{
  return routing;
}

} // extern "C"


namespace IasAudio {

static const std::string cClassName = "IasSmartXPriv::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasSmartXPriv::IasSmartXPriv()
  :mLog(IasAudioLogging::registerDltContext("SMX", "SmartX Common"))
  ,mConfig(nullptr)
  ,mSetup(nullptr)
  ,mRouting(nullptr)
  ,mProcessing(nullptr)
  ,mDebug(nullptr)
  ,mCmdDispatcher(nullptr)
{
}

IasSmartXPriv::~IasSmartXPriv()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  IasEventProvider::getInstance()->clearEventQueue();
  delete mSetup;
  delete mRouting;
  delete mProcessing;
  delete mDebug;
  mCmdDispatcher = nullptr;
  mConfig = nullptr;
}

IasSmartXPriv::IasResult IasSmartXPriv::init()
{
  IasConfigFile *cfgFile = IasConfigFile::getInstance();
  IAS_ASSERT(cfgFile != nullptr);
  cfgFile->load();
  mConfig = std::make_shared<IasConfiguration>();
  IAS_ASSERT(mConfig != nullptr);
  mCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IAS_ASSERT(mCmdDispatcher != nullptr);

  mDebug = debugHook(debug());
  mRouting = routingHook(routing());
  mSetup = setupHook(setup());
  mProcessing = processingHook(processing());

  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "ias-audio-common version:", getLibCommonVersion());
  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "ias-audio-smartx version:", VERSION_STRING);

  return eIasOk;
}

IasISetup* IasSmartXPriv::setup()
{
  if (mSetup == nullptr)
  {
    IAS_ASSERT(mConfig != nullptr);
    mSetup = new IasSetupImpl(mConfig, mCmdDispatcher, mRouting);
    IAS_ASSERT(mSetup != nullptr);
  }
  return mSetup;
}

IasIRouting* IasSmartXPriv::routing()
{
  if (mRouting == nullptr)
  {
    IAS_ASSERT(mConfig != nullptr);
    mRouting = new IasRoutingImpl(mConfig);
    IAS_ASSERT(mRouting != nullptr);
  }
  return mRouting;
}

IasIProcessing* IasSmartXPriv::processing()
{
  if (mProcessing == nullptr)
  {
    mProcessing = new IasProcessingImpl(mCmdDispatcher);
    IAS_ASSERT(mProcessing != nullptr);
  }
  return mProcessing;
}

IasIDebug* IasSmartXPriv::debug()
{
  if (mDebug == nullptr)
  {
    mDebug = new IasDebugImpl(mConfig);
    IAS_ASSERT(mDebug != nullptr);
  }
  return mDebug;
}


IasSmartXPriv::IasResult IasSmartXPriv::start()
{
  IAS_ASSERT(mConfig != nullptr);
  IasSmartXPriv::IasResult result = eIasOk;

  // Iterate over all routing zones and start them.
  const IasRoutingZoneMap &rznMap = mConfig->getRoutingZoneMap();
  IasRoutingZoneMap::const_iterator rznMapIt;
  for (rznMapIt=rznMap.begin(); rznMapIt != rznMap.end(); ++rznMapIt)
  {
    IasRoutingZonePtr routingZone = (*rznMapIt).second;
    IasISetup::IasResult setres = setup()->startRoutingZone(routingZone);
    if (setres != IasISetup::eIasOk)
    {
      result = eIasFailed;
    }
  }

  // Iterate over all source devices and start them.
  const IasSourceDeviceMap &sdMap = mConfig->getSourceDeviceMap();
  IasSourceDeviceMap::const_iterator sdMapIt;
  for (sdMapIt=sdMap.begin(); sdMapIt != sdMap.end(); ++sdMapIt)
  {
    IasAudioSourceDevicePtr audioSourceDevice = (*sdMapIt).second;
    IasISetup::IasResult setres = setup()->startAudioSourceDevice(audioSourceDevice);
    if (setres != IasISetup::eIasOk)
    {
      result = eIasFailed;
    }
  }

  return result;
}

IasSmartXPriv::IasResult IasSmartXPriv::stop()
{
  IAS_ASSERT(mConfig != nullptr);
  IasSmartXPriv::IasResult result = eIasOk;

  // Iterate over all routing zones and stop them.
  const IasRoutingZoneMap &rznMap = mConfig->getRoutingZoneMap();
  IasRoutingZoneMap::const_iterator rznMapIt;
  for (rznMapIt=rznMap.begin(); rznMapIt != rznMap.end(); ++rznMapIt)
  {
    IasRoutingZonePtr routingZone = (*rznMapIt).second;
    setup()->stopRoutingZone(routingZone);
  }

  // Iterate over all source devices and stop them.
  const IasSourceDeviceMap &sdMap = mConfig->getSourceDeviceMap();
  IasSourceDeviceMap::const_iterator sdMapIt;
  for (sdMapIt=sdMap.begin(); sdMapIt != sdMap.end(); ++sdMapIt)
  {
    IasAudioSourceDevicePtr audioSourceDevice = (*sdMapIt).second;
    setup()->stopAudioSourceDevice(audioSourceDevice);
  }

  return result;
}

static IasSmartXPriv::IasResult translate(IasEventProvider::IasResult result)
{
  if (result == IasEventProvider::eIasOk)
  {
    return IasSmartXPriv::eIasOk;
  }
  else if (result == IasEventProvider::eIasTimeout)
  {
    return IasSmartXPriv::eIasTimeout;
  }
  else if (result == IasEventProvider::eIasNoEventAvailable)
  {
    return IasSmartXPriv::eIasNoEventAvailable;
  }
  else
  {
    return IasSmartXPriv::eIasFailed;
  }
}

IasSmartXPriv::IasResult IasSmartXPriv::getNextEvent(IasEventPtr* event)
{
  IasEventProvider *eventProvider =  IasEventProvider::getInstance();
  IAS_ASSERT(eventProvider != nullptr);
  return translate(eventProvider->getNextEvent(event));
}

IasSmartXPriv::IasResult IasSmartXPriv::waitForEvent(uint32_t timeout) const
{
  IasEventProvider *eventProvider =  IasEventProvider::getInstance();
  IAS_ASSERT(eventProvider != nullptr);
  return translate(eventProvider->waitForEvent(timeout));
}



} // namespace IasAudio
