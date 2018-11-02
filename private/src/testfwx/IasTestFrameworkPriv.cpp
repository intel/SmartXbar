/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkPriv.cpp
 * @date   2017
 * @brief  The IasTestFramework private implementation class.
 */

#include "testfwx/IasTestFrameworkPriv.hpp"
#include "testfwx/IasTestFrameworkSetupImpl.hpp"
#include "testfwx/IasTestFrameworkConfiguration.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"
#include "testfwx/IasTestFrameworkRoutingZone.hpp"
#include "smartx/IasProcessingImpl.hpp"
#include "smartx/IasDebugImpl.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "audio/smartx/IasEventProvider.hpp"

#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "model/IasPipeline.hpp"


namespace IasAudio {


static const std::string cClassName = "IasTestFrameworkPriv::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasTestFrameworkPriv::IasTestFrameworkPriv()
  :mLog(IasAudioLogging::registerDltContext("TESTFWX", "Test Framework Priv"))
  ,mConfig(nullptr)
  ,mSetup(nullptr)
  ,mProcessing(nullptr)
  ,mDebug(nullptr)
  ,mCmdDispatcher(nullptr)
{
}


IasTestFrameworkPriv::~IasTestFrameworkPriv()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);

  IasEventProvider::getInstance()->clearEventQueue();

  delete mSetup;
  delete mProcessing;
  mCmdDispatcher = nullptr;
  if (mConfig != nullptr && mConfig->getTestFrameworkPipeline() != nullptr)
  {
    mConfig->getTestFrameworkPipeline()->clearConfig();
  }
  mConfig = nullptr;
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::init(const IasPipelineParams &pipelineParams)
{
  mConfig = std::make_shared<IasTestFrameworkConfiguration>();
  IAS_ASSERT(mConfig != nullptr);

  mCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IAS_ASSERT(mCmdDispatcher != nullptr);

  IasResult result = createTestFrameworkRoutingZone();
  if (result != eIasOk)
  {
    return eIasFailed;
  }

  result = createPipeline(pipelineParams);
  if (result != eIasOk)
  {
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::createTestFrameworkRoutingZone()
{
  IAS_ASSERT(mConfig != nullptr);

  IasTestFrameworkRoutingZoneParamsPtr params = nullptr;
  params = std::make_shared<IasTestFrameworkRoutingZoneParams>("TestFrameworkRoutingZone");

  IasTestFrameworkRoutingZonePtr routingZone = nullptr;
  routingZone = std::make_shared<IasTestFrameworkRoutingZone>(params);

  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to create test framework routing zone");
    return eIasFailed;
  }

  IasTestFrameworkRoutingZone::IasResult rtzres = routingZone->init();
  if (rtzres != IasTestFrameworkRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Routing zone init failed with error code ", toString(rtzres));
    return eIasFailed;
  }

  IasConfiguration::IasResult cfgres = mConfig->addTestFrameworkRoutingZone(routingZone);
  if (cfgres != IasConfiguration::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to add test framework routing zone to configuration");
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::createPipeline(const IasPipelineParams &pipelineParams)
{
  IAS_ASSERT(mConfig != nullptr);
  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "createPipeline");

  if (pipelineParams.name.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline name is not specified");
    return eIasFailed;
  }
  if (pipelineParams.periodSize == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: periodSize is 0");
    return eIasFailed;
  }
  if (pipelineParams.samplerate == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: samplerate is 0");
    return eIasFailed;
  }

  //Create plugin engine
  IasPluginEnginePtr pluginEngine = std::make_shared<IasPluginEngine>(mCmdDispatcher);
  IAS_ASSERT(pluginEngine != nullptr);

  IasAudioProcessingResult res = pluginEngine->loadPluginLibraries();
  if (res != eIasAudioProcOK)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No plug-in libraries found. Pipeline will not be created");
    return eIasFailed;
  }

  IasPipelineParamsPtr pipelineParamsPtr = std::make_shared<IasPipelineParams>();
  IAS_ASSERT(pipelineParamsPtr != nullptr);
  *pipelineParamsPtr = pipelineParams;

  IasPipelinePtr pipeline = std::make_shared<IasPipeline>(pipelineParamsPtr, pluginEngine, mConfig);
  IAS_ASSERT(pipeline != nullptr);

  IasPipeline::IasResult pplres = pipeline->init();
  if(pplres != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to initialize pipeline");
    return eIasFailed;
  }

  IasConfiguration::IasResult cfgres = mConfig->addTestFrameworkPipeline(pipeline);
  if (cfgres != IasConfiguration::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to add test framework pipeline to mConfig");
    return eIasFailed;
  }

  // Add the pipeline to the routing zone.
  IasTestFrameworkRoutingZonePtr routingZone = mConfig->getTestFrameworkRoutingZone();
  IAS_ASSERT(routingZone != nullptr);

  IasTestFrameworkRoutingZone::IasResult rtzres = routingZone->addPipeline(pipeline);
  if (rtzres != IasTestFrameworkRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasTestFrameworkRoutingZone::addPipeline:", toString(rtzres));
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::start()
{
  IAS_ASSERT(mConfig != nullptr);

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  IasPipeline::IasResult pipelineResult = pipeline->initAudioChain();
  if (pipelineResult != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::initAudioChain:", toString(pipelineResult));
    return eIasFailed;
  }

  const IasTestFrameworkRoutingZonePtr routingZone = mConfig->getTestFrameworkRoutingZone();
  IAS_ASSERT(routingZone != nullptr);

  IasTestFrameworkRoutingZone::IasResult res = routingZone->start();
  if (res != IasTestFrameworkRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to start test framework routing zone, error ", toString(res));
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::stop()
{
  IAS_ASSERT(mConfig != nullptr);

  const IasTestFrameworkRoutingZonePtr routingZone = mConfig->getTestFrameworkRoutingZone();
  IAS_ASSERT(routingZone != nullptr);

  IasTestFrameworkRoutingZone::IasResult res = routingZone->stop();
  if (res != IasTestFrameworkRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to stop test framework routing zone, error ", toString(res));
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::process(uint32_t numPeriods)
{
  IAS_ASSERT(mConfig != nullptr);

  const IasTestFrameworkRoutingZonePtr routingZone = mConfig->getTestFrameworkRoutingZone();
  IAS_ASSERT(routingZone != nullptr);

  IasTestFrameworkRoutingZone::IasResult res = routingZone->processPeriods(numPeriods);
  if (res == IasTestFrameworkRoutingZone::eIasFinished)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing of all input files finished successfully.");
    return eIasFinished;
  }
  else if (res != IasTestFrameworkRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Processing of", numPeriods, "periods failed with error ", toString(res));
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkSetup* IasTestFrameworkPriv::setup()
{
  if (mSetup == nullptr)
  {
    IAS_ASSERT(mConfig != nullptr);

    mSetup = new IasTestFrameworkSetupImpl(mConfig);
    IAS_ASSERT(mSetup != nullptr);
  }

  return mSetup;
}


IasIProcessing* IasTestFrameworkPriv::processing()
{
  if (mProcessing == nullptr)
  {
    IAS_ASSERT(mCmdDispatcher != nullptr);

    mProcessing = new IasProcessingImpl(mCmdDispatcher);
    IAS_ASSERT(mProcessing != nullptr);
  }

  return mProcessing;
}

IasIDebug* IasTestFrameworkPriv::debug()
{
  if (mDebug == nullptr)
  {
    mDebug = new IasDebugImpl(mConfig);
    IAS_ASSERT(mDebug != nullptr);
  }
  return mDebug;
}

static IasTestFrameworkPriv::IasResult translate(IasEventProvider::IasResult result)
{
  if (result == IasEventProvider::eIasOk)
  {
    return IasTestFrameworkPriv::eIasOk;
  }
  else if (result == IasEventProvider::eIasTimeout)
  {
    return IasTestFrameworkPriv::eIasTimeout;
  }
  else if (result == IasEventProvider::eIasNoEventAvailable)
  {
    return IasTestFrameworkPriv::eIasNoEventAvailable;
  }
  else
  {
    return IasTestFrameworkPriv::eIasFailed;
  }
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::waitForEvent(uint32_t timeout) const
{
  IasEventProvider *eventProvider =  IasEventProvider::getInstance();
  IAS_ASSERT(eventProvider != nullptr);

  return translate(eventProvider->waitForEvent(timeout));
}


IasTestFrameworkPriv::IasResult IasTestFrameworkPriv::getNextEvent(IasEventPtr* event)
{
  IasEventProvider *eventProvider =  IasEventProvider::getInstance();
  IAS_ASSERT(eventProvider != nullptr);

  return translate(eventProvider->getNextEvent(event));
}

} // namespace IasAudio
