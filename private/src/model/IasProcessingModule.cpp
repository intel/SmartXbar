/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasProcessingModule.cpp
 * @date   2016
 * @brief
 */


#include "model/IasProcessingModule.hpp"
#include "model/IasAudioPin.hpp"

namespace IasAudio {

static const std::string cClassName = "IasProcessingModule::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_MODULE "module=" + mParams->instanceName + ":"


IasProcessingModule::IasProcessingModule(IasProcessingModuleParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("MDL", "SmartX Model"))
  ,mParams(params)
  ,mPipeline(nullptr)
  ,mGenericAudioComponent(nullptr)
{
  IAS_ASSERT(params != nullptr);
  mAudioPinSet.clear();
  mAudioInputOutputPinSet.clear();
  mAudioPinMappingSet.clear();
}


IasProcessingModule::~IasProcessingModule()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_MODULE);
  mAudioPinSet.clear();
  mAudioInputOutputPinSet.clear();
  mAudioPinMappingSet.clear();
}


IasProcessingModule::IasResult IasProcessingModule::addToPipeline(IasPipelinePtr pipeline, const std::string &pipelineName)
{
  IAS_ASSERT(pipeline != nullptr); // already checked in IasSetupImpl::addProcessingModule

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE, "Adding processing module to pipeline", pipelineName);

  if (mPipeline != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Module cannot be added to pipeline", pipelineName,
                "because it already belongs to pipeline", mPipelineName);
    return eIasFailed;
  }
  mPipeline     = pipeline;
  mPipelineName = pipelineName;
  return eIasOk;
}


void IasProcessingModule::deleteFromPipeline()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE, "Deleting processing module from pipeline");

  if (mPipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Module cannot be deleted from pipeline, since it has not been added before.");
  }
  mPipeline = nullptr;

  // Loop over all audio pins that belog to this module and
  // delete them from the pipeline and set their direction to undefined.
  for (IasAudioPinSet::const_iterator setIt = mAudioPinSet.begin(); setIt != mAudioPinSet.end(); setIt++)
  {
    IasAudioPinPtr audioPin = *setIt;
    IasAudioPinParamsPtr pinParams = audioPin->getParameters();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE,
                "Deleting audio pin", pinParams->name, "from pipeline.");
    audioPin->deleteFromPipeline();
    audioPin->setDirection(IasAudioPin::eIasPinDirectionUndef);
  }
  mAudioPinSet.clear();
  mAudioPinMappingSet.clear();
}


IasProcessingModule::IasResult IasProcessingModule::addAudioInOutPin(IasAudioPinPtr inOutPin)
{
  IAS_ASSERT(inOutPin != nullptr);                  // already checked in IasSetupImpl::addProcessingModule
  IAS_ASSERT(inOutPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr pinParams = inOutPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE,
              "Adding input/output pin", pinParams->name, "to this module.");

  IasAudioPin::IasResult pinResult = inOutPin->setDirection(IasAudioPin::eIasPinDirectionModuleInputOutput);
  if (pinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error: input/output pin", pinParams->name,
                "has been already configured with direction", toString(inOutPin->getDirection()));
    return eIasFailed;
  }

  pinResult = inOutPin->addToPipeline(mPipeline, mPipelineName);
  if (pinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error while calling IasAudioPin::addToPipeline:", toString(pinResult));
    return eIasFailed;
  }

  auto status = mAudioPinSet.insert(inOutPin);
  if (!status.second)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error while adding input/output pin", pinParams->name,
                "Pin has already been added");
    return eIasFailed;
  }

  status = mAudioInputOutputPinSet.insert(inOutPin);
  if (!status.second)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error while adding input/output pin", pinParams->name,
                "Pin has already been added");
    return eIasFailed;
  }

  return eIasOk;
}


void IasProcessingModule::deleteAudioInOutPin(IasAudioPinPtr inOutPin)
{

  if (inOutPin == nullptr)   // already checked in IasSetupImpl::addProcessingModule
  {
    return;
  }
  IAS_ASSERT(inOutPin->getParameters() != nullptr);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE,
              "Deleting input/output pin", inOutPin->getParameters()->name, "from this module.");

  if (inOutPin->getDirection() != IasAudioPin::eIasPinDirectionModuleInputOutput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Error while deleting input/output pin", inOutPin->getParameters()->name,
                "due to inappropriate pin direction:", toString(inOutPin->getDirection()));
    return;
  }

  inOutPin->deleteFromPipeline();

  // Set the direction of the audio pin to eIasPinDirectionUndef, since it does not belong to a pipeline anymore.
  inOutPin->setDirection(IasAudioPin::eIasPinDirectionUndef);

  // Erase the inOutPin pin from the map of audio pins.
  IasAudioPinSet::size_type numPinsErased = mAudioPinSet.erase(inOutPin);
  if (numPinsErased != 1)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Error while deleting input/output pin", inOutPin->getParameters()->name);
  }

  numPinsErased = mAudioInputOutputPinSet.erase(inOutPin);
  if (numPinsErased != 1)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Error while deleting input/output pin", inOutPin->getParameters()->name);
  }
}


