/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSetupImpl.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSETUPIMPL_HPP
#define IASSETUPIMPL_HPP


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

class IasConfiguration;
class IasIRouting;

#define MAX_DATA_BUFFER_SIZE_DEVICE 4194304 // 4 MB of data buffer allowed per device

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasSetupImpl : public IasISetup
{

public:
    /**
     * @brief Constructor.
     */
    IasSetupImpl(IasConfigurationPtr config, IasCmdDispatcherPtr cmdDispatcher, IasIRouting* routing);
    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasSetupImpl();

    virtual IasResult createRoutingZone(const IasRoutingZoneParams &params, IasRoutingZonePtr *routingZone);
    virtual void destroyRoutingZone(IasRoutingZonePtr routingZone);
    virtual IasResult startRoutingZone(IasRoutingZonePtr routingZone);
    virtual void stopRoutingZone(IasRoutingZonePtr routingZone);

    virtual IasResult createAudioSourceDevice(const IasAudioDeviceParams &params, IasAudioSourceDevicePtr *audioSourceDevice);
    virtual void destroyAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice);
    virtual IasResult startAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice);
    virtual void stopAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice);

    virtual IasResult createAudioSinkDevice(const IasAudioDeviceParams &params, IasAudioSinkDevicePtr *audioSinkDevice);
    virtual void destroyAudioSinkDevice(IasAudioSinkDevicePtr audioSinkDevice);

    virtual IasResult createAudioPort(const IasAudioPortParams &params, IasAudioPortPtr *audioPort);
    virtual void destroyAudioPort(IasAudioPortPtr audioPort);

    virtual IasResult addDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone);
    virtual void deleteDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone);

    virtual IasResult link(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice);
    virtual void unlink(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice);

    virtual IasResult addAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort);
    virtual void deleteAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort);

    virtual IasResult addAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort);
    virtual void deleteAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort);

    virtual IasResult addAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort);
    virtual void deleteAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort);

    virtual IasResult link(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort);
    virtual void unlink(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort);

    virtual IasAudioPortVector getAudioInputPorts() const;
    virtual IasAudioPortVector getAudioOutputPorts() const;
    virtual IasAudioPortVector getAudioPorts() const;
    virtual IasAudioPinVector getAudioPins() const;

    virtual IasRoutingZoneVector getRoutingZones() const;
    virtual IasAudioSourceDeviceVector getAudioSourceDevices() const;
    virtual IasAudioSinkDeviceVector getAudioSinkDevices() const;
    virtual IasRoutingZonePtr getRoutingZone(const std::string &name);
    virtual IasAudioSourceDevicePtr getAudioSourceDevice(const std::string &name);
    virtual IasAudioSinkDevicePtr getAudioSinkDevice(const std::string &name);
    virtual IasPipelinePtr getPipeline(const std::string &name);

    virtual void addSourceGroup (const std::string &name, int32_t id);
    //todo add delete function
    virtual IasSourceGroupMap getSourceGroups();
    virtual IasResult createPipeline(const IasPipelineParams &params, IasPipelinePtr *pipeline);
    virtual void destroyPipeline(IasPipelinePtr *pipeline);
    virtual IasResult addPipeline(IasRoutingZonePtr routingZone, IasPipelinePtr pipeline);
    virtual void deletePipeline(IasRoutingZonePtr routingZone);
    virtual IasResult createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin);
    virtual void destroyAudioPin(IasAudioPinPtr *pin);
    virtual IasResult addAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin);
    virtual void deleteAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin);
    virtual IasResult addAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin);
    virtual void deleteAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin);
    virtual IasResult addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin);
    virtual void deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin);
    virtual IasResult addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);
    virtual void deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);
    virtual IasResult createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module);
    virtual void destroyProcessingModule(IasProcessingModulePtr *module);
    virtual IasResult addProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module);
    virtual void deleteProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module);
    virtual IasResult link(IasAudioPortPtr inputPort, IasAudioPinPtr inputPin);
    virtual void unlink(IasAudioPortPtr inputPort, IasAudioPinPtr inputPin);
    virtual IasResult link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType);
    virtual void unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin);
    virtual void setProperties(IasProcessingModulePtr module, const IasProperties &properties);
    virtual IasPropertiesPtr getProperties(IasProcessingModulePtr module);
    virtual IasResult initPipelineAudioChain(IasPipelinePtr pipeline);

private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSetupImpl(IasSetupImpl const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSetupImpl& operator=(IasSetupImpl const &other);

    template <typename IasDeviceClass, typename IasConcreteType>
    IasResult createConcreteAudioDevice(const IasAudioDeviceParams &params, std::shared_ptr<IasDeviceClass> *device, IasDeviceType deviceType);

    template <typename IasDeviceClass>
    IasResult createAudioDevice(const IasAudioDeviceParams &params, std::shared_ptr<IasDeviceClass> *device, IasDeviceType deviceType);

    bool verifyBufferSize(const IasAudioDeviceParams &params);

    IasConfigurationPtr     mConfig;
    IasCmdDispatcherPtr     mCmdDispatcher;
    IasPluginEnginePtr      mPluginEngine;
    DltContext             *mLog;
    unsigned int            mMaxDataBuffSize;
    IasIRouting            *mRouting;
};

} //namespace IasAudio

#endif // IASSETUPIMPL_HPP
