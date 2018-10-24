/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSetupEvent.cpp
 * @date   2015
 * @brief
 */

#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/IasEventHandler.hpp"

namespace IasAudio {

IasSetupEvent::IasSetupEvent()
  :mEvenType(eIasUninitialized)
  ,mSourceDevice(nullptr)
  ,mSinkDevice(nullptr)
{
}

IasSetupEvent::~IasSetupEvent()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, "[IasSetupEvent::~IasSetupEvent]");
}

void IasSetupEvent::accept(IasEventHandler& eventHandler)
{
  eventHandler.receivedSetupEvent(this);
}


} //namespace IasAudio