IasProcessingModule::IasResult IasProcessingModule::addAudioPinMapping(IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  IAS_ASSERT(inputPin != nullptr);                   // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(inputPin->getParameters() != nullptr);  // already checked in constructor of IasAudioPin
  IAS_ASSERT(outputPin != nullptr);                  // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(outputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr  inputPinParams =  inputPin->getParameters();
  IasAudioPinParamsPtr outputPinParams = outputPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE,
              "Adding mapping of input pin", inputPinParams->name,
              "to ouput pin", outputPinParams->name);

  IasAudioPin::IasResult pinResult = inputPin->setDirection(IasAudioPin::eIasPinDirectionModuleInput);
  if (pinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error: module input pin", inputPinParams->name,
                "has been already configured with direction", toString(inputPin->getDirection()));
    return eIasFailed;
  }
  pinResult = inputPin->addToPipeline(mPipeline, mPipelineName);
  if (pinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error while calling IasAudioPin::addToPipeline:", toString(pinResult));
    return eIasFailed;
  }

  pinResult = outputPin->setDirection(IasAudioPin::eIasPinDirectionModuleOutput);
  if (pinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error: module output pin", outputPinParams->name,
                "has been already configured with direction", toString(outputPin->getDirection()));
    return eIasFailed;
  }
  pinResult = outputPin->addToPipeline(mPipeline, mPipelineName);
  if (pinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error while calling IasAudioPin::addToPipeline:", toString(pinResult));
    return eIasFailed;
  }

  IasAudioPinMapping pinMapping = IasAudioPinMapping(inputPin, outputPin);
  auto status = mAudioPinMappingSet.insert(pinMapping);
  if (!status.second)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_MODULE,
                "Error: mapping of input pin", inputPinParams->name,
                "to ouput pin", outputPinParams->name,
                "has been already added.");
    return eIasFailed;
  }

  // Insert the input pin and the output pin to the set of audio pins. Ignore the return values
  // of the insert methods, since it is allowed that one audio pin belongs to several mappings.
  mAudioPinSet.insert(inputPin);
  mAudioPinSet.insert(outputPin);

  return eIasOk;
}


void IasProcessingModule::deleteAudioPinMapping(IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  IAS_ASSERT(inputPin != nullptr);                   // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(inputPin->getParameters() != nullptr);  // already checked in constructor of IasAudioPin
  IAS_ASSERT(outputPin != nullptr);                  // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(outputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr  inputPinParams =  inputPin->getParameters();
  IasAudioPinParamsPtr outputPinParams = outputPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_MODULE,
              "Deleting mapping of input pin", inputPinParams->name,
              "to ouput pin", outputPinParams->name);

  if (inputPin->getDirection() != IasAudioPin::eIasPinDirectionModuleInput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Error while deleting input pin", inputPin->getParameters()->name,
                "due to inappropriate pin direction:", toString(inputPin->getDirection()));
    return;
  }
  if (outputPin->getDirection() != IasAudioPin::eIasPinDirectionModuleOutput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Error while deleting output pin", outputPin->getParameters()->name,
                "due to inappropriate pin direction:", toString(outputPin->getDirection()));
    return;
  }

  IasAudioPinMapping pinMapping = IasAudioPinMapping(inputPin, outputPin);
  IasAudioPinMappingSet::size_type numErasedElements = mAudioPinMappingSet.erase(pinMapping);
  if (numErasedElements != 1)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_MODULE,
                "Error: mapping of input pin", inputPinParams->name,
                "to ouput pin", outputPinParams->name,
                "cannot be deleted.");
  }

  bool  inputPinInUse = false;
  bool outputPinInUse = false;
  // Check whether the input pin and output pin still belong to any of the remaining pinMappings.
  for (IasAudioPinMappingSet::const_iterator setIt = mAudioPinMappingSet.begin();
       setIt != mAudioPinMappingSet.end(); setIt++)
  {
    const IasAudioPinMapping& pinMapping = *setIt;
    if (inputPin == pinMapping.inputPin)
    {
      inputPinInUse = true;
    }
    if (outputPin == pinMapping.outputPin)
    {
      outputPinInUse = true;
    }
  }

  // If the input pin does not belong to any of the remaining mappings
  // -> delete it from the set of audio pins that belong to this module.
  if (!inputPinInUse)
  {
    mAudioPinSet.erase(inputPin);
    inputPin->deleteFromPipeline();
    inputPin->setDirection(IasAudioPin::eIasPinDirectionUndef);
  }

  // If the output pin does not belong to any of the remaining mappings
  // -> delete it from the set of audio pins that belong to this module.
  if (!outputPinInUse)
  {
    mAudioPinSet.erase(outputPin);
    outputPin->deleteFromPipeline();
    outputPin->setDirection(IasAudioPin::eIasPinDirectionUndef);
  }
}


void IasProcessingModule::setGenericAudioComponent(IasGenericAudioComp* genericAudioComponent)
{
  mGenericAudioComponent = genericAudioComponent;
}


/*
 * Function to get a IasProcessingModule::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasProcessingModule::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasProcessingModule::eIasOk);
    STRING_RETURN_CASE(IasProcessingModule::eIasFailed);
    DEFAULT_STRING("Invalid IasProcessingModule::IasResult => " + std::to_string(type));
  }
}


} // namespace IasAudio
