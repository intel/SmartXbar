/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasPipeline.cpp
 * @date   2016
 * @brief  Pipeline for hosting a cascade of processing modules.
 */

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/helper/IasCopyAudioAreaBuffers.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasProcessingModule.hpp"
#include "model/IasPipeline.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "smartx/IasConfiguration.hpp"

namespace IasAudio {


static const std::string cClassName = "IasPipeline::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_PIPELINE "pipeline=" + mParams->name + ":"


IasPipeline::IasPipeline(IasPipelineParamsPtr params, IasPluginEnginePtr pluginEngine, IasConfigurationPtr configuration)
  :mLog(IasAudioLogging::registerDltContext("MDL", "SmartX Model"))
  ,mParams(params)
  ,mPluginEngine(pluginEngine)
  ,mConfiguration(configuration)
{
  IAS_ASSERT(params != nullptr);
  IAS_ASSERT(pluginEngine != nullptr);
  mProcessingModuleSchedulingList.clear();
}


IasPipeline::~IasPipeline()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_PIPELINE);
  IAS_ASSERT(mPluginEngine != nullptr); // already checked in constructor of IasPipeline

  // Call the destroy method of all audio components that are hosted by the pipeline.
  for (IasProcessingModulePtr module :mProcessingModuleSchedulingList)
  {
    IasGenericAudioComp *audioComponent = module->getGenericAudioComponent();
    mPluginEngine->destroyModule(audioComponent);
  }

  // Erase the audio channel buffers that belong to the audio pins.
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
    eraseAudioChannelBuffers(pinConnectionParams);
  }

  mAudioChain.reset();
  mSinkPinMap.clear();
}


IasPipeline::IasResult IasPipeline::init()
{

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Initialization of Pipeline.");
  return eIasOk;
}


IasPipeline::IasResult IasPipeline::addAudioInputPin(IasAudioPinPtr pipelineInputPin)
{
  IAS_ASSERT(pipelineInputPin != nullptr);                  // already checked in IasSetupImpl::addAudioInputPin
  IAS_ASSERT(pipelineInputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr pinParams = pipelineInputPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Adding input pin", pinParams->name, "to pipeline");

  IasAudioPin::IasResult result = pipelineInputPin->setDirection(IasAudioPin::eIasPinDirectionPipelineInput);
  if (result != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error: input pin", pinParams->name,
                "has been already configured with direction", toString(pipelineInputPin->getDirection()));
    return eIasFailed;
  }

  IasAudioPinMap::const_iterator mapIt = mAudioPinMap.find(pipelineInputPin);
  if (mapIt != mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding input pin", pinParams->name,
                "Pin has already been added");
    return eIasFailed;
  }

  IasAudioPinConnectionParamsPtr pinConnectionParams = std::make_shared<IasAudioPinConnectionParams>();
  pinConnectionParams->thisPin = pipelineInputPin;
  createAudioChannelBuffers(pinConnectionParams, pinParams->numChannels, mParams->periodSize);
  IasAudioPinPair tmp = std::make_pair(pipelineInputPin, pinConnectionParams);
  mAudioPinMap.insert(tmp);
  mPipelineInputPins.push_back(pipelineInputPin);
  return eIasOk;
}


IasPipeline::IasResult IasPipeline::addAudioOutputPin(IasAudioPinPtr pipelineOutputPin)
{
  IAS_ASSERT(pipelineOutputPin != nullptr);                  // already checked in IasSetupImpl::addAudioOutputPin
  IAS_ASSERT(pipelineOutputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr pinParams = pipelineOutputPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Adding output pin", pinParams->name, "to pipeline");

  IasAudioPin::IasResult result = pipelineOutputPin->setDirection(IasAudioPin::eIasPinDirectionPipelineOutput);
  if (result != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error: output pin", pinParams->name,
                "has been already configured with direction", toString(pipelineOutputPin->getDirection()));
    return eIasFailed;
  }

  IasAudioPinMap::const_iterator mapIt = mAudioPinMap.find(pipelineOutputPin);
  if (mapIt != mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding input pin", pinParams->name,
                "Pin has already been added");
    return eIasFailed;
  }

  IasAudioPinConnectionParamsPtr pinConnectionParams = std::make_shared<IasAudioPinConnectionParams>();
  pinConnectionParams->thisPin = pipelineOutputPin;
  createAudioChannelBuffers(pinConnectionParams, pinParams->numChannels, mParams->periodSize);
  IasAudioPinPair tmp = std::make_pair(pipelineOutputPin, pinConnectionParams);
  mAudioPinMap.insert(tmp);
  mPipelineOutputPins.push_back(pipelineOutputPin);
  return eIasOk;
}


void IasPipeline::deleteAudioPin(IasAudioPinPtr pipelinePin)
{
  if ((pipelinePin == nullptr) || (pipelinePin->getParameters() == nullptr))
  {
    return;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Deleting pipeline pin", pipelinePin->getParameters()->name, "from pipeline");

  // Erase the audio channel buffers that belong to this pipeline output pin.
  eraseAudioChannelBuffers(pipelinePin);

  // Erase the output pin from the map of audio pins.
  mAudioPinMap.erase(pipelinePin);

  // Set the direction of the audio pin to eIasPinDirectionUndef, since it does not belong to a pipeline anymore.
  pipelinePin->setDirection(IasAudioPin::eIasPinDirectionUndef);
}


IasPipeline::IasResult IasPipeline::addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  IAS_ASSERT(module != nullptr);                    // already checked in IasSetupImpl::addAudioInOutPin
  IAS_ASSERT(inOutPin != nullptr);                  // already checked in IasSetupImpl::addAudioInOutPin
  IAS_ASSERT(module->getParameters() != nullptr);   // already checked in constructor of IasProcessingModule
  IAS_ASSERT(inOutPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr pinParams = inOutPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Adding input/output pin", pinParams->name,
              "to module", module->getParameters()->instanceName);


  // Find the module in the map of processing modules.
  IasProcessingModuleMap::iterator mapItProcessingModule = mProcessingModuleMap.find(module);
  if (mapItProcessingModule == mProcessingModuleMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Processing module has not been added to pipeline yet:", module->getParameters()->instanceName);
    return eIasFailed;
  }

  // Declare the inOutPin to the module.
  IasProcessingModule::IasResult result = module->addAudioInOutPin(inOutPin);

  if (result != IasProcessingModule::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding input/output pin", pinParams->name,
                "to module", module->getParameters()->instanceName,
                "The pin has probably already been added");
    return eIasFailed;
  }

  // Add the audio pin to the map of audio pins that belong to this pipeline.
  IasPipeline::IasResult pipelineResult = addAudioPinToMap(inOutPin, module);
  if (pipelineResult != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding input/output pin", pinParams->name,
                "Pin has already been added to pipeline (with different connection parameters).");
    return eIasFailed;
  }

  return eIasOk;
}


void IasPipeline::deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  if ((module == nullptr) || (inOutPin == nullptr) || // already checked in IasSetupImpl::deleteAudioInOutPin
      (module->getParameters() == nullptr) ||         // already checked in constructor of IasProcessingModule
      (inOutPin->getParameters() == nullptr))         // already checked in constructor of IasAudioPin
  {
    return;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Deleting input/output pin", inOutPin->getParameters()->name,
              "from module", module->getParameters()->instanceName);

  module->deleteAudioInOutPin(inOutPin);

  // Erase the audio channel buffers that belong to this audio pin.
  eraseAudioChannelBuffers(inOutPin);

  // Erase the inOutPin pin from the map of audio pins.
  mAudioPinMap.erase(inOutPin);
}


