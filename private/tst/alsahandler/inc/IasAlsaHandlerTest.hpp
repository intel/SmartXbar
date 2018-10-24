/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasAlsaHandlerTest.hpp
 *
 *  Created 2015
 */

#ifndef IASALSAHANDLERTEST_HPP_
#define IASALSAHANDLERTEST_HPP_


#include "alsahandler/IasAlsaHandler.hpp"
#include "gtest/gtest.h"


namespace IasAudio
{

class IAS_AUDIO_PUBLIC IasAlsaHandlerTest : public ::testing::Test
{
protected:
  virtual void SetUp();
  virtual void TearDown();

public:

};


}

#endif /* IASALSAHANDLERTEST_HPP_ */
