/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasSetupMutexDecorator.cpp
 * @date 2017
 * @brief Definition of IasSetupMutexDecorator
 */

#include "smartx/IasSetupMutexDecorator.hpp"
#include "smartx/IasDecoratorGuard.hpp"

namespace IasAudio {

IasSetupMutexDecorator::IasSetupMutexDecorator(IasISetup *setup)
  :mSetup(setup)
{
}

IasSetupMutexDecorator::~IasSetupMutexDecorator()
{
  // original instance of IasISetup has to be deleted here, because the original
  // member variable was overwritten by the decorator implementation
  delete mSetup;
}

IasSetupMutexDecorator::IasResult IasSetupMutexDecorator::addAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioOutputPort(audioSourceDevice, audioPort);
}

void IasSetupMutexDecorator::deleteAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioOutputPort(audioSourceDevice, audioPort);
}

IasSetupMutexDecorator::IasResult IasSetupMutexDecorator::addAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioInputPort(audioSinkDevice, audioPort);
}

void IasSetupMutexDecorator::deleteAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioInputPort(audioSinkDevice, audioPort);
}

IasSetupMutexDecorator::IasResult IasSetupMutexDecorator::addAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioInputPort(routingZone, audioPort);
}

void IasSetupMutexDecorator::deleteAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioInputPort(routingZone, audioPort);
}

IasISetup::IasResult IasSetupMutexDecorator::addDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->addDerivedZone(baseZone, derivedZone);
}

void IasSetupMutexDecorator::deleteDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteDerivedZone(baseZone, derivedZone);
}

IasISetup::IasResult IasSetupMutexDecorator::createAudioPort(const IasAudioPortParams& params, IasAudioPortPtr* audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->createAudioPort(params, audioPort);
}

void IasSetupMutexDecorator::destroyAudioPort(IasAudioPortPtr audioPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyAudioPort(audioPort);
}

void IasSetupMutexDecorator::destroyAudioSinkDevice(IasAudioSinkDevicePtr audioSinkDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyAudioSinkDevice(audioSinkDevice);
}

IasISetup::IasResult IasSetupMutexDecorator::createAudioSourceDevice(const IasAudioDeviceParams& params, IasAudioSourceDevicePtr* audioSourceDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->createAudioSourceDevice(params, audioSourceDevice);
}

IasISetup::IasResult IasSetupMutexDecorator::createAudioSinkDevice(const IasAudioDeviceParams& params, IasAudioSinkDevicePtr* audioSinkDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->createAudioSinkDevice(params, audioSinkDevice);
}

void IasSetupMutexDecorator::destroyAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyAudioSourceDevice(audioSourceDevice);
}

IasISetup::IasResult IasSetupMutexDecorator::startAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->startAudioSourceDevice(audioSourceDevice);
}

void IasSetupMutexDecorator::stopAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->stopAudioSourceDevice(audioSourceDevice);
}

IasSetupMutexDecorator::IasResult IasSetupMutexDecorator::createRoutingZone(const IasRoutingZoneParams &params, IasRoutingZonePtr* routingZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->createRoutingZone(params, routingZone);
}

void IasSetupMutexDecorator::destroyRoutingZone(IasRoutingZonePtr routingZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyRoutingZone(routingZone);
}

IasISetup::IasResult IasSetupMutexDecorator::startRoutingZone(IasRoutingZonePtr routingZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->startRoutingZone(routingZone);
}

void IasSetupMutexDecorator::stopRoutingZone(IasRoutingZonePtr routingZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->stopRoutingZone(routingZone);
}

IasSetupMutexDecorator::IasResult IasSetupMutexDecorator::link(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->link(routingZone, audioSinkDevice);
}

void IasSetupMutexDecorator::unlink(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice)
{
  const IasDecoratorGuard lk{};
  return mSetup->unlink(routingZone, audioSinkDevice);
}

IasSetupMutexDecorator::IasResult IasSetupMutexDecorator::link(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->link(zoneInputPort, sinkDeviceInputPort);
}

void IasSetupMutexDecorator::unlink(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  const IasDecoratorGuard lk{};
  return mSetup->unlink(zoneInputPort, sinkDeviceInputPort);
}

IasAudioPortVector IasSetupMutexDecorator::getAudioPorts() const
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioPorts();
}

IasAudioPinVector IasSetupMutexDecorator::getAudioPins() const
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioPins();
}

IasAudioPortVector IasSetupMutexDecorator::getAudioInputPorts() const
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioInputPorts();
}

IasAudioPortVector IasSetupMutexDecorator::getAudioOutputPorts() const
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioOutputPorts();
}

IasRoutingZoneVector IasSetupMutexDecorator::getRoutingZones() const
{
  const IasDecoratorGuard lk{};;
  return mSetup->getRoutingZones();
}

IasAudioSourceDeviceVector IasSetupMutexDecorator::getAudioSourceDevices() const
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioSourceDevices();
}

IasAudioSinkDeviceVector IasSetupMutexDecorator::getAudioSinkDevices() const
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioSinkDevices();
}

