/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created on: August 2016
 */
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>

extern "C"  {
#include <sndfile.h>
}

#include <gtest/gtest.h>

#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "mixer/IasMixerCore.hpp"
#include "audio/mixerx/IasMixerCmd.hpp"

#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"


using namespace IasAudio;

static std::atomic<bool> isBalanceEventFinished {false};
static std::atomic<bool> isFaderEventFinished {false};
static std::atomic<bool> isInputGainEventFinished {false};

int32_t eventBalance = -1440;
int32_t eventFader   = -1440;
int32_t eventGain = -1440;

class EventHandler : public IasEventHandler
{

  void receivedModuleEvent(IasModuleEvent* event) override
  {
    const IasProperties eventProperties = event->getProperties();
    std::string typeName = "";
    std::string instanceName = "";
    int32_t eventType;
    eventProperties.get<std::string>("typeName", &typeName);
    eventProperties.get<std::string>("instanceName", &instanceName);
    eventProperties.get<int32_t>("eventType", &eventType);

    if (typeName == "ias.mixer")
    {
      std::string  pinName;
      IasProperties::IasResult result;

      result = eventProperties.get<std::string>("pin", &pinName);
      ASSERT_EQ(IasProperties::IasResult::eIasOk, result);

      switch (eventType)
      {
        case static_cast<int32_t>(IasMixer::IasMixerEventTypes::eIasBalanceFinished):

          eventProperties.get<int32_t>("balance", &eventBalance);
          isBalanceEventFinished.store(true);
          break;

        case static_cast<int32_t>(IasMixer::IasMixerEventTypes::eIasFaderFinished):

          eventProperties.get<int32_t>("fader", &eventFader);
          isFaderEventFinished.store(true);
          break;

        case static_cast<int32_t>(IasMixer::IasMixerEventTypes::eIasInputGainOffsetFinished):

          eventProperties.get<int32_t>("gainOffset", &eventGain);
          isInputGainEventFinished.store(true);
          break;

        default:
          std::cout << "Invalid event type" << std::endl;
      }
    }
  }
};


static const uint32_t cIasFrameLength { 64 };
static const uint32_t cIasSampleRate  { 48000 };


int32_t main(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;
  DLT_REGISTER_APP("TST", "Test Application");
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}

namespace IasAudio {

class IasMixerEventTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();

    IasIModuleId::IasResult setBalance(const char* pin, int32_t  balance);
    IasIModuleId::IasResult setFader(const char* pin, int32_t  balance);
    IasIModuleId::IasResult setInputGainOffset(const char* pin, int32_t gain);

    void receiveEventHandler(const uint32_t timeout);

    IasIModuleId* mCmd = nullptr;
};

void IasMixerEventTest::SetUp()
{
}

void IasMixerEventTest::TearDown()
{
}

IasIModuleId::IasResult IasMixerEventTest::setBalance(const char* pin, int32_t balance)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetBalance);
  cmdProperties.set<int32_t>("balance", balance);
  return mCmd->processCmd(cmdProperties, returnProperties);

}

IasIModuleId::IasResult IasMixerEventTest::setFader(const char* pin, int32_t fader)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetFader);
  cmdProperties.set<int32_t>("fader", fader);
  return mCmd->processCmd(cmdProperties, returnProperties);
}

IasIModuleId::IasResult IasMixerEventTest::setInputGainOffset(const char* pin, int32_t gain)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin", std::string{pin});
  cmdProperties.set<int32_t>("cmd", IasMixer::IasMixerCmdIds::eIasSetInputGainOffset);
  cmdProperties.set<int32_t>("gain", gain);
  return mCmd->processCmd(cmdProperties, returnProperties);
}


TEST_F(IasMixerEventTest, main_test)
{
  auto receiveEventHandler = [](const uint32_t timeout)
  {
    auto* smartx = IasSmartX::create();
    ASSERT_TRUE(nullptr != smartx);

    while (smartx->waitForEvent(timeout) == IasSmartX::eIasOk)
    {
      IasEventPtr newEvent;
      const auto res = smartx->getNextEvent(&newEvent);
      ASSERT_EQ(IasSmartX::eIasOk, res);

      EventHandler event;
      newEvent->accept(event);
    }

    const auto res = smartx->stop();
    ASSERT_EQ(IasSmartX::eIasOk, res);

    IasSmartX::destroy(smartx);
  };

  IasAudioChain* myAudioChain = new IasAudioChain();
  ASSERT_TRUE(myAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  IasAudioChain::IasResult chainres = myAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);
  auto* hSink0 = myAudioChain->createInputAudioStream("Sink0", 0, 2, false);
  auto* hSpeaker = myAudioChain->createOutputAudioStream("Speaker", 0, 4,false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  IasCmdDispatcherPtr cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  IasPluginEngine pluginEngine(cmdDispatcher);
  IasAudioProcessingResult procres = pluginEngine.loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  IasIGenericAudioCompConfig* config = nullptr;
  procres = pluginEngine.createModuleConfig(&config);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != config);

  config->addStreamToProcess(hSink0, "Sink0");
  config->addStreamToProcess(hSpeaker, "Speaker");

  config->addStreamMapping(hSink0, "Sink0", hSpeaker, "Speaker");

  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Speaker");

  IasGenericAudioComp* myMixer;
  pluginEngine.createModule(config, "ias.mixer", "MyMixer", &myMixer);
  myAudioChain->addAudioComponent(myMixer);

  auto* mixerCore = myMixer->getCore();
  ASSERT_TRUE(nullptr != mixerCore);

  mCmd = myMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != mCmd);

  mixerCore->enableProcessing();

  const uint32_t timeout_ms {500};

  int32_t testBalance = -1440;
  int32_t testFader = -1440;

  std::thread eventHandler(receiveEventHandler, timeout_ms);

  while(testBalance <= 1440)
  {
    setBalance("Sink0", testBalance);
    setFader("Sink0", testFader);
    eventBalance = -4444; // set these values to some wrong values, so that test will fail if event does not come back with correct values
    eventFader = -4444;
    while (isBalanceEventFinished.load() == false || isFaderEventFinished.load() == false)
    {
      mixerCore->process();
    };
    ASSERT_TRUE(testBalance == eventBalance);
    ASSERT_TRUE(testFader == eventFader);

    isBalanceEventFinished.store(false);
    isFaderEventFinished.store(false);
    testBalance ++;
    testFader ++;
  }

  int32_t testGain = -200;

  //check invalid parameters;
  EXPECT_EQ(IasIModuleId::eIasFailed, setInputGainOffset("Sink0",-201));
  EXPECT_EQ(IasIModuleId::eIasFailed, setInputGainOffset("Sink0",201));

  while(testGain < 201)
  {
    setInputGainOffset("Sink0", testGain);
    eventGain = -1440;

    while(isInputGainEventFinished.load() ==false)
    {
      mixerCore->process();
    };

    ASSERT_TRUE(testGain == eventGain);
    isInputGainEventFinished.store(false);
    testGain++;
  }
  eventHandler.join();
}


} // namespace IasAudio
