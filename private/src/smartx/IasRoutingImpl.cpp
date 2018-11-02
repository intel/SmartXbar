/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRoutingImpl.cpp
 * @date   2015
 * @brief
 */

#include "IasRoutingImpl.hpp"
#include "IasConfiguration.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "diagnostic/IasDiagnostic.hpp"

namespace IasAudio {

static const std::string cClassName = "IasRoutingImpl::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_PORT "port=" + name + ":"

IasRoutingImpl::IasRoutingImpl(IasConfigurationPtr config)
  :mLog(IasAudioLogging::registerDltContext("ROU", "SmartX Routing"))
  ,mConfig(config)
{
}

IasRoutingImpl::~IasRoutingImpl()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  for(auto it = mConnections.begin();it!=mConnections.end();)
  {
    disconnectSafe(it->first->getParameters()->id, it->second->getParameters()->id);
    it->first->clearSwitchMatrix();
    it->second->clearSwitchMatrix();
    it = mConnections.erase(it);
  }

  mConnections.clear();
  mDummySources.clear();
  mConfig = nullptr;
}

IasIRouting::IasResult IasRoutingImpl::connect(int32_t sourceId, int32_t sinkId)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Connect source Id", sourceId, "to sink Id", sinkId);
  IasAudioPortPtr outPort;
  IasAudioPortPtr inPort;
  IasConfiguration::IasResult cfgres = mConfig->getOutputPort(sourceId, &outPort);
  if (cfgres != IasConfiguration::eIasOk)
  {
    return eIasFailed;
  }
  cfgres = mConfig->getInputPort(sinkId, &inPort);
  if (cfgres != IasConfiguration::eIasOk)
  {
    return eIasFailed;
  }

  // check if sink is already in use
  if (inPort->isConnected())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "sink with id", inPort->getParameters()->id, "already connected");
    return eIasFailed;
  }
  // Even if the port is not connected, it is still possible that the port is listed in the active connections,
  // because it was not properly cleaned-up during IasSetupImpl::stopAudioSourceDevice.
  // In this case we simply remove it now, as we know it is not really connected anymore.
  IasConnectionMap::iterator toBeRemoved = mConnections.end();
  for (IasConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++)
  {
    if(it->second == inPort)
    {
      toBeRemoved = it;
      // If we found one connection to this input port, we can stop iterating here,
      // because there can be only one connection to an input port.
      break;
    }
  }
  if (toBeRemoved != mConnections.end())
  {
    mConnections.erase(toBeRemoved);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Cleaned-up outdated connection of source Id", sourceId, "to sink Id", sinkId);
  }

  IasAudioPortOwnerPtr sinkOwner;
  IasAudioPort::IasResult portres = inPort->getOwner(&sinkOwner);
  if (portres != IasAudioPort::eIasOk)
  {
    return eIasFailed;
  }

  IasSwitchMatrixPtr switchMatrix = sinkOwner->getSwitchMatrix();
  if (switchMatrix == nullptr)
  {
    std::string &name = inPort->getParameters()->name;
    /**
     * @log Sink doesn't have a switchMatrix. This can happen when the port is owned by a sink device instead of a routing zone.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "switchMatrix == nullptr");
    return eIasFailed;
  }

  std::set<int32_t>::iterator ret = mDummySources.find(sourceId);
  if (ret != mDummySources.end())
  {
    //we have to remove the dummy connection
    switchMatrix->dummyDisconnect(outPort);
    mDummySources.erase(sourceId);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Dummy connect for sourceId", sourceId, "removed to be able to do a real connection");
  }

  // Let the source output port keep in mind that it is connected with the current switch
  // matrix. Generate an error, if the source output port is already
  // connected to a different switch matrix (which belongs to an independent
  // routing zone).
  portres = inPort->storeConnection(switchMatrix);
  if (portres == IasAudioPort::eIasOk)
  {
    portres = outPort->storeConnection(switchMatrix);
  }
  if (portres != IasAudioPort::eIasOk)
  {
    const IasAudioPortParamsPtr outPortParams = outPort->getParameters();
    const IasAudioPortParamsPtr inPortParams  = inPort->getParameters();

    // We have to revert the connection for the inPort now, because the
    // storeConnection for the outPort did not work
    inPort->forgetConnection(switchMatrix);

    /**
     * @log Audio port <NAME> is already connected to an independent zone (different clock domain).
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot connect source output port", outPortParams->name,
                "to sink input port", inPortParams->name,
                ", since it is already connected to an independent zone");
    return eIasFailed;
  }
  std::set<int32_t> groupedSources = mConfig->findGroupedSourceIds(sourceId);
  std::set<int32_t>::iterator setIt;
  for (setIt=groupedSources.begin(); setIt!=groupedSources.end();++setIt)
  {
    if(*setIt != sourceId)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Found sourceId", *setIt, "logically grouped to sourceId", sourceId);
      IasAudioPortPtr tmpPort;
      mConfig->getOutputPort(*setIt,&tmpPort);
      IasConnectionPair myIt = mConnections.equal_range(tmpPort);
      if(myIt.first == myIt.second)
      {
        //no connection found for that grouped source -> check if there is a dummy connection
         DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Grouped sourceId", *setIt, "is not connected, so check for dummy connection");
        std::set<int32_t>::iterator ret = mDummySources.find(*setIt);
        if (ret == mDummySources.end())
        {
          //no dummy connection established so far, have to create a dummy connection
          IasSwitchMatrix::IasResult result = switchMatrix->dummyConnect(tmpPort,inPort);
          IAS_ASSERT(result == IasSwitchMatrix::eIasOk);
          (void)result;
          mDummySources.insert(*setIt);
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Dummy connect for sourceId", *setIt, "created");
        }
      }
    }
  }
  IasSwitchMatrix::IasResult result = switchMatrix->connect(outPort, inPort);
  if (result != IasSwitchMatrix::eIasOk)
  {
    return eIasFailed;
  }
  std::pair<IasAudioPortPtr,IasAudioPortPtr> tmpPair(outPort,inPort);
  mConnections.insert(tmpPair);

  // Start the diagnostic if a diagnostic stream was registered for one of the ports
  IasDiagnostic::getInstance()->startStream(outPort->getParameters()->name);
  IasDiagnostic::getInstance()->startStream(inPort->getParameters()->name);

  return eIasOk;
}

IasIRouting::IasResult IasRoutingImpl::disconnect(int32_t sourceId, int32_t sinkId)
{
  IasResult res = eIasOk;
  IasAudioPortPtr outPort;
  IasAudioPortPtr inPort;

  IasConfiguration::IasResult cfgres = mConfig->getOutputPort(sourceId, &outPort);
  if (cfgres != IasConfiguration::eIasOk)
  {
    return eIasFailed;
  }
  cfgres = mConfig->getInputPort(sinkId, &inPort);
  if (cfgres != IasConfiguration::eIasOk)
  {
    return eIasFailed;
  }
  IasConnectionPair myIt = mConnections.equal_range(outPort);
  IasConnectionMap::iterator tmpIt = mConnections.end();
  for (IasConnectionMap::iterator it=myIt.first; it!=myIt.second; it++)
  {
    if(it->second == inPort)
    {
      tmpIt = it;
      break;
    }
  }
  if (tmpIt != mConnections.end())
  {
    mConnections.erase(tmpIt);
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No connection between source",sourceId,"and sink",sinkId,"found!!!");
    return eIasFailed;
  }
  res = disconnectSafe(sourceId, sinkId);
  return res;
}

IasIRouting::IasResult IasRoutingImpl::disconnectSafe(int32_t sourceId, int32_t sinkId)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Disconnect source Id", sourceId, "from sink Id", sinkId);
  IasAudioPortPtr outPort;
  IasAudioPortPtr inPort;
  IasConfiguration::IasResult cfgres = mConfig->getOutputPort(sourceId, &outPort);
  if (cfgres != IasConfiguration::eIasOk)
  {
    return eIasFailed;
  }
  cfgres = mConfig->getInputPort(sinkId, &inPort);
  if (cfgres != IasConfiguration::eIasOk)
  {
    return eIasFailed;
  }
  // Stop the diagnostic if a diagnostic stream was registered for one of the ports
  IasDiagnostic::getInstance()->stopStream(outPort->getParameters()->name);
  IasDiagnostic::getInstance()->stopStream(inPort->getParameters()->name);

  IasAudioPortOwnerPtr sinkOwner;
  IasAudioPort::IasResult portres = inPort->getOwner(&sinkOwner);
  // If the port is registered at the configuration, we know it has an owner.
  if (portres != IasAudioPort::eIasOk)
  {
    return eIasFailed;
  }

  if (sinkOwner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "sink Port",inPort->getParameters()->name,"has no owner, can't continue disconnect");
    inPort->clearSwitchMatrix();
    return eIasFailed;
  }

  IasSwitchMatrixPtr switchMatrix = sinkOwner->getSwitchMatrix();
  if (switchMatrix == nullptr)
  {
    return eIasFailed;
  }

  // Let the source and sink ports forget that they were connected with the current switch matrix.
  portres = inPort->forgetConnection(switchMatrix);
  if (portres == IasAudioPort::eIasOk)
  {
    portres = outPort->forgetConnection(switchMatrix);
  }
  if (portres != IasAudioPort::eIasOk)
  {
    const IasAudioPortParamsPtr inPortParams  = inPort->getParameters();
    const IasAudioPortParamsPtr outPortParams = outPort->getParameters();

    /**
     * @log Connection does not exist.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot disconnect: source output port", outPortParams->name,
                "is not connected with sink input port", inPortParams->name);
    return eIasFailed;
  }

  //check if all other grouped ids are disconnected, too. If so , remove all dummy connections
  std::set<int32_t> groupedSources = mConfig->findGroupedSourceIds(sourceId);
  bool allGroupDisconnected = true;
  std::set<int32_t>::iterator setIt;
  for (setIt=groupedSources.begin(); setIt!=groupedSources.end(); ++setIt)
  {
    if(*setIt != sourceId)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Found sourceId ",*setIt, "logically grouped to sourceId ",sourceId);
      IasAudioPortPtr tmpPort;
      mConfig->getOutputPort(*setIt,&tmpPort);
      if(tmpPort->isConnected() == true)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Groupe sourceId ",*setIt, "still active connected");
        allGroupDisconnected = false;
      }
    }
  }

  if(allGroupDisconnected == true)
  {
    for (setIt=groupedSources.begin(); setIt!=groupedSources.end();++setIt)
    {
      if(*setIt != sourceId)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Removing dummy connection for sourceId ",*setIt);
        IasAudioPortPtr tmpPort;
        mConfig->getOutputPort(*setIt,&tmpPort);
        switchMatrix->dummyDisconnect(tmpPort);
        mDummySources.erase(*setIt);
      }
    }
  }
  else
  {
    //other grouped source are connected, so transform this into a dummy connection
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Convert real connection into dummy connection for sourceId",sourceId);
    IasAudioPortPtr tmpPort;
    mConfig->getOutputPort(sourceId,&tmpPort);
    switchMatrix->dummyConnect(tmpPort,inPort);
    mDummySources.insert(sourceId);
  }
  IasSwitchMatrix::IasResult result = switchMatrix->disconnect(outPort, inPort);
  if (result != IasSwitchMatrix::eIasOk)
  {
    return eIasFailed;
  }
  return eIasOk;
}

IasConnectionVector IasRoutingImpl::getActiveConnections() const
{

  IasConnectionVector connections;
  IasConnectionPortPair tempPair;
  IasConnectionMap::const_iterator mapIt;
  for ( mapIt=mConnections.begin();mapIt!=mConnections.end();mapIt++)
  {
    tempPair.first = mapIt->first;
    tempPair.second = mapIt->second;
    connections.push_back(tempPair);
  }
  return connections;
}

IasDummySourcesSet IasRoutingImpl::getDummySources() const
{
  return mDummySources;
}

} //namespace IasAudio
