/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasRoutingZoneTest.hpp
 *
 *  Created 2015
 */

#ifndef IASROUTINGZONETEST_HPP_
#define IASROUTINGZONETEST_HPP_

#include "gtest/gtest.h"

#include "smartx/IasConfigFile.hpp"

namespace IasAudio
{

class IasRoutingZoneTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();

    static void SetUpTestCase()
    {
      // Preload config with default values
      IasConfigFile::getInstance()->load();
    }

  public:
    DltContext *mLog;
};


}

#endif /* IASROUTINGZONETEST_HPP_ */
