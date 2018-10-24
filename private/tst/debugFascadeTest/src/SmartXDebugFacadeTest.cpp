/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file  SmartXDebugFascadeTest.cpp
 * @date  2017
 * @brief
 */

#include <functional>
#include <gtest/gtest.h>
#include <dlt/dlt.h>
#include "boost/filesystem.hpp"
#include "audio/configparser/IasSmartXDebugFacade.hpp"
#include "audio/configparser/IasSmartXconfigParser.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "SmartXDebugFacadeTest.hpp"
#include "rtprocessingfwx/IasAudioChain.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"

#include "gtest/gtest.h"
#include <sndfile.h>
#include <string.h>
#include <iostream>
#include "audio/testfwx/IasTestFramework.hpp"
#include "audio/testfwx/IasTestFrameworkSetup.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "model/IasAudioPin.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"
#include "mixer/IasMixerCore.hpp"
#include "audio/mixerx/IasMixerCmd.hpp"
#include "audio/smartx/IasSetupHelper.hpp"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>

using namespace std;
using namespace IasAudio;
namespace fs = boost::filesystem;

static const Ias::String validXmlFilesPath =   "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestXmlFiles/2017-12-01/valid/";
static const auto PARSER_SUCCESS = true;

namespace IasAudio {

std::vector<Ias::String> SmartXDebugFacadeTest::getValidXmlFiles()
{
  return validXmlFiles;
}

std::vector<Ias::String> SmartXDebugFacadeTest::getFileList(const std::string& path)
{
  std::vector<Ias::String> files;
  if (!path.empty())
  {
    fs::path apk_path(path);
    fs::recursive_directory_iterator end;

    for (fs::recursive_directory_iterator i(apk_path); i != end; ++i)
    {
      const fs::path cp = (*i);
      files.emplace_back(cp.string());
    }
  }
  return files;
}

void SmartXDebugFacadeTest::SetUp()
{
  setenv("AUDIO_PLUGIN_DIR", "../../..", true);
  validXmlFiles = getFileList(validXmlFilesPath);
}

void SmartXDebugFacadeTest::TearDown()
{

}

SmartXDebugFacadeTest::WrapperSmartX::WrapperSmartX()
{
  mSmartx = IasAudio::IasSmartX::create();
  if (mSmartx == nullptr)
  {
    EXPECT_TRUE(false) << "Create smartx error\n";
  }
  if (mSmartx->isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, SMARTX_API_PATCH) == false)
  {
    std::cerr << "SmartX API version does not match" << std::endl;
    IasAudio::IasSmartX::destroy(mSmartx);
    EXPECT_TRUE(false);
  }
}

IasSmartX* SmartXDebugFacadeTest::WrapperSmartX::getSmartX()
{
  return mSmartx;
}

SmartXDebugFacadeTest::WrapperSmartX::~WrapperSmartX()
{
  const IasSmartX::IasResult smres = mSmartx->stop();
  EXPECT_EQ(IasSmartX::eIasOk, smres);
  IasSmartX::destroy(mSmartx);
  mSmartx = nullptr;
}

TEST_F(SmartXDebugFacadeTest, ValidTopologyXmlAllocationFails)
{
  for(int i = 0; i < 4; i++)
  {
    const char* file = "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestXmlFiles/2017-12-01/valid/pipeline_sxb_topology_06.xml";
    WrapperSmartX wrapperSmartX{};
    auto smartx = wrapperSmartX.getSmartX();
    EXPECT_EQ(parseConfig(smartx, file), PARSER_SUCCESS) << "File : " << file;
    EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);

    IasSmartXDebugFacade debugFascade {smartx};
    Ias::String topology;
    const auto result = debugFascade.getSmartxTopology(topology);
    EXPECT_EQ(IasSmartXDebugFacade::IasResult::eIasFailed, result);
  }
}

TEST_F(SmartXDebugFacadeTest, ValidTopology)
{
  for(const auto& file : getValidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    auto smartx = wrapperSmartX.getSmartX();
    EXPECT_EQ(parseConfig(smartx, file.c_str()), PARSER_SUCCESS) << "File : " << file;
    EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);

    IasSmartXDebugFacade debugFascade {smartx};
    Ias::String topology;
    const auto result = debugFascade.getSmartxTopology(topology);
    EXPECT_EQ(IasSmartXDebugFacade::IasResult::eIasOk, result);
    EXPECT_NE("",topology);
  }
}

