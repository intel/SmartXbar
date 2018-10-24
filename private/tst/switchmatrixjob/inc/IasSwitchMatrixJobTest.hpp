/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasSwitchMatrixJobTest.hpp
 *
 *  Created 2016
 */

#ifndef IASSWITCHMATRIXJOBTEST_HPP_
#define IASSWITCHMATRIXJOBTEST_HPP_

#include "gtest/gtest.h"


namespace IasAudio
{

class IasSwitchMatrixJobTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();
};

}

#endif