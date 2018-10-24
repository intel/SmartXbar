/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEventProvider.hpp
 * @date   2015
 * @brief
 */

#ifndef IASEVENTPROVIDER_HPP
#define IASEVENTPROVIDER_HPP

#include <thread>
#include <mutex>
#include <condition_variable>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "tbb/concurrent_queue.h"

namespace IasAudio {

/**
 * @brief Queue type of IasEventPtrs
 */
using IasEventQueue = tbb::concurrent_queue<IasEventPtr>;

class IasConnectionEvent;

/**
 * @brief The event provider is used to create and send events back to the user
 */
class IAS_AUDIO_PUBLIC IasEventProvider
{
  public:
    /**
     * @brief Results of event provider methods
     */
    enum IasResult
    {
      eIasOk,                 //!< Operation successful
      eIasFailed,             //!< Operation failed
      eIasTimeout,            //!< Timeout while waiting for event
      eIasNoEventAvailable,   //!< No event available
    };

    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasEventProvider();

    /**
     * @brief Get the single instance of the event provider
     *
     * @return A pointer to the single instance of the event provider
     */
    static IasEventProvider* getInstance();

    /**
     * @brief Create a connection event
     *
     * After the event was created the event IasConnectionEvent methods for setting
     * the actual event informations have to be called. Please see IasAudio::IasConnectionEvent
     * for more details.
     *
     * @return A pointer to an uninitialized connection event
     */
    IasConnectionEventPtr createConnectionEvent();

    /**
     * @brief Destroy a connection event
     *
     * @param[in] event Connection event to destroy
     */
    void destroyConnectionEvent(IasConnectionEventPtr event);

    /**
     * @brief Create a setup event
     *
     * After the event was created the event IasAudio::IasSetupEvent methods for setting
     * the actual event informations have to be called. Please see IasAudio::IasSetupEvent
     * for more details.
     *
     * @return A pointer to an uninitialized setup event
     */
    IasSetupEventPtr createSetupEvent();

    /**
     * @brief Destroy a setup event
     *
     * @param[in] event Setup event to destroy
     */
    void destroySetupEvent(IasSetupEventPtr event);

    /**
     * @brief Create a module event
     *
     * After the event was created the event IasAudio::IasModuleEvent methods for setting
     * the actual event informations have to be called. Please see IasAudio::IasModuleEvent
     * for more details.
     *
     * @return A pointer to an uninitialized module event
     */
    IasModuleEventPtr createModuleEvent();

    /**
     * @brief Destroy a module event
     *
     * @param[in] event Module event to destroy
     */
    void destroyModuleEvent(IasModuleEventPtr event);

    /**
     * @brief Send the event to the customer application
     *
     * @param[in] event The event to be sent
     *
     * @return The result of the method
     * @retval IasEventProvider::eIasOk Event successfully sent
     * @retval IasEventProvider::eIasFailed Failed to send event
     */
    void send(IasEventPtr event);

    /**
     * @brief Wait for an asynchronous event from the SmartXbar
     *
     * This is a blocking call that waits for an event to occur. After an event occured the method IasEventProvider::getNextEvent has to
     * be called to receive the next event from the event queue.
     *
     * @param[in] timeout The timeout value in msec after which the call will return in any case. If 0 is given call will block forever
     *
     * @return The result of the method call
     * @retval IasEventProvider::eIasOk Event occured. Receive it via IasEventProvider::getNextEvent
     * @retval IasEventProvider::eIasTimeout Timeout occured while waiting for event
     */
    IasResult waitForEvent(uint32_t timeout);

    /**
     * @brief Get next event from the event queue
     *
     * This can either be used to poll the event queue or it can be used after a call to the method
     * IasEventProvider::waitForEvent in a separate thread to be unblocked only when a new event is available. A call of this
     * method removes the pending event from the event queue
     *
     * @param[in,out] event A pointer where a pointer to the event will be stored
     *
     * @return The result of the method call
     * @retval IasEventProvider::eIasOk Event stored in location where the parameter points to
     * @retval IasEventProvider::eIasNoEventAvailable No event in event queue available
     * @retval IasEventProvider::eIasFailed Parameter event is a nullptr
     */
    IasResult getNextEvent(IasEventPtr *event);

    /**
     * @brief Clear the event queue
     */
    void clearEventQueue();

  private:

    /**
     * @brief Constructor.
     */
    IasEventProvider();

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasEventProvider(IasEventProvider const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasEventProvider& operator=(IasEventProvider const &other);

    DltContext               *mLog;           //!< DLT log context
    IasEventQueue             mEventQueue;    //!< The event queue
    std::mutex                mMutex;         //!< Mutex for condition variable
    std::condition_variable   mCondition;     //!< Condition variable to signal new event
    uint32_t               mNrEvents;      //!< The number of events in the queue
};

} //namespace IasAudio

#endif // IASEVENTPROVIDER_HPP