IasPipeline::IasResult IasPipeline::addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  IAS_ASSERT(module != nullptr);                     // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(inputPin != nullptr);                   // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(outputPin != nullptr);                  // already checked in IasSetupImpl::addAudioPinMapping
  IAS_ASSERT(module->getParameters() != nullptr);    // already checked in constructor of IasProcessingModule
  IAS_ASSERT(inputPin->getParameters() != nullptr);  // already checked in constructor of IasAudioPin
  IAS_ASSERT(outputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  IasAudioPinParamsPtr inputPinParams  = inputPin->getParameters();
  IasAudioPinParamsPtr outputPinParams = outputPin->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Adding mapping of input pin", inputPinParams->name,
              "to ouput pin", outputPinParams->name);

  // Find the module in the map of processing modules.
  IasProcessingModuleMap::iterator mapItProcessingModule = mProcessingModuleMap.find(module);
  if (mapItProcessingModule == mProcessingModuleMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Processing module has not been added to pipeline yet:", module->getParameters()->instanceName);
    return eIasFailed;
  }

  // Declare the new pinMapping to the module.
  IasProcessingModule::IasResult moduleResult = module->addAudioPinMapping(inputPin, outputPin);
  if (moduleResult != IasProcessingModule::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding mapping of input pin", inputPinParams->name,
                "to output pin", outputPinParams->name,
                "to module", module->getParameters()->instanceName);
    return eIasFailed;
  }

  // Add the audio pins to the map of audio pins that belong to this pipeline.
  IasPipeline::IasResult pipelineResult = addAudioPinToMap(inputPin, module);
  if (pipelineResult != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding module input pin", inputPinParams->name,
                "Pin has already been added to pipeline (with different connection parameters).");
    return eIasFailed;
  }
  pipelineResult = addAudioPinToMap(outputPin, module);
  if (pipelineResult != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding module output pin", outputPinParams->name,
                "Pin has already been added to pipeline (with different connection parameters).");
    return eIasFailed;
  }



  return eIasOk;

}


void IasPipeline::deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  if ((module == nullptr) ||
      (inputPin == nullptr) || (outputPin == nullptr) || // already checked in IasSetupImpl::deleteAudioPinMapping
      (module->getParameters() == nullptr) ||            // already checked in constructor of IasProcessingModule
      (inputPin->getParameters() == nullptr) ||          // already checked in constructor of IasAudioPin
      (outputPin->getParameters() == nullptr))           // already checked in constructor of IasAudioPin
  {
    return;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Deleting mapping of input pin", inputPin->getParameters()->name,
              "to ouput pin", outputPin->getParameters()->name);

  module->deleteAudioPinMapping(inputPin, outputPin);

  // If the inputPin does not belong to any mappings anymore, i.e., if its direction
  // is eIasPinDirectionUndef, we can erase the inputPin pin from the map of audio pins.
  if (inputPin->getDirection() == IasAudioPin::eIasPinDirectionUndef)
  {
    eraseAudioChannelBuffers(inputPin);
    mAudioPinMap.erase(inputPin);
  }

  // If the outputPin does not belong to any mappings anymore, i.e., if its direction
  // is eIasPinDirectionUndef, we can erase the outputPin pin from the map of audio pins.
  if (outputPin->getDirection() == IasAudioPin::eIasPinDirectionUndef)
  {
    eraseAudioChannelBuffers(outputPin);
    mAudioPinMap.erase(outputPin);
  }
}


IasPipeline::IasResult IasPipeline::addProcessingModule(IasProcessingModulePtr module)
{
  IAS_ASSERT(module != nullptr); // already checked in IasSetupImpl::addProcessingModule

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Adding processing module", module->getParameters()->instanceName, "to pipeline");

  IasProcessingModuleMap::const_iterator mapIt = mProcessingModuleMap.find(module);
  if (mapIt != mProcessingModuleMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error while adding processing module; module has already been added to this pipeline:",
                module->getParameters()->instanceName);
    return eIasFailed;
  }

  // Create a pair (consisting of the module pointer and the module connection parameters,
  // and add the pair to the map of processing modules.
  IasProcessingModuleConnectionParams moduleConnectionParams;
  IasProcessingModulePair modulePair = std::make_pair(module, moduleConnectionParams);
  mProcessingModuleMap.insert(modulePair);

  return eIasOk;
}


void IasPipeline::deleteProcessingModule(IasProcessingModulePtr module)
{
  if (module == nullptr)
  {
    return;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Deleting processing module", module->getParameters()->instanceName, "from pipeline");

  // Find the module in the map of processing modules.
  IasProcessingModuleMap::iterator mapItProcessingModule = mProcessingModuleMap.find(module);
  if (mapItProcessingModule == mProcessingModuleMap.end())
  {
    return;
  }

  // Unlink all pins that are members of the set of audio pins that belong to this module.
  IasAudioPinSetPtr audioPinSet = module->getAudioPinSet();
  for (IasAudioPinPtr audioPin :(*audioPinSet))
  {
    IasAudioPinMap::iterator mapIt = mAudioPinMap.find(audioPin);
    if (mapIt != mAudioPinMap.end())
    {
      const IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
      IasAudioPinPtr sinkPin   = pinConnectionParams->sinkPin;
      IasAudioPinPtr sourcePin = pinConnectionParams->sourcePin;

      if (sinkPin != nullptr)
      {
        unlink(audioPin, sinkPin);
      }
      if (sourcePin != nullptr)
      {
        unlink(sourcePin, audioPin);
      }

      // Erase the audio channel buffers that belong to this audio pin.
      eraseAudioChannelBuffers(audioPin);

      // Delete the pin from the map of pins that belong to this pipeline.
      mAudioPinMap.erase(mapIt);
    }
  }

  // Erase the module from the map of processing modules
  mProcessingModuleMap.erase(module);
}


IasPipeline::IasResult IasPipeline::link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType)
{
  IAS_ASSERT(outputPin != nullptr);                  // already checked in IasSetupImpl::link
  IAS_ASSERT(inputPin  != nullptr);                  // already checked in IasSetupImpl::link
  IAS_ASSERT(outputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin
  IAS_ASSERT(inputPin->getParameters()  != nullptr); // already checked in constructor of IasAudioPin

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Linking pin", outputPin->getParameters()->name,
              "to pin",      inputPin->getParameters()->name);

  if (inputPin->getParameters()->numChannels != outputPin->getParameters()->numChannels)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link pin", outputPin->getParameters()->name,
                "to pin",          inputPin->getParameters()->name,
                "because number of channels are different:", outputPin->getParameters()->numChannels,
                "vs.",             inputPin->getParameters()->numChannels);
    return eIasFailed;
  }

  IasAudioPinMap::iterator mapItOutputPin = mAudioPinMap.find(outputPin);
  IasAudioPinMap::iterator mapItInputPin  = mAudioPinMap.find(inputPin);

  if (mapItOutputPin == mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link output pin (has not been added to pipeline before):", outputPin->getParameters()->name);
    return eIasFailed;
  }
  if (mapItInputPin == mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link input pin (has not been added to pipeline before):", inputPin->getParameters()->name);
    return eIasFailed;
  }

  IAS_ASSERT(outputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin
  IAS_ASSERT( inputPin->getParameters() != nullptr); // already checked in constructor of IasAudioPin

  if ((outputPin->getDirection() != IasAudioPin::eIasPinDirectionPipelineInput) &&
      (outputPin->getDirection() != IasAudioPin::eIasPinDirectionModuleOutput) &&
      (outputPin->getDirection() != IasAudioPin::eIasPinDirectionModuleInputOutput))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link pin", outputPin->getParameters()->name,
                "to pin",          inputPin->getParameters()->name,
                "because pin",     outputPin->getParameters()->name,
                "is of direction", toString(outputPin->getDirection()));
    return eIasFailed;
  }

  if ((inputPin->getDirection() != IasAudioPin::eIasPinDirectionPipelineOutput) &&
      (inputPin->getDirection() != IasAudioPin::eIasPinDirectionModuleInput) &&
      (inputPin->getDirection() != IasAudioPin::eIasPinDirectionModuleInputOutput))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link pin", outputPin->getParameters()->name,
                "to pin",          inputPin->getParameters()->name,
                "because pin",     inputPin->getParameters()->name,
                "is of direction", toString(outputPin->getDirection()));
    return eIasFailed;
  }

  IasAudioPinConnectionParamsPtr outputPinConnectionParams = mapItOutputPin->second;
  IasAudioPinConnectionParamsPtr  inputPinConnectionParams = mapItInputPin->second;

  if (outputPinConnectionParams->sinkPin != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link pin", outputPin->getParameters()->name,
                "to pin",          inputPin->getParameters()->name,
                "because",         outputPin->getParameters()->name,
                "is already linked to", outputPinConnectionParams->sinkPin->getParameters()->name);
    return eIasFailed;
  }

  if (inputPinConnectionParams->sourcePin != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link pin", outputPin->getParameters()->name,
                "to pin",          inputPin->getParameters()->name,
                "because",         inputPin->getParameters()->name,
                "is already linked to", inputPinConnectionParams->sourcePin->getParameters()->name);
    return eIasFailed;
  }

  outputPinConnectionParams->sinkPin           = inputPin;
  inputPinConnectionParams->sourcePin          = outputPin;
  inputPinConnectionParams->isInputDataDelayed = (linkType == IasAudioPinLinkType::eIasAudioPinLinkTypeDelayed);

  // if this is a link via a delay element, create audio channel buffers for the input pin.
  if (linkType == IasAudioPinLinkType::eIasAudioPinLinkTypeDelayed)
  {
    createAudioChannelBuffers(inputPinConnectionParams, inputPin->getParameters()->numChannels, mParams->periodSize);
  }

  return eIasOk;
}

