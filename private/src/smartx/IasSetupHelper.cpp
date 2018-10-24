/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSetupHelper.cpp
 * @date   2015
 * @brief
 */

#include <string>

#include "audio/smartx/IasSetupHelper.hpp"
#include "model/IasAudioDevice.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasRoutingZone.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

static const std::string cClassName = "IasSetupHelper::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

namespace IasSetupHelper {

IasISetup::IasResult createAudioSourceDevice(IasISetup *setup, const IasAudioDeviceParams &params, int32_t id, IasAudioSourceDevicePtr *audioSourceDevice)
{
  if (setup == nullptr || audioSourceDevice == nullptr)
  {
    return IasISetup::eIasFailed;
  }

  // Create an audio source
  IasAudioSourceDevicePtr source = nullptr;
  IasISetup::IasResult result = setup->createAudioSourceDevice(params, &source);
  if (result != IasISetup::eIasOk)
  {
    return result;
  }

  // Create source port and add to source device
  IasAudioPortParams sourcePortParams =
  {
    params.name + "_port",
    params.numChannels,
    id,
    eIasPortDirectionOutput,
    0
  };
  IasAudioPortPtr sourcePort = nullptr;
  result = setup->createAudioPort(sourcePortParams, &sourcePort);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->addAudioOutputPort(source, sourcePort);
  IAS_ASSERT(result == IasISetup::eIasOk);
  *audioSourceDevice = source;
  return result;
}

IasISetup::IasResult createAudioSinkDevice(IasISetup *setup, const IasAudioDeviceParams& params, int32_t id, IasAudioSinkDevicePtr* audioSinkDevice, IasRoutingZonePtr* routingZone)
{
  if (setup == nullptr || audioSinkDevice == nullptr || routingZone == nullptr)
  {
    return IasISetup::eIasFailed;
  }
  // Create audio sink
  IasAudioSinkDevicePtr sink = nullptr;
  IasISetup::IasResult result = setup->createAudioSinkDevice(params, &sink);
  if (result != IasISetup::eIasOk)
  {
    return result;
  }

  // Create sink port and add to sink device
  IasAudioPortParams sinkPortParams =
  {
    params.name + "_port",
    params.numChannels,
    -1,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr sinkPort = nullptr;
  result = setup->createAudioPort(sinkPortParams, &sinkPort);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->addAudioInputPort(sink, sinkPort);
  IAS_ASSERT(result == IasISetup::eIasOk);

  // Create a routing zone
  IasRoutingZoneParams routingZoneParams =
  {
    params.name + "_rz"
  };
  IasRoutingZonePtr myRoutingZone = nullptr;
  result = setup->createRoutingZone(routingZoneParams, &myRoutingZone);
  IAS_ASSERT(result == IasISetup::eIasOk);

  // Link the routing zone with the sink
  result = setup->link(myRoutingZone, sink);
  IAS_ASSERT(result == IasISetup::eIasOk);
  *audioSinkDevice = sink;
  *routingZone = myRoutingZone;

  // Create a zone port
  IasAudioPortParams zonePortParams =
  {
    params.name + "_rznport",
    params.numChannels,
    id,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr zonePort = nullptr;
  result = setup->createAudioPort(zonePortParams, &zonePort);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->addAudioInputPort(myRoutingZone, zonePort);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->link(zonePort, sinkPort);
  return result;
}


IasISetup::IasResult createAudioSinkDevice(IasISetup *setup,
                                           const IasAudioDeviceParams& params,
                                           const std::vector<int32_t>& idVector,
                                           const std::vector<uint32_t>& numChannelsVector,
                                           const std::vector<uint32_t>& channelIndexVector,
                                           IasAudioSinkDevicePtr* audioSinkDevice,
                                           IasRoutingZonePtr* routingZone,
                                           std::vector<IasAudioPortPtr>* sinkPortVector,
                                           std::vector<IasAudioPortPtr>* routingZonePortVector)
{
  if (setup == nullptr || audioSinkDevice == nullptr || routingZone == nullptr ||
      sinkPortVector  == nullptr || routingZonePortVector == nullptr)
  {
    return IasISetup::eIasFailed;
  }

  if ((idVector.size() != numChannelsVector.size()) || (numChannelsVector.size() != channelIndexVector.size()))
  {
    return IasISetup::eIasFailed;
  }

  // Create audio sink
  IasAudioSinkDevicePtr sink = nullptr;
  IasISetup::IasResult result = setup->createAudioSinkDevice(params, &sink);
  if (result != IasISetup::eIasOk)
  {
    return result;
  }

  sinkPortVector->clear();
  routingZonePortVector->clear();

  for (uint32_t cntPorts=0; cntPorts < idVector.size(); cntPorts++)
  {
    int32_t  id = idVector[cntPorts];
    uint32_t numChannels = numChannelsVector[cntPorts];
    uint32_t channelIndex = channelIndexVector[cntPorts];

    // Create sink port and add to sink device
    IasAudioPortParams sinkPortParams =
      {
        params.name + "_port_" + std::to_string(id),
        numChannels,
        -1,
        eIasPortDirectionInput,
        channelIndex
      };
    IasAudioPortPtr sinkPort = nullptr;
    result = setup->createAudioPort(sinkPortParams, &sinkPort);
    IAS_ASSERT(result == IasISetup::eIasOk);
    result = setup->addAudioInputPort(sink, sinkPort);
    IAS_ASSERT(result == IasISetup::eIasOk);
    sinkPortVector->push_back(sinkPort);
  }

  // Create a routing zone
  IasRoutingZoneParams routingZoneParams =
  {
    params.name + "_rz"
  };
  IasRoutingZonePtr myRoutingZone = nullptr;
  result = setup->createRoutingZone(routingZoneParams, &myRoutingZone);
  IAS_ASSERT(result == IasISetup::eIasOk);

  // Link the routing zone with the sink
  result = setup->link(myRoutingZone, sink);
  IAS_ASSERT(result == IasISetup::eIasOk);
  *audioSinkDevice = sink;
  *routingZone = myRoutingZone;

  for (uint32_t cntPorts=0; cntPorts < idVector.size(); cntPorts++)
  {
    int32_t  id = idVector[cntPorts];
    uint32_t numChannels = numChannelsVector[cntPorts];

    // Create a zone port
    IasAudioPortParams zonePortParams =
      {
        params.name + "_rznport_" + std::to_string(id),
        numChannels,
        id,
        eIasPortDirectionInput,
        0
      };
    IasAudioPortPtr zonePort = nullptr;
    result = setup->createAudioPort(zonePortParams, &zonePort);
    IAS_ASSERT(result == IasISetup::eIasOk);
    result = setup->addAudioInputPort(myRoutingZone, zonePort);
    IAS_ASSERT(result == IasISetup::eIasOk);
    routingZonePortVector->push_back(zonePort);
  }
  return IasISetup::eIasOk;
}


IasISetup::IasResult deleteSourceDevice(IasISetup *setup, const std::string& deviceName)
{
  IasISetup::IasResult result = IasISetup::IasResult::eIasOk;
  auto sourceDevice = setup->getAudioSourceDevice(deviceName);

  if (sourceDevice != nullptr)
  {
    setup->destroyAudioSourceDevice(sourceDevice);
  }
  else
  {
    result = IasISetup::IasResult::eIasFailed;
  }

  return result;
}


IasISetup::IasResult deleteSinkDevice(IasISetup *setup, const std::string& deviceName)
{
  const std::string rznPortName{deviceName + "_rznport"};
  const std::string devicePortName{deviceName + "_port"};
  IasISetup::IasResult result = IasISetup::IasResult::eIasOk;

  auto sinkDevice = setup->getAudioSinkDevice(deviceName);

  if (sinkDevice != nullptr)
  {
    auto rz = sinkDevice->getRoutingZone();
    if (rz != nullptr)
    {
      if (rz->isActive() || rz->isActivePending())
      {
        DltContext *mLog = IasAudioLogging::getDltContext("CFG");
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                    "Cannot delete sink device", deviceName, "when routing zone", rz->getName(), "is running");
        return IasISetup::IasResult::eIasFailed;
      }
      
      auto ports = setup->getAudioPorts();
      for(auto port : ports)
      {
        const std::string actualPortName = port->getParameters()->name;

        if(actualPortName == devicePortName)
        {
          if(port->isConnected())
          {
            DltContext *mLog = IasAudioLogging::getDltContext("CFG");
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                        "Cannot delete sink device", deviceName, "when port", port->getParameters()->name, "is connected");
            return IasISetup::IasResult::eIasFailed;
          }

          setup->deleteAudioInputPort(sinkDevice, port);
          setup->destroyAudioPort(port);
        }
        if(actualPortName == rznPortName)
        {
          if(port->isConnected())
          {
            DltContext *mLog = IasAudioLogging::getDltContext("CFG");
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                        "Cannot delete sink device", deviceName, "when port", port->getParameters()->name, "is connected");
            return IasISetup::IasResult::eIasFailed;
          }

          setup->deleteAudioInputPort(rz, port);
          setup->destroyAudioPort(port);
        }
      }
      setup->destroyRoutingZone(rz);
    }
    
    setup->destroyAudioSinkDevice(sinkDevice);
  }
  else
  {
    result = IasISetup::IasResult::eIasFailed;
  }

