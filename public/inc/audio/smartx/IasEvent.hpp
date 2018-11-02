/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEvent.hpp
 * @date   2015
 * @brief
 */

#ifndef IASEVENT_HPP
#define IASEVENT_HPP

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

class IasEventHandler;

/**
 * @brief The base class for all events being returned to the application
 */
class IAS_AUDIO_PUBLIC IasEvent
{
  public:
    /**
     * @brief Constructor.
     */
    IasEvent();
    /**
     * @brief Destructor.
     */
    virtual ~IasEvent();

    /**
     * @brief Dispatch method for all derived event classes
     *
     * This method has to be implemented by all derived events and the
     * corresponding method of the IasEventHandler for the derived event
     * class has to be called
     *
     * @param[in] eventHandler A reference to a customer event handler that is used to dispatch the different events
     */
    virtual void accept(IasEventHandler &eventHandler) = 0;

  protected:
    DltContext  *mLog;    //!< A pointer to the log context

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasEvent(IasEvent const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasEvent& operator=(IasEvent const &other);
};

} //namespace IasAudio

#endif // IASEVENT_HPP
