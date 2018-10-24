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
#include "audio/mixerx/IasMixerCmd.hpp"
#include "IasMixerTest.hpp"

static const uint32_t cIasFrameLength { 64 };
static const uint32_t cIasSampleRate  { 48000 };

namespace IasAudio
{

#define IASTESTLOG(FMTSTR,...) printf("mixerTest" FMTSTR,##__VA_ARGS__)

void IasMixerTest::SetUp()
{
  mAudioChain = new IasAudioChain();
  ASSERT_TRUE(mAudioChain != nullptr);

  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;

  const auto chainres = mAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  mInStream0  = mAudioChain->createInputAudioStream("Input0", 0, 2, false);
  mInStream1  = mAudioChain->createInputAudioStream("Input1", 1, 2, false);
  mInStream6  = mAudioChain->createInputAudioStream("Input6", 1, 6, false);
  mOutStream4 = mAudioChain->createOutputAudioStream("Output4", 0, 4, false);
  mOutStream8 = mAudioChain->createOutputAudioStream("Output8", 1, 8, false);
  mOutStream3 = mAudioChain->createOutputAudioStream("Output3", 2, 3, false);
  mOutStream2 = mAudioChain->createOutputAudioStream("Output2", 3, 2, false);
  mOutStream6 = mAudioChain->createOutputAudioStream("Output6", 4, 6, false);

  setenv("AUDIO_PLUGIN_DIR", "../../..", true);

  mCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  ASSERT_TRUE(mCmdDispatcher != nullptr);

  mPluginEngine = new IasPluginEngine(mCmdDispatcher);
  ASSERT_TRUE(mPluginEngine != nullptr);

  auto procres = mPluginEngine->loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);

  procres = mPluginEngine->createModuleConfig(&mMixerConfig);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != mMixerConfig);

  mMixerConfig->addStreamToProcess(mInStream0, "Input0");
  mMixerConfig->addStreamToProcess(mInStream1, "Input1");
  mMixerConfig->addStreamToProcess(mInStream6, "Input6");
  mMixerConfig->addStreamToProcess(mOutStream4, "Output4");
  mMixerConfig->addStreamToProcess(mOutStream8, "Output8");
  mMixerConfig->addStreamToProcess(mOutStream3, "Output3");
  mMixerConfig->addStreamToProcess(mOutStream2, "Output2");
  mMixerConfig->addStreamToProcess(mOutStream6, "Output6");

  IasStringVector activePins;
  activePins.push_back("Input0");
  activePins.push_back("Input1");
  activePins.push_back("Input6");
  activePins.push_back("Output4");
  activePins.push_back("Output8");
  activePins.push_back("Output3");
  activePins.push_back("Output2");
  activePins.push_back("Output6");

  procres = mPluginEngine->createModule(mMixerConfig, "ias.mixer", "MyMixer", &mMixer);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(mMixer != nullptr);
}

void IasMixerTest::TearDown()
{
  delete mAudioChain;
  mPluginEngine->destroyModule(mMixer);
  delete mPluginEngine;
  mCmdDispatcher = nullptr;
}

