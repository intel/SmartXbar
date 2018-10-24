/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef IASAUDIOSOURCEDEVICE_HPP_
#define IASAUDIOSOURCEDEVICE_HPP_

#include "model/IasAudioDevice.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

/*!
 * @brief Documentation for class IasAudioSourceDevice
 */
class IAS_AUDIO_PUBLIC IasAudioSourceDevice : public IasAudioDevice
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasAudioSourceDevice(IasAudioDeviceParamsPtr params);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioSourceDevice();

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioSourceDevice(IasAudioSourceDevice const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioSourceDevice& operator=(IasAudioSourceDevice const &other);

    IasPortDirection  mDirection;
};

} // namespace IasAudio

#endif // IASAUDIOSOURCEDEVICE_HPP_