void IasPipeline::getAudioPinMap(IasAudioPinPtr audioPin, std::vector<std::string>& audioPinMap)
{
  getPinConnectionInfo(audioPin, audioPinMap);
}

void IasPipeline::getPinConnectionInfo(IasAudioPinPtr audioPin, std::vector<std::string>& audioPinMap)
{
  IasAudioPinMap::iterator mapItPin = mAudioPinMap.find(audioPin);
  if (mapItPin == mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot find audioPin", audioPin->getParameters()->name,
                "in audio pin map.");
    return;
  }
  IasAudioPinConnectionParamsPtr audioPinConnectionParams = mapItPin->second;

  if (audioPinConnectionParams->sinkPin != nullptr)
  {
    audioPinMap.push_back("sink:" + audioPinConnectionParams->sinkPin->getParameters()->name);
  }
  if (audioPinConnectionParams->sourcePin != nullptr)
  {
    audioPinMap.push_back("source:" + audioPinConnectionParams->sourcePin->getParameters()->name);
  }
  if (audioPinConnectionParams->isInputDataDelayed == true)
  {
    audioPinMap.push_back("Delayed");
  }
  else
  {
    audioPinMap.push_back("Immediate");
  }
}

void IasPipeline::unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin)
{
  IAS_ASSERT(inputPin  != nullptr); // already checked in IasSetupImpl::link
  IAS_ASSERT(outputPin != nullptr); // already checked in IasSetupImpl::link

  IasAudioPinMap::iterator mapItInputPin  = mAudioPinMap.find(inputPin);
  IasAudioPinMap::iterator mapItOutputPin = mAudioPinMap.find(outputPin);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Unlinking pin", outputPin->getParameters()->name,
              "from pin",       inputPin->getParameters()->name);

  if (mapItInputPin != mAudioPinMap.end())
  {
    mapItInputPin->second->sourcePin = nullptr;
  }
  if (mapItOutputPin != mAudioPinMap.end())
  {
    mapItOutputPin->second->sinkPin = nullptr;
  }
}




IasPipeline::IasResult IasPipeline::link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  IAS_ASSERT(inputPort != nullptr);   // already checked in IasSetupImpl::link()
  IAS_ASSERT(pipelinePin != nullptr); // already checked in IasSetupImpl::link()

  const IasAudioPortParamsPtr inputPortParams = inputPort->getParameters();
  IAS_ASSERT(inputPortParams != nullptr); // already checked in constructor of IasAudioPort

  const IasAudioPinParamsPtr pinParams = pipelinePin->getParameters();
  IAS_ASSERT(pinParams != nullptr); // already checked in constructor of IasAudioPin

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Linking input port", inputPortParams->name,
              "to pipeline pin",    pinParams->name);

  IAS_ASSERT(inputPortParams->numChannels == pinParams->numChannels); // already checked in IasSetupImpl::link

  // Check whether the audioPin actually belongs to the pipeline.
  IasAudioPinMap::iterator mapIt = mAudioPinMap.find(pipelinePin);
  if (mapIt == mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot link pipeline pin", pinParams->name,
                "with audio port", inputPortParams->name,
                "since pipeline does not know this pin.");
    return eIasFailed;
  }

  IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

  IasAudioPort::IasResult audioPortResult = inputPort->getRingBuffer(&pinConnectionParams->audioPortRingBuffer);
  if (audioPortResult != IasAudioPort::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error during IasAudioPort::getRingBuffer");
    return eIasFailed;
  }

  pinConnectionParams->audioPort = inputPort;
  pinConnectionParams->audioPortChannelIdx = inputPortParams->index;

  IasAudioPin::IasPinDirection direction = pipelinePin->getDirection();
  if (direction == IasAudioPin::eIasPinDirectionPipelineOutput)
  {
    // If the pin is a pipeline output pin, we know that the destination port
    // is a sink device input port.
    // If the pin would be a pipeline input pin, then the port would be an input port
    // of a routing zone, which is not what we are interested in here.
    // Make an additional entry into the sink to pin mapping
    // Multiple different pins maybe assigned to one owning sink device
    IasAudioPortOwnerPtr owner = nullptr;
    inputPort->getOwner(&owner);
    IAS_ASSERT(owner != nullptr);   // already checked in IasSetupImpl::link
    mSinkPinMap[owner->getName()].insert(pinConnectionParams);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "Successfully added mapping for pinConnectionParams of pin", pinParams->name, "to", owner->getName(), "sink");
  }

  return eIasOk;
}

IasPipeline::IasResult IasPipeline::getPipelinePinConnectionLink(IasAudioPinPtr pipelinePin, IasAudioPortPtr& inputPort)
{
   // Check whether the audioPin actually belongs to the pipeline.
   IasAudioPinMap::iterator mapIt = mAudioPinMap.find(pipelinePin);
   if (mapIt == mAudioPinMap.end())
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"The audio pin doesn't belong to the pipeline");
     return eIasFailed;
   }

   inputPort = nullptr;
   IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
   inputPort = pinConnectionParams->audioPort;

   return eIasOk;
}


