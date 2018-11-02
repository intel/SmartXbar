/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasISetup.hpp
 * @date   2015
 * @brief
 */

#ifndef IASISETUP_HPP_
#define IASISETUP_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

using IasAudioSourceDeviceVector = std::vector<IasAudioSourceDevicePtr>;
using IasAudioSinkDeviceVector = std::vector<IasAudioSinkDevicePtr>;
using IasAudioPortVector = std::vector<IasAudioPortPtr>;
using IasRoutingZoneVector = std::vector<IasRoutingZonePtr>;
using IasAudioPinVector = std::vector<IasAudioPinPtr>;

/**
  * @brief The setup interface class
  *
  * This interface is used to setup all static and dynamic aspects of the SmartXbar like e.g. the audio source devices, audio sink devices,
  * routing zones, etc.
  */
class IAS_AUDIO_PUBLIC IasISetup
{
  public:
    /**
     * @brief The result type for the IasISetup methods
     */
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed                  //!< Operation failed
    };

    /**
      * @brief Destructor.
      */
    virtual ~IasISetup() {}

    /**
     * @brief Create an audio routing zone
     *
     * @param[in] params The configuration parameters for the routing zone
     * @param[in,out] routingZone The location where a pointer to the routing zone will be placed
     * @return The result of the creation call
     * @retval IasISetup::eIasOk Routing zone successfully created
     * @retval IasISetup::eIasFailed Failed to create the routing zone
     */
    virtual IasResult createRoutingZone(const IasRoutingZoneParams &params, IasRoutingZonePtr *routingZone) = 0;

    /**
     * @brief Destroy a previously created audio routing zone
     *
     * @param[in] routingZone A pointer to the routing zone to destroy
     */
    virtual void destroyRoutingZone(IasRoutingZonePtr routingZone) = 0;

    /**
     * @brief Start scheduling of the routing zone
     *
     * If the given routing zone is a base routing zone, then the real-time worker thread
     * for that routing zone is started.
     *
     * If the given routing zone is a derived routing zone, then it is only scheduled when its
     * base routing zone is started.
     *
     * In both cases, the linked audio sink device has to be available and ready before
     * starting the routing zone.
     *
     * @param[in] routingZone The routing zone that shall be started
     *
     * @return The result of the start call
     * @retval IasISetup::eIasOk Routing zone successfully started
     * @retval IasISetup::eIasFailed Failed to start routing zone
     */
    virtual IasResult startRoutingZone(IasRoutingZonePtr routingZone) = 0;

    /**
     * @brief Stop scheduling of the routing zone
     *
     * @param[in] routingZone The routing zone that shall be stopped
     */
    virtual void stopRoutingZone(IasRoutingZonePtr routingZone) = 0;

    /**
     * @brief Create an audio source device.
     *        The maximum allowed data buffer size for an audio device is limited
     *        to 4 MB. It is calculated by the multiplication of sample size [bytes], number of Periods,
     *        period Size [samples], number of channels
     *
     * @param[in] params The configuration parameters for the audio source device
     * @param[in,out] audioSourceDevice The location where a pointer to the audio source device will be placed
     * @return The result of the creation call
     * @retval IasISetup::eIasOk Source device successfully created
     * @retval IasISetup::eIasFailed Failed to create the source device
     */
    virtual IasResult createAudioSourceDevice(const IasAudioDeviceParams &params, IasAudioSourceDevicePtr *audioSourceDevice) = 0;

    /**
     * @brief Destroy a previously created audio source device.
     * It disconnects and deletes all connected ports before deleting the source device.
     *
     * @param[in] audioSourceDevice A pointer to the source device to destroy
     */
    virtual void destroyAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice) = 0;
    /**
     * @brief Start scheduling of an audio source device (if it is associated with an ALSA handler).
     *
     * @param[in] audioSourceDevice The audio source device that shall be started
     *
     * @return The result of the start call
     * @retval IasISetup::eIasOk Audio source device successfully started
     * @retval IasISetup::eIasFailed Failed to start audio source device
     */
    virtual IasResult startAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice) = 0;

    /**
     * @brief Stop scheduling of an audio source device (if it is associated with an ALSA handler).
     *
     * @param[in] audioSourceDevice The audio source device that shall be stopped
     */
    virtual void stopAudioSourceDevice(IasAudioSourceDevicePtr audioSourceDevice) = 0;

    /**
     * @brief Create an audio sink device
     *        The maximum allowed data buffer size for an audio device is limited
     *        to 4 MB. It is calculated by the multiplication of sample size [bytes], number of Periods,
     *        period Size [samples], number of channels
     * @param[in] params The configuration parameters for the audio sink device
     * @param[in,out] audioSinkDevice The location where a pointer to the audio sink device will be placed
     * @return The result of the creation call
     * @retval IasISetup::eIasOk Sink device successfully created
     * @retval IasISetup::eIasFailed Failed to create the sink device
     */
    virtual IasResult createAudioSinkDevice(const IasAudioDeviceParams &params, IasAudioSinkDevicePtr *audioSinkDevice) = 0;

    /**
     * @brief Destroy a previously created audio sink device
     *
     * @param[in] audioSinkDevice A pointer to the sink device to destroy
     */
    virtual void destroyAudioSinkDevice(IasAudioSinkDevicePtr audioSinkDevice) = 0;

    /**
     * @brief Create an audio port
     *
     * @param[in] params The configuration parameters for the audio port
     * @param[in,out] audioPort The location where a pointer to the audio port will be placed
     * @return The result of the creation call
     * @retval IasISetup::eIasOk Audio port successfully created
     * @retval IasISetup::eIasFailed Failed to create the audio port
     */
    virtual IasResult createAudioPort(const IasAudioPortParams &params, IasAudioPortPtr *audioPort) = 0;

    /**
     * @brief Destroy a previously created audio port
     *
     * @param[in] audioPort A pointer to the audio port to destroy
     */
    virtual void destroyAudioPort(IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Add a routing zone as a derived zone to a given base zone
     *
     * @note The period time of the derived zone has to be equal to the period time of the
     * base zone or it has to be an integral multiple of it. See section @ref synchronous_zones for details.
     *
     * @param[in] baseZone The base routing zone where the derived zone shall be added to
     * @param[in] derivedZone The routing zone that shall be added as a derived zone to the given base zone
     * @return The result of the call
     * @retval IasISetup::eIasOk Derived zone successfully added
     * @retval IasISetup::eIasFailed Failed to add derived zone
     */
    virtual IasResult addDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone) = 0;

    /**
     * @brief Delete a derived routing zone from a given base zone
     *
     * @param[in] baseZone The base routing zone from where the derived zone shall be deleted
     * @param[in] derivedZone The derived routing zone that shall be deleted from the base routing zone
     */
    virtual void deleteDerivedZone(IasRoutingZonePtr baseZone, IasRoutingZonePtr derivedZone) = 0;

    /**
     * @brief Link a routing zone to an audio sink device
     *
     * @param[in] routingZone The routing zone that shall be linked to the audio sink device
     * @param[in] audioSinkDevice The audio sink device that shall be linked to the routing zone
     * @return The result of the call
     * @retval IasISetup::eIasOk Routing zone successfully linked to audio sink device
     * @retval IasISetup::eIasFailed Failed to link routing zone to audio sink device
     */
    virtual IasResult link(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice) = 0;

    /**
     * @brief Unlink a routing zone from an audio sink device
     *
     * @param[in] routingZone The routing zone that shall be unlinked from the audio sink device
     * @param[in] audioSinkDevice The audio sink device that shall be unlinked from the routing zone
     */
    virtual void unlink(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice) = 0;

    /**
     * @brief Add an audio output port to an audio source device
     *
     * @param[in] audioSourceDevice The audio source device where the audio port shall be added to
     * @param[in] audioPort The audio port that shall be added as an output port to the audio source device
     * @return The result of the call
     * @retval IasISetup::eIasOk Audio port successfully added to the audio source device
     * @retval IasISetup::eIasFailed Failed to add audio port to audio source device
     */
    virtual IasResult addAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Delete audio output port from audio source device
     *
     * @param[in] audioSourceDevice The audio source device where the audio port shall be deleted from
     * @param[in] audioPort The audio port that shall be deleted as an output port from the audio source device
     */
    virtual void deleteAudioOutputPort(IasAudioSourceDevicePtr audioSourceDevice, IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Add an audio input port to an audio sink device
     *
     * @param[in] audioSinkDevice The audio sink device where the audio port shall be added to
     * @param[in] audioPort The audio port that shall be added as an input port to the audio sink device
     * @return The result of the call
     * @retval IasISetup::eIasOk Audio port successfully added to the audio sink device
     * @retval IasISetup::eIasFailed Failed to add audio port to audio sink device
     */
    virtual IasResult addAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Delete audio input port from audio sink device
     *
     * @param[in] audioSinkDevice The audio sink device where the audio port shall be deleted from
     * @param[in] audioPort The audio port that shall be deleted as an input port from the audio sink device
     */
    virtual void deleteAudioInputPort(IasAudioSinkDevicePtr audioSinkDevice, IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Add an audio input port to a routing zone
     *
     * @param[in] routingZone The routing zone where the audio port shall be added to
     * @param[in] audioPort The audio port that shall be added as an input port to the routing zone
     * @return The result of the call
     * @retval IasISetup::eIasOk Audio port successfully added to the routing zone
     * @retval IasISetup::eIasFailed Failed to add audio port to routing zone
     */
    virtual IasResult addAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Delete audio input port from routing zone
     *
     * @param[in] routingZone The routingZone where the audio port shall be deleted from
     * @param[in] audioPort The audio port that shall be deleted as an input port from the routing zone
     */
    virtual void deleteAudioInputPort(IasRoutingZonePtr routingZone, IasAudioPortPtr audioPort) = 0;

    /**
     * @brief Link a zone input port to a sink device input port.
     *
     * This function controls how the associated routing zone transfers the PCM frames from the zone input ports
     * to the sink device input ports.
     *
     * The linking between a zone input port and a sink device input port must be biuniqe.
     * This means that a zone input port cannot be linked to more than one sink device input port.
     * Of course, also a sink device input port cannot receive PCM frames from more than one zone input port.
     *
     * Before this function can be called, the sink device that owns the sink device input port must be linked
     * with the routing zone that owns the zone input port. This has to be done by means of the function
     * IasISetup::link(IasRoutingZonePtr routingZone, IasAudioSinkDevicePtr audioSinkDevice).
     * It is not allowed to link zone input ports with sink device input ports that belong to different routing zones.
     *
     * This function establishes a direct connection from the routing zone input port
     * to the audio sink device input port, i.e., there won't be any audio pipeline in between.
     *
     * @param[in] zoneInputPort The routing zone input port that shall be linked to the audio sink device input port
     * @param[in] sinkDeviceInputPort The audio sink device input port that shall be linked to the routing zone input port
     * @return The result of the call
     * @retval IasISetup::eIasOk Routing zone input port successfully linked to audio sink device input port
     * @retval IasISetup::eIasFailed Failed to link routing zone input port to audio sink device input port
     */
    virtual IasResult link(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort) = 0;

    /**
     * @brief Unlink a routing zone input port from an audio sink device input port
     *
     * @param[in] zoneInputPort The routing zone input port that shall be unlinked from the audio sink device input port
     * @param[in] sinkDeviceInputPort The audio sink device input port that shall be unlinked from the routing zone input port
     */
    virtual void unlink(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort) = 0;

    /**
     * @brief Get a vector with all available audio input ports
     *
     * @return A vector with all available audio input ports
     */
    virtual IasAudioPortVector getAudioInputPorts() const = 0;

    /**
     * @brief Get a vector with all available audio output ports
     *
     * @return A vector with all available audio output ports
     */
    virtual IasAudioPortVector getAudioOutputPorts() const = 0;

    /**
     * @brief Get a vector with all audio ports
     *
     * @return A vector with all audio ports
     */
    virtual IasAudioPortVector getAudioPorts() const = 0;

    /**
     * @brief Get a vector with all audio pins
     *
     * @return A vector with all audio pins
     */
    virtual IasAudioPinVector getAudioPins() const = 0;

    /**
     * @brief Get a vector with all available routing zones
     *
     * @return A vector with all available routing zones
     */
    virtual IasRoutingZoneVector getRoutingZones() const = 0;

    /**
     * @brief Get a vector with all available audio source devices
     *
     * @return A vector with all available audio source devices
     */
    virtual IasAudioSourceDeviceVector getAudioSourceDevices() const = 0;

    /**
     * @brief Get a vector with all available audio sink devices
     *
     * @return A vector with all available audio sink devices
     */
    virtual IasAudioSinkDeviceVector getAudioSinkDevices() const = 0;

    /**
     * @brief Get the handle for a routing zone specified by a specific name
     *
     * @param[in] name The name of the routing zone
     *
     * @return The handle of the routing zone or a nullptr in case there is no routing zone with the given name.
     */
    virtual IasRoutingZonePtr getRoutingZone(const std::string &name) = 0;

    /**
     * @brief Get the handle for an audio source device specified by a specific name
     *
     * @param[in] name The name of the audio source device
     *
     * @return The handle of the audio source device or a nullptr in case there is no audio source device with the given name.
     */
    virtual IasAudioSourceDevicePtr getAudioSourceDevice(const std::string &name) = 0;

    /**
     * @brief Get the handle for an audio sink device specified by a specific name
     *
     * @param[in] name The name of the audio sink device
     *
     * @return The handle of the audio sink device or a nullptr in case there is no audio sink device with the given name.
     */
    virtual IasAudioSinkDevicePtr getAudioSinkDevice(const std::string &name) = 0;

    /**
     * @brief Get the handle for previously created pipeline specified by a specific name
     *
     * @param[in] name the name of the pipeline to retrieve
     *
     * @return The handle of the pipeline or a nullptr in case there is no pipeline with the given name.
     */
    virtual IasPipelinePtr getPipeline(const std::string &name) = 0;

    /**
     * @brief Add a source to a logical source group
     *
     * If the source group does not exist, it is created
     * If the source group already exist, the source id is just added to it
     * @param[in] name the name of the logical group
     * @param[in] id the if of the source
     */
    virtual void addSourceGroup(const std::string &name, int32_t id) = 0;

    /**
     * @brief return the logical source groups
     *
     * @return The map containing the source groups
     */
    virtual IasSourceGroupMap getSourceGroups() = 0;

    /**
     * @brief Create a new audio pipeline
     *
     * @param[in,out] pipeline Pointer to destination for new pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult createPipeline(const IasPipelineParams &params, IasPipelinePtr *pipeline) = 0;

    /**
     * @brief Destroy a previously created pipeline
     *
     * pipeline pointer will equal nullptr after this method call.
     *
     * @param[in,out] pipeline Pointer to pipeline to be destroyed
     */
    virtual void destroyPipeline(IasPipelinePtr *pipeline) = 0;

    /**
     * @brief Add pipeline to routing zone
     *
     * There is only one pipeline per routing zone allowed. Adding a second pipeline will fail.
     * One pipeline can only be added to one routing zone. Adding the same pipeline to a second
     * routing zone will also fail.
     *
     * @note This method has to be called before linking pins of the pipeline to any ports of
     * a routing zone or the ports of the linked sink device of the routing zone by using the method
     * IasAudio::IasISetup::link(IasAudioPortPtr, IasAudioPinPtr).
     *
     * @param[in] routingZone Routing zone where the pipeline shall be added.
     * @param[in] pipeline Pipeline that shall be added to routing zone.
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult addPipeline(IasRoutingZonePtr routingZone, IasPipelinePtr pipeline) = 0;

    /**
     * @brief Delete pipeline from routing zone
     *
     * If a pipeline was added to the routing zone before, this one will be deleted from the routing zone.
     *
     * @param[in] routingZone The routing zone of which the pipeline shall be deleted.
     */
    virtual void deletePipeline(IasRoutingZonePtr routingZone) = 0;

    /**
     * @brief Create an audio pin
     *
     * @param[in] params The configuration parameters of the audio pin
     * @param[in,out] pin Destination where the new audio pin is returned.
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin) = 0;

    /**
     * @brief Destroy a previously created pin
     *
     * pin pointer will equal nullptr after this method call.
     *
     * @param[in,out] pin Pointer to pin to be destroyed
     */
    virtual void destroyAudioPin(IasAudioPinPtr *pin) = 0;

    /**
     * @brief Add audio input pin to pipeline
     *
     * @param[in] pipeline Pipeline where the pin shall be added
     * @param[in] pipelineInputPin Pin that shall be added to pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult addAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin) = 0;

    /**
     * @brief Delete audio input pin from pipeline
     *
     * @param[in] pipeline Pipeline where the pin shall be deleted
     * @param[in] pipelineInputPin Pin that shall be deleted from pipeline
     */
    virtual void deleteAudioInputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineInputPin) = 0;

    /**
     * @brief Add audio output pin to pipeline
     *
     * @param[in] pipeline Pipeline where the pin shall be added
     * @param[in] pipelineOutputPin Pin that shall be added to pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult addAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin) = 0;

    /**
     * @brief Delete audio output pin from pipeline
     *
     * @param[in] pipeline Pipeline where the pin shall be deleted
     * @param[in] pipelineOutputPin Pin that shall be deleted from pipeline
     */
    virtual void deleteAudioOutputPin(IasPipelinePtr pipeline, IasAudioPinPtr pipelineOutputPin) = 0;

    /**
     * @brief Add an audio input/output pin to the processing module
     *
     * An input/output pin has to be used for modules doing in-place processing on this pin
     *
     * @param[in] module Module where the pin shall be added
     * @param[in] inOutPin Pin that shall be added to module
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin) = 0;

    /**
     * @brief Delete an audio input/output pin from the processing module
     *
     * @param[in] module Module where the pin shall be added
     * @param[in] inOutPin Pin that shall be added to module
     */
    virtual void deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin) = 0;

    /**
     * @brief Add audio pin mapping
     *
     * A pin mapping is required if the module is not able to do in-place processing, e.g. if the number of channels
     * for input and output pins is different (up-/down-mixer). Another example is a mixer module, where one output
     * pin is created from several input pins. If several input pins are required to create the signal for one output
     * pin like in the mixer example than this method has to be called multiple times with a different input pin but
     * with the same, single output pin.
     *
     * @param[in] module Module where the pin mapping shall be added
     * @param[in] inputPin Input pin of the mapping pair
     * @param[in] outputPin Output pin of the mapping pair
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin) = 0;

    /**
     * @brief Delete audio pin mapping
     *
     * @param[in] module Module where the pin mapping shall be deleted
     * @param[in] inputPin Input pin of the mapping pair to be deleted
     * @param[in] outputPin Output pin of the mapping pair to be deleted
     */
    virtual void deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin) = 0;

    /**
     * @brief Create a processing module of a certain type
     *
     * After creating the module the pins and/or the pin mappings have to be added.
     *
     * @param[in] params Parameters for configuration of the processing module
     * @param[in,out] module Destination where the created module shall be returned
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module) = 0;

    /**
     * @brief Destroy a previously created processing module
     *
     * module pointer will equal nullptr after this method call.
     *
     * @param[in,out] module Pointer to module to be destroyed
     */
    virtual void destroyProcessingModule(IasProcessingModulePtr *module) = 0;

    /**
     * @brief Add processing module to pipeline
     *
     * Each processing module has to be owned by one single pipeline.
     *
     * @param[in] pipeline The pipeline where the module shall be added to
     * @param[in] module The module that shall be added to the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult addProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module) = 0;

    /**
     * @brief Delete processing module from the pipeline
     *
     * A previously added module can be deleted from a pipeline by using this method. Deleting a module
     * will also remove all links to and from the module.
     *
     * @param[in] pipeline The pipeline where the module shall be deleted from
     * @param[in] module The module that shall be deleted from the pipeline
     */
    virtual void deleteProcessingModule(IasPipelinePtr pipeline, IasProcessingModulePtr module) = 0;

    /**
     * @brief Link an input port with a pipeline pin
     *
     * If a pipeline has been added to a routing zone, its input pins and output pins have to be
     * linked with the appropriate audio ports. For this purpose, this function can be used
     *
     * @li to link a routing zone input port to a pipeline input pin, or
     * @li to link a pipeline output pin to an audio sink device input port.
     *
     * @note The pipeline has to be added to a routing zone by the method IasAudio::IasISetup::addPipeline
     * before calling this method.
     *
     * @param[in] inputPort    Input port of the routing zone or of the audio sink device
     * @param[in] pipelinePin  Input pin or output pin of the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin) = 0;

    /**
     * @brief Unlink an input port from a pipeline pin
     *
     * This function can be used
     *
     * @li to unlink a routing zone input port from a pipeline input pin, or
     * @li to unlink a pipeline output pin from an audio sink device input port.
     *
     * @param[in] inputPort    Input port of the routing zone or of the audio sink device
     * @param[in] pipelinePin  Input pin or output pin of the pipeline
     */
    virtual void unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin) = 0;

    /**
     * @brief Link an audio output pin to an audio input pin
     *
     * The output pin and the input pin can also be combined input/output pins. These combined pins
     * are used for in-place processing components.
     *
     * By means of this function, the links between the processing modules can be configured,
     * in order to set up a cascade of processing modules.
     *
     * The linking between two audio pins must be biuniqe. This means that an input pin cannot
     * be linked to more than one output pin. Of course, also an output pin cannot receive
     * PCM frames from more than one input pin.
     *
     * There are two types how an output pin can be linked to an input pin:
     *
     * @li \ref IasAudioPinLinkType "IasAudioPinLinkType::eIasAudioPinLinkTypeImmediate"
     *          represents the normal linking from an output pin to an input pin.
     * @li \ref IasAudioPinLinkType "IasAudioPinLinkType::eIasAudioPinLinkTypeDelayed"
     *          represents a link with a delay of one period.
     *
     * The delayed link type is necessary to implement feed-back loops. A feed-back loop must
     * contain at least one delayed link, because otherwise there is no solution for breaking
     * up the closed loop of dependencies between the modules' input and output signals.
     * This is similar to the structure of a time-discrete IIR filter, which cannot have a
     * feed-back path with zero delay.
     *
     * @param[in] outputPin  Audio output or combined input/output pin
     * @param[in] inputPin   Audio input or combined input/output pin
     * @param[in] linkType   Link type: immediate or delayed
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType) = 0;

    /**
     * @brief Unlink an audio output pin from an audio input pin
     *
     * The output pin and the input pin can also be combined input/output pins. These combined pins
     * are used for in-place processing components.
     *
     * @param[in] outputPin Audio output or combined input/output pin
     * @param[in] inputPin Audio input or combined input/output pin
     */
    virtual void unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin) = 0;

    /**
     * @brief Set the custom configuration properties of the audio module.
     *
     * Details about the concrete configuration properties can be found in each audio modules datasheet.
     *
     * @param[in] module The module for which the properties shall be set
     * @param[in] properties The custom configuration properties of the audio module.
     */
    virtual void setProperties(IasProcessingModulePtr module, const IasProperties &properties) = 0;

    /**
     * @brief Get the custom configuration properties of the audio module.
     *
     * Details about the concrete configuration properties can be found in each audio modules datasheet.
     *
     * @return pointer the the module properties
     */
    virtual IasPropertiesPtr getProperties(IasProcessingModulePtr module) = 0;

    /**
     * @brief Initialize the pipeline's audio chain of processing modules.
     *
     * After the topology of the pipeline has been defined, i.e., after
     *
     * @li all modules have been created and initialized, and
     * @li all audio pins have been created and linked with each other,
     *
     * this function has to be called before the pipeline can be processed. This function analyzes
     * the signal dependencies and identifies the scheduling order of the processing modules.
     * Furthermore, the function identifies how many audio streams have to be created and how the
     * audio streams are mapped to the audio pins. Finally, the audio chain is set up, i.e., all
     * processing modules and and audio streams are added to the audio chain.
     *
     * After this function has been executed successfully, the pipeline is ready to be used.
     *
     * @param[in] pipeline Pointer to the pipeline whose audio chain shall be initialized.
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult initPipelineAudioChain(IasPipelinePtr pipeline) = 0;

};

/**
 * @brief Function to get a IasISetup::IasResult as string.
 *
 * @return Enum Member
 */
std::string toString(const IasISetup::IasResult& type);


} //namespace IasAudio

#endif /* IASISETUP_HPP_ */
