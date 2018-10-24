/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasBufferTaskTest.hpp
 *
 *  Created 2016
 */

#ifndef IASBUFFERTASKTEST_HPP_
#define IASBUFFERTASKTEST_HPP_

#include "gtest/gtest.h"


namespace IasAudio
{

class IasBufferTaskTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();
};

}

#endif
