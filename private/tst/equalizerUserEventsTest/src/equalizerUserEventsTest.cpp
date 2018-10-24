/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * equalizerUserEventsTest.cpp
 *
 *  Created on: February 2017
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
#include "equalizer/IasEqualizerCore.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"

#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"


using namespace IasAudio;

static const uint32_t cIasFrameLength { 64 };
static const uint32_t cIasSampleRate  { 48000 };

static const int32_t eventGainWrongValue = -1440;
static const int32_t startGainValue = -200;

static std::atomic<bool> isInputGainEventFinished {false};
static std::atomic<int32_t> eventGain {eventGainWrongValue};

struct EqualizerGain
{
  std::string stream;
  int32_t  filterId;
  int32_t  gain;
};

struct EqualizerParams
{
  std::string stream;
  int32_t  filterId;
  int32_t  freq;
  int32_t  quality;
  int32_t  type;
  int32_t  order;
};


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

    if (typeName == "ias.equalizer")
    {
      std::string  pinName;
      IasProperties::IasResult result;

      result = eventProperties.get<std::string>("pin", &pinName);
      ASSERT_EQ(IasProperties::IasResult::eIasOk, result);

      int32_t tmpEventGain;

      switch (eventType)
      {
        case static_cast<int32_t>(IasEqualizer::IasEqualizerEventTypes::eIasGainRampingFinished):
          tmpEventGain = eventGainWrongValue;
          eventProperties.get<int32_t>("gain", &tmpEventGain);
          eventGain.store(tmpEventGain);
          isInputGainEventFinished.store(true);
          break;

        default:
          std::cout << "Invalid event type" << std::endl;
      }
    }
  }
};


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

class IasEqualizerUserEventTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();

    IasIModuleId::IasResult setEqualizer(const EqualizerGain params);
    IasIModuleId::IasResult setEqualizerParams(const EqualizerParams params);

    void receiveEventHandler(const uint32_t timeout);

    IasIModuleId* mCmd = nullptr;
};

void IasEqualizerUserEventTest::SetUp()
{
}

void IasEqualizerUserEventTest::TearDown()
{
}

IasIModuleId::IasResult IasEqualizerUserEventTest::setEqualizer(const EqualizerGain params)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     params.stream);
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<int32_t>("filterId", params.filterId);
  cmdProperties.set<int32_t>("gain",     params.gain);

  return mCmd->processCmd(cmdProperties, returnProperties);
}

IasIModuleId::IasResult IasEqualizerUserEventTest::setEqualizerParams(const EqualizerParams params)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     params.stream);
  cmdProperties.set<int32_t>("cmd",
       IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams);
  cmdProperties.set<int32_t>("filterId", params.filterId);
  cmdProperties.set<int32_t>("freq",     params.freq);
  cmdProperties.set<int32_t>("quality",  params.quality);
  cmdProperties.set<int32_t>("type",     params.type);
  cmdProperties.set<int32_t>("order",    params.order);

  return mCmd->processCmd(cmdProperties, returnProperties);
}


TEST_F(IasEqualizerUserEventTest, main_test)
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

  IasProperties properties;
  properties.set("EqualizerMode",
          static_cast<int32_t>(IasEqualizer::IasEqualizerMode::eIasUser));

  properties.set("numFilterStagesMax", 3);

  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Speaker");

  config->setProperties(properties);

  IasGenericAudioComp* myEqualizer = nullptr;
  pluginEngine.createModule(config, "ias.equalizer", "MyEqualizer", &myEqualizer);
  myAudioChain->addAudioComponent(myEqualizer);

  auto* equalizerCore = myEqualizer->getCore();
  ASSERT_TRUE(nullptr != myEqualizer);

  mCmd = myEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != mCmd);

  equalizerCore->enableProcessing();

  const uint32_t timeout_ms {2000};

  EqualizerParams paramsData;
  paramsData.stream   = "Sink0";  // stream
  paramsData.filterId = 1;        // filter band
  paramsData.freq     = 50;       // frequency = 50 Hz
  paramsData.quality  = 10;       // quality = 1.0
  paramsData.type     = static_cast<int32_t>(eIasFilterTypeLowShelving); // type
  paramsData.order    = 2;

  EqualizerGain gainData;
  gainData.stream   = paramsData.stream;
  gainData.filterId = paramsData.filterId;
  gainData.gain     = startGainValue;

  //check valid, invalid parameters;
  EXPECT_EQ(IasIModuleId::eIasOk, setEqualizerParams(paramsData));

  gainData.gain     = eventGainWrongValue;
  EXPECT_EQ(IasIModuleId::eIasFailed, setEqualizer(gainData));

  gainData.gain     = startGainValue;
  EXPECT_EQ(IasIModuleId::eIasOk, setEqualizer(gainData));

  std::thread eventHandler(receiveEventHandler, timeout_ms);

  while(gainData.gain < 201)
  {
    setEqualizer(gainData);
    eventGain.store(eventGainWrongValue);

    while(isInputGainEventFinished.load() == false)
    {
      equalizerCore->process();
    };

    ASSERT_EQ(eventGain.load(), gainData.gain);
    isInputGainEventFinished.store(false);
    gainData.gain++;
  }
  eventHandler.join();

}


} // namespace IasAudio
