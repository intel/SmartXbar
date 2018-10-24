/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSetupImpl.cpp
 * @date   2015
 * @brief
 */

#include "IasSetupImpl.hpp"

#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasRoutingZone.hpp"
#include "model/IasPipeline.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasProcessingModule.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include <rtprocessingfwx/IasCmdDispatcher.hpp>
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "IasConfiguration.hpp"
#include "alsahandler/IasAlsaHandler.hpp"
#include "IasSmartXClient.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasIRouting.hpp"

namespace IasAudio {

static const std::string cClassName = "IasSetupImpl::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_PORT "port=" + name + ":"

IasISetup::IasResult translate(IasConfiguration::IasResult result)
{
  switch (result)
  {
    case IasConfiguration::eIasOk:
      return IasISetup::eIasOk;
    default:
      return IasISetup::eIasFailed;
  }
}

IasISetup::IasResult translate(IasSmartXClient::IasResult result)
{
  switch (result)
  {
    case IasSmartXClient::eIasOk:
      return IasISetup::eIasOk;
    default:
      return IasISetup::eIasFailed;
  }
}

IasISetup::IasResult translate(IasAlsaHandler::IasResult result)
{
  switch (result)
  {
    case IasAlsaHandler::eIasOk:
      return IasISetup::eIasOk;
    default:
      return IasISetup::eIasFailed;
  }
}

IasISetup::IasResult translate(IasRoutingZone::IasResult result)
{
  switch (result)
  {
    case IasRoutingZone::eIasOk:
      return IasISetup::eIasOk;
    default:
      return IasISetup::eIasFailed;
  }
}

IasISetup::IasResult translate(IasAudioPort::IasResult result)
{
  switch (result)
  {
    case IasAudioPort::eIasOk:
      return IasISetup::eIasOk;
    default:
      return IasISetup::eIasFailed;
  }
}

bool isRoutingZoneActive(IasRoutingZonePtr routingZone)
{
  return routingZone->isActive() || routingZone->isActivePending();
}

IasSetupImpl::IasSetupImpl(IasConfigurationPtr config, IasCmdDispatcherPtr cmdDispatcher, IasIRouting* routing)
  :mConfig(config)
  ,mCmdDispatcher(cmdDispatcher)
  ,mPluginEngine(nullptr)
  ,mLog(IasAudioLogging::getDltContext("CFG"))
  ,mMaxDataBuffSize(MAX_DATA_BUFFER_SIZE_DEVICE)
  ,mRouting(routing)
{
  IAS_ASSERT(mConfig != nullptr);
  IAS_ASSERT(mLog != nullptr);
  IAS_ASSERT(mCmdDispatcher != nullptr);
  IAS_ASSERT(mRouting != nullptr);
}

IasSetupImpl::~IasSetupImpl()
{
  //Stop all routing zones
  IasRoutingZoneMap tmpRoutingZoneMap = mConfig->getRoutingZoneMap();
  for( auto &entry : tmpRoutingZoneMap)
  {
    stopRoutingZone(entry.second);
  }
  // Destroy all pipelines.
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  IasPipelineMap pipelineMap = mConfig->getPipelineMap();
  for (auto &entry : pipelineMap)
  {
    entry.second->clearConfig();
    destroyPipeline(&entry.second);
  }
  // Create a copy of the routing zone map to avoid conflict while destroying a routing zone:
  // Destroying a routing zone removes it from the routingZoneMap. Iterating over this map while
  // deleting an element is not allowed.
  for( auto &entry : tmpRoutingZoneMap)
  {
    destroyRoutingZone(entry.second);
  }
  mCmdDispatcher.reset();
  mConfig.reset();
  mPluginEngine.reset();
  mRouting = nullptr;
}

IasSetupImpl::IasResult IasSetupImpl::addAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort)
{
  if (audioSourceDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioSourceDevice == nullptr");
    return eIasFailed;
  }
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioPort == nullptr");
    return eIasFailed;
  }
  std::string &name = audioPort->getParameters()->name;
  IasPortDirection direction = audioPort->getParameters()->direction;
  if (direction == eIasPortDirectionInput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Port has wrong direction");
    return eIasFailed;
  }

  IasAudioRingBuffer *ringBuffer = audioSourceDevice->getRingBuffer();
  IAS_ASSERT(ringBuffer != nullptr);
  // Check if the index and number of channels of the port are in a valid range for the source device ring buffer
  uint32_t portIndex = audioPort->getParameters()->index;
  uint32_t portNumChannels = audioPort->getParameters()->numChannels;
  uint32_t maxNumChannels = portIndex + portNumChannels;
  if (maxNumChannels > ringBuffer->getNumChannels())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Port index=", portIndex, "+ port num channels=", portNumChannels,
                "are out of range for maximum ring buffer num channels=", ringBuffer->getNumChannels(),
                "of source device", audioSourceDevice->getName());
    return eIasFailed;
  }

  IasAudioPort::IasResult portres = audioPort->setOwner(audioSourceDevice);
  IAS_ASSERT(portres == IasAudioPort::eIasOk);
  IasSmartXClientPtr smartXclient;
  IasAlsaHandlerPtr alsaHandler;
  portres = audioPort->setRingBuffer(ringBuffer);
  IAS_ASSERT(portres == IasAudioPort::eIasOk);
  (void)portres;
  IasAudioCommonResult comres = audioSourceDevice->addAudioPort(audioPort);
  if (comres != eIasResultOk)
  {
    // There is no need to make a log entry, as this is already done by addAudioPort
    return eIasFailed;
  }
  IasResult setres = translate(mConfig->addPort(audioPort));
  if (setres == IasResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PORT, "Successfully added output port to source device", audioSourceDevice->getName(),
                "Id=", audioPort->getParameters()->id,
                "NumChannels=", audioPort->getParameters()->numChannels,
                "Index=", audioPort->getParameters()->index);
  }
  return setres;
}

void IasSetupImpl::deleteAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort)
{
  if (audioSourceDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audioSourceDevice == nullptr");
    return;
  }
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audioPort == nullptr");
    return;
  }

  if(audioPort->isConnected())
  {
   // Start to disconnect
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "audio port is connected, removing connections");
    IasSwitchMatrixPtr switchMatrix = audioPort->getSwitchMatrix();
    if( switchMatrix->removeConnections(audioPort) != IasSwitchMatrix::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Failed to remove connections");
    }
    else
    {
      audioPort->forgetConnection(switchMatrix);
    }
  }
  mConfig->deleteOutputPort(audioPort->getParameters()->id);
  IasAudioPortOwnerPtr owner;
  std::string &name = audioPort->getParameters()->name;
  IasResult res = translate(audioPort->getOwner(&owner));
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_PORT, "audio port has no owner to delete");
    return;
  }
  if (owner != audioSourceDevice)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_PORT, "audio port has different owner. Nothing deleted");
    return;
  }
  audioPort->clearOwner();
  audioSourceDevice->deleteAudioPort(audioPort);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PORT, "Successfully deleted output port from source device", audioSourceDevice->getName(),
              "Id=", audioPort->getParameters()->id,
              "NumChannels=", audioPort->getParameters()->numChannels,
              "Index=", audioPort->getParameters()->index);
}

IasSetupImpl::IasResult IasSetupImpl::addAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort)
{
  if (audioSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioSinkDevice == nullptr");
    return eIasFailed;
  }
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioPort == nullptr");
    return eIasFailed;
  }
  std::string &name = audioPort->getParameters()->name;
  IasPortDirection direction = audioPort->getParameters()->direction;
  if (direction == eIasPortDirectionOutput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Port has wrong direction");
    return eIasFailed;
  }
  IasRoutingZonePtr routingZone = audioSinkDevice->getRoutingZone();
  if (routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Cannot add input port to sink device", audioSinkDevice->getName(),
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }
  IasAudioPort::IasResult portres = audioPort->setOwner(audioSinkDevice);
  IAS_ASSERT(portres == IasAudioPort::eIasOk);
  (void)portres;

  IasAudioRingBuffer *ringBuffer = audioSinkDevice->getRingBuffer();
  IAS_ASSERT(ringBuffer != nullptr);
  portres = audioPort->setRingBuffer(ringBuffer);
  IAS_ASSERT(portres == IasAudioPort::eIasOk);

  // Check if the index and number of channels of the port are in a valid range for the sink device ring buffer
  uint32_t portIndex = audioPort->getParameters()->index;
  uint32_t portNumChannels = audioPort->getParameters()->numChannels;
  uint32_t maxNumChannels = portIndex + portNumChannels;
  if (maxNumChannels > ringBuffer->getNumChannels())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Port index=", portIndex, "+ port num channels=", portNumChannels,
                "are out of range for maximum ring buffer num channels=", ringBuffer->getNumChannels(),
                "of sink device", audioSinkDevice->getName());
    return eIasFailed;
  }

  IasAudioCommonResult deviceResult = audioSinkDevice->addAudioPort(audioPort);
  if (deviceResult != eIasResultOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot add audioPort to sink device");
    return eIasFailed;
  }
  IasResult setres = translate(mConfig->addPort(audioPort));
  if (setres == IasResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PORT, "Successfully added input port to sink device", audioSinkDevice->getName(),
                "Id=", audioPort->getParameters()->id,
                "NumChannels=", audioPort->getParameters()->numChannels,
                "Index=", audioPort->getParameters()->index);
  }
  return setres;
}

