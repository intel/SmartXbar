/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef IASAUDIOSINKDEVICE_HPP_
#define IASAUDIOSINKDEVICE_HPP_

#include "model/IasAudioDevice.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio
{

/*!
 * @brief Documentation for class IasAudioSinkDevice
 */
class IAS_AUDIO_PUBLIC IasAudioSinkDevice : public IasAudioDevice
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasAudioSinkDevice(IasAudioDeviceParamsPtr params);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioSinkDevice();

    /**
     * @brief Link a zone to this sink device
     *
     */
    void linkZone(IasRoutingZonePtr routingZone);

    /**
     * @brief Unlink the zone from this sink device
     *
     */
    void unlinkZone();

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioSinkDevice(IasAudioSinkDevice const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioSinkDevice& operator=(IasAudioSinkDevice const &other);

    IasPortDirection mDirection;
};

} // namespace IasAudio

#endif // IASAUDIOSINKDEVICE_HPP_
