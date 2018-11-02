/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    IasAudioFilterCallback.hpp
 * @brief   Callback function class for the audio filter library.
 * @date    November 07, 2012
 */

#ifndef IASAUDIOFILTERCALLBACK_HPP_
#define IASAUDIOFILTERCALLBACK_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IAS_AUDIO_PUBLIC IasAudioFilterCallback
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasAudioFilterCallback();

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioFilterCallback();

    /*
     *  @brief Callback function. Virtual method, must be implemented by the inherited class.
     */
    virtual void gainRampingFinished(uint32_t  channel, float gain, uint64_t callbackUserData)=0;

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioFilterCallback(IasAudioFilterCallback const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioFilterCallback& operator=(IasAudioFilterCallback const &other); //lint !e1704
};

} //namespace IasAudio

#endif /* IASAUDIOFILTERCALLBACK_HPP_ */