void IasPipeline::unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  IAS_ASSERT(inputPort != nullptr);   // already checked in IasSetupImpl::unlink()
  IAS_ASSERT(pipelinePin != nullptr); // already checked in IasSetupImpl::unlink()

  const IasAudioPortParamsPtr inputPortParams = inputPort->getParameters();
  IAS_ASSERT(inputPortParams != nullptr); // already checked in constructor of IasAudioPort

  const IasAudioPinParamsPtr pinParams = pipelinePin->getParameters();
  IAS_ASSERT(pinParams != nullptr); // already checked in constructor of IasAudioPin

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
              "Unlinking input port", inputPortParams->name,
              "from pipeline pin",    pinParams->name);

  IAS_ASSERT(inputPortParams->numChannels == pinParams->numChannels); // already checked in IasSetupImpl::unlink

  // Check whether the audioPin actually belongs to the pipeline.
  IasAudioPinMap::iterator mapIt = mAudioPinMap.find(pipelinePin);
  if (mapIt == mAudioPinMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Cannot unlink pipeline pin", pinParams->name,
                "from audio port", inputPortParams->name,
                "since pipeline does not know this pin.");
    return;
  }

  IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

  IasAudioPort::IasResult audioPortResult = inputPort->getRingBuffer(&pinConnectionParams->audioPortRingBuffer);
  if (audioPortResult != IasAudioPort::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,
                "Error during IasAudioPort::getRingBuffer");
    return;
  }

  IasAudioPin::IasPinDirection direction = pipelinePin->getDirection();
  if (direction == IasAudioPin::eIasPinDirectionPipelineOutput)
  {
    // If the pin is a pipeline output pin, we know that the destination port
    // is a sink device input port.
    // If the pin would be a pipeline input pin, then the port would be an input port
    // of a routing zone, which is not what we are interested in here.
    // Delete the entry from the sink to pin mapping
    // Multiple different pins maybe assigned to one owning sink device
    IasAudioPortOwnerPtr owner = nullptr;
    inputPort->getOwner(&owner);
    IAS_ASSERT(owner != nullptr);   // already checked in IasSetupImpl::unlink
    auto entryIt = mSinkPinMap[owner->getName()].find(pinConnectionParams);
    if (entryIt != mSinkPinMap[owner->getName()].end())
    {
      mSinkPinMap[owner->getName()].erase(entryIt);
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Successfully removed mapping for pinConnectionParams of pin", pinParams->name, "from", owner->getName(), "sink");
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Mapping for pinConnectionParams of pin", pinParams->name, "from", owner->getName(), "sink could not be found in sink pin mapping");
    }
  }
}


void IasPipeline::dumpConnectionParameters() const
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "----------------------------------");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Connection parameters of all pins:");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "----------------------------------");
  for (IasAudioPinMap::const_iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    const IasAudioPinParamsPtr         pinParams           = mapIt->first->getParameters();
    const IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Connection parameters of pin:", pinParams->name);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  pinDirection       :", toString(mapIt->first->getDirection()));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  isInputDataDelayed :", pinConnectionParams->isInputDataDelayed);

    if (pinConnectionParams->sourcePin != nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  sourcePin          :", pinConnectionParams->sourcePin->getParameters()->name);
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  sourcePin          : n/c");
    }

    if (pinConnectionParams->sinkPin != nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  sinkPin            :", pinConnectionParams->sinkPin->getParameters()->name);
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  sinkPin            : n/c");
    }

    if (pinConnectionParams->processingModule != nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  connected to module:", pinConnectionParams->processingModule->getParameters()->instanceName);
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  connected to module: n/c");
    }
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "-------------------------------------");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Connection parameters of all modules:");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "-------------------------------------");
  for (IasProcessingModuleMap::const_iterator mapIt = mProcessingModuleMap.begin(); mapIt != mProcessingModuleMap.end(); mapIt++)
  {
    const IasProcessingModulePtr& module = mapIt->first;
    const IasAudioPinSetPtr audioPinSet  = module->getAudioPinSet();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Pins that have been added to module:", module->getParameters()->instanceName);
    for (const IasAudioPinPtr audioPin :(*audioPinSet))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, " ", audioPin->getParameters()->name);
    }
  }
}


void IasPipeline::dumpProcessingSequence() const
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "----------------------------------------");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Modules will be processed in this order:");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "----------------------------------------");

  for (const IasProcessingModulePtr module :mProcessingModuleSchedulingList)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, module->getParameters()->instanceName);

    IasAudioPinSetPtr audioInputOutputPins = module->getAudioInputOutputPinSet();
    for (const IasAudioPinPtr audioPin :(*audioInputOutputPins))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  Input/output pin: ", audioPin->getParameters()->name);
    }

    IasAudioPinMappingSetPtr audioPinMappingSet = module->getAudioPinMappingSet();
    for (const IasAudioPinMapping pinMapping :(*audioPinMappingSet))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "  Pin mappings: from", pinMapping.inputPin->getParameters()->name,
                  "to", pinMapping.outputPin->getParameters()->name);
    }
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "----------------------------------------");
}


/*
 * @brief Initialize the pipeline's audio chain of processing modules.
 */
IasPipeline::IasResult IasPipeline::initAudioChain()
{
  IAS_ASSERT(mParams != nullptr);

  // Create and initialize the audio chain that belongs to this pipeline.
  mAudioChain = std::make_shared<IasAudioChain>();
  IasAudioChain::IasInitParams audioChainInitParams(mParams->periodSize, mParams->samplerate);
  IasAudioChain::IasResult audioChainResult = mAudioChain->init(audioChainInitParams);
  if (audioChainResult != IasAudioChain::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE, "Error during IasAudioChain::init:", toString(audioChainResult));
    return eIasFailed;
  }

  identifyProcessingSequence();

  IasPipeline::IasResult pipelineResult = initAudioStreams();
  if (pipelineResult != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE, "Error during IasPipeline::initAudioStreams:", toString(pipelineResult));
    return eIasFailed;
  }

  return eIasOk;
}


/*
 * @brief Provide input data for the pipeline.
 */
IasPipeline::IasResult IasPipeline::provideInputData(const IasAudioPortPtr routingZoneInputPort,
                                                     uint32_t              inputBufferOffset,
                                                     uint32_t              numFramesToRead,
                                                     uint32_t              numFramesToWrite,
                                                     uint32_t*             numFramesRemaining)
{
  *numFramesRemaining = 0;

  // Iterate over all pipeline input pins.
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    const IasAudioPinPtr         currentPin = mapIt->first;
    IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

    // If we have found the pin that is connected to the specified routingZoneInputPort,
    // copy the PCM frames from the routingZoneInputPort to the audio stream that belongs to the audio pin.
    if (pinConnectionParams->audioPort == routingZoneInputPort)
    {
      if (currentPin->getDirection() != IasAudioPin::eIasPinDirectionPipelineInput)
      {
        IasAudioPinParamsPtr pinParams = currentPin->getParameters();
        IAS_ASSERT(pinParams != nullptr);
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE, "Invalid direction of pipeline input pin",
                    pinParams->name);
        return eIasFailed;
      }

      // Get information from pipeline input pin and its audio input frame.
      uint32_t audioStreamId = pinConnectionParams->audioStreamId;
      IAS_ASSERT(audioStreamId < mAudioStreams.size());
      IasAudioStream* audioStream = mAudioStreams[audioStreamId].audioStream;
      IasAudioFrame* audioFrame = audioStream->getInputAudioFrame();
      const uint32_t audioFrameNumChannels = static_cast<uint32_t>(audioFrame->size());

      // Get information from routing zone input port and its ring buffer.
      const IasAudioRingBuffer* audioPortRingBuffer = pinConnectionParams->audioPortRingBuffer;
      IAS_ASSERT(audioPortRingBuffer != nullptr);

      IasAudioCommonDataFormat audioPortDataFormat;
      const uint32_t        audioPortNumChannels = audioPortRingBuffer->getNumChannels();
      IasAudioRingBufferResult ringBufferResult = audioPortRingBuffer->getDataFormat(&audioPortDataFormat);
      const uint32_t        audioPortChanIdx = 0; // always starts with channel 0, since each routing zone input port has its own ring buffer
      IAS_ASSERT(audioPortNumChannels >= 1);
      IAS_ASSERT(audioPortNumChannels == audioFrameNumChannels);
      IAS_ASSERT(ringBufferResult == eIasRingBuffOk);
      IAS_ASSERT(audioPortDataFormat != IasAudioCommonDataFormat::eIasFormatUndef);
      (void)ringBufferResult;

      IasAudioArea* audioPortAreas;
      ringBufferResult = audioPortRingBuffer->getAreas(&audioPortAreas);
      IAS_ASSERT(ringBufferResult == eIasRingBuffOk);

      if (numFramesToWrite + pinConnectionParams->numBufferedFramesInput > mParams->periodSize)
      {
        numFramesToWrite = mParams->periodSize - pinConnectionParams->numBufferedFramesInput;
      }

      // Determine whether the data from AudioPort is non-interleaved.
      bool isNonInterleaved = audioPortAreas[0].step == 8u * static_cast<uint32_t>(toSize(audioPortDataFormat));

      if (isNonInterleaved &&
          (numFramesToRead == mParams->periodSize) && (numFramesToWrite == mParams->periodSize) &&
          (pinConnectionParams->numBufferedFramesInput == 0))
      {
        // If we have received a full period of PCM frames in non-interleaved layout
        // from the routing zone input port, let the audioStream directly read from
        // the ring buffer of the routing zone input port.
        for (uint32_t cntChannels = 0; cntChannels < audioFrameNumChannels; cntChannels++)
        {
          // Let the pointers within audioFrame point to the conversion buffers.
          uint32_t firstIndex = audioPortAreas[cntChannels].first >> 5; // divide by word length (32 bit for Float32)
          (*audioFrame)[cntChannels] = ((float*)(audioPortAreas[cntChannels].start)) + firstIndex + inputBufferOffset;
        }
      }
      else
      {
        // If we have received only a part of a period from the routing zone input port or if the
        // data is not non-interleaved, we have to copy the data into an intermediate buffer.
        // The intermediate buffer is the buffer that belongs to the pipeline input pin.
        // Later, we'll let the audioStream read from the intermediate buffer.
        std::vector<IasAudioArea> audioFrameAreas{audioFrameNumChannels};
        for (uint32_t cntChannels = 0; cntChannels < audioFrameNumChannels; cntChannels++)
        {
          float* channelBuffer = pinConnectionParams->audioChannelBuffers[cntChannels];
          (*audioFrame)[cntChannels] = channelBuffer;
          audioFrameAreas[cntChannels].start = channelBuffer;
          audioFrameAreas[cntChannels].first = 0;
          audioFrameAreas[cntChannels].step  = static_cast<uint32_t>(8 * sizeof(float)); // expressed in bits
          audioFrameAreas[cntChannels].index = 0;
          audioFrameAreas[cntChannels].maxIndex = audioFrameNumChannels-1;
        }

        // Copy from the ring buffer of the routing zone input port to the intermediate buffer.
        copyAudioAreaBuffers(audioFrameAreas.data(), eIasFormatFloat32, pinConnectionParams->numBufferedFramesInput,
                             audioFrameNumChannels, 0,
                             numFramesToWrite,
                             audioPortAreas, audioPortDataFormat, inputBufferOffset,
                             audioPortNumChannels, audioPortChanIdx,
                             numFramesToRead);
      }

      pinConnectionParams->numBufferedFramesInput += numFramesToWrite;
      *numFramesRemaining = mParams->periodSize - pinConnectionParams->numBufferedFramesInput;
    }
  }

  return eIasOk;
}