  return result;
}


static std::string getDeviceNamePriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return "";
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return device->getDeviceParams()->name;
}

static int32_t getDeviceNumChannelsPriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return -1;
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return device->getDeviceParams()->numChannels;
}

static int32_t getDeviceSampleRatePriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return -1;
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return device->getSampleRate();
}

static std::string getDeviceDataFormatPriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return "";
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return toString(device->getSampleFormat());
}

static std::string getDeviceClockTypePriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return "";
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return toString(device->getClockType());
}

static int32_t getDevicePeriodSizePriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return -1;
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return device->getPeriodSize();
}

static int32_t getDeviceNumPeriodPriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return -1;
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return device->getNumPeriods();
}

static int32_t getDevicePeriodsAsrcBufferPriv(IasAudioDevicePtr device)
{
  if (device == nullptr)
  {
    return -1;
  }
  IAS_ASSERT(device->getDeviceParams() != nullptr);
  return device->getPeriodsAsrcBuffer();
}


std::string getDeviceName(IasAudioSourceDevicePtr device)
{
  return getDeviceNamePriv(device);
}

std::string getDeviceName(IasAudioSinkDevicePtr device)
{
  return getDeviceNamePriv(device);
}

int32_t getDeviceNumChannels(IasAudioSourceDevicePtr device)
{
  return getDeviceNumChannelsPriv(device);
}

