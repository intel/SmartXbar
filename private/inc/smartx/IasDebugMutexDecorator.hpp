/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasDebugMutexDecorator.hpp
 * @date 2017
 * @brief This is the declaration of a decorator for the IasIDebug instance
 */

#ifndef IASDEBUGMUTEXDECORATOR_HPP_
#define IASDEBUGMUTEXDECORATOR_HPP_

#include "audio/smartx/IasIDebug.hpp"

namespace IasAudio {

/**
 * @brief Decorator for the IasIDebug instance
 *
 * It adds a mutex around each method to make it safe while being accessed by multiple threads.
 */
class IAS_AUDIO_PUBLIC IasDebugMutexDecorator : public IasIDebug
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] debug   Pointer to the IasIDebug instance to be decorated
     */
    IasDebugMutexDecorator(IasIDebug *debug);

    /**
     * @brief Destructor
     */
    virtual ~IasDebugMutexDecorator();

    /**
     * @brief Inherited from IasIDebug.
     */
    IasResult startInject(const std::string &fileNamePrefix, const std::string &portName, std::uint32_t numSeconds) override;
    /**
     * @brief Inherited from IasIDebug.
     */
    IasResult startRecord(const std::string &fileNamePrefix, const std::string &portName, std::uint32_t numSeconds) override;
    /**
     * @brief Inherited from IasIDebug.
     */
    IasResult stopProbing(const std::string &portName) override;

  private:
    /**
     * @brief Copy constructor, private deleted to prevent misuse.
     */
    IasDebugMutexDecorator(IasDebugMutexDecorator const &other) = delete;
    /**
     * @brief Assignment operator, private deleted to prevent misuse.
     */
    IasDebugMutexDecorator& operator=(IasDebugMutexDecorator const &other) = delete;

    IasIDebug      *mDebug;       //!< Pointer to original IasIDebug instance
};

} /* namespace IasAudio */

#endif /* IASDEBUGMUTEXDECORATOR_HPP_ */