void IasSetupImpl::deleteAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort)
{
  if (audioSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audioSinkDevice == nullptr");
    return;
  }
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audioPort == nullptr");
    return;
  }

  std::string &name = audioPort->getParameters()->name;
  IasAudioPortOwnerPtr owner;
  IasResult res = translate(audioPort->getOwner(&owner));
  if (res != eIasOk)
  {
    /**
     * @log The port given as parameter is not owned by any device.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_PORT, "audio port has no owner to delete");
    return;
  }
  if (owner != audioSinkDevice)
  {
    /**
     * @log The port given as parameter is owned by a different device like the one given as parameter.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_PORT, "audio port has different owner. Nothing deleted");
    return;
  }

  IasRoutingZonePtr routingZone = audioSinkDevice->getRoutingZone();
  if (routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Cannot delete input port from sink device", audioSinkDevice->getName(),
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  audioPort->clearOwner();
  audioSinkDevice->deleteAudioPort(audioPort);

  if (audioPort->getParameters()->id >= 0)
  {
    mConfig->deleteInputPort(audioPort->getParameters()->id);
  }

  mConfig->deletePortByName(audioPort->getParameters()->name);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PORT, "Successfully deleted input port from sink device", audioSinkDevice->getName(),
              "Id=", audioPort->getParameters()->id,
              "NumChannels=", audioPort->getParameters()->numChannels,
              "Index=", audioPort->getParameters()->index);
}

IasSetupImpl::IasResult IasSetupImpl::addAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort)
{
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "routingZone == nullptr");
    return eIasFailed;
  }
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioPort == nullptr");
    return eIasFailed;
  }
  std::string &name = audioPort->getParameters()->name;
  IasPortDirection direction = audioPort->getParameters()->direction;
  if (direction == eIasPortDirectionOutput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Port has wrong direction");
    return eIasFailed;
  }

  // Check that the index is zero
  uint32_t portIndex = audioPort->getParameters()->index;
  if (portIndex != 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Port index=", portIndex, "which is invalid. It has to be zero for a routing zone input port");
    return eIasFailed;
  }

  // Verify that routing zone is not running
  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Cannot add input port to routing zone", routingZone->getName(),
                "which is running" );
    return eIasFailed;
  }

  IasAudioPort::IasResult portres = audioPort->setOwner(routingZone);
  if (portres != IasAudioPort::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while IasAudioPort::setOwner");
    return eIasFailed;
  }

  // Create the conversion buffer. Since we do not define a data format, the conversion buffer
  // will have the same data format as the sink device.
  IasAudioRingBuffer *ringBuffer = nullptr;
  IasResult res = translate(routingZone->createConversionBuffer(audioPort, IasAudioCommonDataFormat::eIasFormatUndef, &ringBuffer));
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during IasRoutingZone::createConversionBuffer");
    return eIasFailed;
  }

  // ringBuffer is always != nullptr after createConversionBuffer successfully returned
  portres = audioPort->setRingBuffer(ringBuffer);
  if (portres != IasAudioPort::eIasOk)
  {
    // Error in case when audio port already has a ring buffer
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while IasAudioPort::setRingBuffer");
    return eIasFailed;
  }

  res = translate(mConfig->addPort(audioPort));
  if (res != eIasOk)
  {
    // No log required because detailed log already done inside addPort method
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PORT, "Successfully added input port to routing zone", routingZone->getName(),
              "Id=", audioPort->getParameters()->id,
              "NumChannels=", audioPort->getParameters()->numChannels,
              "Index=", audioPort->getParameters()->index);
  return eIasOk;
}

void IasSetupImpl::deleteAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort)
{
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "routingZone == nullptr");
    return;
  }
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audioPort == nullptr");
    return;
  }

  IasAudioPortOwnerPtr owner;
  IasResult res = translate(audioPort->getOwner(&owner));
  std::string &name = audioPort->getParameters()->name;

  if (res != eIasOk)
  {
    /**
     * @log The port given as parameter is not owned by any routing zone.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_PORT, "audio port", name, "has no owner to delete");
    return;
  }
  if (owner != routingZone)
  {
    /**
     * @log The port given as parameter is owned by a different routing zone like the one given as parameter.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audio port", name, "has different owner. Nothing deleted");
    return;
  }

  // Verify that routing zone is not running
  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT,
                "Cannot delete input port from routing zone", routingZone->getName(),
                "which is running" );
    return;
  }

  audioPort->clearOwner();

  IasRoutingZone::IasResult result = routingZone->destroyConversionBuffer(audioPort);
  if (result != IasRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Error during IasRoutingZone::destroyConversionBuffer:", toString(result));
  }

  mConfig->deleteInputPort(audioPort->getParameters()->id);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PORT, "Successfully deleted input port from routing zone", routingZone->getName(),
              "Id=", audioPort->getParameters()->id,
              "NumChannels=", audioPort->getParameters()->numChannels,
              "Index=", audioPort->getParameters()->index);
}

IasSetupImpl::IasResult IasSetupImpl::addDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone)
{
  if (baseZone == nullptr)
  {
    /**
     * @log Parameter baseZone is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "baseZone == nullptr");
    return eIasFailed;
  }
  if (derivedZone == nullptr)
  {
    /**
     * @log Parameter derivedZone is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "derivedZone == nullptr");
    return eIasFailed;
  }

  // Verify that both zones are currently not active.
  if (isRoutingZoneActive(baseZone) || isRoutingZoneActive(derivedZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during IasRoutingZone::addDerivedZone: both zones",
                baseZone->getRoutingZoneParams()->name, "and", derivedZone->getRoutingZoneParams()->name, "must not be running.");
    return eIasFailed;
  }

  // We don't need a switch matrix object for the derived zone, so clear it
  derivedZone->clearSwitchMatrix();
  IasRoutingZone::IasResult result = baseZone->addDerivedZone(derivedZone);
  if (result != IasRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during IasRoutingZone::addDerivedZone:", toString(result));
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added derived zone", derivedZone->getName(), "to base zone", baseZone->getName());
  return eIasOk;
}

void IasSetupImpl::deleteDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone)
{
  if (baseZone == nullptr || derivedZone == nullptr)
  {
    return;
  }
  // Verify that both zones are currently not active.
  if (isRoutingZoneActive(baseZone) || isRoutingZoneActive(derivedZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during IasRoutingZone::deleteDerivedZone: both zones",
                baseZone->getName(), "and", derivedZone->getName(), "must not be running.");
    return;
  }

  baseZone->deleteDerivedZone(derivedZone);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted derived zone", derivedZone->getName(), "from base zone", baseZone->getName());
}

IasSetupImpl::IasResult IasSetupImpl::createAudioPort(const IasAudioPortParams& params, IasAudioPortPtr* audioPort)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort == nullptr");
    return eIasFailed;
  }
  if (params.numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: numChannels == 0");
    return eIasFailed;
  }
  if (params.direction == eIasPortDirectionUndef)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: direction ==", toString(params.direction));
    return eIasFailed;
  }
  if (params.index == 0xFFFFFFFF)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: index ==", params.index);
    return eIasFailed;
  }
  IasAudioPortParamsPtr portParams = std::make_shared<IasAudioPortParams>(params);
  IAS_ASSERT(portParams != nullptr);
  IasAudioPortPtr newPort = std::make_shared<IasAudioPort>(portParams);
  IAS_ASSERT(newPort != nullptr);
  *audioPort = newPort;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created new audio port.",
              "Name=", params.name,
              "Id=", params.id,
              "NumChannels=", params.numChannels,
              "Index", params.index,
              "Direction=", toString(params.direction));
  return eIasOk;
}

void IasSetupImpl::destroyAudioPort(IasAudioPortPtr audioPort)
{
  if (audioPort == nullptr)
  {
    return;
  }

  const IasAudioPortParamsPtr params = audioPort->getParameters();
  IasAudioPortOwnerPtr portOwner;
  audioPort->getOwner(&portOwner);
  if (portOwner != nullptr)
  {
    IasRoutingZonePtr routingZone = portOwner->getRoutingZone();
    if (routingZone != nullptr && isRoutingZoneActive(routingZone))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                  "Cannot destroy audio port", params->name,
                  "when routing zone", routingZone->getName(), "is running" );
      return;
    }
  }

  audioPort->clearOwner();
  audioPort.reset();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed audio port.",
              "Name=", params->name,
              "Id=", params->id,
              "NumChannels=", params->numChannels,
              "Index", params->index,
              "Direction=", toString(params->direction));
}


void IasSetupImpl::destroyAudioSinkDevice(IasAudioSinkDevicePtr audioSinkDevice)
{
  if (audioSinkDevice == nullptr)
  {
    return;
  }

  // Check if related routing zone is active
  IasRoutingZonePtr routingZone = audioSinkDevice->getRoutingZone();
  if (routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot destroy sink device", audioSinkDevice->getName(),
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  IasAudioDeviceParamsPtr devParams = audioSinkDevice->getDeviceParams();
  mConfig->deleteAudioDevice(devParams->name);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed audio sink device.",
              "Name=", devParams->name,
              "SampleRate=", devParams->samplerate,
              "PeriodSize=", devParams->periodSize,
              "NumPeriods=",devParams->numPeriods,
              "NumPeriodsAsrcBuffer=", devParams->numPeriodsAsrcBuffer,
              "ClockType=", toString(devParams->clockType),
              "DataFormat=", toString(devParams->dataFormat),
              "NumChannels=", devParams->numChannels);
}

template <typename IasDeviceClass, typename IasConcreteType>
IasSetupImpl::IasResult IasSetupImpl::createConcreteAudioDevice(const IasAudioDeviceParams& params, std::shared_ptr<IasDeviceClass> *device, IasDeviceType deviceType)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
              "name=", params.name,
              "numChannels=", params.numChannels,
              "clockType=", toString(params.clockType),
              "dataFormat=", toString(params.dataFormat),
              "numPeriods=", params.numPeriods,
              "periodSize=", params.periodSize,
              "samplerate=", params.samplerate);
  if (device == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "device == nullptr");
    return eIasFailed;
  }
  // Here we have to check if the samplerate is at least != 0 because else it will only be recognized during later
  // operation. All other parameters are already checked in the ringbuffer factory
  if (params.samplerate == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "samplerate 0 is not allowed");
    return eIasFailed;
  }
  if (!verifyBufferSize(params))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "the parameter set exceeds the maximum allowed data buffer size of", mMaxDataBuffSize);
    return eIasFailed;
  }
  IasISetup::IasResult result = eIasOk;
  // Create concrete device:
  // 1. Check if we already have a source device with the same name
  std::shared_ptr<IasDeviceClass> newAudioDevice = nullptr;
  IasConfiguration::IasResult cfgres = mConfig->getAudioDevice(params.name, &newAudioDevice);
  if (cfgres == IasConfiguration::eIasOk)
  {
    // There is already a source device with that name.
    // It is not allowed to have two devices with the same name
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Object already exists");
    return eIasFailed;
  }
  IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
  IAS_ASSERT(devParams != nullptr);
  *devParams = params;
  newAudioDevice = std::make_shared<IasDeviceClass>(devParams);
  IAS_ASSERT(newAudioDevice != nullptr);
  std::shared_ptr<IasConcreteType> concreteDevice = std::make_shared<IasConcreteType>(devParams);
  IAS_ASSERT(concreteDevice != nullptr);
  result = translate(concreteDevice->init(deviceType));
  if (result != eIasOk)
  {
    return result;
  }
  newAudioDevice->setConcreteDevice(concreteDevice);
  result = translate(mConfig->addAudioDevice(params.name, newAudioDevice));
  IAS_ASSERT(result == eIasOk);
  *device = newAudioDevice;
  return result;
}

template <typename IasDeviceClass>
IasSetupImpl::IasResult IasSetupImpl::createAudioDevice(const IasAudioDeviceParams &params, std::shared_ptr<IasDeviceClass> *device, IasDeviceType deviceType)
{
  if (params.clockType == eIasClockProvided)
  {
    return createConcreteAudioDevice<IasDeviceClass, IasSmartXClient>(params, device, deviceType);
  }
  else if ((params.clockType == eIasClockReceived) || (params.clockType == eIasClockReceivedAsync))
  {
    return createConcreteAudioDevice<IasDeviceClass, IasAlsaHandler>(params, device, deviceType);
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Clock type is undefined");
    return IasISetup::eIasFailed;
  }
}

IasSetupImpl::IasResult IasSetupImpl::createAudioSourceDevice(const IasAudioDeviceParams& params, IasAudioSourceDevicePtr* audioSourceDevice)
{
  return createAudioDevice<IasAudioSourceDevice>(params, audioSourceDevice, eIasDeviceTypeSource);
}

IasSetupImpl::IasResult IasSetupImpl::createAudioSinkDevice(const IasAudioDeviceParams& params, IasAudioSinkDevicePtr* audioSinkDevice)
{
  return createAudioDevice<IasAudioSinkDevice>(params, audioSinkDevice, eIasDeviceTypeSink);
}

void IasSetupImpl::destroyAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice)
{
  if (audioSourceDevice == nullptr)
  {
    return;
  }

  auto ports = getAudioPorts();
  for(auto port : ports)
  {
    IasAudioPortOwnerPtr owner;

    if((translate(port->getOwner(&owner)) == eIasOk) && owner == audioSourceDevice)
    {
      deleteAudioOutputPort(audioSourceDevice, port);
      destroyAudioPort(port);
    }
  }

  IasAudioDeviceParamsPtr devParams = audioSourceDevice->getDeviceParams();
  mConfig->deleteAudioDevice(devParams->name);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed audio source device.",
              "Name=", devParams->name,
              "SampleRate=", devParams->samplerate,
              "PeriodSize=", devParams->periodSize,
              "NumPeriods=",devParams->numPeriods,
              "NumPeriodsAsrcBuffer=", devParams->numPeriodsAsrcBuffer,
              "ClockType=", toString(devParams->clockType),
              "DataFormat=", toString(devParams->dataFormat),
              "NumChannels=", devParams->numChannels);
}


IasISetup::IasResult IasSetupImpl::startAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice)
{
  if (audioSourceDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioSourceDevice == nullptr");
    return eIasFailed;
  }

  if (audioSourceDevice->isAlsaHandler())
  {
    const IasAudioDeviceParamsPtr audioDeviceParams = audioSourceDevice->getDeviceParams();
    IAS_ASSERT(audioDeviceParams != nullptr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Starting audio source device (ALSA handler)", audioDeviceParams->name);

    IasAlsaHandlerPtr alsaHandler = nullptr;
    IasAudioCommonResult acResult = audioSourceDevice->getConcreteDevice(&alsaHandler);
    IAS_ASSERT(acResult == IasAudioCommonResult::eIasResultOk);
    (void)acResult;
    IAS_ASSERT(alsaHandler != nullptr); // already checked in IasAudioDevice::isAlsaHandler()
    IasAlsaHandler::IasResult ahResult = alsaHandler->start();
    if (ahResult != IasAlsaHandler::eIasOk)
    {
      // No log required as a detailed log is provided by the start method of the ALSA handler
      return eIasFailed;
    }
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully started audio source device", audioSourceDevice->getName());
  return eIasOk;
}


void IasSetupImpl::stopAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice)
{
  if (audioSourceDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioSourceDevice == nullptr");
    return;
  }

  auto portSet = audioSourceDevice->getAudioPortSet();
  if (portSet != nullptr)
  {
    // Check if any of the ports of this source device is currently connected
    // and in this case remove all connections for being able to safely stop the device.
    for (auto port : *portSet)
    {
      // Only valid ports are added to the port set
      IAS_ASSERT(port != nullptr);
      if (port->isConnected() == true)
      {
        // Ensured by init sequence in IasSmartXPriv::init() method
        IAS_ASSERT(mRouting != nullptr);
        // Start to disconnect
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Audio port", port->getParameters()->name, port->getParameters()->id, "is connected, removing connections");
        IasSwitchMatrixPtr switchMatrix = port->getSwitchMatrix();
        // If the port is connected, it is guaranteed that it has a valid switchmatrix pointer
        IAS_ASSERT(switchMatrix != nullptr);
        if( switchMatrix->removeConnections(port) != IasSwitchMatrix::eIasOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Failed to remove connections. Continue stopping device.");
        }
        else
        {
          port->forgetConnection(switchMatrix);
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Audio port", port->getParameters()->name, port->getParameters()->id, "connection successfully removed");
          // Now we have to clean-up the sink device input ports of the active connection, else we are not able
          // to establish another connection to this sink devices input port anymore.
          for (auto entry : mRouting->getActiveConnections())
          {
            IasAudioPortPtr outPort = entry.first;
            IasAudioPortPtr inPort = entry.second;
            if (port->getParameters()->id == outPort->getParameters()->id)
            {
              // We found an active connection with that output port, so now we have to also
              // forget the connection of the connected input port.
              // This will lead to an outdated entry in the connection table of the IasRoutingImpl,
              // which will be cleaned after a new connect to this sink id will be done.
              IAS_ASSERT(inPort != nullptr); //this pointer should never be null
              inPort->forgetConnection(switchMatrix);
            }
          }
        }
      }
    }
  }

  if (audioSourceDevice->isAlsaHandler())
  {
    const IasAudioDeviceParamsPtr audioDeviceParams = audioSourceDevice->getDeviceParams();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stopping audio source device (ALSA handler)", audioDeviceParams->name);

    IasAlsaHandlerPtr alsaHandler = nullptr;
    IasAudioCommonResult acResult = audioSourceDevice->getConcreteDevice(&alsaHandler);
    IAS_ASSERT(acResult == IasAudioCommonResult::eIasResultOk);
    (void)acResult;
    IAS_ASSERT(alsaHandler != nullptr); // already checked in IasAudioDevice::isAlsaHandler()
    alsaHandler->stop();
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully stopped audio source device", audioSourceDevice->getName());
}


IasSetupImpl::IasResult IasSetupImpl::createRoutingZone(const IasRoutingZoneParams &params, IasRoutingZonePtr* routingZone)
{
  if (routingZone == nullptr)
  {
    /**
     * @log Destination for return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "routingZone == nullptr");
    return eIasFailed;
  }
  if (params.name.empty())
  {
    /**
     * @log Routing Zone name of initialization structure IasRoutingZoneParams is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Routing Zone name may not be an empty string");
    return eIasFailed;
  }
  IasRoutingZoneParamsPtr zoneParams = std::make_shared<IasRoutingZoneParams>();
  IAS_ASSERT(zoneParams != nullptr);
  *zoneParams = params;
  *routingZone = std::make_shared<IasRoutingZone>(zoneParams);
  IAS_ASSERT(routingZone != nullptr);
  IasRoutingZone::IasResult rzres = (*routingZone)->init();
  IAS_ASSERT(rzres == IasRoutingZone::eIasOk);
  (void)rzres;
  (*routingZone)->setRoutingZone(*routingZone);

  IasSwitchMatrixPtr switchMatrix = std::make_shared<IasSwitchMatrix>();
  IAS_ASSERT(switchMatrix != nullptr);
  // Init of switchMatrix is deferred intentionally because we don't have the init parameter copySize yet
  (*routingZone)->setSwitchMatrix(switchMatrix);
  IasConfiguration::IasResult cfgres = mConfig->addRoutingZone(params.name, *routingZone);
  return translate(cfgres);
}

void IasSetupImpl::destroyRoutingZone(IasRoutingZonePtr routingZone)
{
  if (routingZone != nullptr)
  {
    routingZone->stop();
    routingZone->setRoutingZone(nullptr);
    routingZone->clearSwitchMatrix();
    const IasAudioSinkDevicePtr audioSinkDevice = routingZone->getAudioSinkDevice();
    if (audioSinkDevice != nullptr)
    {
      audioSinkDevice->unlinkZone();
    }
    routingZone->unlinkAudioSinkDevice();
    IasRoutingZoneParamsPtr params = routingZone->getRoutingZoneParams();
    routingZone->clearWorkerThread();
    routingZone->clearDerivedZones();
    IAS_ASSERT(params != nullptr);
    mConfig->deleteRoutingZone(params->name);
    routingZone.reset();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed routing zone", params->name);
  }
}

IasISetup::IasResult IasSetupImpl::startRoutingZone(IasRoutingZonePtr routingZone)
{
  if (routingZone == nullptr)
  {
    /**
     * @log Parameter routingZone is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "routingZone == nullptr");
    return eIasFailed;
  }

  const IasRoutingZoneParamsPtr routingZoneParams = routingZone->getRoutingZoneParams();
  IAS_ASSERT(routingZoneParams != nullptr);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Starting routing zone", routingZoneParams->name);

  IasRoutingZone::IasResult rznResult = routingZone->start();
  if (rznResult != IasRoutingZone::eIasOk)
  {
    // Not required to make log here because more detailed log is provided by start method of routingzone
    return eIasFailed;
  }
  return eIasOk;
}

void IasSetupImpl::stopRoutingZone(IasRoutingZonePtr routingZone)
{
  if (routingZone == nullptr)
  {
    /**
     * @log Parameter routingZone is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "routingZone == nullptr");
    return;
  }

  const IasRoutingZoneParamsPtr routingZoneParams = routingZone->getRoutingZoneParams();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stopping routing zone", routingZoneParams->name);

  routingZone->stop();
}

IasSetupImpl::IasResult IasSetupImpl::link(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice)
{
  if (routingZone == nullptr)
  {
    /**
     * @log Parameter routingZone is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "routingZone == nullptr");
    return eIasFailed;
  }
  if (audioSinkDevice == nullptr)
  {
    /**
     * @log Parameter audioSinkDevice is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "audioSinkDevice == nullptr");
    return eIasFailed;
  }

  IasRoutingZonePtr sinkRoutingZone = audioSinkDevice->getRoutingZone();
  if (sinkRoutingZone != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Sink device is already linked with routing zone", sinkRoutingZone->getName());
    return eIasFailed;
  }

  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot link routing zone", routingZone->getName(),
                "and sink device", audioSinkDevice->getName(), "when routing zone is running" );
    return eIasFailed;
  }

  audioSinkDevice->linkZone(routingZone);
  IasRoutingZone::IasResult result = routingZone->linkAudioSinkDevice(audioSinkDevice);
  IAS_ASSERT(result == IasRoutingZone::eIasOk);
  (void)result;
  IasSwitchMatrixPtr worker = routingZone->getSwitchMatrix();
  if (worker != nullptr)
  {
    IasSwitchMatrix::IasResult comres = worker->init(routingZone->getRoutingZoneParams()->name + "_worker", audioSinkDevice->getPeriodSize(),audioSinkDevice->getSampleRate());
    if (comres != IasSwitchMatrix::eIasOk)
    {
      return eIasFailed;
    }
  }

  return eIasOk;
}

void IasSetupImpl::unlink(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice)
{
  if (routingZone == nullptr || audioSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "input parameter is null");
    return;
  }

  IasRoutingZonePtr sinkRoutingZone = audioSinkDevice->getRoutingZone();
  if (routingZone != sinkRoutingZone)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot unlink routing zone", routingZone->getName(), "from sink device which belongs to",
                "different routing zone", sinkRoutingZone->getName());
    return;
  }

  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot unlink routing zone", routingZone->getName(),
                "from sink device", audioSinkDevice->getName(), "when routing zone is running");
    return;
  }

  audioSinkDevice->unlinkZone();
  routingZone->unlinkAudioSinkDevice();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully unlinked audio sink device", audioSinkDevice->getName(), "from routing zone", routingZone->getName());
}

IasSetupImpl::IasResult IasSetupImpl::link(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  if (zoneInputPort == nullptr)
  {
    /**
     * @log Parameter zoneInputPort is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "zoneInputPort == nullptr");
    return eIasFailed;
  }
  if (sinkDeviceInputPort == nullptr)
  {
    /**
     * @log Parameter sinkDeviceInputPort is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "sinkDeviceInputPort == nullptr");
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
              "linking zoneInputPort", zoneInputPort->getParameters()->name,
              "to sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name);

  // Verify that the zoneInputPort is owned by a routing zone.
  IasAudioPortParamsPtr zoneInputPortParams = zoneInputPort->getParameters();
  IAS_ASSERT(zoneInputPortParams != nullptr); // already checked in constructor of IasAudioPort
  IasAudioPortOwnerPtr zoneInputPortOwner;
  zoneInputPort->getOwner(&zoneInputPortOwner);
  if (zoneInputPortOwner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: zoneInputPort", zoneInputPortParams->name,
                "does not have an port owner");
    return eIasFailed;
  }
  if (!(zoneInputPortOwner->isRoutingZone()))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: zoneInputPort", zoneInputPortParams->name,
                "is not owned by a routing zone");
    return eIasFailed;
  }

  // Verify that the sinkDeviceInputPort is owned by an audio sink device (not by a routing zone).
  IasAudioPortParamsPtr sinkDeviceInputPortParams = sinkDeviceInputPort->getParameters();
  IAS_ASSERT(sinkDeviceInputPortParams != nullptr); // already checked in constructor of IasAudioPort
  IasAudioPortOwnerPtr sinkDeviceInputPortOwner;
  sinkDeviceInputPort->getOwner(&sinkDeviceInputPortOwner);
  if (sinkDeviceInputPortOwner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: sinkDeviceInputPort", sinkDeviceInputPortParams->name,
                "does not have an port owner");
    return eIasFailed;
  }
  if (sinkDeviceInputPortOwner->isRoutingZone())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: sinkDeviceInputPort", sinkDeviceInputPortParams->name,
                "is not owned by a sink device");
    return eIasFailed;
  }

  // Get the handle to the routing zone that owns the zoneInputPort.
  IasRoutingZonePtr routingZone = zoneInputPortOwner->getRoutingZone();
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot determine which routing zone is assigned to port", zoneInputPortParams->name);
    return eIasFailed;
  }

  // Verify that both ports belong to the same routing zone.
  if (routingZone != sinkDeviceInputPortOwner->getRoutingZone())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: audio ports", zoneInputPortParams->name,
                "and", sinkDeviceInputPortParams->name,
                "do not belong to the same routing zone.");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot link ports", zoneInputPort->getParameters()->name,
                "and", sinkDeviceInputPort->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  // Tell the routing zone that both ports shall be linked.
  IasRoutingZone::IasResult result = routingZone->linkAudioPorts(zoneInputPort, sinkDeviceInputPort);
  if (result != IasRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during linkAudioPorts(", zoneInputPortParams->name,
                ",", sinkDeviceInputPortParams->name,
                ")");
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully linked zoneInputPort", zoneInputPort->getParameters()->name,
              "to sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name);

  return eIasOk;
}

void IasSetupImpl::unlink(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  if (zoneInputPort == nullptr)
  {
    /**
     * @log Parameter zoneInputPort is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "zoneInputPort == nullptr");
    return;
  }
  if (sinkDeviceInputPort == nullptr)
  {
    /**
     * @log Parameter sinkDeviceInputPort is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "sinkDeviceInputPort == nullptr");
    return;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
              "unlinking zoneInputPort", zoneInputPort->getParameters()->name,
              "from sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name);

  IasAudioPortParamsPtr zoneInputPortParams = zoneInputPort->getParameters();
  IAS_ASSERT(zoneInputPortParams != nullptr); // already checked in constructor of IasAudioPort
  IasAudioPortOwnerPtr zoneInputPortOwner;
  zoneInputPort->getOwner(&zoneInputPortOwner);
  if (zoneInputPortOwner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: zoneInputPort", zoneInputPortParams->name,
                "does not have an port owner");
    return;
  }

  // Verify that the zoneInputPort is owned by a routing zone.
  if (!(zoneInputPortOwner->isRoutingZone()))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: zoneInputPort", zoneInputPortParams->name,
                "is not owned by a routing zone");
    return;
  }

  IasAudioPortParamsPtr sinkDeviceInputPortParams = sinkDeviceInputPort->getParameters();
  IAS_ASSERT(sinkDeviceInputPortParams != nullptr); // already checked in constructor of IasAudioPort

  // Get the handle to the routing zone that owns the zoneInputPort.
  IasRoutingZonePtr routingZone = zoneInputPortOwner->getRoutingZone();
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot determine which routing zone is assigned to port", zoneInputPortParams->name);
    return;
  }

  // Verify that the routing zone is not running
  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot unlink ports", zoneInputPortParams->name,
                "and", sinkDeviceInputPortParams->name,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  // Tell the routing zone that both ports shall be unlinked.
  routingZone->unlinkAudioPorts(zoneInputPort, sinkDeviceInputPort);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully unlinked zoneInputPort", zoneInputPort->getParameters()->name,
              "from sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name);
}

IasAudioPinVector IasSetupImpl::getAudioPins() const
{
  IasAudioPinVector pins;
  mConfig->getPinMap().size();
  for (auto &entry : mConfig->getPinMap())
  {
    pins.push_back(entry.second);
  }
  return pins;
}

IasAudioPortVector IasSetupImpl::getAudioPorts() const
{
  IasAudioPortVector ports;
  ports.reserve(mConfig->getPortMap().size());
  for (auto &entry : mConfig->getPortMap())
  {
    ports.push_back(entry.second);
  }
  return ports;
}

IasAudioPortVector IasSetupImpl::getAudioInputPorts() const
{
  IasAudioPortVector inPorts;
  inPorts.reserve(mConfig->getInputPortMap().size());
  for (auto &entry : mConfig->getInputPortMap())
  {
    inPorts.push_back(entry.second);
  }
  return inPorts;
}

IasAudioPortVector IasSetupImpl::getAudioOutputPorts() const
{
  IasAudioPortVector outPorts;
  outPorts.reserve(mConfig->getOutputPortMap().size());
  for (auto &entry : mConfig->getOutputPortMap())
  {
    outPorts.push_back(entry.second);
  }
  return outPorts;

}

IasRoutingZoneVector IasSetupImpl::getRoutingZones() const
{
  IasRoutingZoneVector routingZones;
  routingZones.reserve(mConfig->getNumberRoutingZones());
  for (auto &entry : mConfig->getRoutingZoneMap())
  {
    routingZones.push_back(entry.second);
  }
  return routingZones;
}

IasAudioSourceDeviceVector IasSetupImpl::getAudioSourceDevices() const
{
  IasAudioSourceDeviceVector sourceDevices;
  sourceDevices.reserve(mConfig->getNumberSourceDevices());
  for (auto &entry : mConfig->getSourceDeviceMap())
  {
    sourceDevices.push_back(entry.second);
  }
  return sourceDevices;
}

IasAudioSinkDeviceVector IasSetupImpl::getAudioSinkDevices() const
{
  IasAudioSinkDeviceVector sinkDevices;
  sinkDevices.reserve(mConfig->getNumberSinkDevices());
  for (auto &entry : mConfig->getSinkDeviceMap())
  {
    sinkDevices.push_back(entry.second);
  }
  return sinkDevices;
}

IasRoutingZonePtr IasSetupImpl::getRoutingZone(const std::string &name)
{
  IasRoutingZonePtr rzn = nullptr;
  for (auto &entry : getRoutingZones())
  {
    if (entry->getName().compare(name) == 0)
    {
      rzn = entry;
      break;
    }
  }
  return rzn;
}

IasAudioSourceDevicePtr IasSetupImpl::getAudioSourceDevice(const std::string &name)
{
  IasAudioSourceDevicePtr source = nullptr;
  for (auto &entry : getAudioSourceDevices())
  {
    if (entry->getName().compare(name) == 0)
    {
      source = entry;
      break;
    }
  }
  return source;
}

IasAudioSinkDevicePtr IasSetupImpl::getAudioSinkDevice(const std::string &name)
{
  IasAudioSinkDevicePtr sink = nullptr;
  for (auto &entry : getAudioSinkDevices())
  {
    if (entry->getName().compare(name) == 0)
    {
      sink = entry;
      break;
    }
  }
  return sink;
}

void IasSetupImpl::addSourceGroup (const std::string &name, int32_t id)
{
  mConfig->addLogicalSource(name,id);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added source id", id, "to source group", name);
}

IasSourceGroupMap IasSetupImpl::getSourceGroups()
{
  return mConfig->getSourceGroups();
}


IasISetup::IasResult IasSetupImpl::createPipeline(const IasPipelineParams &params, IasPipelinePtr *pipeline)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Destination for return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
    return eIasFailed;
  }
  if (params.name.empty())
  {
    /**
     * @log Audio pipeline name of initialization structure IasAudioPinParams is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: name may not be an empty string");
    return eIasFailed;
  }

  // Create plugin engine, if not created yet.
  if (mPluginEngine == nullptr)
  {
    mPluginEngine = std::make_shared<IasPluginEngine>(mCmdDispatcher);
    IAS_ASSERT(mPluginEngine != nullptr);

    IasAudioProcessingResult res = mPluginEngine->loadPluginLibraries();
    if (res != IasAudioProcessingResult::eIasAudioProcOK)
    {
      /**
       * @log There is no valid plug-in library in the specified plug-in folder available.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No plug-in libraries found. Pipeline will not be created");
      return eIasFailed;
    }
  }

  IasPipelineParamsPtr pipelineParams = std::make_shared<IasPipelineParams>();
  IAS_ASSERT(pipelineParams != nullptr);
  *pipelineParams = params;
  IasPipelinePtr newPipeline = std::make_shared<IasPipeline>(pipelineParams, mPluginEngine, mConfig);
  IAS_ASSERT(newPipeline != nullptr);
  newPipeline->init();
  *pipeline = newPipeline;

  IasConfiguration::IasResult cfgres = mConfig->addPipeline(params.name, *pipeline);
  IasResult setres = translate(cfgres);
  if (setres == IasResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created pipeline", params.name);
  }
  return setres;
}

IasPipelinePtr IasSetupImpl::getPipeline(const std::string &name)
{
  IasPipelinePtr pipeline = nullptr;
  IasConfiguration::IasResult result = mConfig->getPipeline(name, &pipeline);
  if (result != IasConfiguration::eIasOk) {
         DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot find pipeline: ", name);
  }
  return pipeline;
}

void IasSetupImpl::destroyPipeline(IasPipelinePtr *pipeline)
{
  if (pipeline == nullptr)
  {
    return;
  }

  IasPipelineParamsPtr params = (*pipeline)->getParameters();
  IAS_ASSERT(params != nullptr);

  // Verify that routing zone is not active
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult result = mConfig->getRoutingZone(*pipeline, &routingZone);
  if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot destroy pipeline", params->name,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  // Get a vector of all processing modules that belong to the pipeline
  // and tell the modules that they do not belong to the pipeline anymore.
  IasPipeline::IasProcessingModuleVector processingModules;
  (*pipeline)->getProcessingModules(&processingModules);
  for (IasProcessingModulePtr module :processingModules)
  {
    module->deleteFromPipeline();
  }

  // Get a vector of all audio pins that belong to the pipeline
  // and tell the audio pins that they do not belong to the pipeline anymore.
  IasPipeline::IasAudioPinVector audioPins;
  (*pipeline)->getAudioPins(&audioPins);
  for (IasAudioPinPtr pin :audioPins)
  {
    pin->deleteFromPipeline();
  }

  mConfig->deletePipeline(params->name);
  pipeline->reset();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed pipeline", params->name);
}

IasISetup::IasResult IasSetupImpl::addPipeline(IasRoutingZonePtr routingZone, IasPipelinePtr pipeline)
{
  if (routingZone == nullptr)
  {
    /**
     * @log Invalid parameter: routingZone == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: routingZone == nullptr");
    return eIasFailed;
  }
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
    return eIasFailed;
  }

  // Add the pipeline to the routing zone.
  IasRoutingZone::IasResult result = routingZone->addPipeline(pipeline);
  if (result != IasRoutingZone::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasRoutingZone::addPipeline:", toString(result));
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added pipeline", pipeline->getParameters()->name, "to routing zone", routingZone->getName());

  return eIasOk;
}

void IasSetupImpl::deletePipeline(IasRoutingZonePtr routingZone)
{
  if (routingZone != nullptr)
  {
    if (isRoutingZoneActive(routingZone))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                  "Cannot delete pipeline from routing zone", routingZone->getName(), "which is running" );
      return;
    }
    routingZone->deletePipeline();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted pipeline from routing zone", routingZone->getName());
  }
}

IasISetup::IasResult IasSetupImpl::createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin)
{
  if (pin == nullptr)
  {
    /**
     * @log Destination for return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pin == nullptr");
    return eIasFailed;
  }
  if (params.name.empty())
  {
    /**
     * @log Audio pin name of initialization structure IasAudioPinParams is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: name may not be an empty string");
    return eIasFailed;
  }
  if (params.numChannels == 0)
  {
    /**
     * @log Parameter numChannels of initialization structure IasAudioPinParams must not be 0.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: numChannels == 0");
    return eIasFailed;
  }

  IasAudioPinParamsPtr pinParams = std::make_shared<IasAudioPinParams>();
  IAS_ASSERT(pinParams != nullptr);
  *pinParams = params;
  IasAudioPinPtr newPin = std::make_shared<IasAudioPin>(pinParams);
  IAS_ASSERT(newPin != nullptr);
  *pin = newPin;
  IasResult setres = translate(mConfig->addPin(*pin));
  if (setres != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to add pin", newPin->getParameters()->name.c_str(), "to config");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created audio pin", params.name,
                "numChannels=", params.numChannels);
  }
  return setres;
}


void IasSetupImpl::destroyAudioPin(IasAudioPinPtr *pin)
{
  if (pin == nullptr)
  {
    return;
  }

  std::string name = (*pin)->getParameters()->name;

  // Get the handle of the pipeline that owns the audio pin and delete the audio pin from the pipeline.
  const IasPipelinePtr  pipeline = (*pin)->getPipeline();
  if (pipeline != nullptr)
  {
    // Verify that the routing zone is not running
    IasRoutingZonePtr routingZone;
    IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
    if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                  "Cannot destroy audio pin", name, "when routing zone", routingZone->getName(), "is running" );
      return;
    }
    pipeline->deleteAudioPin(*pin);
  }

  (*pin)->deleteFromPipeline();
  pin->reset();
  mConfig->removePin(name);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed audio pin", name);
}


IasISetup::IasResult IasSetupImpl::addAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
    return eIasFailed;
  }
  if (pipelineInputPin == nullptr)
  {
    /**
     * @log Invalid parameter: pipelineInputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineInputPin == nullptr");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot add audio input pin", pipelineInputPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->addAudioInputPin(pipelineInputPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioInputPin:", toString(result));
    return eIasFailed;
  }

  IasAudioPin::IasResult audioPinResult = pipelineInputPin->addToPipeline(pipeline, pipeline->getParameters()->name);
  if (audioPinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasAudioPin::addToPipeline:", toString(audioPinResult));
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio input pin", pipelineInputPin->getParameters()->name,
              "to pipeline", pipeline->getParameters()->name);
  return eIasOk;
}


void IasSetupImpl::deleteAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
  }
  if (pipelineInputPin == nullptr)
  {
    /**
     * @log Invalid parameter: pipelineInputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineInputPin == nullptr");
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
  if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot delete audio input pin", pipelineInputPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  pipeline->deleteAudioPin(pipelineInputPin);
  pipelineInputPin->deleteFromPipeline();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted audio input pin", pipelineInputPin->getParameters()->name,
              "from pipeline", pipeline->getParameters()->name);
}


IasISetup::IasResult IasSetupImpl::addAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
    return eIasFailed;
  }
  if (pipelineOutputPin == nullptr)
  {
    /**
     * @log Invalid parameter: pipelineOutputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineOutputPin == nullptr");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot add audio output pin", pipelineOutputPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->addAudioOutputPin(pipelineOutputPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioOutputPin:", toString(result));
    return eIasFailed;
  }

  IasAudioPin::IasResult audioPinResult = pipelineOutputPin->addToPipeline(pipeline, pipeline->getParameters()->name);
  if (audioPinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasAudioPin::addToPipeline:", toString(audioPinResult));
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio output pin", pipelineOutputPin->getParameters()->name,
              "to pipeline", pipeline->getParameters()->name);

  return eIasOk;
}


void IasSetupImpl::deleteAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
  }
  if (pipelineOutputPin == nullptr)
  {
    /**
     * @log Invalid parameter: pipelineOutputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineOutputPin == nullptr");
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
  if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot delete audio output pin", pipelineOutputPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  pipeline->deleteAudioPin(pipelineOutputPin);
  pipelineOutputPin->deleteFromPipeline();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted audio output pin", pipelineOutputPin->getParameters()->name,
              "from pipeline", pipeline->getParameters()->name);
}


IasISetup::IasResult IasSetupImpl::addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  if (module == nullptr)
  {
    /**
     * @log Invalid parameter: module == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module == nullptr");
    return eIasFailed;
  }
  if (inOutPin == nullptr)
  {
    /**
     * @log Invalid parameter: inOutPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inOutPin == nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid configuration: module does not belong to a pipeline.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: module", module->getParameters()->instanceName,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot add audio input/output pin", inOutPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->addAudioInOutPin(module, inOutPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioInOutPin:", toString(result));
    return eIasFailed;
  }
  // The function call inOutPin->addToPipeline() is omitted here, because
  // this function will be called by the associated processing module.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio input/output pin", inOutPin->getParameters()->name,
              "to module", module->getParameters()->instanceName);

  return eIasOk;
}


void IasSetupImpl::deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  if ((module == nullptr) || (inOutPin == nullptr))
  {
    return;
  }
  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    return;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
  if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot delete audio input/output pin", inOutPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  pipeline->deleteAudioInOutPin(module, inOutPin);
  // The function call inOutPin->deleteFromPipeline() is omitted here, because
  // this function will be called by the associated processing module.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted audio input/output pin", inOutPin->getParameters()->name,
              "from module", module->getParameters()->instanceName);
}


IasISetup::IasResult IasSetupImpl::addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  if (module == nullptr)
  {
    /**
     * @log Invalid parameter: module == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module == nullptr");
    return eIasFailed;
  }
  if (inputPin == nullptr)
  {
    /**
     * @log Invalid parameter: inputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPin == nullptr");
    return eIasFailed;
  }
  if (outputPin == nullptr)
  {
    /**
     * @log Invalid parameter: outputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputPin == nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid configuration: module does not belong to a pipeline.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: module", module->getParameters()->instanceName,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot add pin mapping from", inputPin->getParameters()->name,
                "to", outputPin->getParameters()->name, "to module", module->getParameters()->instanceName,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->addAudioPinMapping(module, inputPin, outputPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioPinMapping:", toString(result));
    return eIasFailed;
  }
  // The function call inputPin->addToPipeline() is omitted here, because
  // this function will be called by the associated processing module.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added pin mapping of audio input pin", inputPin->getParameters()->name,
              "to audio output pin", outputPin->getParameters()->name, "to module", module->getParameters()->instanceName);

  return eIasOk;
}


void IasSetupImpl::deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  if ((module == nullptr) || (inputPin == nullptr) || (outputPin == nullptr))
  {
    return;
  }
  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    return;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
  if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot delete pin mapping from", inputPin->getParameters()->name,
                "to", outputPin->getParameters()->name, "from module", module->getParameters()->instanceName,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  pipeline->deleteAudioPinMapping(module, inputPin, outputPin);
  // The function call inputPin->deleteFromPipeline() is omitted here, because
  // this function will be called by the associated processing module.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted pin mapping of audio input pin", inputPin->getParameters()->name,
              "to audio output pin", outputPin->getParameters()->name, "from module", module->getParameters()->instanceName);
}


IasISetup::IasResult IasSetupImpl::createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module)
{
  if (module == nullptr)
  {
    /**
     * @log Destination for return value is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module == nullptr");
    return eIasFailed;
  }
  if (params.typeName.empty())
  {
    /**
     * @log Processing module typeName of initialization structure IasProcessingModuleParams is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: typeName may not be an empty string");
    return eIasFailed;
  }
  if (params.instanceName.empty())
  {
    /**
     * @log Processing module instanceName of initialization structure IasProcessingModuleParams is undefined.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: instanceName may not be an empty string");
    return eIasFailed;
  }

  IasProcessingModuleParamsPtr moduleParams = std::make_shared<IasProcessingModuleParams>();
  IAS_ASSERT(moduleParams != nullptr);
  *moduleParams = params;
  IasProcessingModulePtr newModule = std::make_shared<IasProcessingModule>(moduleParams);
  IAS_ASSERT(newModule != nullptr);
  *module = newModule;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created processing module", params.instanceName,
              "of type", params.typeName);

  return eIasOk;
}

void IasSetupImpl::destroyProcessingModule(IasProcessingModulePtr *module)
{
  if (module == nullptr)
  {
    return;
  }

  const IasProcessingModuleParamsPtr params = (*module)->getParameters();

  // Get the handle of the pipeline that owns the module and delete the module from the pipeline.
  const IasPipelinePtr pipeline = (*module)->getPipeline();
  if (pipeline != nullptr)
  {
    // Verify that the routing zone is not running
    IasRoutingZonePtr routingZone;
    IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
    if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                  "Cannot destroy processing module", params->instanceName,
                  "when routing zone", routingZone->getName(), "is running" );
      return;
    }

    pipeline->deleteProcessingModule(*module);
  }
  (*module)->deleteFromPipeline();
  module->reset();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed processing module", params->instanceName,
              "of type", params->typeName);
}

IasISetup::IasResult IasSetupImpl::addProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
    return eIasFailed;
  }
  if (module == nullptr)
  {
    /**
     * @log Invalid parameter: module == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module == nullptr");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot add processing module", module->getParameters()->instanceName,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  IasPipeline::IasResult pipelineResult = pipeline->addProcessingModule(module);
  if (pipelineResult != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addProcessingModule:", toString(pipelineResult));
    return eIasFailed;
  }

  IasProcessingModule::IasResult result = module->addToPipeline(pipeline, pipeline->getParameters()->name);
  if (result != IasProcessingModule::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while adding module to pipeline:", toString(result));
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added processing module", module->getParameters()->instanceName,
              "to pipeline", pipeline->getParameters()->name);

  return eIasOk;
}

void IasSetupImpl::deleteProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
  }
  if (module == nullptr)
  {
    /**
     * @log Invalid parameter: module == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module == nullptr");
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult result = mConfig->getRoutingZone(pipeline, &routingZone);
  if (result == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot delete processing module", module->getParameters()->instanceName,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  pipeline->deleteProcessingModule(module);
  module->deleteFromPipeline();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully deleted processing module", module->getParameters()->instanceName,
              "from pipeline", pipeline->getParameters()->name);
}

IasISetup::IasResult IasSetupImpl::link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  if (inputPort == nullptr)
  {
    /**
     * @log Invalid parameter: inputPort == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPort == nullptr");
    return eIasFailed;
  }
  if (pipelinePin == nullptr)
  {
    /**
     * @log Invalid parameter: pipelinePin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelinePin == nullptr");
    return eIasFailed;
  }

  IasAudioPortParamsPtr inputPortParams   = inputPort->getParameters();
  IasAudioPinParamsPtr  pipelinePinParams = pipelinePin->getParameters();
  IAS_ASSERT(inputPortParams   != nullptr); // already checked in constructor of IasAudioPort.
  IAS_ASSERT(pipelinePinParams != nullptr); // already checked in constructor of IasAudioPin.
  const std::string& inputPortName = inputPortParams->name;
  const std::string& pinName       = pipelinePinParams->name;

  // Verify that inputPort has been configured as input port.
  if (inputPortParams->direction != eIasPortDirectionInput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid port configuration,", inputPortName,
                "has invalid direction:", toString(inputPortParams->direction),
                "(should be eIasPortDirectionInput)");
    return eIasFailed;
  }

  IasAudioPortOwnerPtr portOwner;
  inputPort->getOwner(&portOwner);
  if (portOwner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Owner of inputPort", inputPortName,
                "cannot be determined");
    return eIasFailed;
  }

  // Verify that the pipelinePin has been added to a pipeline.
  IasPipelinePtr pipeline = pipelinePin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Associated pipeline of pipelinePin", pinName,
                "cannot be determined");
    return eIasFailed;
  }

  IasSmartXClientPtr smartXClient;
  IasAlsaHandlerPtr  alsaHandler;
  portOwner->getConcreteDevice(&smartXClient);
  portOwner->getConcreteDevice(&alsaHandler);
  IasAudioPin::IasPinDirection pinDirection = pipelinePin->getDirection();

  // Decide whether the inputPort is a routing zone input port or a sink device input port.
  // If the owner of inputPort is neither smartXClient nor alsaHandler, the port must belong to a routing zone.
  bool isRoutingZoneInput = ((smartXClient == nullptr) && (alsaHandler == nullptr));

  if (isRoutingZoneInput)
  {
    // If the inputPort is a routing zone input port, the audio pin must be input of a pipeline.
    if (pinDirection != IasAudioPin::eIasPinDirectionPipelineInput)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot link inputPort", inputPortName,
                  "with pipeline pin", pinName,
                  "because it has direction", toString(pinDirection));
      return eIasFailed;
    }
  }
  else
  {
    // If the inputPort is a sink device input port, the audio pin must be output of a pipeline.
    if (pinDirection != IasAudioPin::eIasPinDirectionPipelineOutput)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot link inputPort", inputPortName,
                  "with pipeline pin", pinName,
                  "because it has direction", toString(pinDirection));
      return eIasFailed;
    }
  }

  // Verify that the number of channels of the inputPort and of the pipelinePin are equal.
  if (inputPortParams->numChannels != pipelinePinParams->numChannels)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot link inputPort", inputPortName,
                "with pipeline pin", pinName,
                "since channel numbers do not match");
    return eIasFailed;
  }

  IasRoutingZonePtr routingZone = portOwner->getRoutingZone();
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot find the routing zone that owns the port", inputPortParams->name);
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  if (isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot link input port", inputPortName, "and pipeline pin", pinName,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  // If the audio port is at the input of a routing zone, check the data format of the associated ring buffer.
  if (isRoutingZoneInput)
  {
    IasAudioRingBuffer* ringBuffer = nullptr;
    IasAudioPort::IasResult portResult = inputPort->getRingBuffer(&ringBuffer);
    if (portResult != IasAudioPort::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasAudioPort::getRingBuffer.");
      return eIasFailed;
    }
    IAS_ASSERT(ringBuffer != nullptr); // already checked in IasAudioPort::getRingBuffer.

    IasAudioCommonDataFormat dataFormat;
    ringBuffer->getDataFormat(&dataFormat);

    // If the data format is not Float32, destroy the ring buffer and create a new ring buffer with Float32 format.
    // By means of this, the input port has the same data format as the pipeline, so that we avoid data conversions.
    if (dataFormat != IasAudioCommonDataFormat::eIasFormatFloat32)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Ring buffer for port", inputPortParams->name,
                  "needs to be re-created, since its current data format is", toString(dataFormat));

      inputPort->clearRingBuffer();
      IasRoutingZone::IasResult rznResult = routingZone->destroyConversionBuffer(inputPort);
      if (rznResult != IasRoutingZone::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Error during IasRoutingZone::destroyConversionBuffer", toString(rznResult));
        return eIasFailed;
      }

      // Re-create the conversion buffer using the data format Float32.
      rznResult = routingZone->createConversionBuffer(inputPort, IasAudioCommonDataFormat::eIasFormatFloat32, &ringBuffer);
      if (rznResult != IasRoutingZone::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during IasRoutingZone::createConversionBuffer");
        return eIasFailed;
      }
      IAS_ASSERT(ringBuffer != nullptr); // ringBuffer is always != nullptr after createConversionBuffer successfully returned

      portResult = inputPort->setRingBuffer(ringBuffer);
      IAS_ASSERT(portResult == IasAudioPort::eIasOk); // cannot happen if ringBuffer != nullptr
      (void)portResult;
    }
  }
  else
  {
    // If the audio port is at the input of a sink device, check if it is a sink device of the own routing zone
    // or of a derived routing zone. In case of a derived routing zone, the routing zone parameters
    // period time, period multiplier and sample rate have to match exactly

    if (routingZone->hasPipeline(pipeline) == false)
    {
      if (routingZone->isDerivedZone() == true)
      {
        // The pipeline is in a different routing zone than the one from the sink device.
        // Now we check if the base routing zone of the sink devices routing zone owns
        // that pipeline
        IAS_ASSERT(routingZone->getBaseZone() != nullptr);
        if (routingZone->getBaseZone()->hasPipeline(pipeline) == false)
        {
          // It is also not the pipeline of the base routing zone. This use-case is not allowed.
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Connecting an output pin of a pipeline to an input port of a sink device, which is not linked to a derived zone is not allowed");
          return eIasFailed;
        }
        // So now we have the use-case that a pipeline output pin, which resides in a base zone, shall be
        // connected to an input port of a sink device, which resides in a different routing zone, that is
        // however a derived routing zone of the base zone containing the pipeline.
        // Check if the routing zone parameters match exactly with the derived routing zone parameters
        IasRoutingZone::IasResult rznResult;
        rznResult = routingZone->addBasePipeline(pipeline);
        if (rznResult != IasRoutingZone::eIasOk)
        {
          // Printouts are already done in addBasePipeline method
          return eIasFailed;
        }
      }
      else
      {
        // The routing zone of the sink device is a base zone, but a different one then that of the pipeline. This use-case is not allowed.
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Either the pipeline was not added to a routing zone yet, or the sink device of the given port is linked to a different base routing zone.");
        return eIasFailed;
      }
    }
  }

  // Call the link method of the pipeline that belongs to the pipelinePin.
  IasPipeline::IasResult pipelineResult = pipeline->link(inputPort, pipelinePin);
  if (pipelineResult != IasPipeline::eIasOk)
  {
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully linked input port", inputPort->getParameters()->name,
              "to pipeline pin", pipelinePin->getParameters()->name);

  return eIasOk;
}

void IasSetupImpl::unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  if (inputPort == nullptr)
  {
    /**
     * @log Invalid parameter: inputPort == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPort == nullptr");
    return;
  }
  if (pipelinePin == nullptr)
  {
    /**
     * @log Invalid parameter: pipelinePin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelinePin == nullptr");
    return;
  }

  IasAudioPinParamsPtr  pipelinePinParams = pipelinePin->getParameters();
  IAS_ASSERT(pipelinePinParams != nullptr); // already checked in constructor of IasAudioPin.
  const std::string& pinName       = pipelinePinParams->name;

  // Verify that the pipelinePin has been added to a pipeline.
  IasPipelinePtr pipeline = pipelinePin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Associated pipeline of pipelinePin", pinName,
                "cannot be determined");
    return;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot unlink input port", inputPort->getParameters()->name, "and pipeline pin", pinName,
                "when routing zone", routingZone->getName(), "is running" );
    return;
  }

  // Call the unlink method of the pipeline that belongs to the pipelinePin.
  pipeline->unlink(inputPort, pipelinePin);
}

IasISetup::IasResult IasSetupImpl::link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType)
{
  if (outputPin == nullptr)
  {
    /**
     * @log Invalid parameter: outputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputPin == nullptr");
  }

  if (inputPin == nullptr)
  {
    /**
     * @log Invalid parameter: inputPin == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPin == nullptr");
  }

  IasPipelinePtr pipeline1 = outputPin->getPipeline();
  IasPipelinePtr pipeline2 = inputPin->getPipeline();

  if (pipeline1 == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: output pin", outputPin->getParameters()->name,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  if (pipeline2 == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: input pin", inputPin->getParameters()->name,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  if (pipeline1 != pipeline2)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: input pin", inputPin->getParameters()->name,
                "and output pin", outputPin->getParameters()->name,
                "belong to different pipelines.");
    return eIasFailed;
  }

  // Verify that the routing zone is not running
  IasRoutingZonePtr routingZone;
  IasConfiguration::IasResult cfgres = mConfig->getRoutingZone(pipeline1, &routingZone);
  if (cfgres == IasConfiguration::eIasOk && routingZone != nullptr && isRoutingZoneActive(routingZone))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Cannot link pins", outputPin->getParameters()->name,
                "and", inputPin->getParameters()->name,
                "when routing zone", routingZone->getName(), "is running" );
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline1->link(outputPin, inputPin, linkType);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during IasPipeline::link()");
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully linked output pin", outputPin->getParameters()->name,
              "to input pin", inputPin->getParameters()->name, "with link type", toString(linkType));

  return eIasOk;
}


void IasSetupImpl::unlink(IasAudioPinPtr /*outputPin*/, IasAudioPinPtr /*inputPin*/)
{
}

