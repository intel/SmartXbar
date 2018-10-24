/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasRoutingMutexDecorator.hpp
 * @date 2017
 * @brief This is the declaration of a decorator for the IasIRouting instance
 */

#ifndef IASROUTINGMUTEXDECORATOR_HPP_
#define IASROUTINGMUTEXDECORATOR_HPP_

#include "audio/smartx/IasIRouting.hpp"

namespace IasAudio {

/**
 * @brief Decorator for the IasIRouting instance
 *
 * It adds a mutex around each method to make it safe while being accessed by multiple threads.
 */
class IAS_AUDIO_PUBLIC IasRoutingMutexDecorator : public IasIRouting
{
  public:
      /**
       * @brief Constructor
       *
       * @param[in] debug   Pointer to the IasIRouting instance to be decorated
       */
      IasRoutingMutexDecorator(IasIRouting *routing);

      /**
       * @brief Destructor
       */
      virtual ~IasRoutingMutexDecorator();

      /**
       * @brief Inherited from IasIRouting.
       */
      IasResult connect(std::int32_t sourceId, std::int32_t sinkId) override;

      /**
       * @brief Inherited from IasIRouting.
       */
      IasResult disconnect(std::int32_t sourceId, std::int32_t sinkId) override;

      /**
       * @brief Inherited from IasIRouting.
       */
      IasConnectionVector getActiveConnections() const override;

      /**
       * @brief Inherited from IasIRouting.
       */
      IasDummySourcesSet getDummySources() const override;

  private:
      /**
       * @brief Copy constructor, private deleted to prevent misuse.
       */
      IasRoutingMutexDecorator(IasRoutingMutexDecorator const &other) = delete;
       /**
         * @brief Assignment operator, private deleted to prevent misuse.
         */
      IasRoutingMutexDecorator& operator=(IasRoutingMutexDecorator const &other) = delete;

      IasIRouting         *mRouting;     //!< Pointer to original IasIRouting instance

};

} /* namespace IasAudio */

#endif /* IASROUTINGMUTEXDECORATOR_HPP_ */
