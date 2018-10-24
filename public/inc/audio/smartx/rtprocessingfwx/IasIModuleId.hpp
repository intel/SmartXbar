/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIModuleId.hpp
 * @date   2013
 * @brief  This is the declaration of the IasIModuleId class.
 */

#ifndef IASIMODULEID_HPP_
#define IASIMODULEID_HPP_

#include <functional>
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/IasProperties.hpp"
/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

/**
 * @brief The command ids for IADK Get/Set parameter module functions
 */
static constexpr std::int32_t cGetParametersCmdId = 200;
static constexpr std::int32_t cSetParametersCmdId = 201;

/*!
 * @brief Documentation for class IasIModuleId
 */
class IAS_AUDIO_PUBLIC IasIModuleId
{
  public:
    enum IasResult
    {
      eIasOk,       //!< Method successful
      eIasFailed    //!< Method failed
    };

    /**
     * @brief Constructor
     *
     * @param[in] config Pointer to the configuration of the processing module
     */
    IasIModuleId(const IasIGenericAudioCompConfig *config)
      :mConfig(config)
    {}

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasIModuleId() {}

    /**
     * @brief Initialize the command instance implementation
     *
     * @returns The result of the method
     * @retval eIasOk Successfully processed cmd
     * @retval eIasFailed Faled to process cmd
     */
    virtual IasResult init() = 0;

    /**
     * @brief This pure virtual function has to be implemented by the derived class.
     *
     * It is used whenever a command, i.e. properties are passed to the audio module. It is up to the module
     * on how to react on the cmd. The command properties that are provided to the module are all defined
     * in the cmdProperties instance. The audio module can directly respond to the cmdProperties by setting
     * certain values in the returnProperties. It is up to the module if and how the module responds using
     * the returnProperties. If there is an error during processing of the cmd this can also be signalled
     * to the caller by setting the returnProperties. It is up to the calling application to react on those
     * errors.
     *
     * @returns The result of the method
     * @retval eIasOk Successfully processed cmd
     * @retval eIasFailed Faled to process cmd
     */
    virtual IasResult processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties) = 0;

  protected:

    /**
     * @brief pointer to the actual audio processing configuration
     *
     * This pointer can be used to get access to the configuration properties.
     */
    const IasIGenericAudioCompConfig *mConfig;

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasIModuleId(IasIModuleId const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasIModuleId& operator=(IasIModuleId const &other);

};

} // namespace IasAudio

#endif // IASIMODULEID_HPP_
