/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file   IasMySimplePipeline.cpp
 * @date   2016
 * @brief  Class for setting up an example pipeline "mySimplePipeline".
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>



#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"

#include "model/IasPipeline.hpp"
#include "IasMySimplePipeline.hpp"
#include "audio/smartx/IasProperties.hpp"


namespace IasAudio {


IasMySimplePipeline::IasMySimplePipeline(IasISetup*        setup,
                                         uint32_t       sampleRate,
                                         uint32_t       periodSize,
                                         IasAudioPortPtr   routingZoneInputPort0,
                                         IasAudioPortPtr   sinkDeviceInputPort0)
  :mSetup(setup)
  ,mSampleRate(sampleRate)
  ,mPeriodSize(periodSize)
  ,mPipeline(nullptr)
  ,mRoutingZoneInputPort0(routingZoneInputPort0)
  ,mSinkDeviceInputPort0(sinkDeviceInputPort0)

{
  IAS_ASSERT(setup != nullptr);
  IAS_ASSERT(routingZoneInputPort0 != nullptr);
  IAS_ASSERT(sinkDeviceInputPort0  != nullptr);
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


IasMySimplePipeline::~IasMySimplePipeline()
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

  mSetup->destroyPipeline(&mPipeline);
  IAS_ASSERT(mPipeline == nullptr);
}


void IasMySimplePipeline::init()
{
  // Get the smartx setup
  IasISetup* setup = mSetup;
  IAS_ASSERT(setup != nullptr);

  IasISetup::IasResult result;

  // Create processing modules
  for (uint32_t cntModules = 0; cntModules < 1; cntModules++)
  {
    IasProcessingModuleParams processingModuleParams =
    {
      /*.typeName     =*/ "ias.volume",
      /*.instanceName =*/ "Volume instance " + std::to_string(cntModules)
    };

    IasProcessingModulePtr processingModule = nullptr;
    result = setup->createProcessingModule(processingModuleParams, &processingModule);
    IAS_ASSERT(IasISetup::eIasOk == result);
    IAS_ASSERT(processingModule != nullptr);
    mProcessingModules.push_back(processingModule);

    // Setup the configuration properties of the ias.volume module.
    IasProperties volumeProperties;
    volumeProperties.set<int32_t>("numFilterBands",2);
    IasStringVector activePins;
    activePins.push_back("module pin 0");
    volumeProperties.set("activePinsForBand.0", activePins);
    volumeProperties.set("activePinsForBand.1", activePins);
    int32_t tempVol = 0;
    int32_t tempGain = 0;
    IasInt32Vector ldGains;
    IasInt32Vector ldVolumes;
    for(uint32_t i=0; i< 8; i++)
    {
      ldVolumes.push_back(tempVol);
      ldGains.push_back(tempGain);
      tempVol-= 60;
      tempGain+= 30;
    }
    volumeProperties.set("ld.volumes.0", ldVolumes);
    volumeProperties.set("ld.gains.0", ldGains);
    volumeProperties.set("ld.volumes.1", ldVolumes);
    volumeProperties.set("ld.gains.1", ldGains);
    volumeProperties.set("ld.volumes.2", ldVolumes);
    volumeProperties.set("ld.gains.2", ldGains);

    IasInt32Vector freqOrderType;
    IasFloat32Vector gainQual;
    freqOrderType.resize(3);
    gainQual.resize(2);
    freqOrderType[0] = 100;
    freqOrderType[1] = 2;
    freqOrderType[2] = eIasFilterTypeLowShelving;
    gainQual[0] = 1.0f; // gain
    gainQual[1] = 2.0f; // quality
    volumeProperties.set("ld.freq_order_type.0", freqOrderType);
    volumeProperties.set("ld.gain_quality.0", gainQual);
    freqOrderType[0] = 8000;
    freqOrderType[1] = 2;
    freqOrderType[2] = eIasFilterTypeHighShelving;
    gainQual[0] = 1.0f; // gain
    gainQual[1] = 2.0f; // quality
    volumeProperties.set("ld.freq_order_type.1", freqOrderType);
    volumeProperties.set("ld.gain_quality.1", gainQual);

    setup->setProperties(processingModule, volumeProperties);
  }

  // Create input pins
  IasAudioPinParams pinParams;
  for (uint32_t cntPins = 0; cntPins < 1; cntPins++)
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
  for (uint32_t cntPins = 0; cntPins < 1; cntPins++)
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
  for (uint32_t cntPins = 0; cntPins < 1; cntPins++)
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
  result = setup->addAudioOutputPin(mPipeline, mOutputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Add pins to processing modules.
  result = setup->addAudioInOutPin(mProcessingModules[0], mModulePins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Create the links between the pins.
  result = setup->link(mInputPins[0], mModulePins[0], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  result = setup->link(mModulePins[0], mOutputPins[0], eIasAudioPinLinkTypeImmediate);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Setup the pipeline's audio chain of processing modules.
  result = setup->initPipelineAudioChain(mPipeline);
  IAS_ASSERT(IasISetup::eIasOk == result);

  mPipeline->dumpConnectionParameters();
  mPipeline->dumpProcessingSequence();


  // Link the routing zone input ports to the pipeline input pins.
  result = setup->link(mRoutingZoneInputPort0, mInputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);

  // Link the pipeline output pins to the sink device input ports.
  result = setup->link(mSinkDeviceInputPort0, mOutputPins[0]);
  IAS_ASSERT(IasISetup::eIasOk == result);
}


}
