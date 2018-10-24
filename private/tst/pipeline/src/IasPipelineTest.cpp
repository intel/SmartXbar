/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file  IasPipelineTest.cpp
 * @date  2016
 * @brief
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>


#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"

#include "model/IasPipeline.hpp"
#include "model/IasAudioPin.hpp"
#include "IasPipelineTest.hpp"


namespace IasAudio
{

#ifndef AUDIO_PLUGIN_DIR
#define AUDIO_PLUGIN_DIR    "."
#endif

void IasPipelineTest::SetUp()
{
  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);
}

void IasPipelineTest::TearDown()
{
}


TEST_F(IasPipelineTest, convertResultToString)
{
  std::cout << "Possible enum values of IasPipeline::IasResult:" << std::endl;
  std::cout << "  " << toString(IasPipeline::eIasOk)              << std::endl;
  std::cout << "  " << toString(IasPipeline::eIasFailed)          << std::endl;
}

TEST_F(IasPipelineTest, pinMapping)
{
  IasAudioPinMapping myMapping;
  ASSERT_TRUE(myMapping.inputPin == nullptr);
  ASSERT_TRUE(myMapping.outputPin == nullptr);
}

TEST_F(IasPipelineTest, Pipeline_Setup)
{
  /*
   *
   * This unit test configures the following pipeline by means of the Setup Interface and
   * identifies in which order the processing modules 0 to 5 can be executed.
   *
   *     +------------------------------------------------------------------------------+
   *     |                                                                              |
   *     |      +----------+      +----------+      +----------+                        |
   *     |     0|          |0    1|          |1    5| module 4 |7                       |
   * in0 O----->O module 0 O----->O module 1 O----->O..........O----------------------->O out0
   *     |      |          |      |          |      |        .°|                        |
   *     |      +----------+      +----------+      |      .°  |                        |
   *     |                                          |    .°    |                        |
   *     |                        +----------+      |  .°      |      +----------+      |
   *     |                       9| module 5 |11   6|.°        |8    3|          |3     |
   * in1 O------------------------O..........O----->O..........O----->O          O----->O out1
   *     |                        |  °.  .°  |      |          |      |          |      |
   *     |      +----------+      |    ::    |      +----------+      | module 3 |      |
   *     |     2|          |2   10|  .°  °.  |12                     4|          |4     |
   *     |  +-->O module 2 O----->O.:......:.O----------------------->O          O---+  |
   *     |  |   |          |      |          |                        |          |   |  |
   *     |  |   +----------+      +----------+                        +----------+   |  |
   *     |  |                                  +---+                                 |  |
   *     |  +----------------------------------| T |<--------------------------------+  |
   *     |                                     +---+                                    |
   *     |                                                                              |
   *     +------------------------------------------------------------------------------+
   *
   */

  // Create the smartx instance
  IasSmartX* smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup* setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasISetup::IasResult result;


  // Create the pipeline
  IasPipelineParams pipelineParams =
  {
    /*.name =*/ "myPipeline",
    /*.samplerate =*/ 48000,
    /*.periodSize =*/ 192
  };
  IasPipelinePtr pipeline = nullptr;
  result = setup->createPipeline(pipelineParams, nullptr); // invalid pipeline pointer
  ASSERT_EQ(IasISetup::eIasFailed, result);

  pipelineParams.name = ""; // invalid name
  result = setup->createPipeline(pipelineParams, &pipeline);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  pipelineParams.name = "myPipeline";
  result = setup->createPipeline(pipelineParams, &pipeline);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(pipeline != nullptr);


  // Create processing modules
  std::vector<IasProcessingModulePtr> processingModules;
  for (uint32_t cntModules = 0; cntModules < 6; cntModules++)
  {
    IasProcessingModuleParams processingModuleParams =
    {
      /*.typeName     =*/ "simplevolume",
      /*.instanceName =*/ "simplevolume " + std::to_string(cntModules)
    };

    IasProcessingModulePtr processingModule = nullptr;
    result = setup->createProcessingModule(processingModuleParams, &processingModule);
    ASSERT_EQ(IasISetup::eIasOk, result);
    ASSERT_TRUE(processingModule != nullptr);
    processingModules.push_back(processingModule);
  }

  // Try to delete a module from the pipeline, although it has not been added to the pipeline before.
  setup->deleteProcessingModule(pipeline, processingModules[0]);

  IasAudioPinParams pinParams;

  // Try to create pins with invalid parameters.
  pinParams.name = "test pin";
  pinParams.numChannels = 1;

  IasAudioPinPtr testPin = nullptr;
  result = setup->createAudioPin(pinParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  pinParams.name = "";
  result = setup->createAudioPin(pinParams, &testPin);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  pinParams.name = "test pin";
  pinParams.numChannels = 0;
  result = setup->createAudioPin(pinParams, &testPin);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Create input pins
  std::vector<IasAudioPinPtr> inputPins;
  for (uint32_t cntPins = 0; cntPins < 2; cntPins++)
  {
    pinParams =
    {
      /*.name =*/ "input pin " + std::to_string(cntPins),
      /*.numChannels =*/ 1,
    };
    IasAudioPinPtr inputPin = nullptr;
    result = setup->createAudioPin(pinParams, &inputPin);
    ASSERT_EQ(IasISetup::eIasOk, result);
    ASSERT_TRUE(inputPin != nullptr);
    inputPins.push_back(inputPin);
  }

  // Create output pins
  std::vector<IasAudioPinPtr> outputPins;
  for (uint32_t cntPins = 0; cntPins < 2; cntPins++)
  {
    pinParams =
    {
      /*.name =*/ "output pin " + std::to_string(cntPins),
      /*.numChannels =*/ 1,
    };
    IasAudioPinPtr outputPin = nullptr;
    result = setup->createAudioPin(pinParams, &outputPin);
    ASSERT_EQ(IasISetup::eIasOk, result);
    ASSERT_TRUE(outputPin != nullptr);
    outputPins.push_back(outputPin);
  }

  // Create module pins
  std::vector<IasAudioPinPtr> modulePins;
  for (uint32_t cntPins = 0; cntPins < 13; cntPins++)
  {
    pinParams =
    {
      /*.name =*/ "module pin " + std::to_string(cntPins),
      /*.numChannels =*/ 1,
    };
    IasAudioPinPtr inputOutputPin = nullptr;
    result = setup->createAudioPin(pinParams, &inputOutputPin);
    ASSERT_EQ(IasISetup::eIasOk, result);
    ASSERT_TRUE(inputOutputPin != nullptr);
    modulePins.push_back(inputOutputPin);
  }

  // Create pin with mismatching channel number
  pinParams =
  {
    /*.name =*/ "mismatching pin",
    /*.numChannels =*/ 42,
  };
  IasAudioPinPtr mismatchingPin = nullptr;
  result = setup->createAudioPin(pinParams, &mismatchingPin);
  ASSERT_EQ(IasISetup::eIasOk, result);
  ASSERT_TRUE(mismatchingPin != nullptr);

  // Try to add a pin to a module, before the module belongs to a pipeline. This test must fail.
  result = setup->addAudioInOutPin(processingModules[0], modulePins[0]);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Add all processing modules to the pipeline
  for (uint32_t cntModules = 0; cntModules < processingModules.size(); cntModules++)
  {
    result = setup->addProcessingModule(pipeline, processingModules[cntModules]);
    ASSERT_EQ(IasISetup::eIasOk, result);
  }

  // Try to add the first processing module to the pipeline twice. This test must fail.
  result = setup->addProcessingModule(pipeline, processingModules[0]);
  ASSERT_EQ(IasISetup::eIasFailed, result);


  // Try to add/delete pins using invalid parameters.
  result = setup->addAudioInputPin(pipeline, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, result);
  result = setup->addAudioInputPin(nullptr, inputPins[0]);
  ASSERT_EQ(IasISetup::eIasFailed, result);
  result = setup->addAudioOutputPin(pipeline, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, result);
  result = setup->addAudioOutputPin(nullptr, outputPins[0]);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Add pins to pipeline
  result = setup->addAudioInputPin(pipeline, inputPins[0]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPin(pipeline, inputPins[1]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPin(pipeline, mismatchingPin);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioOutputPin(pipeline, outputPins[0]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioOutputPin(pipeline, outputPins[1]);
  ASSERT_EQ(IasISetup::eIasOk, result);

  IasAudioPinParamsPtr dummyParams = std::make_shared<IasAudioPinParams>("dummy",2);
  IasAudioPinPtr dummyPin = std::make_shared<IasAudioPin>(dummyParams);
  
  std::string tmpName="dummyFile";
  IasPipeline::IasResult pipeResult = pipeline->startPinProbing(inputPins[0],tmpName,3,false);
  ASSERT_EQ(IasPipeline::eIasFailed, pipeResult);
  pipeResult = pipeline->startPinProbing(outputPins[0],tmpName,3,false);
  ASSERT_EQ(IasPipeline::eIasFailed, pipeResult);

  pipeline->startPinProbing(dummyPin,tmpName,3,false);
  ASSERT_EQ(pipeline->startPinProbing(dummyPin,tmpName,3,false),IasPipeline::eIasFailed);
  pipeline->stopPinProbing(dummyPin);

  dummyPin = nullptr;

  // Add pins to processing modules.
  result = setup->addAudioInOutPin(processingModules[0], modulePins[0]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInOutPin(processingModules[1], modulePins[1]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInOutPin(processingModules[2], modulePins[2]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInOutPin(processingModules[3], modulePins[3]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInOutPin(processingModules[3], modulePins[4]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[4], modulePins[5], modulePins[7]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[4], modulePins[6], modulePins[7]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[4], modulePins[6], modulePins[8]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[5], modulePins[9], modulePins[11]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[5], modulePins[10], modulePins[11]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[5], modulePins[9], modulePins[12]);
  ASSERT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioPinMapping(processingModules[5], modulePins[10], modulePins[12]);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Try do establish links with incorrect pin direction.
  result = setup->link(modulePins[0], inputPins[0], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  result = setup->link(outputPins[0], modulePins[0], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Try do establish links with incorrect number of channels.
  result = setup->link(mismatchingPin, modulePins[0], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Remove the mismatching pin from the pipeline and destroy it.
  setup->deleteAudioInputPin(pipeline, mismatchingPin);
  setup->destroyAudioPin(&mismatchingPin);


  // Create the links between the pins.
  result = setup->link(inputPins[0], modulePins[0], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(inputPins[1], modulePins[9], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[0], modulePins[1], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[1], modulePins[5], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[4], modulePins[2], eIasAudioPinLinkTypeDelayed);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[2], modulePins[10], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[11], modulePins[6], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[12], modulePins[4], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[8], modulePins[3], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[7], outputPins[0], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  result = setup->link(modulePins[3], outputPins[1], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, result);

  // Try do establish additional links. These attempts must fail.
  result = setup->link(inputPins[0], modulePins[4], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  result = setup->link(modulePins[4], outputPins[0], eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, result);

  // Setup the pipeline's audio chain of processing modules.
  result = setup->initPipelineAudioChain(pipeline);
  ASSERT_EQ(IasISetup::eIasOk, result);

  pipeline->dumpConnectionParameters();
  pipeline->dumpProcessingSequence();

  std::cout << std::endl << std::endl << std::endl;
  pipeline->process();
  std::cout << std::endl << std::endl << std::endl;

  // Delete input pins and output pins from pipeline and destroy them
  for (uint32_t cntPins = 0; cntPins < inputPins.size(); cntPins++)
  {
    setup->deleteAudioInputPin(pipeline, inputPins[cntPins]);
    setup->destroyAudioPin(&inputPins[cntPins]);
  }
  for (uint32_t cntPins = 0; cntPins < outputPins.size(); cntPins++)
  {
    setup->deleteAudioOutputPin(pipeline, outputPins[cntPins]);
    setup->destroyAudioPin(&outputPins[cntPins]);
  }

  setup->deleteAudioInOutPin(processingModules[0], modulePins[0]);
  setup->deleteAudioInOutPin(processingModules[1], modulePins[1]);
  setup->deleteAudioInOutPin(processingModules[2], modulePins[2]);
  setup->deleteAudioInOutPin(processingModules[3], modulePins[3]);
  setup->deleteAudioInOutPin(processingModules[3], modulePins[4]);
  setup->deleteAudioPinMapping(processingModules[4], modulePins[5], modulePins[7]);
  setup->deleteAudioPinMapping(processingModules[4], modulePins[6], modulePins[7]);
  setup->deleteAudioPinMapping(processingModules[4], modulePins[6], modulePins[8]);
  setup->deleteAudioPinMapping(processingModules[5], modulePins[9], modulePins[11]);

  // Delete all processing modules from the pipeline.
  for (uint32_t cntModules = 0; cntModules < processingModules.size(); cntModules++)
  {
    setup->deleteProcessingModule(pipeline, processingModules[cntModules]);
  }

  // Destroy all audio pins.
  for (uint32_t cntPins = 0; cntPins < modulePins.size(); cntPins++)
  {
    setup->destroyAudioPin(&modulePins[cntPins]);
  }

  setup->destroyAudioPin(nullptr); // verify that function does not crash if called with nullptr

  setup->destroyProcessingModule(nullptr); // verify that function does not crash if called with nullptr
  setup->destroyProcessingModule(&processingModules[0]);
  ASSERT_TRUE(processingModules[0] == nullptr);

  setup->destroyProcessingModule(&processingModules[1]);
  ASSERT_TRUE(processingModules[1] == nullptr);

  setup->destroyPipeline(nullptr); // verify that function does not crash if called with nullptr
  setup->destroyPipeline(&pipeline);
  ASSERT_TRUE(pipeline == nullptr);
}


}
