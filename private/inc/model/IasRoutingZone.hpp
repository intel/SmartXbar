/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRoutingZone.hpp
 * @date   2015
 * @brief
 */

#ifndef IASROUTINGZONE_HPP
#define IASROUTINGZONE_HPP


#include "audio/common/IasAudioCommonTypes.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "model/IasAudioPortOwner.hpp"

namespace IasAudio {

// Forward declararation of worker thread classes.
class IasSwitchMatrix;
class IasAudioRingBuffer;


/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasRoutingZone : public IasAudioPortOwner
{
  public:
    /**
     * @brief  Result type of the class IasRoutingZone.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasFailed,           //!< other error
    };

    /**
     * @brief Constructor.
     */
    IasRoutingZone(IasRoutingZoneParamsPtr params);
    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasRoutingZone();

    IasResult init();

    const IasRoutingZoneParamsPtr getRoutingZoneParams() const;

    /**
     * @brief Get the shared pointer to this routing zone object.
     *
     * The shared pointer has to be set by means of the method IasRoutingZone::setRoutingZone, before this function can be used.
     *
     * @returns Shared pointer that points to this routing zone.
     */
    virtual IasRoutingZonePtr getRoutingZone() const {return mRoutingZonePtr;}

    /**
     * @brief Set the shared pointer that is pointing to the routing zone.
     *
     * This is the pointer that will be returned if the method IasRoutingZone::getRoutingZone is called.
     *
     * @param[in] routingZonePtr  Shared pointer that points to this routing zone.
     */
    void setRoutingZone(IasRoutingZonePtr routingZonePtr) { mRoutingZonePtr = routingZonePtr; }

    /**
     * @brief Actual implementation of the virtual isRoutingZone() method of the base class IasAudioPortOwner.
     *
     * @returns always true, because this object is actually a routing zone.
     */
    virtual bool isRoutingZone() const {return true; }

    /**
     * @brief Link an audio sink device to this routing zone.
     *
     * @param[in] sinkDevice  Handle to the audio sink device that shall be linked to this routing zone.
     * @return                The result of the call.
     */
    IasResult linkAudioSinkDevice(IasAudioSinkDevicePtr sinkDevice);

    /**
     * @brief Unlink the audio sink device that is currently linked to the routing zone.
     */
    void unlinkAudioSinkDevice();

    /**
     * @brief Return boolean flag indicating whether the routing zone has a link to an audio sink device.
     */
    bool hasLinkedAudioSinkDevice() const { return mSinkDevice != nullptr; }

    /**
     * @brief Get the audio sink device that has been linked to this routing zone.
     *
     * @returns Handle to the audio sink device or nullptr if there is not sink device that has been linked.
     */
    const IasAudioSinkDevicePtr getAudioSinkDevice() const { return mSinkDevice; }

    /**
     * @brief Add a new input port to the routing zone and create the conversion buffer for this input port.
     *
     * There must be a 1:1 relationship between the input ports and the associated conversion buffers.
     * Therefore, it is not allowed that an input port is added to a routing zone more than once.
     *
     * @param[in]  audioPort         The audio port that shall be added to the routing zone.
     * @param[in]  dataFormat        The data format that shall be used for the conversion buffer.
     *                               If the data format is undefined (i.e., IasAudioCommonDataFormat::eIasFormatUndef),
     *                               the conversion buffer will be crated using the dataFormat of the sink device.
     * @param[out] conversionBuffer  The conversion buffer that has been created by this method for the specified port.
     * @return                       The result of the function call.
     */
    IasResult createConversionBuffer(const IasAudioPortPtr    audioPort,
                                     IasAudioCommonDataFormat dataFormat,
                                     IasAudioRingBuffer**     conversionBuffer);


    IasResult destroyConversionBuffer(const IasAudioPortPtr audioPort);

    /**
     * @brief Link a zone input port to a sink device input port.
     *
     * This function controls how the routing zone transfers the PCM frames from the zone input ports
     * to the sink device input ports.
     *
     * @param[in] zoneInputPort The routing zone input port that shall be linked to the audio sink device input port
     * @param[in] sinkDeviceInputPort The audio sink device input port that shall be linked to the routing zone input port
     * @return The result of the call
     */
    IasResult linkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort);

    /**
     * @brief Returns sink device input port inside the routingzone.
     *
     * This function returns if a sink device input port is linked to a routingzone input port
     *
     * @param[in] zoneInputPort The routing zone input port that shall be linked to the audio sink device input port
     * @param[out] sinkDeviceInputPort The audio sink device input port that shall be linked to the routing zone input port
     * @return The result of the call
     */
    IasResult getLinkedSinkPort(IasAudioPortPtr zoneInputPort, IasAudioPortPtr *sinkDeviceInputPort);

