/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartX.cpp
 * @date   2015
 * @brief
 */

#include <iostream>
#include <dlt/dlt_types.h>
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "IasSmartXPriv.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"


namespace IasAudio {

static const std::string cClassName = "IasSmartX::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

std::atomic_int IasSmartX::mNumberInstances(0);

IasSmartX::IasSmartX()
  :mPriv(NULL)
{
}

IasSmartX::~IasSmartX()
{
  delete mPriv;
}

int32_t IasSmartX::getMajor()
{
  return SMARTX_API_MAJOR;
}

int32_t IasSmartX::getMinor()
{
  return SMARTX_API_MINOR;
}

int32_t IasSmartX::getPatch()
{
  return SMARTX_API_PATCH;
}

std::string IasSmartX::getVersion()
{
  return std::to_string(SMARTX_API_MAJOR) + "." + std::to_string(SMARTX_API_MINOR) + "." + std::to_string(SMARTX_API_PATCH);
}

bool IasSmartX::hasFeature(const std::string&)
{
  return false;
}

bool IasSmartX::isAtLeast(int32_t major, int32_t minor, int32_t patch)
{
  if (major > SMARTX_API_MAJOR) return false;
  if (major < SMARTX_API_MAJOR) return true;
  if (minor > SMARTX_API_MINOR) return false;
  if (minor < SMARTX_API_MINOR) return true;
  if (patch > SMARTX_API_PATCH) return false;
  return true;
}

IasSmartX* IasSmartX::create()
{
  IasAudioLogging::addDltContextItem("SMX", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("SMJ", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("SXC", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("SXP", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("RZN", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("BFT", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("SMW", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("AHD", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("CFG", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("ROU", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("RBM", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("EVT", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("MDL", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("PFW", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("AHDD", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("AHDL", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  DltContext *dltCtx = IasAudioLogging::registerDltContext("SMX", "SmartX Common");
  int32_t expected = 0;
  if (mNumberInstances.compare_exchange_strong(expected, 1) == false)
  {
    DLT_LOG_CXX(*dltCtx, DLT_LOG_ERROR, LOG_PREFIX, "SmartXbar instance already created.");
    return nullptr;
  }
  IasSmartX *smartx = new IasSmartX();
  if (smartx == nullptr)
  {
    DLT_LOG_CXX(*dltCtx, DLT_LOG_ERROR, LOG_PREFIX, "Not enough memory to create SmartXbar instance.");
    return nullptr;
  }
  IasResult result = smartx->init();
  IAS_ASSERT(result == eIasOk);
  (void)result;
  return smartx;
}

void IasSmartX::destroy(IasSmartX *instance)
{
  int32_t expected = 1;
  if (mNumberInstances.compare_exchange_strong(expected, 0) == true)
  {
    delete instance;
  }
}

IasSmartX::IasResult IasSmartX::init()
{
  mPriv = new IasSmartXPriv();
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasSmartX::IasResult>(mPriv->init());
}

IasISetup* IasSmartX::setup()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->setup();
}

IasIRouting* IasSmartX::routing()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->routing();
}

IasIProcessing* IasSmartX::processing()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->processing();
}

IasIDebug* IasSmartX::debug()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->debug();
}

IasSmartX::IasResult IasSmartX::start()
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasSmartX::IasResult>(mPriv->start());
}

IasSmartX::IasResult IasSmartX::stop()
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasSmartX::IasResult>(mPriv->stop());
}

IasSmartX::IasResult IasSmartX::waitForEvent(uint32_t timeout) const
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasSmartX::IasResult>(mPriv->waitForEvent(timeout));
}


IasSmartX::IasResult IasSmartX::getNextEvent(IasEventPtr* event)
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasSmartX::IasResult>(mPriv->getNextEvent(event));
}


} // namespace IasAudio
