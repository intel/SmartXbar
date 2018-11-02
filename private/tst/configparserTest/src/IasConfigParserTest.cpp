/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file  IasConfigParserTest.cpp
 * @date  2017
 * @brief
 */


#include <string>
#include <cstdio>
#include <cstdlib>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <gtest/gtest.h>
#include <dlt/dlt.h>
#include <IasAudioDataComparer.hpp>
#include "audio/smartx/IasSetupHelper.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/configparser/IasSmartXconfigParser.hpp"
#include "configparser/IasParseHelper.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasRoutingZone.hpp"
#include "core_libraries/foundation/IasTypes.hpp"
#include "core_libraries/foundation/IasDebug.hpp"
#include "boost/filesystem.hpp"

using namespace IasAudio;
namespace fs = boost::filesystem;

static const Ias::String validXmlFilesPath = "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestXmlFiles/2017-12-01/valid/";
static const Ias::String invalidXmlFilesPath = "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestXmlFiles/2017-12-01/invalid/";

static const auto PARSER_SUCCESS = true;


/**
 *
 * The tests for xml generated from FDK Tools,
 * based on version: FDK Tools 1.2.3-MR1-SmartXBar 0.5 main features (Work Package #1)
 *
 * The test try to coverage all funcionality of the Parser :
 * >Create/Edit/Delete source and sink devices
 * >Create/Edit/Delete routing zones
 * >Create/Edit/Delete switch matrix links
 * >Create/Edit/Delete setup links (links between routing zone and sink device)
 * >Configure derived routing zone
 *
 */
Ias::Int32 main(Ias::Int32 argc, Ias::Char **argv)
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  (void)argc;
  (void)argv;
  DLT_REGISTER_APP("TST", "Test Application");
  DLT_VERBOSE_MODE();

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}


namespace IasAudio {

class InitConfigParser : public ::testing::Test
{
  std::vector<Ias::String>   validXmlFiles;
  std::vector<Ias::String> invalidXmlFiles;

  std::vector<Ias::String> getFileList(const std::string& path);

protected:

    void SetUp() override;
    void TearDown() override;

    std::vector<Ias::String> getValidXmlFiles();
    std::vector<Ias::String> getInvalidXmlFiles();

    class WrapperSmartX
    {
      IasAudio::IasSmartX* mSmartx;
    public:

      WrapperSmartX();
      ~WrapperSmartX();

      IasSmartX* getSmartX();
      IasAudioDataComparer getCompareData();
    };

