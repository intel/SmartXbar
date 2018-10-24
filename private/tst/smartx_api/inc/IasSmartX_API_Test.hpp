/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasSmartX_API_Test.hpp
 *
 *  Created 2015
 */

#ifndef IASSMARTX_API_TEST_HPP_
#define IASSMARTX_API_TEST_HPP_

#include <thread>
#include <mutex>
#include <condition_variable>
#include "gtest/gtest.h"

#include "audio/smartx/IasEventProvider.hpp"
#include "IasAudioTypedefs.hpp"

namespace IasAudio
{

class IasSmartX_API_Test : public ::testing::Test
{
  public:
    void worker_thread();

  protected:
    static void SetUpTestCase();

    virtual void SetUp();
    virtual void TearDown();

    std::mutex                  mMutex;
    std::condition_variable     mCondition;
    uint32_t                 mNrEvents;
    IasEventQueue               mEventQueue;
    bool                   mThreadIsRunning;
};


}

#endif /* IASSMARTX_API_TEST_HPP_ */
