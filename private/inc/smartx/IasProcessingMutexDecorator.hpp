/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasProcessingMutexDecorator.hpp
 * @date 2017
 * @brief This is the declaration of a decorator for the IasIProcessing instance
 */

#ifndef IASPROCESSINGMUTEXDECORATOR_HPP_
#define IASPROCESSINGMUTEXDECORATOR_HPP_

#include "audio/smartx/IasIProcessing.hpp"

namespace IasAudio {

/**
 * @brief Decorator for the IasIProcessing instance
 *
 * It adds a mutex around each method to make it safe while being accessed by multiple threads.
 */
class IAS_AUDIO_PUBLIC IasProcessingMutexDecorator : public IasIProcessing
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] debug   Pointer to the IasIProcessing instance to be decorated
     */
    IasProcessingMutexDecorator(IasIProcessing *processing);

    /**
     * @brief Destructor
     */
    virtual ~IasProcessingMutexDecorator();

    /**
     * @brief Inherited from IasIProcessing.
     */
    IasResult sendCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties) override;

  private:
      /**
       * @brief Copy constructor, private deleted to prevent misuse.
       */
      IasProcessingMutexDecorator(IasProcessingMutexDecorator const &other) = delete;
      /**
       * @brief Assignment operator, private deleted to prevent misuse.
       */
      IasProcessingMutexDecorator& operator=(IasProcessingMutexDecorator const &other) = delete;

      IasIProcessing *mProcessing;   //!< Pointer to original IasIProcessing instance
};

} /* namespace IasAudio */

#endif /* IASPROCESSINGMUTEXDECORATOR_HPP_ */
