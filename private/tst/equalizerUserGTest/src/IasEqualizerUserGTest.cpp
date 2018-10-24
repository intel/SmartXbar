/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasMixerTest.cpp
 *
 *  Created on: August 2016
 */



#include <cstdio>
#include <iostream>
#include <utility>
#include <functional>

#include <gtest/gtest.h>
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "mixer/IasMixerCore.hpp"
#include "mixer/IasMixerCmdInterface.hpp"
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

#include "IasEqualizerUserGTest.hpp"

static const uint32_t cNumChannelsStream1 { 6 };
static const uint32_t cNumChannelsStream2 { 2 };
static const uint32_t cNumChannelsStream3 { 2 };
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

#define IASTESTLOG(FMTSTR,...) printf("equalizerUserGTest" FMTSTR,##__VA_ARGS__)



void IasEqualizerUserGTest::SetUp()
{

}

void IasEqualizerUserGTest::TearDown()
{

}

void IasEqualizerUserGTest::init()
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
  mInStream3  = mAudioChain->createInputAudioStream("Sink3", 3, cNumChannelsStream3, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);
  //setenv("AUDIO_PLUGIN_DIR", "/localdisk/mkielanx/audio/build/audio_project-host_INTERNAL_KC3.0_Base_fc21-64-debug/audio/daemon2/", true);


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
  mEqualizerConfig->addStreamToProcess(mInStream3, "Sink3");

  properties.set("EqualizerMode",
                 static_cast<int32_t>(IasEqualizer::IasEqualizerMode::eIasUser));

  properties.set("numFilterStagesMax", cNumFilterStagesMax);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(mEqualizer != nullptr);
}


void IasEqualizerUserGTest::init(const int numFilterStagesMax)
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
  mInStream3  = mAudioChain->createInputAudioStream("Sink3", 3, cNumChannelsStream3, false);
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
  mEqualizerConfig->addStreamToProcess(mInStream3, "Sink3");

  properties.set("EqualizerMode",
                 static_cast<Ias::Int32>(IasEqualizer::IasEqualizerMode::eIasUser));

  properties.set("numFilterStagesMax", numFilterStagesMax);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(mEqualizer != nullptr);
}


void IasEqualizerUserGTest::exit()
{
  delete mAudioChain;
  mPluginEngine->destroyModule(mEqualizer);
  delete mPluginEngine;
}

IasIModuleId::IasResult IasEqualizerUserGTest::setEqualizer(const char* pin,
  const int32_t filterId, const int32_t gain, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     std::string{pin});
  cmdProperties.set<int32_t>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<int32_t>("filterId", filterId);
  cmdProperties.set<int32_t>("gain",     gain);

  return cmd->processCmd(cmdProperties, returnProperties);
}

IasIModuleId::IasResult IasEqualizerUserGTest::setEqualizerParams(const char* pin,
  const SetEqualizerParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     std::string{pin});
  cmdProperties.set<int32_t>("cmd",
    static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<int32_t>("filterId", params.filterId);
  cmdProperties.set<int32_t>("freq",     params.freq);
  cmdProperties.set<int32_t>("quality",  params.quality);
  cmdProperties.set<int32_t>("type",     params.type);
  cmdProperties.set<int32_t>("order",    params.order);

  return cmd->processCmd(cmdProperties, returnProperties);
}


IasIModuleId::IasResult IasEqualizerUserGTest::setRampGradientSingleStream(const char* pin,
    const SetRampGradientSingleStreamParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     std::string{pin});
  cmdProperties.set<int32_t>("cmd",      params.cmd);
  cmdProperties.set<int32_t>("gradient", params.gradient);

  return cmd->processCmd(cmdProperties, returnProperties);
}


IasIModuleId::IasResult IasEqualizerUserGTest::setConfigFilterParamsStream(const char* pin,
  const SetEqualizerParams params, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     std::string{pin});
  cmdProperties.set<int32_t>("cmd",
    static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));
  cmdProperties.set<int32_t>("filterId", params.filterId);
  cmdProperties.set<int32_t>("freq",     params.freq);
  cmdProperties.set<int32_t>("quality",  params.quality);
  cmdProperties.set<int32_t>("type",     params.type);
  cmdProperties.set<int32_t>("order",    params.order);

  return cmd->processCmd(cmdProperties, returnProperties);
}