TEST_F(IasMixerTest, mixerCreation)
{
  ASSERT_TRUE(nullptr != mMixer);
  IASTESTLOG(".%s:\nMixer creation success!\n", this->test_info_->name());

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nMixer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nMixer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationOutput2Channels)
{
  ASSERT_TRUE(nullptr != mMixer);
  IASTESTLOG(".%s:\nMixer creation success!\n", this->test_info_->name());

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream2, "Output2");

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nMixer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nMixer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationOutput4Channels)
{
  ASSERT_TRUE(nullptr != mMixer);
  IASTESTLOG(".%s:\nMixer creation success!\n", this->test_info_->name());

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nMixer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nMixer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationOutput6Channels)
{
  ASSERT_TRUE(nullptr != mMixer);
  IASTESTLOG(".%s:\nMixer creation success!\n", this->test_info_->name());

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream6, "Output6");

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\nMixer core creation success!\n", this->test_info_->name());

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\nMixer cmd interface creation success!\n", this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationFailMaxNumOutputChannels)
{
  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");
  mMixerConfig->addStreamMapping(mInStream1, "Input1", mOutStream8, "Output8");

  ASSERT_TRUE(nullptr != mMixer);

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  EXPECT_EQ(IasAudio::eIasAudioProcInitializationFailed, core->init());
  IASTESTLOG(".%s:\nMixer core init failed!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationFailOutputChannels)
{
  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");
  mMixerConfig->addStreamMapping(mInStream1, "Input1", mOutStream3, "Output3");

  ASSERT_TRUE(nullptr != mMixer);

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  EXPECT_EQ(IasAudio::eIasAudioProcInitializationFailed, core->init());
  IASTESTLOG(".%s:\nMixer core init failed!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationInputSixChannels)
{
  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");
  mMixerConfig->addStreamMapping(mInStream6, "Input6", mOutStream4, "Output4");

  ASSERT_TRUE(nullptr != mMixer);

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  EXPECT_EQ(IasAudio::eIasAudioProcOK, core->init());
  IASTESTLOG(".%s:\nMixer core init ok!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCore)
{
  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");
  mMixerConfig->addStreamMapping(mInStream6, "Input6", mOutStream4, "Output4");

  ASSERT_TRUE(nullptr != mMixer);

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  EXPECT_EQ(IasAudio::eIasAudioProcOK, core->init());
  IASTESTLOG(".%s:\nMixer core init ok!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}


TEST_F(IasMixerTest, mixerCreationNoStreamMapping)
{
  ASSERT_TRUE(nullptr != mMixer);

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);

  mAudioChain->addAudioComponent(mMixer);
  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  auto processCmd = [&](const int32_t cmdId, std::string&& cmd)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    cmdProperties.set<std::string>("pin", "Input0");
    cmdProperties.set<int32_t>("cmd", cmdId);
    cmdProperties.set<int32_t>(cmd, 1);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  IasIModuleId::IasResult cmdres;

  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetBalance, "balance");
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetFader, "fader");
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetInputGainOffset, "gain");
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCreationNoInit)
{
  ASSERT_TRUE(nullptr != mMixer);

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");

  auto *core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCmdInterfaceSetController)
{
  ASSERT_TRUE(nullptr != mMixer);

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCoreReset)
{
  ASSERT_TRUE(nullptr != mMixer);

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");

  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  EXPECT_EQ(IasAudio::eIasAudioProcOK, core->reset());
  IASTESTLOG(".%s:\nMixer core reset success!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mMixer);
}

TEST_F(IasMixerTest, mixerCmdInterfaceParseMessage)
{
  ASSERT_TRUE(nullptr != mMixer);

  mMixerConfig->addStreamMapping(mInStream0, "Input0", mOutStream4, "Output4");

  auto* cmdInterface = mMixer->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);

  mAudioChain->addAudioComponent(mMixer);
  auto* core = mMixer->getCore();
  ASSERT_TRUE(nullptr != core);

  auto processCmd = [&](int32_t cmdId, const char* pin,
      std::string&& cmd, const int32_t data)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    if(pin != nullptr)
    {
      cmdProperties.set<std::string>("pin", pin);
    }

    cmdProperties.set<int32_t>("cmd", cmdId);

    if(!cmd.empty())
    {
      cmdProperties.set<int32_t>(std::move(cmd), data);
    }

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  auto processCmdSetState = [&](int32_t cmdId,
      std::string&& cmd, std::string&& state)
  {
    IasProperties cmdProperties;
    IasProperties returnProperties;

    cmdProperties.set<int32_t>("cmd", cmdId);
    cmdProperties.set<std::string>(cmd, state);

    return cmdInterface->processCmd(cmdProperties, returnProperties);
  };

  IasIModuleId::IasResult cmdres;

  IASTESTLOG(".%s:\n Testing reaction for invalid commandId...\n",this->test_info_->name());
  cmdres = processCmd(12345678, {}, {} ,{});
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command (balance=10)...\n", this->test_info_->name());
  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetBalance, "Input0", "balance", 10);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command (balance=-10)...\n", this->test_info_->name());
  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetBalance, "Input0", "balance", -10);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command (fader=10)...\n", this->test_info_->name());
  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetFader, "Input0", "fader", 10);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command (fader=-10)...\n", this->test_info_->name());
  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetFader, "Input0", "fader", -10);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command (input gain offset=-10)...\n", this->test_info_->name());
  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetInputGainOffset, "Input0", "gain", -10);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n", this->test_info_->name());
  cmdres = processCmdSetState(IasMixer::IasMixerCmdIds::eIasSetModuleState, "moduleState", "on");
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n", this->test_info_->name());
  cmdres = processCmdSetState(IasMixer::IasMixerCmdIds::eIasSetModuleState, "moduleState", "off");
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for incomplete command EnableAudioProcessingComponent...\n", this->test_info_->name());
  cmdres = processCmd(IasMixer::IasMixerCmdIds::eIasSetModuleState, {}, {}, {});
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n", this->test_info_->name());
  cmdres = processCmdSetState(IasMixer::IasMixerCmdIds::eIasSetModuleState, "moduleState", "on");
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();
}


} // namespace IasAudio