int32_t getDeviceNumChannels(IasAudioSinkDevicePtr device)
{
  return getDeviceNumChannelsPriv(device);
}

int32_t getDeviceSampleRate(IasAudioSourceDevicePtr device)
{
  return getDeviceSampleRatePriv(device);
}

int32_t getDeviceSampleRate(IasAudioSinkDevicePtr device)
{
  return getDeviceSampleRatePriv(device);
}

std::string getDeviceDataFormat(IasAudioSourceDevicePtr device)
{
  return getDeviceDataFormatPriv(device);
}

std::string getDeviceDataFormat(IasAudioSinkDevicePtr device)
{
  return getDeviceDataFormatPriv(device);
}

std::string getDeviceClockType(IasAudioSourceDevicePtr device)
{
  return getDeviceClockTypePriv(device);
}

std::string getDeviceClockType(IasAudioSinkDevicePtr device)
{
  return getDeviceClockTypePriv(device);
}

int32_t getDevicePeriodSize(IasAudioSourceDevicePtr device)
{
  return getDevicePeriodSizePriv(device);
}

int32_t getDevicePeriodSize(IasAudioSinkDevicePtr device)
{
  return getDevicePeriodSizePriv(device);
}

int32_t getDeviceNumPeriod(IasAudioSourceDevicePtr device)
{
  return getDeviceNumPeriodPriv(device);
}

int32_t getDeviceNumPeriod(IasAudioSinkDevicePtr device)
{
  return getDeviceNumPeriodPriv(device);
}

int32_t getDevicePeriodsAsrcBuffer(IasAudioSourceDevicePtr device)
{
  return getDevicePeriodsAsrcBufferPriv(device);
}

int32_t getDevicePeriodsAsrcBuffer(IasAudioSinkDevicePtr device)
{
  return getDevicePeriodsAsrcBufferPriv(device);
}

std::string getPinDirection(IasAudioPinPtr pin)
{
  if (pin == nullptr)
  {
    return "";
  }
  IasAudioPin::IasPinDirection dir = pin->getDirection();
  return toString(dir);
}

std::string getPinName(IasAudioPinPtr pin)
{
  if (pin == nullptr)
  {
    return "";
  }
  IAS_ASSERT(pin->getParameters() != nullptr);
  return pin->getParameters()->name;
}

uint32_t getPinNumChannels(IasAudioPinPtr pin)
{
  if (pin == nullptr)
  {
    return 0;
  }
  IAS_ASSERT(pin->getParameters() != nullptr);
  return pin->getParameters()->numChannels;
}

int32_t getPortId(IasAudioPortPtr port)
{
  if (port == nullptr)
  {
    return -1;
  }
  IAS_ASSERT(port->getParameters() != nullptr);
  return port->getParameters()->id;
}

std::string getPortName(IasAudioPortPtr port)
{
  if (port == nullptr)
  {
    return "";
  }
  IAS_ASSERT(port->getParameters() != nullptr);
  return port->getParameters()->name;
}

uint32_t getPortNumChannels(IasAudioPortPtr port)
{
  if (port == nullptr)
  {
    return 0;
  }
  IAS_ASSERT(port->getParameters() != nullptr);
  return port->getParameters()->numChannels;
}

std::string getPortDirection(IasAudioPortPtr port)
{
  if (port == nullptr)
  {
    return "";
  }
  IAS_ASSERT(port->getParameters() != nullptr);

  IasPortDirection dir = port->getParameters()->direction;

  std::string str = toString(dir);
  return ( str.erase(0,17));
}

std::string getRoutingZoneName(IasRoutingZonePtr zone)
{
  if (zone == nullptr)
  {
    return "";
  }
  return zone->getName();
}

std::string getAudioSourceDeviceName(IasAudioSourceDevicePtr source)
{
  if (source == nullptr)
  {
    return "";
  }
  return source->getName();
}

std::string getAudioSinkDeviceName(IasAudioSinkDevicePtr sink)
{
  if (sink == nullptr)
  {
    return "";
  }
  return sink->getName();
}

} // namespace IasSetupHelper

} //namespace IasAudio
