/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created 2016
 */

#include "gtest/gtest.h"
#include "internal/audio/common/IasAudioLogging.hpp"

using namespace IasAudio;

int main(int argc, char* argv[])
{
  IasAudioLogging::registerDltApp(true);
  IasAudioLogging::addDltContextItem("CFG", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("SMX", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("RZN", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("SMW", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("ROU", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);
  IasAudioLogging::addDltContextItem("RB", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);

  IasAudioLogging::addDltContextItem("TST", DLT_LOG_ERROR, DLT_TRACE_STATUS_ON);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