TEST_F(SmartXDebugFacadeTest, getVersion)
{
  WrapperSmartX wrapperSmartX{};
  auto smartx = wrapperSmartX.getSmartX();
  auto file = getValidXmlFiles()[0].c_str();
  EXPECT_EQ(parseConfig(smartx, file), PARSER_SUCCESS) << "File : " << file;
  EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);

  IasSmartXDebugFacade debugFascade {smartx};
  const auto result = debugFascade.getVersion();
  EXPECT_NE("", result);
}

TEST_F(SmartXDebugFacadeTest, setParameterFailed)
{
  WrapperSmartX wrapperSmartX{};
  auto smartx = wrapperSmartX.getSmartX();
  auto file = getValidXmlFiles()[0].c_str();

  EXPECT_EQ(parseConfig(smartx, file), PARSER_SUCCESS) << "File : " << file;
  EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);

  IasSmartXDebugFacade debugFascade {smartx};
  IasProperties properties;
  const auto result = debugFascade.setParameters("invalid", properties);
  EXPECT_EQ(IasSmartXDebugFacade::IasResult::eIasFailed, result);
}

TEST_F(SmartXDebugFacadeTest, getParameterFailed)
{
  WrapperSmartX wrapperSmartX{};
  auto smartx = wrapperSmartX.getSmartX();
  auto file = getValidXmlFiles()[0].c_str();

  EXPECT_EQ(parseConfig(smartx, file), PARSER_SUCCESS) << "File : " << file;
  EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);

  IasSmartXDebugFacade debugFascade {smartx};
  IasProperties properties;
  const auto result = debugFascade.getParameters("invalid", properties);
  EXPECT_EQ(IasSmartXDebugFacade::IasResult::eIasFailed, result);
}

