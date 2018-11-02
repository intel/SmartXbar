/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXClient.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSMARTXCLIENT_HPP
#define IASSMARTXCLIENT_HPP

#include <thread>
#include <atomic>
#include <tbb/concurrent_queue.h>

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "avbaudiomodules/internal/audio/common/alsa_smartx_plugin/IasAlsaPluginShmConnection.hpp"
#include "model/IasAudioDevice.hpp"

namespace IasAudio {

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasSmartXClient
{
  public:
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed,                 //!< Operation failed
    };

    /**
     * @brief Constructor.
     */
    IasSmartXClient(IasAudioDeviceParamsPtr params);
    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasSmartXClient();

    /**
     * @brief The init method for the SmartX client
     *
     * @return The result of the init call
     */
    IasResult init(IasDeviceType deviceType);

    /**
     * @brief Get a handle to the ring buffer used by the SmartX client.
     *
     * @param[out] ringBuffer  Returns the handle to the ring buffer.
     *
     * @return The result of the call
     * @retval IasResult::eIasOk Operation successful
     * @retval IasResult::eIasFailed Operation failed
     */
    IasResult getRingBuffer(IasAudioRingBuffer **ringBuffer);

    /**
     * @brief Enable the event queue
     *
     * This method has to be used to enable or disable the event queue of the audio device. By default the event queue
     * is disabled. The event queue is currently only supported by the SmartXClient to inform about the commands received by
     * the external client. For the AlsaHandler this functionality is currently not used.
     *
     * @param[in] enable If true, the event queue mechanism will be enabled. False will disable the event queue mechanism.
     */
    void enableEventQueue(bool enable);

    /**
     * @brief Get the next entry from the event queue
     *
     * If there is an event type entry in the event queue, it will be removed from the queue and returned as return value.
     * In case the event queue was not enabled (see IasAudioDevice::enableEventQueue) or the device does not support the
     * event queue, the returned values is IasEventType::eIasNoEvent.
     *
     * @return The event type from the event queue.
     */
    IasAudioDevice::IasEventType getNextEventType();

  private:
    /**
     * @brief The device state
     */
    enum IasDeviceState
    {
      eIasStopped,    //!< Device received a stopped event
      eIasStarted,    //!< Device received a started event
    };

    struct IasEventItem
    {
      IasAudioDevice::IasEventType eventType;   //!< The event type
      std::int32_t sessionId;                   //!< The session id for which this event was created
    };

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSmartXClient(IasSmartXClient const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSmartXClient& operator=(IasSmartXClient const &other);

    /**
     * @brief Set the hw constraints that shall be valid for that SmartX client
     */
    void setHwConstraints();

    /**
     * @brief The IPC thread receiving the commands from the alsa-smartx-plugin
     */
    void ipcThread();

    /**
     * @brief Put an event type into the event queue
     *
     * @param[in] eventType The event type to put into the queue
     */
    void putEventType(IasAudioDevice::IasEventType eventType);

    DltContext                                         *mLog;                 //!< Dlt Log Context
    IasAudioDeviceParamsPtr                             mParams;              //!< Pointer to device params
    IasAlsaPluginShmConnection                          mShmConnection;       //!< The shm connection to the alsa-smartx-plugin
    std::thread                                        *mIpcThread;           //!< Pointer to the IPC thread
    std::atomic_bool                                    mIsRunning;           //!< Flag used as exit condition for the IPC thread
    IasAudioIpc                                        *mInIpc;               //!< Pointer to the incoming ipc (from alsa-smartx-plugin)
    IasAudioIpc                                        *mOutIpc;              //!< Pointer to the outgoing ipc (to alsa-smartx-plugin)
    IasDeviceType                                       mDeviceType;          //!< The device type, i.e. playback or capture
    bool                                                mIsEventQueueEnabled; //!< If the event queue is enabled (true) or not (false:default)
    tbb::concurrent_queue<IasEventItem>                 mEventQueue;          //!< The event queue for passing back informations from the communication thread
    IasDeviceState                                      mDeviceState;         //!< The device state
    std::int32_t                                        mSessionId;           //!< The current session id, which is an increasing counter
};

/**
 * @brief Function to get a IasSmartXClient::IasResult as string.
 *
 * @return Enum Member
 */
std::string toString(const IasSmartXClient::IasResult& type);


} //namespace IasAudio

#endif // IASSMARTXCLIENT_HPP
