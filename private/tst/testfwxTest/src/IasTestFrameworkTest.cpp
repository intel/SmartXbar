#include "gtest/gtest.h"
#include "audio/testfwx/IasTestFramework.hpp"
#include "audio/testfwx/IasTestFrameworkSetup.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "testfwx/IasTestFrameworkWaveFile.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasPipeline.hpp"
#include "stdio.h"

namespace IasAudio
{

//common variables
static const uint32_t cIasFrameLength  =    64;
static const uint32_t cIasSampleRate   = 44100;

static IasPipelineParams pipelineParams = {"TestPipeline", cIasSampleRate, cIasFrameLength};

static IasAudioPinParams pipelineInputPinParams1 = {"PipelineInputPin1", 2};
static IasAudioPinParams pipelineInputPinParams2 = {"PipelineInputPin2", 2};
static IasAudioPinParams pipelineOutputPinParams1 = {"PipelineOutputPin1", 2};
static IasAudioPinParams pipelineOutputPinParams2 = {"PipelineOutputPin2", 2};

static IasAudioPinParams moduleInOutPinParams = {"ModuleInOutPin", 2};
static IasAudioPinParams moduleInputPinParams = {"ModuleInputPin", 2};
static IasAudioPinParams moduleOutputPinParams = {"ModuleOutputPin", 2};

static IasTestFrameworkWaveFileParams inputWaveFileParams = {"intel_masterbrand.wav"};
static IasTestFrameworkWaveFileParams outputWaveFileParams = {"output_testframework_test_file.wav"};

static IasProcessingModuleParams moduleParams = {"ias.volume", "VolumeLoudness"};


class IasTestFrameworkTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
      setenv("AUDIO_PLUGIN_DIR", "../../..", true);
    }
    virtual void TearDown() {}
};


TEST_F(IasTestFrameworkTest, create_and_destroy_test_framework_test)
{
  //create test framework instance
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);

  //it is possible to create only one instance of the test framework,
  //when we call create() when one instance is already created we should obtain nullptr
  IasTestFramework *testfwx_null = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx_null == nullptr);

  IasTestFrameworkRoutingZoneParams rznParams;
  rznParams.name = "testrzn";
  ASSERT_EQ("testrzn", rznParams.name);

  //destroy test framework instance
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, create_and_destroy_test_framework_invalid_cases_test)
{
  IasPipelineParams pipelineInvalidParams;

  //invalid case 1: pipeline parameters are not specified
  IasTestFramework *testfwx_null = IasTestFramework::create(pipelineInvalidParams);
  ASSERT_TRUE(testfwx_null == nullptr);

  //invalid case 2: name is specified but sample rate and period size are 0
  pipelineInvalidParams.name = "TestPipeline";
  testfwx_null = IasTestFramework::create(pipelineInvalidParams);
  ASSERT_TRUE(testfwx_null == nullptr);

  //invalid case 3: name and sample rate are specified but period size is 0
  pipelineInvalidParams.samplerate = cIasSampleRate;
  testfwx_null = IasTestFramework::create(pipelineInvalidParams);
  ASSERT_TRUE(testfwx_null == nullptr);

  //invalid case 4: destroy test framework which was not created before
  IasTestFramework::destroy(testfwx_null);
}