std::pair<IasIModuleId::IasResult, SetEqualizerParams> IasEqualizerUserGTest::getConfigFilterParamsStream(const char* pin,
  const int32_t filterId, IasIModuleId* cmd)
{
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<std::string>("pin",     std::string{pin});
  cmdProperties.set<int32_t>("cmd",
    static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasGetConfigFilterParamsStream));
  cmdProperties.set<int32_t>("filterId", filterId);

  const auto res = cmd->processCmd(cmdProperties, returnProperties);

  int32_t freq, quality, type, order;

  returnProperties.get("freq",    &freq);
  returnProperties.get("quality", &quality);
  returnProperties.get("type",    &type);
  returnProperties.get("order",   &order);

  SetEqualizerParams params;

  params.filterId = filterId;
  params.freq     = freq;
  params.quality  = quality;
  params.type     = type;
  params.order    = order;

  return std::make_pair(res, params);
}


TEST_F(IasEqualizerUserGTest, equalizerCreation)
{
  init();
  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
  exit();
}

TEST_F(IasEqualizerUserGTest, IasEqualizerCmdSetWrongCommand)
{
  init();
  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);

  const auto setCarEqualizerNumFilters = [&](const char* pin)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    int32_t channelIdx {}, numFilters {};
    cmdProperties.set<std::string>("pin",       std::string{pin});
    cmdProperties.set<int32_t>("cmd",
        IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerNumFilters);
    cmdProperties.set<int32_t>("channelIdx", channelIdx);
    cmdProperties.set<int32_t>("numFilters", numFilters);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  const auto setCarEqualizerFilterParams = [&](const char* pin)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    int32_t channelIdx {}, filterId {}, freq {}, gain {}, quality {}, type {}, order {};
    cmdProperties.set<std::string>("pin",       std::string{pin});
    cmdProperties.set<int32_t>("cmd",
        IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams);
    cmdProperties.set<int32_t>("channelIdx", channelIdx);
    cmdProperties.set<int32_t>("filterId",   filterId);
    cmdProperties.set<int32_t>("freq",       freq);
    cmdProperties.set<int32_t>("gain",       gain);
    cmdProperties.set<int32_t>("quality",    quality);
    cmdProperties.set<int32_t>("type",       type);
    cmdProperties.set<int32_t>("order",      order);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  const auto getCarEqualizerNumFilters = [&](const char* pin)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    int32_t channelIdx {};
    cmdProperties.set<std::string>("pin", std::string{pin});
    cmdProperties.set<int32_t>("cmd", IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerNumFilters);
    cmdProperties.set<int32_t>("channelIdx", channelIdx);

    const IasIModuleId::IasResult err = cmdInterface->processCmd(cmdProperties, returnProperties);

    int32_t numFilters = -1;
    returnProperties.get("numFilters", &numFilters);

    return err;
  };

  const auto getCarEqualizerFilterParams = [&](const char* pin)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    int32_t channelIdx {}, filterId {};
    cmdProperties.set<std::string>("pin", std::string{pin});
    cmdProperties.set<int32_t>("cmd",
        IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerFilterParams);
    cmdProperties.set<int32_t>("channelIdx", channelIdx);
    cmdProperties.set<int32_t>("filterId",   filterId);

    const IasIModuleId::IasResult err = cmdInterface->processCmd(cmdProperties, returnProperties);

    int32_t  freq , gain, quality, type, order;

    returnProperties.get("freq",    &freq);
    returnProperties.get("gain",    &gain);
    returnProperties.get("quality", &quality);
    returnProperties.get("type",    &type);
    returnProperties.get("order",   &order);

    return err;
  };

  ASSERT_EQ(IasAudioChain::eIasFailed, setCarEqualizerNumFilters("Sink1") );
  ASSERT_EQ(IasAudioChain::eIasFailed, setCarEqualizerFilterParams("Sink1") );
  ASSERT_EQ(IasAudioChain::eIasFailed, getCarEqualizerNumFilters("Sink1") );
  ASSERT_EQ(IasAudioChain::eIasFailed, getCarEqualizerFilterParams("Sink1") );

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",  "");
  cmdProperties.set<Ias::Int32>("cmd",
  static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasGetConfigFilterParamsStream));
  cmdProperties.set<Ias::Int32>("filterId", {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::Int32>("cmd",
  static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasGetConfigFilterParamsStream));
  cmdProperties.set<Ias::Int32>("filterId", {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",  "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
  static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasGetConfigFilterParamsStream));
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  cmdProperties.set<Ias::Int32>("order",    {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  cmdProperties.set<Ias::Int32>("order",    {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }


  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  cmdProperties.set<Ias::Int32>("order",    {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  cmdProperties.set<Ias::Int32>("order",    1);
  EXPECT_EQ(IasAudioChain::eIasOk, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  cmdProperties.set<Ias::Int32>("order",    2);
  EXPECT_EQ(IasAudioChain::eIasOk, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizerParams));
  cmdProperties.set<Ias::Int32>("filterId", 1500);
  cmdProperties.set<Ias::Int32>("freq",     {});
  cmdProperties.set<Ias::Int32>("quality",  {});
  cmdProperties.set<Ias::Int32>("type",     {});
  cmdProperties.set<Ias::Int32>("order",    2);
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("gain",     {});
  EXPECT_EQ(IasAudioChain::eIasOk, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<Ias::Int32>("filterId", {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<Ias::Int32>("filterId", {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<Ias::Int32>("filterId", {});
  cmdProperties.set<Ias::Int32>("gain",     {});
  EXPECT_EQ(IasAudioChain::eIasOk, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetEqualizer);
  cmdProperties.set<Ias::Int32>("filterId", 50000);
  cmdProperties.set<Ias::Int32>("gain",     {});
  EXPECT_EQ(IasAudioChain::eIasFailed, cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfig)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const char* pin, const SetEqualizerParams params)
  {
    return setConfigFilterParamsStream(pin, params, cmdInterface);
  };

  const auto cmdGetConfigFilterParamsStream = [&](const char* pin, const int32_t filterId)
  {
    return getConfigFilterParamsStream(pin, filterId, cmdInterface);
  };

  SetEqualizerParams paramsFirstStage, paramsSecondStage;

  paramsFirstStage.freq  = 100;
  paramsSecondStage.freq = 2000;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsSecondStage.type = IasAudio::eIasFilterTypePeak;
  paramsFirstStage.order  = 2;
  paramsSecondStage.order = 2;
  paramsFirstStage.quality  = 10;
  paramsSecondStage.quality = 20;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_EQ(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  IASTESTLOG(".%s:\n Testing reaction for invalid stream...\n",this->test_info_->name());
  ASSERT_EQ(IasAudioChain::eIasFailed, cmdSetConfigFilterParamsStream("dummyStream", paramsFirstStage));

  IASTESTLOG(".%s:\n Setting config filter params with a too big filter stage, expect error\n",this->test_info_->name());
  paramsFirstStage.filterId = cNumFilterStagesMax + 1;
  ASSERT_EQ(IasAudioChain::eIasFailed, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  IASTESTLOG(".%s:\n Reading config filter params with too big filter id\n",this->test_info_->name());
  paramsFirstStage.filterId = 16;
  ASSERT_EQ(IasAudioChain::eIasFailed, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  IASTESTLOG(".%s:\n Setting config filter params correctly now for first stream, expect no error\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_EQ(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  auto res = cmdGetConfigFilterParamsStream("Sink1", 0);
  ASSERT_EQ(IasAudioChain::eIasOk,     res.first);
  ASSERT_EQ(paramsFirstStage.freq,     res.second.freq);
  ASSERT_EQ(paramsFirstStage.type,     res.second.type);
  ASSERT_EQ(paramsFirstStage.order,    res.second.order);
  ASSERT_EQ(paramsFirstStage.quality,  res.second.quality);

  IASTESTLOG(".%s:\n Setting config filter params correctly now for first stream, expect no error\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_EQ(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  res = cmdGetConfigFilterParamsStream("Sink1", 0);
  ASSERT_EQ(IasAudioChain::eIasOk,     res.first);
  ASSERT_EQ(paramsFirstStage.freq,     res.second.freq);
  ASSERT_EQ(paramsFirstStage.type,     res.second.type);
  ASSERT_EQ(paramsFirstStage.order,    res.second.order);
  ASSERT_EQ(paramsFirstStage.quality,  res.second.quality);

  IASTESTLOG(".%s:\n Setting config filter params correctly now for first stream, expect no error\n",this->test_info_->name());
  paramsSecondStage.filterId = 1;
  ASSERT_EQ(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsSecondStage));

  res = cmdGetConfigFilterParamsStream("Sink1", 1);
  ASSERT_EQ(IasAudioChain::eIasOk,      res.first);
  ASSERT_EQ(paramsSecondStage.freq,     res.second.freq);
  ASSERT_EQ(paramsSecondStage.type,     res.second.type);
  ASSERT_EQ(paramsSecondStage.order,    res.second.order);
  ASSERT_EQ(paramsSecondStage.quality,  res.second.quality);
  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfigWrongId)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const Ias::Char* pin, const SetEqualizerParams params)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));

    return cmdInterface->processCmd(cmdProperties, returnProperties);

    (void)pin;
    (void)params;
  };

  SetEqualizerParams paramsFirstStage;

  paramsFirstStage.freq  = 100;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsFirstStage.order  = 2;
  paramsFirstStage.quality  = 10;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_NE(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfigWrongFilter)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const Ias::Char* pin, const SetEqualizerParams params)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<Ias::String>("pin",     Ias::String{pin});
    cmdProperties.set<Ias::Int32>("cmd",
    static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));

    return cmdInterface->processCmd(cmdProperties, returnProperties);

    (void)params;
  };

  SetEqualizerParams paramsFirstStage;

  paramsFirstStage.freq  = 100;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsFirstStage.order  = 2;
  paramsFirstStage.quality  = 10;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_NE(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfigWrongFreq)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const Ias::Char* pin, const SetEqualizerParams params)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<Ias::String>("pin",     Ias::String{pin});
    cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));
    cmdProperties.set<Ias::Int32>("filterId", params.filterId);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  SetEqualizerParams paramsFirstStage;

  paramsFirstStage.freq  = 100;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsFirstStage.order  = 2;
  paramsFirstStage.quality  = 10;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_NE(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfigWrongQuality)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const Ias::Char* pin, const SetEqualizerParams params)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<Ias::String>("pin",     Ias::String{pin});
    cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));
     cmdProperties.set<Ias::Int32>("filterId", params.filterId);
     cmdProperties.set<Ias::Int32>("freq",     params.freq);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  SetEqualizerParams paramsFirstStage;

  paramsFirstStage.freq  = 100;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsFirstStage.order  = 2;
  paramsFirstStage.quality  = 10;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_NE(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfigWrongType)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const Ias::Char* pin, const SetEqualizerParams params)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<Ias::String>("pin",     Ias::String{pin});
    cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));
      cmdProperties.set<Ias::Int32>("filterId", params.filterId);
      cmdProperties.set<Ias::Int32>("freq",     params.freq);
      cmdProperties.set<Ias::Int32>("quality",  params.quality);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  SetEqualizerParams paramsFirstStage;

  paramsFirstStage.freq  = 100;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsFirstStage.order  = 2;
  paramsFirstStage.quality  = 10;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_NE(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  exit();
}

