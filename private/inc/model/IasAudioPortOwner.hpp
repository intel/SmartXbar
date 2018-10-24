/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef IASAUDIOPORTOWNER_HPP_
#define IASAUDIOPORTOWNER_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "smartx/IasAudioTypedefs.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

/*!
 * @brief Documentation for class IasAudioPortOwner
 */
class IAS_AUDIO_PUBLIC IasAudioPortOwner
{
  public:

    /*!
     *  @brief Destructor, virtual to allow inheritance.
     */
    virtual ~IasAudioPortOwner(){};

    /**
     * @brief Get the configured sample rate
     *
     * @return The configured sample rate in Hz
     */
    virtual uint32_t getSampleRate() const = 0;

    /**
     * @brief Get the configured sample format
     *
     * @return The configured sample format
     */
    virtual IasAudioCommonDataFormat getSampleFormat() const = 0;

    /**
     * @brief Get the configured period size
     *
     * @return The configured period size in frames
     */
    virtual uint32_t getPeriodSize() const = 0;

    /**
     * @brief Get the assigned switch matrix worker thread
     *
     * @return The assigned switch matrix worker thread
     */
    virtual IasSwitchMatrixPtr getSwitchMatrix() const = 0;

    /**
     * @brief Get the name of the port owner
     *
     * @return The name of the port owner
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the concrete device if it is a smartx client
     *
     * @return The pointer to the smartx client, nullptr if device is not a smartxclient
     */
    virtual IasAudioCommonResult getConcreteDevice(IasSmartXClientPtr* smartXClient) {smartXClient = nullptr;(void)smartXClient; return eIasResultNotInitialized;}

    /**
     * @brief Get the concrete device if it is an alsa handler
     *
     * @return The pointer to the alsa handler, nullptr if device is not an alsa handler
     */
    virtual IasAudioCommonResult getConcreteDevice(IasAlsaHandlerPtr* alsaHandler) {alsaHandler = nullptr;(void)alsaHandler; return eIasResultNotInitialized;}

    /**
     * @brief Get the routing zone
     *
     * This function will be overridden by the derived classes IasAudioDevice and IasRoutingZone
     * in order to implement the following behavior:
     *
     * @li If the audio port owner is an audio sink device or a routing zone,
     *     this function returns the pointer to the routing zone object.
     * @li If the audio port owner is an audio source device, this function returns a nullptr.
     *
     * @return The pointer to the routing zone object.
     */
    virtual IasRoutingZonePtr getRoutingZone() const {return nullptr;}

    virtual bool isSmartXClient() const { return false; }
    virtual bool isAlsaHandler() const {return false; }

    /**
     * @brief Query whether the audio port owner is a routing zone.
     *
     * This function will be overridden by the derived classes IasAudioDevice and IasRoutingZone.
     *
     * @return Boolean flag indicating whether the audio port owner is a routing zone.
     */
    virtual bool isRoutingZone() const {return false; }

};

} // namespace IasAudio

#endif // IASAUDIOPORTOWNER_HPP_
