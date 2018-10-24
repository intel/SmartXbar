/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkSetup.hpp
 * @date   2017
 * @brief  The setup interface of the test framework.
 */

#ifndef AUDIO_DAEMON2_PUBLIC_INC_AUDIO_SMARTX_TESTFWX_IASTESTFRAMEWORKSETUP_HPP_
#define AUDIO_DAEMON2_PUBLIC_INC_AUDIO_SMARTX_TESTFWX_IASTESTFRAMEWORKSETUP_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/testfwx/IasTestFrameworkPublicTypes.hpp"


/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasTestFrameworkWaveFile;
using IasTestFrameworkWaveFilePtr = std::shared_ptr<IasTestFrameworkWaveFile>;
using IasTestFrameworkWaveFileVector = std::vector<IasTestFrameworkWaveFilePtr>;

/*!
 * @brief The test framework setup interface class
 *
 * This interface is used to setup all static and dynamic aspects of the IasTestFramework like e.g. the input and output wave files,
 * processing modules, routing zone, pipeline etc.
 */
class IAS_AUDIO_PUBLIC IasTestFrameworkSetup
{
  public:
    /**
     * @brief The result type for the IasTestFrameworkSetup methods
     */
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed                  //!< Operation failed
    };

    /*!
     *  @brief Destructor.
     */
    virtual ~IasTestFrameworkSetup() {};

    /**
     * @brief Create an audio pin
     *
     * @param[in] params The configuration parameters of the audio pin
     * @param[in,out] pin Destination where the new audio pin is returned.
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin) = 0;

    /**
     * @brief Destroy a previously created audio pin
     *
     * pin pointer will equal nullptr after this method call.
     *
     * @param[in,out] pin Pointer to pin to be destroyed
     */
    virtual void destroyAudioPin(IasAudioPinPtr *pin) = 0;

    /**
     * @brief Add audio input pin to pipeline
     *
     * @param[in] pipelineInputPin Pin that shall be added to pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult addAudioInputPin(IasAudioPinPtr pipelineInputPin) = 0;

    /**
     * @brief Delete audio input pin from pipeline
     *
     * @param[in] pipelineInputPin Pin that shall be deleted from pipeline
     */
    virtual void deleteAudioInputPin(IasAudioPinPtr pipelineInputPin) = 0;

    /**
     * @brief Add audio output pin to pipeline
     *
     * @param[in] pipelineOutputPin Pin that shall be added to pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult addAudioOutputPin(IasAudioPinPtr pipelineOutputPin) = 0;

    /**
     * @brief Delete audio output pin from pipeline
     *
     * @param[in] pipelineOutputPin Pin that shall be deleted from pipeline
     */
    virtual void deleteAudioOutputPin(IasAudioPinPtr pipelineOutputPin) = 0;

    /**
     * @brief Add an audio input/output pin to the processing module
     *
     * An input/output pin has to be used for modules doing in-place processing on this pin
     *
     * @param[in] module Module where the pin shall be added
     * @param[in] inOutPin Pin that shall be added to module
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin) = 0;

    /**
     * @brief Delete an audio input/output pin from the processing module
     *
     * @param[in] module Module where the pin shall be deleted
     * @param[in] inOutPin Pin that shall be deleted from module
     */
    virtual void deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin) = 0;

    /**
     * @brief Add audio pin mapping
     *
     * A pin mapping is required if the module is not able to do in-place processing, e.g. if the number of channels
     * for input and output pins is different (up-/down-mixer). Another example is a mixer module, where one output
     * pin is created from several input pins. If several input pins are required to create the signal for one output
     * pin like in the mixer example than this method has to be called multiple times with a different input pin but
     * with the same, single output pin.
     *
     * @param[in] module Module where the pin mapping shall be added
     * @param[in] inputPin Input pin of the mapping pair
     * @param[in] outputPin Output pin of the mapping pair
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin) = 0;

    /**
     * @brief Delete audio pin mapping
     *
     * @param[in] module Module where the pin mapping shall be deleted
     * @param[in] inputPin Input pin of the mapping pair to be deleted
     * @param[in] outputPin Output pin of the mapping pair to be deleted
     */
    virtual void deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin) = 0;

    /**
     * @brief Create a processing module of a certain type
     *
     * After creating the module the pins and/or the pin mappings have to be added.
     *
     * @param[in] params Parameters for configuration of the processing module
     * @param[in,out] module Destination where the created module shall be returned
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module) = 0;

    /**
     * @brief Destroy a previously created processing module
     *
     * module pointer will equal nullptr after this method call.
     *
     * @param[in,out] module Pointer to module to be destroyed
     */
    virtual void destroyProcessingModule(IasProcessingModulePtr *module) = 0;

    /**
     * @brief Add processing module to pipeline
     *
     * Each processing module has to be owned by one single pipeline.
     *
     * @param[in] module The module that shall be added to the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed.
     */
    virtual IasResult addProcessingModule(IasProcessingModulePtr module) = 0;

    /**
     * @brief Delete processing module from the pipeline
     *
     * A previously added module can be deleted from a pipeline by using this method. Deleting a module
     * will also remove all links to and from the module.
     *
     * @param[in] module The module that shall be deleted from the pipeline
     */
    virtual void deleteProcessingModule(IasProcessingModulePtr module) = 0;

    /**
     * @brief Set the custom configuration properties of the audio module.
     *
     * Details about the concrete configuration properties can be found in each audio modules datasheet.
     *
     * @param[in] module The module for which the properties shall be set
     * @param[in] properties The custom configuration properties of the audio module.
     */
    virtual void setProperties(IasProcessingModulePtr module, const IasProperties &properties) = 0;

    /**
     * @brief Link wave file to an audio pin
     *
     * @param[in] audioPin Audio pin that shall be linked to the wave file
     * @param[in] waveFileParams Parameters of wave file that shall be linked to the audio pin
     *
     * @return The result of the method call
     * @retval eIasOk Wave file successfully linked to the audio pin
     * @retval eIasFailed Failed to link wave file to the audio pin
     */
    virtual IasResult linkWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFileParams &waveFileParams) = 0;

    /**
     * @brief Unlink wave file from an audio pin
     *
     * @param[in] audioPin Audio pin that shall be unlinked from the wave file
     */
    virtual void unlinkWaveFile(IasAudioPinPtr audioPin) = 0;

    /**
     * @brief Link an audio output pin to an audio input pin
     *
     * The output pin and the input pin can also be combined input/output pins. These combined pins
     * are used for in-place processing components.
     *
     * By means of this function, the links between the processing modules can be configured,
     * in order to set up a cascade of processing modules.
     *
     * The linking between two audio pins must be biuniqe. This means that an input pin cannot
     * be linked to more than one output pin. Of course, also an output pin cannot receive
     * PCM frames from more than one input pin.
     *
     * There are two types how an output pin can be linked to an input pin:
     *
     * @li \ref IasAudioPinLinkType "IasAudioPinLinkType::eIasAudioPinLinkTypeImmediate"
     *          represents the normal linking from an output pin to an input pin.
     * @li \ref IasAudioPinLinkType "IasAudioPinLinkType::eIasAudioPinLinkTypeDelayed"
     *          represents a link with a delay of one period.
     *
     * The delayed link type is necessary to implement feed-back loops. A feed-back loop must
     * contain at least one delayed link, because otherwise there is no solution for breaking
     * up the closed loop of dependencies between the modules' input and output signals.
     * This is similar to the structure of a time-discrete IIR filter, which cannot have a
     * feed-back path with zero delay.
     *
     * @param[in] outputPin  Audio output or combined input/output pin
     * @param[in] inputPin   Audio input or combined input/output pin
     * @param[in] linkType   Link type: immediate or delayed
     *
     * @return The result of the method call
     * @retval IasTestFrameworkSetup::eIasOk Method succeeded.
     * @retval IasTestFrameworkSetup::eIasFailed Method failed. Details can be found in the log.
     */
    virtual IasResult link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType) = 0;

    /**
     * @brief Unlink an audio output pin from an audio input pin
     *
     * The output pin and the input pin can also be combined input/output pins. These combined pins
     * are used for in-place processing components.
     *
     * @param[in] outputPin Audio output or combined input/output pin
     * @param[in] inputPin Audio input or combined input/output pin
     */
    virtual void unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin) = 0;
};

/**
 * @brief Function to get a IasISetup::IasResult as string.
 *
 * @return Enum Member
 */
std::string IAS_AUDIO_PUBLIC toString(const IasTestFrameworkSetup::IasResult& type);

} // namespace IasAudio

#endif // AUDIO_DAEMON2_PUBLIC_INC_AUDIO_SMARTX_TESTFWX_IASTESTFRAMEWORKSETUP_HPP_
