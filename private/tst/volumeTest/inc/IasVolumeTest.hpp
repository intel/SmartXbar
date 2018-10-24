/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasVolumeTest.hpp
 *
 *  Created on: Nov 2, 2012
 */

#ifndef IASVOLUMETEST_HPP_
#define IASVOLUMETEST_HPP_

#include <gtest/gtest.h>

namespace IasAudio
{

class IasAudioChain;
class IasIGenericAudioCompConfig;
class IasGenericAudioComp;
class IasAudioStream;

class IasVolumeTest : public ::testing::Test
{
protected:
  virtual void SetUp();
  virtual void TearDown();

public:
  IasAudioChain                 *mAudioChain;
  IasIGenericAudioCompConfig    *mVolumeConfig;
  IasGenericAudioComp           *mVolume;
  IasAudioStream                *mSink0;
  IasAudioStream                *mSink1;
  IasAudioStream                *mSink2;
};


}

#endif /* IASVOLUMETEST_HPP_ */
