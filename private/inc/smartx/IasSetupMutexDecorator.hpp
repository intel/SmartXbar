/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasSetupMutexDecorator.hpp
 * @date 2017
 * @brief This is the declaration of a decorator for the IasISetup instance
 */

#ifndef IASSETUPMUTEXDECORATOR_HPP_
#define IASSETUPMUTEXDECORATOR_HPP_

#include "audio/smartx/IasISetup.hpp"

namespace IasAudio {

/**
 * @brief Decorator for the IasISetup instance
 *
 * It adds a mutex around each method to make it safe while being accessed by multiple threads.
 */
class IAS_AUDIO_PUBLIC IasSetupMutexDecorator : public IasISetup
{
  public:
      /**
       * @brief Constructor
       *
       * @param[in] setup   Pointer to the IasISetup instance to be decorated
       */
      IasSetupMutexDecorator(IasISetup *setup);

      /**
       * @brief Destructor
       */
      virtual ~IasSetupMutexDecorator();

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createRoutingZone(const IasRoutingZoneParams &params, IasRoutingZonePtr *routingZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyRoutingZone(IasRoutingZonePtr routingZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult startRoutingZone(IasRoutingZonePtr routingZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void stopRoutingZone(IasRoutingZonePtr routingZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createAudioSourceDevice(const IasAudioDeviceParams &params, IasAudioSourceDevicePtr *audioSourceDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult startAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void stopAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createAudioSinkDevice(const IasAudioDeviceParams &params, IasAudioSinkDevicePtr *audioSinkDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyAudioSinkDevice(IasAudioSinkDevicePtr audioSinkDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createAudioPort(const IasAudioPortParams &params, IasAudioPortPtr *audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyAudioPort(IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult link(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void unlink(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult link(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void unlink(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioPortVector getAudioInputPorts() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioPortVector getAudioOutputPorts() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioPortVector getAudioPorts() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioPinVector getAudioPins() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasRoutingZoneVector getRoutingZones() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioSourceDeviceVector getAudioSourceDevices() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioSinkDeviceVector getAudioSinkDevices() const override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasRoutingZonePtr getRoutingZone(const std::string &name) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioSourceDevicePtr getAudioSourceDevice(const std::string &name) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasAudioSinkDevicePtr getAudioSinkDevice(const std::string &name) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasPipelinePtr getPipeline(const std::string &name) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void addSourceGroup (const std::string &name, std::int32_t id) override;

      //todo add delete function
      /**
        * @brief Inherited from IasISetup.
        */
      IasSourceGroupMap getSourceGroups() override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createPipeline(const IasPipelineParams &params, IasPipelinePtr *pipeline) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyPipeline(IasPipelinePtr *pipeline) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addPipeline(IasRoutingZonePtr routingZone, IasPipelinePtr pipeline) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deletePipeline(IasRoutingZonePtr routingZone) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyAudioPin(IasAudioPinPtr *pin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void destroyProcessingModule(IasProcessingModulePtr *module) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult addProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void deleteProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult link(IasAudioPortPtr inputPort, IasAudioPinPtr inputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void unlink(IasAudioPortPtr inputPort, IasAudioPinPtr inputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin) override;

      /**
        * @brief Inherited from IasISetup.
        */
      void setProperties(IasProcessingModulePtr module, const IasProperties &properties) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasPropertiesPtr getProperties(IasProcessingModulePtr module) override;

      /**
        * @brief Inherited from IasISetup.
        */
      IasResult initPipelineAudioChain(IasPipelinePtr pipeline) override;

  private:
      /**
       * @brief Copy constructor, private deleted to prevent misuse.
       */
      IasSetupMutexDecorator(IasSetupMutexDecorator const &other) = delete;
      /**
       * @brief Assignment operator, private deleted to prevent misuse.
       */
      IasSetupMutexDecorator& operator=(IasSetupMutexDecorator const &other) = delete;

      IasISetup  *mSetup;         //!< Pointer to original IasISetup instance
};

} /* namespace IasAudio */

#endif /* IASSETUPMUTEXDECORATOR_HPP_ */
