/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file   IasMyComplexPipeline.cpp
 * @date   2016
 * @brief  Class for setting up an example pipeline "myComplexPipeline".
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>



#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"

#include "model/IasPipeline.hpp"
#include "IasMyComplexPipeline.hpp"


namespace IasAudio {


IasMyComplexPipeline::IasMyComplexPipeline(IasISetup*        setup,
                                           uint32_t       sampleRate,
                                           uint32_t       periodSize,
                                           IasAudioPortPtr   routingZoneInputPort0,
                                           IasAudioPortPtr   routingZoneInputPort1,
                                           IasAudioPortPtr   sinkDeviceInputPort0,
                                           IasAudioPortPtr   sinkDeviceInputPort1)
  :mSetup(setup)
  ,mSampleRate(sampleRate)
  ,mPeriodSize(periodSize)
  ,mPipeline(nullptr)
  ,mRoutingZoneInputPort0(routingZoneInputPort0)
  ,mRoutingZoneInputPort1(routingZoneInputPort1)
  ,mSinkDeviceInputPort0(sinkDeviceInputPort0)
  ,mSinkDeviceInputPort1(sinkDeviceInputPort1)

{
  IAS_ASSERT(setup != nullptr);
  IAS_ASSERT(routingZoneInputPort0 != nullptr);
  IAS_ASSERT(routingZoneInputPort1 != nullptr);
  IAS_ASSERT(sinkDeviceInputPort0  != nullptr);
  IAS_ASSERT(sinkDeviceInputPort1  != nullptr);

  IasISetup::IasResult result;

  // Create the pipeline
  IasPipelineParams pipelineParams =
  {
    /*.name =*/ "myPipeline",
    /*.samplerate =*/ mSampleRate,
    /*.periodSize =*/ mPeriodSize
  };
  result = setup->createPipeline(pipelineParams, &mPipeline);
  IAS_ASSERT(IasISetup::eIasOk == result);
  IAS_ASSERT(mPipeline != nullptr);
}


IasMyComplexPipeline::~IasMyComplexPipeline()
{
  // Delete input pins and output pins from pipeline and destroy them
  for (uint32_t cntPins = 0; cntPins < mInputPins.size(); cntPins++)
  {
    mSetup->deleteAudioInputPin(mPipeline, mInputPins[cntPins]);
    mSetup->destroyAudioPin(&mInputPins[cntPins]);
  }
  for (uint32_t cntPins = 0; cntPins < mOutputPins.size(); cntPins++)
  {
    mSetup->deleteAudioOutputPin(mPipeline, mOutputPins[cntPins]);
    mSetup->destroyAudioPin(&mOutputPins[cntPins]);
  }

  mSetup->deleteAudioPinMapping(mProcessingModules[4], mModulePins[5], mModulePins[7]);
  mSetup->deleteAudioPinMapping(mProcessingModules[4], mModulePins[6], mModulePins[7]);
  mSetup->deleteAudioPinMapping(mProcessingModules[4], mModulePins[6], mModulePins[8]);

  mSetup->deleteAudioPinMapping(mProcessingModules[5], mModulePins[9], mModulePins[11]);
  mSetup->deleteAudioPinMapping(mProcessingModules[5], mModulePins[9], mModulePins[12]);
  mSetup->deleteAudioPinMapping(mProcessingModules[5], mModulePins[10], mModulePins[11]);
  mSetup->deleteAudioPinMapping(mProcessingModules[5], mModulePins[10], mModulePins[12]);

  mSetup->destroyPipeline(&mPipeline);
  IAS_ASSERT(mPipeline == nullptr);
}


void IasMyComplexPipeline::init()
{
  // Get the smartx setup
  IasISetup* setup = mSetup;
  IAS_ASSERT(setup != nullptr);

  IasISetup::IasResult result;

  // Create processing modules
  for (uint32_t cntModules = 0; cntModules < 6; cntModules++)
  {
    IasProcessingModuleParams processingModuleParams =
    {
      /*.typeName     =*/ "simplevolume",
      /*.instanceName =*/ "simplevolume " + std::to_string(cntModules)
    };

    IasProcessingModulePtr processingModule = nullptr;
    result = setup->createProcessingModule(processingModuleParams, &processingModule);
    IAS_ASSERT(IasISetup::eIasOk == result);
    IAS_ASSERT(processingModule != nullptr);
    mProcessingModules.push_back(processingModule);
  }

  // Create input pins
  IasAudioPinParams pinParams;
  for (uint32_t cntPins = 0; cntPins < 2; cntPins++)
  {
    pinParams =
    {
      /*.name =*/ "input pin " + std::to_string(cntPins),
      /*.numChannels =*/ 1,
    };
    IasAudioPinPtr inputPin = nullptr;
    result = setup->createAudioPin(pinParams, &inputPin);
    IAS_ASSERT(IasISetup::eIasOk == result);
    IAS_ASSERT(inputPin != nullptr);
    mInputPins.push_back(inputPin);
  }

  // Create output pins
  for (uint32_t cntPins = 0; cntPins < 2; cntPins++)
  {
    pinParams =
    {
      /*.name =*/ "output pin " + std::to_string(cntPins),
      /*.numChannels =*/ 1,
    };
    IasAudioPinPtr outputPin = nullptr;
    result = setup->createAudioPin(pinParams, &outputPin);
    IAS_ASSERT(IasISetup::eIasOk == result);
    IAS_ASSERT(outputPin != nullptr);
    mOutputPins.push_back(outputPin);
  }

  // Create module pins
  for (uint32_t cntPins = 0; cntPins < 13; cntPins++)
  {
    pinParams =
    {
      /*.name =*/ "module pin " + std::to_string(cntPins),
      /*.numChannels =*/ 1,
    };
    IasAudioPinPtr inputOutputPin = nullptr;
    result = setup->createAudioPin(pinParams, &inputOutputPin);
    IAS_ASSERT(IasISetup::eIasOk == result);
    IAS_ASSERT(inputOutputPin != nullptr);
    mModulePins.push_back(inputOutputPin);
  }

  // Add all processing modules to the pipeline
  for (uint32_t cntModules = 0; cntModules < mProcessingModules.size(); cntModules++)
  {
    result = setup->addProcessingModule(mPipeline, mProcessingModules[cntModules]);
    IAS_ASSERT(IasISetup::eIasOk == result);
  }

  // Add pins to pipeline
  result = setup->addAudioInputPin(mPipeline, mInputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioInputPin(mPipeline, mInputPins[1]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioOutputPin(mPipeline, mOutputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioOutputPin(mPipeline, mOutputPins[1]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Add pins to processing modules.
  result = setup->addAudioInOutPin(mProcessingModules[0], mModulePins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioInOutPin(mProcessingModules[1], mModulePins[1]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioInOutPin(mProcessingModules[2], mModulePins[2]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioInOutPin(mProcessingModules[3], mModulePins[3]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioInOutPin(mProcessingModules[3], mModulePins[4]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[4], mModulePins[5], mModulePins[7]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[4], mModulePins[6], mModulePins[7]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[4], mModulePins[6], mModulePins[8]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[5], mModulePins[9], mModulePins[11]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[5], mModulePins[10], mModulePins[11]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[5], mModulePins[9], mModulePins[12]);
  IAS_ASSERT(IasISetup::eIasOk == result);
  result = setup->addAudioPinMapping(mProcessingModules[5], mModulePins[10], mModulePins[12]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Create the links between the pins.
  result = setup->link(mInputPins[0], mModulePins[0], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mInputPins[1], mModulePins[9], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[0], mModulePins[1], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[1], mModulePins[5], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[4], mModulePins[2], eIasAudioPinLinkTypeDelayed);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[2], mModulePins[10], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[11], mModulePins[6], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[12], mModulePins[4], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[8], mModulePins[3], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[7], mOutputPins[0], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[3], mOutputPins[1], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Setup the pipeline's audio chain of processing modules.
  result = setup->initPipelineAudioChain(mPipeline);
  IAS_ASSERT(IasISetup::eIasOk == result);

  mPipeline->dumpConnectionParameters();
  mPipeline->dumpProcessingSequence();


  // Link the routing zone input ports to the pipeline input pins.
  result = setup->link(mRoutingZoneInputPort0, mInputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mRoutingZoneInputPort1, mInputPins[1]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Link the pipeline output pins to the sink device input ports.
  result = setup->link(mSinkDeviceInputPort0, mOutputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mSinkDeviceInputPort1, mOutputPins[1]);
  IAS_ASSERT(IasISetup::eIasOk == result);
}

void IasMyComplexPipeline::unlinkPorts()
{
  // Get the smartx setup
  IasISetup* setup = mSetup;
  IAS_ASSERT(setup != nullptr);

  // Unlink the routing zone input ports from the pipeline input pins.
  setup->unlink(mRoutingZoneInputPort0, mInputPins[0]);
  setup->unlink(mRoutingZoneInputPort1, mInputPins[1]);

  // Unlink the pipeline output pins from the sink device input ports.
  setup->unlink(mSinkDeviceInputPort0, mOutputPins[0]);
  setup->unlink(mSinkDeviceInputPort1, mOutputPins[1]);
}

}
