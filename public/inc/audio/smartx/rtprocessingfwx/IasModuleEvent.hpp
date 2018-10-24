/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasModuleEvent.hpp
 * @date 2016
 * @brief
 */

#ifndef IASMODULEEVENT_HPP_
#define IASMODULEEVENT_HPP_

#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasEventHandler.hpp"

namespace IasAudio {

class IAS_AUDIO_PUBLIC IasModuleEvent : public IasEvent
{
  public:
    /**
     * @brief Constructor
     */
    IasModuleEvent()
      :mProperties()
    {}

    /**
     * @brief Destructor
     */
    virtual ~IasModuleEvent() {}

    /**
     * @brief Dispatch method for all derived event classes
     *
     * This method has to be implemented by all derived events and the
     * corresponding method of the IasEventHandler for the derived event
     * class has to be called
     *
     * @param[in] eventHandler A reference to a customer event handler that is used to dispatch the different events
     */
    virtual void accept(IasEventHandler &eventHandler) { eventHandler.receivedModuleEvent(this); }

    /**
     * @brief Getter method for the properties of the event
     *
     * @returns The properties of the event
     */
    const IasProperties& getProperties() const { return mProperties; }

    /**
     * @brief Setter method for the properties of the event
     *
     * @param[in] properties The properties to be set of the event
     */
    void setProperties(const IasProperties &properties) { mProperties = properties; }

  private:
    IasProperties       mProperties;   //!< The properties of the event.
};

} /* namespace IasAudio */

#endif /* IASMODULEEVENT_HPP_ */
