/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConnectionEvent.cpp
 * @date   2015
 * @brief
 */

#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasEventHandler.hpp"

namespace IasAudio {

IasConnectionEvent::IasConnectionEvent()
  :mEvenType(eIasUninitialized)
  ,mSourceId(-1)
  ,mSinkId(-1)
{
}

IasConnectionEvent::~IasConnectionEvent()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "[IasConnectionEvent::~IasConnectionEvent]");
}

void IasConnectionEvent::accept(IasEventHandler& eventHandler)
{
  eventHandler.receivedConnectionEvent(this);
}


} //namespace IasAudio
