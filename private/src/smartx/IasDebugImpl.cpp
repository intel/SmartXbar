/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDebugImpl.cpp
 * @date   2016
 * @brief
 */

#include "IasDebugImpl.hpp"
#include "model/IasRoutingZone.hpp"
#include "model/IasRoutingZoneWorkerThread.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

#include "switchmatrix/IasSwitchMatrix.hpp"
#include "smartx/IasSmartXClient.hpp"



#include <string.h>
#include <atomic>

#ifndef RW_TMP_PATH
#define RW_TMP_PATH "/tmp/"
#endif

namespace IasAudio {

IasIDebug::IasResult IasDebugImpl::translate(IasPipeline::IasResult result)
{
  switch (result)
  {
    case IasPipeline::eIasOk:
      return IasIDebug::eIasOk;
    default:
      return IasIDebug::eIasFailed;
  }
}

IasIDebug::IasResult IasDebugImpl::translate(IasConfiguration::IasResult result)
{
  switch (result)
  {
    case IasConfiguration::eIasOk:
      return IasIDebug::eIasOk;
    default:
      return IasIDebug::eIasFailed;
  }
}

static const uint32_t cMaxUserFileNameLength = 247;
static const std::string cClassName = "IasDebugImpl::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_PORT "port=" + name + ":"


IasDebugImpl::IasDebugImpl(IasConfigurationPtr config)
 :mLog(IasAudioLogging::registerDltContext("DBG", "SmartX Debug"))
 ,mConfig(config)
{
}

IasDebugImpl::~IasDebugImpl()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  mConfig = nullptr;
}

IasIDebug::IasResult IasDebugImpl::stopProbing(const std::string &location)
{
  IasAudioPort::IasResult portres;
  IasAudioPortPtr port;
  IasAudioPinPtr pin;
  IasAudioPortOwnerPtr portOwner;

  IasConfiguration::IasResult cfgres = mConfig->getPortByName(location, &port);
  if (cfgres == IasConfiguration::eIasObjectNotFound)
  {
    // no port found, maybe we can find a pin?
    cfgres = mConfig->getPinByName(location, &pin);
  }
  if (cfgres != IasConfiguration::eIasOk)
  {
    //even no pin was found, so return error
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,"Neither port or pin was found to stop probing");
    return eIasFailed;
  }
  else
  {
    if(port != nullptr)
    {
      IasAudioPortParamsPtr portParams = port->getParameters();

      portres = port->getOwner(&portOwner);
      IAS_ASSERT(portOwner != nullptr);
      (void)portres;

      IasProbeLocation probeLocation = getProbeLocation(portParams);

      if (probeLocation == eIasProbeSwitchMatrix)
      {
        stopSwitchMatrixProbing(port,portOwner);
      }
      else
      {
        stopRoutingZoneProbing(portOwner);
      }
    }
    else
    {
      IasPipelinePtr pipeline = pin->getPipeline();
      if( pipeline != nullptr)
      {
        pipeline->stopPinProbing(pin);
      }
    }
  }
  return eIasOk;
}

IasIDebug::IasResult IasDebugImpl::startInject(const std::string &fileNamePrefix,
                                               const std::string &location,
                                               uint32_t numSeconds)
{
  IasIDebug::IasResult res = startProbing(fileNamePrefix,location,numSeconds,true);
  return res;
}

IasIDebug::IasResult IasDebugImpl::startRecord(const std::string &fileNamePrefix,
                                               const std::string &location,
                                               uint32_t numSeconds)
{
  IasIDebug::IasResult res = startProbing(fileNamePrefix,location,numSeconds,false);
  return res;
}

IasIDebug::IasResult IasDebugImpl::startProbing(const std::string &fileNamePrefix,
                                                const std::string &location,
                                                uint32_t numSeconds,
                                                bool inject)
{
  IasResult res = eIasOk;
  if(*(fileNamePrefix.begin()) == '/')
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File name prefix must not start with /");
    return eIasFailed;
  }
  std::string prefix = RW_TMP_PATH;
  prefix.append(fileNamePrefix);
  if(prefix.size() > cMaxUserFileNameLength)
  {
    prefix.resize(cMaxUserFileNameLength); // to be able to add "_chxx.wav" to string, which are 9 characters, to guarantee that name will not
    // exceed 256 bytes
  }

  IasAudioPortPtr port = nullptr;
  IasAudioPinPtr pin = nullptr;

  IasConfiguration::IasResult cfgres = mConfig->getPortByName(location, &port);
  if (cfgres == IasConfiguration::eIasObjectNotFound)
  {
    // no port found, maybe we can find a pin?
    cfgres = mConfig->getPinByName(location, &pin);
  }
  if (cfgres != IasConfiguration::eIasOk)
  {
    //even no pin was found, so return error
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "neither port or pin was found for probing");
    return translate(cfgres);
  }

  if(port != nullptr)
  {
    IasAudioPortParamsPtr portParams = port->getParameters();
    IAS_ASSERT(portParams != nullptr);

    IasAudioPortOwnerPtr portOwner;
    IasAudioPort::IasResult portres = port->getOwner(&portOwner);
    //we do not need to check for error. Via smartx api you can not add a port to configuration without owner
    IAS_ASSERT(portres == IasAudioPort::eIasOk);
    IAS_ASSERT(portOwner != nullptr);
    (void)portres;

    //we do not need to check for error. Via smartx api you can not create a port owner with invalid sample rate
    uint32_t sampleRate = portOwner->getSampleRate();
    IAS_ASSERT(sampleRate != 0);
    IasProbeLocation probeLocation = getProbeLocation(portParams);

    if(probeLocation == eIasProbeSwitchMatrix)
    {
      res = startSwitchMatrixProbing(port, portOwner, prefix, inject, numSeconds, sampleRate);
    }
    else
    {
      res = startRoutingZoneProbing(portParams, portOwner, prefix, inject, numSeconds);
    }
    return res;
  }
  else
  {
    IasPipelinePtr pipeline = pin->getPipeline();
    if( pipeline == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,"Pin was not added to any pipeline, no probing possible");
      return eIasFailed;
    }
    res = translate(pipeline->startPinProbing(pin,prefix,numSeconds,inject));
    return res;
  }
}


