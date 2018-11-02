/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkSetupImpl.cpp
 * @date   2017
 * @brief  The Setup interface of the test framework.
 */

#include "testfwx/IasTestFrameworkSetupImpl.hpp"
#include "testfwx/IasTestFrameworkConfiguration.hpp"
#include "testfwx/IasTestFrameworkWaveFile.hpp"
#include "testfwx/IasTestFrameworkRoutingZone.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasPipeline.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasProcessingModule.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"

#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"


namespace IasAudio {

static const std::string cClassName = "IasTestFrameworkSetupImpl::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

#define PORT_NAME(audioPin) ("Port_" + (audioPin)->getParameters()->name)

IasTestFrameworkSetup::IasResult translate(IasTestFrameworkConfiguration::IasResult result)
{
  switch (result)
  {
    case IasTestFrameworkConfiguration::eIasOk:
      return IasTestFrameworkSetup::eIasOk;
    default:
      return IasTestFrameworkSetup::eIasFailed;
  }
}


IasTestFrameworkSetup::IasResult translate(IasTestFrameworkRoutingZone::IasResult result)
{
  switch (result)
  {
    case IasTestFrameworkRoutingZone::eIasOk:
      return IasTestFrameworkSetup::eIasOk;
    default:
      return IasTestFrameworkSetup::eIasFailed;
  }
}


IasTestFrameworkSetup::IasResult translate(IasAudioPort::IasResult result)
{
  switch (result)
  {
    case IasAudioPort::eIasOk:
      return IasTestFrameworkSetup::eIasOk;
    default:
      return IasTestFrameworkSetup::eIasFailed;
  }
}


IasTestFrameworkSetup::IasResult translate(IasTestFrameworkWaveFile::IasResult result)
{
  switch (result)
  {
    case IasTestFrameworkWaveFile::eIasOk:
      return IasTestFrameworkSetup::eIasOk;
    default:
      return IasTestFrameworkSetup::eIasFailed;
  }
}


IasTestFrameworkSetupImpl::IasTestFrameworkSetupImpl(IasTestFrameworkConfigurationPtr config)
  :mConfig(config)
  ,mLog(IasAudioLogging::getDltContext("CFG"))
{
  IAS_ASSERT(mConfig != nullptr);
  IAS_ASSERT(mLog != nullptr);
}


IasTestFrameworkSetupImpl::~IasTestFrameworkSetupImpl()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX);
  mConfig.reset();
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::createAudioPin(const IasAudioPinParams &params, IasAudioPinPtr *pin)
{
  if (pin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pin is nullptr");
    return eIasFailed;
  }
  if (params.name.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pin name is not specified");
    return eIasFailed;
  }
  if (params.numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: numChannels is 0");
    return eIasFailed;
  }

  IasAudioPinParamsPtr pinParams = std::make_shared<IasAudioPinParams>();
  IAS_ASSERT(pinParams != nullptr);
  *pinParams = params;
  IasAudioPinPtr newPin = std::make_shared<IasAudioPin>(pinParams);
  IAS_ASSERT(newPin != nullptr);
  *pin = newPin;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created audio pin", params.name,
              "numChannels=", params.numChannels);
  IasResult setres = translate(mConfig->addPin(*pin));
  if (setres != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to add pin", newPin->getParameters()->name.c_str(), "to config");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created audio pin", params.name,
                "numChannels=", params.numChannels);
  }
  return setres;
}


