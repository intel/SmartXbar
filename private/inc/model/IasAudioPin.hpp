/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasAudioPin.hpp
 * @date   2016
 * @brief  Audio pins are used for connecting processing modules within a pipeline.
 */

#ifndef IASAUDIOPIN_HPP_
#define IASAUDIOPIN_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {


/*!
 * @brief Documentation for class IasAudioPin
 *
 * Audio pins are used for connecting processing modules within a pipeline.
 */
class IAS_AUDIO_PUBLIC IasAudioPin
{

  public:
    /**
     * @brief The result type for the IasAudioPin methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFailed            //!< Operation failed
    };

    /**
     * @brief Pin direction
     */
    enum IasPinDirection
    {
      eIasPinDirectionUndef = 0,          //!< Undefined
      eIasPinDirectionPipelineInput,      //!< Input pin of a pipeline
      eIasPinDirectionPipelineOutput,     //!< Output pin of a pipeline
      eIasPinDirectionModuleInput,        //!< Input pin of a module
      eIasPinDirectionModuleOutput,       //!< Output pin of a module
      eIasPinDirectionModuleInputOutput,  //!< Input and Output pin of a module
    };

    /*!
     *  @brief Constructor.
     */
    IasAudioPin(IasAudioPinParamsPtr params);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioPin();

    /**
     * @brief Get access to the pin parameters
     *
     * @return A read-only pointer to the pin parameters
     */
    const IasAudioPinParamsPtr getParameters() const { return mParams; }

    /**
     * @brief Set the direction of the audio pin.
     *
     * The direction can be changed only,
     * @li if it was eIasPinDirectionUndef before, or
     * @li if it is set to eIasPinDirectionUndef.
     *
     * @param[in]  direction   The direction of the pin.
     * @return                 Error code
     * @retval     eIasOk      Success
     * @retval     eIasFailed  Direction has already been defined (with different value).
     */
    IasResult setDirection(IasPinDirection direction);

    /**
     * @brief Get direction of the audio pin.
     *
     * @return Pin direction.
     */
    IasPinDirection getDirection() const { return mDirection; }


    /**
     * @brief Add the audio pin to the given pipeline.
     *
     * @param[in]  pipeline      The pipeline where the audio pin shall be added to.
     * @param[in]  pipelineName  The name of the pipeline
     */
    IasResult addToPipeline(IasPipelinePtr pipeline, const std::string &pipelineName);

    /**
     * @brief Delete the audio pin from the pipeline, where it has been added before.
     */
    void deleteFromPipeline();

    /**
     * @brief Get access to the pipeline which the pin belongs to
     *
     * @return A read-only pointer to the pipeline which the pin belongs to
     */
    const IasPipelinePtr getPipeline() const { return mPipeline; }



  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioPin(IasAudioPin const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioPin& operator=(IasAudioPin const &other);

    DltContext           *mLog;          //!< Handle for the DLT log object
    IasAudioPinParamsPtr  mParams;       //!< Pointer to the audio pin parameters
    IasPipelinePtr        mPipeline;     //!< Pointer to the associated pipeline (or nullptr if not assigned)
    std::string           mPipelineName; //!< Name of the associated pipeline
    IasPinDirection       mDirection;    //!< Direction of the audio pin
};


/**
 * @brief Function to get a IasAudioPin::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string IAS_AUDIO_PUBLIC toString(const IasAudioPin::IasResult& type);

/*
 * @brief Function to get a IasAudioPin::IasPinDirection as string.
 *
 * @return Enum Member
 * @return eIasPinDirectionInvalid on unknown value.
 */
std::string IAS_AUDIO_PUBLIC toString(const IasAudioPin::IasPinDirection&  type);


} // namespace IasAudio

#endif // IASAUDIOPIN_HPP_