IasDebugImpl::IasProbeLocation IasDebugImpl::getProbeLocation(IasAudioPortParamsPtr portParams)
{
   if (portParams->direction == eIasPortDirectionInput)
   {
     if (portParams->id == -1)
     {
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Probing is in routing zone for input port");
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Port:", portParams->name.c_str(), "; id:", portParams->id);
       return eIasProbeRoutingZone;
     }
     else
     {
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Probing is in switchmatrix for input port");
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Port:", portParams->name.c_str(), "; id:", portParams->id);
       return eIasProbeSwitchMatrix;
     }
   }
   else
   {
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Probing is in switchmatrix for output port");
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Port:", portParams->name.c_str(), "; id:", portParams->id);
       return eIasProbeSwitchMatrix;
   }
}

void IasDebugImpl::stopSwitchMatrixProbing(IasAudioPortPtr port,
                                           IasAudioPortOwnerPtr portOwner)
{
  IasSwitchMatrixPtr switchMatrix = nullptr;

  if(port->getParameters()->direction == eIasPortDirectionOutput)
  {
    switchMatrix = port->getSwitchMatrix();
  }
  else
  {
    switchMatrix = portOwner->getSwitchMatrix();
  }

  if (switchMatrix == nullptr)
  {
    const std::string &name = port->getParameters()->name;
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Probing cannot be stopped, Switch Matrix is null pointer, maybe it was not even started?");
    return;
  }

  switchMatrix->stopProbing(port);
}

IasDebugImpl::IasResult IasDebugImpl::startSwitchMatrixProbing(IasAudioPortPtr port,
                                                               IasAudioPortOwnerPtr portOwner,
                                                               const std::string &prefix,
                                                               bool inject,
                                                               uint32_t numSeconds,
                                                               uint32_t sampleRate)
{
  IasDebugImpl::IasResult res = eIasOk;
  IasSwitchMatrixPtr switchMatrix = nullptr;
  if(port->getParameters()->direction == eIasPortDirectionOutput)
  {
    switchMatrix = port->getSwitchMatrix();
  }
  else
  {
    switchMatrix = portOwner->getSwitchMatrix();
  }
  if (switchMatrix == nullptr)
  {
    const std::string &name = port->getParameters()->name;
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Probing cannot be started, as the port is not connected yet.");
    return eIasFailed;
  }

  IasAudioRingBuffer* ringbuf = nullptr;
  IasAudioPort::IasResult portres = port->getRingBuffer(&ringbuf);
  IAS_ASSERT(ringbuf != nullptr);
  (void)portres;
  if(switchMatrix->startProbing(port, prefix, inject, numSeconds, ringbuf, sampleRate))
  {
    res = eIasFailed;
  }
  return res;
}

void IasDebugImpl::stopRoutingZoneProbing(IasAudioPortOwnerPtr portOwner)
{
  IasRoutingZonePtr rZone = portOwner->getRoutingZone();
  IAS_ASSERT(rZone != nullptr);
  IasRoutingZoneWorkerThreadPtr rzWorker = rZone->getWorkerThread();
  IAS_ASSERT(rzWorker != nullptr);

  rzWorker->stopProbing();
}

IasDebugImpl::IasResult IasDebugImpl::startRoutingZoneProbing(IasAudioPortParamsPtr portParams,
                                                              IasAudioPortOwnerPtr portOwner,
                                                              const std::string &prefix,
                                                              bool inject,
                                                              uint32_t numSeconds)
{
  IasDebugImpl::IasResult res = eIasOk;
  IasRoutingZonePtr rZone = portOwner->getRoutingZone();
  //should not be possible to create a port without owner
  IAS_ASSERT(rZone != nullptr);
  IasRoutingZoneWorkerThreadPtr rzWorker = rZone->getWorkerThread();
  //should not be possible that the zone has no worker thread
  IAS_ASSERT(rzWorker != nullptr);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Start probing");
  if(rzWorker->startProbing(prefix, inject, numSeconds, portParams->index, portParams->numChannels))
  {
    res = eIasFailed;
  }
  return res;
}

} //namespace IasAudio