void IasTestFrameworkSetupImpl::destroyAudioPin(IasAudioPinPtr *pin)
{
  if (pin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pin is nullptr");
    return;
  }

  std::string name = (*pin)->getParameters()->name;

  // Get the handle of the pipeline that owns the audio pin and delete the audio pin from the pipeline.
  const IasPipelinePtr  pipeline = (*pin)->getPipeline();
  if (pipeline != nullptr)
  {
    pipeline->deleteAudioPin(*pin);
    (*pin)->deleteFromPipeline();
  }

  pin->reset();
  mConfig->removePin(name);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed audio pin", name);
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::addAudioInputPin(IasAudioPinPtr pipelineInputPin)
{
  if (pipelineInputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineInputPin is nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  IasPipeline::IasResult result = pipeline->addAudioInputPin(pipelineInputPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioInputPin:", toString(result));
    return eIasFailed;
  }

  IasAudioPin::IasResult audioPinResult = pipelineInputPin->addToPipeline(pipeline, pipeline->getParameters()->name);
  if (audioPinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasAudioPin::addToPipeline:", toString(audioPinResult));
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio input pin", pipelineInputPin->getParameters()->name,
              "to pipeline", pipeline->getParameters()->name);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::deleteAudioInputPin(IasAudioPinPtr pipelineInputPin)
{
  if (pipelineInputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineInputPin is nullptr");
    return;
  }

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  pipeline->deleteAudioPin(pipelineInputPin);
  pipelineInputPin->deleteFromPipeline();
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::addAudioOutputPin(IasAudioPinPtr pipelineOutputPin)
{
  if (pipelineOutputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineOutputPin is nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  IasPipeline::IasResult result = pipeline->addAudioOutputPin(pipelineOutputPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioOutputPin:", toString(result));
    return eIasFailed;
  }

  IasAudioPin::IasResult audioPinResult = pipelineOutputPin->addToPipeline(pipeline, pipeline->getParameters()->name);
  if (audioPinResult != IasAudioPin::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasAudioPin::addToPipeline:", toString(audioPinResult));
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio output pin", pipelineOutputPin->getParameters()->name,
              "to pipeline", pipeline->getParameters()->name);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::deleteAudioOutputPin(IasAudioPinPtr pipelineOutputPin)
{
  if (pipelineOutputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelineOutputPin is nullptr");
    return;
  }

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  pipeline->deleteAudioPin(pipelineOutputPin);
  pipelineOutputPin->deleteFromPipeline();
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return eIasFailed;
  }
  if (inOutPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inOutPin is nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: module", module->getParameters()->instanceName,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->addAudioInOutPin(module, inOutPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioInOutPin:", toString(result));
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio input/output pin", inOutPin->getParameters()->name,
                "to module", module->getParameters()->instanceName);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return;
  }
  if (inOutPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inOutPin is nullptr");
    return;
  }

  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: module", module->getParameters()->instanceName,
                "has not been added to a pipeline yet.");
    return;
  }

  pipeline->deleteAudioInOutPin(module, inOutPin);
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return eIasFailed;
  }
  if (inputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPin is nullptr");
    return eIasFailed;
  }
  if (outputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputPin is nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: module", module->getParameters()->instanceName,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->addAudioPinMapping(module, inputPin, outputPin);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addAudioPinMapping:", toString(result));
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added pin mapping of audio input pin", inputPin->getParameters()->name,
              "to audio output pin", outputPin->getParameters()->name, "to module", module->getParameters()->instanceName);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return;
  }
  if (inputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPin is nullptr");
    return;
  }
  if (outputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputPin is nullptr");
    return;
  }

  IasPipelinePtr pipeline = module->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: module", module->getParameters()->instanceName,
                "has not been added to a pipeline yet.");
    return;
  }

  pipeline->deleteAudioPinMapping(module, inputPin, outputPin);
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::createProcessingModule(const IasProcessingModuleParams &params, IasProcessingModulePtr *module)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return eIasFailed;
  }
  if (params.typeName.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: typeName is not specified");
    return eIasFailed;
  }
  if (params.instanceName.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: instanceName is not specified");
    return eIasFailed;
  }

  IasProcessingModuleParamsPtr moduleParams = std::make_shared<IasProcessingModuleParams>();
  IAS_ASSERT(moduleParams != nullptr);
  *moduleParams = params;
  IasProcessingModulePtr newModule = std::make_shared<IasProcessingModule>(moduleParams);
  IAS_ASSERT(newModule != nullptr);
  *module = newModule;

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created processing module", params.instanceName,
              "of type", params.typeName);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::destroyProcessingModule(IasProcessingModulePtr *module)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return;
  }

  // Get the handle of the pipeline that owns the module and delete the module from the pipeline.
  const IasPipelinePtr pipeline = (*module)->getPipeline();
  if (pipeline != nullptr)
  {
    pipeline->deleteProcessingModule(*module);
    (*module)->deleteFromPipeline();
  }

  module->reset();
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::addProcessingModule(IasProcessingModulePtr module)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return eIasFailed;
  }

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  IasPipeline::IasResult pipelineResult = pipeline->addProcessingModule(module);
  if (pipelineResult != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::addProcessingModule:", toString(pipelineResult));
    return eIasFailed;
  }

  IasProcessingModule::IasResult result = module->addToPipeline(pipeline, pipeline->getParameters()->name);
  if (result != IasProcessingModule::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while adding module to pipeline:", toString(result));
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added processing module", module->getParameters()->instanceName,
              "to pipeline", pipeline->getParameters()->name);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::deleteProcessingModule(IasProcessingModulePtr module)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return;
  }

  IasPipelinePtr pipeline = mConfig->getTestFrameworkPipeline();
  IAS_ASSERT(pipeline != nullptr);

  pipeline->deleteProcessingModule(module);
  module->deleteFromPipeline();
}