IasRoutingZonePtr IasSetupMutexDecorator::getRoutingZone(const std::string &name)
{
  const IasDecoratorGuard lk{};
  return mSetup->getRoutingZone(name);
}

IasAudioSourceDevicePtr IasSetupMutexDecorator::getAudioSourceDevice(const std::string &name)
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioSourceDevice(name);
}

IasAudioSinkDevicePtr IasSetupMutexDecorator::getAudioSinkDevice(const std::string &name)
{
  const IasDecoratorGuard lk{};
  return mSetup->getAudioSinkDevice(name);
}

IasPipelinePtr IasSetupMutexDecorator::getPipeline(const std::string &name)
{
  const IasDecoratorGuard lk{};
  return mSetup->getPipeline(name);
}

void IasSetupMutexDecorator::addSourceGroup (const std::string &name, std::int32_t id)
{
  const IasDecoratorGuard lk{};
  return mSetup->addSourceGroup (name, id);
}

IasSourceGroupMap IasSetupMutexDecorator::getSourceGroups()
{
  const IasDecoratorGuard lk{};
  return mSetup->getSourceGroups();
}

IasISetup::IasResult IasSetupMutexDecorator::createPipeline(const IasPipelineParams &params, IasPipelinePtr *pipeline)
{
  const IasDecoratorGuard lk{};
  return mSetup->createPipeline(params, pipeline);
}

void IasSetupMutexDecorator::destroyPipeline(IasPipelinePtr *pipeline)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyPipeline(pipeline);
}

IasISetup::IasResult IasSetupMutexDecorator::addPipeline(IasRoutingZonePtr routingZone, IasPipelinePtr pipeline)
{
  const IasDecoratorGuard lk{};
  return mSetup->addPipeline(routingZone, pipeline);
}

void IasSetupMutexDecorator::deletePipeline(IasRoutingZonePtr routingZone)
{
  const IasDecoratorGuard lk{};
  return mSetup->deletePipeline(routingZone);
}

IasISetup::IasResult IasSetupMutexDecorator::createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin)
{
  const IasDecoratorGuard lk{};
  return mSetup->createAudioPin(params, pin);
}

void IasSetupMutexDecorator::destroyAudioPin(IasAudioPinPtr *pin)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyAudioPin(pin);
}

IasISetup::IasResult IasSetupMutexDecorator::addAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioInputPin(pipeline, pipelineInputPin);
}

void IasSetupMutexDecorator::deleteAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioInputPin(pipeline, pipelineInputPin);
}

IasISetup::IasResult IasSetupMutexDecorator::addAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioOutputPin(pipeline, pipelineOutputPin);
}

void IasSetupMutexDecorator::deleteAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioOutputPin(pipeline, pipelineOutputPin);
}

IasISetup::IasResult IasSetupMutexDecorator::addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioInOutPin(module, inOutPin);
}

void IasSetupMutexDecorator::deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioInOutPin(module, inOutPin);
}

IasISetup::IasResult IasSetupMutexDecorator::addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->addAudioPinMapping(module, inputPin, outputPin);
}

void IasSetupMutexDecorator::deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteAudioPinMapping(module, inputPin, outputPin);
}

IasISetup::IasResult IasSetupMutexDecorator::createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module)
{
  const IasDecoratorGuard lk{};
  return mSetup->createProcessingModule(params, module);
}

void IasSetupMutexDecorator::destroyProcessingModule(IasProcessingModulePtr *module)
{
  const IasDecoratorGuard lk{};
  return mSetup->destroyProcessingModule(module);
}

IasISetup::IasResult IasSetupMutexDecorator::addProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module)
{
  const IasDecoratorGuard lk{};
  return mSetup->addProcessingModule(pipeline, module);
}

void IasSetupMutexDecorator::deleteProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module)
{
  const IasDecoratorGuard lk{};
  return mSetup->deleteProcessingModule(pipeline, module);
}

IasISetup::IasResult IasSetupMutexDecorator::link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  const IasDecoratorGuard lk{};
  return mSetup->link(inputPort, pipelinePin);
}

void IasSetupMutexDecorator::unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  const IasDecoratorGuard lk{};
  return mSetup->unlink(inputPort, pipelinePin);
}

IasISetup::IasResult IasSetupMutexDecorator::link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType)
{
  const IasDecoratorGuard lk{};
  return mSetup->link(outputPin, inputPin, linkType);
}

void IasSetupMutexDecorator::unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin)
{
  const IasDecoratorGuard lk{};
  return mSetup->unlink(outputPin,  inputPin);
}

void IasSetupMutexDecorator::setProperties(IasProcessingModulePtr module, const IasProperties &properties)
{
  const IasDecoratorGuard lk{};
  return mSetup->setProperties(module, properties);
}

IasPropertiesPtr IasSetupMutexDecorator::getProperties(IasProcessingModulePtr module)
{
  const IasDecoratorGuard lk{};
  return mSetup->getProperties(module);
}

IasISetup::IasResult IasSetupMutexDecorator::initPipelineAudioChain(IasPipelinePtr pipeline)
{
  const IasDecoratorGuard lk{};
  return mSetup->initPipelineAudioChain(pipeline);
}


} /* namespace IasAudio */