void IasSetupImpl::setProperties(IasProcessingModulePtr module, const IasProperties &properties)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "module == nullptr");
    return;
  }
  mConfig->setPropertiesForModule(module, properties);
}


IasPropertiesPtr IasSetupImpl::getProperties(IasProcessingModulePtr module)
{

  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "module == nullptr");
    return nullptr;
  }
  //IasPropertiesPtr moduleProperties = mConfig->getPropertiesForModule(module);
  return mConfig->getPropertiesForModule(module);// moduleProperties;
  //return;
}

IasISetup::IasResult IasSetupImpl::initPipelineAudioChain(IasPipelinePtr pipeline)
{
  if (pipeline == nullptr)
  {
    /**
     * @log Invalid parameter: pipeline == nullptr.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline == nullptr");
  }

  IasPipeline::IasResult pipelineResult = pipeline->initAudioChain();
  if (pipelineResult != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::initAudioChain:", toString(pipelineResult));
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully initialized pipeline audio chain of pipeline", pipeline->getParameters()->name);


  return eIasOk;
}

bool IasSetupImpl::verifyBufferSize(const IasAudioDeviceParams &params)
{

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "checking needed buffer bize for new device");

  std::uint64_t neededBuffersizeBytes =  static_cast<std::uint64_t>(params.periodSize) * static_cast<std::uint64_t>(params.numPeriods);
  if (neededBuffersizeBytes > static_cast<std::uint64_t>(mMaxDataBuffSize))
  {
    return false;
  }
  neededBuffersizeBytes *= params.numChannels;
  if (neededBuffersizeBytes > static_cast<std::uint64_t>(mMaxDataBuffSize))
  {
    return false;
  }
  neededBuffersizeBytes *= toSize(params.dataFormat);
  if (neededBuffersizeBytes > static_cast<std::uint64_t>(mMaxDataBuffSize))
  {
    return false;
  }
  return true;
}



} //namespace IasAudio
