/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created on: June 10th, 2016
 *
 *Coverage test for filter module
 */

#include <gtest/gtest.h>
#include <string.h>
#include <sndfile.h>
#include <iostream>
#include <malloc.h>
#include <math.h>

#include "filter/IasAudioFilter.hpp"

using namespace IasAudio;
using namespace std;

IasAudioFilterParams filterParams;

int32_t main(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;
  DLT_REGISTER_APP("TST", "Test Application");
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}

namespace IasAudio {

class IasFilterCoverageTest : public ::testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

const uint32_t sampleFreq = 48000;


/*
 * Main function.
 */
TEST_F(IasFilterCoverageTest, coverage_test)
{

  IasAudioFilter *filter = new IasAudioFilter(sampleFreq,64); // 1st filter in 1st bundle
  IasAudioFilterParams params;

  params.freq = 200;
  params.gain = 0.2f;
  params.order = 3;
  params.quality = 0.5f;
  params.section = 2;
  params.type = eIasFilterTypeLowpass;

  IasAudioChannelBundle myBundle(64);
  myBundle.init();

  filter->init();
  filter->setBundlePointer(&myBundle);
  params.freq = 1;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);
  params.freq = (sampleFreq>>1) +1000;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);

  params.freq = 200;
  params.order = 21;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);
  params.order = 0;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);

  params.order   = 3;
  params.section = 0;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);
  params.section = 1;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) == 0);
  params.section = 2;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) == 0);

  params.type = eIasFilterTypeHighpass;
  params.order = 21;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);
  params.order = 0;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);

  params.order = 3;
  params.section = 0;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);
  params.section = 1;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) == 0);
  params.section = 2;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) == 0);
  params.section = 50;
  ASSERT_TRUE(filter->setChannelFilter(0,&params) != 0);

  params.type = eIasFilterTypePeak;
  params.order = 3;
  params.freq = 1000;
  ASSERT_TRUE(filter->setChannelFilter(1,&params) != 0);
  params.order = 2;
  params.gain = 0.0001f;
  ASSERT_TRUE(filter->setChannelFilter(1,&params) != 0);
  params.gain = 10000.0f;
  ASSERT_TRUE(filter->setChannelFilter(1,&params) != 0);
  params.gain = 1.0f;
  params.quality = 0.0001f;
  ASSERT_TRUE(filter->setChannelFilter(1,&params) != 0);
  params.quality = 1000.0f;
  ASSERT_TRUE(filter->setChannelFilter(1,&params) != 0);
  params.quality = 1.0f;
  ASSERT_TRUE(filter->setChannelFilter(1,&params) == 0);


  params.type = eIasFilterTypeBandpass;
  params.freq = 100;
  params.order = 5;
  params.quality = 1.0f;
  ASSERT_TRUE(filter->setChannelFilter(2,&params) != 0);
  params.order = 2;
  params.quality = 0.0001f;
  ASSERT_TRUE(filter->setChannelFilter(2,&params) != 0);
  params.quality = 10000.0f;
  ASSERT_TRUE(filter->setChannelFilter(2,&params) != 0);

  params.quality = 10.0f;
  params.order = 2;
  ASSERT_TRUE(filter->setChannelFilter(2,&params) == 0);


  params.type = eIasFilterTypeLowShelving;
  params.order = 0;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.order = 3;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.order = 2;
  params.gain = 0.00001f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.gain = 10000.0f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.gain = 2.0f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);
  params.gain = 0.5f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);
  params.order = 1;
  params.gain = 2.0f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);
  params.gain = 0.5f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);

  params.type = eIasFilterTypeHighShelving;
  params.order = 0;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.order = 3;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.order = 2;
  params.gain = 0.00001f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.gain = 10000.0f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) != 0);
  params.gain = 2.0f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);
  params.gain = 0.5f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);
  params.order = 1;
  params.gain = 2.0f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);
  params.gain = 0.5f;
  ASSERT_TRUE(filter->setChannelFilter(3,&params) == 0);


  ASSERT_TRUE(filter->setRampGradient(8,0.5f) != 0);
  ASSERT_TRUE(filter->setRampGradient(0,0.001f) != 0);
  ASSERT_TRUE(filter->setRampGradient(0,7.0f) != 0);
  ASSERT_TRUE(filter->setRampGradient(0,1.0f) == 0);

  ASSERT_TRUE(filter->updateGain(3,10000.0f) != 0);
  ASSERT_TRUE(filter->updateGain(3,2.0f) == 0);

  filter->calculate();

  delete filter;
}
}
