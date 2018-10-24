/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasPipelineTestMain.cpp
 *
 *  Created 2016
 */

#include <gtest/gtest.h>
#include "internal/audio/common/IasAudioLogging.hpp"


static void printWelcomeMessage()
{
  std::cout << "===================================" << std::endl;
  std::cout << "Unit test for the class IasPipeline" << std::endl;
  std::cout << "===================================" << std::endl;
}


int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(true);
  IasAudio::IasAudioLogging::addDltContextItem("global", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);
  ::testing::InitGoogleTest(&argc, argv);

  printWelcomeMessage();

  int result = RUN_ALL_TESTS();
  return result;
}
