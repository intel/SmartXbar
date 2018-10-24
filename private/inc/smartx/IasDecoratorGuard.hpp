/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDecoratorGuard.hpp
 * @date   2017
 * @brief
 */

#ifndef AUDIO_DAEMON2_PRIVATE_INC_SMARTX_IASDECORATORGUARD_HPP_
#define AUDIO_DAEMON2_PRIVATE_INC_SMARTX_IASDECORATORGUARD_HPP_

#include <mutex>

namespace IasAudio {


/**
 * @brief
 *
 * Lock guard for decorator pattern to make smartx thread-safety, the IasDecoratorGuard
 * class is wrapper for lock_guard with static mutex variable, then the same mutex
 * can be shared between IasRouting, IasSetup and IasProcessing
 *
 */
class IasDecoratorGuard : public std::lock_guard<std::mutex>
{
  friend class IasDebugMutexDecorator;
  friend class IasProcessingMutexDecorator;
  friend class IasRoutingMutexDecorator;
  friend class IasSetupMutexDecorator;

  /**
   * @brief Constructor.
   */
  explicit IasDecoratorGuard();

  /**
   * @brief Destructor.
   */
  virtual ~IasDecoratorGuard();

  IasDecoratorGuard(const IasDecoratorGuard&) = delete;
  IasDecoratorGuard& operator=(const IasDecoratorGuard&) = delete;

  static std::mutex mMutexDecorator;
};


} //namespace IasAudio



#endif /* AUDIO_DAEMON2_PRIVATE_INC_SMARTX_IASDECORATORGUARD_HPP_ */
