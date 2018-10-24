/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasTestFrameworkTestMain.cpp
 * @date  2017
 * @brief Main function for the unit test of the test framework.
 */

#include "gtest/gtest.h"
#include "internal/audio/common/IasAudioLogging.hpp"

using namespace IasAudio;

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  IasAudioLogging::registerDltApp(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