TEST_F(IasEqualizerUserGTest, UserEqualizerConfigWrongOrder)
{
  init();
  mAudioChain->addAudioComponent(mEqualizer);

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  const auto cmdSetConfigFilterParamsStream = [&](const Ias::Char* pin, const SetEqualizerParams params)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;
    cmdProperties.set<Ias::String>("pin",     Ias::String{pin});
    cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasSetConfigFilterParamsStream));
    cmdProperties.set<Ias::Int32>("filterId", params.filterId);
    cmdProperties.set<Ias::Int32>("freq",     params.freq);
    cmdProperties.set<Ias::Int32>("quality",  params.quality);
    cmdProperties.set<Ias::Int32>("type",     params.type);

    return cmdInterface->processCmd(cmdProperties, returnProperties);

    (void)params;
  };

  SetEqualizerParams paramsFirstStage;

  paramsFirstStage.freq  = 100;
  paramsFirstStage.type  = IasAudio::eIasFilterTypeLowShelving;
  paramsFirstStage.order  = 2;
  paramsFirstStage.quality  = 10;

  IASTESTLOG(".%s:\n Testing reaction for valid command\n",this->test_info_->name());
  paramsFirstStage.filterId = 0;
  ASSERT_NE(IasAudioChain::eIasOk, cmdSetConfigFilterParamsStream("Sink1", paramsFirstStage));

  exit();
}

