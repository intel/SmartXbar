/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasVolumeTest.cpp
 *
 *  Created on: Apr 25, 2013
 */

#include <stdio.h>
#include <iostream>
#include <gtest/gtest.h>
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"
#include "volume/IasVolumeHelper.hpp"

static const uint32_t cIasFrameLength     = 64;
static const uint32_t cIasSampleRate      = 48000;
static const uint32_t cIasLoudnessTableTestLength = 8;

#ifndef AUDIO_PLUGIN_DIR
#define AUDIO_PLUGIN_DIR    "../../.."
#endif

using namespace IasAudio;
static IasVolumeLoudnessTable loudnessTable;

namespace IasAudio
{


#define IASTESTLOG(FMTSTR,...) printf("volumeTest" FMTSTR,##__VA_ARGS__)

class IasVolumeTest : public ::testing::Test
{
protected:
  virtual void SetUp();
  virtual void TearDown();

public:
  IasAudioChain                 *mAudioChain;
  IasIGenericAudioCompConfig    *mVolumeConfig;
  IasGenericAudioComp           *mVolume;
  IasAudioStream                *mSink0;
  IasAudioStream                *mSink1;
  IasAudioStream                *mSink2;
  IasCmdDispatcherPtr            mCmdDispatcher;
  IasPluginEngine               *mPluginEngine;
};


void IasVolumeTest::SetUp()
{
  mAudioChain = new IasAudioChain();
  ASSERT_TRUE(mAudioChain != nullptr);
  IasAudioChain::IasInitParams initParams;
  initParams.periodSize = cIasFrameLength;
  initParams.sampleRate = cIasSampleRate;
  IasAudioChain::IasResult chainres = mAudioChain->init(initParams);
  ASSERT_EQ(IasAudioChain::eIasOk, chainres);

  mSink0  = mAudioChain->createInputAudioStream("Sink0",  0, 2,false);
  mSink1  = mAudioChain->createInputAudioStream("Sink1",  1, 2,false);
  mSink2  = mAudioChain->createInputAudioStream("Sink2",  2, 6,false);

  const IasAudioFilterConfigParams loudnessFilterParamsLow  = {    50, 1.0f, 1.0f, eIasFilterTypeLowShelving,  2};
  const IasAudioFilterConfigParams loudnessFilterParamsMid  = {  2000, 1.0f, 2.0f, eIasFilterTypePeak,         2};
  const IasAudioFilterConfigParams loudnessFilterParamsHigh = {  8000, 1.0f, 0.8f, eIasFilterTypeHighShelving, 2};

  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);
  mCmdDispatcher = std::make_shared<IasCmdDispatcher>();
  ASSERT_TRUE(mCmdDispatcher != nullptr);
  mPluginEngine = new IasPluginEngine(mCmdDispatcher);
  ASSERT_TRUE(mPluginEngine != nullptr);
  IasAudioProcessingResult procres = mPluginEngine->loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, procres);
  procres = mPluginEngine->createModuleConfig(&mVolumeConfig);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(nullptr != mVolumeConfig);
  int32_t tempVol = 0;
  int32_t tempGain = 0;
  IasInt32Vector ldGains;
  IasInt32Vector ldVolumes;
  for(uint32_t i=0; i< cIasLoudnessTableTestLength; i++)
  {
    ldVolumes.push_back(tempVol);
    ldGains.push_back(tempGain);
    tempVol-= 100;
    tempGain+= 30;
  }
  IasProperties properties;
  properties.set("ld.volumes.0", ldVolumes);
  properties.set("ld.gains.0", ldGains);
  properties.set("ld.volumes.1", ldVolumes);
  properties.set("ld.gains.1", ldGains);
  properties.set("ld.volumes.2", ldVolumes);
  properties.set("ld.gains.2", ldGains);
  properties.set("numFilterBands", 3);
  IasInt32Vector freqOrderType;
  IasFloat32Vector gainQual;
  freqOrderType.resize(3);
  gainQual.resize(2);
  freqOrderType[0] = loudnessFilterParamsLow.freq;
  freqOrderType[1] = loudnessFilterParamsLow.order;
  freqOrderType[2] = loudnessFilterParamsLow.type;
  gainQual[0] = loudnessFilterParamsLow.gain;
  gainQual[1] = loudnessFilterParamsLow.quality;
  properties.set("ld.freq_order_type.0", freqOrderType);
  properties.set("ld.gain_quality.0", gainQual);
  freqOrderType[0] = loudnessFilterParamsMid.freq;
  freqOrderType[1] = loudnessFilterParamsMid.order;
  freqOrderType[2] = loudnessFilterParamsMid.type;
  gainQual[0] = loudnessFilterParamsMid.gain;
  gainQual[1] = loudnessFilterParamsMid.quality;
  properties.set("ld.freq_order_type.1", freqOrderType);
  properties.set("ld.gain_quality.1", gainQual);
  freqOrderType[0] = loudnessFilterParamsHigh.freq;
  freqOrderType[1] = loudnessFilterParamsHigh.order;
  freqOrderType[2] = loudnessFilterParamsHigh.type;
  gainQual[0] = loudnessFilterParamsHigh.gain;
  gainQual[1] = loudnessFilterParamsHigh.quality;
  properties.set("ld.freq_order_type.2", freqOrderType);
  properties.set("ld.gain_quality.2", gainQual);
  mVolumeConfig->addStreamToProcess(mSink0, "Sink0");
  mVolumeConfig->addStreamToProcess(mSink1, "Sink1");
  mVolumeConfig->addStreamToProcess(mSink2, "Sink2");
  IasStringVector activePins;
  activePins.push_back("Sink0");
  activePins.push_back("Sink1");
  activePins.push_back("Sink2");
  properties.set("activePinsForBand.0", activePins);
  properties.set("activePinsForBand.1", activePins);
  properties.set("activePinsForBand.2", activePins);
  mVolumeConfig->setProperties(properties);
  procres = mPluginEngine->createModule(mVolumeConfig, "ias.volume", "MyVolume", &mVolume);
  ASSERT_EQ(eIasAudioProcOK, procres);
  ASSERT_TRUE(mVolume != nullptr);
}

