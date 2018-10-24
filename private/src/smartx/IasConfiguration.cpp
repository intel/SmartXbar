/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfiguration.cpp
 * @date   2015
 * @brief
 */

#include "IasConfiguration.hpp"

#include "model/IasAudioPort.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasRoutingZone.hpp"

#include "model/IasProcessingModule.hpp"
#include "audio/smartx/IasProperties.hpp"

namespace IasAudio {

static const std::string cClassName = "IasConfiguration::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + name + ":"
#define LOG_ZONE "zone=" + name + ":"

IasConfiguration::IasConfiguration()
  :mLog(IasAudioLogging::registerDltContext("CFG", "SmartX Config"))
  ,mSourceDeviceMap()
  ,mSinkDeviceMap()
  ,mRoutingZoneMap()
  ,mPipelineMap()
  ,mOutputPortMap()
  ,mInputPortMap()
  ,mLogicalSourceMap()
{
}

IasConfiguration::~IasConfiguration()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  mSourceDeviceMap.clear();
  mSinkDeviceMap.clear();

  for (auto &entry : mRoutingZoneMap)
  {
    IasRoutingZonePtr routingZone = entry.second;
    (void)routingZone->stop();
  }
  mRoutingZoneMap.clear();

  mPipelineMap.clear();

  for (auto &entry : mOutputPortMap)
  {
    IasAudioPortPtr port = entry.second;
    port->clearOwner();
  }
  mOutputPortMap.clear();

  for (auto &entry : mInputPortMap)
  {
    IasAudioPortPtr port = entry.second;
    port->clearOwner();
  }
  mInputPortMap.clear();
}