TEST_F(IasEqualizerUserGTest, equalizerNumStageMaxFailed0)
{
  const int numFilterStageMax = 0;
  init(numFilterStageMax);

  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
  exit();
}

TEST_F(IasEqualizerUserGTest, equalizerNumStageMaxFailed1)
{
  const int numFilterStageMax = 1;
  init(numFilterStageMax);

  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
  exit();
}

TEST_F(IasEqualizerUserGTest, equalizerNumStageMaxFailed2)
{
  const int numFilterStageMax = 2;
  init(numFilterStageMax);

  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
  exit();
}

TEST_F(IasEqualizerUserGTest, equalizerNumStageMaxFailedBig)
{
  const int numFilterStageMax = 250;
  init(numFilterStageMax);

  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);
  exit();
}

TEST_F(IasEqualizerUserGTest, numFilterStagesMaxNotSet)
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
  mInStream3  = mAudioChain->createInputAudioStream("Sink3", 3, cNumChannelsStream3, false);

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
  mEqualizerConfig->addStreamToProcess(mInStream3, "Sink3");

  properties.set("EqualizerMode",
                 static_cast<Ias::Int32>(IasEqualizer::IasEqualizerMode::eIasUser));

  //NumFilterStagesMax - not set

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(mEqualizer != nullptr);

  exit();
}

