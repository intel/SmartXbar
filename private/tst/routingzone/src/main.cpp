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
#include "internal/audio/common/IasAudioLogging.hpp"

// Global string variables specifying the ALSA device names. Default is "plughw:31,...".

#ifndef PLUGHW_DEVICE_PREFIX_NAME
#define PLUGHW_DEVICE_PREFIX_NAME   "plughw:31,0,"
#endif

std::string deviceName1{std::string(PLUGHW_DEVICE_PREFIX_NAME) + "6"};
std::string deviceName2{std::string(PLUGHW_DEVICE_PREFIX_NAME) + "7"};

// Global variable specifying that the period size of the derived zone shall be <multiple>
// times longer than the period size of the base zone. Default is 1, because the dummy
// device plughw does not support different period sizes for the subdevices.
uint32_t periodSizeMultiple = 1;

// Global variable specifying how many periods shall be processed.
uint32_t numPeriodsToProcess = 8;

// Global variable specifying whether WAVE files shall be read from nfs.
bool useNfsPath = true;



static void printWelcomeMessage()
{
  std::cout << "======================================" << std::endl;
  std::cout << "Unit test for the class IasRoutingZone" << std::endl;
  std::cout << "======================================" << std::endl;
}

static void printHelpMessage(const char* commandName)
{
  std::cout << "Usage: " << commandName << " [OPTION]"   << std::endl;
  std::cout << "-h,  --help                 help"                    << std::endl;
  std::cout << "-D1, --device1 <name>       ALSA playback device for base zone,    e.g., front:CARD=PCH,DEV=0"     << std::endl;
  std::cout << "-D2, --device2 <name>       ALSA playback device for derived zone, e.g., iec958:CARD=Device,DEV=0" << std::endl;
  std::cout << "-M,  --multiple <number>    Specifies that the period size of the derived zone shall be"           << std::endl
            << "                            <number> times longer than the period size of the base zone"         << std::endl;
  std::cout << "-N,  --numperiods <number>  Number of periods to process"                                          << std::endl;
  std::cout << "--no_nfs_paths              Don't read WAVE files from NFS directory, use local directory instead" << std::endl;
  std::cout << "Example: " << commandName
            << " --device1 front:CARD=PCH,DEV=0 --device2 iec958:CARD=Device,DEV=0 --multiple 2 --numperiods 1000" << std::endl;
}


int main(int argc, char* argv[])
{
  IasAudio::IasAudioLogging::registerDltApp(true);
  IasAudio::IasAudioLogging::addDltContextItem("global", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);
  IasAudio::IasAudioLogging::addDltContextItem("TST", DLT_LOG_INFO, DLT_TRACE_STATUS_ON);

  ::testing::InitGoogleTest(&argc, argv);

  printWelcomeMessage();

  // Parse the command line.
  for (int cnt = 1; cnt < argc ; cnt++)
  {
    if ( (strcmp(argv[cnt], "-D1") == 0) || (strcmp(argv[cnt], "--device1") == 0) )
    {
      cnt++;
      if (cnt >= argc)
      {
        printHelpMessage(argv[0]);
        std::cout << "Error: device name not specified!" << std::endl;
        exit(1);
      }
      deviceName1 = argv[cnt];
    }
    else if ( (strcmp(argv[cnt], "-D2") == 0) || (strcmp(argv[cnt], "--device2") == 0) )
    {
      cnt++;
      if (cnt >= argc)
      {
        printHelpMessage(argv[0]);
        std::cout << "Error: device name not specified!" << std::endl;
        exit(1);
      }
      deviceName2 = argv[cnt];
    }
    else if ( (strcmp(argv[cnt], "-M") == 0) || (strcmp(argv[cnt], "--multiple") == 0) )
    {
      cnt++;
      if (cnt >= argc)
      {
        printHelpMessage(argv[0]);
        std::cout << "Error: value not specified!" << std::endl;
        exit(1);
      }
      periodSizeMultiple = atoi(argv[cnt]);
      if ((periodSizeMultiple < 1) || (periodSizeMultiple > 256))
      {
        printHelpMessage(argv[0]);
        std::cout << "Invalid parameter --multiple " << periodSizeMultiple
                  << ", must be between 1 and 256" << std::endl;
        exit(1);
      }
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
    else if (strncmp(argv[cnt], "--no_nfs_paths", 14) == 0)
    {
      useNfsPath = false;
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

  std::cout << "Writing to devices " << deviceName1 << " and " << deviceName2 << std::endl;

  int result = RUN_ALL_TESTS();

  std::cout << "All tests have been finished!" << std::endl << std::endl;
  return result;
}
