/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigPipeline.hpp
 * @date   2017
 * @brief  The smartXConfigParser parsing interface
 */


#ifndef IASCONFIGPIPELINEFILE_HPP
#define IASCONFIGPIPELINEFILE_HPP

#include "audio/smartx/IasISetup.hpp"
#include "configparser/IasConfigProcessingModule.hpp"
#include "configparser/IasConfigSinkDevice.hpp"
#include <libxml2/libxml/parser.h>
#include <vector>

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IAS_AUDIO_PUBLIC IasConfigPipeline
{
  public:
    /**
     * @brief The result type for the IasConfigPipeline methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFailed            //!< Operation failed
    };

    /*!
     * @brief Constructor.
     *
     */
    IasConfigPipeline();

    /*!
     * @brief Destructor.
     */
    ~IasConfigPipeline();

    /*!
     * @brief Returns pipeline ptr
     *
     * @return Pointer to the pipeline
     */
    IasPipelinePtr getPipelinePtr() const;

    /*!
     * @brief Returns pipeline input pin params vector
     *
     * @return Vector to pipeline input pins params
     */
    std::vector<IasAudioPinParams> getPipelineInputPinParams();

    /*!
     * @brief Returns pipeline output pin params vector
     *
     * @return Vector to pipeline output pins params
     */
    std::vector<IasAudioPinParams> getPipelineOutputPinParams();

    /*!
     * @brief Returns pipeline input pins vector
     *
     * @return Vector to pipeline input pins
     */
    std::vector<IasAudioPinPtr> getPipelineInputPins();

    /*!
     * @brief Returns pipeline output pins vector
     *
     * @return Vector to pipeline output pins
     */
    std::vector<IasAudioPinPtr> getPipelineOutputPins();

    /*!
     * @brief Create pipeline
     *
     * @param[in]  setup Pointer to the smartx setup
     * @return The result of creating pipeline
     * @retval IasConfigPipeline::eIasOk  operation Successful
     * @retval IasConfigPipeline::eIasFailed operation Failed
     */
    IasResult initPipeline(IasISetup *setup, xmlNodePtr processingPipelineNode);

  private:
    //Processing Pipeline data members
    IasPipelineParams mPipelineParams;                        //!< Member variable to store pipeline params
    IasPipelinePtr mPipeline;                                 //!< Pointer to the pipeline
    std::vector<IasAudioPinPtr> mPipelineInputPins;           //!< Vector for pipeline input pins
    std::vector<IasAudioPinParams> mPipelineInputPinParams;   //!< Vector for pipeline input pin params
    std::vector<IasAudioPinPtr> mPipelineOutputPins;          //!< Vector for pipeline output pins
    std::vector<IasAudioPinParams> mPipelineOutputPinParams;  //!< Vector for pipeline output pin params

    DltContext *mLog;                                         //!< The DLT log context
};

} // end of IasAudio namespace

#endif // IASCONFIGPIPELINEFILE_HPP