void IasVolumeTest::TearDown()
{
  delete mAudioChain;
  loudnessTable.gains.clear();
  loudnessTable.volumes.clear();
  mPluginEngine->destroyModule(mVolume);
  delete mPluginEngine;
  mCmdDispatcher = nullptr;
}

bool getSDVTableFromProperties(const IasProperties &properties, IasVolumeSDVTable &sdvTable)
{
  IasInt32Vector speed;
  IasInt32Vector gain_inc;
  IasInt32Vector gain_dec;
  properties.get("sdv.speed", &speed);
  properties.get("sdv.gain_inc", &gain_inc);
  properties.get("sdv.gain_dec", &gain_dec);

  if (speed.size() > 0 && speed.size() == gain_inc.size() && speed.size() == gain_dec.size())
  {
    for (uint32_t i = 0; i < speed.size(); i++)
    {
      sdvTable.speed.push_back(speed[i]);
      sdvTable.gain_inc.push_back(gain_inc[i]);
      sdvTable.gain_dec.push_back(gain_dec[i]);
    }
    return true;
  }
  else
  {
    return false;
  }
}

TEST_F(IasVolumeTest, volumeCreation)
{
  ASSERT_TRUE(nullptr != mVolume);
  IASTESTLOG(".%s:\n Volume creation success!\n",this->test_info_->name());
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr != core);
  IASTESTLOG(".%s:\n Volume core creation success!\n",this->test_info_->name());
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  IASTESTLOG(".%s:\n Volume cmd interface creation success!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mVolume);
}

TEST_F(IasVolumeTest, volumeCoreReset)
{
  ASSERT_TRUE(nullptr != mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr != core);

  EXPECT_EQ(IasAudio::eIasAudioProcOK,core->reset());
  IASTESTLOG(".%s:\n Volume core reset success!\n",this->test_info_->name());

  mAudioChain->addAudioComponent(mVolume);
}