    /*
     * @brief Unlink a zone input port from a sink device input port.
     *
     * @param[in] zoneInputPort The routing zone input port that shall be unlinked from the audio sink device input port
     * @param[in] sinkDeviceInputPort The audio sink device input port that shall be unlinked from the routing zone input port
     */
    void    unlinkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort);

    /**
     * @brief Add pipeline to the routing zone.
     *
     * There is only one pipeline per routing zone allowed. Adding a second pipeline will fail.
     *
     * @param[in] pipeline Pipeline that shall be added to routing zone.
     *
     * @return The result of the method call
     */
    IasResult addPipeline(IasPipelinePtr pipeline);

    /**
     * @brief Delete pipeline from the routing zone.
     */
    void      deletePipeline();

    /**
     * @brief Add pipeline of the base routing zone to this derived routing zone.
     *
     * There is only one base pipeline per derived routing zone allowed. Adding a second pipeline will fail.
     * This routing zone has to be a derived routing zone.
     *
     * @param[in] pipeline Pipeline of the base routing zone that shall be added to routing zone.
     *
     * @return The result of the method call
     */
    IasResult addBasePipeline(IasPipelinePtr pipeline);

    /**
     * @brief Delete base pipeline from the routing zone.
     */
    void      deleteBasePipeline();

    IasResult addDerivedZone(IasRoutingZonePtr derivedZone);
    void deleteDerivedZone(IasRoutingZonePtr derivedZone);

    IasResult start();
    IasResult stop();

    /**
     * @brief Declare the SwitchMatrix.
     *
     * The SwitchMatrix is required, since the worker thread of the
     * routing zone periodically calls the trigger() method of the associated
     * SwitchMatrix. Therefore, the routing zone must not be started,
     * before the associated SwitchMatrix is declared by means of
     * this method.
     */
    void setSwitchMatrix(IasSwitchMatrixPtr switchMatrix);

    /**
     * @brief Clear the SwitchMatrix.
     *
     * In addition, the worker thread of the routing zone is stopped, because
     * worker thread needs the association with a SwitchMatrix for
     * periodically calling its trigger() method.
     */
    void clearSwitchMatrix();

    /**
     * @brief Get the worker thread object
     *
     * @returns Pointer to the worker thread object
     */
    IasRoutingZoneWorkerThreadPtr getWorkerThread() const {return mWorkerThread;}

    virtual uint32_t getSampleRate() const;
    virtual IasAudioCommonDataFormat getSampleFormat() const;
    virtual uint32_t getPeriodSize() const;
    virtual IasSwitchMatrixPtr getSwitchMatrix() const { return mSwitchMatrix; }

    /**
     * @brief Helper function to clear all derived zones of this routing zone
     */
    void clearDerivedZones();


    /**
     * @brief Helper function to clear the worker thread by setting the shared pointer to null
     */
    void clearWorkerThread() { mWorkerThread = nullptr;};

    /**
     * @brief Verify whether the routing zone is running.
     *
     * @returns Boolean flag whether the routing zone is running.
     */
    bool isActive() const;

    /**
     * @brief Verify whether the routing zone is active pending.
     *
     * @returns Boolean flag whether the routing zone is active pending
     */
    bool isActivePending() const;

    /**
     * @brief Get the name of the routing zone
     *
     * @returns The name of the routing zone as string
     */
    virtual std::string getName() const { return mParams->name; }

    /**
     * @brief Check if the given pipeline was added to this routing zone
     *
     * @param[in] pipeline The pipeline that shall be checked for existence
     *
     * @returns Boolean flag whether the given pipeline was added or not
     */
    bool hasPipeline(IasPipelinePtr pipeline) const;

    /**
     * @brief Get the pointer to the pipeline
     *
     * @returns returns pipline or nullptr
     */
    IasPipelinePtr getPipeline() const;

    /**
     * @brief Get the pointer to the base routing zone
     */
    IasRoutingZonePtr getBaseZone() const;

    /**
     * @brief Check whether this is a derived routing zone
     *
     * @returns Boolean flag whether this is a derived routing zone or not
     */
    bool isDerivedZone() const;

  private:
    /**
     * @brief Tell the routing zone whether it is a derived zone or not.
     *
     * Only if the zone is non-derived zone, it will start its worker thread.
     * The pointer to the base routing zone is only relevant and will be stored
     * when the zone shall become a derived zone. If isDerivedZone == false then
     * the baseZone parameter is ignored and the previously stored baseZone pointer
     * is cleared.
     *
     * @param[in] isDerivedZone true, if the zone shall become a derived zone, otherwise false
     * @param[in] baseZone The pointer to the base routing zone.
     *
     */
    void setZoneDerived(bool isDerivedZone, IasRoutingZonePtr baseZone);

    /**
     * @brief Set the isActive flag, indicating whether the routing zone is running.
     *
     * @param[in] isActive Flag, must be true, if the routing zone shall be marked as running, or false otherwise.
     */
    void setActive(bool isActive);


    IasResult prepareStates();


    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasRoutingZone(IasRoutingZone const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasRoutingZone& operator=(IasRoutingZone const &other);

    /**
     * @brief Type definition for pairs of audio ports and their associated conversion buffers.
     */
    using IasConversionBufferPair = std::pair<IasAudioPortPtr, IasAudioRingBuffer*>;

    /**
     * @brief Type definition for maps with all audio ports and their associated conversion buffers.
     */
    using IasConversionBufferMap = std::map<IasAudioPortPtr,  IasAudioRingBuffer*>;

    DltContext                     *mLog;
    IasRoutingZoneParamsPtr         mParams;
    IasAudioSinkDevicePtr           mSinkDevice;
    bool                       mIsDerivedZone;    //!< True, if this zone is a derived zone
    IasRoutingZonePtrList           mDerivedZones;
    IasRoutingZoneWorkerThreadPtr   mWorkerThread;
    IasSwitchMatrixPtr              mSwitchMatrix;
    IasRoutingZonePtr               mRoutingZonePtr;   //!< Shared pointer to this routing zone object.
    IasRoutingZonePtr               mBaseZone;         //!< A pointer to the base zone if this is a derived zone
};


/**
 * @brief Function to get a IasRoutingZone::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasRoutingZone::IasResult& type);


} //namespace IasAudio

#endif // IASROUTINGZONE_HPP
