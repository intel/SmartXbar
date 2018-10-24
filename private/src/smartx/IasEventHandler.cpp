/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEventHandler.cpp
 * @date   2015
 * @brief
 */

#include "audio/smartx/IasEventHandler.hpp"

namespace IasAudio {

static const std::string cClassName = "IasEventHandler::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasEventHandler::IasEventHandler()
  :mLog(IasAudioLogging::registerDltContext("EVT", "SmartX Events"))
{
}

IasEventHandler::~IasEventHandler()
{
}

void IasEventHandler::receivedConnectionEvent(IasConnectionEvent*)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Unhandled IasConnectionEvent");
}

void IasEventHandler::receivedSetupEvent(IasSetupEvent*)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Unhandled IasSetupEvent");
}

void IasEventHandler::receivedModuleEvent(IasModuleEvent*)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Unhandled IasModuleEvent");
}

} //namespace IasAudio