TEST_F(IasTestFrameworkTest, obtain_setup_interface_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);

  //obtain setup interface
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //there should be only one instance of the setup interface at a time,
  //we should get the same instance everytime we call setup() method
  IasTestFrameworkSetup *setup_same = testfwx->setup();
  ASSERT_TRUE(setup_same != nullptr);
  ASSERT_EQ(setup, setup_same);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, obtain_processing_interface_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);

  //obtain processing interface
  IasIProcessing *processing = testfwx->processing();
  ASSERT_TRUE(processing != nullptr);

  //there should be only one instance of the processing interface at a time,
  //we should get the same instance everytime we call processing() method
  IasIProcessing *processing_same = testfwx->processing();
  ASSERT_TRUE(processing_same != nullptr);
  ASSERT_EQ(processing, processing_same);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, event_api_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);

  //there are no events in the event queue at this point
  IasTestFramework::IasResult testfwx_result = testfwx->waitForEvent(1000);
  ASSERT_EQ(IasTestFramework::eIasTimeout, testfwx_result);

  IasEventPtr receivedEvent = nullptr;
  testfwx_result = testfwx->getNextEvent(&receivedEvent);
  ASSERT_EQ(IasTestFramework::eIasNoEventAvailable, testfwx_result);
  ASSERT_EQ(receivedEvent, nullptr);

  //create event provider
  IasEventProvider *eventProvider = IasEventProvider::getInstance();
  ASSERT_TRUE(eventProvider != nullptr);

  //create module event
  IasModuleEventPtr moduleEvent = eventProvider->createModuleEvent();
  ASSERT_TRUE(moduleEvent != nullptr);

  //send module event
  eventProvider->send(moduleEvent);

  //now it should be possible to obrain event from the queue
  testfwx_result = testfwx->waitForEvent(1000);
  ASSERT_EQ(IasTestFramework::eIasOk, testfwx_result);

  testfwx_result = testfwx->getNextEvent(&receivedEvent);
  ASSERT_EQ(IasTestFramework::eIasOk, testfwx_result);
  ASSERT_TRUE(receivedEvent != nullptr);

  //after event has been consumed, we should get timeout on waiting for event
  testfwx_result = testfwx->waitForEvent(1000);
  ASSERT_EQ(IasTestFramework::eIasTimeout, testfwx_result);

  //destroy module event
  eventProvider->destroyModuleEvent(moduleEvent);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, create_and_destroy_audio_pin_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  ASSERT_EQ(toString(setup_result), "IasTestFrameworkSetup::eIasOk");
  ASSERT_TRUE(audioPin != nullptr);

  //destroy audio pin
  setup->destroyAudioPin(&audioPin);
  ASSERT_TRUE(audioPin == nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, create_and_destroy_audio_pin_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasAudioPinPtr audioPin = nullptr;
  IasAudioPinParams audioPinInvalidParams;

  //invalid case 1: create audio pin: valid pin parameters, pin pointer is null
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: create audio pin: pin name is not specified in pin parameters
  setup_result = setup->createAudioPin(audioPinInvalidParams, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_TRUE(audioPin == nullptr);

  //invalid case 3: create audio pin: pin name is specified but channels number is 0
  audioPinInvalidParams.name = "testAudioPin";
  setup_result = setup->createAudioPin(audioPinInvalidParams, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_TRUE(audioPin == nullptr);

  //invalid case 4: destroy audio pin: pin pointer is null
  setup->destroyAudioPin(nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_pipeline_input_pin_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add input pin to the pipeline
  setup_result = setup->addAudioInputPin(audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //verify if audio pin was successfully added as pipeline input pin
  ASSERT_EQ(IasAudioPin::eIasPinDirectionPipelineInput, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() != nullptr);
  ASSERT_EQ(audioPin->getPipeline()->getParameters()->name, pipelineParams.name);

  //delete input pin from the pipeline
  setup->deleteAudioInputPin(audioPin);

  //verify if input pin was successfully deleted from pipeline
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() == nullptr);

  //clean-up
  setup->destroyAudioPin(&audioPin);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_pipeline_input_pin_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //invalid case 1: add null audio pin to the pipeline
  IasTestFrameworkSetup::IasResult setup_result = setup->addAudioInputPin(nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: delete null audio pin from the pipeline
  setup->deleteAudioInputPin(nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_pipeline_output_pin_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineOutputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add output pin to the pipeline
  setup_result = setup->addAudioOutputPin(audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //verify if audio pin was successfully added as pipeline output pin
  ASSERT_EQ(IasAudioPin::eIasPinDirectionPipelineOutput, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() != nullptr);
  ASSERT_EQ(audioPin->getPipeline()->getParameters()->name, pipelineParams.name);

  //delete output pin from the pipeline
  setup->deleteAudioOutputPin(audioPin);

  //verify if output pin was successfully deleted from pipeline
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() == nullptr);

  //clean-up
  setup->destroyAudioPin(&audioPin);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_pipeline_output_pin_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //invalid case 1: add null audio pin to the pipeline
  IasTestFrameworkSetup::IasResult setup_result = setup->addAudioOutputPin(nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: delete null audio pin from the pipeline
  setup->deleteAudioOutputPin(nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_unlink_pipeline_input_pin_with_wave_file_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //pin has to be added to the pipeline before it can be linked with wave file
  setup_result = setup->addAudioInputPin(audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pin with input wave file
  setup_result = setup->linkWaveFile(audioPin, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //unlink input wave file from the pin
  setup->unlinkWaveFile(audioPin);

  //clean-up
  setup->deleteAudioInputPin(audioPin);
  setup->destroyAudioPin(&audioPin);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_automatic_unlink_pipeline_input_pin_with_wave_file_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //pin has to be added to the pipeline before it can be linked with wave file
  setup_result = setup->addAudioInputPin(audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pin with input wave file
  setup_result = setup->linkWaveFile(audioPin, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //clean-up
  setup->deleteAudioInputPin(audioPin);
  setup->destroyAudioPin(&audioPin);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_unlink_pipeline_output_pin_with_wave_file_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineOutputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //pin has to be added to the pipeline before it can be linked with wave file
  setup_result = setup->addAudioOutputPin(audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pin with output wave file
  setup_result = setup->linkWaveFile(audioPin, outputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //unlink output wave file from the pin
  setup->unlinkWaveFile(audioPin);

  //clean-up
  setup->deleteAudioOutputPin(audioPin);
  setup->destroyAudioPin(&audioPin);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_automatic_unlink_pipeline_output_pin_with_wave_file_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pin
  IasAudioPinPtr audioPin = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineOutputPinParams1, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //pin has to be added to the pipeline before it can be linked with wave file
  setup_result = setup->addAudioOutputPin(audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pin with output wave file
  setup_result = setup->linkWaveFile(audioPin, outputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //clean-up
  setup->deleteAudioOutputPin(audioPin);
  setup->destroyAudioPin(&audioPin);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_unlink_two_pipeline_input_pins_with_same_wave_file_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create audio pins
  IasAudioPinPtr inputPin1 = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, &inputPin1);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr inputPin2 = nullptr;
  setup_result = setup->createAudioPin(pipelineInputPinParams2, &inputPin2);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //pins have to be added to the pipeline before they can be linked with wave files
  setup_result = setup->addAudioInputPin(inputPin1);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->addAudioInputPin(inputPin2);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //it should be possible to link several input pins with the same wave file
  setup_result = setup->linkWaveFile(inputPin1, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(inputPin2, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //unlink wave file from audio pins
  setup->unlinkWaveFile(inputPin1);
  setup->unlinkWaveFile(inputPin2);

  //clean-up
  setup->deleteAudioInputPin(inputPin1);
  setup->deleteAudioInputPin(inputPin2);
  setup->destroyAudioPin(&inputPin1);
  setup->destroyAudioPin(&inputPin2);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_unlink_pipeline_pins_with_wave_file_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasTestFrameworkWaveFileParams inputWaveFileInvalidParams = {""};
  IasTestFrameworkWaveFileParams inputNonExistingWaveFileParams = {"Grummelbass_44100.wav"};
  IasAudioPinParams inputPinInvalidParams = {"PipelineInputPin2", 1}; //wave file, that will be linked to this pin, has 2 channels

  //create audio pins, inputPin2 will not have the same number of channels as its corresponding wave file
  IasAudioPinPtr inputPin1 = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createAudioPin(pipelineInputPinParams1, &inputPin1);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr inputPin2 = nullptr;
  setup_result = setup->createAudioPin(inputPinInvalidParams, &inputPin2);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr outputPin1 = nullptr;
  setup_result = setup->createAudioPin(pipelineOutputPinParams1, &outputPin1);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr outputPin2 = nullptr;
  setup_result = setup->createAudioPin(pipelineOutputPinParams2, &outputPin2);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //pins have to be added to the pipeline before they can be linked with wave files,
  //here we add all the pins except inputPin1
  setup_result = setup->addAudioInputPin(inputPin2);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  setup_result = setup->addAudioOutputPin(outputPin1);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  setup_result = setup->addAudioOutputPin(outputPin2);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 1: link wave file: pin is nullptr, valid wave params
  setup_result = setup->linkWaveFile(nullptr, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: link wave file: invalid wave params: file name is not specified
  setup_result = setup->linkWaveFile(inputPin2, inputWaveFileInvalidParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 3: link wave file: pin is not added to the pipeline, wave params are valid
  setup_result = setup->linkWaveFile(inputPin1, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //add inputPin1 to the pipeline to eliminate problem from invalid case 3
  setup_result = setup->addAudioInputPin(inputPin1);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 4: link wave file: input file does not exist
  setup_result = setup->linkWaveFile(inputPin1, inputNonExistingWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 5: link wave file: number of channels does not match in input pin (1 channel)
  //and wave file (2 channels)
  setup_result = setup->linkWaveFile(inputPin2, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 6: link wave file: link the same file to input and output pins
  setup_result = setup->linkWaveFile(inputPin1, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(outputPin1, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 7: link wave file: link the same file to two different output pins
  setup_result = setup->linkWaveFile(outputPin1, outputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(outputPin2, outputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 8: unlink wave file: unlink file from pins which were not linked yet
  setup->unlinkWaveFile(inputPin2);
  setup->unlinkWaveFile(outputPin2);

  //invalid case 9: unlink wave file: unlink file from pin which is null
  setup->unlinkWaveFile(nullptr);

  //clean-up
  setup->deleteAudioInputPin(inputPin1);
  setup->deleteAudioInputPin(inputPin2);
  setup->deleteAudioOutputPin(outputPin1);
  setup->deleteAudioOutputPin(outputPin2);
  setup->destroyAudioPin(&inputPin1);
  setup->destroyAudioPin(&inputPin2);
  setup->destroyAudioPin(&outputPin1);
  setup->destroyAudioPin(&outputPin2);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, create_and_destroy_processing_module_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create processing module
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  ASSERT_TRUE(module != nullptr);

  //set module properties
  IasProperties volumeProperties;
  volumeProperties.set("numFilterBands", 3);
  setup->setProperties(module, volumeProperties);

  //destroy processing module
  setup->destroyProcessingModule(&module);
  ASSERT_TRUE(module == nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, create_and_destroy_processing_module_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasProcessingModuleParams moduleInvalidParams;
  IasProcessingModulePtr module = nullptr;

  //invalid case 1: create processing module: module pointer is null
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: create processing module: type name is not specified
  setup_result = setup->createProcessingModule(moduleInvalidParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_TRUE(module == nullptr);

  //invalid case 3: create processing module: type name is specified but instance name is not specified
  moduleInvalidParams.typeName = "ias.volume";
  setup_result = setup->createProcessingModule(moduleInvalidParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_TRUE(module == nullptr);

  //invalid case 4: destroy processing module: module pointer is null
  setup->destroyProcessingModule(nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_processing_module_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create processing module
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add processing module to pipeline
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //verify that processing module was added successfully
  ASSERT_TRUE(module->getPipeline() != nullptr);
  ASSERT_EQ(module->getPipeline()->getParameters()->name, pipelineParams.name);

  //delete processing module from pipeline
  setup->deleteProcessingModule(module);

  //verify that processing module was deleted from pipeline successfully
  ASSERT_TRUE(module->getPipeline() == nullptr);

  //clean-up
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_processing_module_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //invalid case 1: add processing module: module pointer is null
  IasTestFrameworkSetup::IasResult setup_result = setup->addProcessingModule(nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: set module properties: module pointer is null
  IasProperties volumeProperties;
  volumeProperties.set("numFilterBands", 3);
  setup->setProperties(nullptr, volumeProperties);

  //invalid case 3: delete processing module: module pointer is null
  setup->deleteProcessingModule(nullptr);

  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_module_inout_pin_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create processing module and audio pin
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr audioPin = nullptr;
  setup_result = setup->createAudioPin(moduleInOutPinParams, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //processing module must be added to the pipeline before pin can be added
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add in/out pin to processing module
  setup_result = setup->addAudioInOutPin(module, audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //verify that audio pin was successfully added as module in/out pin
  ASSERT_EQ(IasAudioPin::eIasPinDirectionModuleInputOutput, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() != nullptr);
  ASSERT_EQ(audioPin->getPipeline()->getParameters()->name, pipelineParams.name);

  //delete in/out pin from processing module
  setup->deleteAudioInOutPin(module, audioPin);

  //verify that audio pin was successfully deleted from module
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() == nullptr);

  //clean-up
  setup->destroyAudioPin(&audioPin);
  setup->deleteProcessingModule(module);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_module_inout_pin_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create processing module and audio pin
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr audioPin = nullptr;
  setup_result = setup->createAudioPin(moduleInOutPinParams, &audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 1: add inout pin: processing module is nullptr
  setup_result = setup->addAudioInOutPin(nullptr, audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() == nullptr);

  //invalid case 2: add inout pin: audio pin is nullptr
  setup_result = setup->addAudioInOutPin(module, nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 3: add inout pin: processing module is not added to pipeline
  setup_result = setup->addAudioInOutPin(module, audioPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, audioPin->getDirection());
  ASSERT_TRUE(audioPin->getPipeline() == nullptr);

  //invalid case 4: delete inout pin: processing module is nullptr
  setup->deleteAudioInOutPin(nullptr, audioPin);

  //invalid case 5: delete inout pin: audio pin is nullptr
  setup->deleteAudioInOutPin(module, nullptr);

  //invalid case 6: delete inout pin: processing module is not added to pipeline
  setup->deleteAudioInOutPin(module, audioPin);

  //clean-up
  setup->destroyAudioPin(&audioPin);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_audio_pin_mapping_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create all necessary audio elements
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr moduleInputPin = nullptr;
  setup_result = setup->createAudioPin(moduleInputPinParams, &moduleInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr moduleOutputPin = nullptr;
  setup_result = setup->createAudioPin(moduleOutputPinParams, &moduleOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //module must be added to the pipeline before pin mapping can be added
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add pin mapping to the module
  setup_result = setup->addAudioPinMapping(module, moduleInputPin, moduleOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //verify that audio pin mapping was successfully added
  ASSERT_EQ(IasAudioPin::eIasPinDirectionModuleInput, moduleInputPin->getDirection());
  ASSERT_EQ(IasAudioPin::eIasPinDirectionModuleOutput, moduleOutputPin->getDirection());
  ASSERT_TRUE(moduleInputPin->getPipeline() != nullptr);
  ASSERT_TRUE(moduleOutputPin->getPipeline() != nullptr);
  ASSERT_EQ(moduleInputPin->getPipeline()->getParameters()->name, pipelineParams.name);
  ASSERT_EQ(moduleOutputPin->getPipeline()->getParameters()->name, pipelineParams.name);

  //delete pin mapping from processing module
  setup->deleteAudioPinMapping(module, moduleInputPin, moduleOutputPin);

  //verify that audio pin mapping was successfully deleted from the module
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleInputPin->getDirection());
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleOutputPin->getDirection());
  ASSERT_TRUE(moduleInputPin->getPipeline() == nullptr);
  ASSERT_TRUE(moduleOutputPin->getPipeline() == nullptr);

  //clean-up
  setup->destroyAudioPin(&moduleInputPin);
  setup->destroyAudioPin(&moduleOutputPin);
  setup->deleteProcessingModule(module);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, add_and_delete_audio_pin_mapping_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create all necessary audio elements
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr moduleInputPin = nullptr;
  setup_result = setup->createAudioPin(moduleInputPinParams, &moduleInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr moduleOutputPin = nullptr;
  setup_result = setup->createAudioPin(moduleOutputPinParams, &moduleOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 1: add pin mapping: processing module is nullptr
  setup_result = setup->addAudioPinMapping(nullptr, moduleInputPin, moduleOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleInputPin->getDirection());
  ASSERT_TRUE(moduleInputPin->getPipeline() == nullptr);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleOutputPin->getDirection());
  ASSERT_TRUE(moduleOutputPin->getPipeline() == nullptr);

  //invalid case 2: add pin mapping: module input pin is nullptr
  setup_result = setup->addAudioPinMapping(module, nullptr, moduleOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleOutputPin->getDirection());
  ASSERT_TRUE(moduleOutputPin->getPipeline() == nullptr);

  //invalid case 3: add pin mapping: module output pin is nullptr
  setup_result = setup->addAudioPinMapping(module, moduleInputPin, nullptr);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleInputPin->getDirection());
  ASSERT_TRUE(moduleInputPin->getPipeline() == nullptr);

  //invalid case 4: add pin mapping: processing module is not added to pipeline
  setup_result = setup->addAudioPinMapping(module, moduleInputPin, moduleOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleInputPin->getDirection());
  ASSERT_TRUE(moduleInputPin->getPipeline() == nullptr);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleOutputPin->getDirection());
  ASSERT_TRUE(moduleOutputPin->getPipeline() == nullptr);

  //invalid case 5: delete pin mapping: processing module is nullptr
  setup->deleteAudioPinMapping(nullptr, moduleInputPin, moduleOutputPin);

  //invalid case 6: delete pin mapping: module input pin is nullptr
  setup->deleteAudioPinMapping(module, nullptr, moduleOutputPin);

  //invalid case 7: delete pin mapping: module output pin is nullptr
  setup->deleteAudioPinMapping(module, moduleInputPin, nullptr);

  //invalid case 8: delete pin mapping: processing module is not added to pipeline
  setup->deleteAudioPinMapping(module, moduleInputPin, moduleOutputPin);

  //clean-up
  setup->destroyAudioPin(&moduleInputPin);
  setup->destroyAudioPin(&moduleOutputPin);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_unlink_pipeline_pins_with_module_pins_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create all necessary audio elements
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr moduleInOutPin = nullptr;
  setup_result = setup->createAudioPin(moduleInOutPinParams, &moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineInputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineInputPinParams1, &pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineOutputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineOutputPinParams1, &pipelineOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //processing module must be added to the pipeline before pin can be added
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add in/out pin to processing module
  setup_result = setup->addAudioInOutPin(module, moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add input and output pins to the pipeline
  setup_result = setup->addAudioInputPin(pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->addAudioOutputPin(pipelineOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pipeline input pin with module in/out pin
  setup_result = setup->link(pipelineInputPin, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link module in/out pin with pipeline output pin
  setup_result = setup->link(moduleInOutPin, pipelineOutputPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //unlink pipeline input pin from module in/out pin
  setup->unlink(pipelineInputPin, moduleInOutPin);

  //unlink module in/out pin from pipeline output pin
  setup->unlink(moduleInOutPin, pipelineOutputPin);

  //clean-up
  setup->deleteAudioInOutPin(module, moduleInOutPin);
  setup->deleteProcessingModule(module);
  setup->deleteAudioInputPin(pipelineInputPin);
  setup->deleteAudioOutputPin(pipelineOutputPin);
  setup->destroyAudioPin(&moduleInOutPin);
  setup->destroyAudioPin(&pipelineInputPin);
  setup->destroyAudioPin(&pipelineOutputPin);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, link_and_unlink_pipeline_pins_with_module_pins_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasAudioPinParams moduleInOutPinInvalidParams = {"ModuleInOutPin", 3}; //number of channels does not match

  //create all necessary audio elements
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr moduleInOutPin = nullptr;
  setup_result = setup->createAudioPin(moduleInOutPinInvalidParams, &moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineInputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineInputPinParams1, &pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //processing module must be added to the pipeline before pin can be added
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 1: link: pipelineInputPin is null
  setup_result = setup->link(nullptr, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 2: link: moduleInOutPin is null
  setup_result = setup->link(pipelineInputPin, nullptr, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 3: link: pipelineInputPin is not added to the pipeline
  setup_result = setup->link(pipelineInputPin, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //add input pin to the pipeline to eliminate problem from invalid case 3
  setup_result = setup->addAudioInputPin(pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 4: link: pipelineInputPin is added to the pipeline but moduleInOutPin is not added
  setup_result = setup->link(pipelineInputPin, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //add in/out pin to processing module to eliminate problem from invalid case 4
  setup_result = setup->addAudioInOutPin(module, moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 5: link: pins have different number of channels
  setup_result = setup->link(pipelineInputPin, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasFailed, setup_result);

  //invalid case 6: unlink: pipelineInputPin is null
  setup->unlink(nullptr, moduleInOutPin);

  //invalid case 7: unlink: moduleInOutPin is null
  setup->unlink(pipelineInputPin, nullptr);

  //delete in/out pin from processing module
  setup->deleteAudioInOutPin(module, moduleInOutPin);
  ASSERT_EQ(IasAudioPin::eIasPinDirectionUndef, moduleInOutPin->getDirection());

  //invalid case 8: unlink: moduleInOutPin was removed from the pipeline
  setup->unlink(pipelineInputPin, moduleInOutPin);

  //delete input pin from the pipeline
  setup->deleteAudioInputPin(pipelineInputPin);

  //invalid case 9: unlink: pipelineInputPin was removed from the pipeline
  setup->unlink(pipelineInputPin, moduleInOutPin);

  //clean-up
  setup->deleteProcessingModule(module);
  setup->destroyAudioPin(&moduleInOutPin);
  setup->destroyAudioPin(&pipelineInputPin);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, start_process_and_stop_test_framework_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create all necessary audio elements
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasProperties volumeProperties;
  volumeProperties.set("numFilterBands", 3);
  setup->setProperties(module, volumeProperties);

  IasAudioPinPtr moduleInOutPin = nullptr;
  setup_result = setup->createAudioPin(moduleInOutPinParams, &moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineInputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineInputPinParams1, &pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineOutputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineOutputPinParams1, &pipelineOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add input pin to the pipeline and link it with wave file
  setup_result = setup->addAudioInputPin(pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(pipelineInputPin, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add output pin to the pipeline and link it with wave file
  setup_result = setup->addAudioOutputPin(pipelineOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(pipelineOutputPin, outputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add processing module to the pipeline and add in/out pin
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->addAudioInOutPin(module, moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pipeline input and output pins with module in/out pin
  setup_result = setup->link(pipelineInputPin, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->link(moduleInOutPin, pipelineOutputPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //start test framework
  IasTestFramework::IasResult testfwx_result = testfwx->start();
  ASSERT_EQ(IasTestFramework::eIasOk, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasOk");

  //process one period of data
  testfwx_result = testfwx->process(1);
  ASSERT_EQ(IasTestFramework::eIasOk, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasOk");

  //stop test framework
  testfwx_result = testfwx->stop();
  ASSERT_EQ(IasTestFramework::eIasOk, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasOk");

  //clean-up
  setup->deleteAudioInOutPin(module, moduleInOutPin);
  setup->deleteProcessingModule(module);
  setup->deleteAudioInputPin(pipelineInputPin);
  setup->deleteAudioOutputPin(pipelineOutputPin);
  setup->destroyAudioPin(&moduleInOutPin);
  setup->destroyAudioPin(&pipelineInputPin);
  setup->destroyAudioPin(&pipelineOutputPin);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}


TEST_F(IasTestFrameworkTest, start_process_and_stop_test_framework_invalid_cases_test)
{
  IasTestFramework *testfwx = IasTestFramework::create(pipelineParams);
  ASSERT_TRUE(testfwx != nullptr);
  IasTestFrameworkSetup *setup = testfwx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create all necessary audio elements
  IasProcessingModulePtr module = nullptr;
  IasTestFrameworkSetup::IasResult setup_result = setup->createProcessingModule(moduleParams, &module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasProperties volumeProperties;
  volumeProperties.set("numFilterBands", 3);
  setup->setProperties(module, volumeProperties);

  IasAudioPinPtr moduleInOutPin = nullptr;
  setup_result = setup->createAudioPin(moduleInOutPinParams, &moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineInputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineInputPinParams1, &pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  IasAudioPinPtr pipelineOutputPin = nullptr;
  setup_result = setup->createAudioPin(pipelineOutputPinParams1, &pipelineOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add input pin to the pipeline and link it with wave file
  setup_result = setup->addAudioInputPin(pipelineInputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(pipelineInputPin, inputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add output pin to the pipeline and link it with wave file
  setup_result = setup->addAudioOutputPin(pipelineOutputPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->linkWaveFile(pipelineOutputPin, outputWaveFileParams);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //add processing module to the pipeline and add in/out pin
  setup_result = setup->addProcessingModule(module);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->addAudioInOutPin(module, moduleInOutPin);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //link pipeline input and output pins with module in/out pin
  setup_result = setup->link(pipelineInputPin, moduleInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);
  setup_result = setup->link(moduleInOutPin, pipelineOutputPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasTestFrameworkSetup::eIasOk, setup_result);

  //invalid case 1: start the test framework with problems to open wave file for reading

  //rename input wave file first
  rename(inputWaveFileParams.fileName.c_str(), "InvalidFileName.wav");
  
  //start the test framework should fail now
  IasTestFramework::IasResult testfwx_result = testfwx->start();
  ASSERT_EQ(IasTestFramework::eIasFailed, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasFailed");

  //bring back valid name for input wave file
  rename("InvalidFileName.wav", inputWaveFileParams.fileName.c_str());

  //invalid case 2: process one period of data when test framework was not started properly
  testfwx_result = testfwx->process(1);
  ASSERT_EQ(IasTestFramework::eIasFailed, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasFailed");

  //invalid case 3: process zero periods of data
  testfwx_result = testfwx->process(0);
  ASSERT_EQ(IasTestFramework::eIasFailed, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasFailed");

  //stop test framework when test framework was not started properly should return eIasOk
  testfwx_result = testfwx->stop();
  ASSERT_EQ(IasTestFramework::eIasOk, testfwx_result);
  ASSERT_EQ(toString(testfwx_result), "IasTestFramework::eIasOk");

  //clean-up
  setup->deleteAudioInOutPin(module, moduleInOutPin);
  setup->deleteProcessingModule(module);
  setup->deleteAudioInputPin(pipelineInputPin);
  setup->deleteAudioOutputPin(pipelineOutputPin);
  setup->destroyAudioPin(&moduleInOutPin);
  setup->destroyAudioPin(&pipelineInputPin);
  setup->destroyAudioPin(&pipelineOutputPin);
  setup->destroyProcessingModule(&module);
  IasTestFramework::destroy(testfwx);
}

} //namespace IasAudio