TEST_F(IasVolumeTest, setSDVTableOk)
{
  IASTESTLOG(".%s:\n Setting SDV table...\n",this->test_info_->name());
  ASSERT_TRUE(nullptr != mVolume);
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  mAudioChain->addAudioComponent(mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr != core);
  IasInt32Vector sdvSpeed;
  IasInt32Vector sdvGainInc;
  IasInt32Vector sdvGainDec;
  sdvSpeed.push_back(50);
  sdvSpeed.push_back(80);
  sdvSpeed.push_back(100);
  sdvGainInc.push_back(0);
  sdvGainInc.push_back(20);
  sdvGainInc.push_back(40);
  sdvGainDec.push_back(0);
  sdvGainDec.push_back(25);
  sdvGainDec.push_back(50);
  IasProperties cmdProperties;
  IasProperties returnProperties;
  int32_t cmdId = IasVolume::eIasSetSdvTable;
  cmdProperties.set("cmd", cmdId);
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  IasIModuleId::IasResult cmdres;
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  cmdProperties.clearAll();
  cmdId = IasVolume::eIasGetSdvTable;
  cmdProperties.set("cmd", cmdId);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  IasVolumeSDVTable sdvTableGet;
  bool found;
  found = getSDVTableFromProperties(returnProperties, sdvTableGet);
  ASSERT_TRUE(found);
  uint32_t loopCnt=0;
  std::cout << "Size of table: " << sdvTableGet.speed.size() << " entries" << std::endl;
  for (uint32_t i = 0; i < sdvTableGet.speed.size(); i++)
  {
    printf("\nspeed[%3d] = %3d\t",loopCnt,sdvTableGet.speed[i]);
    printf("gain_inc[%3d] = %3d\t",loopCnt,sdvTableGet.gain_inc[i]);
    printf("gain_dec[%3d] = %3d",loopCnt,sdvTableGet.gain_dec[i]);
    loopCnt++;
  }
  printf("\n\n");

  IASTESTLOG(".%s:\n suceeded as expected\n",this->test_info_->name());
}

TEST_F(IasVolumeTest, setSDVTableNotOk)
{
  IASTESTLOG(".%s:\n Setting SDV table...\n",this->test_info_->name());
  ASSERT_TRUE(nullptr!=mVolume);
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr!=cmdInterface);
  mAudioChain->addAudioComponent(mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr!=core);

  IasInt32Vector sdvSpeed;
  IasInt32Vector sdvGainInc;
  IasInt32Vector sdvGainDec;
  IasProperties cmdProperties;
  IasProperties returnProperties;
  IasIModuleId::IasResult cmdres;
  int32_t cmdId = IasVolume::eIasSetSdvTable;
  cmdProperties.set("cmd", cmdId);
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  std::cout << "Try to set empty sdv table" << std::endl;
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);

  sdvSpeed.push_back(50);
  sdvSpeed.push_back(80);
  //seting a speed value out of order
  sdvSpeed.push_back(70);
  sdvGainInc.push_back(0);
  sdvGainInc.push_back(20);
  sdvGainInc.push_back(40);
  sdvGainDec.push_back(0);
  sdvGainDec.push_back(25);
  sdvGainDec.push_back(50);
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  std::cout << "Try to set sdv table with wrong speed order" << std::endl;
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);

  sdvSpeed[2]=100;
  //setting a gain_inc value out of order
  sdvGainInc[2] =19;
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  std::cout << "Try to set sdv table with wrong volume order" << std::endl;
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);

  sdvGainInc[2]=40;
  //setting a gain_dec value out of order
  sdvGainDec[2]=22;
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  std::cout << "Try to set sdv table with wrong gain order" << std::endl;
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);

  sdvGainDec[2]=50;

  for(uint32_t i=3;i<=cIasAudioMaxSDVTableLength;i++)
  {
    sdvSpeed.push_back(sdvSpeed[i-1] +1);
    sdvGainInc.push_back(sdvGainInc[i-1] +1);
    sdvGainDec.push_back(sdvGainDec[i-1] +1);
  }
  std::cout << "Setting table with size " << sdvSpeed.size() << std::endl;
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);

  IASTESTLOG(".%s:\n succeeded as expected\n",this->test_info_->name());
}


