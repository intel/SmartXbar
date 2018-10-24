/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigProcessingModule.hpp
 * @date   2017
 * @brief  The smartXConfigParser parsing interface
 */

#ifndef IASCONFIGPROCESSINGMODULEFILE_HPP
#define IASCONFIGPROCESSINGMODULEFILE_HPP

#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "configparser/IasConfigPipeline.hpp"
#include <libxml2/libxml/parser.h>
#include <vector>

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IAS_AUDIO_PUBLIC IasConfigProcessingModule
{
  public:
  /**
   * @brief The result type for the IasConfigProcessingModule methods
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
  IasConfigProcessingModule();

  /*!
   * @brief Destructor.
   */
  ~IasConfigProcessingModule();

  /*!
   * @brief Returns module pin param vector
   *
   * @return vector to module pin pararms
   */
  std::vector<IasAudioPinParams> getModulePinParams();

  /*!
   * @brief Returns module pins vector
   *
   * @return vector to module pins
   */
  std::vector<IasAudioPinPtr> getModulePins();

  /*!
   * @brief Create processing module
   *
   * @param[in]  setup Pointer to the smartx setup
   * @param[in]  pipeline Pointer to the pipeline
   * @param[in]  processingModuleNode Pointer to the processing module node
   * @return The result of creating processing module
   * @retval IasConfigProcessingModule::eIasOk  operation Successful
   * @retval IasConfigProcessingModule::eIasFailed operation Failed
   */
  IasResult initProcessingModule(IasISetup *setup, IasPipelinePtr pipeline, xmlNodePtr processingModuleNode);

  /*!
   * @brief set scaler property
   *
   * @param[in]  scalerPropertyType as String
   * @param[in]  dataType as String
   * @param[in]  scalerPropertyContent as String
   * @return The result of setting scaler property
   * @retval IasResult::eIasOk  operation Successful
   * @retval IasResult::eIasFailed operation Failed
   */
  IasResult setScalerProperty(const std::string& scalerPropertyType, const std::string& dataType, const std::string& scalerPropertyContent);

  /*!
   * @brief set vector property
   *
   * @param[in]  vectorPropertyType as String
   * @param[in]  dataType as String
   * @param[in]  vecNodePtr pointer to vector property node
   * @return The result of setting vector property
   * @retval IasResult::eIasOk  operation Successful
   * @retval IasResult::eIasFailed operation Failed
   */
  IasResult setVectorProperty(const std::string& scalerPropertyType, const std::string& vectorPropertyDataFormat, xmlNodePtr cur_vec_node);

  private:
  //Processing module data members
  IasProcessingModuleParams mModuleParams;                   //!< Member variable to store module params
  IasProcessingModulePtr mModule;                            //!< Member variable to store module pointer
  std::vector<IasAudioPinPtr> mModulePins;                   //!< Vector for module pins
  std::vector<IasAudioPinParams> mModulePinParams;           //!< Vector for module pin params
  IasProperties mModuleProperties;                           //!< Member variable for module properties
  IasStringVector mActivePins;                               //!< String vector for active pins for module

  DltContext *mLog;                                          //!< The DLT log context
};

} // end of IasAudio namespace


#endif // IASCONFIGPIPELINEFILE_HPP