void IasTestFrameworkSetupImpl::setProperties(IasProcessingModulePtr module, const IasProperties &properties)
{
  if (module == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: module is nullptr");
    return;
  }

  // Add the properties for this module to the SmartX configuration. Later, when IasPipeline::initAudioStreams
  // is executed, the module properties are received from the SmartX configuration in order to create the
  // module accordingly.
  mConfig->setPropertiesForModule(module, properties);
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::linkWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFileParams &waveFileParams)
{
  static int32_t portid = 0;

  if (audioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPin is nullptr");
    return eIasFailed;
  }
  if (waveFileParams.fileName.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: wave file name is not specified");
    return eIasFailed;
  }

  // Create wave file instance
  IasTestFrameworkWaveFileParamsPtr waveFileParamsPtr = std::make_shared<IasTestFrameworkWaveFileParams>();
  IAS_ASSERT(waveFileParamsPtr != nullptr);
  *waveFileParamsPtr = waveFileParams;

  IasTestFrameworkWaveFilePtr waveFile = std::make_shared<IasTestFrameworkWaveFile>(waveFileParamsPtr);
  IAS_ASSERT(waveFile != nullptr);

  IasResult result = translate(mConfig->addWaveFile(audioPin, waveFile));
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to add wave file to mConfig", waveFileParams.fileName);
    return eIasFailed;
  }

  // Link audioPin to wave file
  result = translate(waveFile->linkAudioPin(audioPin));
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to link audio pin with wave file");
    mConfig->deleteWaveFile(audioPin);
    return eIasFailed;
  }

  // Create audio port and link it with audio pin
  IasAudioPortPtr audioPort = nullptr;
  IasAudioPortParams audioPortParams;
  audioPortParams.name = PORT_NAME(audioPin);
  audioPortParams.numChannels = audioPin->getParameters()->numChannels;
  audioPortParams.id = portid++;
  audioPortParams.direction = eIasPortDirectionInput;
  audioPortParams.index = 0;

  result = createAudioPort(audioPortParams, &audioPort);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during createAudioPort()", toString(result));
    return eIasFailed;
  }

  result = link(audioPort, waveFile);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to link audio port and wave file");
    return eIasFailed;
  }

  result = link(audioPort, audioPin);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to link audioPort and audioPin");
    return eIasFailed;
  }

  return eIasOk;
}


void IasTestFrameworkSetupImpl::unlinkWaveFile(IasAudioPinPtr audioPin)
{
  if (audioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPin is nullptr");
    return;
  }

  IasAudioPortPtr audioPort = nullptr;
  IasResult result = translate(mConfig->getPortByName(PORT_NAME(audioPin), &audioPort));
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Audio port not found for audio pin");
    return;
  }

  unlink(audioPort, audioPin);

  IasTestFrameworkWaveFilePtr waveFile = nullptr;
  result = translate(mConfig->getWaveFile(audioPin, &waveFile));
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Wave file not found for audio pin");
    return;
  }

  unlink(audioPort, waveFile);
  destroyAudioPort(audioPort);

  mConfig->deleteWaveFile(audioPin);
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType)
{
  if (outputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputPin is nullptr");
    return eIasFailed;
  }
  if (inputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPin is nullptr");
    return eIasFailed;
  }

  //input and output pin belong to the same pipeline, here we only check if they are already added
  IasPipelinePtr pipeline = outputPin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: output pin", outputPin->getParameters()->name,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }
  pipeline = inputPin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: input pin", inputPin->getParameters()->name,
                "has not been added to a pipeline yet.");
    return eIasFailed;
  }

  IasPipeline::IasResult result = pipeline->link(outputPin, inputPin, linkType);
  if (result != IasPipeline::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during IasPipeline::link()", toString(result));
    return eIasFailed;
  }

  return eIasOk;
}