/*
 * @brief Execute the audio chain of processing modules.
 */
void IasPipeline::process()
{
  IAS_ASSERT(mAudioChain != nullptr);

  // Iterate over all pipeline pins.
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    const IasAudioPinPtr         currentPin          = mapIt->first;
    IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

    // For all pins at the input of the pipeline: copy the data that has been provided
    // via the audioFrames (in function IasPipeline::provideInputData) to the audio streams.
    if (currentPin->getDirection() == IasAudioPin::eIasPinDirectionPipelineInput)
    {
      pinConnectionParams->numBufferedFramesInput = 0;

      uint32_t audioStreamId = pinConnectionParams->audioStreamId;
      IAS_ASSERT(audioStreamId < mAudioStreams.size());
      IasAudioStream* audioStream = mAudioStreams[audioStreamId].audioStream;
      audioStream->copyFromInputAudioChannels();
    }
  }
  mAudioChain->clearOutputBundleBuffers();
  mAudioChain->process();

  // Now we transfer the PCM frames between all audio pins that are linked via delay elements.
  //
  // Iterate over all pipeline pins.
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    const IasAudioPinPtr         currentPin          = mapIt->first;
    IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

    // Check whether this pin gets its PCM frames via a delay element.
    if (pinConnectionParams->isInputDataDelayed)
    {
      const IasAudioPinPtr& sourcePin   = pinConnectionParams->sourcePin;
      const uint32_t sourceStreamId  = mAudioPinMap[sourcePin]->audioStreamId;
      const uint32_t destinStreamId  = pinConnectionParams->audioStreamId;
      IasAudioStream* sourceAudioStream = mAudioStreams[sourceStreamId].audioStream;
      IasAudioStream* destinAudioStream = mAudioStreams[destinStreamId].audioStream;
      IasAudioFrame*  sourceAudioFrame  = sourceAudioStream->getOutputAudioFrame();
      IasAudioFrame*  destinAudioFrame  = destinAudioStream->getInputAudioFrame();

      IAS_ASSERT(sourceAudioFrame->size() == destinAudioFrame->size());
      IAS_ASSERT(sourceAudioFrame->size() == pinConnectionParams->audioChannelBuffers.size());
      const uint32_t numChannels = static_cast<uint32_t>(sourceAudioFrame->size());

      // Let the audioFrame pointers of the source stream and of the
      // destination stream point to the audioChannelBuffers.
      for (uint32_t cntChannels = 0; cntChannels < numChannels; cntChannels++)
      {
        float* channelBuffer = pinConnectionParams->audioChannelBuffers[cntChannels];
        (*sourceAudioFrame)[cntChannels] = channelBuffer;
        (*destinAudioFrame)[cntChannels] = channelBuffer;
      }

      // Copy from sourceAudioStream into audioChannelBuffers.
      sourceAudioStream->copyToOutputAudioChannels();

      // Copy from audioChannelBuffers to destinAudioStream
      destinAudioStream->copyFromInputAudioChannels();
    }
  }
}

/*
 * @brief Retrieve output data from the pipeline.
 */
void IasPipeline::retrieveOutputData(IasAudioSinkDevicePtr     sinkDevice,
                                     IasAudioArea const       *destinAreas,
                                     IasAudioCommonDataFormat  destinFormat,
                                     uint32_t               destinNumFrames,
                                     uint32_t               destinOffset)
{
  // Iterate over all pipeline pins of this sink device.
  auto result = mSinkPinMap.find(sinkDevice->getName());
  if (result != mSinkPinMap.end())
  {
    auto &pinSet = result->second;
    for (auto &entry : pinSet)
    {
      retrieveOutputData(entry, destinAreas, destinFormat, destinNumFrames, destinOffset);
    }
  }
}

