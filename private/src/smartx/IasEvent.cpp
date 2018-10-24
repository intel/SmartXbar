/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEvent.cpp
 * @date   2015
 * @brief
 */

#include "audio/smartx/IasEvent.hpp"

namespace IasAudio {

IasEvent::IasEvent()
  :mLog(IasAudioLogging::registerDltContext("EVT", "SmartX Events"))
{
}

IasEvent::~IasEvent()
{
}

} //namespace IasAudio
