/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDecoratorGuard.cpp
 * @date   2017
 * @brief
 */


#include <smartx/IasDecoratorGuard.hpp>

namespace IasAudio {


IasDecoratorGuard::IasDecoratorGuard() :
  std::lock_guard<std::mutex>(mMutexDecorator)
{
}

IasDecoratorGuard::~IasDecoratorGuard()
{
}


std::mutex IasDecoratorGuard::mMutexDecorator;

} //namespace IasAudio