void IasTestFrameworkSetupImpl::unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin)
{
  if (outputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputPin is nullptr");
    return;
  }
  if (inputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPin is nullptr");
    return;
  }

  //input and output pin belong to the same pipeline, here we only check if they are already added
  IasPipelinePtr pipeline = outputPin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: output pin", outputPin->getParameters()->name,
                "has not been added to a pipeline yet.");
    return;
  }
  pipeline = inputPin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Invalid configuration: input pin", inputPin->getParameters()->name,
                "has not been added to a pipeline yet.");
    return;
  }

  pipeline->unlink(outputPin, inputPin);
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::createAudioPort(const IasAudioPortParams& params, IasAudioPortPtr* audioPort)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return eIasFailed;
  }
  if (params.numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: numChannels is 0");
    return eIasFailed;
  }
  if (params.direction == eIasPortDirectionUndef)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: direction is undefined");
    return eIasFailed;
  }
  if (params.index == 0xFFFFFFFF)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: index is", params.index);
    return eIasFailed;
  }

  IasAudioPortParamsPtr portParams = std::make_shared<IasAudioPortParams>(params);
  IAS_ASSERT(portParams != nullptr);
  IasAudioPortPtr newPort = std::make_shared<IasAudioPort>(portParams);
  IAS_ASSERT(newPort != nullptr);
  *audioPort = newPort;

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully created new audio port.",
              "Name=", params.name,
              "Id=", params.id,
              "NumChannels=", params.numChannels,
              "Index", params.index,
              "Direction=", toString(params.direction));

  return eIasOk;
}


void IasTestFrameworkSetupImpl::destroyAudioPort(IasAudioPortPtr audioPort)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return;
  }

  // Get the parameters before destroying port
  const IasAudioPortParamsPtr params = audioPort->getParameters();

  audioPort->clearOwner();
  audioPort.reset();

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully destroyed audio port.",
              "Name=", params->name,
              "Id=", params->id,
              "NumChannels=", params->numChannels,
              "Index", params->index,
              "Direction=", toString(params->direction));
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::link(IasAudioPortPtr audioPort, IasTestFrameworkWaveFilePtr waveFile)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return eIasFailed;
  }
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Invalid parameter: waveFile is nullptr");
    return eIasFailed;
  }

  IasPortDirection audioPortDirection = audioPort->getParameters()->direction;
  if (audioPortDirection != eIasPortDirectionInput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Audio port has wrong direction");
    return eIasFailed;
  }

  IasTestFrameworkRoutingZonePtr routingZone = mConfig->getTestFrameworkRoutingZone();
  IAS_ASSERT(routingZone != nullptr);

  IasAudioRingBuffer *ringBuffer = nullptr;
  
  if (waveFile->isInputFile())
  {
    IasAudioPort::IasResult portres = audioPort->setOwner(waveFile->getDummySourceDevice());
    (void)portres;
    IAS_ASSERT(portres == IasAudioPort::eIasOk);

    IasResult res = translate(routingZone->createInputBuffer(audioPort, IasAudioCommonDataFormat::eIasFormatFloat32, waveFile, &ringBuffer));
    if (res != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to create input buffer");
      return eIasFailed;
    }
  }
  else
  {
    IasAudioPort::IasResult portres = audioPort->setOwner(waveFile->getDummySinkDevice());
    (void)portres;
    IAS_ASSERT(portres == IasAudioPort::eIasOk);

    IasResult res = translate(routingZone->createOutputBuffer(audioPort, IasAudioCommonDataFormat::eIasFormatFloat32, waveFile, &ringBuffer));
    if (res != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to create output buffer");
      return eIasFailed;
    }
  }

  // ringBuffer is always != nullptr after createInputBuffer/createOutputBuffer successfully returned
  IasAudioPort::IasResult portres = audioPort->setRingBuffer(ringBuffer);
  (void)portres;
  IAS_ASSERT(portres == IasAudioPort::eIasOk);

  IasResult res = translate(mConfig->addPort(audioPort));
  if (res == IasResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully added audio port", audioPort->getParameters()->name,
                "Id=", audioPort->getParameters()->id,
                "NumChannels=", audioPort->getParameters()->numChannels,
                "Index=", audioPort->getParameters()->index);
  }

  return res;
}


