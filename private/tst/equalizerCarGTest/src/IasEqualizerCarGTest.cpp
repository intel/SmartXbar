/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasEqualizerCarGTest.cpp
 *
 *  Created on: December 2016
 */



#include <cstdio>
#include <iostream>
#include <utility>
#include <functional>

#include <gtest/gtest.h>
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"
#include "filter/IasAudioFilter.hpp"

#include "IasEqualizerCarGTest.hpp"

static const uint32_t cNumChannelsStream1 { 6 };
static const uint32_t cNumChannelsStream2 { 2 };
static const uint32_t cIasFrameLength     { 64 };
static const uint32_t cIasSampleRate      { 44100 };
static const int32_t  cNumFilterStagesMax { 10 };

namespace max
{
  static const int32_t   freq    { 9999999 };  // freq, invalid, use the Nyquist rate instead!
  static const float gain    { 1000.0f };  // gain (linear, not in dB)
  static const float quality { 100.0f };   // quality
  static const int32_t   order   { 20 };       // order
  static const int32_t   section { 10 };       // section, must not be greater than order/2
};

namespace min
{
  static const int32_t   freq    { 10 };      // freq
  static const float gain    { 0.001f };  // gain (linear, not in dB)
  static const float quality { 0.01f };   // quality
  static const int32_t   order   { 1 };       // order
  static const int32_t   section { 1 };       // section
};