TEST_F(IasVolumeTest, volumeSetSpeed)
{
  IASTESTLOG(".%s:\n Calling setSpeed method...\n",this->test_info_->name());
  ASSERT_TRUE(nullptr != mVolume);
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  mAudioChain->addAudioComponent(mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr != core);
  IasProperties cmdProperties;
  IasProperties returnProperties;
  IasIModuleId::IasResult cmdres;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSpeed);
  cmdProperties.set("speed", 50);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);

  printf("\n");
  IASTESTLOG(".%s:\n suceeded as expected\n",this->test_info_->name());
}

TEST_F(IasVolumeTest, volumeGetSDVTableWhenNotSet)
{
  ASSERT_TRUE(nullptr != mVolume);
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  mAudioChain->addAudioComponent(mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr != core);
  IasProperties cmdProperties;
  IasProperties returnProperties;
  IasIModuleId::IasResult cmdres;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasGetSdvTable);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
}


TEST_F(IasVolumeTest, volumeCmdInterfaceParseMessage)
{
  ASSERT_TRUE(nullptr != mVolume);
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr != cmdInterface);
  mAudioChain->addAudioComponent(mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr != core);

  IASTESTLOG(".%s:\n Testing reaction for invalid commandId...\n",this->test_info_->name());
  IasProperties cmdProperties;
  IasProperties returnProperties;
  IasIModuleId::IasResult cmdres;
  cmdProperties.set<int32_t>("cmd", 12345678);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);

  IASTESTLOG(".%s:\n Testing reaction for valid command setMuteState...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink0");
  IasInt32Vector muteParams;
  bool isMuted = true;
  muteParams.push_back(isMuted);              // mute state
  muteParams.push_back(100);                  // ramp time
  muteParams.push_back(eIasRampShapeLinear);  // ramp shape
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command setMuteState...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setMuteState (invalid sink)...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink100000");     // Invalid pin name
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setMuteState (invalid rampTime)...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink0");
  muteParams.clear();
  muteParams.push_back(isMuted);              // mute state
  muteParams.push_back(-100);                 // negative ramp time
  muteParams.push_back(eIasRampShapeLinear);  // ramp shape
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink0");
  muteParams.clear();
  muteParams.push_back(isMuted);              // mute state
  muteParams.push_back(0);                    // zero ramp time
  muteParams.push_back(eIasRampShapeLinear);  // ramp shape
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink0");
  muteParams.clear();
  muteParams.push_back(isMuted);              // mute state
  muteParams.push_back(30000);                // too big ramp time
  muteParams.push_back(eIasRampShapeLinear);  // ramp shape
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setMuteState (invalid rampShape)...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetMuteState);
  cmdProperties.set<std::string>("pin", "Sink0");
  muteParams.clear();
  muteParams.push_back(isMuted);              // mute state
  muteParams.push_back(100);                 // negative ramp time
  muteParams.push_back(eIasRampShapeLogarithm);  // ramp shape
  cmdProperties.set("params", muteParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  muteParams.clear();

  IASTESTLOG(".%s:\n Testing reaction for valid command setVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", -10000);
  IasInt32Vector rampParams;
  rampParams.push_back(100);
  rampParams.push_back(eIasRampShapeLinear);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command setVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", -10);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setVolume (invalid sink) ...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink100000000");
  cmdProperties.set("volume", 2);
  rampParams.clear();
  rampParams.push_back(100);
  rampParams.push_back(eIasRampShapeLinear);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setVolume (invalid ramp time) ...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", 2);
  rampParams.clear();
  rampParams.push_back(-1);
  rampParams.push_back(eIasRampShapeLinear);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", 2);
  rampParams.clear();
  rampParams.push_back(0);
  rampParams.push_back(eIasRampShapeLinear);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", 2);
  rampParams.clear();
  rampParams.push_back(30000);
  rampParams.push_back(eIasRampShapeLinear);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setVolume (invalid ramp shape) ...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", 2);
  rampParams.clear();
  rampParams.push_back(100);
  rampParams.push_back(eIasRampShapeLogarithm);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();


  IASTESTLOG(".%s:\n Testing reaction for valid command setVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", -10);
  rampParams.clear();
  rampParams.push_back(100);
  rampParams.push_back(eIasRampShapeLinear);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for incomplete command setVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  cmdProperties.set("volume", -10);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink100000000");      // Invalid pin name
  cmdProperties.set("volume", -10);
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set("volume", -10);
  rampParams.clear();
  rampParams.push_back(100);
  rampParams.push_back(eIasRampShapeLogarithm); //this is not supported (yet?)
  cmdProperties.set("ramp", rampParams);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command setLoudness...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudness);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set<std::string>("loudness", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command setLoudness...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudness);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set<std::string>("loudness", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setLoudness...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudness);
  cmdProperties.set<std::string>("pin", "Sink1000000000");     // Invalid pin name
  cmdProperties.set<std::string>("loudness", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command setSdvTable...\n",this->test_info_->name());
  IasInt32Vector sdvSpeed;
  IasInt32Vector sdvGainInc;
  IasInt32Vector sdvGainDec;
  sdvSpeed.push_back(50);
  sdvSpeed.push_back(80);
  sdvSpeed.push_back(100);
  sdvGainInc.push_back(0);
  sdvGainInc.push_back(20);
  sdvGainInc.push_back(40);
  sdvGainDec.push_back(0);
  sdvGainDec.push_back(25);
  sdvGainDec.push_back(50);
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSdvTable);
  cmdProperties.set("sdv.speed", sdvSpeed);
  cmdProperties.set("sdv.gain_inc", sdvGainInc);
  cmdProperties.set("sdv.gain_dec", sdvGainDec);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command setSpeedControlledVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSpeedControlledVolume);
  cmdProperties.set<std::string>("pin", "Sink0");
  cmdProperties.set<std::string>("sdv", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid speed information...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSpeed);
  cmdProperties.set("speed", 44);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command setSpeedControlledVolume...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetSpeedControlledVolume);
  cmdProperties.set<std::string>("pin", "Sink100000000"); //Invalid pin name
  cmdProperties.set<std::string>("sdv", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetModuleState);
  cmdProperties.set<std::string>("moduleState", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();


  IASTESTLOG(".%s:\n Testing reaction for valid command EnableAudioProcessingComponent...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetModuleState);
  cmdProperties.set<std::string>("moduleState", "off");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for incomplete command EnableAudioProcessingComponent...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetModuleState);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetModuleState);
  cmdProperties.set<std::string>("moduleState", "on");
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for valid command SetLoudnessFilter...\n",this->test_info_->name());
  IasInt32Vector freqOrderType;
  IasFloat32Vector gainQual;
  freqOrderType.resize(3);
  gainQual.resize(2);
  freqOrderType[0] = 75;
  freqOrderType[1] = 2;
  freqOrderType[2] = eIasFilterTypePeak;
  gainQual[0] = 1.0f;
  gainQual[1] = 2.5f;
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudnessFilter);
  cmdProperties.set("ld.freq_order_type.0", freqOrderType);
  cmdProperties.set("ld.gain_quality.0", gainQual);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  core->process();

  IASTESTLOG(".%s:\n Testing reaction for invalid command SetLoudnessFilter...\n",this->test_info_->name());
  freqOrderType.resize(3);
  gainQual.resize(2);
  freqOrderType[0] = 75;
  freqOrderType[1] = 2;
  freqOrderType[2] = eIasFilterTypePeak;
  gainQual[0] = 1.0f;
  gainQual[1] = 1000.0f;
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudnessFilter);
  cmdProperties.set("ld.freq_order_type.0", freqOrderType);
  cmdProperties.set("ld.gain_quality.0", gainQual);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  gainQual[0]=2.0f;
  gainQual[1] = 2.5f;
  cmdProperties.set("ld.gain_quality.0", gainQual);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasFailed, cmdres);
  core->process();

  gainQual[0] = 1.0f;
  gainQual[1] = 2.5f;

  IASTESTLOG(".%s:\n Testing reaction for valid command GetLoudnessFilter...\n",this->test_info_->name());
  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasGetLoudnessFilter);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  ASSERT_TRUE(returnProperties.hasProperties());
  core->process();

  IasInt32Vector recvFreqOrderType;
  IasFloat32Vector recvGainQual;
  returnProperties.get("ld.freq_order_type.0", &recvFreqOrderType);
  returnProperties.get("ld.gain_quality.0", &recvGainQual);

  EXPECT_EQ(freqOrderType[0], recvFreqOrderType[0]);
  EXPECT_EQ(freqOrderType[1], recvFreqOrderType[1]);
  EXPECT_EQ(freqOrderType[2], recvFreqOrderType[2]);
  EXPECT_EQ(gainQual[0], recvGainQual[0]);
  EXPECT_EQ(gainQual[1], recvGainQual[1]);

  IASTESTLOG(".%s:\n suceeded as expected\n",this->test_info_->name());

  // In this test, mVolumeLoudness does not need to be deleted, because it is deleted
  // together with the mAudioChain, with includes the mVolumeLoudness.
}

