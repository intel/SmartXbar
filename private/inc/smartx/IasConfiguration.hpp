/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfiguration.hpp
 * @date   2015
 * @brief
 */

#ifndef IASCONFIGURATION_HPP
#define IASCONFIGURATION_HPP


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "IasAudioTypedefs.hpp"

namespace IasAudio {

/**
 * @brief Map a device name to an audio source device
 */
using IasSourceDeviceMap = std::map<std::string, IasAudioSourceDevicePtr>;

/**
 * @brief Map a device name to an audio sink device
 */
using IasSinkDeviceMap = std::map<std::string, IasAudioSinkDevicePtr>;

/**
 * @brief Map a routing zone name to a routing zone instance
 */
using IasRoutingZoneMap = std::map<std::string, IasRoutingZonePtr>;

/**
 * @brief Map a pipeline name to a pipeline instance
 */
using IasPipelineMap = std::map<std::string, IasPipelinePtr>;

/**
 * @brief Map a Id to a port
 */
using IasIdPortMap = std::map<int32_t, IasAudioPortPtr>;

/**
 * @brief Map a string to an audio port
 */
using IasPortMap = std::map<std::string, IasAudioPortPtr>;

/**
 * @brief Map a string to an audio pin
 */
using IasPinMap = std::map<std::string, IasAudioPinPtr>;

/**
 * @brief map containing the logical source groups
 */
using IasLogicalSourceMap = std::map<std::string,std::set<int32_t>>;

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasConfiguration
{
  public:
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed,                 //!< Operation failed
      eIasNullPointer,            //!< One of the parameters was a nullptr
      eIasObjectAlreadyExists,    //!< Object already exists
      eIasObjectNotFound,         //!< Object not found
    };

    /**
     * @brief Constructor.
     */
    IasConfiguration();
    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasConfiguration();

    IasResult addAudioDevice(const std::string &name, IasAudioSourceDevicePtr sourceDevice);
    IasResult addAudioDevice(const std::string &name, IasAudioSinkDevicePtr sinkDevice);
    IasResult getAudioDevice(const std::string &name, IasAudioSourceDevicePtr *sourceDevice);
    IasResult getAudioDevice(const std::string &name, IasAudioSinkDevicePtr *sinkDevice);
    void deleteAudioDevice(const std::string &name);

    const IasSourceDeviceMap& getSourceDeviceMap() const { return mSourceDeviceMap; }
    const IasSinkDeviceMap& getSinkDeviceMap() const { return mSinkDeviceMap; }

    IasResult addRoutingZone(const std::string &name, IasRoutingZonePtr routingZone);
    IasResult getRoutingZone(const std::string &name, IasRoutingZonePtr *routingZone);
    IasResult getRoutingZone(const IasPipelinePtr pipeline, IasRoutingZonePtr *routingZone);
    void deleteRoutingZone(const std::string &name);
    const IasRoutingZoneMap& getRoutingZoneMap() const { return mRoutingZoneMap; }

    IasResult addPipeline(const std::string &name, IasPipelinePtr pipeline);
    IasResult getPipeline(const std::string &name, IasPipelinePtr *pipeline);
    void deletePipeline(const std::string &name);
    const IasPipelineMap& getPipelineMap() const { return mPipelineMap; }

    IasResult addPort(IasAudioPortPtr port);
    IasResult addPin(IasAudioPinPtr pin);
    void removePin(std::string name);

    IasResult getOutputPort(int32_t sourceId, IasAudioPortPtr *port);
    IasResult getPortByName(const std::string &name, IasAudioPortPtr *port);
    IasResult getPinByName(const std::string &name,IasAudioPinPtr *pin);

    void deleteOutputPort(int32_t sourceId);
    const IasIdPortMap& getOutputPortMap() const { return mOutputPortMap; }

    IasResult getInputPort(int32_t sinkId, IasAudioPortPtr *port);
    void deleteInputPort(int32_t sinkId);
    const IasIdPortMap& getInputPortMap() const { return mInputPortMap; }
    const IasPortMap& getPortMap() const { return mPortMap; }
    const IasPinMap& getPinMap() const { return mPinMap; }

    void deletePortByName(std::string name);

    uint32_t getNumberSourceDevices() const;
    uint32_t getNumberSinkDevices() const;
    uint32_t getNumberRoutingZones() const;
    uint32_t getNumberPipelines() const;

    void addLogicalSource(std::string, int32_t);
    IasSourceGroupMap getSourceGroups(){return mLogicalSourceMap;};
    std::set<int32_t> findGroupedSourceIds(int32_t sourceId);

    IasPropertiesPtr getPropertiesForModule(IasProcessingModulePtr module);
    void setPropertiesForModule(IasProcessingModulePtr module, const IasProperties &properties);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasConfiguration(IasConfiguration const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasConfiguration& operator=(IasConfiguration const &other);

    using IasModulePropertiesMap = std::map<IasProcessingModulePtr, IasPropertiesPtr>;

    DltContext               *mLog;
    IasSourceDeviceMap        mSourceDeviceMap;
    IasSinkDeviceMap          mSinkDeviceMap;
    IasRoutingZoneMap         mRoutingZoneMap;
    IasPipelineMap            mPipelineMap;
    IasIdPortMap              mOutputPortMap;
    IasIdPortMap              mInputPortMap;
    IasLogicalSourceMap       mLogicalSourceMap;
    IasPortMap                mPortMap;
    IasModulePropertiesMap    mModulePropertiesMap;
    IasPinMap                 mPinMap;
};

/**
 * @brief Function to get a IasISetup::IasResult as string.
 *
 * @return Enum Member
 */
std::string toString(const IasConfiguration::IasResult& type);


} //namespace IasAudio

#endif // IASCONFIGURATION_HPP
