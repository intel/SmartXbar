/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkSetupImpl.hpp
 * @date   2017
 * @brief  The Setup interface of the test framework.
 */

#ifndef AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKSETUPIMPL_HPP_
#define AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKSETUPIMPL_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"
#include "audio/testfwx/IasTestFrameworkSetup.hpp"
#include "smartx/IasAudioTypedefs.hpp"


namespace IasAudio {

/*!
 * @brief Private implementation of class IasTestFrameworkSetup
 */
class IasTestFrameworkSetupImpl : public IasTestFrameworkSetup
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasTestFrameworkSetupImpl(IasTestFrameworkConfigurationPtr config);

    /*!
     *  @brief Destructor
     */
    ~IasTestFrameworkSetupImpl();

    virtual IasResult createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin);
    virtual void destroyAudioPin(IasAudioPinPtr *pin);

    virtual IasResult addAudioInputPin(IasAudioPinPtr pipelineInputPin);
    virtual void deleteAudioInputPin(IasAudioPinPtr pipelineInputPin);

    virtual IasResult addAudioOutputPin(IasAudioPinPtr pipelineOutputPin);
    virtual void deleteAudioOutputPin(IasAudioPinPtr pipelineOutputPin);

    virtual IasResult addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin);
    virtual void deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin);

    virtual IasResult addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);
    virtual void deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);

    virtual IasResult createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module);
    virtual void destroyProcessingModule(IasProcessingModulePtr *module);

    virtual IasResult addProcessingModule(IasProcessingModulePtr module);
    virtual void deleteProcessingModule(IasProcessingModulePtr module);

    virtual void setProperties(IasProcessingModulePtr module, const IasProperties &properties);

    virtual IasResult linkWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFileParams &waveFileParams);
    virtual void unlinkWaveFile(IasAudioPinPtr audioPin);

    virtual IasResult link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType);
    virtual void unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin);

  private:
    /**
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestFrameworkSetupImpl(IasTestFrameworkSetupImpl const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestFrameworkSetupImpl& operator=(IasTestFrameworkSetupImpl const &other);

    /**
     * @brief Create an audio port
     *
     * @param[in] params The configuration parameters for the audio port
     * @param[in,out] audioPort The location where a pointer to the audio port will be placed
     * @return The result of the creation call
     * @retval eIasOk Audio port successfully created
     * @retval eIasFailed Failed to create the audio port
     */
    IasResult createAudioPort(const IasAudioPortParams &params, IasAudioPortPtr *audioPort);

    /**
     * @brief Destroy a previously created audio port
     *
     * @param[in] audioPort A pointer to the audio port to destroy
     */
    void destroyAudioPort(IasAudioPortPtr audioPort);

    /**
     * @brief Add an audio port to a test framework routing zone
     *
     * @param[in] audioPort The audio port that shall be added to test framework routing zone
     * @param[in] waveFile Input wave file that shall be linked with the port
     * @return The result of the call
     * @retval eIasOk Audio port successfully added to the test framework routing zone
     * @retval eIasFailed Failed to add audio port to test framework routing zone
     */
    IasResult link(IasAudioPortPtr audioPort, IasTestFrameworkWaveFilePtr waveFile);

    /**
     * @brief Delete audio port from test framework routing zone
     *
     * @param[in] audioPort The audio port that shall be deleted
     * @param[in] waveFile Input wave file that is linked with the port
     *
     */
    void unlink(IasAudioPortPtr audioPort, IasTestFrameworkWaveFilePtr waveFile);

    /**
     * @brief Link an audio port with a pipeline pin
     *
     * If a pipeline has been added to a test framework routing zone, its input pins and output pins have to be
     * linked with the appropriate audio ports. For this purpose, this function can be used
     *
     * @li to link a test framework routing zone input port to a pipeline input pin, or
     * @li to link a pipeline output pin to a test framework routing zone output port.
     *
     * @param[in] inputPort    Input or output port of the test framework routing zone
     * @param[in] pipelinePin  Input pin or output pin of the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin);

    /**
     * @brief Unlink a port from a pipeline pin
     *
     * This function can be used
     *
     * @li to unlink a test framework routing zone input port from a pipeline input pin, or
     * @li to unlink a pipeline output pin from a test framework routing zone output port.
     *
     * @param[in] inputPort    Input or output port of the test framework routing zone
     * @param[in] pipelinePin  Input pin or output pin of the pipeline
     */
    void unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin);

    IasTestFrameworkConfigurationPtr     mConfig;
    DltContext                           *mLog;
};

} // namespace IasAudio

#endif // AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKSETUPIMPL_HPP_