void IasPipeline::retrieveOutputData(const IasAudioPinConnectionParamsPtr pinConnectionParams,
                                     IasAudioArea const       *destinAreas,
                                     IasAudioCommonDataFormat  destinFormat,
                                     uint32_t               destinNumFrames,
                                     uint32_t               destinOffset)
{
  IAS_ASSERT(pinConnectionParams != nullptr);
  IAS_ASSERT(destinAreas != nullptr);         // already checked in IasRoutingZoneWorkerThread::transferPeriod
  const IasAudioPinPtr currentPin = pinConnectionParams->thisPin;

  // For all pins at the output of the pipeline: copy the data from the audio streams
  // to the audioFrame. Finally, we can copy from the audioFrame into the buffer of
  // the sink device including format conversion.
  if (currentPin->getDirection() == IasAudioPin::eIasPinDirectionPipelineOutput)
  {
    const IasAudioPinParamsPtr pinParams = currentPin->getParameters();
    IAS_ASSERT(pinParams != nullptr);

    const uint32_t audioStreamId = pinConnectionParams->audioStreamId;
    const uint32_t channelIndex  = pinConnectionParams->audioPortChannelIdx;
    const uint32_t numChannels   = pinParams->numChannels;

    IAS_ASSERT(audioStreamId < mAudioStreams.size());
    IasAudioStream* audioStream = mAudioStreams[audioStreamId].audioStream;

    // Get a frame with pointers to the internal channel buffers of the audio stream. Get also the stride,
    // because the internal buffers can be based on interleaved, non-interleaved, or bundled layout.
    IasAudioFrame* audioFrame = nullptr;
    uint32_t    stride = 0;
    audioStream->getAudioDataPointers(&audioFrame, &stride);
    if (audioFrame == nullptr)
    {
      // Log already done inside method getAudioDataPointers
      return;
    }

    // Prepare the areas describing the internal channel buffers of the audio stream.
    IAS_ASSERT(static_cast<uint32_t>(audioFrame->size()) == numChannels);
    std::vector<IasAudioArea> audioFrameAreas{numChannels};
    for (uint32_t cntChannels = 0; cntChannels < numChannels; cntChannels++)
    {
      audioFrameAreas[cntChannels].start = (*audioFrame)[cntChannels];
      audioFrameAreas[cntChannels].first = 0;
      audioFrameAreas[cntChannels].step  = static_cast<uint32_t>(8u * sizeof(float) * stride); // expressed in bits
      audioFrameAreas[cntChannels].index = 0;
      audioFrameAreas[cntChannels].maxIndex = numChannels-1;
    }

    // Copy from the internal channel buffers of the audioStream into the buffer of the sink device.
    copyAudioAreaBuffers(destinAreas, destinFormat, destinOffset,
                         numChannels, channelIndex,
                         destinNumFrames,
                         audioFrameAreas.data(), eIasFormatFloat32, 0,
                         numChannels, 0,
                         mParams->periodSize);
  }
}


void IasPipeline::getProcessingModules(IasPipeline::IasProcessingModuleVector* processingModules)
{
  IAS_ASSERT(processingModules != nullptr);
  processingModules->clear();
  for (IasProcessingModuleMap::iterator mapIt = mProcessingModuleMap.begin(); mapIt != mProcessingModuleMap.end(); mapIt++)
  {
    const IasProcessingModulePtr& modulePtr = mapIt->first;
    processingModules->push_back(modulePtr);
  }
}


void IasPipeline::getAudioPins(IasPipeline::IasAudioPinVector* audioPins)
{
  IAS_ASSERT(audioPins != nullptr);
  audioPins->clear();
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    const IasAudioPinPtr& audioPinPtr = mapIt->first;
    audioPins->push_back(audioPinPtr);
  }
}

IasAudioPinVector IasPipeline::getPipelineInputPins()
{
  if (mPipelineInputPins.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "No input pins defined for pipeline");
  }
  return mPipelineInputPins;
}

IasAudioPinVector IasPipeline::getPipelineOutputPins()
{
  if (mPipelineOutputPins.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "No output pins defined for pipeline");
  }
  return mPipelineOutputPins;
}

/*
 * @brief Private method: identify in which order the processing modules have to be scheduled.
 */
void IasPipeline::identifyProcessingSequence()
{
  // Setup Sequencer: set the outputDataAvailable flag for all input pins to true.
  // For all other pins, the flag is set to false.
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    const IasAudioPin::IasPinDirection pinDirection  = mapIt->first->getDirection();
    const IasAudioPinParamsPtr   pinParams           = mapIt->first->getParameters();
    IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

    // Set inputDataAvailable to true for all audio pins that receive their data from the source pin via a delayed link.
    pinConnectionParams->inputDataAvailable  = (pinConnectionParams->isInputDataDelayed);

    // Set outputDataAvailable to true for all pipeline input pins.
    pinConnectionParams->outputDataAvailable = (pinDirection == IasAudioPin::eIasPinDirectionPipelineInput);
  }

  // Setup Sequencer: clear the isAlsreadyProcessed flag of all processing modules.
  for (IasProcessingModuleMap::iterator mapIt = mProcessingModuleMap.begin(); mapIt != mProcessingModuleMap.end(); mapIt++)
  {
    IasProcessingModuleConnectionParams& moduleConnectionParams = mapIt->second;
    moduleConnectionParams.isAlsreadyProcessed = false;
  }

  mProcessingModuleSchedulingList.clear();
  bool isOutputDataAvailablePipelineOutput  = false;
  bool isModuleScheduledDuringThisIteration = true;
  uint32_t cntIterations = 0;

  // Execute the loop of the scheduler until all output pins of the pipeline provide PCM data.
  // Break the loop if the previous interation of the loop was completed without scheduling
  // any of the processing modules, since this indicates an incorrect topology.
  while ((!isOutputDataAvailablePipelineOutput) && isModuleScheduledDuringThisIteration)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "---------------------------------------------------------------------------");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Pipeline Sequencer, starting iteration", cntIterations);
    cntIterations++;

    isOutputDataAvailablePipelineOutput  = true;
    isModuleScheduledDuringThisIteration = false;
    for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
    {
      const IasAudioPinParamsPtr         pinParams           = mapIt->first->getParameters();
      const IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;

      // If the pin is connected to a sink pin, update the sink pin's inputDataAvailable flag.
      if (pinConnectionParams->sinkPin != nullptr)
      {
        mAudioPinMap[pinConnectionParams->sinkPin]->inputDataAvailable |= pinConnectionParams->outputDataAvailable;
      }

    }

    // Print the status of all audio pins and dDecide whether all output pins of the pipeline have data available.
    for (IasAudioPinMap::const_iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
    {
      const IasAudioPin::IasPinDirection pinDirection        = mapIt->first->getDirection();
      const IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Audio pin:", mapIt->first->getParameters()->name,
                  ", inputDataAvailable:",  pinConnectionParams->inputDataAvailable,
                  ", outputDataAvailable:", pinConnectionParams->outputDataAvailable);

      if (pinDirection == IasAudioPin::eIasPinDirectionPipelineOutput)
      {
        isOutputDataAvailablePipelineOutput = isOutputDataAvailablePipelineOutput && pinConnectionParams->inputDataAvailable;
      }
    }

    // Check for each module in the ProcessingModuleMap, whether all input pins
    // and all and input/output pins have input data available.
    // If this is fulfilled, mark the the module as "already processed".
    for (IasProcessingModuleMap::iterator mapIt = mProcessingModuleMap.begin(); mapIt != mProcessingModuleMap.end(); mapIt++)
    {
      IasProcessingModuleConnectionParams& moduleConnectionParams = mapIt->second;
      if (!moduleConnectionParams.isAlsreadyProcessed)
      {
        IasAudioPinSetPtr audioPinSet = mapIt->first->getAudioPinSet();
        bool isDataAvailableAllInputPins = true;
        for (const IasAudioPinPtr audioPin :(*audioPinSet))
        {
          bool isInputPin = ((audioPin->getDirection() == IasAudioPin::eIasPinDirectionModuleInput) ||
                                  (audioPin->getDirection() == IasAudioPin::eIasPinDirectionModuleInputOutput));
          if (isInputPin && !(mAudioPinMap[audioPin]->inputDataAvailable))
          {
            isDataAvailableAllInputPins = false;
          }
        }

        if (isDataAvailableAllInputPins)
        {
          moduleConnectionParams.isAlsreadyProcessed = true;
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Processing module", mapIt->first->getParameters()->instanceName, "can be processed now");
          mProcessingModuleSchedulingList.push_back(mapIt->first);
          isModuleScheduledDuringThisIteration = true;
        }

        for (const IasAudioPinPtr audioPin :(*audioPinSet))
        {
          bool isOutputPin = ((audioPin->getDirection() == IasAudioPin::eIasPinDirectionModuleOutput) ||
                                   (audioPin->getDirection() == IasAudioPin::eIasPinDirectionModuleInputOutput));
          if (isOutputPin)
          {
            mAudioPinMap[audioPin]->outputDataAvailable = isDataAvailableAllInputPins;
          }
        }
      }
    }
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "---------------------------------------------------------------------------");
  if (isOutputDataAvailablePipelineOutput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Pipeline sequencer has been completed successfully.");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Incorrect topology of the pipeline: last iteration of the sequencer");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "was completed without scheduling any of the processing modules!");
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "---------------------------------------------------------------------------");
}


