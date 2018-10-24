/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIProcessingModuleConfig.hpp
 * @date   2016
 * @brief
 */

#ifndef IASIPROCESSINGMODULECONFIG_HPP_
#define IASIPROCESSINGMODULECONFIG_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
 * @brief The audio processing module config interface class
 *
 * This class is used to configure an audio processing module
 * with specific parameters
 */
class IAS_AUDIO_PUBLIC IasIProcessingModuleConfig
{
  public:
    /**
     * @brief Set the instance name of the processing module being created
     *
     * The instance name has to be unique among all modules.
     *
     * @param[in] instanceName The name of the instance being created
     */
    void setInstanceName(const std::string &instanceName) = 0;

    /**
     * @brief Set the type name of the processing module that shall be created
     *
     * This is the type name which the module used to register itself and by which
     * it can be found in the list of loaded processing modules. The type name can
     * be one of the predefined type names or a customer specific for a third-party
     * module.
     *
     * @param[in] typeName The processing module type name
     */
    void setTypeName(const std::string &typeName) = 0;

};

} // namespace IasAudio

#endif /* IASIPROCESSINGMODULECONFIG_HPP_ */
