/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDummyConnectionTest.hpp
 * @date   2016
 * @brief
 */

#ifndef IASDUMMYCONNECTIONTEST_HPP
#define IASDUMMYCONNECTIONTEST_HPP

#include "gtest/gtest.h"

#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasSmartX.hpp"

namespace IasAudio {


class IAS_AUDIO_PUBLIC IasDummyConnectionTest : public ::testing::Test
{
  public:
  IasSmartX* mSmartx;

  protected:
  virtual void SetUp();
  virtual void TearDown();

};

}

#endif