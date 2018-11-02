/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasICmdRegistration.hpp
 * @date   2013
 * @brief  This is the declaration of the IasICmdRegistration class.
 */

#ifndef IASICMDREGISTRATION_HPP_
#define IASICMDREGISTRATION_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

/**
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasIModuleId;
class IasProperties;

/**
 * @brief Documentation for class IasICmdRegistration
 */
class IAS_AUDIO_PUBLIC IasICmdRegistration
{
  public:
    enum IasResult
    {
      eIasOk,       //!< Method successful
      eIasFailed    //!< Method failed
    };

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasICmdRegistration() {}

    /**
     *  @brief Register a command interface for a specific module.
     *
     *  @params[in] instanceName  Instance name of the audio module providing the interface.
     *  @params[in] interface Reference of the interface, which should be called when a new message comes in.
     *
     *  @returns Returns an error of type IasICmdRegistration::IasResult
     */
    virtual IasResult registerModuleIdInterface(const std::string &instanceName, IasIModuleId *interface) = 0;

    /**
     *  @brief Unregister a previously command interface for a specific module.
     *
     *  @params[in] instanceName  Instance name of the audio module providing the interface.
     */
    virtual void unregisterModuleIdInterface(const std::string &instanceName) = 0;
};

} // namespace IasAudio

#endif // IASICMDREGISTRATION_HPP_
