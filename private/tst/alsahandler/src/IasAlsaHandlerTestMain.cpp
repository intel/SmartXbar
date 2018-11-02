/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasAlsaHandlerTestMain.cpp
 *
 *  Created 2015
 */

#include "gtest/gtest.h"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

#ifndef PLUGHW_DEVICE_NAME
#define PLUGHW_DEVICE_NAME  "plughw:31,0"
#endif

// Global string variable specifying the ALSA device name. Default is "plughw:31,0".
std::string deviceName(PLUGHW_DEVICE_NAME); // default device

// Global variable specifying how many periods shall be processed.
uint32_t numPeriodsToProcess = 20;


static void printWelcomeMessage()
{
  std::cout << "======================================" << std::endl;
  std::cout << "Unit test for the class IasAlsaHandler" << std::endl;
  std::cout << "======================================" << std::endl;
}

static void printHelpMessage(const char* commandName)
{
  std::cout << "Usage: " << commandName << " [OPTION]"         << std::endl;
  std::cout << "-h, --help       help"                         << std::endl;
  std::cout << "-D, --device     ALSA playback device"         << std::endl;
  std::cout << "-N, --numperiods Number of periods to process" << std::endl;
  std::cout << "Example: " << commandName
            << " --device front:CARD=PCH,DEV=0 --numperiods 100" << std::endl;
}

int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(true);
  IasAudio::IasAudioLogging::addDltContextItem("global", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);
  ::testing::InitGoogleTest(&argc, argv);

  printWelcomeMessage();

  // Parse the command line for the keywords "-D" or "--device".
  for (int cnt = 1; cnt < argc ; cnt++)
  {
    if ( (strcmp(argv[cnt], "-D") == 0) || (strcmp(argv[cnt], "--device") == 0) )
    {
      cnt++;
      if (cnt >= argc)
      {
        printHelpMessage(argv[0]);
        std::cout << "Error: device name not specified!" << std::endl;
        exit(1);
      }
      deviceName = argv[cnt];
    }
    else if ( (strcmp(argv[cnt], "-N") == 0) || (strcmp(argv[cnt], "--numperiods") == 0) )
    {
      cnt++;
      if (cnt >= argc)
      {
        printHelpMessage(argv[0]);
        std::cout << "Error: value not specified!" << std::endl;
        exit(1);
      }
      numPeriodsToProcess = atoi(argv[cnt]);
      if (numPeriodsToProcess < 1)
      {
        printHelpMessage(argv[0]);
        std::cout << "Invalid parameter --numperiods " << numPeriodsToProcess
                  << ", must be greater than 0" << std::endl;
        exit(1);
      }
    }
    else if ( (strcmp(argv[cnt], "-h") == 0) || (strcmp(argv[cnt], "--help") == 0) )
    {
      printHelpMessage(argv[0]);
      exit(1);
    }
    else if (strncmp(argv[cnt], "--shared_memory_name=", 21) == 0)
    {
      std::cout << "Ignoring command-line option " << argv[cnt] << std::endl;
    }
    else
    {
      printHelpMessage(argv[0]);
      std::cout << "Error: unsupported option: " << argv[cnt] << std::endl;
      exit(1);
    }
  }

  std::cout << "Writing to device " << deviceName << std::endl;

  int result = RUN_ALL_TESTS();
  return result;
}
