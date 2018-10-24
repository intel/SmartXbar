/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCustomerEventHandler.cpp
 * @date   2015
 * @brief
 */

#include <iostream>
#include <gtest/gtest.h>
#include "IasCustomerEventHandler.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"

using namespace std;

namespace IasAudio {

IasCustomerEventHandler::IasCustomerEventHandler()
  :IasEventHandler()
  ,mCnt(0)
{
  mExpected[0] = IasConnectionEvent::eIasSinkDeleted;
  mExpected[1] = IasConnectionEvent::eIasSourceDeleted;
}

IasCustomerEventHandler::~IasCustomerEventHandler()
{
}

void IasCustomerEventHandler::receivedConnectionEvent(IasConnectionEvent* event)
{
  ASSERT_TRUE(event != nullptr);
  ASSERT_TRUE(mLog != nullptr);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "[IasCustomerEventHandler::receivedConnectionEvent] Event received:", toString(event->getEventType()));
  ASSERT_TRUE(event != nullptr);
  EXPECT_EQ(mExpected[mCnt++], event->getEventType());
}


} //namespace IasAudio
