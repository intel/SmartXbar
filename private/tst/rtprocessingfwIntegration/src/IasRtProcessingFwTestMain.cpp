/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created on: Sep 14, 2012
 */

#include "gtest/gtest.h"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(true);
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}


