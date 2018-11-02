/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasDiagnosticTest.cpp
 */

#include <iostream>
#include <dlt/dlt_types.h>

#include "gtest/gtest.h"
#include "core_libraries/foundation/IasTypes.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "diagnostic/IasDiagnostic.hpp"
#include "smartx/IasConfigFile.hpp"

using namespace IasAudio;
using namespace std;

#ifndef SMARTX_CONFIG_DIR
#define SMARTX_CONFIG_DIR "./"
#endif

class IasDiagnosticTest : public ::testing::Test
{
 protected:

  virtual void SetUp()
  {
  }

  virtual void TearDown()
  {
  }
};

int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(true);
  IasAudio::IasAudioLogging::addDltContextItem("global", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}

TEST_F(IasDiagnosticTest, basic_ok)
{
  IasDiagnostic* diag = IasDiagnostic::getInstance();
  ASSERT_TRUE(diag != nullptr);
  diag->setConfigParameters(500, 18);
  IasDiagnosticStream::IasParams params;
  params.deviceName = "392_snk_I_1";
  params.portName = "392_rzn_port_1";
  params.periodTime = 5333;
  params.errorThreshold = 2;
  params.copyTo = "/tmp/transfer";

  IasDiagnosticStreamPtr diagStream = nullptr;
  IasDiagnostic::IasResult res = diag->registerStream(params, nullptr);
  ASSERT_EQ(IasDiagnostic::eIasFailed, res);
  ASSERT_TRUE(diagStream == nullptr);
  res = diag->registerStream(params, &diagStream);
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  ASSERT_TRUE(diagStream != nullptr);
  diagStream->writeAlsaHandlerData(0xAFFEAFFEAFFEAFFE, 0x12C0FFEE12C0FFEE, 0xBEEFBEEFBEEFBEEF, 0xAFFEAFFEAFFEAFFE, 0x12C0FFEE, 0xBEEFBEEF, 1.0f);
  res = diag->startStream("392_rzn_port_1");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  res = diag->startStream("392_rzn_port_1");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  uint64_t testData64[][4] = {
    {0x0000000000000000, 0x1111111111111111, 0x2222222222222222, 0x3333333333333333},
    {0x1111111111111111, 0x2222222222222222, 0x3333333333333333, 0x4444444444444444},
    {0x2222222222222222, 0x3333333333333333, 0x4444444444444444, 0x5555555555555555},
    {0x3333333333333333, 0x4444444444444444, 0x5555555555555555, 0x6666666666666666},
    {0x4444444444444444, 0x5555555555555555, 0x6666666666666666, 0x7777777777777777},
    {0x5555555555555555, 0x6666666666666666, 0x7777777777777777, 0x8888888888888888},
    {0x6666666666666666, 0x7777777777777777, 0x8888888888888888, 0x9999999999999999},
    {0x7777777777777777, 0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA},
    {0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB},
    {0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC},
    {0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD},
    {0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE},
    {0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF},
    {0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000},
    {0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111},
    {0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111, 0x2222222222222222},
  };
  uint32_t testData32[][2] = {
    {0x00000000, 0x11111111},
    {0x11111111, 0x22222222},
    {0x22222222, 0x33333333},
    {0x33333333, 0x44444444},
    {0x44444444, 0x55555555},
    {0x55555555, 0x66666666},
    {0x66666666, 0x77777777},
    {0x77777777, 0x88888888},
    {0x88888888, 0x99999999},
    {0x99999999, 0xAAAAAAAA},
    {0xAAAAAAAA, 0xBBBBBBBB},
    {0xBBBBBBBB, 0xCCCCCCCC},
    {0xCCCCCCCC, 0xDDDDDDDD},
    {0xDDDDDDDD, 0xEEEEEEEE},
    {0xEEEEEEEE, 0xFFFFFFFF},
    {0xFFFFFFFF, 0x00000000},
  };
  float_t testDataFl[] = {
    0.0f,
    1.0f,
    2.0f,
    3.0f,
    4.0f,
    5.0f,
    6.0f,
    7.0f,
    8.0f,
    9.0f,
    10.0f,
    11.0f,
    12.0f,
    13.0f,
    14.0f,
    15.0f,
  };
  if (diagStream->isStarted() == true)
  {
    for (int iterations = 0; iterations < 100; ++iterations)
    {
      for (int index = 0; index < 16; ++index)
      {
        diagStream->writeAlsaHandlerData(testData64[index][0],
                                         testData64[index][1],
                                         testData64[index][2],
                                         testData64[index][3],
                                         testData32[index][0],
                                         testData32[index][1],
                                         testDataFl[index]);
      }
      diagStream->errorOccurred();
    }
  }
  res = diag->stopStream("392_rzn_port_1");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  res = diag->stopStream("392_rzn_port_1");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
}

