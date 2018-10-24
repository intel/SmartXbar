/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasRoutingMutexDecorator.cpp
 * @date 2017
 * @brief Definition of IasRoutingMutexDecorator
 */

#include <smartx/IasDecoratorGuard.hpp>
#include "smartx/IasRoutingMutexDecorator.hpp"

namespace IasAudio {

IasRoutingMutexDecorator::IasRoutingMutexDecorator(IasIRouting *routing)
  :mRouting(routing)
{
}

IasRoutingMutexDecorator::~IasRoutingMutexDecorator()
{
  // original instance of IasIRouting has to be deleted here, because the original
  // member variable was overwritten by the decorator implementation
  delete mRouting;
}

IasIRouting::IasResult IasRoutingMutexDecorator::connect(std::int32_t sourceId, std::int32_t sinkId)
{
  const IasDecoratorGuard lk{};
  return mRouting->connect(sourceId, sinkId);
}

IasIRouting::IasResult IasRoutingMutexDecorator::disconnect(std::int32_t sourceId, std::int32_t sinkId)
{
  const IasDecoratorGuard lk{};
  return mRouting->disconnect(sourceId, sinkId);
}

IasConnectionVector IasRoutingMutexDecorator::getActiveConnections() const
{
  const IasDecoratorGuard lk{};
  return mRouting->getActiveConnections();
}

IasDummySourcesSet IasRoutingMutexDecorator::getDummySources() const
{
  const IasDecoratorGuard lk{};
  return mRouting->getDummySources();
}

} /* namespace IasAudio */
