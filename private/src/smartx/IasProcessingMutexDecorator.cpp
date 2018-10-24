/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasProcessingMutexDecorator.cpp
 * @date 2017
 * @brief Definition of IasProcessingMutexDecorator
 */

#include <smartx/IasDecoratorGuard.hpp>
#include "smartx/IasProcessingMutexDecorator.hpp"

namespace IasAudio {

IasProcessingMutexDecorator::IasProcessingMutexDecorator(IasIProcessing *processing)
  :mProcessing(processing)
{
}

IasProcessingMutexDecorator::~IasProcessingMutexDecorator()
{
  // original instance of IasIProcessing has to be deleted here, because the original
  // member variable was overwritten by the decorator implementation
  delete mProcessing;
}

IasIProcessing::IasResult IasProcessingMutexDecorator::sendCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  const IasDecoratorGuard lk{};
  return mProcessing->sendCmd(instanceName, cmdProperties, returnProperties);
}

} /* namespace IasAudio */
