/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasProcessingModule.hpp
 * @date   2016
 * @brief
 */

#ifndef IASPROCESSINGMODULE_HPP_
#define IASPROCESSINGMODULE_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasGenericAudioComp;

/**
 * Type definition for a set of audio pins (e.g., set of all audio pins that belong to a module).
 */
using IasAudioPinSet = std::set<IasAudioPinPtr>;

/**
 * @brief Shared ptr type for IasAudioPinSet
 */
using IasAudioPinSetPtr = std::shared_ptr<IasAudioPinSet>;

/**
 * @brief Type definition for an audio pin mapping
 */
struct IasAudioPinMapping
{
  /**
   * @brief Constructor with initial values.
   */
  inline IasAudioPinMapping()
    :inputPin(nullptr)
    ,outputPin(nullptr)
  {;}

  /**
   * @brief Constructor for initializer list.
   */
  inline IasAudioPinMapping(IasAudioPinPtr p_inputPin, IasAudioPinPtr p_outputPin)
    :inputPin(p_inputPin)
    ,outputPin(p_outputPin)
  {;}

  /**
   * @brief Compare operator, required to build a set of audio pin mappings.
   */
  friend bool operator<(const IasAudioPinMapping a, const IasAudioPinMapping b)
  {
    if (a.inputPin < b.inputPin)
    {
      return true;
    }
    if (b.inputPin < a.inputPin)
    {
      return false;
    }
    // At this point, we know that the inputPins are equal, so we have to compare the outputPins
    if (a.outputPin < b.outputPin)
    {
      return true;
    }
    return false;
  }

  IasAudioPinPtr inputPin;
  IasAudioPinPtr outputPin;
};

/**
 * Type definition for a set of audio pin mappings.
 */
using IasAudioPinMappingSet = std::set<IasAudioPinMapping>;

/**
 * @brief Shared ptr type for IasAudioPinMappingSet
 */
using IasAudioPinMappingSetPtr = std::shared_ptr<IasAudioPinMappingSet>;


/*!
 * @brief Documentation for class IasProcessingModule
 */
class IAS_AUDIO_PUBLIC IasProcessingModule
{

  public:
    /**
     * @brief The result type for the IasProcessingModule methods
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Other error (operation failed)
    };

    /*!
     *  @brief Constructor.
     */
    IasProcessingModule(IasProcessingModuleParamsPtr params);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasProcessingModule();

    /**
     * @brief Get access to the module parameters
     *
     * @return A read-only pointer to the module parameters
     */
    inline const IasProcessingModuleParamsPtr getParameters() const { return mParams; }

    /**
     * @brief Add the processing module to the given pipeline.
     *
     * @param[in]  pipeline      The pipeline where the processing module shall be added to.
     * @param[in]  pipelineName  The name of the pipeline
     */
    IasResult addToPipeline(IasPipelinePtr pipeline, const std::string &pipelineName);

    /**
     * @brief Delete the processing module from the pipeline, where it has been added before.
     */
    void deleteFromPipeline();

    /**
     * @brief Get access to the pipeline which the module belongs to
     *
     * @return A read-only pointer to the pipeline which the module belongs to
     */
    inline const IasPipelinePtr getPipeline() const { return mPipeline; }

    /**
     * @brief Add an audio input/output pin to the processing module.
     *
     * @param[in] inOutPin Pin that shall be added to module.
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addAudioInOutPin(IasAudioPinPtr inOutPin);

    /**
     * @brief Delete an audio input/output pin from the processing module.
     *
     * @param[in] inOutPin Pin that shall be added to module,
     */
    void deleteAudioInOutPin(IasAudioPinPtr inOutPin);

    /**
     * @brief Add audio pin mapping.
     *
     * @param[in] inputPin Input pin of the mapping pair
     * @param[in] outputPin Output pin of the mapping pair
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addAudioPinMapping(IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);

    /**
     * @brief Delete audio pin mapping.
     *
     * @param[in] inputPin Input pin of the mapping pair to be deleted
     * @param[in] outputPin Output pin of the mapping pair to be deleted
     */
    void deleteAudioPinMapping(IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);

    /**
     * @brief Get a set of all audio pins that belong to this module (combination of all input/output pins and all pins that belong to pin mappings).
     */
    inline const IasAudioPinSetPtr getAudioPinSet() const { return std::make_shared<IasAudioPinSet>(mAudioPinSet); };

    /**
     * @brief Get a set of all audio input/output pins that belong to this module (only those pins that are used for in-place processing).
     */
    inline const IasAudioPinSetPtr getAudioInputOutputPinSet() const { return std::make_shared<IasAudioPinSet>(mAudioInputOutputPinSet); };

    /**
     * @brief Get a set of all audio pin mappings that belong to this module.
     */
    inline const IasAudioPinMappingSetPtr getAudioPinMappingSet() const { return std::make_shared<IasAudioPinMappingSet>(mAudioPinMappingSet); };

    /**
     * @brief Set the associated genericAudioComponent
     *
     * @param[in] genericAudioComponent Handle to the associated genericAudioComponent
     */
    void setGenericAudioComponent(IasGenericAudioComp* genericAudioComponent);

    /**
     * @brief Get the associated genericAudioComponent
     */
    inline IasGenericAudioComp* getGenericAudioComponent() const { return mGenericAudioComponent; };


  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasProcessingModule(IasProcessingModule const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasProcessingModule& operator=(IasProcessingModule const &other);


    /*
     * Member Variables
     */
    DltContext*                   mLog;                     //!< Handle for the DLT log object
    IasProcessingModuleParamsPtr  mParams;                  //!< Pointer to the processing module parameters
    IasPipelinePtr                mPipeline;                //!< Pointer to the associated pipeline (or nullptr if not assigned)
    std::string                   mPipelineName;            //!< Name of the associated pipeline
    IasAudioPinSet                mAudioPinSet;             //!< Set of all audio pins that belong to this module (combination of all input/output pins and all pin mappings).
    IasAudioPinSet                mAudioInputOutputPinSet;  //!< Set of all audio input/output pins (only those pins that are used for in-place processing).
    IasAudioPinMappingSet         mAudioPinMappingSet;      //!< Set of all audio pin mappings that belong to this module.
    IasGenericAudioComp*          mGenericAudioComponent;   //!< Handle for the associated genericAudioComponent

};


/**
 * @brief Function to get a IasProcessingModule::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string IAS_AUDIO_PUBLIC toString(const IasProcessingModule::IasResult& type);


} // namespace IasAudio

#endif // IASPROCESSINGMODULE_HPP_
