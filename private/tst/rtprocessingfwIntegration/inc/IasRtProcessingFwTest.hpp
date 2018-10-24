/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRtProcessingFwIntegration.hpp
 * @date   Sep 17, 2012
 * @brief
 */
#ifndef RTPROCESSINGFW_INTEGRATION_HPP_
#define RTPROCESSINGFW_INTEGRATION_HPP_

#include "gtest/gtest.h"

#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "rtprocessingfwx/IasBundleSequencer.hpp"

namespace IasAudio {

static const uint32_t cIasTestAudioFrameLength = 128;

class IasRtProcessingFwTest : public ::testing::Test
{
  protected:
  virtual void SetUp();
  virtual void TearDown();

};

}


#endif /* RTPROCESSINGFW_INTEGRATION_HPP_ */
