/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * equalizerCarTestMain.cpp
 *
 *  Created on: November 2016
 */

#include <gtest/gtest.h>
#include <dlt/dlt.h>

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  DLT_REGISTER_APP("TST", "Test Application");
//  DLT_ENABLE_LOCAL_PRINT();
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();

  printf("test complete!\n");

  return result;
}