    /*!
     * @brief Create and delete smartx object from xml file
     *
     * @param[in] xml file path
     *
     * @returns smartx params
     */
    IasAudioDataComparer makeCompareData (const char* file)
    {
      WrapperSmartX wrapperSmartX{};
      EXPECT_TRUE(parseConfig(wrapperSmartX.getSmartX(), file));
      return wrapperSmartX.getCompareData();
    };
};


void InitConfigParser::SetUp()
{
  setenv("AUDIO_PLUGIN_DIR", "../../..", true);
  validXmlFiles   = getFileList(validXmlFilesPath);
  invalidXmlFiles = getFileList(invalidXmlFilesPath);
}

void InitConfigParser::TearDown()
{

}

std::vector<Ias::String> InitConfigParser::getValidXmlFiles()
{
  return validXmlFiles;
}

std::vector<Ias::String> InitConfigParser::getInvalidXmlFiles()
{
  return invalidXmlFiles;
}

std::vector<Ias::String> InitConfigParser::getFileList(const std::string& path)
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

InitConfigParser::WrapperSmartX::WrapperSmartX()
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

IasSmartX* InitConfigParser::WrapperSmartX::getSmartX()
{
  return mSmartx;
}

IasAudioDataComparer InitConfigParser::WrapperSmartX::getCompareData()
{
  return IasAudioDataComparer{mSmartx};
}

InitConfigParser::WrapperSmartX::~WrapperSmartX()
{
  const IasSmartX::IasResult smres = mSmartx->stop();
  EXPECT_EQ(IasSmartX::eIasOk, smres);
  IasSmartX::destroy(mSmartx);
  mSmartx = nullptr;
}

TEST_F(InitConfigParser, parseValid)
{
  for(const auto& file : getValidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    EXPECT_EQ(parseConfig(wrapperSmartX.getSmartX(), file.c_str()), PARSER_SUCCESS) << "File : " << file;
    EXPECT_NE(wrapperSmartX.getSmartX(), nullptr);
  }
}

TEST_F(InitConfigParser, compareFilesData)
{
  for(const auto& file : getValidXmlFiles())
  {
    WrapperSmartX* smartXl = new WrapperSmartX{};
    EXPECT_EQ(parseConfig(smartXl->getSmartX(), file.c_str()), PARSER_SUCCESS) << "File : " << file;
    EXPECT_NE(smartXl->getSmartX(), nullptr);
    IasAudioDataComparer l{smartXl->getSmartX()};
    delete smartXl;

    WrapperSmartX* smartXr = new WrapperSmartX{};
    EXPECT_EQ(parseConfig(smartXr->getSmartX(), file.c_str()), PARSER_SUCCESS) << "File : " << file;
    EXPECT_NE(smartXr->getSmartX(), nullptr);
    IasAudioDataComparer r{smartXr->getSmartX()};
    delete smartXr;

    EXPECT_EQ(l,r);
  }
}

TEST_F(InitConfigParser, parseInvalid)
{
  for(const auto& file : getInvalidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    const auto res = parseConfig(wrapperSmartX.getSmartX(), file.c_str());
    EXPECT_NE(res, PARSER_SUCCESS) << "File : " << file;
  }
}

TEST_F(InitConfigParser, nullptrTest)
{
  WrapperSmartX wrapperSmartX{};
  EXPECT_NE(parseConfig(wrapperSmartX.getSmartX(), nullptr), PARSER_SUCCESS);
  EXPECT_NE(parseConfig(nullptr, getValidXmlFiles()[0].c_str()), PARSER_SUCCESS);
}

TEST_F(InitConfigParser, cannotOpenTest)
{
  WrapperSmartX wrapperSmartX{};
  Ias::Bool returnVal = parseConfig(wrapperSmartX.getSmartX(), (validXmlFilesPath + "file-no-exist-5#54$3%5&3").c_str());
  EXPECT_EQ(returnVal, false);
}

TEST_F(InitConfigParser, addExtraSink)
{
  IasAudioSinkDevicePtr sink = nullptr;
  IasRoutingZonePtr     rzn = nullptr;
  IasAudioDeviceParams  params;
  params.clockType = eIasClockProvided;
  params.name = "DummySink";
  params.dataFormat = eIasFormatInt16;
  params.periodSize = 256;
  params.numPeriods = 4;
  params.numChannels = 2;
  params.samplerate = 48000;

  for(const auto file : getValidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    EXPECT_TRUE(parseConfig(wrapperSmartX.getSmartX(), file.c_str()));
    const IasAudioDataComparer l = wrapperSmartX.getCompareData();

    const auto res = IasSetupHelper::createAudioSinkDevice(wrapperSmartX.getSmartX()->setup(), params, 328, &sink, &rzn);
    EXPECT_EQ(res, IasISetup::eIasOk);

    const auto r = wrapperSmartX.getCompareData();
    ASSERT_NE(l,r);
  }
}

TEST_F(InitConfigParser, addExtraSource)
{
  IasAudioSourceDevicePtr source = nullptr;
  IasAudioDeviceParams  params;
  params.clockType = eIasClockProvided;
  params.name = "DummySource";
  params.dataFormat = eIasFormatInt16;
  params.periodSize = 256;
  params.numPeriods = 4;
  params.numChannels = 2;
  params.samplerate = 48000;

  for(const auto file : getValidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    EXPECT_TRUE(parseConfig(wrapperSmartX.getSmartX(), file.c_str()));
    const auto l = wrapperSmartX.getCompareData();

    const auto res = IasSetupHelper::createAudioSourceDevice(wrapperSmartX.getSmartX()->setup(), params, 328, &source);
    EXPECT_EQ(res, IasISetup::eIasOk);
    const auto r = wrapperSmartX.getCompareData();

    ASSERT_NE(l,r);
  }
}

TEST_F(InitConfigParser, removeSink)
{
  for(const auto file : getValidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    EXPECT_TRUE(parseConfig(wrapperSmartX.getSmartX(), file.c_str()));
    const IasAudioDataComparer l = wrapperSmartX.getCompareData();

    const auto devices = wrapperSmartX.getSmartX()->setup()->getAudioSinkDevices();

    if(!devices.empty())
    {
      IasSetupHelper::deleteSinkDevice(wrapperSmartX.getSmartX()->setup(), devices[0]->getName());
      const auto r = wrapperSmartX.getCompareData();
      ASSERT_NE(l,r);
    }
  }
}


TEST_F(InitConfigParser, removeSource)
{
  for(const auto file : getValidXmlFiles())
  {
    WrapperSmartX wrapperSmartX{};
    EXPECT_TRUE(parseConfig(wrapperSmartX.getSmartX(), file.c_str()));
    const IasAudioDataComparer l = wrapperSmartX.getCompareData();

    const auto devices = wrapperSmartX.getSmartX()->setup()->getAudioSinkDevices();

    if(!devices.empty())
    {
      IasSetupHelper::deleteSinkDevice(wrapperSmartX.getSmartX()->setup(), devices[0]->getName());
      const auto r = wrapperSmartX.getCompareData();
      ASSERT_NE(l,r);
    }
  }
}

TEST_F(InitConfigParser, checkData_sxb_topology_1_xml)
{
  const Ias::String file {validXmlFilesPath + "sxb_topology_1.xml"};

  WrapperSmartX smartX{};
  EXPECT_EQ(parseConfig(smartX.getSmartX(), file.c_str()), PARSER_SUCCESS);
  EXPECT_NE(smartX.getSmartX(), nullptr);

  // Check source params
  const auto src = smartX.getSmartX()->setup()->getAudioSourceDevice("avb_source");
  EXPECT_NE(src, nullptr);
  EXPECT_EQ(src->getName(), "avb_source");
  EXPECT_EQ(src->getDeviceParams()->clockType, IasClockType::eIasClockProvided);
  EXPECT_EQ(src->getDeviceParams()->numChannels, 4);
  EXPECT_EQ(src->getDeviceParams()->periodSize, 192);
  EXPECT_EQ(src->getDeviceParams()->numPeriods, 4);
  EXPECT_EQ(src->getDeviceParams()->numPeriodsAsrcBuffer, 4);

  // Check sink params
  const auto sink = smartX.getSmartX()->setup()->getAudioSinkDevice("hw:0,0");
  EXPECT_NE(sink, nullptr);
  EXPECT_EQ(sink->getName(), "hw:0,0");
  EXPECT_EQ(sink->getDeviceParams()->clockType, IasClockType::eIasClockReceived);
  EXPECT_EQ(sink->getDeviceParams()->numChannels, 4);
  EXPECT_EQ(sink->getDeviceParams()->periodSize, 32);
  EXPECT_EQ(sink->getDeviceParams()->numPeriods, 4);
  EXPECT_EQ(sink->getDeviceParams()->numPeriodsAsrcBuffer, 4);

}


} /* namespace IasAudio */



