/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioTypedefs.hpp
 * @date   2015
 * @brief
 */

#ifndef IASAUDIOTYPEDEFS_HPP_
#define IASAUDIOTYPEDEFS_HPP_

#include <memory>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "tbb/concurrent_queue.h"

namespace IasAudio {

class IasAlsaHandler;
/**
 * @brief Shared ptr type for IasAlsaHandler
 */
using IasAlsaHandlerPtr = std::shared_ptr<IasAlsaHandler>;

class IasSmartXClient;
/**
 * @brief Shared ptr type for IasSmartXClient
 */
using IasSmartXClientPtr = std::shared_ptr<IasSmartXClient>;

/**
 * @brief List type of IasAudioPortPtrs
 */
using IasAudioPortPtrList = std::list<IasAudioPortPtr>;

/**
 * @brief List type of IasAudioRoutingZonePtrs
 */
using IasRoutingZonePtrList = std::list<IasRoutingZonePtr>;

class IasAudioPortOwner;
/**
 * @brief Shared ptr type for IasAudioPortOwner
 */
using IasAudioPortOwnerPtr = std::shared_ptr<IasAudioPortOwner>;

class IasSwitchMatrix;
/**
 * @brief Shared ptr for IasSwitchMatrixWorkerThread
 */
using IasSwitchMatrixPtr = std::shared_ptr<IasSwitchMatrix>;

class IasRoutingZoneWorkerThread;
/**
 * @brief Shared ptr for IasRoutingZoneWorkerThread
 */
using IasRoutingZoneWorkerThreadPtr = std::shared_ptr<IasRoutingZoneWorkerThread>;

class IasRunnerThread;
/**
 * @brief Shared ptr for IasRoutingZoneRunnerThread
 */
using IasRoutingZoneRunnerThreadPtr = std::shared_ptr<IasRunnerThread>;

class IasAudioChain;
/**
 * @brief Shared ptr type for IasAudioChain
 */
using IasAudioChainPtr = std::shared_ptr<IasAudioChain>;

class IasCmdDispatcher;
/**
 * @brief Shared ptr type for IasCmdDispatcher
 */
using IasCmdDispatcherPtr = std::shared_ptr<IasCmdDispatcher>;

class IasICmdRegistration;
/**
 * @brief Shared ptr type for IasCmdDispatcher
 */
using IasICmdRegistrationPtr = std::shared_ptr<IasICmdRegistration>;

class IasPluginEngine;
/**
 * @brief Shared ptr type for IasPluginEngine
 */
using IasPluginEnginePtr = std::shared_ptr<IasPluginEngine>;



} //namespace IasAudio

#endif /* IASAUDIOTYPEDEFS_HPP_ */
