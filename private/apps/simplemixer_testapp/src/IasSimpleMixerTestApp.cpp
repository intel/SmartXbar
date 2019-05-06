/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSimpleMixerTestApp.cpp
 * @date   2017
 * @brief  Test application for the simple mixer, using the SmartXbar test framework.
 *
 * This application also provides an example how the SmartX test framework can be used
 * for setting up unit tests for custom processing modules.
 */

#include <iostream>
#include <sndfile.h>
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"

#include "audio/testfwx/IasTestFramework.hpp"
#include "audio/testfwx/IasTestFrameworkSetup.hpp"

#include "audio/simplemixer/IasSimpleMixerCmd.hpp"  // Command interface of the simple mixer plugin-module

static const uint32_t cIasFrameLength  =    64;
static const uint32_t cIasSampleRate   = 48000;
static const uint32_t cNumInputStreams =     2;
static const uint32_t cNumOutputStreams =    2;

using namespace IasAudio;
using namespace std;

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  cout << "This test application for the simple mixer implements the following" << endl;
  cout << "pipeline by means of the SmartX Test Framework:" << endl;
  cout << "                                               " << endl;
  cout << "                simple mixer                    " << endl;
  cout << "             +----------------+                 " << endl;
  cout << "             |                |                 " << endl;
  cout << "infile 1 --->|-----O--->+---->|---> outfile1    " << endl;
  cout << "             |      \\  /      |                 " << endl;
  cout << "             |       \\/       |                 " << endl;
  cout << "             |       /\\       |                 " << endl;
  cout << "             |      /  \\      |                 " << endl;
  cout << "infile 2 --->|-----O--->+---->|---> outfile2    " << endl;
  cout << "             |                |                 " << endl;
  cout << "             +----------------+                 " << endl << endl;
  cout << "The SmartXbar Plugin Engine assumes that the library providing the simplemixer module" << endl;
  cout << "is located at '/opt/audio/plugin'. If this does not match to your environment, you"    << endl;
  cout << "have to set the environment variable AUDIO_PLUGIN_DIR accordingly."            << endl << endl;

  if (argc != 5)
  {
    cout << "Usage:                                                       " << endl;
    cout << "  ./simplemixer_testapp infile1.wav infile2.wav outfile1.wav outfile2.wav" << endl;
    cout << "                                                             " << endl;
    exit(1);
  }

  DLT_REGISTER_APP("XBAR", "SmartXbar example");
  DLT_VERBOSE_MODE();

  IasTestFrameworkSetup::IasResult setupResult = IasTestFrameworkSetup::eIasOk;
  IasTestFramework::IasResult      testfwxResult = IasTestFramework::eIasOk;

  IasPipelineParams pipelineParams;   //pipeline params

  IasProcessingModulePtr    simpleMixerModule = nullptr;
  IasProcessingModuleParams simpleMixerModuleParams;

  IasTestFrameworkWaveFileParams inputFileParams [cNumInputStreams];   //input wave file params
  IasTestFrameworkWaveFileParams outputFileParams[cNumOutputStreams];  //output wave file params

  IasAudioPinParams pipelineInputPinParams [cNumInputStreams];    //pipeline input pin params
  IasAudioPinParams pipelineOutputPinParams[cNumOutputStreams];   //pipeline output pin params
  IasAudioPinParams moduleInputPinParams   [cNumInputStreams];    //module input pin params
  IasAudioPinParams moduleOutputPinParams  [cNumOutputStreams];   //module output pin params

  IasAudioPinPtr pipelineInputPin [cNumInputStreams];    //pipeline input pin pointers
  IasAudioPinPtr pipelineOutputPin[cNumOutputStreams];   //pipeline output pin pointers
  IasAudioPinPtr moduleInputPin   [cNumInputStreams];    //module input pin pointers
  IasAudioPinPtr moduleOutputPin  [cNumOutputStreams];   //module output pin pointers

  pipelineParams.name = "TestFwxSimpleMixerTestPipeline";
  pipelineParams.samplerate = cIasSampleRate;
  pipelineParams.periodSize = cIasFrameLength;

  simpleMixerModuleParams.typeName     = "simplemixer";
  simpleMixerModuleParams.instanceName = "SimpleMixer";

  // Get the names for the wave files from the command line arguments.
  inputFileParams[0].fileName = std::string(argv[1]);
  inputFileParams[1].fileName = std::string(argv[2]);
  outputFileParams[0].fileName = std::string(argv[3]);
  outputFileParams[1].fileName = std::string(argv[4]);

  cout << "reading 1st stream from: " << inputFileParams[0].fileName << endl;
  cout << "reading 2nd stream from: " << inputFileParams[1].fileName << endl;
  cout << "writing 1st stream to:   " << outputFileParams[0].fileName << endl;
  cout << "writing 2st stream to:   " << outputFileParams[1].fileName << endl << endl;

  pipelineInputPinParams[0].name = "pipelineInputPin0";
  pipelineInputPinParams[1].name = "pipelineInputPin1";

  pipelineOutputPinParams[0].name = "pipelineOutputPin0";
  pipelineOutputPinParams[1].name = "pipelineOutputPin1";

  moduleInputPinParams[0].name  = "moduleInputPin0";
  moduleInputPinParams[1].name  = "moduleInputPin1";
  moduleOutputPinParams[0].name = "moduleOutputPin0";
  moduleOutputPinParams[1].name = "moduleOutputPin1";

  // Obtain number of channels of input wave files and update pin properties accordingly
  for (uint32_t i = 0; i < cNumInputStreams; i++)
  {
    SNDFILE *inputFile = nullptr;
    SF_INFO inputFileInfo;

    inputFile = sf_open(inputFileParams[i].fileName.c_str(), SFM_READ, &inputFileInfo);
    if (inputFile == nullptr)
    {
      cout << "Unable to open input file " << inputFileParams[i].fileName << endl;
      exit(1);
    }

    // Set the number channels for all input pins.
    pipelineInputPinParams[i].numChannels = static_cast<uint32_t>(inputFileInfo.channels);
    moduleInputPinParams[i].numChannels = pipelineInputPinParams[i].numChannels;

    cout << inputFileParams[i].fileName << " provides " << pipelineInputPinParams[i].numChannels << " channels." << endl;
    sf_close(inputFile);
  }

  // Verify that all input files provide the same number of channels.
  uint32_t numChannels = pipelineInputPinParams[0].numChannels;
  for (uint32_t i = 1; i < cNumInputStreams; i++)
  {
    if (pipelineInputPinParams[i].numChannels != numChannels)
    {
      cout << "Number of channels do not match!" << endl;
      exit(1);
    }
  }

  // Set the number channels for all output pins.
  for (uint32_t i = 0; i < cNumOutputStreams; i++)
  {
    pipelineOutputPinParams[i].numChannels = numChannels;
    moduleOutputPinParams[i].numChannels   = numChannels;
  }

  // Create test framework
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  if (testfwx == nullptr)
  {
    cout << "IasTestFramework::create() has failed" << endl;
    exit(1);
  }

  // Obtain test framework setup interface
  IasTestFrameworkSetup *setup = testfwx->setup();
  if (setup == nullptr)
  {
    cout << "IasTestFramework setup interface does not exist" << endl;
    exit(1);
  }

  // Obtain processing interface
  IasIProcessing *processing = testfwx->processing();
  if (processing == nullptr)
  {
    cout << "IasTestFramework processing interface does not exist" << endl;
    exit(1);
  }

  // Create simple mixer processing module
  setupResult = setup->createProcessingModule(simpleMixerModuleParams, &simpleMixerModule);
  if (IasTestFrameworkSetup::eIasOk != setupResult)
  {
    cout << "setup->createProcessingModule() has returned " << toString(setupResult) << endl;
    exit(1);
  }
  if (simpleMixerModule == nullptr)
  {
    cout << "simpleMixerModule must not be nullptr" << endl;
    exit(1);
  }

  // Configuration of the simple mixer: set the default gain to -6.0 dB.
  IasProperties simpleMixerProperties;
  simpleMixerProperties.set<int32_t>("defaultGain", -60);
  setup->setProperties(simpleMixerModule,simpleMixerProperties);

  // Add simple mixer processing module to pipeline
  setupResult = setup->addProcessingModule(simpleMixerModule);
  if (IasTestFrameworkSetup::eIasOk != setupResult)
  {
    cout << "setup->addProcessingModule() has returned " << toString(setupResult) << endl;
    exit(1);
  }

  for (uint32_t i = 0; i < cNumInputStreams; i++)
  {
    // Create and add pipeline input pin
    setupResult = setup->createAudioPin(pipelineInputPinParams[i], &pipelineInputPin[i]);
    if ((setupResult != IasTestFrameworkSetup::eIasOk) || (pipelineInputPin[i] == nullptr))
    {
      cout << "Error while calling setup->createAudioPin(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }

    setupResult = setup->addAudioInputPin(pipelineInputPin[i]);
    if (setupResult != IasTestFrameworkSetup::eIasOk)
    {
      cout << "Error while calling setup->addAudioInputPin(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }

    // Link pipeline input pin with wave file
    setupResult = setup->linkWaveFile(pipelineInputPin[i], inputFileParams[i]);
    if (setupResult != IasTestFrameworkSetup::eIasOk)
    {
      cout << "Error while calling setup->linkWaveFile(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }

    // Create input pin for simple mixer processing module
    setupResult = setup->createAudioPin(moduleInputPinParams[i], &moduleInputPin[i]);
    if ((setupResult != IasTestFrameworkSetup::eIasOk) || (moduleInputPin[i] == nullptr))
    {
      cout << "Error while calling setup->createAudioPin(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }
  }

  for (uint32_t i = 0; i < cNumOutputStreams; i++)
  {
    // Create and add pipeline output pin
    setupResult = setup->createAudioPin(pipelineOutputPinParams[i], &pipelineOutputPin[i]);
    if ((setupResult != IasTestFrameworkSetup::eIasOk) || (pipelineOutputPin[i] == nullptr))
    {
      cout << "Error while calling setup->createAudioPin(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }

    setupResult = setup->addAudioOutputPin(pipelineOutputPin[i]);
    if (setupResult != IasTestFrameworkSetup::eIasOk)
    {
      cout << "Error while calling setup->addAudioOutputPin(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }

    // Link output pin with wave file
    setupResult = setup->linkWaveFile(pipelineOutputPin[i], outputFileParams[i]);
    if (setupResult != IasTestFrameworkSetup::eIasOk)
    {
      cout << "Error while calling setup->linkWaveFile(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }

    // Create output pin for simple mixer processing module
    setupResult = setup->createAudioPin(moduleOutputPinParams[i], &moduleOutputPin[i]);
    if ((setupResult != IasTestFrameworkSetup::eIasOk) || (moduleOutputPin[i] == nullptr))
    {
      cout << "Error while calling setup->createAudioPin(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }
  }

  // Add pin mappings for simple mixer module. We implement all possible pin mappings:
  // input0->output0, input1->output0, input0->output1, and input1->output1
  setupResult = setup->addAudioPinMapping(simpleMixerModule, moduleInputPin[0], moduleOutputPin[0]);
  if (setupResult != IasTestFrameworkSetup::eIasOk)
  {
    cout << "Error while calling setup->addAudioPinMapping(), setupResult = " << toString(setupResult) << endl;
    exit(1);
  }
  setupResult = setup->addAudioPinMapping(simpleMixerModule, moduleInputPin[0], moduleOutputPin[1]);
  if (setupResult != IasTestFrameworkSetup::eIasOk)
  {
    cout << "Error while calling setup->addAudioPinMapping(), setupResult = " << toString(setupResult) << endl;
    exit(1);
  }
  setupResult = setup->addAudioPinMapping(simpleMixerModule, moduleInputPin[1], moduleOutputPin[0]);
  if (setupResult != IasTestFrameworkSetup::eIasOk)
  {
    cout << "Error while calling setup->addAudioPinMapping(), setupResult = " << toString(setupResult) << endl;
    exit(1);
  }
  setupResult = setup->addAudioPinMapping(simpleMixerModule, moduleInputPin[1], moduleOutputPin[1]);
  if (setupResult != IasTestFrameworkSetup::eIasOk)
  {
    cout << "Error while calling setup->addAudioPinMapping(), setupResult = " << toString(setupResult) << endl;
    exit(1);
  }

  for (uint32_t i = 0; i < cNumInputStreams; i++)
  {
    // Link pipeline input pin with module input pin
    setupResult = setup->link(pipelineInputPin[i], moduleInputPin[i] ,eIasAudioPinLinkTypeImmediate);
    if (setupResult != IasTestFrameworkSetup::eIasOk)
    {
      cout << "Error while calling setup->link(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }
  }

  for (uint32_t i = 0; i < cNumOutputStreams; i++)
  {
    // Link module output pin with pipeline output pin
    setupResult = setup->link(moduleOutputPin[i],  pipelineOutputPin[i], eIasAudioPinLinkTypeImmediate);
    if (setupResult != IasTestFrameworkSetup::eIasOk)
    {
      cout << "Error while calling setup->link(), setupResult = " << toString(setupResult) << endl;
      exit(1);
    }
  }

  // Start processing
  testfwxResult = testfwx->start();
  if (testfwxResult != IasTestFramework::eIasOk)
  {
    cout << "Error while calling testfwx->start()" << endl;
    exit(1);
  }

  uint32_t periodCounter = 0;       // number of periods already processed
  uint32_t numPeriodsToProcess = 1; // number of periods to process in one iteration

  // Main processing loop
  do
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    int32_t cmdId;

    // Send commands for output channel 0
    if ((periodCounter % 8000) == 0)
    {
      cout << endl << "Output 0: Input stream 0 stays audible, input stream 1 becomes muted." << endl;
      cmdProperties.clearAll();
      cmdId = IasSimpleMixer::eIasSetXMixerGain;
      cmdProperties.set<int32_t>("cmd", cmdId);
      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[0].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[0].name);
      cmdProperties.set<int32_t>("gain", -60);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);

      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[1].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[0].name);
      cmdProperties.set<int32_t>("gain", -1440);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);
    }
    else if ((periodCounter % 8000) == 2000)
    {
      cout << endl << "Output 0: Input stream 1 becomes audible." << endl;
      cmdId = IasSimpleMixer::eIasSetXMixerGain;
      cmdProperties.set<int32_t>("cmd", cmdId);
      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[1].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[0].name);
      cmdProperties.set<int32_t>("gain", -60);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);
    }
    else if ((periodCounter % 8000) == 4000)
    {
      cout << endl << "Output 0: Input stream 0 becomes muted." << endl;
      cmdId = IasSimpleMixer::eIasSetXMixerGain;
      cmdProperties.set<int32_t>("cmd", cmdId);
      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[0].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[0].name);
      cmdProperties.set<int32_t>("gain", -1440);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);
    }
    else if ((periodCounter % 8000) == 6000)
    {
      cout << endl << "Output 0: Input stream 0 becomes maudible." << endl;
      cmdId = IasSimpleMixer::eIasSetXMixerGain;
      cmdProperties.set<int32_t>("cmd", cmdId);
      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[0].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[0].name);
      cmdProperties.set<int32_t>("gain", -60);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);
    }

    // Send commands for output channel 1
    if ((periodCounter % 6000) == 0)
    {
      cout << endl << "Output 1: Input stream 0 becomes audible, input stream 1 becomes muted." << endl;
      cmdProperties.clearAll();
      cmdId = IasSimpleMixer::eIasSetXMixerGain;
      cmdProperties.set<int32_t>("cmd", cmdId);
      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[0].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[1].name);
      cmdProperties.set<int32_t>("gain", 0);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);

      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[1].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[1].name);
      cmdProperties.set<int32_t>("gain", -1440);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);
    }
    else if ((periodCounter % 6000) == 3000)
    {
      cout << endl << "Output 1: Input stream 0 becomes muted, input stream 1 becomes audible." << endl;
      cmdId = IasSimpleMixer::eIasSetXMixerGain;
      cmdProperties.set<int32_t>("cmd", cmdId);
      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[0].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[1].name);
      cmdProperties.set<int32_t>("gain", -1440);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);

      cmdProperties.set<std::string>("input_pin",  moduleInputPinParams[1].name);
      cmdProperties.set<std::string>("output_pin", moduleOutputPinParams[1].name);
      cmdProperties.set<int32_t>("gain", 0);
      processing->sendCmd(simpleMixerModuleParams.instanceName, cmdProperties, returnProperties);
    }

    // Process given number of periods
    testfwxResult = testfwx->process(numPeriodsToProcess);
    if ((testfwxResult != IasTestFramework::eIasOk) && (testfwxResult != IasTestFramework::eIasFinished))
    {
      cout << "Error while calling testfwx->process()" << endl;
      exit(1);
    }

    // Check for events.
    while (testfwx->waitForEvent(0) == IasTestFramework::eIasOk)
    {
      IasEventPtr newEvent;
      testfwxResult = testfwx->getNextEvent(&newEvent);
      if (testfwxResult == IasTestFramework::eIasOk)
      {
        class : public IasEventHandler
        {
          virtual void receivedConnectionEvent(IasConnectionEvent* event)
          {
            cout << "Received connection event(" << toString(event->getEventType()) << ")" << endl;
          }
          virtual void receivedSetupEvent(IasSetupEvent* event)
          {
            cout << "Received setup event(" << toString(event->getEventType()) << ")" << endl;
          }
          virtual void receivedModuleEvent(IasModuleEvent* event)
          {
            const IasProperties eventProperties = event->getProperties();
            std::string typeName = "";
            std::string instanceName = "";
            int32_t eventType;
            eventProperties.get<std::string>("typeName", &typeName);
            eventProperties.get<std::string>("instanceName", &instanceName);
            eventProperties.get<int32_t>("eventType", &eventType);
            cout << "Received module event: "
                 << "typeName = " << typeName << ", "
                 << "instanceName = " << instanceName << endl;

            // Process all events from simple mixer module.
            if (typeName == "simplemixer")
            {
              // Module-specific handling of simple mixer events. The simple mixer supports only one event type.
              if (eventType == static_cast<int32_t>(IasSimpleMixer::eIasGainUpdateFinished))
              {
                float gain; std::string  inputPinName; std::string  outputPinName;
                IasProperties::IasResult result;
                result = eventProperties.get("gain", &gain);
                result = eventProperties.get("input_pin",  &inputPinName);
                result = eventProperties.get("output_pin", &outputPinName);
                (void)result;
                cout << inputPinName << " to " << outputPinName
                     << " updated gain (linear): " << gain << endl;
              }
            }
            // Events from further module type names could be processed here...
            // This is not required, because this test app includes only the simple mixer.
          }
        } myEventHandler;
        newEvent->accept(myEventHandler);
      }
    }

    periodCounter++;

  } while ( testfwxResult == IasTestFramework::eIasOk );

  // Stop processing
  testfwxResult = testfwx->stop();
  if (testfwxResult != IasTestFramework::eIasOk)
  {
    cout << "Error while calling testfwx->stop()" << endl;
    exit(1);
  }

  cout <<"All periods have been processed." << endl;

  // Clean up
  for (uint32_t i = 0; i< cNumInputStreams; i++)
  {
    // Unlink wave files from pins
    setup->unlinkWaveFile(pipelineInputPin[i]);

    setup->unlink(pipelineInputPin[i], moduleInputPin[i]);

    // Destroy audio pins
    setup->destroyAudioPin(&pipelineInputPin[i]);
    setup->destroyAudioPin(&moduleInputPin[i]);
  }

  for (uint32_t i = 0; i< cNumOutputStreams; i++)
  {
    // Unlink wave files from pins
    setup->unlinkWaveFile(pipelineOutputPin[i]);

    setup->unlink(moduleOutputPin[i], pipelineOutputPin[i]);

    // Destroy audio pins
    setup->destroyAudioPin(&pipelineOutputPin[i]);
    setup->destroyAudioPin(&moduleOutputPin[i]);
  }

  // Destroy simple mixer processing module
  setup->destroyProcessingModule(&simpleMixerModule);

  // Destroy test framework
  IasTestFramework::destroy(testfwx);

  cout << "Test framework has been cleaned up." << endl;

  return 0;
}
