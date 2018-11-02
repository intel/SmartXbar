/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSetupHelper.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSETUPHELPER_HPP
#define IASSETUPHELPER_HPP


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasISetup.hpp"

namespace IasAudio {

/**
 * @brief Setup helper methods that provide simplified creation and general query methods
 */
namespace IasSetupHelper {

/**
  * @brief Create an audio source device and an audio output port linked to it
  *
  * The audio port params like e.g. number of channels match the one from the audio device.
  *
  * @param[in] setup A pointer to the setup instance. Can be received via IasSmartX::setup
  * @param[in] params The configuration parameters for the audio source device
  * @param[in] id The source id of the created audio output port attached to the audio source device
  * @param[in,out] audioSourceDevice A pointer to the location where the created audio source device will be returned
  *
  * @return The result of the call
  * @retval IasISetup::eIasOk Audio source device and audio port successfully created and linked
  * @retval IasISetup::eIasFailed Failed to create and link audio source device and audio port
  */
IAS_AUDIO_PUBLIC IasISetup::IasResult createAudioSourceDevice(IasISetup *setup, const IasAudioDeviceParams &params, int32_t id, IasAudioSourceDevicePtr *audioSourceDevice);

/**
  * @brief Create an audio sink device, a routing zone and an audio input port linked to it
  *
  * The audio port params like e.g. number of channels match the one from the audio device.
  *
  * @param[in] setup A pointer to the setup instance. Can be received via IasSmartX::setup
  * @param[in] params The configuration parameters for the audio sink device
  * @param[in] id The sink id of the created audio output port attached to the audio sink device
  * @param[in,out] audioSinkDevice A pointer to the location where the created audio sink device will be returned
  * @param[in,out] routingZone A pointer to the location where the created routing zone will be returned
  *
  * @return The result of the call
  * @retval IasISetup::eIasOk Audio sink device, routing zone and audio port successfully created and linked
  * @retval IasISetup::eIasFailed Failed to create and link audio source device, routing zone and audio port
  */
IAS_AUDIO_PUBLIC IasISetup::IasResult createAudioSinkDevice(IasISetup *setup, const IasAudioDeviceParams &params, int32_t id, IasAudioSinkDevicePtr *audioSinkDevice, IasRoutingZonePtr *routingZone);

/**
  * @brief Create an audio sink device, a routing zone, a set of sink input ports, and a set of routing zone input ports.
  *
  * The routing zone will be linked with the sink. This function does *not* link the routing zone input ports
  * with the sink device input ports. This has to be done by the caller after this function has been executed.
  * This linking can be done either directly by means of the function
  * IasISetup::link(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
  * or via a pipeline. If a pipeline is used, the pipeline must be added to the routing zone by means of the function
  * IasISetup::addPipeline(IasRoutingZonePtr routingZone, IasPipelinePtr pipeline) .
  *
  * @param[in] setup                     A pointer to the setup instance. Can be received via IasSmartX::setup.
  * @param[in] params                    The configuration parameters for the audio sink device.
  * @param[in] idVector                  The sink ids of the routing zone input ports that will be created by this function.
  * @param[in] numChannelsVector         A vector with the number of channels of each audio port,
  *                                      one element for each port.
  * @param[in] channelIdxVector          A vector with the indexes of the first channel of the sink device input ports,
  *                                      one element for each port.
  * @param[in,out] audioSinkDevice       A pointer to the location where the created audio sink device will be returned
  * @param[in,out] routingZone           A pointer to the location where the created routing zone will be returned
  * @param[out]    sinkPortVector        A vector with the sink input ports hat have been created by the function.
  * @param[out]    routingZonePortVector A vector with the routing zone input ports hat have been created by the function.
  *
  * @return The result of the call
  * @retval IasISetup::eIasOk Audio sink device, routing zone and audio port successfully created and linked
  * @retval IasISetup::eIasFailed Failed to create and link audio source device, routing zone and audio port
  */
IAS_AUDIO_PUBLIC IasISetup::IasResult createAudioSinkDevice(IasISetup *setup,
                                                          const IasAudioDeviceParams& params,
                                                          const std::vector<int32_t>& idVector,
                                                          const std::vector<uint32_t>& numChannelsVector,
                                                          const std::vector<uint32_t>& channelIndexVector,
                                                          IasAudioSinkDevicePtr* audioSinkDevice,
                                                          IasRoutingZonePtr* routingZone,
                                                          std::vector<IasAudioPortPtr>* sinkPortVector,
                                                          std::vector<IasAudioPortPtr>* routingZonePortVector);

/**
  * @brief Delete Sink Device with routing zone and all ports
  *
  * @param[in] setup      A pointer to the setup instance. Can be received via IasSmartX::setup.
  * @param[in] deviceName The name of the audio sink device
  *
  * @return The result of the call
  * @retval IasISetup::eIasOk Audio sink device, routing zone successfully deleted
  * @retval IasISetup::eIasFailed Failed to delete audio sink device or/and routing zone
  */
IAS_AUDIO_PUBLIC IasISetup::IasResult deleteSinkDevice(IasISetup *setup, const std::string& deviceName);

/**
  * @brief Delete Source Device with all ports
  *
  * @param[in] setup      A pointer to the setup instance. Can be received via IasSmartX::setup.
  * @param[in] deviceName The name of the audio source device
  *
  * @return The result of the call
  * @retval IasISetup::eIasOk Audio sink device, successfully deleted
  * @retval IasISetup::eIasFailed Failed to delete audio source device or/and routing zone
  */
IAS_AUDIO_PUBLIC IasISetup::IasResult deleteSourceDevice(IasISetup *setup, const std::string& deviceName);


/**
  * @brief Get the source device name
  *
  * @param[in] device Pointer to the source device
  * @return The name of the source device or an empty string if device is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getDeviceName(IasAudioSourceDevicePtr device);

/**
  * @brief Get the sink device name
  *
  * @param[in] device Pointer to the sink device
  * @return The name of the sink device or an empty string if device is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getDeviceName(IasAudioSinkDevicePtr device);

/**
  * @brief Get the pin direction
  *
  * @param[in] pin Pointer to the pin
  * @return The pin direction as string, empty if pin is nullptr
  */
IAS_AUDIO_PUBLIC std::string getPinDirection(IasAudioPinPtr pin);

/**
  * @brief Get the pin name
  *
  * @param[in] pin Pointer to the pin
  * @return The pin name as string, empty if pin is nullptr
  */
IAS_AUDIO_PUBLIC std::string getPinName(IasAudioPinPtr pin);

/**
  * @brief Get the pin number of channels
  *
  * @param[in] pin Pointer to the pin
  * @return The pin number of channels, zero if pin is nullptr
  */
IAS_AUDIO_PUBLIC uint32_t getPinNumChannels(IasAudioPinPtr pin);

/**
  * @brief Get the audio source device number of channels
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device number of channels  or -1 if source is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDeviceNumChannels(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device number of channels
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device number of channels  or -1 if sink is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDeviceNumChannels(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the audio source device sample rate
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device sample rate  or -1 if source is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDeviceSampleRate(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device sample rate
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device sample rate  or -1 if sink is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDeviceSampleRate(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the audio source device data format
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device data format  or empty string if source is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getDeviceDataFormat(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device data format
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device data format  or empty string if sink is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getDeviceDataFormat(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the audio source device clock type
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device clock type  or -1 if source is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getDeviceClockType(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device clock type
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device clock type  or -1 if sink is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getDeviceClockType(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the audio source device period size
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device period size  or -1 if source is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDevicePeriodSize(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device period size
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device period size  or -1 if sink is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDevicePeriodSize(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the audio source device period number
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device period number  or -1 if source is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDeviceNumPeriod(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device period number
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device period number  or -1 if sink is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDeviceNumPeriod(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the audio source device asrc period count
  *
  * @param[in] source Pointer to the audio source device
  * @return The source device asrc period count  or -1 if source is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDevicePeriodsAsrcBuffer(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device asrc period count
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The sink device asrc period count  or -1 if sink is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getDevicePeriodsAsrcBuffer(IasAudioSinkDevicePtr sink);

/**
  * @brief Get the port id
  *
  * @param[in] port Pointer to the port
  * @return The port id or -1 if port is a nullptr
  */
IAS_AUDIO_PUBLIC int32_t getPortId(IasAudioPortPtr port);

/**
  * @brief Get the port name
  *
  * @param[in] port Pointer to the port
  * @return The name of the port or an empty string if port is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getPortName(IasAudioPortPtr port);

/**
  * @brief Get the number of channels of the port
  *
  * @param[in] port Pointer to the port
  * @return The number of the channels of the port or 0 if port is a nullptr
  */
IAS_AUDIO_PUBLIC uint32_t getPortNumChannels(IasAudioPortPtr port);

/**
  * @brief Get the direction of the port
  *
  * @param[in] port Pointer to the port
  * @return The port direction as text string
  */
IAS_AUDIO_PUBLIC std::string getPortDirection(IasAudioPortPtr port);

/**
  * @brief Get the routing zone name
  *
  * @param[in] zone Pointer to the routing zone
  * @return The name of the routing zone or an empty string if zone is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getRoutingZoneName(IasRoutingZonePtr zone);

/**
  * @brief Get the audio source device name
  *
  * @param[in] source Pointer to the audio source device
  * @return The name of the routing audio source device or an empty string if source is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getAudioSourceDeviceName(IasAudioSourceDevicePtr source);

/**
  * @brief Get the audio sink device name
  *
  * @param[in] sink Pointer to the audio sink device
  * @return The name of the routing audio sink device or an empty string if sink is a nullptr
  */
IAS_AUDIO_PUBLIC std::string getAudioSinkDeviceName(IasAudioSinkDevicePtr sink);

} // namespace IasSetupHelper

} //namespace IasAudio

#endif // IASSETUPHELPER_HPP