TEST_F(IasDiagnosticTest, dont_stop)
{
  IasDiagnostic* diag = IasDiagnostic::getInstance();
  ASSERT_TRUE(diag != nullptr);
  diag->setConfigParameters(500, 18);
  IasDiagnosticStream::IasParams params;
  params.deviceName = "392_snk_I_2";
  params.portName = "392_rzn_port_2";
  params.periodTime = 5333;
  params.errorThreshold = 2;
  params.copyTo = "/tmp/transfer";

  IasDiagnosticStreamPtr diagStream = nullptr;
  IasDiagnostic::IasResult res = diag->registerStream(params, nullptr);
  ASSERT_EQ(IasDiagnostic::eIasFailed, res);
  ASSERT_TRUE(diagStream == nullptr);
  res = diag->registerStream(params, &diagStream);
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  ASSERT_TRUE(diagStream != nullptr);
  diagStream->writeAlsaHandlerData(0xAFFEAFFEAFFEAFFE, 0x12C0FFEE12C0FFEE, 0xBEEFBEEFBEEFBEEF, 0xAFFEAFFEAFFEAFFE, 0x12C0FFEE, 0xBEEFBEEF, 1.0f);
  res = diag->startStream("392_rzn_port_2");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  res = diag->startStream("392_rzn_port_2");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  uint64_t testData64[][4] = {
    {0x0000000000000000, 0x123456789ABCDEF0, 0x2222222222222222, 0x3333333333333333},
    {0x1111111111111111, 0x2222222222222222, 0x3333333333333333, 0x4444444444444444},
    {0x2222222222222222, 0x3333333333333333, 0x4444444444444444, 0x5555555555555555},
    {0x3333333333333333, 0x4444444444444444, 0x5555555555555555, 0x6666666666666666},
    {0x4444444444444444, 0x5555555555555555, 0x6666666666666666, 0x7777777777777777},
    {0x5555555555555555, 0x6666666666666666, 0x7777777777777777, 0x8888888888888888},
    {0x6666666666666666, 0x7777777777777777, 0x8888888888888888, 0x9999999999999999},
    {0x7777777777777777, 0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA},
    {0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB},
    {0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC},
    {0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD},
    {0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE},
    {0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF},
    {0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000},
    {0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111},
    {0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111, 0x2222222222222222},
  };
  uint32_t testData32[][2] = {
    {0x00000000, 0x11111111},
    {0x11111111, 0x22222222},
    {0x22222222, 0x33333333},
    {0x33333333, 0x44444444},
    {0x44444444, 0x55555555},
    {0x55555555, 0x66666666},
    {0x66666666, 0x77777777},
    {0x77777777, 0x88888888},
    {0x88888888, 0x99999999},
    {0x99999999, 0xAAAAAAAA},
    {0xAAAAAAAA, 0xBBBBBBBB},
    {0xBBBBBBBB, 0xCCCCCCCC},
    {0xCCCCCCCC, 0xDDDDDDDD},
    {0xDDDDDDDD, 0xEEEEEEEE},
    {0xEEEEEEEE, 0xFFFFFFFF},
    {0xFFFFFFFF, 0x00000000},
  };
  float_t testDataFl[] = {
    0.0f,
    1.0f,
    2.0f,
    3.0f,
    4.0f,
    5.0f,
    6.0f,
    7.0f,
    8.0f,
    9.0f,
    10.0f,
    11.0f,
    12.0f,
    13.0f,
    14.0f,
    15.0f,
  };
  if (diagStream->isStarted() == true)
  {
    for (int iterations = 0; iterations < 100; ++iterations)
    {
      for (int index = 0; index < 16; ++index)
      {
        diagStream->writeAlsaHandlerData(testData64[index][0],
                                         testData64[index][1],
                                         testData64[index][2],
                                         testData64[index][3],
                                         testData32[index][0],
                                         testData32[index][1],
                                         testDataFl[index]);
      }
      diagStream->errorOccurred();
    }
  }
  diagStream->isStopped();
}