void IasTestFrameworkSetupImpl::unlink(IasAudioPortPtr audioPort, IasTestFrameworkWaveFilePtr waveFile)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return;
  }
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Invalid parameter: waveFile is nullptr");
    return;
  }
  if (audioPort->getParameters()->id >= 0)
  {
    mConfig->deleteInputPort(audioPort->getParameters()->id);
  }

  mConfig->deletePortByName(audioPort->getParameters()->name);

  IasAudioPortOwnerPtr owner;
  IasResult res = translate(audioPort->getOwner(&owner));
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "audio port has no owner to delete");
  }
  else
  {
    audioPort->clearOwner();
  }

  IasTestFrameworkRoutingZonePtr routingZone = mConfig->getTestFrameworkRoutingZone();
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Invalid configuration: routingZone is nullptr");
    return;
  }

  if (waveFile->isInputFile())
  {
    res = translate(routingZone->destroyInputBuffer(audioPort));
    if (res != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Error during IasTestFrameworkRoutingZone::destroyInputBuffer:", toString(res));
      return;
    }
  }
  else
  {
    res = translate(routingZone->destroyOutputBuffer(audioPort));
    if (res != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Error during IasTestFrameworkRoutingZone::destroyOutputBuffer:", toString(res));
      return;
    }
  }
}


IasTestFrameworkSetupImpl::IasResult IasTestFrameworkSetupImpl::link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  if (inputPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPort is nullptr");
    return eIasFailed;
  }
  if (pipelinePin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelinePin is nullptr");
    return eIasFailed;
  }

  IasAudioPortParamsPtr inputPortParams   = inputPort->getParameters();
  IAS_ASSERT(inputPortParams != nullptr);
  const std::string& inputPortName = inputPortParams->name;
  IasPortDirection inputPortDirection = inputPortParams->direction;

  IasAudioPinParamsPtr pipelinePinParams = pipelinePin->getParameters();
  IAS_ASSERT(pipelinePinParams != nullptr);
  const std::string& pipelinePinName = pipelinePinParams->name;

  // Verify that inputPort has been configured as input port.
  if (inputPortDirection != eIasPortDirectionInput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid port configuration,", inputPortName,
                "has invalid direction:", toString(inputPortDirection),
                "(should be eIasPortDirectionInput)");
    return eIasFailed;
  }

  // Verify that the number of channels of the audioPort and of the pipelinePin are equal.
  if (inputPortParams->numChannels != pipelinePinParams->numChannels)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Cannot link inputPort", inputPortName,
                "with pipeline pin", pipelinePinName,
                "since channel numbers do not match");
    return eIasFailed;
  }

  // Verify that the audioPin has been added to a pipeline.
  IasPipelinePtr pipeline = pipelinePin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Associated pipeline of pipelinePin", pipelinePinName,
                "cannot be determined");
    return eIasFailed;
  }

  // Call the link method of the pipeline that belongs to the pipelinePin.
  IasPipeline::IasResult pipelineResult = pipeline->link(inputPort, pipelinePin);
  if (pipelineResult != IasPipeline::eIasOk)
  {
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully linked input port", inputPortName,
                "to pipeline pin", pipelinePinName);

  return eIasOk;
}


void IasTestFrameworkSetupImpl::unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin)
{
  if (inputPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputPort is nullptr");
    return;
  }
  if (pipelinePin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipelinePin is nullptr");
    return;
  }

  IasAudioPinParamsPtr  pipelinePinParams = pipelinePin->getParameters();
  IAS_ASSERT(pipelinePinParams != nullptr);
  const std::string& pinName = pipelinePinParams->name;

  IasPipelinePtr pipeline = pipelinePin->getPipeline();
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Associated pipeline of pipelinePin", pinName,
                "cannot be determined");
    return;
  }

  pipeline->unlink(inputPort, pipelinePin);
}


#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasTestFrameworkSetup::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasTestFrameworkSetup::eIasOk);
    STRING_RETURN_CASE(IasTestFrameworkSetup::eIasFailed);
    DEFAULT_STRING("Invalid IasTestFrameworkSetup::IasResult => " + std::to_string(type));
  }
}

} // namespace IasAudio
