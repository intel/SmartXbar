/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include "IasDummyConnectionTest.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasSetupHelper.hpp"


namespace IasAudio
{

const int32_t monoInputId = 0;
const int32_t stereoInputId = 1;
const int32_t multiInputId = 2;

const int32_t monoOutputId = 0;
const int32_t stereoOutputId = 1;

void IasDummyConnectionTest::SetUp()
{
  mSmartx = IasSmartX::create();
  ASSERT_TRUE(mSmartx != nullptr);

}

void IasDummyConnectionTest::TearDown()
{
  IasSmartX::destroy(mSmartx);
}


TEST_F(IasDummyConnectionTest, dummySources)
{
  IasISetup *setup = mSmartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);

  IasAudioSourceDevicePtr Input_stereo;
  IasAudioSourceDevicePtr Input_mono;
  IasAudioSourceDevicePtr Input_multi;

  IasAudioDeviceParams sourceParams;

  sourceParams.clockType = IasAudio::eIasClockProvided;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "Input_stereo";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 2048;
  sourceParams.samplerate = 48000;

  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, stereoInputId, &Input_stereo);

  sourceParams.name = "Input_mono";
  sourceParams.numChannels = 1;
  setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, monoInputId, &Input_mono);
  ASSERT_TRUE(setupRes == IasISetup::eIasOk);

  sourceParams.name = "Input_multi";
  sourceParams.numChannels = 6;
  setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, multiInputId, &Input_multi);
  ASSERT_TRUE(setupRes == IasISetup::eIasOk);
  std::string sourceGroupName = "MySourceGroup";

  std::set<int32_t> sourceGroupIds;

  sourceGroupIds.insert(0);
  sourceGroupIds.insert(1);
  sourceGroupIds.insert(2);
  setup->addSourceGroup(sourceGroupName,0.);
  setup->addSourceGroup(sourceGroupName,1);
  setup->addSourceGroup(sourceGroupName,2);

  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockProvided;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "Output_stereo";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 2048;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr Output_stereo;
  IasRoutingZonePtr stereoZone;
  setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, stereoOutputId, &Output_stereo, &stereoZone);
  ASSERT_TRUE(setupRes == IasISetup::eIasOk);

  sinkParams.name = "Output_mono";
   sinkParams.periodSize = 10*2048;
  IasAudioSinkDevicePtr Output_mono;
  IasRoutingZonePtr monoZone;
  setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, monoOutputId, &Output_mono, &monoZone);
  ASSERT_TRUE(setupRes == IasISetup::eIasOk);

  setupRes = setup->addDerivedZone(stereoZone, monoZone);
  ASSERT_TRUE(setupRes == IasISetup::eIasOk);

  IasSourceGroupMap sourceGroupMap = setup->getSourceGroups();
  ASSERT_TRUE(sourceGroupMap.size() == 1);

  IasSourceGroupMap::iterator mapIt;

  for(mapIt=sourceGroupMap.begin();mapIt!=sourceGroupMap.end();mapIt++)
  {
    std::string tmpName = mapIt->first;
    ASSERT_TRUE( tmpName.compare(sourceGroupName) == 0);
    ASSERT_TRUE( mapIt->second.size() == sourceGroupIds.size());
    std::set<int32_t>::iterator setIt1,setIt2;
    for(setIt1=mapIt->second.begin(),setIt2=sourceGroupIds.begin();
        setIt1!=mapIt->second.end() && setIt2!=sourceGroupIds.end();
        setIt1++,setIt2++)
    {
       ASSERT_TRUE( *setIt1 == *setIt2 );
    }

  }

  IasDummySourcesSet dummySources = routing->getDummySources();
  ASSERT_TRUE(dummySources.size()== 0);

  IasSmartX::IasResult smxres = mSmartx->start();
  ASSERT_TRUE(smxres == IasSmartX::eIasFailed);


  routing->connect(stereoInputId,stereoOutputId);
  dummySources = routing->getDummySources();
  ASSERT_TRUE(dummySources.size() == 2);



}

}
