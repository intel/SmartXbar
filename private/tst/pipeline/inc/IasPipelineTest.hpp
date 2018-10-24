/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * @file  IasPipelineTest.hpp
 * @date  2016
 * @brief
 */

#ifndef IASPIPELINETEST_HPP_
#define IASPIPELINETEST_HPP_


#include <gtest/gtest.h>


namespace IasAudio
{

class IAS_AUDIO_PUBLIC IasPipelineTest : public ::testing::Test
{
protected:
  virtual void SetUp();
  virtual void TearDown();

public:

};


}

#endif /* IASPIPELINETEST_HPP_ */