namespace IasAudio
{

using IasResult = IasIModuleId::IasResult;

#define IASTESTLOG(FMTSTR,...) printf("equalizerCarGTest" FMTSTR,##__VA_ARGS__)


void IasEqualizerCarGTest::SetUp()
{
  mAudioChain = new (std::nothrow) IasAudioChain();
  ASSERT_TRUE(mAudioChain != nullptr);

  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  const auto chainres = mAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  mInStream1  = mAudioChain->createInputAudioStream("Sink1", 1, cNumChannelsStream1, false);
  mInStream2  = mAudioChain->createInputAudioStream("Sink2", 2, cNumChannelsStream2, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  mCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  ASSERT_TRUE(mCmdDispatcher != nullptr);

  mPluginEngine = new (std::nothrow) IasPluginEngine(mCmdDispatcher);
  ASSERT_TRUE(mPluginEngine != nullptr);

  auto procres = mPluginEngine->loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  procres = mPluginEngine->createModuleConfig(&mEqualizerConfig);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != mEqualizerConfig);

  IasProperties properties;
  mEqualizerConfig->addStreamToProcess(mInStream1, "Sink1");
  mEqualizerConfig->addStreamToProcess(mInStream2, "Sink2");

  properties.set("EqualizerMode",
                 static_cast<int32_t>(IasEqualizer::IasEqualizerMode::eIasCar));

  properties.set("numFilterStagesMax", cNumFilterStagesMax);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(mEqualizer != nullptr);
}


void IasEqualizerCarGTest::TearDown()
{
  delete mAudioChain;
  mPluginEngine->destroyModule(mEqualizer);
  delete mPluginEngine;
}


IasIModuleId::IasResult IasEqualizerCarGTest::setCarEqualizerNumFilters(const char* pin,
    const SetCarEqualizerNumFiltersParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",       std::string{pin});
  cmdProperties.set<int32_t>("cmd",        params.cmd);
  cmdProperties.set<int32_t>("channelIdx", params.channelIdx);
  cmdProperties.set<int32_t>("numFilters", params.numFilters);

  return cmd->processCmd(cmdProperties, returnProperties);
}

std::pair<IasIModuleId::IasResult, int32_t> IasEqualizerCarGTest::getCarEqualizerNumFilters(const char* pin,
    GetCarEqualizerNumFiltersParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",       std::string{pin});
  cmdProperties.set<int32_t>("cmd",        params.cmd);
  cmdProperties.set<int32_t>("channelIdx", params.channelIdx);

  IasIModuleId::IasResult err = cmd->processCmd(cmdProperties, returnProperties);

  int32_t numFilters = -1;
  returnProperties.get("numFilters", &numFilters);

  return std::make_pair(err, numFilters);
}

std::pair<IasIModuleId::IasResult, AudioFilterParamsData>
    IasEqualizerCarGTest::getCarEqualizerFilterParams(const char* pin,
    GetCarEqualizerFilterParamsParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",       std::string{pin});
  cmdProperties.set<int32_t>("cmd",        params.cmd);
  cmdProperties.set<int32_t>("channelIdx", params.channelIdx);
  cmdProperties.set<int32_t>("filterId",   params.filterId);

  const IasIModuleId::IasResult err = cmd->processCmd(cmdProperties, returnProperties);

  AudioFilterParamsData filterParams;

  filterParams.freq = -1;
  filterParams.gain = -1;
  filterParams.quality = -1;
  filterParams.type = -1;
  filterParams.order = -1;

  returnProperties.get("freq",    &filterParams.freq);
  returnProperties.get("gain",    &filterParams.gain);
  returnProperties.get("quality", &filterParams.quality);
  returnProperties.get("type",    &filterParams.type);
  returnProperties.get("order",   &filterParams.order);

  return std::make_pair(err, filterParams);
}

IasIModuleId::IasResult IasEqualizerCarGTest::setCarEqualizerFilterParams(const char* pin,
   SetCarEqualizerFilterParamsParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",       std::string{pin});
  cmdProperties.set<int32_t>("cmd",        params.cmd);
  cmdProperties.set<int32_t>("channelIdx", params.channelIdx);
  cmdProperties.set<int32_t>("filterId",   params.filterId);
  cmdProperties.set<int32_t>("freq",       params.freq);
  cmdProperties.set<int32_t>("gain",       params.gain);
  cmdProperties.set<int32_t>("quality",    params.quality);
  cmdProperties.set<int32_t>("type",       params.type);
  cmdProperties.set<int32_t>("order",      params.order);

  return cmd->processCmd(cmdProperties, returnProperties);
}


TEST_F(IasEqualizerCarGTest, equalizerCreation)
{
  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizerr creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
}


TEST_F(IasEqualizerCarGTest, CarEqualizerConfig)
{
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetCarEqualizerNumFilters = [&](const char* pin,
      SetCarEqualizerNumFiltersParams params)
  {
    return setCarEqualizerNumFilters(pin, params, cmdInterface);
  };

  const auto cmdGetCarEqualizerNumFilters = [&](const char* pin,
      GetCarEqualizerNumFiltersParams params)
  {
    return getCarEqualizerNumFilters(pin, params, cmdInterface);
  };

  const auto cmdSetCarEqualizerFilterParams = [&](const char* pin,
      SetCarEqualizerFilterParamsParams params)
  {
    return setCarEqualizerFilterParams(pin, params, cmdInterface);
  };

  const auto cmdGetCarEqualizerFilterParams = [&](const char* pin,
      GetCarEqualizerFilterParamsParams params)
  {
    return getCarEqualizerFilterParams(pin, params, cmdInterface);
  };

  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("Sink1",       {6, 5}) );
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("Sink1",       {0, 20}) );
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("",            {0, 5}) );
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("dummyStream", {0, 5}) );
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("Sink1",       {200, 20}) );
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("Sink1",       {0, 200}) );


  { // wrong pin
  IasProperties cmdPropertiesPin;
  IasProperties returnPropertiesPin;
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdPropertiesPin, returnPropertiesPin));
  }

  { // wrong cmd
  IasProperties cmdPropertiesPin;
  IasProperties returnPropertiesPin;
  cmdPropertiesPin.set<Ias::String>("pin",       "Sink1");
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdPropertiesPin, returnPropertiesPin));
  }

  { // wrong channelIdx
  IasProperties cmdPropertiesPin;
  IasProperties returnPropertiesPin;
  cmdPropertiesPin.set<Ias::String>("pin",       "Sink1");
  cmdPropertiesPin.set<Ias::Int32>("cmd",static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerNumFilters));
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdPropertiesPin, returnPropertiesPin));

  }

  { // wrong num filters
  IasProperties cmdPropertiesPin;
  IasProperties returnPropertiesPin;
  cmdPropertiesPin.set<Ias::String>("pin",       "Sink1");
  cmdPropertiesPin.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerNumFilters));
  cmdPropertiesPin.set<Ias::Int32>("channelIdx", {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdPropertiesPin, returnPropertiesPin));
  }

  { // wrong channel Idx
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong filterId
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong freq
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  cmdProperties.set<Ias::Int32>("filterId",   {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong gain
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  cmdProperties.set<Ias::Int32>("filterId",   {});
  cmdProperties.set<Ias::Int32>("freq",       {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong quality
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  cmdProperties.set<Ias::Int32>("filterId",   {});
  cmdProperties.set<Ias::Int32>("freq",       {});
  cmdProperties.set<Ias::Int32>("gain",       {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong type
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  cmdProperties.set<Ias::Int32>("filterId",   {});
  cmdProperties.set<Ias::Int32>("freq",       {});
  cmdProperties.set<Ias::Int32>("gain",       {});
  cmdProperties.set<Ias::Int32>("quality",    {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong order
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  cmdProperties.set<Ias::Int32>("filterId",   {});
  cmdProperties.set<Ias::Int32>("freq",       {});
  cmdProperties.set<Ias::Int32>("gain",       {});
  cmdProperties.set<Ias::Int32>("quality",    {});
  cmdProperties.set<Ias::Int32>("type",       {});
  EXPECT_EQ(IasResult::eIasFailed,  cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong pin
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerNumFilters));
  cmdProperties.set<Ias::Int32>("channelIdx", {});

  EXPECT_EQ(IasResult::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong channel
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerNumFilters));

  EXPECT_EQ(IasResult::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong pin
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerFilterParams));
  EXPECT_EQ(IasResult::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong channel
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerFilterParams));
  EXPECT_EQ(IasResult::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  { // wrong filter
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",       "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerFilterParams));
  cmdProperties.set<Ias::Int32>("channelIdx", {});
  EXPECT_EQ(IasResult::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }


  EXPECT_EQ(IasResult::eIasFailed, cmdGetCarEqualizerFilterParams("Sink1",       {9, 0 }).first);
  EXPECT_EQ(IasResult::eIasFailed, cmdGetCarEqualizerFilterParams("Sink1",       {0, 20}).first );
  EXPECT_EQ(IasResult::eIasFailed, cmdGetCarEqualizerFilterParams("",            {0, 5}).first );
  EXPECT_EQ(IasResult::eIasFailed, cmdGetCarEqualizerFilterParams("dummyStream", {0, 5}).first );

  IASTESTLOG(".%s:\n Compare if set and get number filters data are the same\n",this->test_info_->name());
  const std::array<int32_t , cNumChannelsStream1> numFiltersStream1 = {6, 5, 4, 3, 2, 1};

  for(uint32_t i=0; i< numFiltersStream1.size(); i++)
  {
    EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink1", {static_cast<int32_t>(i), numFiltersStream1[i]}));

    const auto res = cmdGetCarEqualizerNumFilters("Sink1", {static_cast<int32_t>(i)});

    EXPECT_EQ(IasResult::eIasOk,      res.first);
    EXPECT_NE(numFiltersStream1[i]+1, res.second);
    EXPECT_NE(numFiltersStream1[i]-1, res.second);
  }

  const int32_t numFilter0 = 7, numFilter1 = 8;
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink2", {0, numFilter0}));
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink2", {1, numFilter1}));

  EXPECT_EQ(IasResult::eIasOk, cmdGetCarEqualizerNumFilters("Sink2", {0}).first);
  EXPECT_EQ(numFilter0,        cmdGetCarEqualizerNumFilters("Sink2", {0}).second);

  EXPECT_EQ(IasResult::eIasOk, cmdGetCarEqualizerNumFilters("Sink2", {1}).first);
  EXPECT_EQ(numFilter1,        cmdGetCarEqualizerNumFilters("Sink2", {1}).second);

  IASTESTLOG(".%s:\n Compare if set and get filters params data are the same\n",this->test_info_->name());
  SetCarEqualizerFilterParamsParams
  param { {}, {}, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 };

  param.channelIdx = 0;
  param.filterId = 1;
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", param));

  auto returnedParam = cmdGetCarEqualizerFilterParams("Sink1", {param.channelIdx, param.filterId });
  EXPECT_EQ(IasResult::eIasOk, returnedParam.first);

  EXPECT_EQ(param.freq,    returnedParam.second.freq);
  EXPECT_EQ(param.gain,    returnedParam.second.gain);
  EXPECT_EQ(param.type,    returnedParam.second.type);
  EXPECT_EQ(param.quality, returnedParam.second.quality);
  EXPECT_EQ(param.order,   returnedParam.second.order);

  param.channelIdx = 1;
  param.filterId = 0;
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", param));

  returnedParam = cmdGetCarEqualizerFilterParams("Sink1", {param.channelIdx, param.filterId });
  EXPECT_EQ(IasResult::eIasOk, returnedParam.first);
}


TEST_F(IasEqualizerCarGTest, IasEqualizerCmdSetWrongCommand)
{
  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);

  const auto userModeSetEqualizer = [&]()
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<std::string>("pin", std::string{"Input0"});
    cmdProperties.set<int32_t>("cmd",
         IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  const auto userModeSetEqualizerParams = [&]()
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<std::string>("pin", std::string{"Sink0"});
    cmdProperties.set<int32_t>("cmd",
         IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  ASSERT_EQ(IasIModuleId::eIasFailed, userModeSetEqualizerParams());
  ASSERT_EQ(IasIModuleId::eIasFailed, userModeSetEqualizer());
}


TEST_F(IasEqualizerCarGTest, CarEqualizerCmdInterfaceCommands)
{
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetCarEqualizerNumFilters = [&](const char* pin,
      SetCarEqualizerNumFiltersParams params)
  {
    return setCarEqualizerNumFilters(pin, params, cmdInterface);
  };

  const auto cmdSetCarEqualizerFilterParams = [&](const char* pin,
     SetCarEqualizerFilterParamsParams params)
  {
    return setCarEqualizerFilterParams(pin, params, cmdInterface);
  };

  const auto cmdGetCarEqualizerFilterParams = [&](const char* pin,
      GetCarEqualizerFilterParamsParams params)
  {
    return getCarEqualizerFilterParams(pin, params, cmdInterface);
  };

  const auto cmdProcessSetState = [&](int32_t cmdId,
      std::string&& cmd, std::string&& state)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    cmdProperties.set<int32_t>("cmd", cmdId);
    cmdProperties.set<std::string>(cmd, state);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  IASTESTLOG(".%s:\n Testing reaction for invalid commandId...\n",this->test_info_->name());
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<std::string>("pin", "Sink1");
    cmdProperties.set<int32_t>("cmd", 123456);
    cmdInterface->processCmd(cmdProperties, returnProperties);
  }

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerNumFilters...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("InvalidSink", {1, cNumFilterStagesMax}));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerNumFilters...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerNumFilters("Sink1", {1, cNumFilterStagesMax + 1}));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eIasCarModeSetEqualizerNumFilters...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink1", {1, cNumFilterStagesMax - 2}));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eIasCarModeSetEqualizerNumFilters...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink1", {0, 0}));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerFilterParams (invalid streamId)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("InvalidSink", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eCommandSetCarEqualizerFilterParams (filterIdx too high)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 1, (cNumFilterStagesMax-2), 1000, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerFilterParams (order too high)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighpass), max::order+1 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerFilterParams (order too low)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighpass), min::order-1 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerFilterParams (invalid order)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeBandpass), min::order-1 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eIasCarModeSetEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeBandpass), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eIasCarModeSetEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerFilterParams (frequency too high)...\n",this->test_info_->name());

  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink1", {0, 2}));

  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 0, 0, max::freq + 1, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 }));
  EXPECT_EQ(IasResult::eIasOk, cmdGetCarEqualizerFilterParams("Sink1", {0, 0 }).first);

  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eIasCarModeSetEqualizerFilterParams (frequency max range value)...\n",this->test_info_->name());

  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerNumFilters("Sink1", {0, 2}));

  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 0, 0, max::freq, 120, 10, static_cast<int32_t>(eIasFilterTypePeak), 2 }));
  EXPECT_EQ(IasResult::eIasOk, cmdGetCarEqualizerFilterParams("Sink1", {0, 0 }).first);

  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, -120, 10, static_cast<int32_t>(eIasFilterTypeLowShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eCommandSetCarEqualizerFilterParams (too many stages due to high order)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighpass), 7 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighpass), 6 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, -120, 10, static_cast<int32_t>(eIasFilterTypeLowShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeLowShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, -120, 10, static_cast<int32_t>(eIasFilterTypeHighShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, -12, 10, static_cast<int32_t>(eIasFilterTypeLowShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 12, 10, static_cast<int32_t>(eIasFilterTypeLowShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, -12, 10, static_cast<int32_t>(eIasFilterTypeHighShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command eCommandSetCarEqualizerFilterParams...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasOk, cmdSetCarEqualizerFilterParams("Sink1", { 1, 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command eCommandSetCarEqualizerFilterParams (wrong channelIdx)...\n",this->test_info_->name());
  EXPECT_EQ(IasResult::eIasFailed, cmdSetCarEqualizerFilterParams("Sink1", { static_cast<int32_t>(cNumChannelsStream1), 1, 1000, 120, 10, static_cast<int32_t>(eIasFilterTypeHighShelving), 2 }));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n", this->test_info_->name());
  EXPECT_EQ(IasIModuleId::eIasOk, cmdProcessSetState(IasEqualizer::IasEqualizerCmdIds::eIasSetModuleState, "moduleState", "on"));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n", this->test_info_->name());
  EXPECT_EQ(IasIModuleId::eIasOk, cmdProcessSetState(IasEqualizer::IasEqualizerCmdIds::eIasSetModuleState, "moduleState", "off"));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for incomplete command EnableAudioProcessingComponent...\n", this->test_info_->name());
  EXPECT_EQ(IasIModuleId::eIasFailed, cmdProcessSetState(IasEqualizer::IasEqualizerCmdIds::eIasSetModuleState, {}, {}));
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n", this->test_info_->name());
  EXPECT_EQ(IasIModuleId::eIasOk, cmdProcessSetState(IasEqualizer::IasEqualizerCmdIds::eIasSetModuleState, "moduleState", "on"));

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;

  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeLast)+1);

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;

  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeFirst)-1);

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;

  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", -2000);

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;

  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd", 2000);

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

}

TEST_F(IasEqualizerCarGTest, CarEqualizerFailed)
{
  // delete actual setup
  delete mAudioChain;
  mPluginEngine->destroyModule(mEqualizer);
  delete mPluginEngine;

  mAudioChain = new (std::nothrow) IasAudioChain();
  ASSERT_TRUE(mAudioChain != nullptr);

  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  const auto chainres = mAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  mInStream1  = mAudioChain->createInputAudioStream("Sink1", 1, cNumChannelsStream1, false);
  mInStream2  = mAudioChain->createInputAudioStream("Sink2", 2, cNumChannelsStream2, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  mCmdDispatcher = std::make_shared<IasCmdDispatcher>();

  mPluginEngine = new (std::nothrow) IasPluginEngine(mCmdDispatcher);

  auto procres = mPluginEngine->loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  procres = mPluginEngine->createModuleConfig(&mEqualizerConfig);

  IasProperties properties;
  mEqualizerConfig->addStreamToProcess(mInStream1, "Sink1");
  mEqualizerConfig->addStreamToProcess(mInStream2, "Sink2");

  properties.set("EqualizerMode",
                 static_cast<Ias::Int32>(IasEqualizer::IasEqualizerMode::eIasCar));

  properties.set("numFilterStagesMax", cNumFilterStagesMax);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
}


} // namespace IasAudio
