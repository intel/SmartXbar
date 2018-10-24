/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIProcessing.hpp
 * @date   2015
 * @brief
 */

#ifndef IASIPROCESSING_HPP_
#define IASIPROCESSING_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IasProperties;

/**
  * @brief The processing interface class
  *
  * This class is used to control the audio processing modules
  */
class IAS_AUDIO_PUBLIC IasIProcessing
{
  public:
    /**
     * @brief The result type for the IasIProcessing methods
     */
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed,                 //!< Operation failed
    };

    /**
    * @brief Destructor.
    */
    virtual ~IasIProcessing() {}

    /**
     * @brief Send cmd to audio module
     *
     * This method is used to send a cmd to the audio processing module identified by its unique instanceName.
     * The cmdProperties for each command that an audio module supports are defined by the audio module itself
     * and are described in the accompanying documentation.
     *
     * @param[in] instanceName The unique instance name of the audio module being addressed.
     * @param[in] cmdProperties The properties for the command containing audio module dependent properties.
     * @param[out] returnProperties The properties returned as an answer by the module. The content also depends on the audio module and might be empty.
     *
     * @returns Result of the method call
     * @retval eIasOk Successfully send cmd
     * @retval eIasFailed Error sending cmd
     */
    virtual IasResult sendCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties) = 0;
};

/**
 * @brief Function to get a IasIProcessing::IasResult as string.
 *
 * @param[in] type The IasIProcessing::IasResult value
 *
 * @return Enum Member as string
 */
std::string toString(const IasIProcessing::IasResult& type);


} //namespace IasAudio

#endif /* IASIPROCESSING_HPP_ */
