/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created 2015
 */

#include "gtest/gtest.h"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

using namespace IasAudio;

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  IasAudioLogging::registerDltApp(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