TEST_F(SmartXDebugFacadeTest, setParameterOk)
{
  WrapperSmartX wrapperSmartX{};
  auto smartx = wrapperSmartX.getSmartX();

  EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);

  auto setup = smartx->setup();

  IasSmartXDebugFacade debugFascade {smartx};

  IasRoutingZonePtr rzn = nullptr;

  // create sink device
  IasAudioDeviceParams sinkParam;
  sinkParam.clockType = IasAudio::eIasClockProvided;
  sinkParam.dataFormat = IasAudio::eIasFormatInt16;
  sinkParam.name = "mono";
  sinkParam.numChannels = 1;
  sinkParam.numPeriods = 4;
  sinkParam.periodSize = 192;
  sinkParam.samplerate = 48000;
  IasAudioSinkDevicePtr monoSink;
  auto setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParam, 0, &monoSink,&rzn);
  EXPECT_EQ(setupRes, IasISetup::eIasOk) << "Error creating " << sinkParam.name << ": " << toString(setupRes) << "\n";

  // Create the pipeline
  IasPipelineParams pipelineParams;
  pipelineParams.name ="ExamplePipeline";
  pipelineParams.samplerate = sinkParam.samplerate;
  pipelineParams.periodSize = sinkParam.periodSize;

  IasPipelinePtr pipeline = nullptr;
  IasISetup::IasResult res = setup->createPipeline(pipelineParams, &pipeline);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating example pipeline (please review DLT output for details)\n";

  // create pin
  IasAudioPinPtr pipelineInputMonoPin = nullptr;
  IasAudioPinPtr monoPin = nullptr;
  IasAudioPinParams pipelineInputMonoParams;
  pipelineInputMonoParams.name = "pipeInputMono";
  pipelineInputMonoParams.numChannels = 1;

  res = setup->createAudioPin(pipelineInputMonoParams, &pipelineInputMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating pipeline input mono pin\n";

  /// create volume module
  IasProcessingModuleParams volumeModuleParams;
  volumeModuleParams.typeName     = "ias.volume";
  volumeModuleParams.instanceName = "VolumeLoudness";
  IasProcessingModulePtr volume = nullptr;

  IasAudioPinPtr volumeMonoPin = nullptr;
  IasAudioPinParams volumeMonoPinParams;
  volumeMonoPinParams.name = "Volume_InOutMonoPin";
  volumeMonoPinParams.numChannels = 1;

  res = setup->createProcessingModule(volumeModuleParams, &volume);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating volume module\n";

  //// create volume loundness
  IasProperties volumeProperties;
  volumeProperties.set<Ias::Int32>("numFilterBands",1);
  IasStringVector activePins;
  activePins.push_back("Volume_InOutMonoPin");
  volumeProperties.set("activePinsForBand.0", activePins);
  Ias::Int32 tempVol = 0;
  Ias::Int32 tempGain = 0;
  IasInt32Vector ldGains;
  IasInt32Vector ldVolumes;
  for(Ias::UInt32 i=0; i< 8; i++)
  {
    ldVolumes.push_back(tempVol);
    ldGains.push_back(tempGain);
    tempVol-= 60;
    tempGain+= 30;
  }
  volumeProperties.set("ld.volumes.0", ldVolumes);
  volumeProperties.set("ld.gains.0", ldGains);
  volumeProperties.set("ld.volumes.1", ldVolumes);
  volumeProperties.set("ld.gains.1", ldGains);
  volumeProperties.set("ld.volumes.2", ldVolumes);
  volumeProperties.set("ld.gains.2", ldGains);

  IasInt32Vector freqOrderType;
  IasFloat32Vector gainQual;
  freqOrderType.resize(3);
  gainQual.resize(2);
  freqOrderType[0] = 100;
  freqOrderType[1] = 2;
  freqOrderType[2] = eIasFilterTypeLowShelving;
  gainQual[0] = 1.0f; // gain
  gainQual[1] = 2.0f; // quality
  volumeProperties.set("ld.freq_order_type.0", freqOrderType);
  volumeProperties.set("ld.gain_quality.0", gainQual);
  freqOrderType[0] = 8000;
  freqOrderType[1] = 2;
  freqOrderType[2] = eIasFilterTypeHighShelving;
  gainQual[0] = 1.0f; // gain
  gainQual[1] = 2.0f; // quality
  volumeProperties.set("ld.freq_order_type.1", freqOrderType);
  volumeProperties.set("ld.gain_quality.1", gainQual);

  setup->setProperties(volume,volumeProperties);

  res = setup->addProcessingModule(pipeline,volume);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding volume module to pipeline\n";

  res = setup->createAudioPin(volumeMonoPinParams,&volumeMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating volume mono pin\n";

  res = setup->addAudioInOutPin(volume, volumeMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding volume mono pin\n";

  res = setup->addAudioInputPin(pipeline,pipelineInputMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding pipeline input mono pin\n";

  res = setup->link(pipelineInputMonoPin, volumeMonoPin, eIasAudioPinLinkTypeImmediate);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error linking pipe input to mono volume mono input\n";

  res = setup->initPipelineAudioChain(pipeline);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error init pipeline chain - Please check DLT output for additional info\n";

  res = setup->addPipeline(rzn, pipeline);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding pipeline to zone\n";

  IasProperties volCmdProperties;
  volCmdProperties.set("cmd",static_cast<Ias::Int32>(IasAudio::IasVolume::eIasSetVolume));

  volCmdProperties.set<Ias::String>("pin","Volume_InOutMonoPin");
  volCmdProperties.set("volume",0);
  IasAudio::IasInt32Vector ramp;
  ramp.push_back(100);
  ramp.push_back(0);
  volCmdProperties.set("ramp",ramp);

  Ias::Int32 minVol = 2;
  Ias::Int32 maxVol = 10;
  volCmdProperties.set<Ias::Int32>("cmd", 201);
  volCmdProperties.set("MinVol", minVol);
  volCmdProperties.set("MaxVol", maxVol);

  IasProperties returnProps;
  auto result = debugFascade.setParameters("VolumeLoudness", volCmdProperties);
  EXPECT_EQ(IasSmartXDebugFacade::IasResult::eIasOk, result);


  IasProperties cmdReturnProperties;
  cmdReturnProperties.set<Ias::Int32>("cmd", 200);
  result = debugFascade.getParameters("VolumeLoudness", cmdReturnProperties);
  EXPECT_EQ(IasSmartXDebugFacade::IasResult::eIasOk, result);

}


} /* namespace IasAudio */



