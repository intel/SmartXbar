/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasProcessingImpl.hpp
 * @date   2015
 * @brief
 */

#ifndef IASPROCESSINGIMPL_HPP
#define IASPROCESSINGIMPL_HPP


#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

class IasProperties;

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasProcessingImpl : public IasIProcessing
{
  public:
    /**
     * @brief Constructor.
     */
    IasProcessingImpl(IasCmdDispatcherPtr cmdDispatcher);
    /**
     * @brief Destructor.
     */
    virtual ~IasProcessingImpl();

    /**
     * @brief Send cmd to audio module
     *
     * This method is used to send a cmd to the audio processing module identified by its unique instanceName.
     * The cmdProperties for each command that an audio module supports are defined by the audio module itself
     * and are described in the accompanying documentation.
     *
     * @param[in] instanceName The unique instance name of the audio module being addressed.
     * @param[in] cmdProperties The properties for the command containing audio module dependent properties.
     * @param[out] returnProperties The properties returned as an answer by the module. They also depend on the audio module and might be empty.
     *
     * @returns Result of the method call
     * @retval eIasOk Successfully send cmd
     * @retval eIasFailed Error sending cmd
     */
    virtual IasIProcessing::IasResult sendCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties);


  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasProcessingImpl(IasProcessingImpl const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasProcessingImpl& operator=(IasProcessingImpl const &other);

    IasCmdDispatcherPtr         mCmdDispatcher;
};

} //namespace IasAudio

#endif // IASPROCESSINGIMPL_HPP
