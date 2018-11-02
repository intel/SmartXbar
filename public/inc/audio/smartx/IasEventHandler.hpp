/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEventHandler.hpp
 * @date   2015
 * @brief
 */

#ifndef IASEVENTHANDLER_HPP
#define IASEVENTHANDLER_HPP


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

class IasConnectionEvent;
class IasSetupEvent;
class IasModuleEvent;

/**
 * @brief The base class for a customized IasEventHandler
 *
 * It is used to dispatch the events received from the event queue. You have to derive from that class
 * and override the virtual methods of the events you are interested in. Then after an event was received via the
 * method IasSmartX::getNextEvent the IasEvent::accept method of the received event has to be called with the
 * customized event handler as parameter.
 */
class IAS_AUDIO_PUBLIC IasEventHandler
{
  public:
    /**
     * @brief Constructor.
     */
    IasEventHandler();
    /**
     * @brief Destructor.
     *
     * Virtual to allow inheritance
     */
    virtual ~IasEventHandler();

    /**
     * @name Dispatch methods
     *
     * The following methods have to be overridden to deal with the several events
     * the SmartXbar produces.
     */
    ///@{
    /**
     * @brief Received connection event
     *
     * This method is called in case the received event was a connection event
     *
     * @param[in] event A pointer to the connection event
     */
    virtual void receivedConnectionEvent(IasConnectionEvent *event);

    /**
     * @brief Received setup event
     *
     * This method is called in case the received event was a setup event
     *
     * @param[in] event A pointer to the setup event
     */
    virtual void receivedSetupEvent(IasSetupEvent *event);

    /**
     * @brief Received module event
     *
     * This method is called in case the received event was a module event
     *
     * @param[in] event A pointer to the module event
     */
    virtual void receivedModuleEvent(IasModuleEvent *event);
    ///@}

  protected:
    DltContext   *mLog;     //!< The dlt log context

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasEventHandler(IasEventHandler const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasEventHandler& operator=(IasEventHandler const &other);
};

} //namespace IasAudio

#endif // IASEVENTHANDLER_HPP