TEST_F(IasVolumeTest, setgetLoudnessTable)
{
  IASTESTLOG(".%s:\n Getting Loudness table...\n",this->test_info_->name());
  ASSERT_TRUE(nullptr!=mVolume);
  IasIModuleId *cmdInterface = mVolume->getCmdInterface();
  ASSERT_TRUE(nullptr!=cmdInterface);
  mAudioChain->addAudioComponent(mVolume);
  IasGenericAudioCompCore *core = mVolume->getCore();
  ASSERT_TRUE(nullptr!=core);

  IasAudio::IasInt32Vector ldVolumes0;
  IasAudio::IasInt32Vector ldGains0;

  core->process();

  IASTESTLOG(".%s:\n Try to get loudness table...\n",this->test_info_->name());
  IasProperties cmdProperties;
  IasProperties returnProperties;
  IasIModuleId::IasResult cmdres;
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasGetLoudnessTable);
  cmdProperties.set<IasAudio::IasInt32Vector>("ld.gains.0", ldGains0);
  cmdProperties.set<IasAudio::IasInt32Vector>("ld.volumes.0", ldVolumes0);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);

  ldGains0.resize(0);
  ldVolumes0.resize(0);
  returnProperties.get("ld.gains.0",&ldGains0);
  returnProperties.get("ld.volumes.0",&ldVolumes0);

  ASSERT_TRUE(ldGains0.size() > 1);
  cmdProperties.clearAll();
  IASTESTLOG(".%s:\n Try to set loudness table with previously received...\n",this->test_info_->name());
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudnessTable);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);

  core->process();

  cmdProperties.clearAll();
  cmdProperties.set<int32_t>("cmd", IasVolume::eIasSetLoudnessTable);
  cmdProperties.set<IasAudio::IasInt32Vector>("ld.gains.0", ldGains0);
  cmdProperties.set<IasAudio::IasInt32Vector>("ld.volumes.0", ldVolumes0);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);
  int32_t gainSize = static_cast<int32_t>(ldGains0.size());
  ASSERT_TRUE(gainSize > 1);
  ldGains0.resize(gainSize-1);

  std::cout << "gain table size =" << ldGains0.size() <<std::endl;
  std::cout << "volume table size =" << ldVolumes0.size() <<std::endl;


  cmdProperties.set<IasAudio::IasInt32Vector>("ld.gains.0", ldGains0);
  cmdProperties.set<IasAudio::IasInt32Vector>("ld.volumes.0", ldVolumes0);
  cmdres = cmdInterface->processCmd(cmdProperties, returnProperties);
  ASSERT_EQ(IasIModuleId::eIasOk, cmdres);

}


} // namespace IasAudio