TEST_F(IasEqualizerUserGTest, CreateModuleFailed)
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
  mInStream3  = mAudioChain->createInputAudioStream("Sink3", 3, cNumChannelsStream3, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);
  //setenv("AUDIO_PLUGIN_DIR", "/localdisk/mkielanx/audio/build/audio_project-host_INTERNAL_KC3.0_Base_fc21-64-debug/audio/daemon2/", true);

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
  mEqualizerConfig->addStreamToProcess(mInStream3, "Sink3");

  properties.set("EqualizerMode", -20);
  properties.set("numFilterStagesMax", cNumFilterStagesMax);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);
  ASSERT_EQ(eIasAudioProcOK, procres);

  ASSERT_TRUE(nullptr != mEqualizer);

  mEqualizer->getCore();
  mEqualizer->getCmdInterface();
  mAudioChain->addAudioComponent(mEqualizer);

  exit();
}

TEST_F(IasEqualizerUserGTest, CreateModuleFailed1)
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
  mInStream3  = mAudioChain->createInputAudioStream("Sink3", 3, cNumChannelsStream3, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);
  //setenv("AUDIO_PLUGIN_DIR", "/localdisk/mkielanx/audio/build/audio_project-host_INTERNAL_KC3.0_Base_fc21-64-debug/audio/daemon2/", true);


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
  mEqualizerConfig->addStreamToProcess(mInStream3, "Sink3");


  properties.set("EqualizerMode",
                 static_cast<Ias::Int32>(IasEqualizer::IasEqualizerMode::eIasLastEntry)+1);

  properties.set("numFilterStagesMax", cNumFilterStagesMax);

  IasStringVector activePins;
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  activePins.push_back("Sink3");

  mEqualizerConfig->setProperties(properties);

  procres = mPluginEngine->createModule(mEqualizerConfig, "ias.equalizer", "MyEqualizer", &mEqualizer);
  ASSERT_EQ(eIasAudioProcOK, procres);

  ASSERT_TRUE(nullptr != mEqualizer);

  mEqualizer->getCore();
  mEqualizer->getCmdInterface();
  mAudioChain->addAudioComponent(mEqualizer);

  exit();
}

TEST_F(IasEqualizerUserGTest, IasEqualizerCmdOutOfRange)
{
  init();
  ASSERT_TRUE(nullptr != mEqualizer);
  IASTESTLOG(".%s:\nEqualizer creation success!\n", this->test_info_->name());

  auto* core = mEqualizer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nEqualizer core creation success!\n", this->test_info_->name());

  IasIModuleId* cmdInterface = mEqualizer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nEqualizer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mEqualizer);

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;

  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeLast)+1);

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;

  cmdProperties.set<Ias::String>("pin", "Sink1");
  cmdProperties.set<Ias::Int32>("cmd",
      static_cast<Ias::Int32>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeFirst)-1);

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

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::String>("pin",     "");
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetRampGradientSingleStream);
  cmdProperties.set<Ias::Int32>("gradient", {});

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetRampGradientSingleStream);
  cmdProperties.set<Ias::Int32>("gradient", {});

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }

  {
  IasProperties cmdProperties;
  IasProperties returnProperties;
  cmdProperties.set<Ias::Int32>("cmd",      IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetRampGradientSingleStream);

  EXPECT_EQ(IasResult::eIasFailed,cmdInterface->processCmd(cmdProperties, returnProperties));
  }


  exit();
}


} // namespace IasAudio