IasConfiguration::IasResult IasConfiguration::addAudioDevice(const std::string& name, IasAudioSourceDevicePtr sourceDevice)
{
  if (mSourceDeviceMap.count(name) > 0)
  {
    /**
     * @log Source device names have to be unique.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Audio source device already exists");
    return eIasObjectAlreadyExists;
  }
  mSourceDeviceMap[name] = sourceDevice;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Audio source device added");
  return eIasOk;
}

IasConfiguration::IasResult IasConfiguration::getAudioDevice(const std::string& name, IasAudioSourceDevicePtr* sourceDevice)
{
  // You cannot pass a nullptr. The compiler prevents this because we have an overloaded function
  // which would be ambiguous.
  IAS_ASSERT(sourceDevice != nullptr);
  if (mSourceDeviceMap.count(name) > 0)
  {
    *sourceDevice = mSourceDeviceMap.at(name);
    return eIasOk;
  }
  else
  {
    return eIasObjectNotFound;
  }
}

void IasConfiguration::deleteAudioDevice(const std::string& name)
{
  if (mSourceDeviceMap.count(name) > 0)
  {
    mSourceDeviceMap.erase(name);
  }
  if (mSinkDeviceMap.count(name) > 0)
  {
    mSinkDeviceMap.erase(name);
  }
}

IasConfiguration::IasResult IasConfiguration::addAudioDevice(const std::string& name, IasAudioSinkDevicePtr sinkDevice)
{
  if (mSinkDeviceMap.count(name) > 0)
  {
    /**
     * @log Sink device names have to be unique.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Audio sink device already exists");
    return eIasObjectAlreadyExists;
  }
  mSinkDeviceMap[name] = sinkDevice;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Audio sink device added");
  return eIasOk;
}

IasConfiguration::IasResult IasConfiguration::getAudioDevice(const std::string& name, IasAudioSinkDevicePtr* sinkDevice)
{
  // You cannot pass a nullptr. The compiler prevents this because we have an overloaded function
  // which would be ambiguous.
  IAS_ASSERT(sinkDevice != nullptr);
  if (mSinkDeviceMap.count(name) > 0)
  {
    *sinkDevice = mSinkDeviceMap.at(name);
    return eIasOk;
  }
  else
  {
    return eIasObjectNotFound;
  }
}

IasConfiguration::IasResult IasConfiguration::addRoutingZone(const std::string& name, IasRoutingZonePtr routingZone)
{
  if (mRoutingZoneMap.count(name) > 0)
  {
    /**
     * @log Routing zone names have to be unique.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Routing zone already exists");
    return eIasObjectAlreadyExists;
  }
  mRoutingZoneMap[name] = routingZone;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Routing zone added");
  return eIasOk;
}

IasConfiguration::IasResult IasConfiguration::getRoutingZone(const std::string& name, IasRoutingZonePtr* routingZone)
{
  if (routingZone == nullptr)
  {
    return eIasNullPointer;
  }
  if (mRoutingZoneMap.count(name) > 0)
  {
    *routingZone = mRoutingZoneMap.at(name);
    return eIasOk;
  }
  else
  {
    return eIasObjectNotFound;
  }
}

IasConfiguration::IasResult IasConfiguration::getRoutingZone(const IasPipelinePtr pipeline, IasRoutingZonePtr *routingZone)
{
  if (routingZone == nullptr || pipeline == nullptr)
  {
    return eIasNullPointer;
  }

  for(auto& it : mRoutingZoneMap)
  {
    if (it.second->hasPipeline(pipeline))
    {
      *routingZone = it.second;
      return eIasOk;
    }
  }

  return eIasObjectNotFound;
}

void IasConfiguration::deleteRoutingZone(const std::string& name)
{
  if (mRoutingZoneMap.count(name) > 0)
  {
    mRoutingZoneMap.erase(name);
  }
}

IasConfiguration::IasResult IasConfiguration::addPipeline(const std::string& name, IasPipelinePtr pipeline)
{
  if (mPipelineMap.count(name) > 0)
  {
    /**
     * @log Pipeline names have to be unique.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pipeline", name, "already exists");
    return eIasObjectAlreadyExists;
  }
  mPipelineMap[name] = pipeline;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline", name, "added");
  return eIasOk;
}

IasConfiguration::IasResult IasConfiguration::getPipeline(const std::string& name, IasPipelinePtr* pipeline)
{
  if (pipeline == nullptr)
  {
    return eIasNullPointer;
  }
  if (mPipelineMap.count(name) > 0)
  {
    *pipeline = mPipelineMap.at(name);
    return eIasOk;
  }
  else
  {
    return eIasObjectNotFound;
  }
}

void IasConfiguration::deletePipeline(const std::string& name)
{
  if (mPipelineMap.count(name) > 0)
  {
    mPipelineMap.erase(name);
  }
}

uint32_t IasConfiguration::getNumberSourceDevices() const
{
  return static_cast<uint32_t>(mSourceDeviceMap.size());
}

uint32_t IasConfiguration::getNumberSinkDevices() const
{
  return static_cast<uint32_t>(mSinkDeviceMap.size());
}

uint32_t IasConfiguration::getNumberRoutingZones() const
{
  return static_cast<uint32_t>(mRoutingZoneMap.size());
}

uint32_t IasConfiguration::getNumberPipelines() const
{
  return static_cast<uint32_t>(mPipelineMap.size());
}

IasConfiguration::IasResult IasConfiguration::addPin(IasAudioPinPtr pin)
{
  if (pin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "pin == nullptr");
    return eIasFailed;
  }

  std::string name = pin->getParameters()->name;
  if (mPinMap.find(name) == mPinMap.end())
  {
    mPinMap[name] = pin;
  }
  return eIasOk;
}

void IasConfiguration::removePin(std::string name)
{

  if (mPinMap.find(name) != mPinMap.end())
  {
    mPinMap.erase(name);
  }
}

IasConfiguration::IasResult IasConfiguration::addPort(IasAudioPortPtr port)
{
  if (port == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Port == nullptr");
    return eIasFailed;
  }
  int32_t id = port->getParameters()->id;

  std::string name = port->getParameters()->name;
  if (mPortMap.find(name) == mPortMap.end())
  {
    mPortMap[name] = port;
  }

  // If the port ID is specified (i.e., if it is not -1), add the port to the Port Map.
  if (id >= 0)
  {
    IasPortDirection direction = port->getParameters()->direction;
    std::string directionStr;
    IasIdPortMap *portMap;
    if (direction == eIasPortDirectionInput)
    {
      directionStr = "Input";
      portMap = &mInputPortMap;
    }
    else
    {
      directionStr = "Output";
      portMap = &mOutputPortMap;
    }
    IAS_ASSERT(portMap != nullptr);
    if (portMap->find(id) != portMap->end())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, directionStr, "Port with Id", id, "already exists");
      return eIasFailed;
    }
    (*portMap)[id] = port;
  }
  return eIasOk;
}

IasConfiguration::IasResult IasConfiguration::getPinByName(const std::string &name, IasAudioPinPtr *pin)
{
  if(pin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "pin == nullptr");
    return eIasFailed;
  }
  bool bPinNameFound = false;
  for(auto& it : mPinMap)
  {
    std::string nameFound = it.second->getParameters()->name;
    if(nameFound.compare(name) == 0)
    {
      bPinNameFound = true;
      *pin = it.second;
      break;
    }
  }
  if(bPinNameFound == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pin with name", name.c_str(), "doesn't exist");
    return eIasObjectNotFound;
  }
  else
  {
    return eIasOk;
  }
}

IasConfiguration::IasResult IasConfiguration::getPortByName(const std::string &name, IasAudioPortPtr *port)
{
  if (port == nullptr)
  {
    /**
     * @log Location where to put return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "port == nullptr");
    return eIasFailed;
  }

  bool bPortNameFound = false;

  for(auto& it : mPortMap)
  {
    std::string nameFound = it.second->getParameters()->name;
    if(nameFound.compare(name) == 0)
    {
      bPortNameFound = true;
      *port = it.second;
      break;
    }
  }

  if(bPortNameFound == false)
  {
    /**
     * @log Port with name doesn't exist.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Port with name", name.c_str(), "doesn't exist");
    return eIasObjectNotFound;
  }
  else
  {
    return eIasOk;
  }
}

IasConfiguration::IasResult IasConfiguration::getOutputPort(int32_t sourceId, IasAudioPortPtr* port)
{
  if (port == nullptr)
  {
    /**
     * @log Location where to put return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Port == nullptr");
    return eIasFailed;
  }
  if (mOutputPortMap.find(sourceId) == mOutputPortMap.end())
  {
    /**
     * @log Port with sourceId <ID> doesn't exist.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Port with sourceId", sourceId, "doesn't exist");
    return eIasFailed;
  }
  *port = mOutputPortMap[sourceId];
  return eIasOk;
}

void IasConfiguration::deleteOutputPort(int32_t sourceId)
{
  if (sourceId >= 0)
  {
    IasIdPortMap::iterator it = mOutputPortMap.find(sourceId);
    if(it == mOutputPortMap.end())
    {
      /**
       * @log Output port with <ID> doesn't exist.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Couldn't find output port with sourceId", sourceId);
      return;
    }
    std::string portName = it->second->getParameters()->name;
    uint64_t numDeleted =  mPortMap.erase(portName);
    if(numDeleted != 1)
    {
      /**
       * @log Could not delete port with <ID> from global port list.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Couldn't delete port with sourceId from global port list", sourceId);
      return;
    }
    mOutputPortMap.erase(sourceId); //should work, cause we checked above that sourceId can be found in this map
  }
}

IasConfiguration::IasResult IasConfiguration::getInputPort(int32_t sinkId, IasAudioPortPtr* port)
{
  if (port == nullptr)
  {
    /**
     * @log Destination for return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Port == nullptr");
    return eIasFailed;
  }
  if (mInputPortMap.find(sinkId) == mInputPortMap.end())
  {
    /**
     * @log Port with sinkId <ID> doesn't exist.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Port with sinkId", sinkId, "doesn't exist");
    return eIasFailed;
  }
  *port = mInputPortMap[sinkId];
  return eIasOk;
}

void IasConfiguration::deleteInputPort(int32_t sinkId)
{
  if (sinkId >= 0)
  {
    IasIdPortMap::iterator it = mInputPortMap.find(sinkId);
    if(it == mInputPortMap.end())
    {
      /**
       * @log Output port with <ID> doesn't exist.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Couldn't find input port with sinkId", sinkId);
      return;
    }
    std::string portName = it->second->getParameters()->name;
    uint64_t numDeleted =  mPortMap.erase(portName);
    if(numDeleted != 1)
    {
      /**
       * @log Could not delete port with <ID> from global port list.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Couldn't delete port with sinkId from global port list", sinkId);
      return;
    }
    mInputPortMap.erase(sinkId); //should work, cause we checked above that sourceId can be found in this map
  }
}

void IasConfiguration::deletePortByName(std::string name)
{
  IasPortMap::iterator it = mPortMap.find(name);
  if(it == mPortMap.end())
  {
    /**
     * @log Could not delete port with <name> from global port list.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Couldn't delete port with name",name.c_str(), "from global port list");
    return;
  }
  mPortMap.erase(name);

}

void IasConfiguration::addLogicalSource(std::string name, int32_t id)
{
  IasLogicalSourceMap::iterator it;
  std::set<int32_t> tempSet;
  it=mLogicalSourceMap.find(name);

  if (it == mLogicalSourceMap.end())
  {
    tempSet.insert(id);
    std::pair<std::string,std::set<int32_t>> tmpPair(name,tempSet);
    mLogicalSourceMap.insert(tmpPair);
  }
  else
  {
    it->second.insert(id);
  }

}

std::set<int32_t> IasConfiguration::findGroupedSourceIds(int32_t sourceId)
{
  std::set<int32_t> groupedSources;
  std::set<int32_t>::iterator setIt;
  IasLogicalSourceMap::iterator it;

  for(it=mLogicalSourceMap.begin();it!=mLogicalSourceMap.end();it++)
  {
    for (setIt=it->second.begin(); setIt!=it->second.end(); ++setIt)
    {
      if(*setIt == sourceId)
      {
        return it->second;
      }
    }
  }
  return groupedSources;
}

IasPropertiesPtr IasConfiguration::getPropertiesForModule(IasProcessingModulePtr module)
{
  IAS_ASSERT(module != nullptr);
  auto entry = mModulePropertiesMap.find(module);
  if (entry != mModulePropertiesMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Return existing properties for module", module->getParameters()->instanceName);
    return entry->second;
  }
  else
  {
    IasPropertiesPtr newProperties = std::make_shared<IasProperties>();
    IAS_ASSERT(newProperties != nullptr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Created new properties for module", module->getParameters()->instanceName);
    return newProperties;
  }
}

void IasConfiguration::setPropertiesForModule(IasProcessingModulePtr module, const IasProperties &properties)
{
  IAS_ASSERT(module != nullptr);
  auto entry = mModulePropertiesMap.find(module);
  if (entry != mModulePropertiesMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Existing properties for module", module->getParameters()->instanceName, "will be overwritten");
    *entry->second = properties;
  }
  else
  {
    IasPropertiesPtr newProperties = std::make_shared<IasProperties>(properties);
    IAS_ASSERT(newProperties != nullptr);
    mModulePropertiesMap[module] = newProperties;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Created new properties for module", module->getParameters()->instanceName);
  }

}


} //namespace IasAudio