/*
 * @brief Private method: identify all required audio streams and initialize them.
 */
IasPipeline::IasResult IasPipeline::initAudioStreams()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "---------------------------------------------------------");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Associated streams:");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "---------------------------------------------------------");

  uint32_t streamIdCounter = 0;

  // Iterate over all audio pins that belong to the pipeline.
  for (IasAudioPinMap::iterator mapIt = mAudioPinMap.begin(); mapIt != mAudioPinMap.end(); mapIt++)
  {
    // Find all audio pins that are on the same stream in front of the current audio pin and collect
    // them in the vector connectedPins. Starting from the current audio pin, we recursively follow
    // the path via the sourcePin of the current audio pin. The path ends, if we reach an audio pin
    // whose direction is not ModuleInput, ModuleInputOutput, or PipelineOutput. Additionally, the
    // path ends if we reach an audio pin whose sourcePin is linked with a delay element, because
    // a delay element requires that we have separate streams at its input and output.
    std::vector<IasAudioPinPtr>  connectedPins;
    IasAudioPinPtr                     currentPin = mapIt->first;
    IasAudioPinConnectionParamsPtr       pinConnectionParams = mapIt->second;
    connectedPins.push_back(currentPin);
    while (((currentPin->getDirection() == IasAudioPin::eIasPinDirectionModuleInputOutput) ||
            (currentPin->getDirection() == IasAudioPin::eIasPinDirectionModuleInput) ||
            (currentPin->getDirection() == IasAudioPin::eIasPinDirectionPipelineOutput)) &&
           (pinConnectionParams->sourcePin != nullptr) &&
           (!pinConnectionParams->isInputDataDelayed))
    {
      currentPin = pinConnectionParams->sourcePin;
      pinConnectionParams = mAudioPinMap[currentPin];
      connectedPins.push_back(currentPin);
    }

    // Iterate over all audio pins that are collected in the vector connectedPins
    // and check whether any of the pins is already assigned to an audio stream
    // (this means that its audioStreamId != cAudioStreamIdUndefined).
    uint32_t audioStreamId = cAudioStreamIdUndefined;
    for (const IasAudioPinPtr audioPin :connectedPins)
    {
      pinConnectionParams = mAudioPinMap[audioPin];
      if (pinConnectionParams->audioStreamId != cAudioStreamIdUndefined)
      {
        audioStreamId = pinConnectionParams->audioStreamId;
      }
    }

    // If none of the audio pins is already assigned to an audio stream,
    // create a new audio stream (initialize it later).
    if (audioStreamId == cAudioStreamIdUndefined)
    {
      audioStreamId = streamIdCounter;
      streamIdCounter++;
      IasAudioStreamConnectionParams audioStreamConnectionParams;
      audioStreamConnectionParams.audioStream = nullptr;
      audioStreamConnectionParams.numChannels = currentPin->getParameters()->numChannels;
      mAudioStreams.push_back(audioStreamConnectionParams);
    }

    // Assign this audio stream to all pins that are collected in the vector connectedPins.
    for (const IasAudioPinPtr audioPin :connectedPins)
    {
      pinConnectionParams = mAudioPinMap[audioPin];
      pinConnectionParams->audioStreamId = audioStreamId;

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "audioPin:", audioPin->getParameters()->name,
                  "streamId:", pinConnectionParams->audioStreamId);
    }

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "--------------------------------------------");
  }


  // Create all audio streams (and add them to the Audio Chain).
  for (uint32_t audioStreamId = 0; audioStreamId < mAudioStreams.size(); audioStreamId++)
  {
    std::stringstream streamName;
    streamName << mParams->name << "_stream_" << std::setfill ('0') << std::setw (2) << audioStreamId;

    IasAudioStreamConnectionParams& streamConnectionParams = mAudioStreams[audioStreamId];
    IasAudioStream* audioStream = nullptr;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE, "Creating audio stream for streamId", audioStreamId);
    audioStream = mAudioChain->createInputAudioStream(streamName.str(),
                                                      static_cast<int32_t>(audioStreamId),
                                                      streamConnectionParams.numChannels,
                                                      false);
    streamConnectionParams.audioStream = audioStream;
  }

  // Iterate over all modules, create the associated GenericAudioComponentConfigurations,
  // and add all audio streams and audio stream mappings to the configurations.
  // Finally, create the GenericAudioComponents based on the GenericAudioComponentConfigurations.
  for (IasProcessingModulePtr module :mProcessingModuleSchedulingList)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
                "Creating actual audio component for processing module", module->getParameters()->instanceName);

    IasAudioProcessingResult res;
    IasIGenericAudioCompConfig *config = nullptr;
    res = mPluginEngine->createModuleConfig(&config);
    IAS_ASSERT(res == eIasAudioProcOK);

    // Iterate over all input/output pins of this module, identify the assoicated streams, and add them to the configuration.
    IasAudioPinSetPtr audioInputOutputPins = module->getAudioInputOutputPinSet();
    for (const IasAudioPinPtr currentPin :(*audioInputOutputPins))
    {
      IasAudioPinConnectionParamsPtr pinConnectionParams = mAudioPinMap[currentPin];
      uint32_t audioStreamId = pinConnectionParams->audioStreamId;
      IasAudioStreamConnectionParams& streamConnectionParams = mAudioStreams[audioStreamId];
      IasAudioStream* audioStream = streamConnectionParams.audioStream;

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
                  "Input/output pin", currentPin->getParameters()->name,
                  "is implemented by streamId", audioStreamId);
      config->addStreamToProcess(audioStream, currentPin->getParameters()->name);
    }

    // Iterate over all pin mappings of this module, identify the assoicated stream mappings, and add them to the configuration.
    IasAudioPinMappingSetPtr audioPinMappings = module->getAudioPinMappingSet();
    for (const IasAudioPinMapping pinMapping :(*audioPinMappings))
    {
      IasAudioPinPtr inputPin  = pinMapping.inputPin;
      IasAudioPinPtr outputPin = pinMapping.outputPin;

      IasAudioPinConnectionParamsPtr inputPinConnectionParams  = mAudioPinMap[inputPin];
      IasAudioPinConnectionParamsPtr outputPinConnectionParams = mAudioPinMap[outputPin];

      uint32_t inputStreamId  = inputPinConnectionParams->audioStreamId;
      uint32_t outputStreamId = outputPinConnectionParams->audioStreamId;

      IasAudioStreamConnectionParams& inputStreamConnectionParams  = mAudioStreams[inputStreamId];
      IasAudioStreamConnectionParams& outputStreamConnectionParams = mAudioStreams[outputStreamId];

      IasAudioStream* inputStream  = inputStreamConnectionParams.audioStream;
      IasAudioStream* outputStream = outputStreamConnectionParams.audioStream;

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIPELINE,
                  "Pin mapping from", inputPin->getParameters()->name,
                  "to",              outputPin->getParameters()->name,
                  "is implemented by stream mapping", inputStreamId, "->", outputStreamId);
      config->addStreamMapping(inputStream, inputPin->getParameters()->name, outputStream, outputPin->getParameters()->name);
    }

    // Finally, create the GenericAudioComponent based on the configuration and add it to the audio chain.
    // But don't forget to fetch the custom properties of the module from the configuration and set them in the module config
    // The properties for the audio module have to be set before calling this method, else they are empty and the audio module
    // can't be initialized properly
    IasPropertiesPtr properties = mConfiguration->getPropertiesForModule(module);
    config->setProperties(*properties);
    IasGenericAudioComp *audioComponent = nullptr;
    res = mPluginEngine->createModule(config, module->getParameters()->typeName, module->getParameters()->instanceName, &audioComponent);
    if (res != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE, "Error while calling IasPluginEngine::createModule");
      return eIasFailed;
    }

    IAS_ASSERT(audioComponent != nullptr);

    module->setGenericAudioComponent(audioComponent);
    mAudioChain->addAudioComponent(audioComponent);
  }

  return eIasOk;
}