TEST_F(IasDiagnosticTest, config_ok)
{
  setenv("SMARTX_CFG_DIR", Ias::String(Ias::String(SMARTX_CONFIG_DIR) + "config_ok").c_str(), true);
  IasConfigFile *cfgFile = IasConfigFile::getInstance();
  ASSERT_TRUE(cfgFile != nullptr);
  cfgFile->load();

  const IasConfigFile::IasAlsaHandlerDiagnosticParams* params = cfgFile->getAlsaHandlerDiagParams("invalid_sink_device");
  ASSERT_TRUE(params == nullptr);
  params = cfgFile->getAlsaHandlerDiagParams("392_snk_I");
  ASSERT_TRUE(params != nullptr);
  ASSERT_EQ("392_rzn_port", params->portName);
  ASSERT_EQ("/tmp/transfer", params->copyTo);
  ASSERT_EQ(2, params->errorThreshold);
  ASSERT_EQ(500, cfgFile->getLogPeriodTime());
  ASSERT_EQ(18, cfgFile->getNumEntriesPerMsg());
}

TEST_F(IasDiagnosticTest, check_strftime)
{
  char currentTime[20] = {'\0'};
  std::cout << "Time string  = ***" << std::string(currentTime) << "***" << std::endl;
}

TEST_F(IasDiagnosticTest, min_config_ok)
{
  setenv("SMARTX_CFG_DIR", Ias::String(Ias::String(SMARTX_CONFIG_DIR) + "min_config_ok").c_str(), true);
  IasConfigFile *cfgFile = IasConfigFile::getInstance();
  ASSERT_TRUE(cfgFile != nullptr);
  cfgFile->load();

  const IasConfigFile::IasAlsaHandlerDiagnosticParams* cfgParams = cfgFile->getAlsaHandlerDiagParams("invalid_sink_device");
  ASSERT_TRUE(cfgParams == nullptr);
  cfgParams = cfgFile->getAlsaHandlerDiagParams("392_snk_I_4");
  ASSERT_TRUE(cfgParams != nullptr);
  ASSERT_EQ("392_rzn_port_4", cfgParams->portName);
  ASSERT_EQ("log", cfgParams->copyTo);
  ASSERT_EQ(2, cfgParams->errorThreshold);
  ASSERT_EQ(500, cfgFile->getLogPeriodTime());
  ASSERT_EQ(18, cfgFile->getNumEntriesPerMsg());

  IasDiagnostic* diag = IasDiagnostic::getInstance();
  ASSERT_TRUE(diag != nullptr);
  diag->setConfigParameters(cfgFile->getLogPeriodTime(), cfgFile->getNumEntriesPerMsg());
  IasDiagnosticStream::IasParams diagParams;
  diagParams.deviceName = "392_snk_I_4";
  diagParams.periodTime = 5333;
  diagParams.portName = cfgParams->portName;
  diagParams.errorThreshold = cfgParams->errorThreshold;
  diagParams.copyTo = cfgParams->copyTo;

  IasDiagnosticStreamPtr diagStream = nullptr;
  IasDiagnostic::IasResult res = diag->registerStream(diagParams, nullptr);
  ASSERT_EQ(IasDiagnostic::eIasFailed, res);
  ASSERT_TRUE(diagStream == nullptr);
  res = diag->registerStream(diagParams, &diagStream);
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  ASSERT_TRUE(diagStream != nullptr);
  diagStream->writeAlsaHandlerData(0xAFFEAFFEAFFEAFFE, 0x12C0FFEE12C0FFEE, 0xBEEFBEEFBEEFBEEF, 0xAFFEAFFEAFFEAFFE, 0x12C0FFEE, 0xBEEFBEEF, 1.0f);
  res = diag->startStream("392_rzn_port_4");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  res = diag->startStream("392_rzn_port_4");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  uint64_t testData64[][4] = {
    {0x0123456789ABCDEF, 0x1111111111111111, 0x2222222222222222, 0x3333333333333333},
    {0x1111111111111111, 0x2222222222222222, 0x3333333333333333, 0x4444444444444444},
    {0x2222222222222222, 0x3333333333333333, 0x4444444444444444, 0x5555555555555555},
    {0x3333333333333333, 0x4444444444444444, 0x5555555555555555, 0x6666666666666666},
    {0x4444444444444444, 0x5555555555555555, 0x6666666666666666, 0x7777777777777777},
    {0x5555555555555555, 0x6666666666666666, 0x7777777777777777, 0x8888888888888888},
    {0x6666666666666666, 0x7777777777777777, 0x8888888888888888, 0x9999999999999999},
    {0x7777777777777777, 0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA},
    {0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB},
    {0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC},
    {0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD},
    {0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE},
    {0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF},
    {0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000},
    {0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111},
    {0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111, 0x2222222222222222},
  };
  uint32_t testData32[][2] = {
    {0x00000000, 0x11111111},
    {0x11111111, 0x22222222},
    {0x22222222, 0x33333333},
    {0x33333333, 0x44444444},
    {0x44444444, 0x55555555},
    {0x55555555, 0x66666666},
    {0x66666666, 0x77777777},
    {0x77777777, 0x88888888},
    {0x88888888, 0x99999999},
    {0x99999999, 0xAAAAAAAA},
    {0xAAAAAAAA, 0xBBBBBBBB},
    {0xBBBBBBBB, 0xCCCCCCCC},
    {0xCCCCCCCC, 0xDDDDDDDD},
    {0xDDDDDDDD, 0xEEEEEEEE},
    {0xEEEEEEEE, 0xFFFFFFFF},
    {0xFFFFFFFF, 0x00000000},
  };
  float_t testDataFl[] = {
    0.0f,
    1.0f,
    2.0f,
    3.0f,
    4.0f,
    5.0f,
    6.0f,
    7.0f,
    8.0f,
    9.0f,
    10.0f,
    11.0f,
    12.0f,
    13.0f,
    14.0f,
    15.0f,
  };
  if (diagStream->isStarted() == true)
  {
    for (int iterations = 0; iterations < 100; ++iterations)
    {
      for (int index = 0; index < 16; ++index)
      {
        diagStream->writeAlsaHandlerData(testData64[index][0],
                                         testData64[index][1],
                                         testData64[index][2],
                                         testData64[index][3],
                                         testData32[index][0],
                                         testData32[index][1],
                                         testDataFl[index]);
      }
      diagStream->errorOccurred();
    }
  }
  res = diag->stopStream("392_rzn_port_4");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  res = diag->stopStream("392_rzn_port_4");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  // Wait until the diag stream is really stopped
  while (diagStream->isStopped() == false);
  // Wait until the log thread finished
  diag->isThreadFinished();
}

