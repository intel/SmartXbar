/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasDebugMutexDecorator.cpp
 * @date 2017
 * @brief Definition of IasDebugMutexDecorator
 */

#include "smartx/IasDebugMutexDecorator.hpp"
#include "smartx/IasDecoratorGuard.hpp"

namespace IasAudio {

IasDebugMutexDecorator::IasDebugMutexDecorator(IasIDebug *debug)
  :mDebug(debug)
{
}

IasDebugMutexDecorator::~IasDebugMutexDecorator()
{
  // original instance of IasIDebug has to be deleted here, because the original
  // member variable was overwritten by the decorator implementation
  delete mDebug;
}

IasIDebug::IasResult IasDebugMutexDecorator::startInject(const std::string& fileNamePrefix, const std::string& portName,
                                                        uint32_t numSeconds)
{
  const IasDecoratorGuard lk{};
  return mDebug->startInject(fileNamePrefix, portName, numSeconds);
}

IasIDebug::IasResult IasDebugMutexDecorator::startRecord(const std::string& fileNamePrefix, const std::string& portName,
                                                        uint32_t numSeconds)
{
  const IasDecoratorGuard lk{};
  return mDebug->startRecord(fileNamePrefix, portName, numSeconds);
}

IasIDebug::IasResult IasDebugMutexDecorator::stopProbing(const std::string& portName)
{
  const IasDecoratorGuard lk{};
  return mDebug->stopProbing(portName);
}

} /* namespace IasAudio */

