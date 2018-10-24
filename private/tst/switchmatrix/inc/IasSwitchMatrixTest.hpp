/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasSwitchMatrixTest.hpp
 *
 *  Created 2015
 */

#ifndef IASSWITCHMATRIXTEST_HPP_
#define IASSWITCHMATRIXTEST_HPP_

#include "gtest/gtest.h"

#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasEventHandler.hpp"

namespace IasAudio
{

class IasSwitchMatrixTest : public ::testing::Test
{
  protected:
    virtual void SetUp();
    virtual void TearDown();
};


class IAS_AUDIO_PUBLIC IasConnectEventHandler : public IasEventHandler
{
  public:

    IasConnectEventHandler();

    /**
     * @brief Destructor.
    */
    virtual ~IasConnectEventHandler();

    virtual void receivedConnectionEvent(IasConnectionEvent *event);


  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
    */
    IasConnectEventHandler(IasConnectEventHandler const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
    */
    IasConnectEventHandler& operator=(IasConnectEventHandler const &other);

    uint32_t   mCnt;
};


}

#endif /* IASSWITCHMATRIXTEST_HPP_ */