TEST_F(IasDiagnosticTest, trigger_two_files)
{
  IasDiagnostic* diag = IasDiagnostic::getInstance();
  ASSERT_TRUE(diag != nullptr);
  diag->setConfigParameters(1, 18);
  IasDiagnosticStream::IasParams diagParams;
  diagParams.deviceName = "392_snk_I_5";
  diagParams.periodTime = 5333;
  diagParams.portName = "392_rzn_port_5";
  diagParams.errorThreshold = 0;
  diagParams.copyTo = "log";

  IasDiagnosticStreamPtr diagStream = nullptr;
  IasDiagnostic::IasResult res = diag->registerStream(diagParams, nullptr);
  ASSERT_EQ(IasDiagnostic::eIasFailed, res);
  ASSERT_TRUE(diagStream == nullptr);
  res = diag->registerStream(diagParams, &diagStream);
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  ASSERT_TRUE(diagStream != nullptr);
  diagStream->writeAlsaHandlerData(0xAFFEAFFEAFFEAFFE, 0x12C0FFEE12C0FFEE, 0xBEEFBEEFBEEFBEEF, 0xAFFEAFFEAFFEAFFE, 0x12C0FFEE, 0xBEEFBEEF, 1.0f);
  res = diag->startStream("392_rzn_port_5");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  uint64_t testData64[][4] = {
    {0x0123456789ABCDEF, 0x1111111111111111, 0x2222222222222222, 0x3333333333333333},
    {0x1111111111111111, 0x2222222222222222, 0x3333333333333333, 0x4444444444444444},
    {0x2222222222222222, 0x3333333333333333, 0x4444444444444444, 0x5555555555555555},
    {0x3333333333333333, 0x4444444444444444, 0x5555555555555555, 0x6666666666666666},
    {0x4444444444444444, 0x5555555555555555, 0x6666666666666666, 0x7777777777777777},
    {0x5555555555555555, 0x6666666666666666, 0x7777777777777777, 0x8888888888888888},
    {0x6666666666666666, 0x7777777777777777, 0x8888888888888888, 0x9999999999999999},
    {0x7777777777777777, 0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA},
    {0x8888888888888888, 0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB},
    {0x9999999999999999, 0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC},
    {0xAAAAAAAAAAAAAAAA, 0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD},
    {0xBBBBBBBBBBBBBBBB, 0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE},
    {0xCCCCCCCCCCCCCCCC, 0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF},
    {0xDDDDDDDDDDDDDDDD, 0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000},
    {0xEEEEEEEEEEEEEEEE, 0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111},
    {0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x1111111111111111, 0x2222222222222222},
  };
  uint32_t testData32[][2] = {
    {0x00000000, 0x11111111},
    {0x11111111, 0x22222222},
    {0x22222222, 0x33333333},
    {0x33333333, 0x44444444},
    {0x44444444, 0x55555555},
    {0x55555555, 0x66666666},
    {0x66666666, 0x77777777},
    {0x77777777, 0x88888888},
    {0x88888888, 0x99999999},
    {0x99999999, 0xAAAAAAAA},
    {0xAAAAAAAA, 0xBBBBBBBB},
    {0xBBBBBBBB, 0xCCCCCCCC},
    {0xCCCCCCCC, 0xDDDDDDDD},
    {0xDDDDDDDD, 0xEEEEEEEE},
    {0xEEEEEEEE, 0xFFFFFFFF},
    {0xFFFFFFFF, 0x00000000},
  };
  float_t testDataFl[] = {
    1.23456f,
    2.34567f,
    3.45678f,
    4.56789f,
    5.67890f,
    6.78901f,
    7.89012f,
    8.90123f,
    9.01234f,
    10.12345f,
    11.23456f,
    12.34567f,
    13.45678f,
    14.56789f,
    15.67890f,
    16.78901f,
  };
  if (diagStream->isStarted() == true)
  {
    for (int iterations = 0; iterations < 100; ++iterations)
    {
      for (int index = 0; index < 16; ++index)
      {
        diagStream->writeAlsaHandlerData(testData64[index][0],
                                         testData64[index][1],
                                         testData64[index][2],
                                         testData64[index][3],
                                         testData32[index][0],
                                         testData32[index][1],
                                         testDataFl[index]);
      }
    }
  }
  res = diag->stopStream("392_rzn_port_5");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  ASSERT_EQ(true, diagStream->isStopped());
  res = diag->startStream("392_rzn_port_5");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  if (diagStream->isStarted() == true)
  {
    for (int iterations = 0; iterations < 100; ++iterations)
    {
      for (int index = 0; index < 16; ++index)
      {
        diagStream->writeAlsaHandlerData(testData64[index][0],
                                         testData64[index][1],
                                         testData64[index][2],
                                         testData64[index][3],
                                         testData32[index][0],
                                         testData32[index][1],
                                         testDataFl[index]);
      }
    }
  }
  res = diag->stopStream("392_rzn_port_5");
  ASSERT_EQ(IasDiagnostic::eIasOk, res);
  // Wait until the log thread finished
  diag->isThreadFinished();
}