/*
 * @brief Private method: addAudioPinToMap
 */
IasPipeline::IasResult IasPipeline::addAudioPinToMap(IasAudioPinPtr         audioPin,
                                                     IasProcessingModulePtr module)
{
  // Check whether the audioPin has already been added to the pipeline.
  IasAudioPinMap::const_iterator mapIt = mAudioPinMap.find(audioPin);
  if (mapIt != mAudioPinMap.end())
  {
    const IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
    if (pinConnectionParams->processingModule != module)
    {
      // Return with error code, if it has already been added with different connection parameters.
      return eIasFailed;
    }
    return eIasOk;
  }

  IasAudioPinConnectionParamsPtr pinConnectionParams = std::make_shared<IasAudioPinConnectionParams>();
  pinConnectionParams->thisPin = audioPin;
  pinConnectionParams->processingModule = module;
  IasAudioPinPair tmp = std::make_pair(audioPin, pinConnectionParams);
  mAudioPinMap.insert(tmp);

  return eIasOk;
}


/**
 * @brief Private method: createAudioChannelBuffers
 *
 * Create the audioChannelBuffers an audio pin and add them to the pinConnectionParams.
 */
void IasPipeline::createAudioChannelBuffers(IasAudioPinConnectionParamsPtr pinConnectionParams,
                                            uint32_t numChannels,
                                            uint32_t periodSize)
{
  // Check whether the audioChannelBuffers already exist
  if (pinConnectionParams->audioChannelBuffers.size() == numChannels)
  {
    return;
  }

  eraseAudioChannelBuffers(pinConnectionParams);
  for (uint32_t channel = 0; channel < numChannels; channel++)
  {
    float* channelBuffer = new float[periodSize];
    pinConnectionParams->audioChannelBuffers.push_back(channelBuffer);
  }
}


/**
 * @brief Private method: eraseAudioChannelBuffers
 *
 * Erase the audio channel buffers that belong to the audio pin that is specified by the given pinConnectionParams.
 *
 * @param[in] pinConnectionParams  Pin connection parameters of the audio pin whose audio channel buffers shall be erased.
 */
void IasPipeline::eraseAudioChannelBuffers(IasAudioPinConnectionParamsPtr pinConnectionParams)
{
  IAS_ASSERT(pinConnectionParams != nullptr);
  IAS_ASSERT(pinConnectionParams->thisPin != nullptr);
  IAS_ASSERT(pinConnectionParams->thisPin->getParameters() != nullptr);

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_PIPELINE,
              "Erasing audioChannelBuffers for audio pin", pinConnectionParams->thisPin->getParameters()->name);

  std::vector<float*>& audioChannelBuffers = pinConnectionParams->audioChannelBuffers;
  for (float* channelBuffer :audioChannelBuffers)
  {
    delete[] channelBuffer;
  }
  pinConnectionParams->audioChannelBuffers.clear();
}


/**
 * @brief Private method: eraseAudioChannelBuffers
 *
 * Erase the audio channel buffers that belong to the specified audio pin.
 *
 * @param[in] audioPin  Audio pin whose audio channel buffers shall be erased.
 */
void IasPipeline::eraseAudioChannelBuffers(IasAudioPinPtr audioPin)
{
  IasAudioPinMap::iterator mapIt = mAudioPinMap.find(audioPin);
  if (mapIt != mAudioPinMap.end())
  {
    IasAudioPinConnectionParamsPtr pinConnectionParams = mapIt->second;
    eraseAudioChannelBuffers(pinConnectionParams);
  }
}


const IasPipeline::IasAudioPinConnectionParamsPtr IasPipeline::getPinConnectionParams(IasAudioPinPtr pin) const
{
  IasAudioPinMap::const_iterator mapIt = mAudioPinMap.find(pin);
  if (mapIt != mAudioPinMap.end())
  {
    return mapIt->second;
  }
  else
  {
    return nullptr;
  }
}

void IasPipeline::stopPinProbing(IasAudioPinPtr pin)
{
  IasResult res = eIasOk;
  IasGenericAudioCompCore* core = nullptr;
  IasAudioPinConnectionParamsPtr params = getPinConnectionParams(pin);
  if (params != nullptr)
  {
    IasProcessingModulePtr module = params->processingModule;
    if(module != nullptr)
    {
      IasGenericAudioComp* genericComp = module->getGenericAudioComponent();
      if(genericComp != nullptr)
      {
        core = genericComp->getCore();
        if(core != nullptr)
        {
          IasAudioPin::IasPinDirection pinDir = pin->getDirection();
          bool doInputProbing = false;
          bool doOutputProbing = false;
          switch(pinDir)
          {
            case IasAudioPin::eIasPinDirectionModuleInputOutput:
              doInputProbing = true;
              doOutputProbing = true;
              break;
            case IasAudioPin::eIasPinDirectionModuleInput:
              doInputProbing = true;
              doOutputProbing = false;
              break;
            case IasAudioPin::eIasPinDirectionModuleOutput:
              doInputProbing = false;
              doOutputProbing = true;
              break;
            default:
              res = eIasFailed;
              DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"Pin direction",toString(pinDir)," not ok for probing, only pins attached to modules can be probed");
              break;
          }
          if(res == eIasOk)
          {
            core->stopProbe(params->audioStreamId,doInputProbing,doOutputProbing);
          }
        }
      }
    }
  }

  return;
}

IasPipeline::IasResult IasPipeline::startPinProbing(IasAudioPinPtr pin,
                                                    std::string fileName,
                                                    uint32_t numSeconds,
                                                    bool inject)
{
  IasResult res = eIasOk;

  IasAudioPinConnectionParamsPtr params = getPinConnectionParams(pin);
  if (params == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"could not get pin connection params");
    return eIasFailed;
  }
  IasProcessingModulePtr module = params->processingModule;
  if(module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"got module nullptr");
    return eIasFailed;
  }
  IasGenericAudioComp* genericComp = module->getGenericAudioComponent();
  if(genericComp == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"got generic component nullptr");
    return eIasFailed;
  }
  IasGenericAudioCompCore* core = genericComp->getCore();
  if(core == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"got comp core nullptr");
    return eIasFailed;
  }

  IasAudioPin::IasPinDirection pinDir = pin->getDirection();
  bool doInputProbing = false;
  bool doOutputProbing = false;

  switch(pinDir)
  {
    case IasAudioPin::eIasPinDirectionModuleInputOutput:
      doInputProbing = true;
      doOutputProbing = true;
      break;
    case IasAudioPin::eIasPinDirectionModuleInput:
      doInputProbing = true;
      doOutputProbing = false;
      break;
    case IasAudioPin::eIasPinDirectionModuleOutput:
      doInputProbing = false;
      doOutputProbing = true;
      break;
    default:
      res = eIasFailed;
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIPELINE,"Pin direction",toString(pinDir)," not ok for probing, only pins attached to modules can be probed");
      break;
  }

  if (res != eIasOk)
  {
    return res;
  }
  if(inject == false)
  {
    fileName.append("_");
    fileName.append(pin->getParameters()->name);
  }
  core->startProbe(fileName,inject,numSeconds,params->audioStreamId,doInputProbing,doOutputProbing);

  return res;
}


/*
 * Function to get a IasPipeline::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasPipeline::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasPipeline::eIasOk);
    STRING_RETURN_CASE(IasPipeline::eIasFailed);
    DEFAULT_STRING("Invalid IasPipeline::IasResult => " + std::to_string(type));
  }
}

} // namespace IasAudio
